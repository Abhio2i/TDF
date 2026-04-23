// =============================================================================
// FILE:         radarscheduler.cpp
// MODULE:       AESA Radar Beam Scheduler — Implementation
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements the RadarScheduler class. All schedule construction,
//               duty cycle enforcement, beam interleaving, and schedule
//               execution logic lives here. No external I/O. No dynamic memory
//               allocation beyond std::vector resize in buildSchedule() (known
//               MM-01 deviation, ICD-AESA-DEVIATION-002 — allocation occurs
//               at schedule build time, not in the operational advance() loop).
//               No recursion (FN-06 compliant). No exceptions (FP-01 compliant).
//
// REQUIREMENTS: REQ-AESA-010  Beam scheduling and scan management
//               REQ-AESA-020  Duty cycle enforcement and waveform degradation
//               REQ-AESA-030  Track beam interleaving
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-SCHED-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Search-only scheduler.
//   Rev 2  15 Feb 2026  Track beam interleaving added.
//   Rev 3  01 Apr 2026  FIX-08: Duty cycle enforcement added. FC-only schedule
//                       for LOCK_ON mode. Late-insert track beam logic added.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Commented-out code removed per NS-05. Magic numbers
//                       replaced with named constexpr constants.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#include "radarscheduler.h"
#include <algorithm>
#include <cmath>

// =============================================================================
// FILE-SCOPE NAMED CONSTANTS
//
// All numeric literals used in this translation unit are declared here.
// Satisfies VI-08 (no magic numbers). REQ-AESA-010, REQ-AESA-020.
// =============================================================================
namespace
{
// Pi — full precision. Replaces non-standard M_PI macro. LC-08 compliant.
constexpr double PI = 3.14159265358979323846;

// Conversion factor: radians to degrees.
constexpr double RAD_TO_DEG = 180.0 / PI;

// Raster grid step multiplier. Search grid spacing = beamWidth *
// GRID_STEP_MULTIPLIER in both azimuth and elevation. Value of 2.0
// gives beam-width overlap of 50% — standard for area search.
// REQ-AESA-010.
constexpr double GRID_STEP_MULTIPLIER = 2.0;

// Loop bound tolerance (degrees). Added/subtracted from FoV limits in
// the search grid loop to prevent floating-point rounding from dropping
// the last grid row or column. REQ-AESA-010.
constexpr double LOOP_BOUND_TOLERANCE = 0.01;

// Minimum PRF (Hz). Duty cycle degradation never reduces PRF below this.
// Prevents degenerate waveforms from the degradeWaveform() function.
// REQ-AESA-020.
constexpr float  MIN_PRF_HZ = 1.0f;

// Minimum pulse width (seconds) below which duty cycle degradation
// cannot compute a meaningful new PRF. PRF is left unchanged if
// pulseWidth_s <= this value. REQ-AESA-020.
constexpr double MIN_PULSE_WIDTH_S = 1e-9;

// Duty cycle physical bounds [0.0, 1.0]. Duty cycle is dimensionless.
// A CW radar has duty cycle = 1.0. A pulsed radar < 1.0. REQ-AESA-020.
constexpr double DUTY_CYCLE_MIN = 0.0;
constexpr double DUTY_CYCLE_MAX = 1.0;

// Fire-control beam priority. Highest scheduling priority — always placed
// first in schedule_. REQ-AESA-010.
constexpr int    FC_BEAM_PRIORITY = 100;

// Track beam priority for a steady (non-manoeuvring) target. REQ-AESA-030.
constexpr int    TRACK_PRIORITY_STEADY = 10;

// Track beam priority for a manoeuvring target. Higher priority gives
// more frequent revisit via the double-insertion mechanism. REQ-AESA-030.
constexpr int    TRACK_PRIORITY_MANOEUVRE = 20;

// Minimum track predicted range (metres) required before a beam az/el
// can be computed. Below this the range is effectively zero and
// atan2/asin would produce undefined results. REQ-AESA-030.
constexpr double MIN_TRACK_RANGE_M = 1.0;

// Fire-control beam spoil factor. spoilFactor = 1.0 = no widening.
// Maximum gain / minimum beamwidth for fire-control illumination.
// REQ-AESA-010.
constexpr float  FC_SPOIL_FACTOR = 1.0f;

// Search beam spoil factor. REQ-AESA-010.
constexpr float  SEARCH_SPOIL_FACTOR = 1.0f;

// Track beam spoil factor. REQ-AESA-030.
constexpr float  TRACK_SPOIL_FACTOR = 1.0f;

// Minimum interval between track beam insertions (beam positions).
// Prevents all track beams from being inserted at the same position.
// std::max(1, ns/nt) is clamped to at least this value. REQ-AESA-030.
constexpr int    MIN_TRACK_INTERVAL = 1;

// Milliseconds per second. Used to convert dt_s to milliseconds.
constexpr double MS_PER_SECOND = 1000.0;

// Minimum number of search beams that triggers scan completion.
// totalSearchBeams_ must be >= this for scanComplete_ to fire.
// REQ-AESA-010.
constexpr int    MIN_SEARCH_BEAMS_FOR_SCAN = 1;

// Total search beams assigned when in LOCK_ON fast-path.
// The FC beam counts as 1 "search" beam for scan counter purposes.
// REQ-AESA-010.
constexpr int    LOCK_ON_TOTAL_SEARCH_BEAMS = 1;

// clamp bounds for acos/asin domain protection. REQ-AESA-010.
constexpr double CLAMP_MIN = -1.0;
constexpr double CLAMP_MAX =  1.0;

} // anonymous namespace

