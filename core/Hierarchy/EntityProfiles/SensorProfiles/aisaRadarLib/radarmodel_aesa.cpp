// =============================================================================
// FILE:         radarmodel_aesa.cpp
// MODULE:       AESA Radar Simulation Model — Implementation
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements the RadarModel_AESA class. All physics, detection,
//               tracking, EW, and output assembly logic lives here.
//               This file coordinates five subsystems:
//                 signal_    — SINR, CFAR, RCS, propagation
//                 antenna_   — beam steering, gain
//                 scheduler_ — beam schedule generation and execution
//                 tracker_   — Kalman / IMM / JPDA multi-target tracking
//                 library_   — ESM signal intercept accumulation
//
//               Internal design principles:
//                 - All public methods acquire mutex_ before touching state.
//                 - All private methods are called from update() which already
//                   holds mutex_ — they must NOT re-acquire it (deadlock).
//                 - No dynamic memory allocation occurs in the operational
//                   update() loop except through std::vector reserve/push_back
//                   on pre-reserved vectors (known MM-01 deviation,
//                   ICD-AESA-DEVIATION-002).
//                 - No recursion anywhere in the file (FN-06 compliant).
//                 - No exceptions (FP-01 compliant).
//
// REQUIREMENTS: REQ-AESA-001  Lifecycle
//               REQ-AESA-002  Configuration
//               REQ-AESA-003  Mode control
//               REQ-AESA-004  Output assembly
//               REQ-AESA-010  Beam steering coordination
//               REQ-AESA-020  PRF / waveform management
//               REQ-AESA-021  Staggered PRF ambiguity resolution
//               REQ-AESA-030  Multi-target tracking
//               REQ-AESA-040  Detection pipeline
//               REQ-AESA-050  IFF interrogation
//               REQ-AESA-060  Electronic warfare injection
//               REQ-AESA-061  Chaff clutter
//               REQ-AESA-070  Occlusion model
//               REQ-AESA-071  Propagation loss
//               REQ-AESA-072  Multipath
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-MODEL-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation.
//   Rev 2  15 Feb 2026  13 audit fixes applied (FIX-01 through FIX-13).
//   Rev 3  01 Apr 2026  Staggered PRF, RGPO/VGPO, occlusion, IMM, STAP added.
//                       Commented-out code removed per NS-05.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Magic numbers replaced with named constexpr constants.
//                       Section markers added for DER navigation.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#include "radarmodel_aesa.h"
#include "radarantenna_aesa.h"
#include "radarsignalprocessor_aesa.h"
#include "radarscheduler.h"
#include "radartracker_aesa.h"
#include "radarsignallibrary_aesa.h"
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <random>

// =============================================================================
// FILE-SCOPE NAMED CONSTANTS
//
// All numeric literals used in this translation unit are declared here.
// Satisfies VI-08 (no magic numbers) and ensures single-point change control.
// =============================================================================
namespace
{
// Speed of light in vacuum (m/s). Used for Rmax and wavelength computation.
// REQ-AESA-020, REQ-AESA-021.
constexpr double SPEED_OF_LIGHT = 299792458.0;

// Pi — full precision. Replaces non-standard M_PI macro. LC-08 compliant.
constexpr double PI = 3.14159265358979323846;

// Conversion factor: degrees to radians.
constexpr double DEG_TO_RAD = PI / 180.0;

// Conversion factor: radians to degrees.
constexpr double RAD_TO_DEG = 180.0 / PI;

// Earth mean radius (metres). Used in horizon range computation.
// REQ-AESA-071.
constexpr double EARTH_RADIUS_M = 6371000.0;

// Minimum platform altitude (metres) above which radarHeight is updated
// from pose.y. Below this threshold the platform is assumed to be on
// the ground and radarHeight is not modified. REQ-AESA-071.
constexpr double MIN_AIRBORNE_HEIGHT_M = 50.0;

// Minimum change in radarHeight (metres) required to trigger a display
// range recomputation. Prevents unnecessary recalculation on small
// altitude fluctuations. REQ-AESA-004.
constexpr double HEIGHT_CHANGE_THRESHOLD_M = 1.0;

// Minimum detectable PRF (Hz). PRF values below this are clamped here
// to prevent division by zero in Rmax computation. REQ-AESA-020.
constexpr float  MIN_PRF_HZ = 1.0f;

// Threshold PRF2 value (Hz) below which staggered PRF second Rmax is
// treated as disabled (Rmax2 = 0). REQ-AESA-021.
constexpr double PRF2_ENABLED_THRESHOLD = 1.0;

// Chaff cloud expiry multiplier. A cloud is pruned when its age exceeds
// decayTime_s * CHAFF_EXPIRY_FACTOR. At 5x the decay constant the
// remaining RCS is exp(-5) ≈ 0.007 of initial — negligible. REQ-AESA-061.
constexpr double CHAFF_EXPIRY_FACTOR = 5.0;

// Maximum number of consecutive missed fire-control dwells before the
// lock is broken and the mode reverts to SURVEILLANCE. Prevents a single
// bad SINR frame from breaking lock due to Swerling RCS fluctuation.
// REQ-AESA-003.
constexpr int    LOCK_MISS_THRESHOLD = 10;

// Minimum display range (km). Prevents the UI from scaling to a degenerate
// range. REQ-AESA-004.
constexpr double DISPLAY_RANGE_MIN_KM = 5.0;

// Maximum display range (km). REQ-AESA-004.
constexpr double DISPLAY_RANGE_MAX_KM = 1000.0;

// Minimum valid Rmax2 for staggered velocity resolution (metres).
// REQ-AESA-021.
constexpr double RMAX2_MIN_FOR_VELOCITY_RESOLVE = 1.0;

// Minimum valid prf2_Hz for staggered velocity resolution. REQ-AESA-021.
constexpr float  PRF2_MIN_FOR_VELOCITY_RESOLVE = 1.0f;

// Minimum range (metres) below which range is clamped to minDetectableRange
// rather than reported as-is. Prevents sub-minimum range detections
// appearing in the output. REQ-AESA-040.
constexpr double DRFM_MIN_RANGE_M = 1.0;

// DRFM pull-off timestep (seconds). Fixed dwell time used for DRFM
// pull-off accumulation in injectDRFMGhost(). REQ-AESA-060.
constexpr double DRFM_DWELL_S = 0.05;

// DRFM ghost SNR scaling factor. Ghost SNR = real SNR * DRFM_GHOST_SNR_SCALE.
// 0.6 = ghost is 4.4 dB weaker than the real return, which is realistic
// for a DRFM repeater with finite delay and phase noise. REQ-AESA-060.
constexpr double DRFM_GHOST_SNR_SCALE = 0.6;

// DRFM ghost Rmax multiplier. Ghost range is capped at
// DRFM_RMAX_MULTIPLIER * maxUnambiguousRange. REQ-AESA-060.
constexpr double DRFM_RMAX_MULTIPLIER = 3.0;

// RGPO ghost SNR boost factor (relative to real SNR).
// 1.2 = RGPO return is slightly stronger than real — jammer amplifies.
// REQ-AESA-060.
constexpr double RGPO_GHOST_SNR_SCALE = 1.2;

// VGPO ghost SNR scale factor (relative to real SNR). REQ-AESA-060.
constexpr double VGPO_GHOST_SNR_SCALE = 1.1;

// Knife-edge diffraction lower boundary of nu. Below this value the
// target is in the lit region — no diffraction loss. ITU-R P.526-15.
// REQ-AESA-070.
constexpr double KE_LIT_BOUNDARY = -0.78;

// Knife-edge polynomial / asymptotic regime boundary.
// nu <= KE_POLY_LIMIT: polynomial approximation.
// nu >  KE_POLY_LIMIT: asymptotic log formula. REQ-AESA-070.
constexpr double KE_POLY_LIMIT = 2.4;

// ITU-R P.526-15 polynomial coefficients for J(nu) in polynomial regime.
// J(nu) = KE_A0 + KE_A1*nu + KE_A2*nu^2. REQ-AESA-070.
constexpr double KE_A0 = 6.02;
constexpr double KE_A1 = 9.11;
constexpr double KE_A2 = 1.27;

// ITU-R P.526-15 asymptotic constant for J(nu) in log regime.
// J(nu) = 20*log10(nu) + KE_ASYMPTOTE. REQ-AESA-070.
constexpr double KE_ASYMPTOTE = 13.0;

// Two-way loss multiplier for diffraction. Diffraction loss is computed
// one-way then doubled for the two-way radar path. REQ-AESA-070.
constexpr double TWO_WAY_LOSS_FACTOR = 2.0;

// Occlusion SHADOW threshold (dB). Total loss >= this value classifies
// the target as fully shadowed — rejected from detection pipeline.
// REQ-AESA-070.
constexpr double SHADOW_THRESHOLD_DB = 40.0;

// Occlusion PENUMBRA threshold (dB). Loss >= this value but below SHADOW
// classifies target as partially occluded — RCS attenuated. REQ-AESA-070.
constexpr double PENUMBRA_THRESHOLD_DB = 6.0;

// Occluder radius defaults (metres) for platforms without valid dimensions.
// Used in perpendicular distance computation for knife-edge geometry.
// REQ-AESA-070.
constexpr double OCCLUDER_RADIUS_SHIP_M   = 15.0;
constexpr double OCCLUDER_RADIUS_BOMBER_M = 12.0;
constexpr double OCCLUDER_RADIUS_DEFAULT_M =  5.0;

// First Fresnel zone clearance threshold. If the perpendicular distance
// from the LOS to the occluder exceeds 0.577 * r1 (first Fresnel radius)
// the target is in the lit region — skip diffraction computation.
// ITU-R P.526-15 Sec 4.1. REQ-AESA-070.
constexpr double FRESNEL_CLEARANCE_FACTOR = 0.577;

// Minimum distance (metres) for a valid occluder-to-radar path segment.
// Occluders closer than this are skipped to avoid division by zero.
// REQ-AESA-070.
constexpr double MIN_OCCLUDER_SEGMENT_M = 1.0;

// Minimum candidate range (metres) for occlusion to be meaningful.
// A target at zero range cannot be meaningfully occluded. REQ-AESA-070.
constexpr double MIN_CANDIDATE_RANGE_M = 1.0;

// Radial velocity threshold (m/s) below which the relaxed CFAR threshold
// is used instead of the standard one. Slow-moving or stationary targets
// near the clutter notch receive a higher threshold (lower Pfa equivalent)
// to avoid false alarms from residual clutter. REQ-AESA-040.
constexpr double CFAR_RELAXED_RADVEL_THRESHOLD = 5.0;

// clamp bounds for acos domain protection. REQ-AESA-010.
constexpr double DOT_CLAMP_MIN = -1.0;
constexpr double DOT_CLAMP_MAX =  1.0;

} // anonymous namespace

