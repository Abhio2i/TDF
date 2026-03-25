// =============================================================================
// radarmodel.cpp  —  Radar simulation model — orchestrator
//
// All original pipeline steps (§5.0 – §5.9) are PRESERVED exactly.
// Additions vs original:
//   §5.2  selectActivePRF() — PRF cycling for STAGGERED/SWITCHED/JITTERED
//   §5.5  Category filter early-out (DetectionCategory)
//   §5.5  computeBeamGainFactor() applied to effective RCS
//   §5.5  library_->accumulate() called on each confirmed detection
//   §5.6  library_->pruneStale() called at scan boundary
//   §5.9  intercepts assembled into latestOutput_
//   §10   loadSignalLibrary() public method
//   §11   selectActivePRF() private helper
// =============================================================================

#include "radarmodel.h"

#include "radarsignalprocessor.h"  // IWYU pragma: keep
#include "radarantenna.h"          // IWYU pragma: keep
#include "radartracker.h"          // IWYU pragma: keep
#include "radarsignallibrary.h"    // IWYU pragma: keep  ← new 4th sub-object

#include <algorithm>
#include <cmath>
#include <random>
#include <QDebug>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr double SPEED_OF_LIGHT = 299792458.0;

static thread_local std::default_random_engine tl_rng{ std::random_device{}() };

// =============================================================================
// Construction / destruction
// =============================================================================

RadarModel::RadarModel()
    : signal_ (std::make_unique<RadarSignalProcessor>())
    , antenna_(std::make_unique<RadarAntenna>())
    , tracker_(std::make_unique<RadarTracker>())
    , library_(std::make_unique<RadarSignalLibrary>())
{}

RadarModel::~RadarModel() = default;

// =============================================================================
// §1  Lifecycle
// =============================================================================

void RadarModel::init(const RadarConfig& cfg)
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_            = cfg;
    displayRangeDirty_ = true;
    latestOutput_      = RadarOutput{};
    latestOutput_.mode = cfg.mode;

    tracker_->clear();
    antenna_->reset(cfg);
    library_->clear();          // ← new

    activePRFIndex_ = 0;        // ← new
    initialised_    = true;
    running_        = false;
}

void RadarModel::start()
{
    std::lock_guard<std::mutex> lk(mutex_);
    tracker_->clear();
    antenna_->reset(config_);
    library_->clear();          // ← new

    latestOutput_      = RadarOutput{};
    latestOutput_.mode = config_.mode;
    displayRangeDirty_ = true;
    activePRFIndex_    = 0;     // ← new
    running_           = true;
}

void RadarModel::end()
{
    std::lock_guard<std::mutex> lk(mutex_);
    running_ = false;
    tracker_->clear();
    library_->clear();          // ← new
    latestOutput_ = RadarOutput{};
}

void RadarModel::reset()
{
    RadarConfig saved;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        saved = config_;
    }
    init(saved);
    start();
}

// =============================================================================
// §2  Configuration
// =============================================================================

void RadarModel::setConfig(const RadarConfig& cfg)
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_            = cfg;
    displayRangeDirty_ = true;
}

RadarConfig RadarModel::getConfig() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return config_;
}

// =============================================================================
// §3  Mode control
// =============================================================================

void RadarModel::setMode(RadarMode mode)
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_.mode = mode;
}

void RadarModel::lockOn(uint32_t targetID)
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_.mode           = RadarMode::LOCK_ON;
    config_.lockedTargetID = targetID;
}

void RadarModel::breakLock()
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_.mode           = RadarMode::SURVEILLANCE;
    config_.lockedTargetID = 0;
}

// =============================================================================
// §4  Output snapshot
// =============================================================================

RadarOutput RadarModel::getOutput() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return latestOutput_;
}
// =============================================================================
// §13  Platform attitude compensation — body frame → world frame beam pointing
//
// The antenna az/el from RadarAntenna is in platform body frame.
// Target az/el computed in processTargetDetection() is in world frame.
// This method rotates the body-frame beam direction into world frame using
// the platform's heading, pitch, and roll from currentPose_.
//
// Rotation order: R = Rz(heading) · Ry(pitch) · Rx(roll)
// Applied to the body-frame beam unit vector.
// =============================================================================