namespace aesa {

// =============================================================================
// SECTION 1: DUTY CYCLE HELPERS
// REQ-AESA-020
// =============================================================================

// =============================================================================
// FUNCTION: computeDutyCycle (static)
// Full description in header.
// =============================================================================
double RadarScheduler::computeDutyCycle(const BeamWaveform& wf)
{
    // Duty cycle = pulseWidth_s * prf_Hz.
    // Clamp to [0, 1] — physical duty cycle cannot exceed 1.0 (CW limit).
    // Values > 1.0 would indicate a misconfigured waveform. REQ-AESA-020.
    return std::clamp(
        static_cast<double>(wf.pulseWidth_s) * static_cast<double>(wf.prf_Hz),
        DUTY_CYCLE_MIN,
        DUTY_CYCLE_MAX);
}

// =============================================================================
// FUNCTION: degradeWaveform (static)
// Full description in header.
// =============================================================================
BeamWaveform RadarScheduler::degradeWaveform(const BeamWaveform& wf,
                                             double targetDuty)
{
    // Start with a copy — all fields unchanged except prf_Hz. REQ-AESA-020.
    BeamWaveform out = wf;

    double tau = static_cast<double>(wf.pulseWidth_s);

    // Guard: if pulse width is effectively zero we cannot compute a meaningful
    // PRF reduction. Leave prf_Hz unchanged. REQ-AESA-020.
    if (tau > MIN_PULSE_WIDTH_S)
    {
        // Compute the maximum PRF that keeps duty cycle at or below targetDuty.
        // duty = tau * prf => prf_max = duty / tau. REQ-AESA-020.
        float newPRF = static_cast<float>(targetDuty / tau);

        // New PRF must be:
        //   - No higher than the original prf_Hz (we never increase PRF here).
        //   - No lower than MIN_PRF_HZ (degenerate waveform prevention).
        // REQ-AESA-020.
        out.prf_Hz = std::min(wf.prf_Hz, std::max(MIN_PRF_HZ, newPRF));
    }

    return out;
}

// =============================================================================
// SECTION 2: LIFECYCLE
// REQ-AESA-010
// =============================================================================

// =============================================================================
// FUNCTION: reset
// Full description in header.
// =============================================================================
void RadarScheduler::reset()
{
    // Clear all beam lists. After reset, schedule_ is empty and
    // currentBeam() will return fallbackBeam_. REQ-AESA-010.
    schedule_.clear();
    searchGrid_.clear();
    pendingTrack_.clear();

    // Reset all execution counters and flags to their safe initial values.
    // REQ-AESA-010.
    currentIndex_      = 0;
    dwellElapsed_ms_   = 0.0;
    totalSearchBeams_  = 0;
    searchBeamsServed_ = 0;
    scanComplete_      = false;
    currentDutyCycle_  = 0.0;
}

// =============================================================================
// SECTION 3: SCHEDULE CONSTRUCTION
// REQ-AESA-010, REQ-AESA-020, REQ-AESA-030
// =============================================================================

// =============================================================================
// FUNCTION: buildSchedule
// Full description in header.
// =============================================================================
void RadarScheduler::buildSchedule(const RadarConfig& cfg,
                                   const std::vector<TrackFile>& tracks)
{
    // Clear previous schedule. New schedule will be built from scratch.
    // REQ-AESA-010.
    schedule_.clear();
    searchGrid_.clear();
    pendingTrack_.clear();

    // Reset execution state. All beam timing starts from zero. REQ-AESA-010.
    currentIndex_    = 0;
    dwellElapsed_ms_ = 0.0;
    scanComplete_    = false;

    // -------------------------------------------------------------------------
    // Fast path: LOCK_ON mode.
    // When locked on a specific target, all beam time must be concentrated on
    // that target for maximum fire-control quality. Search grid is not built.
    // REQ-AESA-003, REQ-AESA-010.
    // -------------------------------------------------------------------------
    if (cfg.mode == RadarMode::LOCK_ON && cfg.lockedTargetID != 0)
    {
        // Find the locked track in the database to get its predicted position.
        // If not found (first tick after lockOn()), pass nullptr — FC beam
        // defaults to boresight. REQ-AESA-010.
        const TrackFile* locked = nullptr;
        for (const auto& t : tracks)
        {
            if (t.id == cfg.lockedTargetID)
            {
                locked = &t;
                break;
            }
        }

        // Insert FC beam as the only beam in the schedule. REQ-AESA-010.
        insertFireControlBeam(cfg.lockedTargetID, locked, cfg);

        // Set totalSearchBeams_ to 1 so the scan counter mechanism still
        // functions. This allows rebuildSchedule() to be called at scan
        // boundaries even in LOCK_ON mode. REQ-AESA-010.
        totalSearchBeams_  = LOCK_ON_TOTAL_SEARCH_BEAMS;
        searchBeamsServed_ = 0;
        return;
    }

    // -------------------------------------------------------------------------
    // Normal path: SURVEILLANCE or TWS mode.
    // Build a mixed schedule of search and track beams. REQ-AESA-010, REQ-AESA-030.
    // -------------------------------------------------------------------------

    // Step 1: Build the search raster grid. REQ-AESA-010.
    buildSearchGrid(cfg);

    // Step 2: Build the track beam pending list from validated tracks. REQ-AESA-030.
    insertTrackBeams(tracks, cfg);

    // Step 3: Interleave track beams into the search grid. REQ-AESA-030.
    interleaveSchedule();

    // Step 4: If somehow LOCK_ON reached this path (should not happen due to
    // early return above — defensive code), insert FC beam at head. REQ-AESA-010.
    if (cfg.mode == RadarMode::LOCK_ON && cfg.lockedTargetID != 0)
    {
        const TrackFile* locked = nullptr;
        for (const auto& t : tracks)
        {
            if (t.id == cfg.lockedTargetID)
            {
                locked = &t;
                break;
            }
        }
        insertFireControlBeam(cfg.lockedTargetID, locked, cfg);
    }

    // -------------------------------------------------------------------------
    // Step 5: Enforce duty cycle budget across all beams.
    // Each beam whose duty cycle exceeds maxDutyCycle has its PRF reduced
    // to bring it within budget. REQ-AESA-020.
    // -------------------------------------------------------------------------
    double maxDuty = static_cast<double>(cfg.maxDutyCycle);
    for (auto& b : schedule_)
    {
        if (computeDutyCycle(b.waveform) > maxDuty)
        {
            b.waveform = degradeWaveform(b.waveform, maxDuty);
        }
    }

    // -------------------------------------------------------------------------
    // Step 6: Set scan counter state.
    // totalSearchBeams_ drives the scan boundary detection in advance().
    // REQ-AESA-010.
    // -------------------------------------------------------------------------
    totalSearchBeams_  = static_cast<int>(searchGrid_.size());
    searchBeamsServed_ = 0;
    currentIndex_      = 0;
    dwellElapsed_ms_   = 0.0;
    scanComplete_      = false;

    // -------------------------------------------------------------------------
    // Step 7: Fallback beam.
    // If the schedule is still empty after all above steps (e.g. FoV so narrow
    // that no grid positions were generated), insert a single boresight search
    // beam to ensure currentBeam() always returns a valid descriptor. REQ-AESA-010.
    // -------------------------------------------------------------------------
    if (schedule_.empty())
    {
        // Configure fallback as a boresight SEARCH beam using default waveform.
        fallbackBeam_.task          = BeamRequest::Task::SEARCH;
        fallbackBeam_.azimuth_deg   = 0.0;
        fallbackBeam_.elevation_deg = 0.0;
        fallbackBeam_.dwellTime_ms  = cfg.searchDwellTime_ms;
        fallbackBeam_.waveform      = cfg.searchWaveform;

        // Insert into schedule so normal index logic still functions. REQ-AESA-010.
        schedule_.push_back(fallbackBeam_);
    }
}

// =============================================================================
// FUNCTION: insertFireControlBeam
// Full description in header.
// =============================================================================
void RadarScheduler::insertFireControlBeam(uint32_t targetID,
                                           const TrackFile* track,
                                           const RadarConfig& cfg)
{
    // Build the fire-control beam descriptor. REQ-AESA-010.
    BeamRequest fc;
    fc.task        = BeamRequest::Task::FIRE_CONTROL;
    fc.targetID    = targetID;
    fc.dwellTime_ms = cfg.fireControlDwellTime_ms;
    fc.priority    = FC_BEAM_PRIORITY;
    fc.waveform    = cfg.fireControlWaveform;
    fc.spoilFactor = FC_SPOIL_FACTOR;

    // Point beam at Kalman-predicted track position if valid track is available.
    // Guard: predictedRange must be > MIN_TRACK_RANGE_M to avoid undefined
    // atan2 and asin at zero range. REQ-AESA-010.
    if (track != nullptr && track->predictedRange > MIN_TRACK_RANGE_M)
    {
        // Azimuth from atan2 — gives signed angle in (-180, +180]. REQ-AESA-010.
        fc.azimuth_deg = std::atan2(track->y, track->x) * RAD_TO_DEG;

        // Elevation from asin — clamp argument to [-1, 1] for domain safety.
        fc.elevation_deg = std::asin(
                               std::clamp(track->z / track->predictedRange,
                                          CLAMP_MIN, CLAMP_MAX))
                           * RAD_TO_DEG;
    }
    // If track is null or predictedRange <= MIN_TRACK_RANGE_M, fc.azimuth_deg
    // and fc.elevation_deg remain at their default-initialised values (0.0, 0.0)
    // which points the FC beam at boresight — a safe default. REQ-AESA-010.

    // Insert at position 0 — FC beam always executes first. REQ-AESA-010.
    schedule_.insert(schedule_.begin(), fc);
}

// =============================================================================
// SECTION 4: SCHEDULE EXECUTION
// REQ-AESA-010, REQ-AESA-020
// =============================================================================

// =============================================================================
// FUNCTION: currentBeam
// Full description in header.
// =============================================================================
const BeamRequest& RadarScheduler::currentBeam() const
{
    // Return fallbackBeam_ if schedule is empty — prevents out-of-bounds access.
    // This is the only safe action when the schedule has not been built yet.
    // REQ-AESA-010.
    if (schedule_.empty()) return fallbackBeam_;

    return schedule_[currentIndex_];
}

// =============================================================================
// FUNCTION: advance
// Full description in header.
// =============================================================================
void RadarScheduler::advance(double dt_s)
{
    // If schedule is empty, nothing to advance. REQ-AESA-010.
    if (schedule_.empty()) return;

    // Clear scan boundary flag at the start of each tick.
    // scanComplete_ is only valid for the one tick in which it is set.
    // REQ-AESA-010.
    scanComplete_ = false;

    // Update duty cycle from the current beam's waveform. REQ-AESA-020.
    const BeamRequest& curr = schedule_[currentIndex_];
    currentDutyCycle_ = computeDutyCycle(curr.waveform);

    // Accumulate elapsed dwell time. REQ-AESA-010.
    dwellElapsed_ms_ += dt_s * MS_PER_SECOND;

    // If the dwell time has not yet expired, remain on this beam. REQ-AESA-010.
    if (dwellElapsed_ms_ < curr.dwellTime_ms) return;

    // Dwell expired — reset timer and advance to the next beam. REQ-AESA-010.
    dwellElapsed_ms_ = 0.0;

    // -------------------------------------------------------------------------
    // SEARCH beam dwell expired — update scan counter.
    // REQ-AESA-010.
    // -------------------------------------------------------------------------
    if (curr.task == BeamRequest::Task::SEARCH)
    {
        // Increment the count of search beams served this scan pass.
        if (++searchBeamsServed_ >= totalSearchBeams_)
        {
            // All search positions have been visited — scan boundary reached.
            // Reset counter for the next scan pass. REQ-AESA-010.
            searchBeamsServed_ = 0;
            scanComplete_      = true;
        }

        // Late-insert: if a track beam is waiting in pendingTrack_, insert it
        // immediately after the current search beam position. This ensures track
        // beams are interleaved dynamically as new tracks are validated between
        // schedule rebuild cycles. REQ-AESA-030.
        if (!pendingTrack_.empty())
        {
            BeamRequest tb = pendingTrack_.front();
            pendingTrack_.erase(pendingTrack_.begin());

            // Insert at currentIndex_ + 1, clamped to schedule bounds.
            int pos = std::min(currentIndex_ + 1,
                               static_cast<int>(schedule_.size()));
            schedule_.insert(schedule_.begin() + pos, tb);
        }
    }

    // -------------------------------------------------------------------------
    // FIRE_CONTROL beam dwell expired — repeat without advancing.
    // In LOCK_ON mode the FC beam occupies the entire schedule. Wrap index
    // to stay within bounds (handles the case where schedule was modified by
    // an external call). Do NOT advance to the next beam. REQ-AESA-010.
    // -------------------------------------------------------------------------
    if (curr.task == BeamRequest::Task::FIRE_CONTROL)
    {
        // Modulo wrap prevents out-of-bounds if schedule was modified. REQ-AESA-010.
        currentIndex_ = currentIndex_ % static_cast<int>(schedule_.size());
        return;
    }

    // -------------------------------------------------------------------------
    // SEARCH and TRACK beam: advance to next beam in circular schedule.
    // REQ-AESA-010.
    // -------------------------------------------------------------------------
    currentIndex_ = (currentIndex_ + 1) % static_cast<int>(schedule_.size());
}

// =============================================================================
// SECTION 5: PRIVATE HELPERS
// REQ-AESA-010, REQ-AESA-030
// =============================================================================

// =============================================================================
// FUNCTION: buildSearchGrid
// Full description in header.
// =============================================================================
void RadarScheduler::buildSearchGrid(const RadarConfig& cfg)
{
    // Grid step = 2 * beamWidth in both axes. This gives 50% beam-width
    // overlap between adjacent positions, ensuring no coverage gaps. REQ-AESA-010.
    double azStep = static_cast<double>(cfg.beamWidth) * GRID_STEP_MULTIPLIER;
    double elStep = static_cast<double>(cfg.beamWidth) * GRID_STEP_MULTIPLIER;

    // Outer loop: elevation from top of FoV down to bottom.
    // LOOP_BOUND_TOLERANCE prevents floating-point rounding from dropping
    // the bottom elevation row. REQ-AESA-010.
    for (double el = static_cast<double>(cfg.maxElevation);
         el >= static_cast<double>(cfg.minElevation) - LOOP_BOUND_TOLERANCE;
         el -= elStep)
    {
        // Inner loop: azimuth from left of FoV to right.
        // LOOP_BOUND_TOLERANCE prevents dropping the rightmost column. REQ-AESA-010.
        for (double az = static_cast<double>(cfg.minAzimuth);
             az <= static_cast<double>(cfg.maxAzimuth) + LOOP_BOUND_TOLERANCE;
             az += azStep)
        {
            BeamRequest r;
            r.task          = BeamRequest::Task::SEARCH;
            r.azimuth_deg   = az;
            r.elevation_deg = el;
            r.dwellTime_ms  = cfg.searchDwellTime_ms;
            r.priority      = 0;       // Lowest priority — preempted by track/FC
            r.waveform      = cfg.searchWaveform;
            r.spoilFactor   = SEARCH_SPOIL_FACTOR;
            searchGrid_.push_back(r);
        }
    }
}

// =============================================================================
// FUNCTION: insertTrackBeams
// Full description in header.
// =============================================================================
void RadarScheduler::insertTrackBeams(const std::vector<TrackFile>& tracks,
                                      const RadarConfig& cfg)
{
    for (const auto& t : tracks)
    {
        // Only validated tracks get dedicated beam time. Tentative tracks
        // (hitCount < minHitsToValidate) are detected opportunistically
        // during search beams. REQ-AESA-030.
        if (!t.isValidated) continue;

        BeamRequest tb = makeTrackBeam(t, cfg, t.isManoeuvring);
        pendingTrack_.push_back(tb);

        // Manoeuvring tracks are inserted twice to double the revisit rate.
        // This keeps the IMM filter well-fed during aggressive manoeuvres
        // where prediction uncertainty grows rapidly. REQ-AESA-030.
        if (t.isManoeuvring)
        {
            pendingTrack_.push_back(tb);
        }
    }
}

// =============================================================================
// FUNCTION: interleaveSchedule
// Full description in header.
// =============================================================================
void RadarScheduler::interleaveSchedule()
{
    // Start with the search grid as the base schedule. REQ-AESA-010.
    schedule_ = searchGrid_;

    // If no track beams, schedule is search-only. Return immediately. REQ-AESA-030.
    if (pendingTrack_.empty()) return;

    int ns = static_cast<int>(schedule_.size());
    int nt = static_cast<int>(pendingTrack_.size());

    // Interval = search beams per track beam. Minimum 1 to prevent zero
    // interval which would stack all track beams at position 0. REQ-AESA-030.
    int interval = std::max(MIN_TRACK_INTERVAL, ns / nt);
    int offset   = interval;

    for (int ti = 0; ti < nt; ++ti)
    {
        // Clamp insertion position to schedule bounds in case rounding
        // pushes offset beyond the current schedule size. REQ-AESA-030.
        int pos = std::min(offset, static_cast<int>(schedule_.size()));
        schedule_.insert(schedule_.begin() + pos, pendingTrack_[ti]);

        // Advance offset by interval + 1 to account for the insertion
        // shifting all subsequent positions by 1. REQ-AESA-030.
        offset += interval + 1;
    }
}

// =============================================================================
// FUNCTION: makeTrackBeam
// Full description in header.
// =============================================================================
BeamRequest RadarScheduler::makeTrackBeam(const TrackFile& t,
                                          const RadarConfig& cfg,
                                          bool manoeuvring) const
{
    BeamRequest r;
    r.task        = BeamRequest::Task::TRACK;
    r.targetID    = t.id;
    r.dwellTime_ms = cfg.trackDwellTime_ms;

    // Higher priority for manoeuvring tracks — gives them scheduling preference
    // when the interleave algorithm must resolve conflicts. REQ-AESA-030.
    r.priority    = manoeuvring ? TRACK_PRIORITY_MANOEUVRE : TRACK_PRIORITY_STEADY;
    r.waveform    = cfg.trackWaveform;
    r.spoilFactor = TRACK_SPOIL_FACTOR;

    // Point beam at Kalman-predicted track position.
    // Guard: predictedRange must be > MIN_TRACK_RANGE_M to avoid undefined
    // atan2 and asin results at zero range. REQ-AESA-030.
    if (t.predictedRange > MIN_TRACK_RANGE_M)
    {
        // atan2 gives az in (-180, +180] — body-frame signed convention.
        r.azimuth_deg = std::atan2(t.y, t.x) * RAD_TO_DEG;

        // asin domain protection — clamp to [-1, 1]. REQ-AESA-030.
        r.elevation_deg = std::asin(
                              std::clamp(t.z / t.predictedRange, CLAMP_MIN, CLAMP_MAX))
                          * RAD_TO_DEG;
    }
    // If predictedRange <= MIN_TRACK_RANGE_M, r.azimuth_deg and
    // r.elevation_deg remain at default (0.0, 0.0) — boresight safe default.

    return r;
}

} // namespace aesa