// =============================================================================
// FILE-SCOPE RANDOM NUMBER GENERATOR
//
// thread_local RNG seeded from std::random_device at thread startup.
// thread_local ensures each thread has its own state — no mutex required.
// KNOWN DEVIATION: LC-02 — std::random_device seeding is implementation-defined.
// Mitigated by: deterministic clamp on all outputs, no safety decision depends
// on a specific RNG value. Documented in ICD-AESA-DEVIATION-003.
// REQ-AESA-040.
// =============================================================================
static thread_local std::default_random_engine tl_rng{
    std::random_device{}()
};

namespace aesa {

// =============================================================================
// SECTION 1: CONSTRUCTION
// REQ-AESA-001
// =============================================================================

// =============================================================================
// FUNCTION: RadarModel_AESA (constructor)
// Full description in header.
// =============================================================================
RadarModel_AESA::RadarModel_AESA()
    : signal_   (std::make_unique<RadarSignalProcessor_AESA>())
    , antenna_  (std::make_unique<RadarAntenna_AESA>())
    , scheduler_(std::make_unique<RadarScheduler>())
    , tracker_  (std::make_unique<RadarTracker_AESA>())
    , library_  (std::make_unique<RadarSignalLibrary_AESA>())
{
    // All subsystems constructed via make_unique. No other initialisation
    // occurs here — the model is NOT operational until init() is called.
    // REQ-AESA-001.
}

// =============================================================================
// FUNCTION: ~RadarModel_AESA (destructor)
// Full description in header. Default destructor — unique_ptr handles cleanup.
// =============================================================================
RadarModel_AESA::~RadarModel_AESA() = default;

// =============================================================================
// SECTION 2: LIFECYCLE
// REQ-AESA-001
// =============================================================================

// =============================================================================
// FUNCTION: init
// Full description in header.
// =============================================================================
void RadarModel_AESA::init(const RadarConfig& cfg)
{
    // Acquire mutex_ before modifying any shared state. REQ-AESA-001.
    std::lock_guard<std::mutex> lk(mutex_);

    // Store the supplied configuration. This is the single source of truth
    // for all operational parameters. REQ-AESA-002.
    config_            = cfg;
    displayRangeDirty_ = true;

    // Reset output to a clean default. Publish the initial mode so consumers
    // see the correct mode immediately after init, not the default SURVEILLANCE.
    latestOutput_      = RadarOutput{};
    latestOutput_.mode = cfg.mode;

    // Reset all subsystems to their initial states. Order matters:
    // tracker and antenna must be cleared before scheduler (which may
    // reference tracker state in a future redesign). REQ-AESA-001.
    tracker_->clear();
    antenna_->reset();
    library_->clear();
    scheduler_->reset();

    // Clear all EW and environmental state. REQ-AESA-060, REQ-AESA-061.
    chaffClouds_.clear();
    drfmPullOff_.clear();
    rgpoPullOff_.clear();
    vgpoPullOff_.clear();

    // Clear scan detection accumulator. Any detections from a previous run
    // must not persist into the new initialisation cycle. REQ-AESA-004.
    scanDetectionCache_.clear();

    // Mark model as initialised but not yet running.
    // update() returns immediately until start() is called. REQ-AESA-001.
    initialised_   = true;
    running_       = false;
    lockMissCount_ = 0;
}

// =============================================================================
// FUNCTION: start
// Full description in header.
// =============================================================================
void RadarModel_AESA::start()
{
    // Acquire mutex_ before modifying shared state. REQ-AESA-001.
    std::lock_guard<std::mutex> lk(mutex_);

    // Clear all runtime state that may have accumulated since init().
    // This mirrors the init() cleanup but without overwriting config_,
    // so the configuration set by init() or setConfig() is preserved.
    // REQ-AESA-001.
    tracker_->clear();
    antenna_->reset();
    library_->clear();
    chaffClouds_.clear();
    drfmPullOff_.clear();

    // Reset TWS first-scan guard. The first full scan boundary in TWS mode
    // must complete before tracks are published to the display. This prevents
    // partial-scan ghost tracks appearing immediately after start().
    // REQ-AESA-003.
    firstScanComplete_ = false;

    // Clear any stale detections from a previous run. REQ-AESA-004.
    scanDetectionCache_.clear();

    // Build the initial beam schedule from the current config.
    // Empty track database on first start — no track beams yet.
    // REQ-AESA-010.
    scheduler_->buildSchedule(config_, {});

    // Reset output. Publish the current mode so consumers see correct mode
    // from first getOutput() call. REQ-AESA-004.
    latestOutput_             = RadarOutput{};
    latestOutput_.mode        = config_.mode;
    displayRangeDirty_        = true;

    // Model is now operational. update() will execute the pipeline. REQ-AESA-001.
    running_ = true;
}

// =============================================================================
// FUNCTION: end
// Full description in header.
// =============================================================================
void RadarModel_AESA::end()
{
    // Acquire mutex_ before modifying shared state. REQ-AESA-001.
    std::lock_guard<std::mutex> lk(mutex_);

    // Stop operational processing immediately. REQ-AESA-001.
    running_ = false;

    // Clear all runtime state. Releasing tracker and library state ensures
    // no stale data is returned if getOutput() is called after end().
    tracker_->clear();
    library_->clear();
    scheduler_->reset();
    chaffClouds_.clear();
    drfmPullOff_.clear();

    // Reset output to a clean default state. REQ-AESA-004.
    latestOutput_ = RadarOutput{};
}

// =============================================================================
// FUNCTION: reset
// Full description in header.
// =============================================================================
void RadarModel_AESA::reset()
{
    // Save current configuration under mutex_ so the reset uses the same
    // config that was active when reset() was called. REQ-AESA-001.
    RadarConfig saved;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        saved = config_;
    }

    // Reset TWS first-scan guard before init/start so it is in the correct
    // state when start() runs. REQ-AESA-003.
    firstScanComplete_ = false;

    // Re-initialise and restart with the saved configuration.
    // init() and start() each acquire mutex_ internally. REQ-AESA-001.
    init(saved);
    start();

    // Reset EW pull-off accumulators. These are not reset by init/start
    // because they are not protected by mutex_ in the same way.
    // REQ-AESA-060.
    lockMissCount_ = 0;
    rgpoPullOff_.clear();
    vgpoPullOff_.clear();
}

// =============================================================================
// SECTION 3: CONFIGURATION AND MODE CONTROL
// REQ-AESA-002, REQ-AESA-003
// =============================================================================

// =============================================================================
// FUNCTION: setConfig
// Full description in header.
// =============================================================================
void RadarModel_AESA::setConfig(const RadarConfig& cfg)
{
    // Acquire mutex_ before reading or writing config_. REQ-AESA-002.
    std::lock_guard<std::mutex> lk(mutex_);

    // Mode transition guard: if the new config changes mode to SURVEILLANCE
    // from any other mode, clear the tracker and output caches.
    // Rationale: stale TWS or LOCK_ON tracks must not appear on the display
    // after a mode downgrade. REQ-AESA-003.
    if (cfg.mode == RadarMode::SURVEILLANCE &&
        config_.mode != RadarMode::SURVEILLANCE)
    {
        tracker_->clear();
        latestOutput_.tracks.clear();
        latestOutput_.detections.clear();
    }

    // Apply new configuration and mark display range for recomputation.
    // REQ-AESA-002.
    config_            = cfg;
    displayRangeDirty_ = true;
}

// =============================================================================
// FUNCTION: getConfig
// Full description in header.
// =============================================================================
RadarConfig RadarModel_AESA::getConfig() const
{
    // Acquire mutex_ for thread-safe config copy. REQ-AESA-002.
    std::lock_guard<std::mutex> lk(mutex_);
    return config_;
}

// =============================================================================
// FUNCTION: setMode
// Full description in header.
// =============================================================================
void RadarModel_AESA::setMode(RadarMode mode)
{
    // Acquire mutex_ before modifying config_ and scheduler. REQ-AESA-003.
    std::lock_guard<std::mutex> lk(mutex_);

    // Update mode and force display range recalculation. REQ-AESA-003.
    config_.mode       = mode;
    displayRangeDirty_ = true;

    // Rebuild schedule immediately for the new mode. In TWS mode this adds
    // track beams for all validated tracks. In SURVEILLANCE it returns to
    // pure search. REQ-AESA-010.
    scheduler_->buildSchedule(config_, tracker_->database());
}

// =============================================================================
// FUNCTION: lockOn
// Full description in header.
// =============================================================================
void RadarModel_AESA::lockOn(uint32_t targetID)
{
    // Acquire mutex_ before modifying config_ and caches. REQ-AESA-003.
    std::lock_guard<std::mutex> lk(mutex_);

    // Transition to LOCK_ON mode targeting the specified platform. REQ-AESA-003.
    config_.mode           = RadarMode::LOCK_ON;
    config_.lockedTargetID = targetID;

    // Reset lock miss counter. A fresh lock should not start with accumulated
    // miss count from a previous lock attempt. REQ-AESA-003.
    lockMissCount_ = 0;

    // Clear TWS detection cache and output. In TWS mode, scanDetectionCache_
    // may contain detections at various azimuths. Publishing this during
    // fire-control would show ghost targets at non-lock positions on the display.
    // REQ-AESA-004.
    scanDetectionCache_.clear();
    latestOutput_.detections.clear();
    latestOutput_.tracks.clear();

    // Reset first-scan guard. Fire-control loop must re-establish track
    // quality from scratch. REQ-AESA-003.
    firstScanComplete_ = false;

    // Rebuild schedule with fire-control beam on the locked target.
    // The scheduler places the FC beam at the head of the schedule.
    // REQ-AESA-010.
    scheduler_->buildSchedule(config_, tracker_->database());
}