void RadarModel::applyAttitudeToBeam(
    double bodyAz, double bodyEl,
    double& worldAz, double& worldEl) const
{
    // If attitude is negligible, skip the rotation entirely.
    // Threshold: 0.1 degrees — below this, rotation effect is sub-beamwidth.
    if (std::abs(currentPose_.roll)  < 0.1f &&
        std::abs(currentPose_.pitch) < 0.1f)
    {
        // Heading only — simple azimuth offset, no elevation change
        worldAz = bodyAz + static_cast<double>(currentPose_.heading);
        if (worldAz >= 360.0) worldAz -= 360.0;
        if (worldAz <    0.0) worldAz += 360.0;
        worldEl = bodyEl;
        return;
    }

    // ---- Step 1: body-frame az/el → unit vector -------------------------
    double azRad  = bodyAz * (M_PI / 180.0);
    double elRad  = bodyEl * (M_PI / 180.0);

    double bx = std::cos(elRad) * std::cos(azRad);  // Forward/East
    double by = std::cos(elRad) * std::sin(azRad);  // Right/North
    double bz = std::sin(elRad);                     // Up

    // ---- Step 2: apply roll (rotation around X / forward axis) ----------
    double rollRad  = static_cast<double>(currentPose_.roll)  * (M_PI / 180.0);
    double bx1 = bx;
    double by1 = by * std::cos(rollRad) - bz * std::sin(rollRad);
    double bz1 = by * std::sin(rollRad) + bz * std::cos(rollRad);

    // ---- Step 3: apply pitch (rotation around Y / right axis) -----------
    double pitchRad = static_cast<double>(currentPose_.pitch) * (M_PI / 180.0);
    double bx2 =  bx1 * std::cos(pitchRad) + bz1 * std::sin(pitchRad);
    double by2 =  by1;
    double bz2 = -bx1 * std::sin(pitchRad) + bz1 * std::cos(pitchRad);

    // ---- Step 4: apply heading (rotation around Z / up axis) ------------
    double headRad  = static_cast<double>(currentPose_.heading) * (M_PI / 180.0);
    double bx3 = bx2 * std::cos(headRad) - by2 * std::sin(headRad);
    double by3 = bx2 * std::sin(headRad) + by2 * std::cos(headRad);
    double bz3 = bz2;

    // ---- Step 5: world-frame unit vector → az/el ------------------------
    worldAz = std::atan2(by3, bx3) * (180.0 / M_PI);
    if (worldAz < 0.0) worldAz += 360.0;

    worldEl = std::asin(std::clamp(bz3, -1.0, 1.0)) * (180.0 / M_PI);
}
// =============================================================================
// §5  Main update — per-tick pipeline
// =============================================================================

void RadarModel::update(double                          dt,
                        const RadarPose&                pose,
                        const std::vector<TargetInput>& worldInputs,
                        double                          simTime)
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (!running_) return;

    // 5.0  Reset one-shot flags
    latestOutput_.lockBroken = false;

    // 5.1  Apply platform pose
    // if (pose.y > 50.0)
    //     config_.radarHeight = pose.y;
    if (pose.y > 50.0)
    {
        if (std::abs(pose.y - config_.radarHeight) > 1.0)
        {
            config_.radarHeight = pose.y;
            displayRangeDirty_  = true;
        }
    }

    currentPose_ = pose;   // ← store for use in processTargetDetection()

    // 5.2  PRF selection / cycling
    double currentPRF          = selectActivePRF();
    double maxUnambiguousRange = SPEED_OF_LIGHT / (2.0 * currentPRF);

    // 5.3  Kalman prediction
    if (config_.mode == RadarMode::TWS ||
        config_.mode == RadarMode::LOCK_ON)
        tracker_->predict(dt);

    // 5.4  Antenna positioning
    if (config_.mode == RadarMode::LOCK_ON)
    {
        if (!antenna_->lockOn(worldInputs, config_))
        {
            config_.mode           = RadarMode::SURVEILLANCE;
            config_.lockedTargetID = 0;
            latestOutput_.lockBroken = true;
        }
    }
    else
    {
        antenna_->update(dt, config_);
    }

    // 5.5  Per-target detection loop
    std::normal_distribution<double> rangeNoise  (0.0, config_.noise.rangeStdDev);
    std::normal_distribution<double> azNoise     (0.0, config_.noise.azimuthStdDev);
    std::normal_distribution<double> elNoise     (0.0, config_.noise.elevationStdDev);
    std::normal_distribution<double> dopplerNoise(0.0, config_.noise.dopplerStdDev);

    std::vector<DetectionOutput> scanDetections;
    scanDetections.reserve(worldInputs.size());

    bool lockedTargetVisible = false;

    for (const auto& target : worldInputs)
    {
        if (config_.mode == RadarMode::LOCK_ON &&
            target.id != config_.lockedTargetID)
            continue;

        bool detected = processTargetDetection(
            target, dt, simTime, maxUnambiguousRange,
            scanDetections,
            rangeNoise, azNoise, elNoise, dopplerNoise);

        if (detected &&
            config_.mode == RadarMode::LOCK_ON &&
            target.id    == config_.lockedTargetID)
            lockedTargetVisible = true;
    }

    // 5.6  Scan-miss logic (TWS, at scan boundary)
    if (config_.mode == RadarMode::TWS &&
        antenna_->scanBoundaryOccurred())
    {
        tracker_->applyScanMissLogic(simTime, config_);
        library_->pruneStale(simTime, config_.trackCoastSeconds); // ← new
    }

    // 5.7  Break-lock guard
    if (config_.mode == RadarMode::LOCK_ON && !lockedTargetVisible)
    {
        config_.mode           = RadarMode::SURVEILLANCE;
        config_.lockedTargetID = 0;
        latestOutput_.lockBroken = true;
    }

    // 5.8  Recompute display range if config changed
    if (displayRangeDirty_)
    {
        cachedDisplayRange_km_ = computeMaxDetectionRange_locked();
        cachedDisplayRange_km_ = std::max(5.0,
                                          std::min(1000.0, cachedDisplayRange_km_));
        displayRangeDirty_     = false;
    }

    // 5.9  Assemble output snapshot
    latestOutput_.detections       = std::move(scanDetections);
    latestOutput_.currentAzimuth   = antenna_->currentAzimuth();
    latestOutput_.currentElevation = antenna_->currentElevation();
    latestOutput_.mode             = config_.mode;
    latestOutput_.displayRange_km  = cachedDisplayRange_km_;

    tracker_->getValidatedTracks(latestOutput_.tracks);

    // Collect signal intercepts from the library              ← new
    library_->getIntercepts(latestOutput_.intercepts);
}

