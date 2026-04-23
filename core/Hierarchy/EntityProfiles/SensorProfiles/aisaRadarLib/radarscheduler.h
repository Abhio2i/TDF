// =============================================================================
// FILE:         radarscheduler.h
// MODULE:       AESA Radar Beam Scheduler
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Declares the RadarScheduler class which manages the time-
//               multiplexed beam schedule for the AESA radar. Responsibilities:
//
//               1. Build a search grid covering the configured FoV (azimuth
//                  and elevation raster at 2 * beamWidth spacing).
//               2. Interleave track maintenance beams into the search grid
//                  at a rate proportional to the number of validated tracks.
//               3. Insert a high-priority fire-control beam at the head of
//                  the schedule when in LOCK_ON mode.
//               4. Enforce a maximum duty cycle budget across all beams —
//                  degrade PRF if any beam would exceed cfg.maxDutyCycle.
//               5. Advance the schedule each tick and report scan boundary
//                  events when the beam completes one full search grid pass.
//               6. Expose the current beam descriptor and duty cycle for
//                  consumption by RadarModel_AESA::update().
//
// REQUIREMENTS: REQ-AESA-010  Beam scheduling and scan management
//               REQ-AESA-020  Duty cycle enforcement
//               REQ-AESA-030  Track beam interleaving
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-SCHED-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Search-only scheduler.
//   Rev 2  15 Feb 2026  Track beam interleaving added. REQ-AESA-030.
//   Rev 3  01 Apr 2026  FIX-08: Duty cycle enforcement added. Fire-control
//                       only schedule for LOCK_ON mode. REQ-AESA-020.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Commented-out code removed per NS-05. Magic numbers
//                       replaced with named constexpr constants.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#pragma once
#ifndef RADARSCHEDULER_H
#define RADARSCHEDULER_H

#include "radarmodel_aesa.h"
#include <vector>

namespace aesa {

// =============================================================================
// CLASS: RadarScheduler
//
// DESCRIPTION:  Manages the time-multiplexed beam dwell schedule for an AESA
//               radar. Generates a static schedule at build time (buildSchedule)
//               and advances through it at runtime (advance). Exposes the
//               current beam descriptor each tick for use by the detection
//               pipeline.
//
//               The scheduler maintains three internal beam lists:
//                 schedule_     — the active execution sequence (mix of search,
//                                 track, and fire-control beams).
//                 searchGrid_   — the full raster of search beam positions.
//                 pendingTrack_ — track beams waiting to be interleaved.
//
//               A fallbackBeam_ is returned by currentBeam() whenever
//               schedule_ is empty — this prevents null-pointer dereference
//               and ensures a safe default beam is always available.
//
// THREAD SAFETY: Not thread-safe. All access is serialised by the mutex in
//               RadarModel_AESA. Do not call from multiple threads without
//               external synchronisation.
//
// REQUIREMENTS: REQ-AESA-010, REQ-AESA-020, REQ-AESA-030
//
// TRACEABILITY:
//   Test suite:  radarScheduler_test (radarscheduler_test.cpp)
//   Test cases:  TC-AESA-SCHED-001 through TC-AESA-SCHED-030
// =============================================================================
class RadarScheduler
{
public:

    // =========================================================================
    // CONSTRUCTOR: RadarScheduler (default)
    //
    // DESCRIPTION: All member variables initialised by in-class initialisers.
    //              No resources acquired. Call reset() then buildSchedule()
    //              before operational use. REQ-AESA-010.
    // =========================================================================
    RadarScheduler() = default;