// =============================================================================
// FUNCTION: breakLock
// Full description in header.
// =============================================================================
void RadarModel_AESA::breakLock()
{
    // Acquire mutex_ before modifying config_ and caches. REQ-AESA-003.
    std::lock_guard<std::mutex> lk(mutex_);

    // Return to SURVEILLANCE mode and clear the locked target ID. REQ-AESA-003.
    config_.mode           = RadarMode::SURVEILLANCE;
    config_.lockedTargetID = 0;

    // Clear scan detection cache. In LOCK_ON mode all beam time was concentrated
    // on one target at one azimuth. If that biased cache were published as the
    // first complete scan after returning to SURVEILLANCE, the display would show
    // a ghost detection at the old lock azimuth until the next full scan boundary
    // flushed it naturally. Clearing here prevents this artefact. REQ-AESA-004.
    scanDetectionCache_.clear();
    latestOutput_.detections.clear();
    latestOutput_.tracks.clear();

    // Reset first-scan guard so the first full surveillance scan must complete
    // before tracks are published again. REQ-AESA-003.
    firstScanComplete_ = false;

    // Rebuild the beam schedule for SURVEILLANCE mode. REQ-AESA-010.
    scheduler_->buildSchedule(config_, tracker_->database());
}

// =============================================================================
// FUNCTION: getOutput
// Full description in header.
// =============================================================================
RadarOutput RadarModel_AESA::getOutput() const
{
    // Acquire mutex_ for thread-safe output copy. REQ-AESA-004.
    std::lock_guard<std::mutex> lk(mutex_);
    return latestOutput_;
}

// =============================================================================
// SECTION 4: CHAFF AND EXTERNAL TRACK
// REQ-AESA-061, REQ-AESA-030
// =============================================================================

// =============================================================================
// FUNCTION: addChaffCloud
// Full description in header.
// =============================================================================
void RadarModel_AESA::addChaffCloud(const ChaffCloud& cloud)
{
    // Acquire mutex_ before modifying chaffClouds_. REQ-AESA-061.
    std::lock_guard<std::mutex> lk(mutex_);
    chaffClouds_.push_back(cloud);
}

// =============================================================================
// FUNCTION: clearChaffClouds
// Full description in header.
// =============================================================================
void RadarModel_AESA::clearChaffClouds()
{
    // Acquire mutex_ before modifying chaffClouds_. REQ-AESA-061.
    std::lock_guard<std::mutex> lk(mutex_);
    chaffClouds_.clear();
}

// =============================================================================
// FUNCTION: injectExternalTrack
// Full description in header.
// =============================================================================
void RadarModel_AESA::injectExternalTrack(const TrackOutput& ext)
{
    // Acquire mutex_ before modifying tracker database. REQ-AESA-030.
    std::lock_guard<std::mutex> lk(mutex_);
    tracker_->injectExternalTrack(ext, currentSimTime_, config_);
}

// =============================================================================
// FUNCTION: loadSignalLibrary
// Full description in header.
// =============================================================================
void RadarModel_AESA::loadSignalLibrary(
    const std::vector<SignalLibraryEntry>& entries)
{
    // Acquire mutex_ before modifying library. REQ-AESA-040.
    std::lock_guard<std::mutex> lk(mutex_);
    library_->loadLibrary(entries);
}

// =============================================================================
// SECTION 5: ATTITUDE COMPENSATION
// REQ-AESA-010
// =============================================================================

// =============================================================================
// FUNCTION: applyAttitudeToBeam
// Full description in header.
// =============================================================================
void RadarModel_AESA::applyAttitudeToBeam(double bodyAz, double bodyEl,
                                          double& worldAz, double& worldEl) const
{
    // Fast path: if roll and pitch are negligible (< 0.1 deg), apply heading
    // rotation only. This avoids the full 3D rotation matrix computation for
    // level flight — the common case for most simulation scenarios. REQ-AESA-010.
    if (std::abs(currentPose_.roll)  < 0.1f &&
        std::abs(currentPose_.pitch) < 0.1f)
    {
        worldAz = bodyAz + static_cast<double>(currentPose_.heading);

        // Wrap worldAz into [0, 360) for compass convention.
        if (worldAz >= 360.0) worldAz -= 360.0;
        if (worldAz <    0.0) worldAz += 360.0;

        worldEl = bodyEl;
        return;
    }

    // Full path: apply roll, pitch, then heading rotation matrices sequentially.
    // Converts body-frame unit vector to world-frame unit vector.
    // REQ-AESA-010.

    // Step 1: Convert body az/el to Cartesian unit vector.
    double azR = bodyAz * DEG_TO_RAD;
    double elR = bodyEl * DEG_TO_RAD;
    double bx  = std::cos(elR) * std::cos(azR);
    double by  = std::cos(elR) * std::sin(azR);
    double bz  = std::sin(elR);

    // Step 2: Apply roll rotation (rotation about x-axis).
    double rr  = static_cast<double>(currentPose_.roll) * DEG_TO_RAD;
    double bx1 = bx;
    double by1 = by * std::cos(rr) - bz * std::sin(rr);
    double bz1 = by * std::sin(rr) + bz * std::cos(rr);

    // Step 3: Apply pitch rotation (rotation about y-axis).
    double pp  = static_cast<double>(currentPose_.pitch) * DEG_TO_RAD;
    double bx2 =  bx1 * std::cos(pp) + bz1 * std::sin(pp);
    double by2 =  by1;
    double bz2 = -bx1 * std::sin(pp) + bz1 * std::cos(pp);

    // Step 4: Apply heading rotation (rotation about z-axis).
    double hh  = static_cast<double>(currentPose_.heading) * DEG_TO_RAD;
    double bx3 = bx2 * std::cos(hh) - by2 * std::sin(hh);
    double by3 = bx2 * std::sin(hh) + by2 * std::cos(hh);
    double bz3 = bz2;

    // Step 5: Convert world-frame Cartesian back to az/el.
    // atan2 gives az in (-pi, pi] — convert to [0, 360) for compass convention.
    worldAz = std::atan2(by3, bx3) * RAD_TO_DEG;
    if (worldAz < 0.0) worldAz += 360.0;

    // asin domain is [-1, 1]. Clamp to prevent NaN from floating-point rounding.
    worldEl = std::asin(std::clamp(bz3, DOT_CLAMP_MIN, DOT_CLAMP_MAX))
              * RAD_TO_DEG;
}

// =============================================================================
// SECTION 6: IFF INTERROGATION
// REQ-AESA-050
// =============================================================================

// =============================================================================
// FUNCTION: queryIFF
// Full description in header.
// =============================================================================
IFFResult RadarModel_AESA::queryIFF(const TrackFile& track,
                                    const std::vector<TargetInput>& world) const
{
    IFFResult res;
    res.modeUsed = config_.interrogationMode;

    // If IFF is disabled, return NO_REPLY immediately — no interrogation
    // transmitted. REQ-AESA-050.
    if (config_.interrogationMode == IFFMode::OFF) return res;

    // Search worldInputs for the target with matching ID. REQ-AESA-050.
    for (const auto& t : world)
    {
        if (t.id != track.id) continue;

        // Target found. Check if it has a functioning IFF transponder.
        if (!t.hasIFF || t.iffMode == IFFMode::OFF)
        {
            // Target has no IFF — no reply. REQ-AESA-050.
            res.response = IFFResponseCode::NO_REPLY;
            return res;
        }

        // Target replied — record the squawk code. REQ-AESA-050.
        res.squawk = t.iffSquawk;

        // Check if the received squawk is in the friendly squawk list.
        bool friendly = false;
        for (uint32_t sq : config_.friendlySquawks)
        {
            if (sq == t.iffSquawk)
            {
                friendly = true;
                break;
            }
        }

        // Classify the response. REQ-AESA-050.
        if (friendly)
        {
            // Squawk matched a friendly code — high confidence friendly.
            res.response   = IFFResponseCode::FRIENDLY;
            res.confidence = 0.95;
        }
        else if (t.iffSquawk != 0)
        {
            // Squawk received but not in friendly list — unknown platform.
            res.response   = IFFResponseCode::UNKNOWN;
            res.confidence = 0.70;
        }
        else
        {
            // No squawk transmitted despite IFF being active — treat as no reply.
            res.response   = IFFResponseCode::NO_REPLY;
            res.confidence = 0.0;
        }
        return res;
    }

    // Target not found in worldInputs — no reply (may be out of sensor range).
    return res;
}

// =============================================================================
// SECTION 7: MAIN UPDATE LOOP
// REQ-AESA-001 through REQ-AESA-004, REQ-AESA-010 through REQ-AESA-072
// =============================================================================