// =============================================================================
// §6  Per-target detection pipeline
// =============================================================================

bool RadarModel::processTargetDetection(
    const TargetInput&               target,
    double                           dt,
    double                           simTime,
    double                           maxUnambiguousRange,
    std::vector<DetectionOutput>&    scanDetections,
    std::normal_distribution<double>& rangeNoise,
    std::normal_distribution<double>& azNoise,
    std::normal_distribution<double>& elNoise,
    std::normal_distribution<double>& dopplerNoise)
{
    // ---- Category filter (NEW) ------------------------------------------
    if (config_.targetCategory == DetectionCategory::AIR_ONLY &&
        target.surface != SurfaceType::AIR)
        return false;

    if (config_.targetCategory == DetectionCategory::SURFACE_ONLY &&
        target.surface == SurfaceType::AIR)
        return false;

    // ---- Geometry pre-checks (original) ---------------------------------
    double range = std::sqrt(target.x*target.x +
                             target.y*target.y +
                             target.z*target.z);

    if (range < config_.minDetectableRange) return false;
    if (!signal_->checkHorizon(range, target.z, config_)) return false;

    double targetAz = std::atan2(target.y, target.x) * (180.0 / M_PI);
    if (targetAz < 0.0) targetAz += 360.0;
    if (range < 1e-6) return false;

    double ratio    = std::clamp(target.z / range, -1.0, 1.0);
    double targetEl = std::asin(ratio) * (180.0 / M_PI);

    // ---- Beam illumination check (original) -----------------------------
    // double azDiff, elDiff, scanMargin;
    // if (!signal_->isTargetInBeam(
    //         antenna_->currentAzimuth(), antenna_->currentElevation(),
    //         targetAz, targetEl, dt, config_,
    //         azDiff, elDiff, scanMargin))
    //     return false;
    double worldBeamAz, worldBeamEl;
    applyAttitudeToBeam(antenna_->currentAzimuth(), antenna_->currentElevation(),
                        worldBeamAz, worldBeamEl);

    double azDiff, elDiff, scanMargin;
    if (!signal_->isTargetInBeam(
            worldBeamAz, worldBeamEl,
            targetAz, targetEl, dt, config_,
            azDiff, elDiff, scanMargin))
        return false;

    // ---- Beam gain factor — sidelobe attenuation (NEW) ------------------
    double beamGainFactor = signal_->computeBeamGainFactor(azDiff, elDiff, config_);

    // ---- Signal chain (original, with gain factor applied to RCS) -------
    double effRCS = signal_->computeEffectiveRCS(target, range) * beamGainFactor;
    double Pr     = signal_->calculateSignalStrength(range, effRCS, config_);
    double sinr   = signal_->computeSINR(Pr, range, target.surface, target, config_);

    auto   cells    = signal_->generateReferenceCells(target.surface, config_);
    double radVel   = signal_->computeRadialVelocity(target, range, dopplerNoise);

    bool   stationary = (std::abs(radVel) < 5.0);
    double threshold  = stationary
                           ? signal_->computeCFARThresholdRelaxed(cells, config_)
                           : signal_->computeCFARThreshold(cells, config_);

    if (sinr <= threshold) return false;

    // ---- Build detection record (original) ------------------------------
    DetectionOutput det;
    det.targetID       = target.id;
    det.azimuth        = targetAz + azNoise(tl_rng);
    det.elevation      = targetEl + elNoise(tl_rng);
    det.snr            = sinr;
    det.radialVelocity = radVel;

    signal_->computeTargetMotionParams(det, target, range);
    signal_->computeCPA(det, target, range);
    det.Pk = signal_->computePk(range, det.radialVelocity);

    signal_->applyRangeAmbiguity(det, range, maxUnambiguousRange, rangeNoise);

    if (config_.mode == RadarMode::LOCK_ON)
    {
        signal_->resolveRangeForLockOn(det, range, maxUnambiguousRange,
                                       target.id, tracker_->database());
    }
    else if (!det.isAmbiguous && det.range < config_.minDetectableRange)
    {
        det.range = config_.minDetectableRange;
    }

    if (signal_->shouldMergeDetection(det, scanDetections, config_))
        return false;

    scanDetections.push_back(det);

    // ---- Signal library accumulation (NEW) ------------------------------
    library_->accumulate(target.id, Pr, simTime, config_);

    // ---- Track association (original) -----------------------------------
    if (config_.mode == RadarMode::TWS ||
        config_.mode == RadarMode::LOCK_ON)
    {
        double     prob  = 0.0;
        TrackFile* track = tracker_->findBestTrackMatch(
            det, maxUnambiguousRange, prob);

        if (track)
            tracker_->performKalmanUpdate(
                *track, det, simTime, dt, maxUnambiguousRange, config_);
        else
            tracker_->createNewTrack(
                det, target, maxUnambiguousRange, simTime, config_);
    }

    return true;
}