    // =========================================================================
    // FUNCTION:    reset
    //
    // DESCRIPTION: Clears all schedule state and resets all counters to their
    //              safe default values. Must be called during radar init() and
    //              end() to guarantee a clean scheduler state before the next
    //              buildSchedule() call.
    //
    // REQUIREMENT: REQ-AESA-010
    //
    // PARAMETERS:  None.
    // RETURNS:     void
    //
    // SIDE EFFECTS: Clears schedule_, searchGrid_, pendingTrack_.
    //               Resets currentIndex_, dwellElapsed_ms_, totalSearchBeams_,
    //               searchBeamsServed_, scanComplete_, currentDutyCycle_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-SCHED-001  reset() clears schedule
    //               TC-AESA-SCHED-002  reset() resets all counters to 0
    // =========================================================================
    void reset();

    // =========================================================================
    // FUNCTION:    buildSchedule
    //
    // DESCRIPTION: Constructs the beam execution schedule from the supplied
    //              radar configuration and current track database.
    //
    //              Algorithm (normal TWS / SURVEILLANCE mode):
    //                1. Build search grid (raster of az/el positions at
    //                   2 * beamWidth spacing within configured FoV).
    //                2. Build pending track beam list from validated tracks.
    //                   Manoeuvring tracks are inserted twice (double rate).
    //                3. Interleave track beams into search grid at uniform
    //                   intervals (ns / nt spacing).
    //                4. If mode is LOCK_ON and a locked target exists, insert
    //                   a fire-control beam at the head of the schedule.
    //                5. Enforce duty cycle budget — degrade PRF of any beam
    //                   that would exceed cfg.maxDutyCycle.
    //                6. If schedule is empty after all steps, insert the
    //                   fallback search beam at boresight.
    //
    //              Fast path (LOCK_ON mode):
    //                If cfg.mode == LOCK_ON and cfg.lockedTargetID != 0,
    //                skip the search grid entirely and build a schedule
    //                containing only the fire-control beam. This concentrates
    //                all beam time on the locked target for maximum fire-control
    //                quality. REQ-AESA-003, REQ-AESA-010.
    //
    // REQUIREMENT: REQ-AESA-010  Schedule construction
    //              REQ-AESA-020  Duty cycle enforcement
    //              REQ-AESA-030  Track beam interleaving
    //
    // PARAMETERS:
    //   cfg     [in]  Radar configuration. Key fields used:
    //                   cfg.mode                 — LOCK_ON triggers fast path
    //                   cfg.lockedTargetID       — target for FC beam
    //                   cfg.beamWidth            — raster step = 2 * beamWidth
    //                   cfg.minAzimuth/maxAzimuth — azimuth FoV bounds
    //                   cfg.minElevation/maxElevation — elevation FoV bounds
    //                   cfg.searchDwellTime_ms   — dwell for SEARCH beams
    //                   cfg.trackDwellTime_ms    — dwell for TRACK beams
    //                   cfg.fireControlDwellTime_ms — dwell for FC beam
    //                   cfg.maxDutyCycle         — duty budget (0.0–1.0)
    //                   cfg.searchWaveform       — waveform for SEARCH beams
    //                   cfg.trackWaveform        — waveform for TRACK beams
    //                   cfg.fireControlWaveform  — waveform for FC beam
    //
    //   tracks  [in]  Current tracker database. Only validated tracks
    //                 (isValidated == true) are included in the schedule.
    //                 May be empty — produces a search-only schedule.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Clears and rebuilds schedule_, searchGrid_, pendingTrack_.
    //               Resets currentIndex_, dwellElapsed_ms_, scanComplete_.
    //               Sets totalSearchBeams_, searchBeamsServed_.
    //
    // ASSUMPTIONS: cfg.beamWidth > 0. If beamWidth = 0, the search grid
    //              loop would be infinite. Caller must validate. REQ-AESA-010.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-SCHED-003  buildSchedule() SURVEILLANCE produces
    //                                  non-empty schedule
    //               TC-AESA-SCHED-004  buildSchedule() LOCK_ON produces
    //                                  single FC beam schedule
    //               TC-AESA-SCHED-005  buildSchedule() with tracks inserts
    //                                  track beams
    //               TC-AESA-SCHED-006  Duty cycle budget enforced —
    //                                  no beam exceeds maxDutyCycle
    //               TC-AESA-SCHED-007  Empty schedule produces fallback beam
    // =========================================================================
    void buildSchedule(const RadarConfig& cfg,
                       const std::vector<TrackFile>& tracks);