// =============================================================================
// FUNCTION: update
// Full description in header.
// =============================================================================
void RadarModel_AESA::update(double dt, const RadarPose& pose,
                             const std::vector<TargetInput>& worldInputs,
                             double simTime)
{
    // Acquire mutex_ for the entire update tick. All subsystem calls within
    // this function are protected by this single lock. Private helper methods
    // called below must NOT re-acquire mutex_. REQ-AESA-001.
    std::lock_guard<std::mutex> lk(mutex_);

    // Early exit if model has not been started. REQ-AESA-001.
    if (!running_) return;

    // -------------------------------------------------------------------------
    // Step 1: Update per-tick context state.
    // -------------------------------------------------------------------------

    // Store current simulation time for use by chaff decay, track coast,
    // and library timestamping functions. REQ-AESA-001.
    currentSimTime_     = simTime;
    currentPose_        = pose;

    // Store world inputs for occlusion computation. computeOcclusion() runs
    // inside processTargetDetection() and needs all other targets — it cannot
    // receive worldInputs as a parameter from that call site. REQ-AESA-070.
    currentWorldInputs_ = worldInputs;

    // Clear the lock-broken flag. Set again below only if lock is broken this tick.
    latestOutput_.lockBroken = false;

    // -------------------------------------------------------------------------
    // Step 2: Update radarHeight from platform altitude.
    // Only update if the platform is meaningfully airborne and the height
    // has changed significantly (prevents unnecessary display range recalculation
    // on minor altitude fluctuations). REQ-AESA-071.
    // -------------------------------------------------------------------------
    if (pose.y > MIN_AIRBORNE_HEIGHT_M &&
        std::abs(pose.y - config_.radarHeight) > HEIGHT_CHANGE_THRESHOLD_M)
    {
        config_.radarHeight = pose.y;
        displayRangeDirty_  = true;
    }

    // -------------------------------------------------------------------------
    // Step 3: Prune expired chaff clouds.
    // A cloud is expired when its age exceeds decayTime_s * CHAFF_EXPIRY_FACTOR.
    // At 5x the decay constant, remaining RCS is negligible. REQ-AESA-061.
    // -------------------------------------------------------------------------
    chaffClouds_.erase(
        std::remove_if(chaffClouds_.begin(), chaffClouds_.end(),
                       [&](const ChaffCloud& c)
                       {
                           return (simTime - c.birthTime_s) >
                                  c.decayTime_s * CHAFF_EXPIRY_FACTOR;
                       }),
        chaffClouds_.end());

    // -------------------------------------------------------------------------
    // Step 4: Kalman prediction.
    // Advance all track files forward in time by dt using the IMM predictor
    // (for tracks with hitCount >= 2) or pure CV (for tentative tracks).
    // Only run in tracking modes — SURVEILLANCE has no tracker. REQ-AESA-030.
    // -------------------------------------------------------------------------
    if (config_.mode == RadarMode::TWS ||
        config_.mode == RadarMode::LOCK_ON)
    {
        tracker_->predict(dt);
    }

    // -------------------------------------------------------------------------
    // Step 5: Beam scheduling — advance then read.
    // advance() MUST be called before reading currentBeam() so that both the
    // Rmax computation and the detection pipeline use the same waveform.
    // Previously Rmax was computed from the pre-advance beam, causing range
    // ambiguity checks to use the wrong PRF when the scheduler stepped to a
    // new beam in the same tick. REQ-AESA-010, REQ-AESA-020.
    // -------------------------------------------------------------------------
    scheduler_->advance(dt);
    const BeamRequest& beam = scheduler_->currentBeam();

    // -------------------------------------------------------------------------
    // Step 6: Compute unambiguous range limits.
    // Primary Rmax from beam waveform PRF. REQ-AESA-020.
    // -------------------------------------------------------------------------

    // Clamp PRF to minimum to prevent division by zero. REQ-AESA-020.
    double prf  = static_cast<double>(std::max(MIN_PRF_HZ, beam.waveform.prf_Hz));
    double Rmax = SPEED_OF_LIGHT / (2.0 * prf);

    // Secondary Rmax for staggered PRF mode. Zero when prf2_Hz is not set.
    // REQ-AESA-021.
    double prf2  = static_cast<double>(beam.waveform.prf2_Hz);
    double Rmax2 = (prf2 > PRF2_ENABLED_THRESHOLD)
                       ? SPEED_OF_LIGHT / (2.0 * prf2)
                       : 0.0;

    // -------------------------------------------------------------------------
    // Step 7: Beam pointing.
    // In LOCK_ON mode, re-point the beam from the Kalman-predicted track
    // position every tick so the beam follows a moving target rather than
    // staying at the schedule's baked-in lock azimuth. REQ-AESA-010.
    // -------------------------------------------------------------------------
    if (config_.mode == RadarMode::LOCK_ON && config_.lockedTargetID != 0)
    {
        bool pointed = false;

        // Search the tracker database for the locked track.
        for (const auto& tr : tracker_->database())
        {
            if (tr.id == config_.lockedTargetID && tr.predictedRange > DRFM_MIN_RANGE_M)
            {
                // Compute fire-control beam direction from Kalman-predicted position.
                double fcAz = std::atan2(tr.y, tr.x) * RAD_TO_DEG;
                double fcEl = std::asin(
                                  std::clamp(tr.z / tr.predictedRange,
                                             DOT_CLAMP_MIN, DOT_CLAMP_MAX))
                              * RAD_TO_DEG;

                // Point beam at predicted target position. spoilFactor=1 for
                // maximum fire-control gain — no beam widening. REQ-AESA-010.
                antenna_->pointBeam(fcAz, fcEl, config_, 1.0f);
                pointed = true;
                break;
            }
        }

        // Fallback: if the locked track is not yet in the database (first tick
        // after lockOn()), use the scheduled beam position. REQ-AESA-010.
        if (!pointed)
        {
            antenna_->pointBeam(beam.azimuth_deg, beam.elevation_deg,
                                config_, beam.spoilFactor);
        }
    }
    else
    {
        // Normal surveillance or TWS beam pointing from schedule. REQ-AESA-010.
        antenna_->pointBeam(beam.azimuth_deg, beam.elevation_deg,
                            config_, beam.spoilFactor);
    }

    // -------------------------------------------------------------------------
    // Step 8: Scan boundary detection.
    // scanCompleted() returns true when the scheduler has cycled through
    // all search grid positions once. Set/clear the antenna boundary flag
    // so output assembly can make mode-dependent cadence decisions. REQ-AESA-004.
    // -------------------------------------------------------------------------
    bool scanBnd = scheduler_->scanCompleted();
    if (scanBnd)
        antenna_->setScanBoundary();
    else
        antenna_->clearScanBoundary();

    // -------------------------------------------------------------------------
    // Step 9: Per-target detection pipeline.
    // Initialise measurement noise distributions for this tick. Standard
    // deviation = 0 produces noise-free ideal measurements (used in test).
    // REQ-AESA-040.
    // -------------------------------------------------------------------------
    std::normal_distribution<double> rNoise (0.0, config_.noise.rangeStdDev);
    std::normal_distribution<double> azNoise(0.0, config_.noise.azimuthStdDev);
    std::normal_distribution<double> elNoise(0.0, config_.noise.elevationStdDev);
    std::normal_distribution<double> dvNoise(0.0, config_.noise.dopplerStdDev);

    // Pre-reserve output vector. size*2 accounts for potential ghost injections
    // (one DRFM/RGPO/VGPO ghost per real detection). REQ-AESA-040.
    std::vector<DetectionOutput> scanDets;
    scanDets.reserve(worldInputs.size() * 2);

    // Track whether the locked target was seen this dwell. REQ-AESA-003.
    bool lockedSeen = false;

    for (const auto& t : worldInputs)
    {
        // In LOCK_ON mode, skip all targets except the locked one to concentrate
        // all processing time on the fire-control target. REQ-AESA-003.
        if (config_.mode == RadarMode::LOCK_ON &&
            t.id != config_.lockedTargetID)
        {
            continue;
        }

        // Run the detection pipeline for this target. REQ-AESA-040.
        bool got = processTargetDetection(
            t, beam, dt, simTime, Rmax, Rmax2,
            scanDets, rNoise, azNoise, elNoise, dvNoise);

        // Record whether the locked target was successfully detected. REQ-AESA-003.
        if (got &&
            config_.mode == RadarMode::LOCK_ON &&
            t.id == config_.lockedTargetID)
        {
            lockedSeen = true;
        }
    }

    // -------------------------------------------------------------------------
    // Step 10: Track association.
    // Associate new detections with existing tracks or create new tracks.
    // Only runs in TWS and LOCK_ON modes — SURVEILLANCE has no tracker.
    // REQ-AESA-030.
    // -------------------------------------------------------------------------
    if (config_.mode == RadarMode::TWS ||
        config_.mode == RadarMode::LOCK_ON)
    {
        if (config_.useJPDA)
        {
            // Joint Probabilistic Data Association update. Handles multiple
            // targets in the same gate simultaneously. REQ-AESA-030.
            tracker_->performJPDAUpdate(scanDets, simTime, Rmax, config_);

            // Create new tracks for detections not matched to any existing track.
            for (const auto& det : scanDets)
            {
                bool hasTrack = false;
                for (const auto& tr : tracker_->database())
                {
                    if (tr.id == det.targetID && tr.isUpdated)
                    {
                        hasTrack = true;
                        break;
                    }
                }
                if (!hasTrack)
                {
                    for (const auto& t : worldInputs)
                    {
                        if (t.id == det.targetID)
                        {
                            tracker_->createNewTrack(det, t, Rmax,
                                                     simTime, config_);
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            // Nearest-neighbour association. Simpler but may fail with closely
            // spaced targets. REQ-AESA-030.
            for (const auto& det : scanDets)
            {
                double    prob = 0.0;
                TrackFile* tr  = tracker_->findBestTrackMatch(det, Rmax, prob);
                if (tr)
                {
                    // Found a matching track — apply Kalman update. REQ-AESA-030.
                    tracker_->performKalmanUpdate(*tr, det, simTime,
                                                  dt, Rmax, config_);
                }
                else
                {
                    // No matching track — create a new one. REQ-AESA-030.
                    for (const auto& t : worldInputs)
                    {
                        if (t.id == det.targetID)
                        {
                            tracker_->createNewTrack(det, t, Rmax,
                                                     simTime, config_);
                            break;
                        }
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Step 11: Scan miss logic.
    // At each scan boundary, increment miss counts for tracks that were not
    // updated this scan and drop tracks that have exceeded the miss threshold.
    // Not applied in LOCK_ON — lock-on uses the separate lockMissCount_ counter
    // and should not drop the locked track due to normal miss logic. REQ-AESA-030.
    // -------------------------------------------------------------------------
    if (scanBnd && config_.mode != RadarMode::LOCK_ON)
    {
        tracker_->applyScanMissLogic(simTime, config_);
        library_->pruneStale(simTime, config_.trackCoastSeconds);
    }

    // -------------------------------------------------------------------------
    // Step 12: Break-lock miss counting.
    // Tolerate up to LOCK_MISS_THRESHOLD consecutive missed dwells before
    // dropping lock. Prevents Swerling RCS fluctuation or a single bad SINR
    // frame from immediately breaking the fire-control loop. REQ-AESA-003.
    // -------------------------------------------------------------------------
    if (config_.mode == RadarMode::LOCK_ON)
    {
        if (!lockedSeen)
        {
            // Locked target not detected this dwell — increment miss counter.
            if (++lockMissCount_ > LOCK_MISS_THRESHOLD)
            {
                // Miss threshold exceeded — break lock and revert to SURVEILLANCE.
                lockMissCount_           = 0;
                config_.mode             = RadarMode::SURVEILLANCE;
                config_.lockedTargetID   = 0;
                latestOutput_.lockBroken = true;

                // Clear biased cache before returning to surveillance scan.
                scanDetectionCache_.clear();

                // Rebuild schedule for SURVEILLANCE mode. REQ-AESA-010.
                scheduler_->buildSchedule(config_, tracker_->database());
            }
        }
        else
        {
            // Locked target seen this dwell — reset miss counter. REQ-AESA-003.
            lockMissCount_ = 0;
        }
    }

    // -------------------------------------------------------------------------
    // Step 13: IFF interrogation.
    // Query IFF for every validated track. Result stored in TrackFile::iff
    // and published via TrackOutput::iff. REQ-AESA-050.
    // -------------------------------------------------------------------------
    for (auto& tr : tracker_->database())
    {
        if (tr.isValidated)
        {
            tr.iff = queryIFF(tr, worldInputs);
        }
    }

    // -------------------------------------------------------------------------
    // Step 14: Display range computation.
    // Recompute only when displayRangeDirty_ is set (config change, mode change,
    // or altitude change). Cap at min(RF horizon, radar horizon). REQ-AESA-004.
    // -------------------------------------------------------------------------
    if (displayRangeDirty_)
    {
        // RF physics maximum range from radar equation. REQ-AESA-040.
        double rfRange = computeMaxDetectionRange_locked();

        // Radar horizon range from 4/3 earth radius model. REQ-AESA-071.
        double Re      = EARTH_RADIUS_M
                    * config_.earthRadiusFactor
                    * config_.atmosphericFactor;
        double dRadar  = std::sqrt(2.0 * Re *
                                  std::max(0.0, config_.radarHeight));
        double horizonRange_km = dRadar / 1000.0;

        // Display range is the minimum of RF range and horizon range,
        // clamped to [DISPLAY_RANGE_MIN_KM, DISPLAY_RANGE_MAX_KM]. REQ-AESA-004.
        cachedDisplayRange_km_ = std::max(
            DISPLAY_RANGE_MIN_KM,
            std::min(DISPLAY_RANGE_MAX_KM,
                     std::min(rfRange, horizonRange_km)));

        displayRangeDirty_ = false;
    }

    // -------------------------------------------------------------------------
    // Step 15: Scan detection cache update.
    // Accumulate detections from every beam position across the scan.
    // Deduplication by targetID ensures each target appears only once:
    //   - If a targetID is not in the cache, append it.
    //   - If it is already in the cache, update the existing entry with the
    //     latest detection (newer position/velocity estimate). REQ-AESA-004.
    // -------------------------------------------------------------------------
    for (auto& d : scanDets)
    {
        bool alreadyHave = false;
        for (const auto& existing : scanDetectionCache_)
        {
            if (existing.targetID == d.targetID)
            {
                alreadyHave = true;
                break;
            }
        }

        if (!alreadyHave)
        {
            // New target for this scan — append. REQ-AESA-004.
            scanDetectionCache_.push_back(d);
        }
        else
        {
            // Update existing entry with latest detection data.
            // Most recent dwell gives the best position estimate. REQ-AESA-004.
            for (auto& existing : scanDetectionCache_)
            {
                if (existing.targetID == d.targetID)
                {
                    existing = d;
                    break;
                }
            }
        }
    }

    // Publish the accumulated cache as the current detections output. REQ-AESA-004.
    latestOutput_.detections = scanDetectionCache_;

    // At scan boundary, flush the cache. The next scan starts fresh.
    // REQ-AESA-004.
    if (scanBnd)
    {
        scanDetectionCache_.clear();
    }

    // -------------------------------------------------------------------------
    // Step 16: Assemble scalar output fields.
    // These are published every tick regardless of mode. REQ-AESA-004.
    // -------------------------------------------------------------------------
    latestOutput_.currentAzimuth   = antenna_->currentAzimuth();
    latestOutput_.currentElevation = antenna_->currentElevation();
    latestOutput_.mode             = config_.mode;
    latestOutput_.displayRange_km  = cachedDisplayRange_km_;
    latestOutput_.currentTask      = beam.task;

    // Duty cycle is computed from the current beam waveform. REQ-AESA-020.
    latestOutput_.currentDutyCycle = scheduler_->currentDutyCycle();

    // -------------------------------------------------------------------------
    // Step 17: Mode-dependent track output assembly.
    // Output cadence is intentionally different per mode:
    //
    //   SURVEILLANCE — tracker not running. Clear tracks every tick.
    //
    //   TWS — tracks published at scan boundary only after the first full
    //         scan completes. Between boundaries the display holds the last
    //         complete scan picture. This keeps detections and tracks
    //         temporally consistent — the display never shows a track from
    //         beam N+1 alongside detections from beam N-3. REQ-AESA-004.
    //
    //   LOCK_ON — fire-control loop needs the latest Kalman state every tick
    //             for minimum latency. Detections also published every tick
    //             because the scan boundary rarely fires in this mode
    //             (all beam time is on one target). REQ-AESA-003, REQ-AESA-004.
    // -------------------------------------------------------------------------
    switch (config_.mode)
    {
    case RadarMode::SURVEILLANCE:
        // No tracking in SURVEILLANCE. Always clear. REQ-AESA-003.
        latestOutput_.tracks.clear();
        break;

    case RadarMode::TWS:
        // Set firstScanComplete_ flag at scan boundary. REQ-AESA-003.
        if (scanBnd) firstScanComplete_ = true;

        // Suppress track output until first scan is complete. REQ-AESA-004.
        if (firstScanComplete_)
        {
            tracker_->getValidatedTracks(latestOutput_.tracks);
        }
        break;

    case RadarMode::LOCK_ON:
        // Publish latest Kalman track every tick — fire-control latency
        // requirement. REQ-AESA-003.
        tracker_->getValidatedTracks(latestOutput_.tracks);

        // Also flush raw detections every tick in LOCK_ON — the scan boundary
        // rarely fires when all beam time is concentrated on one target.
        if (!scanDets.empty())
        {
            latestOutput_.detections = scanDets;
        }
        break;
    }

    // Publish ESM intercepts collected this scan. REQ-AESA-040.
    library_->getIntercepts(latestOutput_.intercepts);

    // -------------------------------------------------------------------------
    // Step 18: Rebuild beam schedule at scan boundary.
    // Updates track beam positions based on latest Kalman predictions and
    // adds/removes track beams as tracks are validated or dropped. REQ-AESA-010.
    // -------------------------------------------------------------------------
    if (scanBnd)
    {
        rebuildSchedule();
    }
}

// =============================================================================
// SECTION 8: PER-TARGET DETECTION PIPELINE
// REQ-AESA-040
// =============================================================================

// =============================================================================
// FUNCTION: processTargetDetection
// Full description in header.
// =============================================================================
bool RadarModel_AESA::processTargetDetection(
    const TargetInput& target, const BeamRequest& beam,
    double /*dt*/, double simTime,
    double maxUnambiguousRange, double maxUnambiguousRange2,
    std::vector<DetectionOutput>& scanDets,
    std::normal_distribution<double>& rNoise,
    std::normal_distribution<double>& azNoise,
    std::normal_distribution<double>& elNoise,
    std::normal_distribution<double>& dvNoise)
{
    // -------------------------------------------------------------------------
    // Gate 1: Category filter.
    // Reject targets whose surface type does not match the configured
    // detection category. Applied before any physics computation to minimise
    // processing load. REQ-AESA-040.
    // -------------------------------------------------------------------------
    if (config_.targetCategory == DetectionCategory::AIR_ONLY &&
        target.surface != SurfaceType::AIR)
    {
        return false;
    }
    if (config_.targetCategory == DetectionCategory::SURFACE_ONLY &&
        target.surface == SurfaceType::AIR)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Gate 2: Minimum detectable range.
    // Targets inside the minimum range are in the blind zone created by the
    // transmit pulse — the receiver is blanked during transmission. REQ-AESA-040.
    // -------------------------------------------------------------------------
    double range = std::sqrt(target.x * target.x +
                             target.y * target.y +
                             target.z * target.z);
    if (range < config_.minDetectableRange) return false;

    // -------------------------------------------------------------------------
    // Gate 3: Horizon check.
    // Targets beyond the radar and target horizon cannot be detected due to
    // Earth curvature. Uses 4/3 earth radius model. REQ-AESA-071.
    // -------------------------------------------------------------------------
    if (!signal_->checkHorizon(range, target.z, config_)) return false;

    // -------------------------------------------------------------------------
    // Gate 4: Occlusion check.
    // Compute knife-edge diffraction loss from all other platforms between
    // the radar and this target. If the target is in the SHADOW zone (>= 40 dB
    // loss), reject it entirely. REQ-AESA-070.
    // -------------------------------------------------------------------------
    OcclusionResult occlusion = computeOcclusion(
        target, currentWorldInputs_, config_);

    // Debug output for non-LIT targets. Retained for certification evidence —
    // occlusion events must be observable during validation testing. REQ-AESA-070.
    if (occlusion.zone != OcclusionResult::Zone::LIT)
    {
        qDebug() << "[OCCLUSION] Target:" << target.id
                 << "Zone:"      << static_cast<int>(occlusion.zone)
                 << "Loss_dB:"   << occlusion.diffractionLoss_dB
                 << "PowerFactor:" << occlusion.powerReduction;
    }

    // Target in SHADOW zone — fully blocked. Reject. REQ-AESA-070.
    if (occlusion.zone == OcclusionResult::Zone::SHADOW) return false;

    // -------------------------------------------------------------------------
    // Gate 5: Beam gate — target must be within the effective beam pattern.
    // Use effective beamwidth (natural * spoilFactor) for the gate threshold.
    // azDiff and elDiff are also used for beam gain and monopulse computations.
    // REQ-AESA-010.
    // -------------------------------------------------------------------------

    // Compute target azimuth and elevation in body frame.
    // atan2 gives az in (-180, +180] — body-frame signed convention. REQ-AESA-040.
    double tAz = std::atan2(target.y, target.x) * RAD_TO_DEG;
    double tEl = std::asin(std::clamp(target.z / range,
                                      DOT_CLAMP_MIN, DOT_CLAMP_MAX))
                 * RAD_TO_DEG;

    double azDiff, elDiff;
    if (!signal_->isTargetInBeam(antenna_->currentAzimuth(),
                                 antenna_->currentElevation(),
                                 tAz, tEl, config_,
                                 azDiff, elDiff,
                                 antenna_->effectiveBeamWidth()))
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Gate 6: Sidelobe blanking.
    // If a jammer is present in the sidelobe region and its power exceeds the
    // sidelobeBlanking_dB threshold, blank this detection. Prevents sidelobe
    // jamming from creating false detections. REQ-AESA-040.
    // -------------------------------------------------------------------------
    if (signal_->isJammerInSidelobe(azDiff, elDiff, target, config_))
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Gate 7: Doppler blind zone / STAP recovery.
    // Compute raw radial velocity (before noise). If target is in the MTI
    // clutter notch, determine whether STAP can recover it. REQ-AESA-040.
    // -------------------------------------------------------------------------
    double radVelRaw = (target.vx * target.x +
                        target.vy * target.y +
                        target.vz * target.z)
                       / std::max(range, DRFM_MIN_RANGE_M);

    bool inBlind = false;

    // Doppler gate only applies outside LOCK_ON — in fire-control mode the
    // tracker provides a velocity prediction that overrides the blind zone.
    // REQ-AESA-040.
    if (config_.mode != RadarMode::LOCK_ON)
    {
        bool inStapNotch = signal_->isInClutterNotchSTAP(
            radVelRaw, config_, beam.waveform);
        bool inMtiNotch  = signal_->isInDopplerBlindZone(
            radVelRaw, config_, beam.waveform);

        if (inMtiNotch && !inStapNotch)
        {
            // Target is in MTI notch but STAP can recover it.
            // Only STAP-capable arrays (>= 100 elements) can do this.
            // Arrays with fewer elements do not have enough spatial DOF.
            // REQ-AESA-040.
            if (config_.numElements < 100)
            {
                // STAP not available — reject unless HPRF can be used.
                if (beam.waveform.mode != WaveformMode::HPRF) return false;
                inBlind = true;
            }
            // For capable arrays: fall through — STAP gain applied to SINR below.
        }
        else if (inMtiNotch && inStapNotch)
        {
            // Target is in both MTI notch and STAP notch — fully blind.
            // Only HPRF can recover by operating at a different clutter geometry.
            if (beam.waveform.mode != WaveformMode::HPRF) return false;
            inBlind = true;
        }
    }

    // -------------------------------------------------------------------------
    // Step A: Array gain computation.
    // Compute the off-boresight steering angle from current beam to target,
    // then compute total array gain including element pattern, array factor,
    // spoiling, and failed module degradation. REQ-AESA-012.
    // Also apply beam gain factor for targets at the edge of the beam pattern.
    // -------------------------------------------------------------------------
    double steer = antenna_->computeSteeringAngle(
        antenna_->currentAzimuth(), antenna_->currentElevation(),
        tAz, tEl);

    double gain = antenna_->computeArrayGain(
                      steer, config_, antenna_->currentSpoilFactor())
                  * signal_->computeBeamGainFactor(
                      azDiff, elDiff, config_,
                      antenna_->effectiveBeamWidth());

    // -------------------------------------------------------------------------
    // Step B: Waveform selection.
    // For SEARCH beams, select the waveform appropriate for the target range.
    // For TRACK and FIRE_CONTROL beams, use the waveform specified in the
    // beam request — the scheduler has already optimised it. REQ-AESA-020.
    // -------------------------------------------------------------------------
    BeamWaveform wf = signal_->selectWaveformForRange(range, config_);
    if (beam.task != BeamRequest::Task::SEARCH)
    {
        wf = beam.waveform;
    }

    // -------------------------------------------------------------------------
    // Step C: Effective RCS computation.
    // Applies Physical Optics 6-facet model (if dimensions.valid) or table
    // interpolation, plus Swerling fluctuation, aspect-angle weighting,
    // multipath factor, and occlusion power reduction. REQ-AESA-040.
    // -------------------------------------------------------------------------
    double effRCS = signal_->computeEffectiveRCS(target, range,
                                                 config_.frequency_Hz);

    // Two-ray multipath factor — applies constructive/destructive interference
    // at low elevation angles over a reflecting surface. REQ-AESA-072.
    effRCS *= signal_->computeMultipathFactor(range, tEl, target.z, config_);

    // Occlusion penumbra power reduction — attenuates RCS by diffraction loss.
    // In SHADOW zone the target is already rejected above. REQ-AESA-070.
    effRCS *= occlusion.powerReduction;

    // -------------------------------------------------------------------------
    // Step D: Chaff clutter power in current beam. REQ-AESA-061.
    // -------------------------------------------------------------------------
    double chaffPwr = signal_->computeChaffReturn(
        antenna_->currentAzimuth(), antenna_->currentElevation(),
        chaffClouds_, simTime, config_);

    // -------------------------------------------------------------------------
    // Step E: Received signal power and SINR.
    // SINR = (Pr * processing_gain * integration_gain) / (Pn + Pc + Pj).
    // REQ-AESA-040.
    // -------------------------------------------------------------------------
    double Pr   = signal_->calculateSignalStrength(range, effRCS, gain,
                                                 wf, config_);
    double sinr = signal_->computeSINR(Pr, range, target.surface,
                                       target, config_, wf);

    // Apply STAP improvement gain. For targets outside the clutter notch,
    // STAP provides processing gain proportional to array elements * pulses.
    // For targets in the notch, partial recovery proportional to distance
    // from notch centre. REQ-AESA-040.
    double stapGain = signal_->computeSTAPGain(
        radVelRaw,
        static_cast<double>(config_.platformSpeed_m_s),
        wf, config_);
    sinr *= stapGain;

    // Reduce SINR by chaff clutter power. Chaff enters the SINR denominator
    // as an additional interference term. REQ-AESA-061.
    if (chaffPwr > 0.0)
    {
        double Pn = signal_->computeNoisePower(
            config_, static_cast<double>(wf.bandwidth_Hz));
        if (Pn + chaffPwr > 0.0)
        {
            // Scale SINR by noise / (noise + chaff) — chaff dilutes SNR.
            sinr *= Pn / (Pn + chaffPwr);
        }
    }

    // -------------------------------------------------------------------------
    // Gate 8: CFAR threshold.
    // Compare SINR against the CFAR threshold. Targets below threshold are
    // not detected — CFAR maintains a constant false alarm rate. REQ-AESA-040.
    // Slow targets (|radVel| < threshold) near the clutter notch receive the
    // relaxed threshold to reduce false alarms from residual clutter. REQ-AESA-040.
    // -------------------------------------------------------------------------
    auto   cells  = signal_->generateReferenceCells(target.surface, config_);
    double radVel = signal_->computeRadialVelocity(target, range, dvNoise);

    double thresh = (std::abs(radVel) < CFAR_RELAXED_RADVEL_THRESHOLD)
                        ? signal_->computeCFARThresholdRelaxed(cells, config_)
                        : signal_->computeCFARThreshold(cells, config_);

    // SINR at or below threshold — not detected. REQ-AESA-040.
    if (sinr <= thresh) return false;

    // =========================================================================
    // Detection declared. Build the DetectionOutput. REQ-AESA-040.
    // =========================================================================
    DetectionOutput det;
    det.targetID       = target.id;

    // Apply measurement noise to azimuth and elevation. REQ-AESA-040.
    det.azimuth        = tAz  + azNoise(tl_rng);
    det.elevation      = tEl  + elNoise(tl_rng);
    det.snr            = sinr;
    det.radialVelocity = radVel;
    det.inDopplerBlind = inBlind;

    // Apply monopulse angle correction. Monopulse refines angle estimate
    // beyond the beamwidth limit using the amplitude difference between
    // sum and difference beam channels. REQ-AESA-040.
    signal_->computeMonopulseAngleError(azDiff, elDiff, sinr, config_,
                                        det.azError_deg, det.elError_deg);
    det.azimuth   += det.azError_deg;
    det.elevation += det.elError_deg;

    // Compute derived kinematic parameters from target state. REQ-AESA-004.
    signal_->computeTargetMotionParams(det, target, range);
    signal_->computeCPA(det, target, range);

    // Albersheim probability of kill estimate. REQ-AESA-040.
    det.Pk = signal_->computePk(sinr, config_.targetPfa,
                                wf.pulsesPerDwell, target.swerlingCase);

    // -------------------------------------------------------------------------
    // Step F: Range ambiguity resolution.
    // Apply folded range measurement and attempt staggered PRF resolution.
    // REQ-AESA-021.
    // -------------------------------------------------------------------------
    signal_->applyRangeAmbiguity(det, range, maxUnambiguousRange,
                                 maxUnambiguousRange2, rNoise);

    // Staggered velocity resolver — only runs when prf2_Hz is configured.
    // Takes two independent Doppler measurements and uses the Chinese Remainder
    // Theorem coincidence detector to resolve velocity ambiguity. REQ-AESA-021.
    if (maxUnambiguousRange2 > RMAX2_MIN_FOR_VELOCITY_RESOLVE &&
        wf.prf2_Hz > PRF2_MIN_FOR_VELOCITY_RESOLVE)
    {
        double lambda = SPEED_OF_LIGHT / config_.frequency_Hz;
        double Vmax1  = lambda * static_cast<double>(wf.prf_Hz)  / 2.0;
        double Vmax2  = lambda * static_cast<double>(wf.prf2_Hz) / 2.0;

        // Second independent Doppler measurement from interleaved PRF.
        double radVel2 = signal_->computeRadialVelocity(target, range, dvNoise);

        // Resolve velocity using coincidence detector. REQ-AESA-021.
        det.radialVelocity = signal_->resolveVelocityStaggered(
            det.radialVelocity, radVel2, Vmax1, Vmax2, radVelRaw);
    }

    // Resolve folded range using true slant range as the prediction. REQ-AESA-021.
    if (det.isAmbiguous)
    {
        det.range       = signal_->resolveRangeAmbiguity(det.range, range,
                                                   maxUnambiguousRange);
        det.isAmbiguous = false;
    }

    // Fire-control specific range resolution using Kalman-predicted track range
    // as the prior. Provides tighter resolution than modulo-Rmax alone. REQ-AESA-021.
    if (config_.mode == RadarMode::LOCK_ON)
    {
        signal_->resolveRangeForLockOn(det, range, maxUnambiguousRange,
                                       target.id, tracker_->database());
    }
    else if (!det.isAmbiguous && det.range < config_.minDetectableRange)
    {
        // Floor range at minDetectableRange — prevents sub-minimum range
        // detections appearing in the output. REQ-AESA-040.
        det.range = config_.minDetectableRange;
    }

    // -------------------------------------------------------------------------
    // Gate 9: Detection merge guard.
    // Prevent duplicate detections of the same target from adjacent beam
    // positions within the same scan. Merges are based on range proximity
    // and angular separation. REQ-AESA-040.
    // -------------------------------------------------------------------------
    if (signal_->shouldMergeDetection(det, scanDets, config_)) return false;

    // -------------------------------------------------------------------------
    // Commit detection to output vector. REQ-AESA-040.
    // -------------------------------------------------------------------------
    scanDets.push_back(det);

    // Accumulate signal intercept for ESM library. isDRFMGhost = false —
    // accumulate the real detection, not the ghost. REQ-AESA-040.
    library_->accumulate(target.id, Pr, simTime, config_, wf, false);

    // -------------------------------------------------------------------------
    // Step G: Electronic warfare ghost injection.
    // Inject DRFM and/or RGPO/VGPO ghost detections if the target has an
    // active EW jammer. REQ-AESA-060.
    // -------------------------------------------------------------------------

    // DRFM gate-stealing ghost — creates a false target walking away in range
    // and velocity from the real target. REQ-AESA-060.
    if (target.jammer.active && target.jammer.type == JammerType::DRFM)
    {
        injectDRFMGhost(target, beam, simTime, maxUnambiguousRange,
                        scanDets, rNoise, azNoise, elNoise, dvNoise);
    }

    // RGPO (range) and VGPO (velocity) ghost injection. REQ-AESA-060.
    if (target.jammer.active &&
        (target.jammer.rgpoActive || target.jammer.vgpoActive))
    {
        injectRGPOVGPO(target, beam, simTime, 0.05, maxUnambiguousRange,
                       scanDets, rNoise, azNoise, elNoise, dvNoise);
    }

    return true;
}

// =============================================================================
// SECTION 9: ELECTRONIC WARFARE GHOST INJECTION
// REQ-AESA-060
// =============================================================================

// =============================================================================
// FUNCTION: injectDRFMGhost
// Full description in header.
// =============================================================================
void RadarModel_AESA::injectDRFMGhost(
    const TargetInput& real, const BeamRequest& beam,
    double simTime, double maxUnambiguousRange,
    std::vector<DetectionOutput>& scanDets,
    std::normal_distribution<double>& rNoise,
    std::normal_distribution<double>& azNoise,
    std::normal_distribution<double>& elNoise,
    std::normal_distribution<double>& dvNoise)
{
    const auto& j = real.jammer;

    // Only inject ghost if gate-stealing is active. REQ-AESA-060.
    if (!j.gateStealingActive) return;

    // Advance the DRFM pull-off accumulator for this target.
    // Pull-off rate is in m/s, DRFM_DWELL_S is the fixed dwell time.
    // Cap at DRFM_RMAX_MULTIPLIER * Rmax to prevent unbounded range growth.
    // REQ-AESA-060.
    drfmPullOff_[real.id] += static_cast<double>(j.drfmPullOffRate_m_s)
                             * DRFM_DWELL_S;
    drfmPullOff_[real.id]  = std::min(drfmPullOff_[real.id],
                                     DRFM_RMAX_MULTIPLIER * maxUnambiguousRange);

    // Compute real target range. Guard against zero range. REQ-AESA-060.
    double range = std::sqrt(real.x * real.x +
                             real.y * real.y +
                             real.z * real.z);
    if (range < DRFM_MIN_RANGE_M) return;

    // Ghost appears at same azimuth and elevation as real target.
    double gAz = std::atan2(real.y, real.x) * RAD_TO_DEG;
    double gEl = std::asin(std::clamp(real.z / range,
                                      DOT_CLAMP_MIN, DOT_CLAMP_MAX))
                 * RAD_TO_DEG;

    // Build ghost detection. REQ-AESA-060.
    DetectionOutput ghost;
    ghost.targetID = real.id;

    // Ghost range = real range + accumulated pull-off + noise. REQ-AESA-060.
    ghost.range    = range + drfmPullOff_[real.id] + rNoise(tl_rng);

    ghost.azimuth   = gAz + azNoise(tl_rng);
    ghost.elevation = gEl + elNoise(tl_rng);

    // DRFM velocity: negate true radial velocity (creates a false heading)
    // and add fixed velocity offset. REQ-AESA-060.
    ghost.radialVelocity = -(real.vx * real.x +
                             real.vy * real.y +
                             real.vz * real.z) / range
                           + static_cast<double>(j.drfmVelocityOffset_m_s)
                           + dvNoise(tl_rng);

    // Ghost SNR is weaker than real return — DRFM repeater has phase noise.
    // If scanDets is empty (no real detection), use unit SNR as fallback.
    // REQ-AESA-060.
    ghost.snr         = scanDets.empty() ? 1.0
                                 : DRFM_GHOST_SNR_SCALE * scanDets.back().snr;
    ghost.isDRFMGhost = true;
    ghost.isAmbiguous = (ghost.range > maxUnambiguousRange);
    ghost.Pk          = 0.0;   // Ghost has no real Pk — never engage. REQ-AESA-060.

    // Append ghost only if it would not be merged with an existing detection.
    // If merged, the ghost is suppressed — the CFAR merging logic prevents
    // the ghost from flooding the detection list. REQ-AESA-060.
    if (!signal_->shouldMergeDetection(ghost, scanDets, config_))
    {
        scanDets.push_back(ghost);

        // Accumulate ghost in ESM library with isDRFMGhost = true — library
        // tracks DRFM intercepts separately. REQ-AESA-060.
        library_->accumulate(real.id, 0.0, simTime, config_,
                             beam.waveform, true);
    }
}

// =============================================================================
// FUNCTION: injectRGPOVGPO
// Full description in header.
// =============================================================================
void RadarModel_AESA::injectRGPOVGPO(
    const TargetInput& real, const BeamRequest& /*beam*/,
    double /*simTime*/, double dt, double maxUnambiguousRange,
    std::vector<DetectionOutput>& scanDets,
    std::normal_distribution<double>& rNoise,
    std::normal_distribution<double>& azNoise,
    std::normal_distribution<double>& elNoise,
    std::normal_distribution<double>& dvNoise)
{
    const auto& j = real.jammer;

    // Compute real target range. Guard against zero range. REQ-AESA-060.
    double range = std::sqrt(real.x * real.x +
                             real.y * real.y +
                             real.z * real.z);
    if (range < DRFM_MIN_RANGE_M) return;

    // Ghost appears at same direction as real target. REQ-AESA-060.
    double gAz = std::atan2(real.y, real.x) * RAD_TO_DEG;
    double gEl = std::asin(std::clamp(real.z / range,
                                      DOT_CLAMP_MIN, DOT_CLAMP_MAX))
                 * RAD_TO_DEG;

    // -------------------------------------------------------------------------
    // RGPO: Range Gate Pull-Off.
    // Creates a false target that walks away from the real target in range
    // at rgpoRate_m_s. The tracker acquires the ghost instead of the real
    // target — a successful RGPO breaks the track. REQ-AESA-060.
    // -------------------------------------------------------------------------
    if (j.rgpoActive)
    {
        // Advance RGPO pull-off. Cap at maximum configured offset. REQ-AESA-060.
        rgpoPullOff_[real.id] += static_cast<double>(j.rgpoRate_m_s) * dt;
        rgpoPullOff_[real.id]  = std::min(
            rgpoPullOff_[real.id],
            static_cast<double>(j.rgpoMaxOffset_m));

        DetectionOutput ghost;
        ghost.targetID = real.id;

        // Ghost range walks away from real target by accumulated pull-off.
        ghost.range    = range + rgpoPullOff_[real.id] + rNoise(tl_rng);

        ghost.azimuth        = gAz + azNoise(tl_rng);
        ghost.elevation      = gEl + elNoise(tl_rng);

        // Ghost radial velocity matches real target (RGPO is range-only pull-off).
        ghost.radialVelocity = (real.vx * real.x +
                                real.vy * real.y +
                                real.vz * real.z) / range
                               + dvNoise(tl_rng);

        // RGPO ghost is slightly stronger than real — jammer amplifies the
        // stolen gate to attract the tracker. REQ-AESA-060.
        ghost.snr         = scanDets.empty() ? 1.0
                                     : scanDets.back().snr * RGPO_GHOST_SNR_SCALE;
        ghost.isDRFMGhost = true;
        ghost.isAmbiguous = (ghost.range > maxUnambiguousRange);
        ghost.Pk          = 0.0;

        if (!signal_->shouldMergeDetection(ghost, scanDets, config_))
        {
            scanDets.push_back(ghost);
        }
    }

    // -------------------------------------------------------------------------
    // VGPO: Velocity Gate Pull-Off.
    // Creates a false Doppler target that walks away from the real target's
    // velocity. The tracker acquires the ghost Doppler — a successful VGPO
    // causes the radar to lose the real target's velocity track. REQ-AESA-060.
    // -------------------------------------------------------------------------
    if (j.vgpoActive)
    {
        // Advance VGPO velocity offset. Cap at maximum configured offset.
        // vgpoRate_m_s2 is an acceleration (m/s^2) — multiply by dt to get
        // velocity increment. REQ-AESA-060.
        vgpoPullOff_[real.id] += static_cast<double>(j.vgpoRate_m_s2) * dt;
        vgpoPullOff_[real.id]  = std::min(
            vgpoPullOff_[real.id],
            static_cast<double>(j.vgpoMaxOffset_m_s));

        DetectionOutput ghost;
        ghost.targetID = real.id;

        // Ghost range is unshifted — VGPO is velocity-only pull-off. REQ-AESA-060.
        ghost.range    = range + rNoise(tl_rng);

        ghost.azimuth   = gAz + azNoise(tl_rng);
        ghost.elevation = gEl + elNoise(tl_rng);

        // Ghost velocity = real radial velocity + accumulated VGPO offset.
        ghost.radialVelocity = (real.vx * real.x +
                                real.vy * real.y +
                                real.vz * real.z) / range
                               + vgpoPullOff_[real.id]
                               + dvNoise(tl_rng);

        // VGPO ghost slightly stronger — attracts Doppler tracker. REQ-AESA-060.
        ghost.snr         = scanDets.empty() ? 1.0
                                     : scanDets.back().snr * VGPO_GHOST_SNR_SCALE;
        ghost.isDRFMGhost = true;
        ghost.isAmbiguous = false;   // VGPO ghost is at true range — not ambiguous.
        ghost.Pk          = 0.0;

        if (!signal_->shouldMergeDetection(ghost, scanDets, config_))
        {
            scanDets.push_back(ghost);
        }
    }
}

// =============================================================================
// SECTION 10: OCCLUSION MODEL
// REQ-AESA-070
// =============================================================================

// =============================================================================
// FUNCTION: computeKnifeEdgeDiffraction
// Full description in header.
// =============================================================================
double RadarModel_AESA::computeKnifeEdgeDiffraction(double nu) const
{
    // nu < KE_LIT_BOUNDARY: target is in the lit region (> 0.78 Fresnel radii
    // of clearance). No diffraction loss. ITU-R P.526-15 Sec 4.1. REQ-AESA-070.
    if (nu < KE_LIT_BOUNDARY) return 0.0;

    double J_dB;

    if (nu <= KE_POLY_LIMIT)
    {
        // Polynomial regime: nu in [-0.78, 2.4].
        // J(nu) = KE_A0 + KE_A1*nu + KE_A2*nu^2
        // Accuracy: within 0.4 dB of exact Fresnel integral. REQ-AESA-070.
        J_dB = KE_A0 + KE_A1 * nu + KE_A2 * nu * nu;
    }
    else
    {
        // Asymptotic regime: nu > 2.4 (deep shadow).
        // J(nu) = 20*log10(nu) + KE_ASYMPTOTE
        // Converges to exact Fresnel integral at large nu. REQ-AESA-070.
        J_dB = 20.0 * std::log10(nu) + KE_ASYMPTOTE;
    }

    // Floor at 0.0 dB — diffraction cannot produce gain, only loss.
    // A negative J_dB would indicate a numerical error in nu computation.
    // REQ-AESA-070.
    return std::max(0.0, J_dB);
}

// =============================================================================
// FUNCTION: computeOcclusion
// Full description in header.
// =============================================================================
OcclusionResult RadarModel_AESA::computeOcclusion(
    const TargetInput& candidate,
    const std::vector<TargetInput>& allTargets,
    const RadarConfig& cfg) const
{
    OcclusionResult result;  // Default: LIT, powerReduction=1.0, loss=0.0 dB

    // Compute candidate position and range from radar. REQ-AESA-070.
    double cx      = candidate.x;
    double cy      = candidate.y;
    double cz      = candidate.z;
    double d_total = std::sqrt(cx * cx + cy * cy + cz * cz);

    // Target at essentially zero range cannot be occluded. REQ-AESA-070.
    if (d_total < MIN_CANDIDATE_RANGE_M) return result;

    // Unit vector from radar toward candidate (line-of-sight direction).
    double ux = cx / d_total;
    double uy = cy / d_total;
    double uz = cz / d_total;

    // Radar wavelength for Fresnel zone radius computation. REQ-AESA-070.
    double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;

    // Accumulate total two-way diffraction loss over all occluders. REQ-AESA-070.
    double totalLoss_dB = 0.0;

    for (const auto& other : allTargets)
    {
        // Skip the candidate itself — a target cannot occlude itself. REQ-AESA-070.
        if (other.id == candidate.id) continue;

        double ox = other.x;
        double oy = other.y;
        double oz = other.z;

        // Distance from radar to the potential occluder. REQ-AESA-070.
        double d1 = std::sqrt(ox * ox + oy * oy + oz * oz);

        // Occluder must be between radar and candidate (d1 < d_total)
        // and at a meaningful distance from radar. REQ-AESA-070.
        if (d1 < MIN_OCCLUDER_SEGMENT_M || d1 >= d_total) continue;

        // Distance from occluder to candidate. REQ-AESA-070.
        double d2 = d_total - d1;
        if (d2 < MIN_OCCLUDER_SEGMENT_M) continue;

        // Perpendicular distance from occluder to line-of-sight.
        // dot = projection of occluder position onto LOS unit vector.
        // perp = component perpendicular to LOS. REQ-AESA-070.
        double dot   = ox * ux + oy * uy + oz * uz;
        double perpX = ox - dot * ux;
        double perpY = oy - dot * uy;
        double perpZ = oz - dot * uz;
        double perpDist = std::sqrt(perpX * perpX + perpY * perpY + perpZ * perpZ);

        // Estimate occluder effective radius.
        // Use physical dimensions if available, otherwise use platform-type defaults.
        // REQ-AESA-070.
        double occluderRadius;
        if (other.dimensions.valid)
        {
            // Use the smaller of width and height as the effective blocking radius.
            occluderRadius = 0.5 * std::min(other.dimensions.width,
                                            other.dimensions.height);
        }
        else if (other.platformType == "SHIP")
        {
            occluderRadius = OCCLUDER_RADIUS_SHIP_M;
        }
        else if (other.platformType == "BOMBER")
        {
            occluderRadius = OCCLUDER_RADIUS_BOMBER_M;
        }
        else
        {
            occluderRadius = OCCLUDER_RADIUS_DEFAULT_M;
        }

        // Obstacle clearance height h = occluderRadius - perpDist.
        // h > 0: LOS passes through the occluder (blocked).
        // h < 0: LOS clears the occluder by |h| metres. REQ-AESA-070.
        double h = occluderRadius - perpDist;

        // First Fresnel zone radius at the occluder position. REQ-AESA-070.
        double r1 = std::sqrt(lambda * d1 * d2 / (d1 + d2));

        // Clearance check: if LOS clears the occluder by more than
        // FRESNEL_CLEARANCE_FACTOR * r1 (0.577 Fresnel radii),
        // the target is in the lit region — skip this occluder. REQ-AESA-070.
        if (-h > FRESNEL_CLEARANCE_FACTOR * r1) continue;

        // Fresnel-Kirchhoff diffraction parameter nu. REQ-AESA-070.
        double nu      = h * std::sqrt(2.0 * (d1 + d2) / (lambda * d1 * d2));

        // One-way diffraction loss. Multiply by TWO_WAY_LOSS_FACTOR for
        // radar two-way path (transmit and receive both affected). REQ-AESA-070.
        double loss_dB = computeKnifeEdgeDiffraction(nu);
        totalLoss_dB  += TWO_WAY_LOSS_FACTOR * loss_dB;
    }

    // Populate result. REQ-AESA-070.
    result.diffractionLoss_dB = totalLoss_dB;

    // Linear power reduction factor: 10^(-loss_dB/10). REQ-AESA-070.
    result.powerReduction     = std::pow(10.0, -totalLoss_dB / 10.0);

    // Classify occlusion zone by total loss. REQ-AESA-070.
    if (totalLoss_dB >= SHADOW_THRESHOLD_DB)
    {
        result.zone = OcclusionResult::Zone::SHADOW;
    }
    else if (totalLoss_dB >= PENUMBRA_THRESHOLD_DB)
    {
        result.zone = OcclusionResult::Zone::PENUMBRA;
    }
    else
    {
        result.zone = OcclusionResult::Zone::LIT;
    }

    return result;
}

// =============================================================================
// SECTION 11: UTILITIES
// =============================================================================

// =============================================================================
// FUNCTION: rebuildSchedule
// Full description in header.
// =============================================================================
void RadarModel_AESA::rebuildSchedule()
{
    // Called from update() which holds mutex_. Do NOT acquire mutex_ here.
    // Rebuilds from current config_ and current tracker database. REQ-AESA-010.
    scheduler_->buildSchedule(config_, tracker_->database());
}

// =============================================================================
// FUNCTION: computeMaxDetectionRange (public, locking)
// Full description in header.
// =============================================================================
double RadarModel_AESA::computeMaxDetectionRange(double rcs) const
{
    // Public interface — must acquire mutex_ before calling locked version.
    std::lock_guard<std::mutex> lk(mutex_);
    return computeMaxDetectionRange_locked(rcs);
}

// =============================================================================
// FUNCTION: computeMaxDetectionRange_locked (private, non-locking)
// Full description in header.
// =============================================================================
double RadarModel_AESA::computeMaxDetectionRange_locked(double rcs) const
{
    // Called from update() (holds mutex_) and computeMaxDetectionRange() (locks
    // before calling). Must NOT acquire mutex_ here. REQ-AESA-040.
    return signal_->computeMaxDetectionRange(rcs, config_);
}

// =============================================================================
// FUNCTION: resolveRangeAmbiguity (public, locking)
// Full description in header.
// =============================================================================
double RadarModel_AESA::resolveRangeAmbiguity(double measured,
                                              double predicted,
                                              double Rmax) const
{
    // Public interface — must acquire mutex_. REQ-AESA-021.
    std::lock_guard<std::mutex> lk(mutex_);
    return signal_->resolveRangeAmbiguity(measured, predicted, Rmax);
}

} // namespace aesa

// =============================================================================
// SECTION 12: C ABI WRAPPERS
// REQ-AESA-001
// =============================================================================

extern "C"
{
// =========================================================================
// FUNCTION:    aesaradar_create
// DESCRIPTION: Allocates and returns a new RadarModel_AESA on the heap.
//              Caller takes ownership. Must be paired with aesaradar_destroy().
//              REQ-AESA-001.
// RETURNS:     Non-null pointer to a default-constructed RadarModel_AESA.
//              Returns nullptr only if heap allocation fails — which is
//              treated as a fatal system error in the simulation framework.
// =========================================================================
aesa::RadarModel_AESA* aesaradar_create()
{
    return new aesa::RadarModel_AESA();
}

// =========================================================================
// FUNCTION:    aesaradar_destroy
// DESCRIPTION: Destroys a RadarModel_AESA created by aesaradar_create().
//              REQ-AESA-001.
// PARAMETERS:
//   p  [in]  Pointer from aesaradar_create(). Must be non-null.
//            Passing null is undefined behaviour — caller must check.
// =========================================================================
void aesaradar_destroy(aesa::RadarModel_AESA* p)
{
    delete p;
}
}