// =============================================================================
// §7  Maximum detection range
// =============================================================================

double RadarModel::computeMaxDetectionRange(double rcs) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return computeMaxDetectionRange_locked(rcs);
}

double RadarModel::computeMaxDetectionRange_locked(double rcs) const
{
    return signal_->computeMaxDetectionRange(rcs, config_);
}

// =============================================================================
// §8  Range ambiguity
// =============================================================================

double RadarModel::resolveRangeAmbiguity(
    double measured, double predicted, double Rmax) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return signal_->resolveRangeAmbiguity(measured, predicted, Rmax);
}

// =============================================================================
// §9  Output assembly placeholder  (retained for API parity)
// =============================================================================

void RadarModel::assembleFinalOutput()
{
    // Intentionally empty — assembled inline at the end of update().
}

// =============================================================================
// §10  Signal library loader  (NEW public method)
// =============================================================================

void RadarModel::loadSignalLibrary(const std::vector<SignalLibraryEntry>& entries)
{
    std::lock_guard<std::mutex> lk(mutex_);
    library_->loadLibrary(entries);
}

// =============================================================================
// §11  PRF selection helper  (NEW private, called with mutex_ held)
//
// FIXED      — always returns prfLevels[0]
// STAGGERED  — increments activePRFIndex_ each call, cycles through non-zero levels
// SWITCHED   — same cycling as STAGGERED (operator switch modelled as round-robin)
// JITTERED   — returns prfLevels[0] ± uniform 5 % jitter
// =============================================================================

double RadarModel::selectActivePRF()
{
    // Count valid (non-zero) PRF levels
    int validLevels = 0;
    for (int i = 0; i < 4; ++i)
        if (config_.prfLevels[i] > 0.0f) validLevels++;

    if (validLevels == 0) return 5000.0; // Hard fallback

    switch (config_.prfType)
    {
    case PRFType::STAGGERED:
    case PRFType::SWITCHED:
        if (validLevels > 1)
        {
            activePRFIndex_ = (activePRFIndex_ + 1) % validLevels;
            return static_cast<double>(config_.prfLevels[activePRFIndex_]);
        }
        // Only one level configured — fall through to FIXED
        [[fallthrough]];

    case PRFType::FIXED:
    default:
        return static_cast<double>(config_.prfLevels[0]);

    case PRFType::JITTERED:
    {
        // ±5 % uniform jitter around prfLevels[0]
        std::uniform_real_distribution<double> jitter(-0.05, 0.05);
        double base = static_cast<double>(config_.prfLevels[0]);
        return base * (1.0 + jitter(tl_rng));
    }
    }
}