    // =========================================================================
    // FUNCTION:    insertFireControlBeam
    //
    // DESCRIPTION: Creates a FIRE_CONTROL beam descriptor and inserts it at
    //              the head of schedule_ (position 0). The FC beam has the
    //              highest priority (100) and uses the fire-control waveform.
    //              If a valid track is supplied, the beam is pointed at the
    //              Kalman-predicted track position. Otherwise it defaults to
    //              boresight (az=0, el=0).
    //
    // REQUIREMENT: REQ-AESA-003  Fire-control beam placement
    //              REQ-AESA-010  Beam schedule management
    //
    // PARAMETERS:
    //   targetID  [in]  ID of the target to illuminate. Stored in fc.targetID.
    //                   Must be non-zero for a meaningful FC beam.
    //
    //   track     [in]  Pointer to the track file for the locked target.
    //                   May be nullptr if the track is not yet in the database
    //                   (first tick after lockOn()). When nullptr, beam is
    //                   pointed at boresight. REQ-AESA-010.
    //
    //   cfg       [in]  Radar configuration. Uses fireControlDwellTime_ms,
    //                   fireControlWaveform.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Inserts one BeamRequest at schedule_.begin().
    //               All existing schedule entries shift forward by one index.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-SCHED-008  FC beam is at index 0 after insert
    //               TC-AESA-SCHED-009  FC beam has task FIRE_CONTROL
    //               TC-AESA-SCHED-010  FC beam priority is 100
    //               TC-AESA-SCHED-011  FC beam pointed at track position
    //                                  when valid track supplied
    //               TC-AESA-SCHED-012  FC beam at boresight when track = nullptr
    // =========================================================================
    void insertFireControlBeam(uint32_t targetID,
                               const TrackFile* track,
                               const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    currentBeam
    //
    // DESCRIPTION: Returns a const reference to the beam descriptor at the
    //              current schedule index. This is the beam that the antenna
    //              must point at and that the detection pipeline uses for
    //              waveform parameters this tick.
    //              Returns fallbackBeam_ if schedule_ is empty — guaranteed
    //              to never return a dangling reference. REQ-AESA-010.
    //
    // REQUIREMENT: REQ-AESA-010
    //
    // PARAMETERS:  None.
    //
    // RETURNS:    Const reference to current BeamRequest.
    //             Lifetime: valid until the next call to advance() or
    //             buildSchedule() which may modify schedule_.
    //             Do NOT store this reference across ticks. REQ-AESA-010.
    //
    // SIDE EFFECTS: None. Pure const query.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-SCHED-013  currentBeam() does not crash on
    //                                  empty schedule (returns fallback)
    //               TC-AESA-SCHED-014  currentBeam() returns SEARCH beam
    //                                  as first beam after buildSchedule()
    // =========================================================================
    const BeamRequest& currentBeam() const;

    // =========================================================================
    // FUNCTION:    advance
    //
    // DESCRIPTION: Advances the beam schedule by dt_s seconds. Accumulates
    //              elapsed dwell time. When the current dwell time is exhausted:
    //                - For SEARCH beams: increments searchBeamsServed_. If
    //                  all search beams have been served, sets scanComplete_ =
    //                  true and resets the search counter. If a pending track
    //                  beam is available, inserts it immediately after the
    //                  current position.
    //                - For FIRE_CONTROL beams: wraps currentIndex_ within
    //                  schedule_ bounds and returns — the FC beam repeats
    //                  continuously without advancing to a search beam.
    //                - For TRACK beams: advances to the next beam in the
    //                  circular schedule (currentIndex_ + 1 mod size).
    //
    //              scanComplete_ is cleared at the start of each advance()
    //              call — it is only valid for one tick. REQ-AESA-010.
    //
    // REQUIREMENT: REQ-AESA-010  Schedule execution and scan boundary detection
    //              REQ-AESA-020  Duty cycle tracking
    //
    // PARAMETERS:
    //   dt_s  [in]  Elapsed time since last advance() call (seconds).
    //               Valid range: (0.0, 1.0]. Values outside this range
    //               produce valid but potentially inaccurate dwell timing.
    //               Caller is responsible for clamping. REQ-AESA-010.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Updates dwellElapsed_ms_, currentIndex_,
    //               searchBeamsServed_, scanComplete_, currentDutyCycle_.
    //               May insert track beams into schedule_ from pendingTrack_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-SCHED-015  advance() does not crash on empty schedule
    //               TC-AESA-SCHED-016  advance() clears scanComplete_ each call
    //               TC-AESA-SCHED-017  scanCompleted() true after all search
    //                                  beams served
    //               TC-AESA-SCHED-018  FC beam does not advance past itself
    //               TC-AESA-SCHED-019  TRACK beam advances to next index
    // =========================================================================
    void advance(double dt_s);

    // =========================================================================
    // ACCESSORS — read-only state queries
    // All return by value. No side effects. REQ-AESA-010, REQ-AESA-020.
    // =========================================================================

    // =========================================================================
    // FUNCTION:    scanCompleted
    // DESCRIPTION: Returns true if the scheduler completed one full pass
    //              through all search grid positions in the most recent
    //              advance() call. Valid for exactly one tick after set.
    //              Consumed by RadarModel_AESA::update() to trigger output
    //              assembly and schedule rebuild. REQ-AESA-010.
    // TRACEABILITY: TC-AESA-SCHED-017
    // =========================================================================
    bool   scanCompleted()    const { return scanComplete_;      }

    // =========================================================================
    // FUNCTION:    searchGridSize
    // DESCRIPTION: Returns the total number of search beam positions in the
    //              current search grid. Equals totalSearchBeams_ set during
    //              buildSchedule(). REQ-AESA-010.
    // TRACEABILITY: TC-AESA-SCHED-020
    // =========================================================================
    int    searchGridSize()   const { return totalSearchBeams_;  }

    // =========================================================================
    // FUNCTION:    currentIndex
    // DESCRIPTION: Returns the index into schedule_ of the current beam.
    //              Range: [0, schedule_.size()). Used for diagnostics and
    //              test verification only. REQ-AESA-010.
    // TRACEABILITY: TC-AESA-SCHED-014
    // =========================================================================
    int    currentIndex()     const { return currentIndex_;      }

    // =========================================================================
    // FUNCTION:    scheduleSize
    // DESCRIPTION: Returns the total number of beams in the current schedule.
    //              Includes search, track, and fire-control beams. REQ-AESA-010.
    // TRACEABILITY: TC-AESA-SCHED-003
    // =========================================================================
    int    scheduleSize()     const { return static_cast<int>(schedule_.size()); }

    // =========================================================================
    // FUNCTION:    dwellElapsed_ms
    // DESCRIPTION: Returns the time elapsed in the current beam dwell (ms).
    //              Range: [0.0, current beam dwellTime_ms). Used for diagnostics
    //              and test verification. REQ-AESA-010.
    // =========================================================================
    double dwellElapsed_ms()  const { return dwellElapsed_ms_;   }

    // =========================================================================
    // FUNCTION:    currentDutyCycle
    // DESCRIPTION: Returns the duty cycle of the current beam's waveform
    //              (dimensionless, [0.0, 1.0]). Computed as
    //              pulseWidth_s * prf_Hz, clamped to [0, 1]. Published
    //              to the output and emitted via AESARadar::schedulerDutyCycle
    //              signal each tick. REQ-AESA-020.
    // TRACEABILITY: TC-AESA-SCHED-021
    // =========================================================================
    double currentDutyCycle() const { return currentDutyCycle_;  }

private:

    // =========================================================================
    // PRIVATE HELPER METHODS
    // All called from buildSchedule() or advance().
    // Do NOT call from outside this class.
    // =========================================================================

    // =========================================================================
    // FUNCTION:    buildSearchGrid
    //
    // DESCRIPTION: Populates searchGrid_ with a raster of SEARCH beam positions
    //              covering the configured azimuth and elevation FoV.
    //              Grid spacing = 2 * cfg.beamWidth in both axes.
    //              Elevation is swept outer loop (top to bottom),
    //              azimuth is swept inner loop (left to right).
    //              0.01 degree tolerances on loop bounds prevent floating-point
    //              rounding from dropping the last row or column. REQ-AESA-010.
    //
    // REQUIREMENT: REQ-AESA-010
    //
    // PARAMETERS:
    //   cfg  [in]  Radar config. Uses beamWidth, minAzimuth, maxAzimuth,
    //              minElevation, maxElevation, searchDwellTime_ms,
    //              searchWaveform.
    //
    // SIDE EFFECTS: Populates searchGrid_. Does NOT modify schedule_.
    //
    // TRACEABILITY: TC-AESA-SCHED-022  Grid covers full FoV
    //               TC-AESA-SCHED-023  Grid spacing is 2 * beamWidth
    // =========================================================================
    void buildSearchGrid(const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    insertTrackBeams
    //
    // DESCRIPTION: Populates pendingTrack_ with TRACK beam descriptors for
    //              all validated tracks in the tracker database. Non-validated
    //              (tentative) tracks are excluded — they do not yet have
    //              enough hits to warrant dedicated beam time. Manoeuvring
    //              tracks are inserted twice to give double the update rate,
    //              which improves IMM filter performance during manoeuvres.
    //              REQ-AESA-030.
    //
    // REQUIREMENT: REQ-AESA-030
    //
    // PARAMETERS:
    //   tracks  [in]  Tracker database. Only entries with isValidated = true
    //                 are processed.
    //   cfg     [in]  Uses trackDwellTime_ms, trackWaveform. REQ-AESA-030.
    //
    // SIDE EFFECTS: Populates pendingTrack_. Does NOT modify schedule_.
    //
    // TRACEABILITY: TC-AESA-SCHED-005  Validated tracks produce track beams
    //               TC-AESA-SCHED-024  Manoeuvring track produces two beams
    //               TC-AESA-SCHED-025  Non-validated track produces no beam
    // =========================================================================
    void insertTrackBeams(const std::vector<TrackFile>& tracks,
                          const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    interleaveSchedule
    //
    // DESCRIPTION: Copies searchGrid_ into schedule_ and inserts track beams
    //              from pendingTrack_ at uniform intervals. Track beam
    //              insertion interval = max(1, ns / nt) where ns = number of
    //              search beams and nt = number of track beams. This ensures
    //              track beams are evenly distributed across the scan, giving
    //              each tracked target approximately equal revisit rate.
    //              Returns immediately if pendingTrack_ is empty.
    //              REQ-AESA-030.
    //
    // REQUIREMENT: REQ-AESA-030
    //
    // PARAMETERS:  None. Uses schedule_, searchGrid_, pendingTrack_.
    //
    // SIDE EFFECTS: Sets schedule_ = searchGrid_ + interleaved track beams.
    //
    // TRACEABILITY: TC-AESA-SCHED-026  Track beams uniformly distributed
    //               TC-AESA-SCHED-027  Schedule size = search + track count
    // =========================================================================
    void interleaveSchedule();

    // =========================================================================
    // FUNCTION:    makeTrackBeam
    //
    // DESCRIPTION: Creates a TRACK beam descriptor for the given track file.
    //              Points the beam at the Kalman-predicted track position
    //              (from predictedRange, x, y, z). Manoeuvring tracks get
    //              higher priority (20 vs 10). REQ-AESA-030.
    //
    // REQUIREMENT: REQ-AESA-030
    //
    // PARAMETERS:
    //   t            [in]  Track file. Uses id, predictedRange, x, y, z,
    //                      isManoeuvring.
    //   cfg          [in]  Uses trackDwellTime_ms, trackWaveform.
    //   manoeuvring  [in]  If true, priority = 20. If false, priority = 10.
    //
    // RETURNS:    BeamRequest with task = TRACK, pointed at track position.
    //
    // SIDE EFFECTS: None. Pure construction.
    //
    // TRACEABILITY: TC-AESA-SCHED-028  Track beam has task TRACK
    //               TC-AESA-SCHED-029  Track beam priority 20 for manoeuvring
    //               TC-AESA-SCHED-030  Track beam priority 10 for steady
    // =========================================================================
    BeamRequest makeTrackBeam(const TrackFile& track,
                              const RadarConfig& cfg,
                              bool manoeuvring) const;

    // =========================================================================
    // FUNCTION:    computeDutyCycle (static)
    //
    // DESCRIPTION: Computes the instantaneous duty cycle of a waveform as
    //              pulseWidth_s * prf_Hz, clamped to [0.0, 1.0].
    //              A duty cycle > 1.0 is physically impossible — clamping
    //              guards against misconfigured waveforms. REQ-AESA-020.
    //
    // REQUIREMENT: REQ-AESA-020
    //
    // PARAMETERS:
    //   wf  [in]  Waveform descriptor. Uses pulseWidth_s, prf_Hz.
    //
    // RETURNS:    Duty cycle in [0.0, 1.0] (dimensionless).
    //
    // SIDE EFFECTS: None. Pure static computation.
    //
    // TRACEABILITY: TC-AESA-SCHED-021  computeDutyCycle correct for known values
    // =========================================================================
    static double computeDutyCycle(const BeamWaveform& wf);

    // =========================================================================
    // FUNCTION:    degradeWaveform (static)
    //
    // DESCRIPTION: Returns a copy of wf with its prf_Hz reduced so that the
    //              resulting duty cycle does not exceed targetDuty.
    //              newPRF = targetDuty / pulseWidth_s, then clamped to
    //              [1.0, wf.prf_Hz] so PRF is never increased above the
    //              original value and never falls below 1 Hz. REQ-AESA-020.
    //
    // REQUIREMENT: REQ-AESA-020
    //
    // PARAMETERS:
    //   wf          [in]  Original waveform. Fields other than prf_Hz
    //                     are copied unchanged.
    //   targetDuty  [in]  Maximum permissible duty cycle (0.0–1.0).
    //
    // RETURNS:    Modified BeamWaveform with reduced prf_Hz.
    //
    // SIDE EFFECTS: None. Pure static computation.
    //
    // TRACEABILITY: TC-AESA-SCHED-006  degradeWaveform reduces PRF to meet budget
    // =========================================================================
    static BeamWaveform degradeWaveform(const BeamWaveform& wf,
                                        double targetDuty);

    // =========================================================================
    // PRIVATE MEMBER VARIABLES
    // =========================================================================

    // Active beam execution sequence. Mix of SEARCH, TRACK, FIRE_CONTROL beams.
    // Built by buildSchedule(), advanced by advance(). REQ-AESA-010.
    std::vector<BeamRequest> schedule_;

    // Full raster of search beam positions covering the configured FoV.
    // Built by buildSearchGrid(). Copied into schedule_ by interleaveSchedule().
    // Retained after build for searchGridSize() query. REQ-AESA-010.
    std::vector<BeamRequest> searchGrid_;

    // Queue of track beams waiting to be interleaved into the schedule.
    // Built by insertTrackBeams(). Consumed by interleaveSchedule() and
    // by advance() for late-insert of urgent track beams. REQ-AESA-030.
    std::vector<BeamRequest> pendingTrack_;

    // Index of the current beam in schedule_. Range: [0, schedule_.size()).
    // Advanced by advance(). Wraps circularly. REQ-AESA-010.
    int    currentIndex_      = 0;

    // Accumulated elapsed time in the current beam dwell (milliseconds).
    // Compared against currentBeam().dwellTime_ms to detect dwell expiry.
    // Reset to 0.0 on dwell expiry. REQ-AESA-010.
    double dwellElapsed_ms_   = 0.0;

    // Total number of search beam positions in the current search grid.
    // Set by buildSchedule(). Used to detect scan completion. REQ-AESA-010.
    int    totalSearchBeams_  = 0;

    // Number of search beams served in the current scan pass.
    // Incremented each time a SEARCH beam dwell expires.
    // Reset to 0 when scanComplete_ is set. REQ-AESA-010.
    int    searchBeamsServed_ = 0;

    // true = all search beams have been served once in this scan pass.
    // Valid for exactly one tick — cleared at the start of each advance().
    // Consumed by RadarModel_AESA::update() to trigger scan boundary actions.
    // REQ-AESA-010.
    bool   scanComplete_      = false;

    // Duty cycle of the current beam's waveform (dimensionless, [0.0, 1.0]).
    // Updated at the start of each advance() call from computeDutyCycle().
    // Published to RadarOutput::currentDutyCycle. REQ-AESA-020.
    double currentDutyCycle_  = 0.0;

    // Fallback beam descriptor. Returned by currentBeam() when schedule_ is
    // empty. Provides a safe boresight search beam as default. REQ-AESA-010.
    // Populated in buildSchedule() when the schedule is otherwise empty,
    // and used as a const return reference in currentBeam().
    BeamRequest fallbackBeam_;
};

} // namespace aesa

#endif // RADARSCHEDULER_H

// #pragma once
// #ifndef RADARSCHEDULER_H
// #define RADARSCHEDULER_H
// // radarscheduler.h  —  Rev 3  FIX-08 duty-cycle enforcement
// #include "radarmodel_aesa.h"
// #include <vector>

// namespace aesa {

// class RadarScheduler
// {
// public:
//     RadarScheduler() = default;

//     void reset();
//     void buildSchedule(const RadarConfig& cfg, const std::vector<TrackFile>& tracks);
//     void insertFireControlBeam(uint32_t targetID, const TrackFile* track,
//                                const RadarConfig& cfg);

//     const BeamRequest& currentBeam() const;
//     void advance(double dt_s);

//     bool   scanCompleted()    const { return scanComplete_;      }
//     int    searchGridSize()   const { return totalSearchBeams_;  }
//     int    currentIndex()     const { return currentIndex_;      }
//     int    scheduleSize()     const { return static_cast<int>(schedule_.size()); }
//     double dwellElapsed_ms()  const { return dwellElapsed_ms_;   }
//     double currentDutyCycle() const { return currentDutyCycle_;  } // FIX-08

// private:
//     void buildSearchGrid    (const RadarConfig& cfg);
//     void insertTrackBeams   (const std::vector<TrackFile>& tracks, const RadarConfig& cfg);
//     void interleaveSchedule ();

//     BeamRequest makeTrackBeam(const TrackFile& track,
//                               const RadarConfig& cfg, bool manoeuvring) const;

//     static double      computeDutyCycle  (const BeamWaveform& wf);
//     static BeamWaveform degradeWaveform  (const BeamWaveform& wf, double targetDuty);

//     std::vector<BeamRequest> schedule_, searchGrid_, pendingTrack_;

//     int    currentIndex_      = 0;
//     double dwellElapsed_ms_   = 0.0;
//     int    totalSearchBeams_  = 0;
//     int    searchBeamsServed_ = 0;
//     bool   scanComplete_      = false;
//     double currentDutyCycle_  = 0.0;

//     BeamRequest fallbackBeam_;
// };

// } // namespace aesa
// #endif