// =============================================================================
// §12  C ABI
// =============================================================================

extern "C"
{
RadarModel* radarmodel_create()              { return new RadarModel();  }
void        radarmodel_destroy(RadarModel* p) { delete p;                }
}
// // =============================================================================
// // radarmodel.cpp  —  Radar simulation model — orchestrator
// //
// // This file is the ONLY place that knows about all three sub-objects together.
// // Its sole job is to:
// //   1. Own the mutex_, config_, and latestOutput_.
// //   2. Drive the per-tick pipeline by calling sub-objects in the correct order.
// //   3. Expose the clean public API declared in radarmodel.h.
// //
// // NO physics lives here.  Physics is in:
// //   RadarSignalProcessor  (radarsignalprocessor.cpp)
// //   RadarAntenna          (radarantenna.cpp)
// //   RadarTracker          (radartracker.cpp)
// //
// // Adding a new feature:
// //   • Physics change  → edit radarsignalprocessor.cpp
// //   • Scan pattern    → edit radarantenna.cpp
// //   • Tracking algo   → edit radartracker.cpp
// //   • New pipeline step → add a call here in update()
// // =============================================================================

// #include "radarmodel.h"

// // Sub-object headers — only this translation unit includes them.
// // radarmodel.h only forward-declares the three classes (pimpl pattern).
// // These are required here to complete unique_ptr<T> (destructor / sizeof)
// // and to call methods on signal_ / antenna_ / tracker_.
// // IWYU pragma: keep — suppress clangd "not used directly" warning.
// #include "radarsignalprocessor.h"  // IWYU pragma: keep
// #include "radarantenna.h"          // IWYU pragma: keep
// #include "radartracker.h"          // IWYU pragma: keep

// #include <algorithm>
// #include <cmath>
// #include <random>

// #ifndef M_PI
// #define M_PI 3.14159265358979323846
// #endif

// // -----------------------------------------------------------------------------
// // Speed of light — needed for max-unambiguous-range calculation
// // -----------------------------------------------------------------------------
// static constexpr double SPEED_OF_LIGHT = 299792458.0; // m/s

// // Thread-local RNG — noise distributions are instantiated per update() call.
// // Declared static so the symbol is internal to this translation unit and does
// // not collide with the identically-named tl_rng in radarsignalprocessor.cpp.
// static thread_local std::default_random_engine tl_rng{ std::random_device{}() };

// // =============================================================================
// // Construction / destruction
// // =============================================================================

// RadarModel::RadarModel()
//     : signal_ (std::make_unique<RadarSignalProcessor>())
//     , antenna_(std::make_unique<RadarAntenna>())
//     , tracker_(std::make_unique<RadarTracker>())
// {}

// // unique_ptr members need a destructor definition here (not in the header)
// // because the header only forward-declares the sub-object types.
// RadarModel::~RadarModel() = default;

// // =============================================================================
// // §1  Lifecycle
// // =============================================================================

// void RadarModel::init(const RadarConfig& cfg)
// {
//     std::lock_guard<std::mutex> lk(mutex_);

//     config_            = cfg;
//     displayRangeDirty_ = true;
//     latestOutput_      = RadarOutput{};
//     latestOutput_.mode = cfg.mode;

//     // Delegate sub-object initialisation
//     tracker_->clear();
//     antenna_->reset(cfg);

//     initialised_ = true;
//     running_     = false;
// }

// void RadarModel::start()
// {
//     std::lock_guard<std::mutex> lk(mutex_);

//     // Wipe dynamic state but keep the configuration
//     tracker_->clear();
//     antenna_->reset(config_);

//     latestOutput_      = RadarOutput{};
//     latestOutput_.mode = config_.mode;
//     displayRangeDirty_ = true;
//     running_           = true;
// }

// void RadarModel::end()
// {
//     std::lock_guard<std::mutex> lk(mutex_);
//     running_ = false;
//     tracker_->clear();
//     latestOutput_ = RadarOutput{};
// }

// void RadarModel::reset()
// {
//     // Save config under the lock, then re-init outside to avoid recursive lock
//     RadarConfig saved;
//     {
//         std::lock_guard<std::mutex> lk(mutex_);
//         saved = config_;
//     }
//     init(saved);
//     start();
// }

// // =============================================================================
// // §2  Configuration — hot-reload
// // =============================================================================

// void RadarModel::setConfig(const RadarConfig& cfg)
// {
//     std::lock_guard<std::mutex> lk(mutex_);
//     config_            = cfg;
//     displayRangeDirty_ = true;
// }

// RadarConfig RadarModel::getConfig() const
// {
//     std::lock_guard<std::mutex> lk(mutex_);
//     return config_;
// }

// // =============================================================================
// // §3  Mode control
// // =============================================================================

// void RadarModel::setMode(RadarMode mode)
// {
//     std::lock_guard<std::mutex> lk(mutex_);
//     config_.mode = mode;
// }

// void RadarModel::lockOn(uint32_t targetID)
// {
//     std::lock_guard<std::mutex> lk(mutex_);
//     config_.mode           = RadarMode::LOCK_ON;
//     config_.lockedTargetID = targetID;
// }

// void RadarModel::breakLock()
// {
//     std::lock_guard<std::mutex> lk(mutex_);
//     config_.mode           = RadarMode::SURVEILLANCE;
//     config_.lockedTargetID = 0;
// }

// // =============================================================================
// // §4  Output snapshot
// // =============================================================================

// RadarOutput RadarModel::getOutput() const
// {
//     std::lock_guard<std::mutex> lk(mutex_);
//     return latestOutput_;
// }

// // =============================================================================
// // §5  Main update — per-tick pipeline
// //
// // Execution order is critical:
// //   1. Reset one-shot flags
// //   2. Apply platform pose (altitude update for horizon model)
// //   3. Compute PRF / max-unambiguous range
// //   4. Tracker Kalman prediction (before detections arrive)
// //   5. Advance antenna (or slave to locked target)
// //   6. Per-target detection loop (signal chain + tracker association)
// //   7. Scan-miss logic (once per scan boundary)
// //   8. Break-lock guard (if locked target vanished)
// //   9. Recompute display range (if config changed)
// //  10. Assemble latestOutput_ snapshot
// // =============================================================================

// void RadarModel::update(double                          dt,
//                         const RadarPose&                pose,
//                         const std::vector<TargetInput>& worldInputs,
//                         double                          simTime)
// {
//     std::lock_guard<std::mutex> lk(mutex_);

//     if (!running_) return;

//     // ------------------------------------------------------------------
//     // 5.0  Reset one-shot flags from last tick
//     // ------------------------------------------------------------------
//     latestOutput_.lockBroken = false;

//     // ------------------------------------------------------------------
//     // 5.1  Apply platform pose
//     //      Only override radarHeight when the platform is clearly airborne.
//     //      Ground vehicles / ships sit at their configured height.
//     // ------------------------------------------------------------------
//     if (pose.y > 50.0)
//         config_.radarHeight = pose.y;

//     // ------------------------------------------------------------------
//     // 5.2  PRF / max unambiguous range
//     //      Rmax = c / (2 · PRF)
//     // ------------------------------------------------------------------
//     double currentPRF =
//         (config_.prfLevels[0] > 0.0f)
//             ? static_cast<double>(config_.prfLevels[0]) : 5000.0;
//     double maxUnambiguousRange = SPEED_OF_LIGHT / (2.0 * currentPRF);

//     // ------------------------------------------------------------------
//     // 5.3  Kalman prediction for all existing tracks
//     //      Must happen BEFORE detection so predictedRange is current.
//     // ------------------------------------------------------------------
//     if (config_.mode == RadarMode::TWS ||
//         config_.mode == RadarMode::LOCK_ON)
//         tracker_->predict(dt);

//     // ------------------------------------------------------------------
//     // 5.4  Antenna positioning
//     // ------------------------------------------------------------------
//     if (config_.mode == RadarMode::LOCK_ON)
//     {
//         // Slave beam to the locked target; if it returns false the target
//         // has left the visible set — break lock and revert to surveillance.
//         if (!antenna_->lockOn(worldInputs, config_))
//         {
//             config_.mode           = RadarMode::SURVEILLANCE;
//             config_.lockedTargetID = 0;
//             latestOutput_.lockBroken = true;
//         }
//     }
//     else
//     {
//         // Mechanical / conical sector scan
//         antenna_->update(dt, config_);
//     }

//     // ------------------------------------------------------------------
//     // 5.5  Per-target detection loop
//     // ------------------------------------------------------------------
//     std::normal_distribution<double> rangeNoise  (0.0, config_.noise.rangeStdDev);
//     std::normal_distribution<double> azNoise     (0.0, config_.noise.azimuthStdDev);
//     std::normal_distribution<double> elNoise     (0.0, config_.noise.elevationStdDev);
//     std::normal_distribution<double> dopplerNoise(0.0, config_.noise.dopplerStdDev);

//     std::vector<DetectionOutput> scanDetections;
//     scanDetections.reserve(worldInputs.size());

//     bool lockedTargetVisible = false;

//     for (const auto& target : worldInputs)
//     {
//         // In LOCK_ON mode only process the locked target
//         if (config_.mode == RadarMode::LOCK_ON &&
//             target.id != config_.lockedTargetID)
//             continue;

//         bool detected = processTargetDetection(
//             target, dt, simTime, maxUnambiguousRange,
//             scanDetections,
//             rangeNoise, azNoise, elNoise, dopplerNoise);

//         if (detected &&
//             config_.mode  == RadarMode::LOCK_ON &&
//             target.id     == config_.lockedTargetID)
//             lockedTargetVisible = true;
//     }

//     // ------------------------------------------------------------------
//     // 5.6  Per-scan miss / dropout logic (TWS only, at scan boundary)
//     // ------------------------------------------------------------------
//     if (config_.mode == RadarMode::TWS &&
//         antenna_->scanBoundaryOccurred())
//         tracker_->applyScanMissLogic(simTime, config_);

//     // ------------------------------------------------------------------
//     // 5.7  Break-lock guard — target vanished from detection list
//     // ------------------------------------------------------------------
//     if (config_.mode == RadarMode::LOCK_ON && !lockedTargetVisible)
//     {
//         config_.mode           = RadarMode::SURVEILLANCE;
//         config_.lockedTargetID = 0;
//         latestOutput_.lockBroken = true;
//     }

//     // ------------------------------------------------------------------
//     // 5.8  Recompute display range if config changed
//     //      Uses the unlocked internal worker — mutex_ is already held.
//     // ------------------------------------------------------------------
//     if (displayRangeDirty_)
//     {
//         cachedDisplayRange_km_ = computeMaxDetectionRange_locked();
//         cachedDisplayRange_km_ = std::max(5.0,
//                                           std::min(1000.0, cachedDisplayRange_km_));
//         displayRangeDirty_     = false;
//     }

//     // ------------------------------------------------------------------
//     // 5.9  Assemble output snapshot
//     // ------------------------------------------------------------------
//     latestOutput_.detections       = std::move(scanDetections);
//     latestOutput_.currentAzimuth   = antenna_->currentAzimuth();
//     latestOutput_.currentElevation = antenna_->currentElevation();
//     latestOutput_.mode             = config_.mode;
//     latestOutput_.displayRange_km  = cachedDisplayRange_km_;

//     // Collect all validated tracks from the tracker
//     tracker_->getValidatedTracks(latestOutput_.tracks);
// }

// // =============================================================================
// // §6  Per-target detection pipeline
// //
// // This method intentionally stays in RadarModel (not in a sub-object) because
// // it is the coordination point between signal_ and tracker_ — it calls both
// // and orchestrates the result.  Moving it would require cross-references
// // between sub-objects, which would defeat the modularity goal.
// // =============================================================================

// bool RadarModel::processTargetDetection(
//     const TargetInput&               target,
//     double                           dt,
//     double                           simTime,
//     double                           maxUnambiguousRange,
//     std::vector<DetectionOutput>&    scanDetections,
//     std::normal_distribution<double>& rangeNoise,
//     std::normal_distribution<double>& azNoise,
//     std::normal_distribution<double>& elNoise,
//     std::normal_distribution<double>& dopplerNoise)
// {
//     // ---- Geometry pre-checks ----
//     double range = std::sqrt(target.x*target.x +
//                              target.y*target.y +
//                              target.z*target.z);

//     if (range < config_.minDetectableRange) return false;
//     if (!signal_->checkHorizon(range, target.z, config_)) return false;

//     // Target azimuth and elevation in radar-local frame
//     double targetAz = std::atan2(target.y, target.x) * (180.0 / M_PI);
//     if (targetAz < 0.0) targetAz += 360.0;
//     if (range < 1e-6) return false;

//     double ratio    = std::clamp(target.z / range, -1.0, 1.0);
//     double targetEl = std::asin(ratio) * (180.0 / M_PI);

//     // ---- Beam illumination check ----
//     double azDiff, elDiff, scanMargin;
//     if (!signal_->isTargetInBeam(
//             antenna_->currentAzimuth(), antenna_->currentElevation(),
//             targetAz, targetEl, dt, config_,
//             azDiff, elDiff, scanMargin))
//         return false;

//     // ---- Signal chain ----
//     double effRCS = signal_->computeEffectiveRCS(target, range);
//     double Pr     = signal_->calculateSignalStrength(range, effRCS, config_);
//    // double sinr   = signal_->computeSINR(Pr, range, target.surface, config_);
//     double sinr   = signal_->computeSINR(Pr, range, target.surface, target, config_);

//     auto   cells    = signal_->generateReferenceCells(target.surface, config_);
//     double radVel   = signal_->computeRadialVelocity(target, range, dopplerNoise);

//     // Choose CFAR threshold: relaxed for near-stationary targets
//     // (< 5 m/s radial velocity — matches STATIONARY_VEL_THRESHOLD in
//     //  radarsignalprocessor.cpp which owns the physics constants)
//     bool   stationary  = (std::abs(radVel) < 5.0);
//     double threshold   = stationary
//                            ? signal_->computeCFARThresholdRelaxed(cells, config_)
//                            : signal_->computeCFARThreshold(cells, config_);

//     if (sinr <= threshold) return false; // Below detection threshold — no detect

//     // ---- Build detection record ----
//     DetectionOutput det;
//     det.targetID       = target.id;
//     det.azimuth        = targetAz + azNoise(tl_rng);
//     det.elevation      = targetEl + elNoise(tl_rng);
//     det.snr            = sinr;
//     det.radialVelocity = radVel;

//     signal_->computeTargetMotionParams(det, target, range);
//     signal_->computeCPA(det, target, range);
//     det.Pk = signal_->computePk(range, det.radialVelocity);

//     signal_->applyRangeAmbiguity(det, range, maxUnambiguousRange, rangeNoise);

//     if (config_.mode == RadarMode::LOCK_ON)
//     {
//         // In lock-on mode, use the Kalman track to unfold range ambiguity
//         signal_->resolveRangeForLockOn(det, range, maxUnambiguousRange,
//                                        target.id, tracker_->database());
//     }
//     else if (!det.isAmbiguous && det.range < config_.minDetectableRange)
//     {
//         // Clamp unambiguous range to minimum detectable range
//         det.range = config_.minDetectableRange;
//     }

//     // Suppress detection if it is too close to an existing one (merge guard)
//     if (signal_->shouldMergeDetection(det, scanDetections, config_))
//         return false;

//     scanDetections.push_back(det);

//     // ---- Track association (TWS and LOCK_ON) ----
//     if (config_.mode == RadarMode::TWS ||
//         config_.mode == RadarMode::LOCK_ON)
//     {
//         double     prob  = 0.0;
//         TrackFile* track = tracker_->findBestTrackMatch(
//             det, maxUnambiguousRange, prob);

//         if (track)
//             tracker_->performKalmanUpdate(
//                 *track, det, simTime, dt, maxUnambiguousRange, config_);
//         else
//             tracker_->createNewTrack(
//                 det, target, maxUnambiguousRange, simTime, config_);
//     }

//     return true;
// }

// // =============================================================================
// // §7  Maximum detection range — public thread-safe wrapper
// // =============================================================================

// double RadarModel::computeMaxDetectionRange(double rcs) const
// {
//     std::lock_guard<std::mutex> lk(mutex_);
//     return computeMaxDetectionRange_locked(rcs);
// }

// double RadarModel::computeMaxDetectionRange_locked(double rcs) const
// {
//     // Delegates entirely to the signal processor.
//     // Caller (update() or the public wrapper) must already hold mutex_.
//     return signal_->computeMaxDetectionRange(rcs, config_);
// }

// // =============================================================================
// // §8  Range ambiguity — public utility wrapper
// // =============================================================================

// double RadarModel::resolveRangeAmbiguity(
//     double measured, double predicted, double Rmax) const
// {
//     // Stateless utility — no lock needed, but take it for API consistency
//     std::lock_guard<std::mutex> lk(mutex_);
//     return signal_->resolveRangeAmbiguity(measured, predicted, Rmax);
// }

// // =============================================================================
// // §9  Output assembly placeholder
// //
// // assembleFinalOutput() exists for API parity with the original radarmodel.cpp.
// // In the original it was a stub with the comment "already assembled inline in
// // update()".  That remains true here — output is assembled at the end of
// // update() — but the symbol is retained so any external code that calls it
// // does not get a linker error.
// // =============================================================================

// void RadarModel::assembleFinalOutput()
// {
//     // Intentionally empty — output is assembled inline at the end of update().
//     // Retained for API parity with original radarmodel.cpp.
// }

// // =============================================================================
// // §10  C ABI — for dlopen / FFI
// // =============================================================================

// extern "C"
// {
// RadarModel* radarmodel_create()           { return new RadarModel();  }
// void        radarmodel_destroy(RadarModel* p) { delete p;             }
// }
