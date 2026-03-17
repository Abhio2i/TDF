// =============================================================================
// radarmodel.cpp  —  Pure C++ radar simulation model
//
// Implements the RadarModel lifecycle:
//   init() → start() → update() [loop] → end()
//
// All public methods are guarded by mutex_.
// No Qt / engine dependencies — only std C++17.
// =============================================================================

#include "radarmodel.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Physical constants
// =============================================================================
static constexpr double BOLTZMANN       = 1.380649e-23;   // J/K
static constexpr double SPEED_OF_LIGHT  = 299792458.0;    // m/s
static constexpr double RANGE_GATE      = 2000.0;         // m — association gate

// Clutter backscatter
static constexpr double SEA_SIGMA0_PER_SS = 3e-3;
static constexpr double LAND_SIGMA0       = 1e-2;

// Radial-velocity threshold below which relaxed CFAR is applied (m/s)
static constexpr double STATIONARY_VEL_THRESHOLD = 5.0;

// Thread-local RNG — each thread that calls update() gets its own engine
thread_local std::default_random_engine tl_rng{ std::random_device{}() };

// =============================================================================
// §1  Lifecycle
// =============================================================================

void RadarModel::init(const RadarConfig& cfg)
{
    std::lock_guard<std::mutex> lk(mutex_);

    config_              = cfg;
    trackDatabase_.clear();
    trackDatabase_.reserve(2048);

    currentAzimuth_      = 0.0;
    currentElevation_    = 0.0;
    scanDirection_       = 1.0;
    previousAzimuth_     = 0.0;
    scanBoundaryOccurred_ = false;

    displayRangeDirty_   = true;
    latestOutput_        = RadarOutput{};
    latestOutput_.mode   = cfg.mode;

    initialised_ = true;
    running_     = false;
}

void RadarModel::start()
{
    std::lock_guard<std::mutex> lk(mutex_);

    // Wipe scan state and track DB but keep config
    currentAzimuth_   = static_cast<double>(config_.minAzimuth);
    currentElevation_ = (config_.minElevation + config_.maxElevation) / 2.0;
    scanDirection_    = 1.0;
    previousAzimuth_  = currentAzimuth_;
    scanBoundaryOccurred_ = false;

    trackDatabase_.clear();
    latestOutput_ = RadarOutput{};
    latestOutput_.mode = config_.mode;

    displayRangeDirty_ = true;
    running_ = true;
}

void RadarModel::end()
{
    std::lock_guard<std::mutex> lk(mutex_);
    running_ = false;
    trackDatabase_.clear();
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
// §2  Configuration (hot-reload)
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
    config_.mode          = RadarMode::LOCK_ON;
    config_.lockedTargetID = targetID;
}

void RadarModel::breakLock()
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_.mode          = RadarMode::SURVEILLANCE;
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
// §5  Main update
// =============================================================================

void RadarModel::update(double                          dt,
                        const RadarPose&                pose,
                        const std::vector<TargetInput>& worldInputs,
                        double                          simTime)
{
    std::lock_guard<std::mutex> lk(mutex_);

    if (!running_) return;

    // ------------------------------------------------------------------
    // 5.0  Reset one-shot flags from last tick
    // ------------------------------------------------------------------
    latestOutput_.lockBroken = false;

    // ------------------------------------------------------------------
    // 5.1  Apply platform pose — update radarHeight for horizon
    // ------------------------------------------------------------------
    // Only override height if platform is clearly airborne
    if (pose.y > 50.0)
        config_.radarHeight = pose.y;

    // ------------------------------------------------------------------
    // 5.2  PRF / max unambiguous range
    // ------------------------------------------------------------------
    double currentPRF =
        (config_.prfLevels[0] > 0.0f) ? static_cast<double>(config_.prfLevels[0]) : 5000.0;
    double maxUnambiguousRange = SPEED_OF_LIGHT / (2.0 * currentPRF);

    // ------------------------------------------------------------------
    // 5.3  Kalman prediction for all existing tracks
    // ------------------------------------------------------------------
    if (config_.mode == RadarMode::TWS || config_.mode == RadarMode::LOCK_ON)
        updateTWSPrediction(dt);

    // ------------------------------------------------------------------
    // 5.4  Antenna positioning
    // ------------------------------------------------------------------
    scanBoundaryOccurred_ = false;

    if (config_.mode == RadarMode::LOCK_ON)
    {
        if (!updateAntennaLockOn(worldInputs))
        {
            // Lost the target — fall back to surveillance
            config_.mode          = RadarMode::SURVEILLANCE;
            config_.lockedTargetID = 0;
            latestOutput_.lockBroken = true;
        }
    }
    else
    {
        updateAntennaScan(dt);   // sets scanBoundaryOccurred_
    }

    // ------------------------------------------------------------------
    // 5.5  Per-target detection
    // ------------------------------------------------------------------
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
            scanDetections, rangeNoise, azNoise, elNoise, dopplerNoise);

        if (detected && config_.mode == RadarMode::LOCK_ON &&
            target.id == config_.lockedTargetID)
            lockedTargetVisible = true;
    }

    // ------------------------------------------------------------------
    // 5.6  Per-scan miss / dropout logic (TWS)
    // ------------------------------------------------------------------
    if (config_.mode == RadarMode::TWS && scanBoundaryOccurred_)
        applyScanMissLogic(simTime);

    // ------------------------------------------------------------------
    // 5.7  Break-lock if target vanished
    // ------------------------------------------------------------------
    if (config_.mode == RadarMode::LOCK_ON && !lockedTargetVisible)
    {
        config_.mode          = RadarMode::SURVEILLANCE;
        config_.lockedTargetID = 0;
        latestOutput_.lockBroken = true;
    }

    // ------------------------------------------------------------------
    // 5.8  Recompute display range if dirty
    // ------------------------------------------------------------------
    if (displayRangeDirty_)
    {
        // Use unlocked worker — mutex_ is already held by this update() call
        cachedDisplayRange_km_ = computeMaxDetectionRange_locked();
        cachedDisplayRange_km_ = std::max(5.0, std::min(1000.0, cachedDisplayRange_km_));
        displayRangeDirty_     = false;
    }

    // ------------------------------------------------------------------
    // 5.9  Assemble output snapshot
    // ------------------------------------------------------------------
    latestOutput_.detections       = std::move(scanDetections);
    latestOutput_.currentAzimuth   = currentAzimuth_;
    latestOutput_.currentElevation = currentElevation_;
    latestOutput_.mode             = config_.mode;
    latestOutput_.displayRange_km  = cachedDisplayRange_km_;

    // Build track output list
    latestOutput_.tracks.clear();
    for (const auto& t : trackDatabase_)
    {
        if (!t.isValidated) continue;
        latestOutput_.tracks.push_back(buildTrackOutput(t));
    }
}

// =============================================================================
// §6  Antenna
// =============================================================================

void RadarModel::updateAntennaScan(double dt)
{
    previousAzimuth_ = currentAzimuth_;

    double rotSpeed  = (static_cast<double>(config_.scanningRate_RPM) / 60.0) * 360.0;
    bool   full360   = (config_.minAzimuth <= -180.0f && config_.maxAzimuth >= 180.0f);

    if (full360)
    {
        currentAzimuth_ += rotSpeed * dt;
        if (currentAzimuth_ >= 360.0) currentAzimuth_ -= 360.0;
    }
    else
    {
        currentAzimuth_ += scanDirection_ * rotSpeed * dt;
        if (currentAzimuth_ > static_cast<double>(config_.maxAzimuth))
        {
            currentAzimuth_ = config_.maxAzimuth;
            scanDirection_  = -1.0;
        }
        if (currentAzimuth_ < static_cast<double>(config_.minAzimuth))
        {
            currentAzimuth_ = config_.minAzimuth;
            scanDirection_  =  1.0;
        }
    }

    double centEl = (config_.minElevation + config_.maxElevation) / 2.0;
    if (config_.scanType == ScanType::CONICAL)
        currentElevation_ = centEl +
                            std::sin(currentAzimuth_ * M_PI / 180.0) *
                                (config_.beamWidth / 4.0);
    else
        currentElevation_ = centEl;

    scanBoundaryOccurred_ = detectScanBoundary(previousAzimuth_, currentAzimuth_);
}

bool RadarModel::updateAntennaLockOn(const std::vector<TargetInput>& targets)
{
    for (const auto& t : targets)
    {
        if (t.id != config_.lockedTargetID) continue;

        double range = std::sqrt(t.x*t.x + t.y*t.y + t.z*t.z);
        if (range < 1e-6) continue;

        currentAzimuth_   = std::atan2(t.y, t.x) * (180.0 / M_PI);
        if (currentAzimuth_ < 0.0) currentAzimuth_ += 360.0;

        double ratio      = std::clamp(t.z / range, -1.0, 1.0);
        currentElevation_ = std::asin(ratio) * (180.0 / M_PI);
        return true;
    }
    return false;
}

bool RadarModel::detectScanBoundary(double prevAz, double newAz) const
{
    bool full360 = (config_.minAzimuth <= -180.0f && config_.maxAzimuth >= 180.0f);

    if (full360)
        return (newAz - prevAz) < -180.0;

    return (newAz <= static_cast<double>(config_.minAzimuth) + 0.1 ||
            newAz >= static_cast<double>(config_.maxAzimuth) - 0.1);
}

void RadarModel::applyScanMissLogic(double simTime)
{
    for (auto& track : trackDatabase_)
    {
        if (!track.updatedThisScan)
        { track.scanMissCount++; track.missCount++; }
        else
            track.scanMissCount = 0;

        track.updatedThisScan = false;
    }

    trackDatabase_.erase(
        std::remove_if(trackDatabase_.begin(), trackDatabase_.end(),
                       [&](const TrackFile& t)
                       {
                           return (t.scanMissCount > config_.missedScansToDrop) ||
                                  ((simTime - t.lastSeenTime) > config_.trackCoastSeconds);
                       }),
        trackDatabase_.end());
}

// =============================================================================
// §7  Beam / horizon geometry
// =============================================================================

bool RadarModel::checkHorizon(double range, double targetZ) const
{
    double Re     = 6371000.0 * config_.earthRadiusFactor * config_.atmosphericFactor;
    double dRadar = std::sqrt(2.0 * Re * config_.radarHeight);
    double dTgt   = std::sqrt(2.0 * Re * std::max(0.0, targetZ));
    return range <= (dRadar + dTgt);
}

bool RadarModel::isTargetInBeam(double targetAz, double targetEl,
                                double dt,
                                double& outAzDiff, double& outElDiff,
                                double& outScanMargin) const
{
    outAzDiff = std::abs(currentAzimuth_ - targetAz);
    if (outAzDiff > 180.0) outAzDiff = 360.0 - outAzDiff;

    outElDiff = std::abs(currentElevation_ - targetEl);

    double scanSpeed = (config_.scanningRate_RPM / 60.0) * 360.0;
    outScanMargin    = (config_.beamWidth * 2.5) + (scanSpeed * dt);

    return (outAzDiff <= outScanMargin && outElDiff <= outScanMargin);
}

double RadarModel::computeEffectiveRCS(const TargetInput& target, double range) const
{
    double velMag = std::sqrt(
        target.vx*target.vx + target.vy*target.vy + target.vz*target.vz);

    if (velMag < 0.01) return target.rcs * 0.6;

    double dot = std::clamp(
        (target.vx/velMag)*(target.x/range) +
            (target.vy/velMag)*(target.y/range) +
            (target.vz/velMag)*(target.z/range), -1.0, 1.0);

    double aspectRad    = std::acos(dot);
    double aspectFactor = 0.2 + 0.8 * std::abs(std::sin(aspectRad));
    return target.rcs * aspectFactor;
}

// =============================================================================
// §8  Signal chain
// =============================================================================

double RadarModel::computeNoisePower() const
{
    double B    = std::max(1.0, config_.antennaBandwidth);
    double F    = std::pow(10.0, config_.noiseFigure_dB / 10.0);
    return BOLTZMANN * config_.systemTemperature_K * B * F;
}

double RadarModel::computeClutterPower(double range, SurfaceType surface) const
{
    if (surface == SurfaceType::AIR || range < 1.0) return 0.0;

    double sigma0 = 0.0;
    if (surface == SurfaceType::SEA)
        sigma0 = config_.seaState   * SEA_SIGMA0_PER_SS;
    else if (surface == SurfaceType::LAND)
        sigma0 = config_.landClutter * LAND_SIGMA0;

    if (sigma0 <= 0.0) return 0.0;

    double tau   = (config_.pulseWidth > 0.0f)
                     ? static_cast<double>(config_.pulseWidth) : 1e-6;
    double bwRad = static_cast<double>(config_.beamWidth) * M_PI / 180.0;
    double patch = (SPEED_OF_LIGHT * tau / 2.0) * (range * bwRad);

    double lambda = SPEED_OF_LIGHT / config_.frequency_Hz;
    double Pt     = config_.emissionPower_kW * 1000.0;
    double G      = std::pow(10.0, config_.antennaGain / 10.0);

    double Pc = (Pt * G * G * lambda * lambda * sigma0 * patch)
                / (std::pow(4.0 * M_PI, 3) * std::pow(range, 3));

    thread_local std::exponential_distribution<double> fluct(1.0);
    return Pc * fluct(tl_rng);
}

double RadarModel::computeJammerPower(double targetRange_m) const
{
    const auto& j = config_.jammer;
    if (!j.active || j.power_kW <= 0.0) return 0.0;

    double Pj    = j.power_kW * 1000.0;
    double Gj    = std::pow(10.0, j.gain_dBi / 10.0);
    double Gr    = std::pow(10.0, config_.antennaGain / 10.0);
    double lam   = SPEED_OF_LIGHT / config_.frequency_Hz;
    double Rj    = j.selfScreening ? targetRange_m
                                : (j.range_m > 1.0 ? j.range_m : targetRange_m);

    double Pr_j  = (Pj * Gj * Gr * lam * lam)
                  / (std::pow(4.0 * M_PI, 2) * Rj * Rj);

    double B_r   = std::max(1.0, config_.antennaBandwidth);
    double B_j   = std::max(1.0, j.bandwidth_Hz);
    return Pr_j * std::min(1.0, B_r / B_j);
}

double RadarModel::computePropagationLoss(double range_m) const
{
    double loss_dB = 0.0;

    if (config_.rainRate_mmph > 0.0)
    {
        constexpr double k = 0.00887, a = 1.255;
        double gamma = k * std::pow(config_.rainRate_mmph, a);   // dB/km one-way
        loss_dB += 2.0 * gamma * (range_m / 1000.0);
    }

    if (config_.fogVisibility_m > 1.0 && config_.fogVisibility_m < 2000.0)
    {
        double M     = 0.0367 * std::pow(1000.0 / config_.fogVisibility_m, 1.43);
        double gamma = 0.0157 * std::pow(M, 1.05);
        loss_dB += 2.0 * gamma * (range_m / 1000.0);
    }

    return std::pow(10.0, -loss_dB / 10.0);
}

double RadarModel::calculateSignalStrength(double range, double rcs) const
{
    if (range < 1.0) range = 1.0;

    double freq = config_.frequency_Hz;
    if (config_.frequencyAgility)
        freq += static_cast<double>(config_.hopStepFrequency * config_.hopRate);

    double lambda  = SPEED_OF_LIGHT / freq;
    double Pt      = config_.emissionPower_kW * 1000.0;
    double G       = std::pow(10.0, config_.antennaGain / 10.0);

    double Pr = (Pt * G * G * lambda * lambda * rcs)
                / (std::pow(4.0 * M_PI, 3) * std::pow(range, 4));

    return std::max(0.0, Pr * computePropagationLoss(range));
}

double RadarModel::computeSINR(double receivedPower, double range,
                               SurfaceType surface) const
{
    double Pn = computeNoisePower();
    double Pc = computeClutterPower(range, surface);
    double Pj = computeJammerPower(range);
    return std::max(0.0, receivedPower / (Pn + Pc + Pj));
}

std::vector<double> RadarModel::generateReferenceCells(SurfaceType surface) const
{
    thread_local std::exponential_distribution<double> cellDist(1.0);

    std::vector<double> cells;
    cells.reserve(16);
    for (int i = 0; i < 16; ++i)
    {
        double c = cellDist(tl_rng);
        if (surface == SurfaceType::SEA)  c *= (1.0 + config_.seaState    * 0.3);
        if (surface == SurfaceType::LAND) c *= (1.0 + config_.landClutter  * 0.5);
        cells.push_back(c);
    }
    return cells;
}

double RadarModel::computeCFARThreshold(const std::vector<double>& cells) const
{
    if (cells.empty()) return 1e12;
    double sum = 0.0;
    for (double v : cells) sum += v;
    double N     = static_cast<double>(cells.size());
    double alpha = N * (std::pow(config_.targetPfa, -1.0 / N) - 1.0);
    return (sum / N) * alpha;
}

double RadarModel::computeCFARThresholdRelaxed(const std::vector<double>& cells) const
{
    if (cells.empty()) return 1e12;
    double sum = 0.0;
    for (double v : cells) sum += v;
    double N      = static_cast<double>(cells.size());
    double pfa    = std::min(1e-4, config_.targetPfa * 100.0);
    double alpha  = N * (std::pow(pfa, -1.0 / N) - 1.0);
    return (sum / N) * alpha;
}

// =============================================================================
// §9  Max detection range
//
// Split into two layers to avoid deadlock:
//   computeMaxDetectionRange_locked()  — internal, called while mutex_ is held
//                                        (used by update() and by the public wrapper)
//   computeMaxDetectionRange()         — public, acquires the mutex itself
//
// Rule: all private helper functions (computeNoisePower, computeClutterPower …)
//       read config_ directly and must only be called while the mutex is held.
// =============================================================================

double RadarModel::computeMaxDetectionRange_locked(double rcs) const
{
    // Caller must already hold mutex_
    double lambda  = SPEED_OF_LIGHT / config_.frequency_Hz;
    double Pt      = config_.emissionPower_kW * 1000.0;
    double G       = std::pow(10.0, config_.antennaGain / 10.0);
    double Pn      = computeNoisePower();
    double N       = 16.0;
    double alpha   = N * (std::pow(config_.targetPfa, -1.0 / N) - 1.0);

    double R_est   = 50000.0;
    for (int i = 0; i < 5; ++i)
    {
        double Pc   = std::max(computeClutterPower(R_est, SurfaceType::SEA),
                             computeClutterPower(R_est, SurfaceType::LAND));
        double Pj   = computeJammerPower(R_est);
        double Pnt  = Pn + Pc + Pj;
        double prop = computePropagationLoss(R_est);
        double Pt_e = Pt * prop * prop;

        double num  = Pt_e * G * G * lambda * lambda * (rcs * 0.6);
        double den  = std::pow(4.0 * M_PI, 3) * Pnt * alpha;
        if (den <= 0.0) break;
        R_est = std::pow(num / den, 0.25);
    }

    double Re      = 6371000.0 * config_.earthRadiusFactor * config_.atmosphericFactor;
    double horizon = std::sqrt(2.0 * Re * config_.radarHeight) / 1000.0;   // km
    double R_km    = std::min(R_est / 1000.0, horizon);
    return std::max(R_km, config_.minDetectableRange / 1000.0 * 2.0);
}

// Public wrapper — safe to call from any thread
double RadarModel::computeMaxDetectionRange(double rcs) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return computeMaxDetectionRange_locked(rcs);
}

// =============================================================================
// §10  Motion / geometry helpers
// =============================================================================

void RadarModel::computeTargetMotionParams(DetectionOutput& det,
                                           const TargetInput& target,
                                           double range) const
{
    det.speedOverGround = std::sqrt(target.vx*target.vx + target.vz*target.vz);
    det.heading         = std::atan2(target.vx, target.vz) * (180.0 / M_PI);
    if (det.heading < 0.0) det.heading += 360.0;
    det.acceleration    = 0.0;

    if (det.speedOverGround > 0.01)
    {
        double vxn = target.vx / det.speedOverGround;
        double vzn = target.vz / det.speedOverGround;
        double dot = std::clamp(vxn*(target.x/range) + vzn*(target.z/range), -1.0, 1.0);
        det.targetAspect = std::acos(dot) * 180.0 / M_PI;
    }
    else
        det.targetAspect = 0.0;
}

double RadarModel::computeRadialVelocity(const TargetInput& target, double range,
                                         std::normal_distribution<double>& dopplerNoise) const
{
    double dot = target.vx*target.x + target.vy*target.y + target.vz*target.z;
    return ((range > 1e-6) ? dot / range : 0.0) + dopplerNoise(tl_rng);
}

void RadarModel::computeCPA(DetectionOutput& det,
                            const TargetInput& target,
                            double range) const
{
    det.cpa_distance = range;
    det.time_to_cpa  = 0.0;

    double v2 = target.vx*target.vx + target.vy*target.vy + target.vz*target.vz;
    if (v2 > 0.01)
    {
        double t = -(target.x*target.vx + target.y*target.vy + target.z*target.vz) / v2;
        det.time_to_cpa = std::max(0.0, t);
        double cx = target.x + target.vx * det.time_to_cpa;
        double cy = target.y + target.vy * det.time_to_cpa;
        double cz = target.z + target.vz * det.time_to_cpa;
        det.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
    }
}

double RadarModel::computePk(double range, double radialVelocity) const
{
    return std::min(0.99,
                    0.95 * std::exp(-range / 45000.0) * (radialVelocity < 0.0 ? 1.2 : 0.8));
}

// =============================================================================
// §11  Range ambiguity
// =============================================================================

double RadarModel::resolveRangeAmbiguity(double measured,
                                         double predicted,
                                         double Rmax) const
{
    if (Rmax < 1.0) return measured;
    double best = measured, minErr = 1e12;
    for (int k = -5; k <= 5; ++k)
    {
        double cand = measured + k * Rmax;
        double err  = std::abs(cand - predicted);
        if (err < minErr) { minErr = err; best = cand; }
    }
    return best;
}

void RadarModel::applyRangeAmbiguity(DetectionOutput& det, double range,
                                     double maxUnambiguousRange,
                                     std::normal_distribution<double>& rangeNoise) const
{
    if (range > maxUnambiguousRange)
    {
        det.range       = std::fmod(range, maxUnambiguousRange) + rangeNoise(tl_rng);
        det.isAmbiguous = true;
    }
    else
    {
        det.range       = range + rangeNoise(tl_rng);
        det.isAmbiguous = false;
    }
}

void RadarModel::resolveRangeForLockOn(DetectionOutput& det, double range,
                                       double maxUnambiguousRange,
                                       uint32_t targetId) const
{
    double predicted = range;
    for (const auto& t : trackDatabase_)
        if (t.id == targetId) { predicted = t.predictedRange; break; }

    det.range       = resolveRangeAmbiguity(det.range, predicted, maxUnambiguousRange);
    det.isAmbiguous = false;
}

// =============================================================================
// §12  Detection merge
// =============================================================================

bool RadarModel::shouldMergeDetection(const DetectionOutput& det,
                                      const std::vector<DetectionOutput>& existing) const
{
    for (const auto& ex : existing)
    {
        double azDiff = std::abs(ex.azimuth - det.azimuth);
        if (azDiff > 180.0) azDiff = 360.0 - azDiff;
        if (std::abs(ex.range - det.range) < 150.0 &&
            azDiff < config_.beamWidth &&
            std::abs(ex.elevation - det.elevation) < config_.beamWidth)
            return true;
    }
    return false;
}

// =============================================================================
// §13  Tracking — Kalman prediction
// =============================================================================

void RadarModel::updateTWSPrediction(double dt)
{
    for (auto& track : trackDatabase_)
    {
        // F = constant-velocity state transition
        double F[6][6] = {
            {1,0,0,dt, 0, 0},
            {0,1,0, 0,dt, 0},
            {0,0,1, 0, 0,dt},
            {0,0,0, 1, 0, 0},
            {0,0,0, 0, 1, 0},
            {0,0,0, 0, 0, 1}
        };

        std::array<double,6> Xnew = {};
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
                Xnew[i] += F[i][j] * track.X[j];
        track.X = Xnew;

        double FP[6][6] = {};
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
                for (int k = 0; k < 6; ++k)
                    FP[i][j] += F[i][k] * track.P[k][j];

        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
            {
                double tmp = 0.0;
                for (int k = 0; k < 6; ++k)
                    tmp += FP[i][k] * F[j][k];
                track.P[i][j] = tmp + track.Q[i][j];
            }

        track.x  = track.X[0]; track.y  = track.X[1]; track.z  = track.X[2];
        track.vx = track.X[3]; track.vy = track.X[4]; track.vz = track.X[5];
        track.predictedRange = std::sqrt(
            track.x*track.x + track.y*track.y + track.z*track.z);
        track.isUpdated = false;
    }
}

// =============================================================================
// §14  Tracking — association
// =============================================================================

double RadarModel::computeAssociationProbability(double measured,
                                                 double predicted,
                                                 double gateSize) const
{
    double err   = measured - predicted;
    double sigma = gateSize / 2.0;
    return std::exp(-(err*err) / (2.0*sigma*sigma));
}

TrackFile* RadarModel::findBestTrackMatch(const DetectionOutput& det,
                                          double maxUnambiguousRange,
                                          double& outBestProb)
{
    TrackFile* best = nullptr;
    outBestProb     = 0.0;

    for (auto& track : trackDatabase_)
    {
        if (!std::isfinite(track.x) || !std::isfinite(track.y) ||
            !std::isfinite(track.z) || !std::isfinite(track.vx) ||
            !std::isfinite(track.vy) || !std::isfinite(track.vz))
        {
            track.hitCount = 0; track.isValidated = false;
            continue;
        }
        if (track.isUpdated) continue;

        double candRange = resolveRangeAmbiguity(
            det.range, track.predictedRange, maxUnambiguousRange);

        if (std::abs(candRange - track.predictedRange) > RANGE_GATE) continue;

        double azRad = det.azimuth   * M_PI / 180.0;
        double elRad = det.elevation * M_PI / 180.0;
        double zx = det.range * std::cos(elRad) * std::cos(azRad);
        double zy = det.range * std::cos(elRad) * std::sin(azRad);
        double zz = det.range * std::sin(elRad);

        double dx = zx - track.X[0];
        double dy = zy - track.X[1];
        double dz = zz - track.X[2];

        double S[3][3];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                S[i][j] = track.P[i][j] + track.R[i][j];

        double detS = S[0][0]*(S[1][1]*S[2][2] - S[1][2]*S[2][1])
                      - S[0][1]*(S[1][0]*S[2][2] - S[1][2]*S[2][0])
                      + S[0][2]*(S[1][0]*S[2][1] - S[1][1]*S[2][0]);
        if (std::abs(detS) < 1e-2) continue;

        double invS[3][3];
        invS[0][0]=(S[1][1]*S[2][2]-S[1][2]*S[2][1])/detS;
        invS[0][1]=(S[0][2]*S[2][1]-S[0][1]*S[2][2])/detS;
        invS[0][2]=(S[0][1]*S[1][2]-S[0][2]*S[1][1])/detS;
        invS[1][0]=(S[1][2]*S[2][0]-S[1][0]*S[2][2])/detS;
        invS[1][1]=(S[0][0]*S[2][2]-S[0][2]*S[2][0])/detS;
        invS[1][2]=(S[0][2]*S[1][0]-S[0][0]*S[1][2])/detS;
        invS[2][0]=(S[1][0]*S[2][1]-S[1][1]*S[2][0])/detS;
        invS[2][1]=(S[0][1]*S[2][0]-S[0][0]*S[2][1])/detS;
        invS[2][2]=(S[0][0]*S[1][1]-S[0][1]*S[1][0])/detS;

        double d2 = dx*(invS[0][0]*dx + invS[0][1]*dy + invS[0][2]*dz)
                    + dy*(invS[1][0]*dx + invS[1][1]*dy + invS[1][2]*dz)
                    + dz*(invS[2][0]*dx + invS[2][1]*dy + invS[2][2]*dz);

        if (d2 > 9.21) continue;   // χ² gate (3-DOF, 99%)

        double prob = computeAssociationProbability(
            candRange, track.predictedRange, RANGE_GATE);

        if (prob > outBestProb) { outBestProb = prob; best = &track; }
    }
    return best;
}

// =============================================================================
// §15  Tracking — Kalman update
// =============================================================================

void RadarModel::performKalmanUpdate(TrackFile& track,
                                     const DetectionOutput& det,
                                     double simTime, double /*dt*/,
                                     double maxUnambiguousRange)
{
    double bestRange = resolveRangeAmbiguity(
        det.range, track.predictedRange, maxUnambiguousRange);

    double azRad = det.azimuth   * M_PI / 180.0;
    double elRad = det.elevation * M_PI / 180.0;

    double z[3] = {
        bestRange * std::cos(elRad) * std::cos(azRad),
        bestRange * std::cos(elRad) * std::sin(azRad),
        bestRange * std::sin(elRad)
    };

    double y[3] = { z[0]-track.X[0], z[1]-track.X[1], z[2]-track.X[2] };

    double S[3][3] = {};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            S[i][j] = track.P[i][j] + track.R[i][j];

    double detS = S[0][0]*(S[1][1]*S[2][2]-S[1][2]*S[2][1])
                  - S[0][1]*(S[1][0]*S[2][2]-S[1][2]*S[2][0])
                  + S[0][2]*(S[1][0]*S[2][1]-S[1][1]*S[2][0]);

    if (std::abs(detS) < 1e-3) { track.hitCount = std::max(0, track.hitCount-1); return; }

    double invS[3][3];
    invS[0][0]=(S[1][1]*S[2][2]-S[1][2]*S[2][1])/detS;
    invS[0][1]=(S[0][2]*S[2][1]-S[0][1]*S[2][2])/detS;
    invS[0][2]=(S[0][1]*S[1][2]-S[0][2]*S[1][1])/detS;
    invS[1][0]=(S[1][2]*S[2][0]-S[1][0]*S[2][2])/detS;
    invS[1][1]=(S[0][0]*S[2][2]-S[0][2]*S[2][0])/detS;
    invS[1][2]=(S[0][2]*S[1][0]-S[0][0]*S[1][2])/detS;
    invS[2][0]=(S[1][0]*S[2][1]-S[1][1]*S[2][0])/detS;
    invS[2][1]=(S[0][1]*S[2][0]-S[0][0]*S[2][1])/detS;
    invS[2][2]=(S[0][0]*S[1][1]-S[0][1]*S[1][0])/detS;

    double K[6][3] = {};
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                K[i][j] += track.P[i][k] * invS[k][j];

    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j)
            track.X[i] += K[i][j] * y[j];

    // Joseph form covariance update
    double IKH[6][6] = {};
    for (int i = 0; i < 6; ++i)
    {
        IKH[i][i] = 1.0;
        for (int j = 0; j < 3; ++j)
            IKH[i][j] -= K[i][j];
    }

    double Pnew[6][6] = {};
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            for (int k = 0; k < 6; ++k)
                Pnew[i][j] += IKH[i][k] * track.P[k][j];
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            track.P[i][j] = Pnew[i][j];

    for (int i = 3; i < 6; ++i)
        track.X[i] = std::clamp(track.X[i], -config_.maxTrackSpeed, config_.maxTrackSpeed);

    track.x  = track.X[0]; track.y  = track.X[1]; track.z  = track.X[2];
    track.vx = track.X[3]; track.vy = track.X[4]; track.vz = track.X[5];

    track.range = std::sqrt(track.x*track.x + track.y*track.y + track.z*track.z);
    track.velocity = (track.range > 1e-6)
                         ? (track.vx*track.x + track.vy*track.y + track.vz*track.z) / track.range : 0.0;

    track.lastSeenTime    = simTime;
    track.hitCount++;
    track.isUpdated       = true;
    track.updatedThisScan = true;
    track.missCount       = 0;
    track.wasAmbiguous    = det.isAmbiguous;

    if (track.hitCount >= config_.minHitsToValidate)
        track.isValidated = true;
}

void RadarModel::createNewTrack(const DetectionOutput& det,
                                const TargetInput& target,
                                double maxUnambiguousRange,
                                double simTime)
{
    for (const auto& t : trackDatabase_)
        if (t.id == det.targetID) return;

    TrackFile tr;
    tr.id = det.targetID;

    double r = det.range;
    if (det.isAmbiguous && det.radialVelocity < 0.0)
        r += maxUnambiguousRange;

    double azRad = det.azimuth   * M_PI / 180.0;
    double elRad = det.elevation * M_PI / 180.0;

    tr.x  = r * std::cos(elRad) * std::cos(azRad);
    tr.y  = r * std::cos(elRad) * std::sin(azRad);
    tr.z  = r * std::sin(elRad);
    tr.vx = target.vx;
    tr.vy = target.vy;
    tr.vz = target.vz;
    tr.X  = { tr.x, tr.y, tr.z, tr.vx, tr.vy, tr.vz };

    double posVar = config_.noise.rangeStdDev * config_.noise.rangeStdDev;
    double velVar = 500.0 * 500.0;

    for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j) tr.P[i][j] = 0.0;
    tr.P[0][0]=posVar; tr.P[1][1]=posVar; tr.P[2][2]=posVar;
    tr.P[3][3]=velVar; tr.P[4][4]=velVar; tr.P[5][5]=velVar;

    tr.Q[0][0]=10.0; tr.Q[1][1]=10.0; tr.Q[2][2]=10.0;
    tr.Q[3][3]= 1.0; tr.Q[4][4]= 1.0; tr.Q[5][5]= 1.0;

    for (int i = 0; i < 3; ++i) tr.R[i][i] = 25.0;

    tr.range           = r;
    tr.predictedRange  = r;
    tr.velocity        = det.radialVelocity;
    tr.lastSeenTime    = simTime;
    tr.hitCount        = 1;
    tr.scanMissCount   = 0;
    tr.updatedThisScan = true;
    tr.wasAmbiguous    = det.isAmbiguous;

    if (trackDatabase_.size() > 2000)
        trackDatabase_.erase(trackDatabase_.begin());

    trackDatabase_.push_back(std::move(tr));
}

// =============================================================================
// §16  Per-target detection pipeline
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
    double range = std::sqrt(target.x*target.x + target.y*target.y + target.z*target.z);

    if (range < config_.minDetectableRange)    return false;
    if (!checkHorizon(range, target.z))        return false;

    double targetAz = std::atan2(target.y, target.x) * (180.0 / M_PI);
    if (targetAz < 0.0) targetAz += 360.0;
    if (range < 1e-6)   return false;

    double ratio    = std::clamp(target.z / range, -1.0, 1.0);
    double targetEl = std::asin(ratio) * (180.0 / M_PI);

    double azDiff, elDiff, scanMargin;
    if (!isTargetInBeam(targetAz, targetEl, dt, azDiff, elDiff, scanMargin))
        return false;

    double effRCS   = computeEffectiveRCS(target, range);
    double Pr       = calculateSignalStrength(range, effRCS);
    double sinr     = computeSINR(Pr, range, target.surface);

    auto   cells    = generateReferenceCells(target.surface);
    double radVel   = computeRadialVelocity(target, range, dopplerNoise);
    bool   stationary = (std::abs(radVel) < STATIONARY_VEL_THRESHOLD);

    double threshold = stationary ? computeCFARThresholdRelaxed(cells)
                                  : computeCFARThreshold(cells);

    if (sinr <= threshold) return false;

    DetectionOutput det;
    det.targetID       = target.id;
    det.azimuth        = targetAz + azNoise(tl_rng);
    det.elevation      = targetEl + elNoise(tl_rng);
    det.snr            = sinr;
    det.radialVelocity = radVel;

    computeTargetMotionParams(det, target, range);
    computeCPA(det, target, range);
    det.Pk = computePk(range, det.radialVelocity);

    applyRangeAmbiguity(det, range, maxUnambiguousRange, rangeNoise);

    if (config_.mode == RadarMode::LOCK_ON)
        resolveRangeForLockOn(det, range, maxUnambiguousRange, target.id);
    else if (!det.isAmbiguous && det.range < config_.minDetectableRange)
        det.range = config_.minDetectableRange;

    if (shouldMergeDetection(det, scanDetections)) return false;

    scanDetections.push_back(det);

    // Track association (TWS and LOCK_ON)
    if (config_.mode == RadarMode::TWS || config_.mode == RadarMode::LOCK_ON)
    {
        double     prob  = 0.0;
        TrackFile* track = findBestTrackMatch(det, maxUnambiguousRange, prob);

        if (track) performKalmanUpdate(*track, det, simTime, dt, maxUnambiguousRange);
        else       createNewTrack(det, target, maxUnambiguousRange, simTime);
    }

    return true;
}

// =============================================================================
// §17  Output assembly
// =============================================================================

TrackOutput RadarModel::buildTrackOutput(const TrackFile& tr) const
{
    TrackOutput out;
    out.id           = tr.id;
    out.x            = tr.x; out.y = tr.y; out.z = tr.z;
    out.vx           = tr.vx; out.vy = tr.vy; out.vz = tr.vz;
    out.radialVelocity = tr.velocity;
    out.isValidated  = tr.isValidated;
    out.hitCount     = tr.hitCount;
    out.scanMissCount = tr.scanMissCount;
    out.wasAmbiguous = tr.wasAmbiguous;

    double reportRange = tr.isUpdated ? tr.range : tr.predictedRange;
    out.range = reportRange;

    if (reportRange > 1e-6)
    {
        out.azimuth   = std::atan2(tr.y, tr.x) * (180.0 / M_PI);
        if (out.azimuth < 0.0) out.azimuth += 360.0;
        out.elevation = std::asin(std::clamp(tr.z / reportRange, -1.0, 1.0)) * (180.0 / M_PI);
    }

    out.speedOverGround = std::sqrt(tr.vx*tr.vx + tr.vz*tr.vz);
    out.heading         = std::atan2(tr.vx, tr.vz) * (180.0 / M_PI);
    if (out.heading < 0.0) out.heading += 360.0;

    out.targetAspect = std::abs(out.heading - out.azimuth);
    if (out.targetAspect > 180.0) out.targetAspect = 360.0 - out.targetAspect;

    double v2 = tr.vx*tr.vx + tr.vy*tr.vy + tr.vz*tr.vz;
    if (v2 > 0.01)
    {
        double t_cpa = -(tr.x*tr.vx + tr.y*tr.vy + tr.z*tr.vz) / v2;
        if (t_cpa > 0.0)
        {
            out.time_to_cpa  = t_cpa;
            double cx = tr.x + tr.vx*t_cpa;
            double cy = tr.y + tr.vy*t_cpa;
            double cz = tr.z + tr.vz*t_cpa;
            out.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
        }
        else
        {
            out.time_to_cpa  = 0.0;
            out.cpa_distance = reportRange;
        }
    }
    else
    {
        out.time_to_cpa  = 0.0;
        out.cpa_distance = reportRange;
    }

    out.Pk = std::min(0.99,
                      0.95 * std::exp(-reportRange / 45000.0) * (tr.velocity < 0.0 ? 1.2 : 0.8));

    return out;
}

// ---------------------------------------------------------------------------
// buildTWSDetection — convert a validated TrackFile into a DetectionOutput
// for callers that need the detection-style record (e.g. display overlays).
// ---------------------------------------------------------------------------
DetectionOutput RadarModel::buildTWSDetection(const TrackFile& tr) const
{
    DetectionOutput out;
    out.targetID = tr.id;

    double reportRange = tr.isUpdated ? tr.range : tr.predictedRange;
    out.range          = reportRange;
    out.radialVelocity = tr.velocity;
    out.isAmbiguous    = false;

    if (reportRange > 1e-6)
    {
        out.azimuth   = std::atan2(tr.y, tr.x) * (180.0 / M_PI);
        if (out.azimuth < 0.0) out.azimuth += 360.0;
        out.elevation = std::asin(std::clamp(tr.z / reportRange, -1.0, 1.0))
                        * (180.0 / M_PI);
    }

    out.speedOverGround = std::sqrt(tr.vx*tr.vx + tr.vz*tr.vz);
    out.heading         = std::atan2(tr.vx, tr.vz) * (180.0 / M_PI);
    if (out.heading < 0.0) out.heading += 360.0;

    out.targetAspect = std::abs(out.heading - out.azimuth);
    if (out.targetAspect > 180.0) out.targetAspect = 360.0 - out.targetAspect;

    double v2 = tr.vx*tr.vx + tr.vy*tr.vy + tr.vz*tr.vz;
    if (v2 > 0.01)
    {
        double t_cpa = -(tr.x*tr.vx + tr.y*tr.vy + tr.z*tr.vz) / v2;
        if (t_cpa > 0.0)
        {
            out.time_to_cpa  = t_cpa;
            double cx = tr.x + tr.vx * t_cpa;
            double cy = tr.y + tr.vy * t_cpa;
            double cz = tr.z + tr.vz * t_cpa;
            out.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
        }
        else
        {
            out.time_to_cpa  = 0.0;
            out.cpa_distance = reportRange;
        }
    }
    else
    {
        out.time_to_cpa  = 0.0;
        out.cpa_distance = reportRange;
    }

    out.Pk = std::min(0.99,
                      0.95 * std::exp(-reportRange / 45000.0) * (tr.velocity < 0.0 ? 1.2 : 0.8));

    out.snr        = 0.0;   // not recomputed at report time
    out.lockBroken = false;

    return out;
}

// ---------------------------------------------------------------------------
// generateTWSReport — fill a TrackOutput vector with all validated tracks.
// Mirrors the old generateTWSReport() / buildTWSOutput() pair exactly.
// ---------------------------------------------------------------------------
void RadarModel::generateTWSReport(std::vector<TrackOutput>& out) const
{
    out.clear();
    out.reserve(trackDatabase_.size());
    for (const auto& tr : trackDatabase_)
    {
        if (!tr.isValidated) continue;
        out.push_back(buildTrackOutput(tr));
    }
}

void RadarModel::assembleFinalOutput()
{
    // Already assembled inline in update() — placeholder for future decoupling
}

// =============================================================================
// §18  C ABI
// =============================================================================

extern "C"
{
RadarModel* radarmodel_create()  { return new RadarModel(); }
void        radarmodel_destroy(RadarModel* p) { delete p; }
}
// #include <cmath>
// #include <algorithm>
// #include <vector>
// #include <random>
// #include <iostream>
// #include <mutex>
// #include "radarmodel.h"

// #ifndef M_PI
// #define M_PI 3.14159265358979323846
// #endif

// // ---------------------------------------------------------------------------
// // Physical constants
// // ---------------------------------------------------------------------------
// static constexpr double BOLTZMANN      = 1.380649e-23;   // J/K
// static constexpr double SPEED_OF_LIGHT = 299792458.0;    // m/s
// static constexpr double RANGE_GATE     = 2000.0;         // m

// // ---------------------------------------------------------------------------
// // Clutter backscatter coefficients
// // ---------------------------------------------------------------------------
// static constexpr double SEA_SIGMA0_PER_SS = 3e-3;
// static constexpr double LAND_SIGMA0       = 1e-2;

// // ---------------------------------------------------------------------------
// // Stationary-target Doppler threshold (m/s)
// // Targets with |radialVelocity| below this get relaxed CFAR
// // ---------------------------------------------------------------------------
// static constexpr double STATIONARY_VEL_THRESHOLD = 5.0;

// thread_local std::default_random_engine rng(std::random_device{}());

// // ===========================================================================
// // Constructor / Configuration
// // ===========================================================================
// RadarModel::RadarModel()
//     : currentAzimuth(0.0),
//     currentElevation(0.0),
//     scanDirection(1.0),
//     previousAzimuth(0.0),
//     scanBoundaryOccurred(false)
// {
//     trackDatabase.reserve(2048);
// }

// void RadarModel::setConfiguration(const RadarAttributes& attrs)
// {
//     config = attrs;
// }

// // ===========================================================================
// // FIX 3 — Thermal noise power:  Pn = k · T · B · F
// // ===========================================================================
// double RadarModel::computeNoisePower() const
// {
//     double B     = std::max(1.0, config.antennaBandwidth);
//     double F_lin = std::pow(10.0, config.noiseFigure_dB / 10.0);
//     return BOLTZMANN * config.systemTemperature_K * B * F_lin;
// }

// // ===========================================================================
// // FIX 1 — Clutter power in watts (R³ law, physical patch area)
// // ===========================================================================
// double RadarModel::computeClutterPower(double range, SurfaceType surface) const
// {
//     if (surface == SurfaceType::AIR || range < 1.0)
//         return 0.0;

//     double sigma0 = 0.0;
//     if (surface == SurfaceType::SEA)
//         sigma0 = config.seaState * SEA_SIGMA0_PER_SS;
//     else if (surface == SurfaceType::LAND)
//         sigma0 = config.landClutter * LAND_SIGMA0;

//     if (sigma0 <= 0.0) return 0.0;

//     double tau    = (config.pulseWidth > 0.0f)
//                      ? static_cast<double>(config.pulseWidth) : 1e-6;
//     double bwRad  = static_cast<double>(config.beamWidth) * M_PI / 180.0;
//     double patch  = (SPEED_OF_LIGHT * tau / 2.0) * (range * bwRad);

//     double lambda = SPEED_OF_LIGHT / config.frequency_Hz;
//     double Pt     = config.emissionPower_kW * 1000.0;
//     double G_lin  = std::pow(10.0, config.antennaGain / 10.0);

//     double Pc = (Pt * G_lin * G_lin * lambda * lambda * sigma0 * patch)
//                 / (std::pow(4.0 * M_PI, 3) * std::pow(range, 3));

//     //static std::exponential_distribution<double> fluct(1.0);
//     // FIX 6: thread_local instead of static — safe for multi-threaded scan
//     thread_local std::exponential_distribution<double> fluct(1.0);
//     return Pc * fluct(rng);   // Rayleigh amplitude fluctuation
// }
// // ===========================================================================
// // Jammer power at radar receiver (J/S model)
// //
// // Self-screening jammer: jammer is ON the target, so jammerRange = targetRange.
// // Stand-off jammer:      jammer sits at a fixed range_m from the radar.
// //
// // Returns jamming power in watts (same units as Pn and Pc).
// // ===========================================================================
// double RadarModel::computeJammerPower(double targetRange_m) const
// {
//     const auto& j = config.jammer;
//     if (!j.active || j.power_kW <= 0.0) return 0.0;

//     double Pj_watts  = j.power_kW * 1000.0;
//     double Gj_lin    = std::pow(10.0, j.gain_dBi / 10.0);
//     double Gr_lin    = std::pow(10.0, config.antennaGain / 10.0);
//     double lambda    = SPEED_OF_LIGHT / config.frequency_Hz;

//     // Jammer range: self-screening uses target range, stand-off uses fixed range
//     double Rj = j.selfScreening ? targetRange_m
//                                 : (j.range_m > 1.0 ? j.range_m : targetRange_m);

//     // One-way link: Pj_received = Pj * Gj * Gr * lambda² / (4π·Rj)²
//     double Pj_received = (Pj_watts * Gj_lin * Gr_lin * lambda * lambda)
//                          / (std::pow(4.0 * M_PI, 2) * Rj * Rj);

//     // Scale by jammer bandwidth filling radar bandwidth
//     double B_radar  = std::max(1.0, config.antennaBandwidth);
//     double B_jammer = std::max(1.0, j.bandwidth_Hz);
//     double bwFactor = std::min(1.0, B_radar / B_jammer);

//     return Pj_received * bwFactor;
// }
// // ===========================================================================
// // FIX 1+3 — SINR = Pr / (Pn + Pc)
// // ===========================================================================
// double RadarModel::applyClutterEffects(double receivedPower,
//                                        double range,
//                                        SurfaceType surface)
// {
//     double Pn = computeNoisePower();
//     double Pc = computeClutterPower(range, surface);
//     double Pj = computeJammerPower(range);           // <-- new
//     return std::max(0.0, receivedPower / (Pn + Pc + Pj));
// }

// // ===========================================================================
// // FIX 2 — CA-CFAR threshold:  α = N · (Pfa^(-1/N) − 1)
// // ===========================================================================
// double RadarModel::computeCFARThreshold(const std::vector<double>& referenceCells)
// {
//     if (referenceCells.empty()) return 1e12;

//     double sum = 0.0;
//     for (double v : referenceCells) sum += v;
//     double noiseMean = sum / static_cast<double>(referenceCells.size());

//     double N     = static_cast<double>(referenceCells.size());
//     double alpha = N * (std::pow(config.targetPfa, -1.0 / N) - 1.0);
//     return noiseMean * alpha;
// }

// // ===========================================================================
// // PRODUCTION FIX — Relaxed CFAR for stationary / near-zero-Doppler targets
// //
// // Real radars apply MTI (Moving Target Indicator) notch filters that blank
// // zero-Doppler returns, but since this sim has no MTI, we do the opposite:
// // use a more permissive Pfa so stationary targets aren't thresholded out.
// // Pfa is relaxed by 2 decades (1e-4 vs default 1e-6).
// // ===========================================================================
// double RadarModel::computeCFARThresholdRelaxed(const std::vector<double>& referenceCells)
// {
//     if (referenceCells.empty()) return 1e12;

//     double sum = 0.0;
//     for (double v : referenceCells) sum += v;
//     double noiseMean = sum / static_cast<double>(referenceCells.size());

//     double N        = static_cast<double>(referenceCells.size());
//     double relaxPfa = std::min(1e-4, config.targetPfa * 100.0);  // 2 decades relaxed
//     double alpha    = N * (std::pow(relaxPfa, -1.0 / N) - 1.0);
//     return noiseMean * alpha;
// }
// // ===========================================================================
// // Propagation attenuation — rain (ITU-R P.838) + fog (Kunkel model)
// // Returns a LINEAR scale factor [0..1] to multiply received power by.
// // Both models are one-way; we square for the two-way radar path.
// // ===========================================================================
// double RadarModel::computePropagationLoss(double range_m) const
// {
//     double totalLoss_dB = 0.0;

//     // --- Rain (ITU-R P.838-3 simplified, X-band ~8-10 GHz) ---
//     // k=0.00887, alpha=1.255 for 9 GHz horizontal polarisation
//     if (config.rainRate_mmph > 0.0) {
//         constexpr double k_rain  = 0.00887;
//         constexpr double a_rain  = 1.255;
//         double gamma = k_rain * std::pow(config.rainRate_mmph, a_rain); // dB/km one-way
//         totalLoss_dB += 2.0 * gamma * (range_m / 1000.0);              // two-way
//     }

//     // --- Fog (Kunkel 1984: L = 0.0157 * M^1.05 dB/km, M = liquid water g/m³) ---
//     // Approximate M from visibility: M ≈ 0.0367 * (1000/vis)^1.43
//     if (config.fogVisibility_m > 1.0 && config.fogVisibility_m < 2000.0) {
//         double M     = 0.0367 * std::pow(1000.0 / config.fogVisibility_m, 1.43);
//         double gamma = 0.0157 * std::pow(M, 1.05);                     // dB/km one-way
//         totalLoss_dB += 2.0 * gamma * (range_m / 1000.0);              // two-way
//     }

//     // Convert total dB loss to linear scale factor
//     return std::pow(10.0, -totalLoss_dB / 10.0);
// }
// // ===========================================================================
// // CFAR reference cells — SINR domain (exponential distribution, mean = 1)
// // ===========================================================================
// std::vector<double> RadarModel::generateReferenceCells(SurfaceType surface)
// {
//     std::vector<double> cells;
//     cells.reserve(16);

//     //static std::exponential_distribution<double> cellDist(1.0);
//     thread_local std::exponential_distribution<double> cellDist(1.0);

//     for (int i = 0; i < 16; ++i) {
//         double cell = cellDist(rng);
//         if (surface == SurfaceType::SEA)
//             cell *= (1.0 + config.seaState * 0.3);
//         else if (surface == SurfaceType::LAND)
//             cell *= (1.0 + config.landClutter * 0.5);
//         cells.push_back(cell);
//     }
//     return cells;
// }
// // In radarmodel.cpp — add this function
// double RadarModel::computeMaxDetectionRange(double rcs) const
// {
//     double lambda  = SPEED_OF_LIGHT / config.frequency_Hz;
//     double Pt      = config.emissionPower_kW * 1000.0;
//     double G_lin   = std::pow(10.0, config.antennaGain / 10.0);
//     double Pn      = computeNoisePower();

//     double N        = 16.0;
//     double alpha    = N * (std::pow(config.targetPfa, -1.0 / N) - 1.0);

//     // FIX 1: use average aspect factor 0.6 (not full broadside RCS)
//     // FIX 2: iterate to include clutter + jammer at estimated range
//     // FIX 5: include propagation loss at estimated range
//     double R_est = 50000.0;  // initial guess 50 km in metres
//     for (int i = 0; i < 5; ++i)
//     {
//         double Pc_sea     = computeClutterPower(R_est, SurfaceType::SEA);
//         double Pc_land    = computeClutterPower(R_est, SurfaceType::LAND);
//         double Pc         = std::max(Pc_sea, Pc_land);  // worst-case clutter
//         double Pj         = computeJammerPower(R_est);
//         double Pn_total   = Pn + Pc + Pj;

//         // FIX 5: two-way propagation loss (squared because radar path is out+back)
//         double propFactor = computePropagationLoss(R_est);
//         double Pt_eff     = Pt * propFactor * propFactor;

//         // FIX 1: rcs * 0.6 = average aspect factor
//         double num = Pt_eff * G_lin * G_lin * lambda * lambda * (rcs * 0.6);
//         double den = std::pow(4.0 * M_PI, 3) * Pn_total * alpha;

//         if (den <= 0.0) break;
//         R_est = std::pow(num / den, 0.25);
//     }

//     // Horizon limit
//     double Re      = 6371000.0 * config.earthRadiusFactor * config.atmosphericFactor;
//     double horizon = std::sqrt(2.0 * Re * config.radarHeight) / 1000.0;  // km

//     // FIX 3: never show range inside the blind zone
//     double R_km = std::min(R_est / 1000.0, horizon);
//     return std::max(R_km, config.minDetectableRange / 1000.0 * 2.0);
// }
// // double RadarModel::computeMaxDetectionRange(double rcs) const
// // {
// //     double lambda  = SPEED_OF_LIGHT / config.frequency_Hz;
// //     double Pt      = config.emissionPower_kW * 1000.0;
// //     double G_lin   = std::pow(10.0, config.antennaGain / 10.0);
// //     double Pn      = computeNoisePower();

// //     // SNR_min from Pfa using inverse of approximation
// //     // For Pfa=1e-6 → SNR_min ≈ 13.2 dB linear ≈ 20.9
// //     double N        = 16.0;   // reference cells (matches generateReferenceCells)
// //     double alpha    = N * (std::pow(config.targetPfa, -1.0 / N) - 1.0);
// //     double SNR_min  = alpha;   // CFAR threshold multiplier

// //     double numerator   = Pt * G_lin * G_lin * lambda * lambda * rcs;
// //     double denominator = std::pow(4.0 * M_PI, 3) * Pn * SNR_min;

// //     double R_max_m = std::pow(numerator / denominator, 0.25);

// //     // Also apply horizon limit
// //     double Re      = 6371000.0 * config.earthRadiusFactor * config.atmosphericFactor;
// //     double horizon = std::sqrt(2.0 * Re * config.radarHeight) / 1000.0; // km

// //     return std::min(R_max_m / 1000.0, horizon);  // return in km
// // }
// // ===========================================================================
// // Signal strength — returns pure received power Pr (watts)
// // ===========================================================================
// double RadarModel::calculateSignalStrength(double range, double rcs)
// {
//     if (range < 1.0) range = 1.0;

//     double freq = config.frequency_Hz;
//     if (config.frequencyAgility)
//         freq += static_cast<double>(config.hopStepFrequency * config.hopRate);

//     double lambda = SPEED_OF_LIGHT / freq;
//     double Pt     = config.emissionPower_kW * 1000.0;
//     double G_lin  = std::pow(10.0, config.antennaGain / 10.0);

//     double numerator   = Pt * G_lin * G_lin * lambda * lambda * rcs;
//     double denominator = std::pow(4.0 * M_PI, 3) * std::pow(range, 4);

//     double Pr = numerator / denominator;
//     Pr *= computePropagationLoss(range);
//     return std::max(0.0, Pr);
//    // return std::max(0.0, numerator / denominator);
// }

// // ===========================================================================
// // PRODUCTION FIX — Scan boundary detection
// //
// // A scan boundary occurs when the antenna crosses the scan reversal point
// // (sector scan) or completes a full 360° sweep (surveillance).
// // We detect it by watching for a direction change or azimuth wrap-around.
// // ===========================================================================
// bool RadarModel::detectScanBoundary(double prevAz, double newAz) const
// {
//     bool fullRotation = (config.minAzimuth <= -180.0f && config.maxAzimuth >= 180.0f);

//     if (fullRotation) {
//         // 360° scan: boundary when azimuth wraps from ~359° back to ~0°
//         double delta = newAz - prevAz;
//         // Large backward jump = wrap-around
//         return (delta < -180.0);
//     } else {
//         // Sector scan: boundary at either limit
//         return (newAz <= static_cast<double>(config.minAzimuth) + 0.1 ||
//                 newAz >= static_cast<double>(config.maxAzimuth) - 0.1);
//     }
// }

// // ===========================================================================
// // PRODUCTION FIX — Per-scan miss logic (called once per scan boundary)
// //
// // This is the heart of the fix.  Instead of incrementing missCount every
// // update() call (which fires 10-100× per scan), we increment scanMissCount
// // exactly once per completed antenna scan.
// //
// // Dropout rule:  scanMissCount > missedScansToDrop  (default = 5 scans)
// // Hard timeout:  (currentTime - lastSeenTime) > trackCoastSeconds (default 60s)
// //
// // This matches how production radar trackers implement M-of-N logic.
// // ===========================================================================
// void RadarModel::applyScanMissLogic(double currentTime)
// {
//     for (auto& track : trackDatabase) {
//         if (!track.updatedThisScan) {
//             // No detection during the scan that just completed
//             track.scanMissCount++;
//             track.missCount++;   // keep for diagnostics / logging
//         } else {
//             // Had at least one detection this scan — reset miss counter
//             track.scanMissCount = 0;
//         }
//         // Reset for next scan
//         track.updatedThisScan = false;
//     }

//     // Prune tracks — two independent conditions, both must be checked
//     trackDatabase.erase(
//         std::remove_if(trackDatabase.begin(), trackDatabase.end(),
//                        [&](const TrackFile& t) {
//                            bool tooManyMissedScans =
//                                (t.scanMissCount > config.missedScansToDrop);
//                            bool coastTimeout =
//                                ((currentTime - t.lastSeenTime) > config.trackCoastSeconds);
//                            return tooManyMissedScans || coastTimeout;
//                        }),
//         trackDatabase.end());
// }

// // ===========================================================================
// // Antenna — LOCK_ON mode
// // ===========================================================================
// bool RadarModel::updateAntennaLockOn(const std::vector<TargetInput>& worldTargets)
// {
//     for (const auto& t : worldTargets) {
//         if (t.id != config.lockedTargetID) continue;

//         double range = std::sqrt(t.x*t.x + t.y*t.y + t.z*t.z);
//         if (range < 1e-6) continue;

//         double targetAz = std::atan2(t.y, t.x) * (180.0 / M_PI);
//         if (targetAz < 0.0) targetAz += 360.0;

//         double ratio    = std::clamp(t.z / range, -1.0, 1.0);
//         double targetEl = std::asin(ratio) * (180.0 / M_PI);

//         currentAzimuth   = targetAz;
//         currentElevation = targetEl;
//         return true;
//     }
//     return false;
// }

// // ===========================================================================
// // Antenna — Scanning modes
// // ===========================================================================
// void RadarModel::updateAntennaScan(double dt)
// {
//     previousAzimuth = currentAzimuth;

//     double rotationSpeed = (static_cast<double>(config.scanningRate_RPM) / 60.0) * 360.0;
//     bool   fullRotation  = (config.minAzimuth <= -180.0f && config.maxAzimuth >= 180.0f);

//     if (fullRotation) {
//         currentAzimuth += rotationSpeed * dt;
//         if (currentAzimuth >= 360.0) currentAzimuth -= 360.0;
//     } else {
//         currentAzimuth += scanDirection * rotationSpeed * dt;
//         if (currentAzimuth > static_cast<double>(config.maxAzimuth)) {
//             currentAzimuth = config.maxAzimuth;
//             scanDirection  = -1.0;
//         }
//         if (currentAzimuth < static_cast<double>(config.minAzimuth)) {
//             currentAzimuth = config.minAzimuth;
//             scanDirection  =  1.0;
//         }
//     }

//     double centerEl = (config.minElevation + config.maxElevation) / 2.0;
//     if (config.scanType == ScanType::CONICAL)
//         currentElevation = centerEl +
//                            std::sin(currentAzimuth * M_PI / 180.0) * (config.beamWidth / 4.0);
//     else
//         currentElevation = centerEl;

//     // Detect scan boundary this cycle
//     scanBoundaryOccurred = detectScanBoundary(previousAzimuth, currentAzimuth);
// }

// // ===========================================================================
// // Beam check
// // ===========================================================================
// bool RadarModel::isTargetInBeam(double targetAz, double targetEl, double dt,
//                                 double& outAzDiff, double& outElDiff,
//                                 double& outScanMargin)
// {
//     outAzDiff = std::abs(currentAzimuth - targetAz);
//     if (outAzDiff > 180.0) outAzDiff = 360.0 - outAzDiff;

//     outElDiff = std::abs(currentElevation - targetEl);

//     double scanSpeed  = (config.scanningRate_RPM / 60.0) * 360.0;
//     outScanMargin     = (config.beamWidth * 2.5) + (scanSpeed * dt);

//     return (outAzDiff <= outScanMargin && outElDiff <= outScanMargin);
// }

// // ===========================================================================
// // Kalman prediction — called every update() cycle
// // ===========================================================================
// void RadarModel::updateTWS(double dt, double /*currentTime*/)
// {
//     for (auto& track : trackDatabase) {
//         // Constant-velocity state transition
//         double F[6][6] = {
//             {1,0,0,dt, 0, 0 },
//             {0,1,0, 0,dt, 0 },
//             {0,0,1, 0, 0,dt },
//             {0,0,0, 1, 0, 0 },
//             {0,0,0, 0, 1, 0 },
//             {0,0,0, 0, 0, 1 }
//         };

//         // State prediction:  X = F·X
//         std::array<double, 6> Xnew = {};
//         for (int i = 0; i < 6; ++i)
//             for (int j = 0; j < 6; ++j)
//                 Xnew[i] += F[i][j] * track.X[j];
//         track.X = Xnew;

//         // Covariance prediction:  P = F·P·Fᵀ + Q
//         double FP[6][6] = {};
//         for (int i = 0; i < 6; ++i)
//             for (int j = 0; j < 6; ++j)
//                 for (int k = 0; k < 6; ++k)
//                     FP[i][j] += F[i][k] * track.P[k][j];

//         for (int i = 0; i < 6; ++i)
//             for (int j = 0; j < 6; ++j) {
//                 double tmp = 0.0;
//                 for (int k = 0; k < 6; ++k)
//                     tmp += FP[i][k] * F[j][k];   // FP · Fᵀ
//                 track.P[i][j] = tmp + track.Q[i][j];
//             }

//         // Sync scalar fields from state vector
//         track.x  = track.X[0]; track.y  = track.X[1]; track.z  = track.X[2];
//         track.vx = track.X[3]; track.vy = track.X[4]; track.vz = track.X[5];

//         // PRODUCTION FIX: update predictedRange but do NOT overwrite
//         //                  track.range — keep last measured range for reporting
//         track.predictedRange = std::sqrt(
//             track.x*track.x + track.y*track.y + track.z*track.z);

//         // Reset per-cycle update flag (NOT the per-scan flag)
//         track.isUpdated = false;
//     }
//     // NOTE: track pruning is now done in applyScanMissLogic(), not here.
//     // We do NOT erase tracks in updateTWS() anymore.
// }

// // ===========================================================================
// // Kalman update — called when a detection is associated to a track
// // ===========================================================================
// void RadarModel::performKalmanUpdate(TrackFile& track,
//                                      const DetectionOutput& det,
//                                      double currentTime,
//                                      double /*dt*/,
//                                      double maxUnambiguousRange)
// {
//     double bestRange = resolveRangeAmbiguity(
//         det.range, track.predictedRange, maxUnambiguousRange);

//     double azRad = det.azimuth   * M_PI / 180.0;
//     double elRad = det.elevation * M_PI / 180.0;

//     // Measurement in Cartesian
//     double z[3] = {
//         bestRange * std::cos(elRad) * std::cos(azRad),
//         bestRange * std::cos(elRad) * std::sin(azRad),
//         bestRange * std::sin(elRad)
//     };

//     // Innovation
//     double y[3] = {
//         z[0] - track.X[0],
//         z[1] - track.X[1],
//         z[2] - track.X[2]
//     };

//     // Innovation covariance S = P[0:3,0:3] + R
//     double S[3][3] = {};
//     for (int i = 0; i < 3; ++i)
//         for (int j = 0; j < 3; ++j)
//             S[i][j] = track.P[i][j] + track.R[i][j];

//     double detS = S[0][0]*(S[1][1]*S[2][2] - S[1][2]*S[2][1])
//                   - S[0][1]*(S[1][0]*S[2][2] - S[1][2]*S[2][0])
//                   + S[0][2]*(S[1][0]*S[2][1] - S[1][1]*S[2][0]);

//     if (std::abs(detS) < 1e-3) {
//         track.hitCount = std::max(0, track.hitCount - 1);
//         return;
//     }

//     double invS[3][3];
//     invS[0][0] = (S[1][1]*S[2][2] - S[1][2]*S[2][1]) / detS;
//     invS[0][1] = (S[0][2]*S[2][1] - S[0][1]*S[2][2]) / detS;
//     invS[0][2] = (S[0][1]*S[1][2] - S[0][2]*S[1][1]) / detS;
//     invS[1][0] = (S[1][2]*S[2][0] - S[1][0]*S[2][2]) / detS;
//     invS[1][1] = (S[0][0]*S[2][2] - S[0][2]*S[2][0]) / detS;
//     invS[1][2] = (S[0][2]*S[1][0] - S[0][0]*S[1][2]) / detS;
//     invS[2][0] = (S[1][0]*S[2][1] - S[1][1]*S[2][0]) / detS;
//     invS[2][1] = (S[0][1]*S[2][0] - S[0][0]*S[2][1]) / detS;
//     invS[2][2] = (S[0][0]*S[1][1] - S[0][1]*S[1][0]) / detS;

//     // Kalman gain K = P·Hᵀ·S⁻¹  (H selects first 3 states)
//     double K[6][3] = {};
//     for (int i = 0; i < 6; ++i)
//         for (int j = 0; j < 3; ++j)
//             for (int k = 0; k < 3; ++k)
//                 K[i][j] += track.P[i][k] * invS[k][j];

//     // State update:  X = X + K·y
//     for (int i = 0; i < 6; ++i)
//         for (int j = 0; j < 3; ++j)
//             track.X[i] += K[i][j] * y[j];

//     // Covariance update — Joseph form for numerical stability
//     double I_KH[6][6] = {};
//     for (int i = 0; i < 6; ++i) {
//         I_KH[i][i] = 1.0;
//         for (int j = 0; j < 3; ++j)
//             I_KH[i][j] -= K[i][j];
//     }

//     double Pnew[6][6] = {};
//     for (int i = 0; i < 6; ++i)
//         for (int j = 0; j < 6; ++j)
//             for (int k = 0; k < 6; ++k)
//                 Pnew[i][j] += I_KH[i][k] * track.P[k][j];

//     for (int i = 0; i < 6; ++i)
//         for (int j = 0; j < 6; ++j)
//             track.P[i][j] = Pnew[i][j];

//     // Velocity sanity clamp
//     for (int i = 3; i < 6; ++i)
//         track.X[i] = std::clamp(track.X[i], -config.maxTrackSpeed, config.maxTrackSpeed);

//     // Sync scalar fields
//     track.x  = track.X[0]; track.y  = track.X[1]; track.z  = track.X[2];
//     track.vx = track.X[3]; track.vy = track.X[4]; track.vz = track.X[5];

//     track.range = std::sqrt(track.x*track.x + track.y*track.y + track.z*track.z);
//     track.velocity = (track.range > 1e-6)
//                          ? (track.vx*track.x + track.vy*track.y + track.vz*track.z) / track.range
//                          : 0.0;

//     track.lastSeenTime    = currentTime;
//     track.hitCount++;
//     track.isUpdated       = true;
//     track.updatedThisScan = true;   // PRODUCTION: mark hit for this scan
//     track.missCount       = 0;
//     track.wasAmbiguous    = det.isAmbiguous;

//     if (track.hitCount >= config.minHitsToValidate)
//         track.isValidated = true;
// }

// // ===========================================================================
// // Create new track
// // ===========================================================================
// void RadarModel::createNewTrack(const DetectionOutput& det,
//                                 const TargetInput& target,
//                                 double maxUnambiguousRange,
//                                 double currentTime)
// {
//     // Duplicate guard
//     for (const auto& t : trackDatabase)
//         if (t.id == det.targetID) return;

//     TrackFile newTrack;
//     newTrack.id = det.targetID;

//     double unfoldedRange = det.range;
//     if (det.isAmbiguous && det.radialVelocity < 0.0)
//         unfoldedRange = det.range + maxUnambiguousRange;

//     double azRad = det.azimuth   * M_PI / 180.0;
//     double elRad = det.elevation * M_PI / 180.0;

//     newTrack.x  = unfoldedRange * std::cos(elRad) * std::cos(azRad);
//     newTrack.y  = unfoldedRange * std::cos(elRad) * std::sin(azRad);
//     newTrack.z  = unfoldedRange * std::sin(elRad);
//     newTrack.vx = target.vx;
//     newTrack.vy = target.vy;
//     newTrack.vz = target.vz;

//     newTrack.X = { newTrack.x, newTrack.y, newTrack.z,
//                   newTrack.vx, newTrack.vy, newTrack.vz };

//     double posVar = config.noise.rangeStdDev * config.noise.rangeStdDev;
//     double velVar = 500.0 * 500.0;

//     for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j) newTrack.P[i][j] = 0.0;
//     newTrack.P[0][0] = posVar; newTrack.P[1][1] = posVar; newTrack.P[2][2] = posVar;
//     newTrack.P[3][3] = velVar; newTrack.P[4][4] = velVar; newTrack.P[5][5] = velVar;

//     newTrack.Q[0][0] = 10.0; newTrack.Q[1][1] = 10.0; newTrack.Q[2][2] = 10.0;
//     newTrack.Q[3][3] =  1.0; newTrack.Q[4][4] =  1.0; newTrack.Q[5][5] =  1.0;

//     for (int i = 0; i < 3; ++i) newTrack.R[i][i] = 25.0;

//     newTrack.range          = unfoldedRange;
//     newTrack.predictedRange = unfoldedRange;
//     newTrack.velocity       = det.radialVelocity;
//     newTrack.lastSeenTime   = currentTime;
//     newTrack.hitCount       = 1;
//     newTrack.scanMissCount  = 0;
//     newTrack.updatedThisScan = true;   // just created — counts as a hit
//     newTrack.wasAmbiguous   = det.isAmbiguous;

//     // Cap database size
//     if (trackDatabase.size() > 2000)
//         trackDatabase.erase(trackDatabase.begin());

//     trackDatabase.push_back(std::move(newTrack));
// }

// // ===========================================================================
// // Process single target detection
// // ===========================================================================
// bool RadarModel::processTargetDetection(
//     const TargetInput& target,
//     double dt, double currentTime,
//     double maxUnambiguousRange,
//     std::vector<DetectionOutput>& scanDetections,
//     std::normal_distribution<double>& rangeNoise,
//     std::normal_distribution<double>& azNoise,
//     std::normal_distribution<double>& elNoise,
//     std::normal_distribution<double>& dopplerNoise)
// {
//     double range = std::sqrt(target.x*target.x + target.y*target.y + target.z*target.z);

//     if (range < config.minDetectableRange) return false;
//     if (!checkHorizon(range, target.z))   return false;

//     double targetAz = std::atan2(target.y, target.x) * (180.0 / M_PI);
//     if (targetAz < 0.0) targetAz += 360.0;
//     if (range < 1e-6)  return false;

//     double ratio    = std::clamp(target.z / range, -1.0, 1.0);
//     double targetEl = std::asin(ratio) * (180.0 / M_PI);

//     double azDiff, elDiff, scanMargin;
//     if (!isTargetInBeam(targetAz, targetEl, dt, azDiff, elDiff, scanMargin))
//         return false;

//     // Signal chain
//     double effectiveRCS  = computeEffectiveRCS(target, range);
//     double receivedPower = calculateSignalStrength(range, effectiveRCS);
//     double sinr          = applyClutterEffects(receivedPower, range, target.surface);

//     auto   referenceCells = generateReferenceCells(target.surface);

//     // PRODUCTION FIX: compute radial velocity first so we can choose
//     //                 which CFAR threshold to apply
//     double radialVel = computeRadialVelocity(target, range, dopplerNoise);
//     bool   isStationary = (std::abs(radialVel) < STATIONARY_VEL_THRESHOLD);

//     double threshold = isStationary
//                            ? computeCFARThresholdRelaxed(referenceCells)   // relaxed for zero-Doppler
//                            : computeCFARThreshold(referenceCells);

//     if (sinr <= threshold) return false;

//     // Build detection output
//     DetectionOutput det;
//     det.targetID      = target.id;
//     det.azimuth       = targetAz + azNoise(rng);
//     det.elevation     = targetEl + elNoise(rng);
//     det.snr           = sinr;
//     det.radialVelocity = radialVel;

//     computeTargetMotionParams(det, target, range);
//     computeCPA(det, target, range);
//     det.Pk = computePk(range, det.radialVelocity);

//     applyRangeAmbiguity(det, range, maxUnambiguousRange, rangeNoise);

//     if (config.mode == RadarMode::LOCK_ON) {
//         resolveRangeForLockOn(det, range, maxUnambiguousRange, target.id);
//     } else {
//         if (!det.isAmbiguous && det.range < config.minDetectableRange)
//             det.range = config.minDetectableRange;
//     }

//     if (shouldMergeDetection(det, scanDetections)) return false;

//     scanDetections.push_back(det);

//     // TWS track association
//     // if (config.mode == RadarMode::TWS) {
//     //     double     bestProb  = 0.0;
//     //     TrackFile* bestTrack = findBestTrackMatch(det, maxUnambiguousRange, bestProb);

//     //     if (bestTrack)
//     //         performKalmanUpdate(*bestTrack, det, currentTime, dt, maxUnambiguousRange);
//     //     else
//     //         createNewTrack(det, target, maxUnambiguousRange, currentTime);
//     // }
//     if (config.mode == RadarMode::TWS || config.mode == RadarMode::LOCK_ON) {
//         double     bestProb  = 0.0;
//         TrackFile* bestTrack = findBestTrackMatch(det, maxUnambiguousRange, bestProb);

//         if (bestTrack)
//             performKalmanUpdate(*bestTrack, det, currentTime, dt, maxUnambiguousRange);
//         else
//             createNewTrack(det, target, maxUnambiguousRange, currentTime);
//     }
//     lastDetections.push_back(det);
//     return true;
// }

// // ===========================================================================
// // Main update loop
// // ===========================================================================
// void RadarModel::update(double dt,
//                         const std::vector<TargetInput>& worldTargets,
//                         double currentTime)
// {
//     std::lock_guard<std::mutex> lock(radarMutex);

//     lastDetections.clear();

//     std::normal_distribution<double> rangeNoise  (0.0, config.noise.rangeStdDev);
//     std::normal_distribution<double> azNoise     (0.0, config.noise.azimuthStdDev);
//     std::normal_distribution<double> elNoise     (0.0, config.noise.elevationStdDev);
//     std::normal_distribution<double> dopplerNoise(0.0, config.noise.dopplerStdDev);

//     double currentPRF = (config.prfLevels[0] > 0) ? config.prfLevels[0] : 5000.0;
//     double maxUnambiguousRange = SPEED_OF_LIGHT / (2.0 * currentPRF);

//     // TWS Kalman prediction (every cycle)
//     // if (config.mode == RadarMode::TWS)
//     //     updateTWS(dt, currentTime);
//     if (config.mode == RadarMode::TWS || config.mode == RadarMode::LOCK_ON)
//         updateTWS(dt, currentTime);

//     // Antenna control
//     scanBoundaryOccurred = false;
//     if (config.mode == RadarMode::LOCK_ON) {
//         if (!updateAntennaLockOn(worldTargets))
//             config.mode = RadarMode::SURVEILLANCE;
//     } else {
//         updateAntennaScan(dt);   // sets scanBoundaryOccurred
//     }

//     // Detection pass
//     bool lockedTargetVisible = false;
//     std::vector<DetectionOutput> scanDetections;
//     scanDetections.reserve(worldTargets.size());

//     for (const auto& target : worldTargets) {
//         if (config.mode == RadarMode::LOCK_ON && target.id != config.lockedTargetID)
//             continue;

//         bool detected = processTargetDetection(
//             target, dt, currentTime, maxUnambiguousRange,
//             scanDetections, rangeNoise, azNoise, elNoise, dopplerNoise);

//         if (detected && config.mode == RadarMode::LOCK_ON &&
//             target.id == config.lockedTargetID)
//             lockedTargetVisible = true;
//     }

//     // PRODUCTION FIX: apply per-scan miss logic exactly once per scan boundary
//     if (config.mode == RadarMode::TWS && scanBoundaryOccurred)
//         applyScanMissLogic(currentTime);

//     // TWS reporting
//     if (config.mode == RadarMode::TWS)
//         generateTWSReport();

//     // Break-lock
//     if (config.mode == RadarMode::LOCK_ON && !lockedTargetVisible) {
//         config.mode           = RadarMode::SURVEILLANCE;
//         config.lockedTargetID = 0;
//     }
// }

// // ===========================================================================
// // TWS report builder
// // ===========================================================================
// DetectionOutput RadarModel::buildTWSOutput(const TrackFile& track)
// {
//     DetectionOutput out;

//     // Use last measured range if updated this cycle, else predicted
//     double reportRange = track.isUpdated ? track.range : track.predictedRange;

//     out.targetID       = track.id;
//     out.range          = reportRange;
//     out.radialVelocity = track.velocity;
//     out.isAmbiguous    = false;

//     // CPA
//     double v_sq = track.vx*track.vx + track.vy*track.vy + track.vz*track.vz;
//     if (v_sq > 0.01) {
//         double t_cpa = -(track.x*track.vx + track.y*track.vy + track.z*track.vz) / v_sq;
//         if (t_cpa > 0.0) {
//             out.time_to_cpa  = t_cpa;
//             double cx = track.x + track.vx*t_cpa;
//             double cy = track.y + track.vy*t_cpa;
//             double cz = track.z + track.vz*t_cpa;
//             out.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
//         } else {
//             out.time_to_cpa  = 0.0;
//             out.cpa_distance = reportRange;
//         }
//     } else {
//         out.time_to_cpa  = 0.0;
//         out.cpa_distance = reportRange;
//     }

//     out.Pk = std::min(0.99,
//                       0.95 * std::exp(-reportRange / 45000.0) * (track.velocity < 0.0 ? 1.2 : 0.8));

//     // out.speedOverGround = std::sqrt(
//     //     track.vx*track.vx + track.vy*track.vy + track.vz*track.vz);

//     // CORRECT — horizontal only:
//     out.speedOverGround = std::sqrt(
//         track.vx*track.vx + track.vz*track.vz);
//    // out.heading = std::atan2(track.vy, track.vx) * (180.0 / M_PI);
//     // SHOULD BE — matching the fix in computeTargetMotionParams:
//     out.heading = std::atan2(track.vx, track.vz) * (180.0 / M_PI);
//     if (out.heading < 0.0) out.heading += 360.0;

//     out.acceleration = 0.0;

//     double losAngle = std::atan2(track.y, track.x) * (180.0 / M_PI);
//     if (losAngle < 0.0) losAngle += 360.0;

//     out.targetAspect = std::abs(out.heading - losAngle);
//     if (out.targetAspect > 180.0) out.targetAspect = 360.0 - out.targetAspect;

//     out.azimuth = losAngle;

//     if (reportRange > 1e-6) {
//         out.elevation = std::asin(
//                             std::clamp(track.z / reportRange, -1.0, 1.0)) * (180.0 / M_PI);
//     }

//     return out;
// }

// void RadarModel::generateTWSReport()
// {
//     lastDetections.clear();
//     for (auto& track : trackDatabase) {
//         if (!track.isValidated) continue;
//         lastDetections.push_back(buildTWSOutput(track));
//         track.isUpdated = false;
//     }
// }

// // ===========================================================================
// // Remaining functions — unchanged from your original
// // ===========================================================================
// double RadarModel::computeAssociationProbability(double measurementRange,
//                                                  double predictedRange,
//                                                  double gateSize)
// {
//     double error  = measurementRange - predictedRange;
//     double sigma  = gateSize / 2.0;
//     return std::exp(-(error*error) / (2.0*sigma*sigma));
// }

// TrackFile* RadarModel::findBestTrackMatch(const DetectionOutput& det,
//                                           double maxUnambiguousRange,
//                                           double& outBestProb)
// {
//     TrackFile* bestTrack = nullptr;
//     outBestProb = 0.0;

//     for (auto& track : trackDatabase) {
//         if (!std::isfinite(track.x) || !std::isfinite(track.y) ||
//             !std::isfinite(track.z) || !std::isfinite(track.vx) ||
//             !std::isfinite(track.vy) || !std::isfinite(track.vz)) {
//             track.hitCount    = 0;
//             track.isValidated = false;
//             continue;
//         }
//         if (track.isUpdated) continue;

//         double candidateRange = resolveRangeAmbiguity(
//             det.range, track.predictedRange, maxUnambiguousRange);

//         if (std::abs(candidateRange - track.predictedRange) > RANGE_GATE) continue;

//         double azRad = det.azimuth   * M_PI / 180.0;
//         double elRad = det.elevation * M_PI / 180.0;

//         double zx = det.range * std::cos(elRad) * std::cos(azRad);
//         double zy = det.range * std::cos(elRad) * std::sin(azRad);
//         double zz = det.range * std::sin(elRad);

//         double dx = zx - track.X[0];
//         double dy = zy - track.X[1];
//         double dz = zz - track.X[2];

//         double S[3][3];
//         for (int i = 0; i < 3; ++i)
//             for (int j = 0; j < 3; ++j)
//                 S[i][j] = track.P[i][j] + track.R[i][j];

//         double detS = S[0][0]*(S[1][1]*S[2][2] - S[1][2]*S[2][1])
//                       - S[0][1]*(S[1][0]*S[2][2] - S[1][2]*S[2][0])
//                       + S[0][2]*(S[1][0]*S[2][1] - S[1][1]*S[2][0]);
//         if (std::abs(detS) < 1e-2) continue;

//         double invS[3][3];
//         invS[0][0]=(S[1][1]*S[2][2]-S[1][2]*S[2][1])/detS;
//         invS[0][1]=(S[0][2]*S[2][1]-S[0][1]*S[2][2])/detS;
//         invS[0][2]=(S[0][1]*S[1][2]-S[0][2]*S[1][1])/detS;
//         invS[1][0]=(S[1][2]*S[2][0]-S[1][0]*S[2][2])/detS;
//         invS[1][1]=(S[0][0]*S[2][2]-S[0][2]*S[2][0])/detS;
//         invS[1][2]=(S[0][2]*S[1][0]-S[0][0]*S[1][2])/detS;
//         invS[2][0]=(S[1][0]*S[2][1]-S[1][1]*S[2][0])/detS;
//         invS[2][1]=(S[0][1]*S[2][0]-S[0][0]*S[2][1])/detS;
//         invS[2][2]=(S[0][0]*S[1][1]-S[0][1]*S[1][0])/detS;

//         double d2 = dx*(invS[0][0]*dx + invS[0][1]*dy + invS[0][2]*dz)
//                     + dy*(invS[1][0]*dx + invS[1][1]*dy + invS[1][2]*dz)
//                     + dz*(invS[2][0]*dx + invS[2][1]*dy + invS[2][2]*dz);

//         if (d2 > 9.21) continue;   // χ² gate (3-DOF, 99%)

//         double prob = computeAssociationProbability(
//             candidateRange, track.predictedRange, RANGE_GATE);

//         if (prob > outBestProb) {
//             outBestProb = prob;
//             bestTrack   = &track;
//         }
//     }
//     return bestTrack;
// }

// void RadarModel::computeTargetMotionParams(DetectionOutput& det,
//                                            const TargetInput& target,
//                                            double range)
// {
//     det.speedOverGround = std::sqrt(
//         target.vx*target.vx + target.vz*target.vz);  // horizontal only

//     det.heading = std::atan2(target.vx, target.vz) * (180.0 / M_PI);  // X/Z not X/Y
//     if (det.heading < 0.0) det.heading += 360.0;

//     det.acceleration = 0.0;

//     if (det.speedOverGround > 0.01) {
//         double vx_n = target.vx / det.speedOverGround;
//         double vz_n = target.vz / det.speedOverGround;
//         double dot  = std::clamp(vx_n*(target.x/range) +
//                                     vz_n*(target.z/range), -1.0, 1.0);
//         det.targetAspect = std::acos(dot) * 180.0 / M_PI;
//     } else {
//         det.targetAspect = 0.0;
//     }
// }
// // void RadarModel::computeTargetMotionParams(DetectionOutput& det,
// //                                            const TargetInput& target,
// //                                            double range)
// // {
// //     det.speedOverGround = std::sqrt(
// //         target.vx*target.vx + target.vy*target.vy + target.vz*target.vz);

// //     det.heading = std::atan2(target.vy, target.vx) * (180.0 / M_PI);
// //     if (det.heading < 0.0) det.heading += 360.0;
// //     det.acceleration = 0.0;

// //     if (det.speedOverGround > 0.01) {
// //         double vx_n = target.vx / det.speedOverGround;
// //         double vy_n = target.vy / det.speedOverGround;
// //         double vz_n = target.vz / det.speedOverGround;
// //         double dot  = std::clamp(vx_n*(target.x/range) +
// //                                     vy_n*(target.y/range) +
// //                                     vz_n*(target.z/range), -1.0, 1.0);
// //         det.targetAspect = std::acos(dot) * 180.0 / M_PI;
// //     } else {
// //         det.targetAspect = 0.0;
// //     }
// // }

// double RadarModel::computeRadialVelocity(const TargetInput& target,
//                                          double range,
//                                          std::normal_distribution<double>& dopplerNoise)
// {
//     double dot = target.vx*target.x + target.vy*target.y + target.vz*target.z;
//     return ((range > 1e-6) ? dot / range : 0.0) + dopplerNoise(rng);
// }

// void RadarModel::computeCPA(DetectionOutput& det,
//                             const TargetInput& target,
//                             double range)
// {
//     det.cpa_distance = range;
//     det.time_to_cpa  = 0.0;

//     double v_sq = target.vx*target.vx + target.vy*target.vy + target.vz*target.vz;
//     if (v_sq > 0.01) {
//         double t_cpa = -(target.x*target.vx + target.y*target.vy + target.z*target.vz) / v_sq;
//         det.time_to_cpa = (t_cpa > 0.0) ? t_cpa : 0.0;
//         double cx = target.x + target.vx * det.time_to_cpa;
//         double cy = target.y + target.vy * det.time_to_cpa;
//         double cz = target.z + target.vz * det.time_to_cpa;
//         det.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
//     }
// }

// double RadarModel::computePk(double range, double radialVelocity)
// {
//     return std::min(0.99,
//                     0.95 * std::exp(-range / 45000.0) * (radialVelocity < 0.0 ? 1.2 : 0.8));
// }

// void RadarModel::applyRangeAmbiguity(DetectionOutput& det, double range,
//                                      double maxUnambiguousRange,
//                                      std::normal_distribution<double>& rangeNoise)
// {
//     if (range > maxUnambiguousRange) {
//         det.range       = std::fmod(range, maxUnambiguousRange) + rangeNoise(rng);
//         det.isAmbiguous = true;
//     } else {
//         det.range       = range + rangeNoise(rng);
//         det.isAmbiguous = false;
//     }
// }

// void RadarModel::resolveRangeForLockOn(DetectionOutput& det, double range,
//                                        double maxUnambiguousRange, int targetId)
// {
//     double predictedRange = range;
//     for (const auto& track : trackDatabase)
//         if (track.id == static_cast<uint32_t>(targetId)) {
//             predictedRange = track.predictedRange;
//             break;
//         }
//     det.range       = resolveRangeAmbiguity(det.range, predictedRange, maxUnambiguousRange);
//     det.isAmbiguous = false;
// }

// bool RadarModel::shouldMergeDetection(const DetectionOutput& det,
//                                       const std::vector<DetectionOutput>& scanDetections)
// {
//     for (const auto& ex : scanDetections) {
//         double azDiff = std::abs(ex.azimuth - det.azimuth);
//         if (azDiff > 180.0) azDiff = 360.0 - azDiff;
//         if (std::abs(ex.range - det.range) < 150.0 &&
//             azDiff                          < config.beamWidth &&
//             std::abs(ex.elevation - det.elevation) < config.beamWidth)
//             return true;
//     }
//     return false;
// }

// double RadarModel::resolveRangeAmbiguity(double measuredRange,
//                                          double predictedRange,
//                                          double Rmax)
// {
//     if (Rmax < 1.0) return measuredRange;
//     double bestRange = measuredRange, minError = 1e12;
//     for (int k = -5; k <= 5; ++k) {
//         double candidate = measuredRange + k * Rmax;
//         double error     = std::abs(candidate - predictedRange);
//         if (error < minError) { minError = error; bestRange = candidate; }
//     }
//     return bestRange;
// }
// bool RadarModel::checkHorizon(double range, double targetZ)
// {
//     // Effective earth radius = physical radius × earthRadiusFactor × atmosphericFactor
//     // Standard atmosphere:  factor = 1.33  (4/3 earth)
//     // Ducting (warm humid):  atmosphericFactor > 1.0  → extended range
//     // Sub-refraction (cold dry): atmosphericFactor < 1.0 → reduced range
//     double Re      = 6371000.0
//                 * config.earthRadiusFactor
//                 * config.atmosphericFactor;

//     double dRadar  = std::sqrt(2.0 * Re * config.radarHeight);
//     double dTarget = std::sqrt(2.0 * Re * std::max(0.0, targetZ));
//     return range <= (dRadar + dTarget);
// }


// bool RadarModel::checkBeamIntersection(double targetAz, double targetEl)
// {
//     double azDiff = std::abs(currentAzimuth - targetAz);
//     if (azDiff > 180.0) azDiff = 360.0 - azDiff;
//     return std::sqrt(azDiff*azDiff +
//                      std::pow(currentElevation - targetEl, 2)) <= config.beamWidth;
// }

// double RadarModel::computeEffectiveRCS(const TargetInput& target, double range)
// {
//     double velMag = std::sqrt(
//         target.vx*target.vx + target.vy*target.vy + target.vz*target.vz);

//     if (velMag < 0.01) return target.rcs * 0.6;   // stationary — broadside assumption

//     double dot = std::clamp(
//         (target.vx/velMag)*(target.x/range) +
//             (target.vy/velMag)*(target.y/range) +
//             (target.vz/velMag)*(target.z/range), -1.0, 1.0);

//     double aspectRad    = std::acos(dot);
//     double aspectFactor = 0.2 + 0.8 * std::abs(std::sin(aspectRad));
//     return target.rcs * aspectFactor;
// }

// std::vector<DetectionOutput> RadarModel::getActiveDetections()
// {
//     std::lock_guard<std::mutex> lock(radarMutex);
//     return lastDetections;
// }

// std::vector<TrackFile> RadarModel::getActiveTracks()
// {
//     std::lock_guard<std::mutex> lock(radarMutex);
//     std::vector<TrackFile> active;

//     for (const auto& t : trackDatabase) {
//         if (config.mode == RadarMode::LOCK_ON
//             && t.id == config.lockedTargetID) {
//             active.push_back(t);   // always include locked target
//             continue;
//         }
//         if (t.isValidated) active.push_back(t);
//     }
//     return active;
// }
// // std::vector<TrackFile> RadarModel::getActiveTracks()
// // {
// //     std::lock_guard<std::mutex> lock(radarMutex);
// //     std::vector<TrackFile> active;
// //     for (const auto& t : trackDatabase)
// //         if (t.isValidated) active.push_back(t);
// //     return active;
// // }

// void RadarModel::reset()
// {
//     std::lock_guard<std::mutex> lock(radarMutex);
//     currentAzimuth       = 0.0;
//     currentElevation     = 0.0;
//     previousAzimuth      = 0.0;
//     scanBoundaryOccurred = false;
//     trackDatabase.clear();
//     lastDetections.clear();
// }

// extern "C" {
// RadarModel* createRadar()          { return new RadarModel(); }
// void        destroyRadar(RadarModel* obj) { delete obj; }
// }
// #include <cmath>
// #include <algorithm>
// #include <vector>
// #include <random>
// #include <iostream>
// #include <mutex>
// #include <unordered_map>
// #include "radarmodel.h"

// #ifndef M_PI
// #define M_PI 3.14159265358979323846
// #endif
// static constexpr double BOLTZMANN      = 1.380649e-23;
// static constexpr double SPEED_OF_LIGHT = 299792458.0;
// static constexpr double RANGE_GATE     = 2000.0;
// //constexpr double RANGE_GATE = 2000;

// thread_local std::default_random_engine rng(std::random_device{}());

// RadarModel::RadarModel()
//     : currentAzimuth(0.0),
//     currentElevation(0.0),
//     scanDirection(1)
// {
//     trackDatabase.reserve(2048);
// }

// void RadarModel::setConfiguration(const RadarAttributes& attrs) {
//     config = attrs;
// }
// double RadarModel::computeCFARThreshold(const std::vector<double>& referenceCells)
// {
//     if (referenceCells.empty())
//         return 1e12;

//     double sum = 0.0;
//     for (double v : referenceCells)
//         sum += v;

//     double noiseMean = sum / static_cast<double>(referenceCells.size());

//     // CA-CFAR: alpha = N * (Pfa^(-1/N) - 1)
//     double N     = static_cast<double>(referenceCells.size());
//     double alpha = N * (std::pow(config.targetPfa, -1.0 / N) - 1.0);

//     return noiseMean * alpha;
// }
// // double RadarModel::computeCFARThreshold(const std::vector<double>& referenceCells)
// // {
// //     if (referenceCells.empty())
// //         return 1e12;

// //     double sum = 0;
// //     for (double v : referenceCells)
// //         sum += v;

// //     double noiseMean = sum / referenceCells.size();
// //     double scale = 8.0;
// //     return noiseMean * scale;
// // }

// double RadarModel::computeAssociationProbability(
//     double measurementRange,
//     double predictedRange,
//     double gateSize)
// {
//     double error = measurementRange - predictedRange;
//     double sigma = gateSize / 2.0;
//     double exponent = -(error * error) / (2 * sigma * sigma);
//     return std::exp(exponent);
// }

// // ============================================================================
// // SUB-FUNCTION: Update antenna pointing for LOCK_ON mode
// // ============================================================================
// bool RadarModel::updateAntennaLockOn(const std::vector<TargetInput>& worldTargets)
// {
//     for (const auto& t : worldTargets) {
//         if (t.id == config.lockedTargetID) {
//             double range = std::sqrt(t.x * t.x + t.y * t.y + t.z * t.z);
//             if (range < 1e-6)
//                 continue;

//             double targetAz = std::atan2(t.y, t.x) * (180.0 / M_PI);
//             if (targetAz < 0) targetAz += 360.0;

//             double ratio = std::clamp(t.z / range, -1.0, 1.0);
//             double targetEl = std::asin(ratio) * (180.0 / M_PI);

//             currentAzimuth = targetAz;
//             currentElevation = targetEl;
//             return true;
//         }
//     }
//     return false;
// }

// // ============================================================================
// // SUB-FUNCTION: Update antenna pointing for scanning modes
// // ============================================================================
// void RadarModel::updateAntennaScan(double dt)
// {
//     double rotationSpeed = (static_cast<double>(config.scanningRate_RPM) / 60.0) * 360.0;

//     bool fullRotation = (config.minAzimuth <= -180 && config.maxAzimuth >= 180);

//     if (fullRotation) {
//         currentAzimuth += rotationSpeed * dt;
//         while (currentAzimuth >= 360.0)
//             currentAzimuth -= 360.0;
//     } else {
//         currentAzimuth += scanDirection * rotationSpeed * dt;

//         if (currentAzimuth > config.maxAzimuth) {
//             currentAzimuth = config.maxAzimuth;
//             scanDirection = -1;
//         }
//         if (currentAzimuth < config.minAzimuth) {
//             currentAzimuth = config.minAzimuth;
//             scanDirection = 1;
//         }
//     }

//     double centerEl = (config.minElevation + config.maxElevation) / 2.0;
//     if (config.scanType == ScanType::CONICAL) {
//         currentElevation = centerEl +
//                            (std::sin(currentAzimuth * M_PI / 180.0) * (config.beamWidth / 4.0));
//     } else {
//         currentElevation = centerEl;
//     }
// }

// // ============================================================================
// // SUB-FUNCTION: Check if target is within beam
// // ============================================================================
// bool RadarModel::isTargetInBeam(
//     double targetAz,
//     double targetEl,
//     double dt,
//     double& outAzDiff,
//     double& outElDiff,
//     double& outScanMargin)
// {
//     outAzDiff = std::abs(currentAzimuth - targetAz);
//     if (outAzDiff > 180.0) outAzDiff = 360.0 - outAzDiff;

//     outElDiff = std::abs(currentElevation - targetEl);

//     double scanSpeed = (config.scanningRate_RPM / 60.0) * 360.0;
//     outScanMargin = (config.beamWidth * 2.5) + (scanSpeed * dt);

//     return (outAzDiff <= outScanMargin && outElDiff <= outScanMargin);
// }

// // ============================================================================
// // SUB-FUNCTION: Compute clutter effects on SNR
// // ============================================================================
// // Fix 1 — clutter power in watts, surface clutter follows R³ not R⁴
// double RadarModel::computeClutterPower(double range, SurfaceType surface) const
// {
//     if (surface == SurfaceType::AIR || range < 1.0)
//         return 0.0;

//     double sigma0 = 0.0;
//     if (surface == SurfaceType::SEA)
//         sigma0 = config.seaState * 3e-3;
//     else if (surface == SurfaceType::LAND)
//         sigma0 = config.landClutter * 1e-2;

//     if (sigma0 <= 0.0) return 0.0;

//     double tau      = (config.pulseWidth > 0) ? static_cast<double>(config.pulseWidth) : 1e-6;
//     double bwRad    = static_cast<double>(config.beamWidth) * M_PI / 180.0;
//     double patch    = (SPEED_OF_LIGHT * tau / 2.0) * (range * bwRad);

//     double lambda   = SPEED_OF_LIGHT / config.frequency_Hz;
//     double Pt       = config.emissionPower_kW * 1000.0;
//     double G_lin    = std::pow(10.0, config.antennaGain / 10.0);

//     double Pc = (Pt * G_lin * G_lin * lambda * lambda * sigma0 * patch)
//                 / (std::pow(4.0 * M_PI, 3) * std::pow(range, 3));

//     // Rayleigh fluctuation
//     static std::exponential_distribution<double> fluct(1.0);
//     return Pc * fluct(rng);
// }

// // Fix 1+3 — SINR = Pr / (Pn + Pc)
// double RadarModel::applyClutterEffects(double receivedPower, double range, SurfaceType surface)
// {
//     double Pn   = computeNoisePower();
//     double Pc   = computeClutterPower(range, surface);
//     return std::max(0.0, receivedPower / (Pn + Pc));
// }
// // double RadarModel::applyClutterEffects(double snr, double range, SurfaceType surface)
// // {
// //     static std::normal_distribution<double> clutterNoise(0.0, 1.0);

// //     if (surface == SurfaceType::SEA) {
// //         double clutter = config.seaState * 5e-19 *
// //                          std::exp(-range / 8000.0) *
// //                          std::abs(clutterNoise(rng));
// //         snr -= clutter;
// //     }

// //     if (surface == SurfaceType::LAND) {
// //         double clutter = config.landClutter * 3e-19 *
// //                          std::exp(-range / 5000.0) *
// //                          std::abs(clutterNoise(rng));
// //         snr -= clutter;
// //     }

// //     return std::max(snr, 0.0);
// // }

// // ============================================================================
// // SUB-FUNCTION: Generate CFAR reference cells
// // ============================================================================
// std::vector<double> RadarModel::generateReferenceCells(SurfaceType surface)
// {
//     std::vector<double> referenceCells;
//     referenceCells.reserve(16);

//     // SINR-domain cells: noise-only SINR = 1.0, exponential distribution
//     static std::exponential_distribution<double> cellDist(1.0);

//     for (int i = 0; i < 16; i++) {
//         double cell = cellDist(rng);
//         if (surface == SurfaceType::SEA)
//             cell *= (1.0 + config.seaState * 0.3);
//         else if (surface == SurfaceType::LAND)
//             cell *= (1.0 + config.landClutter * 0.5);
//         referenceCells.push_back(cell);
//     }

//     return referenceCells;
// }
// // std::vector<double> RadarModel::generateReferenceCells(SurfaceType surface)
// // {
// //     std::vector<double> referenceCells;
// //     referenceCells.reserve(16);

// //     static std::normal_distribution<double> noiseDist(0.0, 1e-18);

// //     for (int i = 0; i < 16; i++) {
// //         double noise = std::abs(noiseDist(rng));
// //         if (surface == SurfaceType::SEA)
// //             noise += config.seaState * 5e-19;
// //         referenceCells.push_back(noise);
// //     }

// //     return referenceCells;
// // }

// // ============================================================================
// // SUB-FUNCTION: Compute target motion parameters
// // ============================================================================
// void RadarModel::computeTargetMotionParams(
//     DetectionOutput& det,
//     const TargetInput& target,
//     double range)
// {
//     det.speedOverGround = std::sqrt(
//         target.vx * target.vx +
//         target.vy * target.vy +
//         target.vz * target.vz);

//     det.heading = std::atan2(target.vy, target.vx) * (180.0 / M_PI);
//     if (det.heading < 0)
//         det.heading += 360.0;

//     det.acceleration = 0.0;

//     double velMag = det.speedOverGround;
//     if (velMag > 0.01) {
//         double vx_n = target.vx / velMag;
//         double vy_n = target.vy / velMag;
//         double vz_n = target.vz / velMag;

//         double los_x = target.x / range;
//         double los_y = target.y / range;
//         double los_z = target.z / range;

//         double dot = vx_n * los_x + vy_n * los_y + vz_n * los_z;
//         dot = std::clamp(dot, -1.0, 1.0);
//         det.targetAspect = std::acos(dot) * 180.0 / M_PI;
//     } else {
//         det.targetAspect = 0.0;
//     }
// }

// // ============================================================================
// // SUB-FUNCTION: Compute radial velocity
// // ============================================================================
// double RadarModel::computeRadialVelocity(
//     const TargetInput& target,
//     double range,
//     std::normal_distribution<double>& dopplerNoise)
// {
//     double dot = target.vx * target.x +
//                  target.vy * target.y +
//                  target.vz * target.z;

//     double radialVel = (range > 1e-6) ? dot / range : 0.0;
//     radialVel += dopplerNoise(rng);

//     return radialVel;
// }

// // ============================================================================
// // SUB-FUNCTION: Compute CPA (Closest Point of Approach)
// // ============================================================================
// void RadarModel::computeCPA(
//     DetectionOutput& det,
//     const TargetInput& target,
//     double range)
// {
//     det.cpa_distance = range;
//     det.time_to_cpa = 0.0;

//     double v_sq = target.vx * target.vx +
//                   target.vy * target.vy +
//                   target.vz * target.vz;

//     if (v_sq > 0.01) {
//         double dot_pv = target.x * target.vx +
//                         target.y * target.vy +
//                         target.z * target.vz;

//         double t_cpa = -dot_pv / v_sq;
//         det.time_to_cpa = (t_cpa > 0.0) ? t_cpa : 0.0;

//         double cpa_x = target.x + target.vx * det.time_to_cpa;
//         double cpa_y = target.y + target.vy * det.time_to_cpa;
//         double cpa_z = target.z + target.vz * det.time_to_cpa;

//         det.cpa_distance = std::sqrt(cpa_x * cpa_x + cpa_y * cpa_y + cpa_z * cpa_z);
//     }
// }

// // ============================================================================
// // SUB-FUNCTION: Compute probability of kill
// // ============================================================================
// double RadarModel::computePk(double range, double radialVelocity)
// {
//     double rangeScale = std::exp(-range / 45000.0);
//     double aspectScale = (radialVelocity < 0.0) ? 1.2 : 0.8;
//     return std::min(0.99, 0.95 * rangeScale * aspectScale);
// }

// // ============================================================================
// // SUB-FUNCTION: Apply range ambiguity
// // ============================================================================
// void RadarModel::applyRangeAmbiguity(
//     DetectionOutput& det,
//     double range,
//     double maxUnambiguousRange,
//     std::normal_distribution<double>& rangeNoise)
// {
//     double measuredRange = range;

//     if (range > maxUnambiguousRange) {
//         measuredRange = std::fmod(range, maxUnambiguousRange);
//         det.isAmbiguous = true;
//     } else {
//         det.isAmbiguous = false;
//     }

//     measuredRange += rangeNoise(rng);
//     det.range = measuredRange;
// }

// // ============================================================================
// // SUB-FUNCTION: Resolve range for LOCK_ON mode
// // ============================================================================
// void RadarModel::resolveRangeForLockOn(
//     DetectionOutput& det,
//     double range,
//     double maxUnambiguousRange,
//     int targetId)
// {
//     double predictedRange = range;

//     for (const auto& track : trackDatabase) {
//         if (track.id == targetId) {
//             predictedRange = track.predictedRange;
//             break;
//         }
//     }

//     det.range = resolveRangeAmbiguity(det.range, predictedRange, maxUnambiguousRange);
//     det.isAmbiguous = false;
// }

// // ============================================================================
// // SUB-FUNCTION: Check if detection should be merged with existing
// // ============================================================================
// bool RadarModel::shouldMergeDetection(
//     const DetectionOutput& det,
//     const std::vector<DetectionOutput>& scanDetections)
// {
//     for (const auto& existing : scanDetections) {
//         double rangeDiff = std::abs(existing.range - det.range);

//         double azDiff2 = std::abs(existing.azimuth - det.azimuth);
//         if (azDiff2 > 180.0)
//             azDiff2 = 360.0 - azDiff2;

//         double elDiff2 = std::abs(existing.elevation - det.elevation);

//         if (rangeDiff < 150.0 &&
//             azDiff2 < config.beamWidth &&
//             elDiff2 < config.beamWidth) {
//             return true;
//         }
//     }
//     return false;
// }

// // ============================================================================
// // SUB-FUNCTION: Find best matching track for TWS association
// // ============================================================================
// TrackFile* RadarModel::findBestTrackMatch(
//     const DetectionOutput& det,
//     double maxUnambiguousRange,
//     double& outBestProb)
// {
//     TrackFile* bestTrack = nullptr;
//     outBestProb = 0.0;

//     for (auto& track : trackDatabase) {
//         if (!std::isfinite(track.x) || !std::isfinite(track.y) ||
//             !std::isfinite(track.z) || !std::isfinite(track.vx) ||
//             !std::isfinite(track.vy) || !std::isfinite(track.vz)) {
//             track.hitCount = 0;
//             track.isValidated = false;
//             continue;
//         }

//         if (track.isUpdated)
//             continue;

//         double candidateRange = resolveRangeAmbiguity(
//             det.range, track.predictedRange, maxUnambiguousRange);

//         double rangeError = std::abs(candidateRange - track.predictedRange);
//         if (rangeError > RANGE_GATE)
//             continue;

//         // Mahalanobis distance gating
//         double azRad = det.azimuth * M_PI / 180.0;
//         double elRad = det.elevation * M_PI / 180.0;

//         double zx = det.range * cos(elRad) * cos(azRad);
//         double zy = det.range * cos(elRad) * sin(azRad);
//         double zz = det.range * sin(elRad);

//         double dx = zx - track.X[0];
//         double dy = zy - track.X[1];
//         double dz = zz - track.X[2];

//         double S[3][3];
//         for (int i = 0; i < 3; i++)
//             for (int j = 0; j < 3; j++)
//                 S[i][j] = track.P[i][j] + track.R[i][j];

//         double detS = S[0][0] * (S[1][1] * S[2][2] - S[1][2] * S[2][1]) -
//                       S[0][1] * (S[1][0] * S[2][2] - S[1][2] * S[2][0]) +
//                       S[0][2] * (S[1][0] * S[2][1] - S[1][1] * S[2][0]);

//         if (std::abs(detS) < 1e-2)
//             continue;

//         double invS[3][3];
//         invS[0][0] = (S[1][1] * S[2][2] - S[1][2] * S[2][1]) / detS;
//         invS[0][1] = (S[0][2] * S[2][1] - S[0][1] * S[2][2]) / detS;
//         invS[0][2] = (S[0][1] * S[1][2] - S[0][2] * S[1][1]) / detS;
//         invS[1][0] = (S[1][2] * S[2][0] - S[1][0] * S[2][2]) / detS;
//         invS[1][1] = (S[0][0] * S[2][2] - S[0][2] * S[2][0]) / detS;
//         invS[1][2] = (S[0][2] * S[1][0] - S[0][0] * S[1][2]) / detS;
//         invS[2][0] = (S[1][0] * S[2][1] - S[1][1] * S[2][0]) / detS;
//         invS[2][1] = (S[0][1] * S[2][0] - S[0][0] * S[2][1]) / detS;
//         invS[2][2] = (S[0][0] * S[1][1] - S[0][1] * S[1][0]) / detS;

//         double d2 = dx * (invS[0][0] * dx + invS[0][1] * dy + invS[0][2] * dz) +
//                     dy * (invS[1][0] * dx + invS[1][1] * dy + invS[1][2] * dz) +
//                     dz * (invS[2][0] * dx + invS[2][1] * dy + invS[2][2] * dz);

//         if (d2 > 9.21)
//             continue;

//         double prob = computeAssociationProbability(
//             candidateRange, track.predictedRange, RANGE_GATE);

//         if (prob > outBestProb) {
//             outBestProb = prob;
//             bestTrack = &track;
//         }
//     }

//     return bestTrack;
// }

// // ============================================================================
// // SUB-FUNCTION: Perform Kalman update on track
// // ============================================================================
// void RadarModel::performKalmanUpdate(
//     TrackFile& track,
//     const DetectionOutput& det,
//     double currentTime,
//     double dt,
//     double maxUnambiguousRange)
// {
//     double bestRange = resolveRangeAmbiguity(
//         det.range, track.predictedRange, maxUnambiguousRange);

//     double azRad = det.azimuth * M_PI / 180.0;
//     double elRad = det.elevation * M_PI / 180.0;

//     // Measurement vector
//     double z[3];
//     z[0] = bestRange * cos(elRad) * cos(azRad);
//     z[1] = bestRange * cos(elRad) * sin(azRad);
//     z[2] = bestRange * sin(elRad);

//     // Innovation
//     double y[3];
//     y[0] = z[0] - track.X[0];
//     y[1] = z[1] - track.X[1];
//     y[2] = z[2] - track.X[2];

//     // Innovation covariance
//     double S[3][3] = {0};
//     for (int i = 0; i < 3; i++)
//         for (int j = 0; j < 3; j++)
//             S[i][j] = track.P[i][j] + track.R[i][j];

//     double detS = S[0][0] * (S[1][1] * S[2][2] - S[1][2] * S[2][1]) -
//                   S[0][1] * (S[1][0] * S[2][2] - S[1][2] * S[2][0]) +
//                   S[0][2] * (S[1][0] * S[2][1] - S[1][1] * S[2][0]);

//     if (std::abs(detS) < 1e-3) {
//         track.isUpdated = false;
//         track.hitCount = std::max(0, track.hitCount - 1);
//         return;
//     }

//     double invS[3][3];
//     invS[0][0] = (S[1][1] * S[2][2] - S[1][2] * S[2][1]) / detS;
//     invS[0][1] = (S[0][2] * S[2][1] - S[0][1] * S[2][2]) / detS;
//     invS[0][2] = (S[0][1] * S[1][2] - S[0][2] * S[1][1]) / detS;
//     invS[1][0] = (S[1][2] * S[2][0] - S[1][0] * S[2][2]) / detS;
//     invS[1][1] = (S[0][0] * S[2][2] - S[0][2] * S[2][0]) / detS;
//     invS[1][2] = (S[0][2] * S[1][0] - S[0][0] * S[1][2]) / detS;
//     invS[2][0] = (S[1][0] * S[2][1] - S[1][1] * S[2][0]) / detS;
//     invS[2][1] = (S[0][1] * S[2][0] - S[0][0] * S[2][1]) / detS;
//     invS[2][2] = (S[0][0] * S[1][1] - S[0][1] * S[1][0]) / detS;

//     // Kalman gain
//     double K[6][3] = {0};
//     for (int i = 0; i < 6; i++)
//         for (int j = 0; j < 3; j++)
//             for (int k = 0; k < 3; k++)
//                 K[i][j] += track.P[i][k] * invS[k][j];

//     // State update
//     for (int i = 0; i < 6; i++)
//         for (int j = 0; j < 3; j++)
//             track.X[i] += K[i][j] * y[j];

//     // Covariance update (Joseph form)
//     double I_KH[6][6] = {0};
//     for (int i = 0; i < 6; i++) {
//         for (int j = 0; j < 6; j++) {
//             I_KH[i][j] = (i == j) ? 1.0 : 0.0;
//             if (j < 3)
//                 I_KH[i][j] -= K[i][j];
//         }
//     }

//     double Pnew[6][6] = {0};
//     for (int i = 0; i < 6; i++)
//         for (int j = 0; j < 6; j++)
//             for (int k = 0; k < 6; k++)
//                 Pnew[i][j] += I_KH[i][k] * track.P[k][j];

//     for (int i = 0; i < 6; i++)
//         for (int j = 0; j < 6; j++)
//             track.P[i][j] = Pnew[i][j];

//     // Velocity sanity limit
//     double vmax = config.maxTrackSpeed;
//     track.X[3] = std::clamp(track.X[3], -vmax, vmax);
//     track.X[4] = std::clamp(track.X[4], -vmax, vmax);
//     track.X[5] = std::clamp(track.X[5], -vmax, vmax);

//     // Sync state
//     track.x = track.X[0];
//     track.y = track.X[1];
//     track.z = track.X[2];
//     track.vx = track.X[3];
//     track.vy = track.X[4];
//     track.vz = track.X[5];

//     track.range = std::sqrt(track.x * track.x + track.y * track.y + track.z * track.z);

//     if (track.range > 1e-6) {
//         track.velocity = (track.vx * track.x + track.vy * track.y + track.vz * track.z) / track.range;
//     } else {
//         track.velocity = 0;
//     }

//     track.lastSeenTime = currentTime;
//     track.hitCount++;
//     track.isUpdated = true;
//     track.missCount = 0;
//     track.wasAmbiguous = det.isAmbiguous;

//     if (track.hitCount >= 3)
//         track.isValidated = true;
// }

// // ============================================================================
// // SUB-FUNCTION: Create new track from detection
// // ============================================================================
// void RadarModel::createNewTrack(
//     const DetectionOutput& det,
//     const TargetInput& target,
//     double maxUnambiguousRange,
//     double currentTime)
// {
//     // Check if track already exists
//     for (const auto& t : trackDatabase) {
//         if (t.id == det.targetID)
//             return;
//     }

//     TrackFile newTrack;
//     newTrack.id = det.targetID;

//     double unfoldedRange = det.range;
//     if (det.isAmbiguous && det.radialVelocity < 0.0) {
//         unfoldedRange = det.range + maxUnambiguousRange;
//     }

//     double azRad = det.azimuth * M_PI / 180.0;
//     double elRad = det.elevation * M_PI / 180.0;

//     newTrack.x = unfoldedRange * cos(elRad) * cos(azRad);
//     newTrack.y = unfoldedRange * cos(elRad) * sin(azRad);
//     newTrack.z = unfoldedRange * sin(elRad);

//     newTrack.vx = target.vx;
//     newTrack.vy = target.vy;
//     newTrack.vz = target.vz;

//     newTrack.X = {newTrack.x, newTrack.y, newTrack.z,
//                   newTrack.vx, newTrack.vy, newTrack.vz};

//     // Initialize covariance
//     for (int i = 0; i < 6; i++)
//         for (int j = 0; j < 6; j++)
//             newTrack.P[i][j] = 0.0;

//     double posVar = config.noise.rangeStdDev * config.noise.rangeStdDev;
//     double velVar = 500.0 * 500.0;

//     newTrack.P[0][0] = posVar;
//     newTrack.P[1][1] = posVar;
//     newTrack.P[2][2] = posVar;
//     newTrack.P[3][3] = velVar;
//     newTrack.P[4][4] = velVar;
//     newTrack.P[5][5] = velVar;

//     newTrack.range = unfoldedRange;
//     newTrack.predictedRange = unfoldedRange;
//     newTrack.velocity = det.radialVelocity;
//     newTrack.lastSeenTime = currentTime;
//     newTrack.hitCount = 1;
//     newTrack.wasAmbiguous = det.isAmbiguous;

//     newTrack.Q[0][0] = 10; newTrack.Q[1][1] = 10; newTrack.Q[2][2] = 10;
//     newTrack.Q[3][3] = 1;  newTrack.Q[4][4] = 1;  newTrack.Q[5][5] = 1;

//     for (int i = 0; i < 3; i++)
//         newTrack.R[i][i] = 25.0;

//     if (trackDatabase.size() > 2000)
//         trackDatabase.erase(trackDatabase.begin());

//     trackDatabase.push_back(newTrack);
// }

// // ============================================================================
// // SUB-FUNCTION: Process single target detection
// // ============================================================================
// bool RadarModel::processTargetDetection(
//     const TargetInput& target,
//     double dt,
//     double currentTime,
//     double maxUnambiguousRange,
//     std::vector<DetectionOutput>& scanDetections,
//     std::normal_distribution<double>& rangeNoise,
//     std::normal_distribution<double>& azNoise,
//     std::normal_distribution<double>& elNoise,
//     std::normal_distribution<double>& dopplerNoise)
// {
//     double range = std::sqrt(target.x * target.x + target.y * target.y + target.z * target.z);

//     std::cout << "Range: " << range << std::endl;
//     if (range < 30) return false;

//     if (!checkHorizon(range, target.z))
//         return false;

//     double targetAz = std::atan2(target.y, target.x) * (180.0 / M_PI);
//     if (targetAz < 0) targetAz += 360.0;

//     if (range < 1e-6)
//         return false;

//     double ratio = std::clamp(target.z / range, -1.0, 1.0);
//     double targetEl = std::asin(ratio) * (180.0 / M_PI);

//     double azDiff, elDiff, scanMargin;
//     if (!isTargetInBeam(targetAz, targetEl, dt, azDiff, elDiff, scanMargin)) {
//         std::cout << "Beam check | azDiff: " << azDiff
//                   << " elDiff: " << elDiff
//                   << " margin: " << scanMargin << std::endl;
//         return false;
//     }

//     std::cout << "Beam check | azDiff: " << azDiff
//               << " elDiff: " << elDiff
//               << " margin: " << scanMargin << std::endl;
//     double effectiveRCS  = computeEffectiveRCS(target, range);
//     double receivedPower = calculateSignalStrength(range, effectiveRCS);  // Pr in watts
//     double sinr          = applyClutterEffects(receivedPower, range, target.surface); // SINR
//     std::cout << "SINR: " << sinr << std::endl;
//     // double effectiveRCS = computeEffectiveRCS(target, range);
//     // double snr = calculateSignalStrength(range, effectiveRCS);
//     // //double snr = calculateSignalStrength(range, target.rcs);
//     // std::cout << "SNR: " << snr << std::endl;

//     // snr = applyClutterEffects(snr, range, target.surface);

//     auto referenceCells = generateReferenceCells(target.surface);
//     double threshold = computeCFARThreshold(referenceCells);
//     std::cout << "CFAR threshold: " << threshold << std::endl;

//     if (sinr <= threshold)
//         return false;

//     // Build detection
//     DetectionOutput det;
//     det.targetID = target.id;
//     det.azimuth = targetAz + azNoise(rng);
//     det.elevation = targetEl + elNoise(rng);
//    det.snr = sinr;
//     computeTargetMotionParams(det, target, range);
//     det.radialVelocity = computeRadialVelocity(target, range, dopplerNoise);
//     computeCPA(det, target, range);
//     det.Pk = computePk(range, det.radialVelocity);

//     applyRangeAmbiguity(det, range, maxUnambiguousRange, rangeNoise);

//     if (config.mode == RadarMode::LOCK_ON) {
//         resolveRangeForLockOn(det, range, maxUnambiguousRange, target.id);
//     } else {
//         if (!det.isAmbiguous && det.range < config.minDetectableRange)
//             det.range = config.minDetectableRange;
//     }

//     if (shouldMergeDetection(det, scanDetections))
//         return false;

//     scanDetections.push_back(det);
//     std::cout << "Raw detection stored: " << det.targetID << std::endl;

//     // TWS track association
//     if (config.mode == RadarMode::TWS) {
//         double bestProb;
//         TrackFile* bestTrack = findBestTrackMatch(det, maxUnambiguousRange, bestProb);

//         if (bestTrack != nullptr) {
//             performKalmanUpdate(*bestTrack, det, currentTime, dt, maxUnambiguousRange);
//         } else {
//             createNewTrack(det, target, maxUnambiguousRange, currentTime);
//         }
//     }

//     lastDetections.push_back(det);
//     return true;
// }

// // ============================================================================
// // SUB-FUNCTION: Build TWS output from track
// // ============================================================================
// DetectionOutput RadarModel::buildTWSOutput(const TrackFile& track)
// {
//     DetectionOutput out;

//     double reportRange = track.isUpdated ? track.range : track.predictedRange;

//     out.targetID = track.id;
//     out.range = reportRange;
//     out.radialVelocity = track.velocity;
//     out.isAmbiguous = false;

//     // CPA computation
//     double dot_pv = track.x * track.vx + track.y * track.vy + track.z * track.vz;
//     double v_sq = track.vx * track.vx + track.vy * track.vy + track.vz * track.vz;

//     if (v_sq > 0.01) {
//         double t_cpa = -dot_pv / v_sq;
//         if (t_cpa > 0.0) {
//             out.time_to_cpa = t_cpa;
//             double cx = track.x + track.vx * t_cpa;
//             double cy = track.y + track.vy * t_cpa;
//             double cz = track.z + track.vz * t_cpa;
//             out.cpa_distance = std::sqrt(cx * cx + cy * cy + cz * cz);
//         } else {
//             out.time_to_cpa = 0.0;
//             out.cpa_distance = reportRange;
//         }
//     } else {
//         out.time_to_cpa = 0.0;
//         out.cpa_distance = reportRange;
//     }

//     // Pk
//     double rangeScale = std::exp(-reportRange / 45000.0);
//     double aspectScale = (track.velocity < 0.0) ? 1.2 : 0.8;
//     out.Pk = std::min(0.99, 0.95 * rangeScale * aspectScale);

//     // Motion parameters
//     out.speedOverGround = std::sqrt(
//         track.vx * track.vx + track.vy * track.vy + track.vz * track.vz);

//     out.heading = std::atan2(track.vy, track.vx) * (180.0 / M_PI);
//     if (out.heading < 0) out.heading += 360.0;

//     out.acceleration = 0.0;

//     double losAngle = std::atan2(track.y, track.x) * (180.0 / M_PI);
//     if (losAngle < 0) losAngle += 360.0;

//     out.targetAspect = std::abs(out.heading - losAngle);
//     if (out.targetAspect > 180.0)
//         out.targetAspect = 360.0 - out.targetAspect;

//     out.azimuth = std::atan2(track.y, track.x) * (180.0 / M_PI);
//     if (out.azimuth < 0) out.azimuth += 360.0;

//     if (reportRange > 1e-6) {
//         double ratio = std::clamp(track.z / reportRange, -1.0, 1.0);
//         out.elevation = std::asin(ratio) * (180.0 / M_PI);
//     } else {
//         out.elevation = 0;
//     }

//     return out;
// }

// // ============================================================================
// // SUB-FUNCTION: Generate TWS report
// // ============================================================================
// void RadarModel::generateTWSReport()
// {
//     lastDetections.clear();

//     for (auto& track : trackDatabase) {
//         if (!track.isValidated)
//             continue;

//         DetectionOutput out = buildTWSOutput(track);
//         lastDetections.push_back(out);
//         track.isUpdated = false;
//     }
// }

// // ============================================================================
// // SUB-FUNCTION: Compute aspect-dependent RCS
// // ============================================================================
// double RadarModel::computeEffectiveRCS(const TargetInput& target, double range)
// {
//     double velMag = std::sqrt(
//         target.vx*target.vx +
//         target.vy*target.vy +
//         target.vz*target.vz);

//     double aspectDeg = 0.0;

//     if (velMag > 0.01)
//     {
//         double vx_n = target.vx / velMag;
//         double vy_n = target.vy / velMag;
//         double vz_n = target.vz / velMag;

//         double los_x = target.x / range;
//         double los_y = target.y / range;
//         double los_z = target.z / range;

//         double dot = vx_n*los_x + vy_n*los_y + vz_n*los_z;
//         dot = std::clamp(dot, -1.0, 1.0);

//         aspectDeg = std::acos(dot) * 180.0 / M_PI;
//     }

//     double aspectRad = aspectDeg * M_PI / 180.0;

//     // Aircraft-like RCS variation
//     double aspectFactor =
//         0.2 + 0.8 * std::abs(std::sin(aspectRad));

//     return target.rcs * aspectFactor;
// }
// // ============================================================================
// // MAIN UPDATE FUNCTION (Refactored)
// // ============================================================================
// void RadarModel::updateTWS(double dt, double currentTime)
// {
//     for (auto& track : trackDatabase) {
//         // Kalman prediction
//         double F[6][6] = {
//             {1, 0, 0, dt, 0,  0 },
//             {0, 1, 0, 0,  dt, 0 },
//             {0, 0, 1, 0,  0,  dt},
//             {0, 0, 0, 1,  0,  0 },
//             {0, 0, 0, 0,  1,  0 },
//             {0, 0, 0, 0,  0,  1 }
//         };

//         std::array<double, 6> Xnew;
//         for (int i = 0; i < 6; i++) {
//             Xnew[i] = 0;
//             for (int j = 0; j < 6; j++)
//                 Xnew[i] += F[i][j] * track.X[j];
//         }
//         track.X = Xnew;

//         // Covariance prediction
//         double Pnew[6][6] = {0};
//         for (int i = 0; i < 6; i++)
//             for (int j = 0; j < 6; j++)
//                 for (int k = 0; k < 6; k++)
//                     Pnew[i][j] += F[i][k] * track.P[k][j];

//         for (int i = 0; i < 6; i++)
//             for (int j = 0; j < 6; j++) {
//                 double temp = 0;
//                 for (int k = 0; k < 6; k++)
//                     temp += Pnew[i][k] * F[j][k];
//                 track.P[i][j] = temp + track.Q[i][j];
//             }

//         // Sync state
//         track.x = track.X[0];
//         track.y = track.X[1];
//         track.z = track.X[2];
//         track.vx = track.X[3];
//         track.vy = track.X[4];
//         track.vz = track.X[5];

//         track.predictedRange = std::sqrt(
//             track.x * track.x + track.y * track.y + track.z * track.z);
//         track.range = track.predictedRange;

//         if (!track.isUpdated)
//             track.missCount++;

//         track.isUpdated = false;

//         std::cout << "Track " << track.id << " missCount: " << track.missCount << std::endl;
//     }

//     trackDatabase.erase(
//         std::remove_if(trackDatabase.begin(), trackDatabase.end(),
//                        [](const TrackFile& t) { return t.missCount > 60; }),
//         trackDatabase.end());
// }

// void RadarModel::update(double dt, const std::vector<TargetInput>& worldTargets, double currentTime)
// {
//     std::cout << "Radar Mode: " << (int)config.mode << std::endl;
//     std::lock_guard<std::mutex> lock(radarMutex);

//     lastDetections.clear();

//     // Setup noise distributions
//     std::normal_distribution<double> rangeNoise(0.0, config.noise.rangeStdDev);
//     std::normal_distribution<double> azNoise(0.0, config.noise.azimuthStdDev);
//     std::normal_distribution<double> elNoise(0.0, config.noise.elevationStdDev);
//     std::normal_distribution<double> dopplerNoise(0.0, config.noise.dopplerStdDev);

//     // Compute max unambiguous range
//     double currentPRF = config.prfLevels[0];
//     if (currentPRF <= 0) currentPRF = 5000.0;
//     double maxUnambiguousRange = 299792458.0 / (2.0 * currentPRF);

//     // TWS prediction step
//     if (config.mode == RadarMode::TWS) {
//         updateTWS(dt, currentTime);
//     }

//     // Antenna control
//     if (config.mode == RadarMode::LOCK_ON) {
//         if (!updateAntennaLockOn(worldTargets)) {
//             config.mode = RadarMode::SURVEILLANCE;
//         }
//     } else {
//         updateAntennaScan(dt);
//     }

//     // Process targets
//     bool lockedTargetVisible = false;
//     std::vector<DetectionOutput> scanDetections;
//     scanDetections.reserve(worldTargets.size());

//     for (const auto& target : worldTargets) {
//         std::cout << "Target received: " << target.id
//                   << " pos(" << target.x << "," << target.y << "," << target.z << ")"
//                   << std::endl;

//         // STT filter
//         if (config.mode == RadarMode::LOCK_ON && target.id != config.lockedTargetID)
//             continue;

//         bool detected = processTargetDetection(
//             target, dt, currentTime, maxUnambiguousRange,
//             scanDetections, rangeNoise, azNoise, elNoise, dopplerNoise);

//         if (detected && config.mode == RadarMode::LOCK_ON &&
//             target.id == config.lockedTargetID) {
//             lockedTargetVisible = true;
//         }
//     }

//     // TWS reporting phase
//     if (config.mode == RadarMode::TWS) {
//         generateTWSReport();
//     }

//     std::cout << "Track DB size: " << trackDatabase.size() << std::endl;

//     int validated = 0;
//     for (const auto& t : trackDatabase) {
//         if (t.isValidated) validated++;
//     }
//     std::cout << "Validated tracks: " << validated << std::endl;

//     // Break-lock logic
//     if (config.mode == RadarMode::LOCK_ON && !lockedTargetVisible) {
//         config.mode = RadarMode::SURVEILLANCE;
//         config.lockedTargetID = 0;
//         std::cout << "\n[!] BREAK LOCK: Target lost at " << currentAzimuth << " deg\n";
//     }
// }

// double RadarModel::resolveRangeAmbiguity(double measuredRange, double predictedRange, double Rmax)
// {
//     double bestRange = measuredRange;
//     double minError = 1e12;

//     if (Rmax < 1) return measuredRange;

//     for (int k = -5; k <= 5; k++) {
//         double candidate = measuredRange + k * Rmax;
//         double error = std::abs(candidate - predictedRange);
//         if (error < minError) {
//             minError = error;
//             bestRange = candidate;
//         }
//     }

//     return bestRange;
// }

// bool RadarModel::checkHorizon(double range, double targetZ)
// {
//     double Re = 6371000.0;
//     double effectiveRadius = Re * config.earthRadiusFactor;

//     double radarHeight = config.radarHeight;
//     double targetHeight = std::max(0.0, targetZ);

//     double d_radar = std::sqrt(2.0 * effectiveRadius * radarHeight);
//     double d_target = std::sqrt(2.0 * effectiveRadius * targetHeight);

//     double maxVisibleRange = d_radar + d_target;

//     return range <= maxVisibleRange;
// }

// bool RadarModel::checkBeamIntersection(double targetAz, double targetEl)
// {
//     double azDiff = std::abs(currentAzimuth - targetAz);
//     if (azDiff > 180.0) azDiff = 360.0 - azDiff;

//     double elDiff = std::abs(currentElevation - targetEl);
//     double angularDist = std::sqrt(azDiff * azDiff + elDiff * elDiff);

//     return (angularDist <= config.beamWidth);
// }
// double RadarModel::calculateSignalStrength(double range, double rcs)
// {
//     if (range < 1.0) range = 1.0;

//     double freq = config.frequency_Hz;
//     if (config.frequencyAgility)
//         freq += (config.hopStepFrequency * config.hopRate);

//     double lambda = SPEED_OF_LIGHT / freq;
//     double Pt     = config.emissionPower_kW * 1000.0;
//     double G_lin  = std::pow(10.0, config.antennaGain / 10.0);

//     double numerator   = Pt * G_lin * G_lin * lambda * lambda * rcs;
//     double denominator = std::pow(4.0 * M_PI, 3) * std::pow(range, 4);

//     // Returns pure received power Pr in watts — NO noise floor added here
//     return std::max(0.0, numerator / denominator);
// }

// // Fix 3 — kTBF noise power
// double RadarModel::computeNoisePower() const
// {
//     double B     = std::max(1.0, config.antennaBandwidth);
//     double F_lin = std::pow(10.0, config.noiseFigure_dB / 10.0);
//     return BOLTZMANN * config.systemTemperature_K * B * F_lin;
// }
// // double RadarModel::calculateSignalStrength(double range, double rcs)
// // {
// //     if (range < 1.0) range = 1.0;

// //     double freq = config.frequency_Hz;
// //     if (config.frequencyAgility) {
// //         freq += (config.hopStepFrequency * config.hopRate);
// //     }

// //     double lambda = 299792458.0 / freq;
// //     double Pt = config.emissionPower_kW * 1000.0;
// //     double G_linear = std::pow(10.0, config.antennaGain / 10.0);

// //     double numerator = Pt * std::pow(G_linear, 2) * std::pow(lambda, 2) * rcs;
// //     double denominator = std::pow(4.0 * M_PI, 3) * std::pow(range, 4);

// //     double snr = numerator / denominator;
// //     double noiseFloor = 1e-18;

// //     return std::max(0.0, snr + noiseFloor);
// // }

// std::vector<DetectionOutput> RadarModel::getActiveDetections()
// {
//     std::lock_guard<std::mutex> lock(radarMutex);
//     return lastDetections;
// }

// std::vector<TrackFile> RadarModel::getActiveTracks()
// {
//     std::lock_guard<std::mutex> lock(radarMutex);

//     std::vector<TrackFile> active;
//     for (const auto& track : trackDatabase) {
//         if (track.isValidated)
//             active.push_back(track);
//     }

//     return active;
// }

// void RadarModel::reset()
// {
//     std::lock_guard<std::mutex> lock(radarMutex);
//     currentAzimuth = 0.0;
//     currentElevation = 0.0;
//     trackDatabase.clear();
//     lastDetections.clear();
// }

// extern "C" {
// RadarModel* createRadar() { return new RadarModel(); }
// void destroyRadar(RadarModel* obj) { delete obj; }
// }

// #include <cmath>
// #include <algorithm>
// #include <vector>
// #include <random>
// #include <iostream>
// #include <mutex>
// #include <unordered_map>
// #include "radarmodel.h"
// #ifndef M_PI
// #define M_PI 3.14159265358979323846
// #endif

// constexpr double RANGE_GATE = 2000;   // 2 km gating window//Detection must be within ±2 km of predicted track
// //static std::default_random_engine rng(std::random_device{}());
// thread_local std::default_random_engine rng(std::random_device{}());
// //RadarModel::RadarModel() : currentAzimuth(0.0), currentElevation(0.0) {}
// RadarModel::RadarModel()
//     : currentAzimuth(0.0),
//     currentElevation(0.0)
// {
//     trackDatabase.reserve(2048);
// }

// void RadarModel::setConfiguration(const RadarAttributes& attrs) {
//     config = attrs;
// }
// double RadarModel::computeCFARThreshold(
//     const std::vector<double>& referenceCells)
// {
//     if(referenceCells.empty())
//         return 1e12;

//     double sum = 0;

//     for(double v : referenceCells)
//         sum += v;

//     double noiseMean = sum / referenceCells.size();

//     double scale = 8.0; // CFAR constant

//     return noiseMean * scale;
// }

// double RadarModel::computeAssociationProbability(
//     double measurementRange,
//     double predictedRange,
//     double gateSize)
// {
//     double error = measurementRange - predictedRange;

//     double sigma = gateSize / 2.0;

//     double exponent =
//         -(error * error) / (2 * sigma * sigma);

//     return std::exp(exponent);
// }

// void RadarModel::updateTWS(double dt, double currentTime)
// {
//     for (auto& track : trackDatabase)
//     {
//         // -------- KALMAN PREDICTION --------

//         double F[6][6] =
//             {
//                 {1,0,0,dt,0,0},
//                 {0,1,0,0,dt,0},
//                 {0,0,1,0,0,dt},
//                 {0,0,0,1,0,0},
//                 {0,0,0,0,1,0},
//                 {0,0,0,0,0,1}
//             };

//         std::array<double,6> Xnew;

//         for(int i=0;i<6;i++)
//         {
//             Xnew[i] = 0;

//             for(int j=0;j<6;j++)
//                 Xnew[i] += F[i][j] * track.X[j];
//         }

//         track.X = Xnew;


//         // -------- COVARIANCE PREDICTION --------

//         double Pnew[6][6] = {0};

//         for(int i=0;i<6;i++)
//             for(int j=0;j<6;j++)
//             {
//                 for(int k=0;k<6;k++)
//                     Pnew[i][j] += F[i][k] * track.P[k][j];
//             }

//         for(int i=0;i<6;i++)
//             for(int j=0;j<6;j++)
//             {
//                 double temp = 0;

//                 for(int k=0;k<6;k++)
//                     temp += Pnew[i][k] * F[j][k];

//                 track.P[i][j] = temp + track.Q[i][j];
//             }

//         // -------- Sync state --------

//         track.x = track.X[0];
//         track.y = track.X[1];
//         track.z = track.X[2];

//         track.vx = track.X[3];
//         track.vy = track.X[4];
//         track.vz = track.X[5];

//         track.predictedRange =
//             std::sqrt(track.x*track.x +
//                       track.y*track.y +
//                       track.z*track.z);

//         track.range = track.predictedRange;
//         if(!track.isUpdated)
//         {
//             track.missCount++;
//         }
//         track.isUpdated = false;
//         std::cout
//             << "Track "
//             << track.id
//             << " missCount: "
//             << track.missCount
//             << std::endl;
//     }

//     trackDatabase.erase(
//         std::remove_if(trackDatabase.begin(), trackDatabase.end(),
//                        [&](const TrackFile& t)
//                        {
//                            return t.missCount > 60;
//                        }),
//         trackDatabase.end());
//     // trackDatabase.erase(
//     //     std::remove_if(trackDatabase.begin(), trackDatabase.end(),
//     //                    [&](const TrackFile& t)
//     //                    {
//     //                        if(!t.isValidated)
//     //                        {
//     //                            if(t.isUpdated)
//     //                                return false;

//     //                            return (currentTime - t.lastSeenTime) > 8.0;
//     //                        }

//     //                        return (currentTime - t.lastSeenTime) > 6.0;
//     //                    }),
//     //     trackDatabase.end());
// }

// void RadarModel::update(double dt, const std::vector<TargetInput>& worldTargets, double currentTime) {
//     std::cout << "Radar Mode: " << (int)config.mode << std::endl;
//     std::lock_guard<std::mutex> lock(radarMutex);

//     lastDetections.clear();
//     std::normal_distribution<double> rangeNoise(
//         0.0, config.noise.rangeStdDev);

//     std::normal_distribution<double> azNoise(
//         0.0, config.noise.azimuthStdDev);

//     std::normal_distribution<double> elNoise(
//         0.0, config.noise.elevationStdDev);

//     std::normal_distribution<double> dopplerNoise(
//         0.0, config.noise.dopplerStdDev);
//     // --- 1. SYNCED PRF LOGIC ---
//     double currentPRF = config.prfLevels[0];
//     if (currentPRF <= 0) currentPRF = 5000.0;
//     double maxUnambiguousRange = 299792458.0 / (2.0 * currentPRF);

//     // --- NEW: TWS PREDICTION STEP ---
//     if (config.mode == RadarMode::TWS) {
//         updateTWS(dt, currentTime);
//     }

//     // --- 2. ANTENNA CONTROL ---
//     if (config.mode == RadarMode::LOCK_ON) {
//         bool targetFoundInWorld = false;
//         for (const auto& t : worldTargets) {

//             if (t.id == config.lockedTargetID) {
//                 double range = std::sqrt(t.x*t.x + t.y*t.y + t.z*t.z);
//                 double targetAz = std::atan2(t.y, t.x) * (180.0 / M_PI);
//                 if (targetAz < 0) targetAz += 360.0;
//                 if(range < 1e-6)
//                     continue;

//                 double ratio = std::clamp(t.z / range, -1.0, 1.0);
//                 double targetEl = std::asin(ratio) * (180.0/M_PI);
//                 //double targetEl = std::asin(t.z / range) * (180.0 / M_PI);

//                 currentAzimuth = targetAz;
//                 currentElevation = targetEl;
//                 targetFoundInWorld = true;
//                 break;
//             }
//         }
//         if (!targetFoundInWorld) config.mode = RadarMode::SURVEILLANCE;
//     } else {
//         /*
// Radar antenna scan controller.

// This code updates the radar beam azimuth based on the configured scan limits.

// Two scan modes are supported:

// 1) Full Rotation Mode (360° radar)
//    If minAzimuth <= -180 and maxAzimuth >= 180, the radar performs a continuous
//    360° rotation. This is typical for surveillance radars (naval, ground, AWACS).
//    The antenna rotates continuously and wraps back to 0° after reaching 360°.

// 2) Sector Sweep Mode (fighter radar)
//    If the azimuth limits define a sector (e.g. -60° to +60°), the radar performs
//    a back-and-forth sweep inside that sector. The beam moves until it reaches
//    the sector boundary and then reverses direction.

// Why this does NOT break other functionality:
// This code only controls the antenna pointing direction. All radar physics and
// processing (beam intersection, signal strength, CFAR detection, track creation,
// and Kalman tracking) happen after the beam position is computed. Therefore the
// rest of the radar pipeline continues to work exactly the same regardless of
// whether the antenna performs a full rotation or a sector sweep.
// */
//         double rotationSpeed = (static_cast<double>(config.scanningRate_RPM) / 60.0) * 360.0;
//         // currentAzimuth += rotationSpeed * dt;
//         // while (currentAzimuth >= 360.0) currentAzimuth -= 360.0;

//         bool fullRotation =
//             (config.minAzimuth <= -180 && config.maxAzimuth >= 180);

//         if(fullRotation)
//         {
//             // 360° radar
//             currentAzimuth += rotationSpeed * dt;

//             while (currentAzimuth >= 360.0)
//                 currentAzimuth -= 360.0;
//         }
//         else
//         {
//             // sector sweep radar
//             currentAzimuth += scanDirection * rotationSpeed * dt;

//             if(currentAzimuth > config.maxAzimuth)
//             {
//                 currentAzimuth = config.maxAzimuth;
//                 scanDirection = -1;
//             }

//             if(currentAzimuth < config.minAzimuth)
//             {
//                 currentAzimuth = config.minAzimuth;
//                 scanDirection = 1;
//             }
//         }
//         double centerEl = (config.minElevation + config.maxElevation) / 2.0;
//         if (config.scanType == ScanType::CONICAL)
//         {
//             currentElevation =
//                 centerEl +
//                 (std::sin(currentAzimuth * M_PI / 180.0) *
//                  (config.beamWidth / 4.0));
//         }
//         else
//         {
//             currentElevation = centerEl;
//         }
//     }

//     // --- 3. PROCESS ALL WORLD TARGETS ---
//     bool lockedTargetVisible = false;
//     std::vector<DetectionOutput> scanDetections;
//     scanDetections.reserve(worldTargets.size());
//     for (const auto& target : worldTargets) {

//         std::cout << "Target received: "
//                   << target.id
//                   << " pos(" << target.x << ","
//                   << target.y << ","
//                   << target.z << ")"
//                   << std::endl;

//         // 🔴 STT: Ignore other targets
//         if (config.mode == RadarMode::LOCK_ON &&
//             target.id != config.lockedTargetID)
//         {
//             continue;
//         }
//         double range = std::sqrt(target.x*target.x + target.y*target.y + target.z*target.z);
//         //Real radars cannot detect very close targets because of transmit blanking.
//         std::cout << "Range: " << range << std::endl;
//         if (range < 30) continue;

//         // Horizon check
//         if (!checkHorizon(range, target.z))
//             continue;

//         double targetAz = std::atan2(target.y, target.x) * (180.0 / M_PI);
//         if (targetAz < 0) targetAz += 360.0;

//         if(range < 1e-6)
//             continue;

//         // double ratio = std::clamp(target.z / range, -1.0, 1.0);

//         // double targetEl = std::asin(target.z / range) * (180.0 / M_PI);
//         double ratio = std::clamp(target.z / range, -1.0, 1.0);
//         double targetEl = std::asin(ratio) * (180.0 / M_PI);
//         // double azDiff = std::abs(currentAzimuth - targetAz);
//         // if (azDiff > 180.0) azDiff = 360.0 - azDiff;
//         // double elDiff = std::abs(currentElevation - targetEl);

//         // BEAM INTERSECTION
//         // BEAM INTERSECTION
//         /*if (azDiff <= (config.beamWidth / 2.0) &&
//             elDiff <= (config.beamWidth / 2.0))*/
//         /* double scanMargin = config.beamWidth * 3;

//         if (azDiff <= scanMargin &&
//             elDiff <= scanMargin)*/ double azDiff = std::abs(currentAzimuth - targetAz);
//         if (azDiff > 180.0) azDiff = 360.0 - azDiff;

//         double elDiff = std::abs(currentElevation - targetEl);

//         // Compute scan angular velocity
//         double scanSpeed =
//             (config.scanningRate_RPM / 60.0) * 360.0;

//         // Effective detection margin
//         // double scanMargin =
//         //     (config.beamWidth / 2.0) +
//         //     (scanSpeed * dt);
//         //i added *2.5 so that scan margin increases..
//         double scanMargin =
//             (config.beamWidth*2.5) +
//             (scanSpeed * dt);
//         std::cout << "Beam check | azDiff: "
//                   << azDiff
//                   << " elDiff: "
//                   << elDiff
//                   << " margin: "
//                   << scanMargin
//                   << std::endl;
//         if (azDiff <= scanMargin &&
//             elDiff <= scanMargin)
//         {

//             double snr = calculateSignalStrength(range, target.rcs);
//             std::cout << "SNR: " << snr << std::endl;

//             // Random clutter noise generator
//             static std::normal_distribution<double> clutterNoise(0.0,1.0);

//             // SEA CLUTTER
//             if(target.surface == SurfaceType::SEA)
//             {
//                 double clutter =
//                     config.seaState * 5e-19 *
//                     std::exp(-range / 8000.0) *
//                     std::abs(clutterNoise(rng));

//                 snr -= clutter;
//             }

//             // LAND CLUTTER
//             if(target.surface == SurfaceType::LAND)
//             {
//                 double clutter =
//                     config.landClutter * 3e-19 *
//                     std::exp(-range / 5000.0) *
//                     std::abs(clutterNoise(rng));

//                 snr -= clutter;
//             }

//             snr = std::max(snr, 0.0);
//             // Generate reference noise cells
//             std::vector<double> referenceCells;

//             referenceCells.reserve(16);
//             static std::normal_distribution<double> noiseDist(0.0,1e-18);

//             for(int i=0;i<16;i++)
//             {

//                 double noise = std::abs(noiseDist(rng));
//                 if(target.surface == SurfaceType::SEA)
//                     noise += config.seaState * 5e-19;
//                 referenceCells.push_back(noise);
//             }

//             double threshold = computeCFARThreshold(referenceCells);
//             std::cout << "CFAR threshold: " << threshold << std::endl;
//             if (snr > threshold) {

//                 DetectionOutput det;



//                 det.targetID  = target.id;
//                 det.azimuth   = targetAz + azNoise(rng);
//                 det.elevation = targetEl + elNoise(rng);
//                 det.snr       = snr;
//                 // -------------------------------------------------------
//                 // TARGET MOTION PARAMETERS
//                 // -------------------------------------------------------

//                 // Speed over ground
//                 det.speedOverGround =
//                     std::sqrt(target.vx*target.vx +
//                               target.vy*target.vy +
//                               target.vz*target.vz);

//                 // Heading (horizontal plane)
//                 det.heading =
//                     std::atan2(target.vy, target.vx) * (180.0 / M_PI);

//                 if (det.heading < 0)
//                     det.heading += 360.0;

//                 // Acceleration (simple model: assume constant velocity)
//                 det.acceleration = 0.0;

//                 // Target aspect (relative direction from radar)
//                 double losAngle =
//                     std::atan2(target.y, target.x) * (180.0 / M_PI);

//                 if (losAngle < 0)
//                     losAngle += 360.0;

//                 // det.targetAspect = std::abs(det.heading - losAngle);

//                 // if (det.targetAspect > 180.0)
//                 //     det.targetAspect = 360.0 - det.targetAspect;
//                 double velMag = det.speedOverGround;

//                 if (velMag > 0.01)
//                 {
//                     double vx_n = target.vx / velMag;
//                     double vy_n = target.vy / velMag;
//                     double vz_n = target.vz / velMag;

//                     double los_x = target.x / range;
//                     double los_y = target.y / range;
//                     double los_z = target.z / range;

//                     double dot = vx_n*los_x + vy_n*los_y + vz_n*los_z;
//                     dot = std::clamp(dot, -1.0, 1.0);
//                     det.targetAspect = std::acos(dot) * 180.0 / M_PI;
//                 }

//                 // -------------------------------------------------------
//                 // RADIAL VELOCITY (projection onto LOS)
//                 // -------------------------------------------------------
//                 // det.radialVelocity =
//                 //     (target.vx * target.x +
//                 //      target.vy * target.y +
//                 //      target.vz * target.z) / range;
//                 double dot =
//                     target.vx * target.x +
//                     target.vy * target.y +
//                     target.vz * target.z;

//                 det.radialVelocity = (range > 1e-6) ? dot / range : 0.0;

//                 det.radialVelocity += dopplerNoise(rng);

//                 // -------------------------------------------------------
//                 // CPA INITIALIZATION (FIX FOR GARBAGE VALUES)
//                 // -------------------------------------------------------
//                 det.cpa_distance = range;   // Default: current range
//                 det.time_to_cpa  = 0.0;

//                 double v_sq =
//                     (target.vx * target.vx) +
//                     (target.vy * target.vy) +
//                     (target.vz * target.vz);

//                 if (v_sq > 0.01) {

//                     double dot_pv =
//                         (target.x * target.vx +
//                          target.y * target.vy +
//                          target.z * target.vz);

//                     double t_cpa = -dot_pv / v_sq;

//                     // Only future CPA matters
//                     det.time_to_cpa = (t_cpa > 0.0) ? t_cpa : 0.0;

//                     double cpa_x = target.x + target.vx * det.time_to_cpa;
//                     double cpa_y = target.y + target.vy * det.time_to_cpa;
//                     double cpa_z = target.z + target.vz * det.time_to_cpa;

//                     det.cpa_distance =
//                         std::sqrt(cpa_x * cpa_x +
//                                   cpa_y * cpa_y +
//                                   cpa_z * cpa_z);
//                 }

//                 // -------------------------------------------------------
//                 // PROBABILITY OF KILL (simple engagement model)
//                 // -------------------------------------------------------
//                 double rangeScale  = std::exp(-range / 45000.0);
//                 double aspectScale = (det.radialVelocity < 0.0) ? 1.2 : 0.8;

//                 det.Pk = std::min(0.99,
//                                   0.95 * rangeScale * aspectScale);



//                 double measuredRange = range;

//                 if (range > maxUnambiguousRange)
//                 {
//                     measuredRange = std::fmod(range, maxUnambiguousRange);
//                     det.isAmbiguous = true;
//                 }
//                 else
//                 {
//                     measuredRange = range;
//                     det.isAmbiguous = false;
//                 }

//                 // // 🔴 ADD THIS HERE
//                 // if(config.mode == RadarMode::SURVEILLANCE && det.isAmbiguous)
//                 // {
//                 //     continue;
//                 // }
//                 // Apply measurement noise AFTER ambiguity folding
//                 measuredRange += rangeNoise(rng);
//                 // ----------------------------------------------------
//                 // FIRE CONTROL RANGE RESOLUTION (LOCK MODE)
//                 // ----------------------------------------------------

//                 if (config.mode == RadarMode::LOCK_ON)
//                 {
//                     // double predictedRange = range;  // In real radar this comes from track filter

//                     // det.range = resolveRangeAmbiguity(
//                     //     measuredRange,
//                     //     predictedRange,
//                     //     maxUnambiguousRange);
//                     double predictedRange = range;

//                     // Try to use track prediction if available
//                     for (const auto& track : trackDatabase)
//                     {
//                         if (track.id == target.id)
//                         {
//                             predictedRange = track.predictedRange;
//                             break;
//                         }
//                     }

//                     det.range = resolveRangeAmbiguity(
//                         measuredRange,
//                         predictedRange,
//                         maxUnambiguousRange);

//                     det.isAmbiguous = false;
//                 }
//                 else
//                 {

//                     //det.range = std::max(config.minDetectableRange, measuredRange);
//                     det.range = measuredRange;

//                     if(!det.isAmbiguous && det.range < config.minDetectableRange)
//                         det.range = config.minDetectableRange;
//                 }
//                 // -------------------------------------------------------
//                 // RESOLUTION CELL MERGE CHECK
//                 // -------------------------------------------------------

//                 bool merged = false;

//                 for (const auto& existing : scanDetections)
//                 {
//                     double rangeDiff = std::abs(existing.range - det.range);

//                     double azDiff2 = std::abs(existing.azimuth - det.azimuth);
//                     if (azDiff2 > 180.0)
//                         azDiff2 = 360.0 - azDiff2;

//                     double elDiff2 = std::abs(existing.elevation - det.elevation);

//                     if (rangeDiff < 150.0 &&
//                         azDiff2 < config.beamWidth &&
//                         elDiff2 < config.beamWidth)
//                     {
//                         merged = true;
//                         break;
//                     }
//                 }

//                 if (merged)
//                 {
//                     continue;
//                 }

//                 scanDetections.push_back(det);
//                 std::cout << "Raw detection stored: " << det.targetID << std::endl;

//                 if (config.mode == RadarMode::TWS)
//                 {
//                     TrackFile* bestTrack = nullptr;
//                     double bestProb = 0.0;
//                     for (auto& track : trackDatabase)
//                     {
//                         // Safety check
//                         if(!std::isfinite(track.x) ||
//                             !std::isfinite(track.y) ||
//                             !std::isfinite(track.z) ||
//                             !std::isfinite(track.vx) ||
//                             !std::isfinite(track.vy) ||
//                             !std::isfinite(track.vz))
//                         {
//                             track.hitCount = 0;
//                             track.isValidated = false;
//                             continue;
//                         }

//                         if (track.isUpdated)
//                             continue;
//                         // IMPORTANT FIX
//                         // if(track.id != det.targetID)
//                         //     continue;
//                         double Rmax = maxUnambiguousRange;

//                         double candidateRange = resolveRangeAmbiguity(
//                             det.range,
//                             track.predictedRange,
//                             Rmax);
//                         double rangeError =
//                             std::abs(candidateRange - track.predictedRange);

//                         if(rangeError > RANGE_GATE)
//                             continue;
//                         // double error = std::abs(candidateRange - track.predictedRange);

//                         // if (error < bestError && error < RANGE_GATE)
//                         // {
//                         //     bestError = error;
//                         //     bestTrack = &track;
//                         // }

//                         // double rangeError =
//                         //     std::abs(candidateRange - track.predictedRange);

//                         // // double velocityError =
//                         // //     std::abs(det.radialVelocity - track.velocity);

//                         // // if(rangeError > RANGE_GATE || velocityError > 200)
//                         // //     continue;
//                         // double predictedVel = track.velocity;

//                         // if(track.range > 1e-6)
//                         // {
//                         //     predictedVel =
//                         //         (track.vx * track.x +
//                         //          track.vy * track.y +
//                         //          track.vz * track.z) / track.range;
//                         // }

//                         // double velocityError =
//                         //     std::abs(det.radialVelocity - predictedVel);

//                         // if(rangeError > RANGE_GATE || velocityError > 200)
//                         //     continue;


//                         // Convert measurement to Cartesian
//                         double azRad = det.azimuth * M_PI / 180.0;
//                         double elRad = det.elevation * M_PI / 180.0;

//                         double zx = det.range * cos(elRad) * cos(azRad);
//                         double zy = det.range * cos(elRad) * sin(azRad);
//                         double zz = det.range * sin(elRad);

//                         // Innovation vector
//                         double dx = zx - track.X[0];
//                         double dy = zy - track.X[1];
//                         double dz = zz - track.X[2];

//                         // Innovation covariance (same as Kalman update)
//                         double S[3][3];

//                         for(int i=0;i<3;i++)
//                             for(int j=0;j<3;j++)
//                                 S[i][j] = track.P[i][j] + track.R[i][j];

//                         // determinant
//                         double detS =
//                             S[0][0]*(S[1][1]*S[2][2]-S[1][2]*S[2][1]) -
//                             S[0][1]*(S[1][0]*S[2][2]-S[1][2]*S[2][0]) +
//                             S[0][2]*(S[1][0]*S[2][1]-S[1][1]*S[2][0]);

//                         if(std::abs(detS) < 1e-2)
//                             continue;

//                         // inverse
//                         double invS[3][3];

//                         invS[0][0]=(S[1][1]*S[2][2]-S[1][2]*S[2][1])/detS;
//                         invS[0][1]=(S[0][2]*S[2][1]-S[0][1]*S[2][2])/detS;
//                         invS[0][2]=(S[0][1]*S[1][2]-S[0][2]*S[1][1])/detS;

//                         invS[1][0]=(S[1][2]*S[2][0]-S[1][0]*S[2][2])/detS;
//                         invS[1][1]=(S[0][0]*S[2][2]-S[0][2]*S[2][0])/detS;
//                         invS[1][2]=(S[0][2]*S[1][0]-S[0][0]*S[1][2])/detS;

//                         invS[2][0]=(S[1][0]*S[2][1]-S[1][1]*S[2][0])/detS;
//                         invS[2][1]=(S[0][1]*S[2][0]-S[0][0]*S[2][1])/detS;
//                         invS[2][2]=(S[0][0]*S[1][1]-S[0][1]*S[1][0])/detS;

//                         // Mahalanobis distance
//                         double d2 =
//                             dx*(invS[0][0]*dx + invS[0][1]*dy + invS[0][2]*dz) +
//                             dy*(invS[1][0]*dx + invS[1][1]*dy + invS[1][2]*dz) +
//                             dz*(invS[2][0]*dx + invS[2][1]*dy + invS[2][2]*dz);

//                         // gate test
//                         if(d2 > 9.21)
//                             continue;

//                         double prob =
//                             computeAssociationProbability(
//                                 candidateRange,
//                                 track.predictedRange,
//                                 RANGE_GATE);

//                         if(prob > bestProb)
//                         {
//                             bestProb = prob;
//                             bestTrack = &track;
//                         }

//                     }

//                     bool trackFound = false;

//                     if (bestTrack != nullptr)
//                     {
//                         auto& track = *bestTrack;

//                         double Rmax = maxUnambiguousRange;

//                         double bestRange = resolveRangeAmbiguity(
//                             det.range,
//                             track.predictedRange,
//                             Rmax);

//                         double dt_local = currentTime - track.lastSeenTime;
//                         if (dt_local < 1e-6)
//                             dt_local = dt;

//                         double error = bestRange - track.predictedRange;

//                         // track.range = track.predictedRange + ALPHA * error;

//                         double azRad = det.azimuth * M_PI / 180.0;
//                         double elRad = det.elevation * M_PI / 180.0;

//                         // track.x = track.range * cos(elRad) * cos(azRad);
//                         // track.y = track.range * cos(elRad) * sin(azRad);
//                         // track.z = track.range * sin(elRad);

//                         // track.velocity += (BETA / dt_local) * error;
//                         // -------- KALMAN UPDATE --------

//                         // -------- TRUE KALMAN UPDATE --------

//                         // Measurement vector
//                         double z[3];
//                         z[0] = det.range * cos(elRad) * cos(azRad);
//                         z[1] = det.range * cos(elRad) * sin(azRad);
//                         z[2] = det.range * sin(elRad);

//                         // Innovation y = z - Hx
//                         double y[3];
//                         y[0] = z[0] - track.X[0];
//                         y[1] = z[1] - track.X[1];
//                         y[2] = z[2] - track.X[2];

//                         // Innovation covariance S = HPHᵀ + R
//                         double S[3][3] = {0};

//                         for(int i=0;i<3;i++)
//                             for(int j=0;j<3;j++)
//                             {
//                                 // S = HPHᵀ + R
//                                 // Since H = [I3 0], S = P(0..2,0..2) + R
//                                 S[i][j] = track.P[i][j] + track.R[i][j];
//                             }

//                         // Inverse of 3x3 matrix S
//                         double detS =
//                             S[0][0]*(S[1][1]*S[2][2]-S[1][2]*S[2][1]) -
//                             S[0][1]*(S[1][0]*S[2][2]-S[1][2]*S[2][0]) +
//                             S[0][2]*(S[1][0]*S[2][1]-S[1][1]*S[2][0]);

//                         // if (std::abs(detS) < 1e-6)
//                         //     continue;
//                         if(std::abs(detS) < 1e-3)
//                         {
//                             track.isUpdated = false;
//                             track.hitCount = std::max(0, track.hitCount - 1);
//                             continue;
//                         }
//                         double invS[3][3];

//                         invS[0][0]=(S[1][1]*S[2][2]-S[1][2]*S[2][1])/detS;
//                         invS[0][1]=(S[0][2]*S[2][1]-S[0][1]*S[2][2])/detS;
//                         invS[0][2]=(S[0][1]*S[1][2]-S[0][2]*S[1][1])/detS;

//                         invS[1][0]=(S[1][2]*S[2][0]-S[1][0]*S[2][2])/detS;
//                         invS[1][1]=(S[0][0]*S[2][2]-S[0][2]*S[2][0])/detS;
//                         invS[1][2]=(S[0][2]*S[1][0]-S[0][0]*S[1][2])/detS;

//                         invS[2][0]=(S[1][0]*S[2][1]-S[1][1]*S[2][0])/detS;
//                         invS[2][1]=(S[0][1]*S[2][0]-S[0][0]*S[2][1])/detS;
//                         invS[2][2]=(S[0][0]*S[1][1]-S[0][1]*S[1][0])/detS;

//                         // Kalman Gain K = P Hᵀ S⁻¹
//                         double K[6][3] = {0};

//                         for(int i=0;i<6;i++)
//                             for(int j=0;j<3;j++)
//                                 for(int k=0;k<3;k++)
//                                 {
//                                     K[i][j] += track.P[i][k] * invS[k][j];
//                                 }

//                         // State update X = X + K*y
//                         for(int i=0;i<6;i++)
//                         {
//                             for(int j=0;j<3;j++)
//                                 track.X[i] += K[i][j] * y[j];
//                         }
//                         // double Pnew[6][6];

//                         // for(int i=0;i<6;i++)
//                         //     for(int j=0;j<6;j++)
//                         //     {
//                         //         double sum = 0;

//                         //         for(int k=0;k<3;k++)
//                         //             sum += K[i][k] * track.P[k][j];

//                         //         Pnew[i][j] = track.P[i][j] - sum;
//                         //     }

//                         // for(int i=0;i<6;i++)
//                         //     for(int j=0;j<6;j++)
//                         //         track.P[i][j] = Pnew[i][j];
//                         double I_KH[6][6] = {0};

//                         // Build (I - K H)
//                         // H = [I3 0], so KH only affects first 3 columns
//                         for(int i=0;i<6;i++)
//                         {
//                             for(int j=0;j<6;j++)
//                             {
//                                 if(i == j)
//                                     I_KH[i][j] = 1.0;
//                                 else
//                                     I_KH[i][j] = 0.0;

//                                 if(j < 3)
//                                     I_KH[i][j] -= K[i][j];
//                             }
//                         }

//                         // Pnew = (I-KH) * P
//                         double Pnew[6][6] = {0};

//                         for(int i=0;i<6;i++)
//                             for(int j=0;j<6;j++)
//                                 for(int k=0;k<6;k++)
//                                     Pnew[i][j] += I_KH[i][k] * track.P[k][j];

//                         // Copy back
//                         for(int i=0;i<6;i++)
//                             for(int j=0;j<6;j++)
//                                 track.P[i][j] = Pnew[i][j];

//                         // velocity sanity limit
//                         double vmax = config.maxTrackSpeed;

//                         track.X[3] = std::clamp(track.X[3], -vmax, vmax);
//                         track.X[4] = std::clamp(track.X[4], -vmax, vmax);
//                         track.X[5] = std::clamp(track.X[5], -vmax, vmax);
//                         // Sync back to variables
//                         track.x = track.X[0];
//                         track.y = track.X[1];
//                         track.z = track.X[2];

//                         track.vx = track.X[3];
//                         track.vy = track.X[4];
//                         track.vz = track.X[5];

//                         track.range = std::sqrt(
//                             track.x*track.x +
//                             track.y*track.y +
//                             track.z*track.z);
//                         if(track.range > 1e-6)
//                         {
//                             track.velocity =
//                                 (track.vx * track.x +
//                                  track.vy * track.y +
//                                  track.vz * track.z) /
//                                 track.range;
//                         }
//                         else
//                         {
//                             track.velocity = 0;
//                         }
//                         track.lastSeenTime = currentTime;
//                         track.hitCount++;
//                         track.isUpdated = true;
//                         track.missCount = 0;
//                         track.wasAmbiguous = det.isAmbiguous;

//                         if (track.hitCount >= 3)//production=3
//                             track.isValidated = true;

//                         trackFound = true;
//                     }
//                     // 3️⃣ Create new track if not found
//                     if (!trackFound)
//                     {
//                         bool exists = false;

//                         for(const auto& t : trackDatabase)
//                         {
//                             if(t.id == det.targetID)
//                             {
//                                 exists = true;
//                                 break;
//                             }
//                         }

//                         if(!exists){

//                         TrackFile newTrack;
//                         newTrack.id = det.targetID;



//                         double Rmax = maxUnambiguousRange;
//                         double unfoldedRange = det.range;

//                         if (det.isAmbiguous)
//                         {
//                             if (det.radialVelocity < 0.0)
//                                 unfoldedRange = det.range + Rmax;
//                         }

//                         // Convert spherical to Cartesian using UNFOLDED RANGE
//                         double azRad = det.azimuth * M_PI / 180.0;
//                         double elRad = det.elevation * M_PI / 180.0;

//                         newTrack.x = unfoldedRange * cos(elRad) * cos(azRad);
//                         newTrack.y = unfoldedRange * cos(elRad) * sin(azRad);
//                         newTrack.z = unfoldedRange * sin(elRad);

//                         newTrack.vx = target.vx;
//                         newTrack.vy = target.vy;
//                         newTrack.vz = target.vz;

//                         // -------- KALMAN INITIALIZATION --------

//                         newTrack.X = {
//                             newTrack.x,
//                             newTrack.y,
//                             newTrack.z,
//                             newTrack.vx,
//                             newTrack.vy,
//                             newTrack.vz
//                         };

//                         // for(int i=0;i<6;i++)
//                         //     for(int j=0;j<6;j++)
//                         //         newTrack.P[i][j] = (i==j)?1000.0:0.0;
//                         for(int i=0;i<6;i++)
//                             for(int j=0;j<6;j++)
//                                 newTrack.P[i][j] = 0.0;

//                         double posVar = config.noise.rangeStdDev * config.noise.rangeStdDev;
//                         double velVar = 500.0 * 500.0;   // expected velocity uncertainty

//                         newTrack.P[0][0] = posVar;
//                         newTrack.P[1][1] = posVar;
//                         newTrack.P[2][2] = posVar;

//                         newTrack.P[3][3] = velVar;
//                         newTrack.P[4][4] = velVar;
//                         newTrack.P[5][5] = velVar;


//                         newTrack.range = unfoldedRange;
//                         newTrack.predictedRange = unfoldedRange;

//                         newTrack.velocity = det.radialVelocity;
//                         newTrack.lastSeenTime = currentTime;
//                         newTrack.hitCount = 1;
//                         newTrack.wasAmbiguous = det.isAmbiguous;
//                         newTrack.Q[0][0] = 10;
//                         newTrack.Q[1][1] = 10;
//                         newTrack.Q[2][2] = 10;

//                         newTrack.Q[3][3] = 1;
//                         newTrack.Q[4][4] = 1;
//                         newTrack.Q[5][5] = 1;

//                         for(int i=0;i<3;i++)
//                         {
//                             newTrack.R[i][i] = 25.0;
//                         }
//                         if(trackDatabase.size() > 2000)
//                             trackDatabase.erase(trackDatabase.begin());

//                         trackDatabase.push_back(newTrack);
//                     }
//                     }
//                 }
//                 // -------------------------------------------------------
//                 // LOCK VISIBILITY CHECK
//                 // -------------------------------------------------------
//                 if (config.mode == RadarMode::LOCK_ON &&
//                     target.id == config.lockedTargetID) {
//                     lockedTargetVisible = true;
//                 }

//                 //if (config.mode != RadarMode::TWS)
//                     lastDetections.push_back(det);            }
//         }

//     }

//     // --- TWS REPORTING PHASE ---
//     if (config.mode == RadarMode::TWS)
//     {
//         lastDetections.clear();

//         for (auto& track : trackDatabase)
//         {

//             if (!track.isValidated)
//                 continue;

//             DetectionOutput out;

//             double reportRange = track.isUpdated
//                                      ? track.range
//                                      : track.predictedRange;

//             out.targetID = track.id;
//             out.range = reportRange;
//             out.radialVelocity = track.velocity;
//             out.isAmbiguous = false;
//             //out.isAmbiguous = track.wasAmbiguous;
//             // ---------------------------------------
//             // 🔵 Recompute CPA (Radial Approximation)
//             // ---------------------------------------
//             // ---------------------------------------
//             // 🔵 Recompute CPA (Full 3D Geometry)
//             // ---------------------------------------
//             double dot_pv = track.x*track.vx +
//                             track.y*track.vy +
//                             track.z*track.vz;

//             double v_sq = track.vx*track.vx +
//                           track.vy*track.vy +
//                           track.vz*track.vz;

//             if (v_sq > 0.01)
//             {
//                 double t_cpa = -dot_pv / v_sq;

//                 if (t_cpa > 0.0)
//                 {
//                     out.time_to_cpa = t_cpa;

//                     double cx = track.x + track.vx * t_cpa;
//                     double cy = track.y + track.vy * t_cpa;
//                     double cz = track.z + track.vz * t_cpa;

//                     out.cpa_distance =
//                         std::sqrt(cx*cx + cy*cy + cz*cz);
//                 }
//                 else
//                 {
//                     out.time_to_cpa = 0.0;
//                     out.cpa_distance = reportRange;
//                 }
//             }
//             else
//             {
//                 out.time_to_cpa = 0.0;
//                 out.cpa_distance = reportRange;
//             }
//             // ---------------------------------------
//             // 🔵 Recompute Probability of Kill
//             // ---------------------------------------
//             double rangeScale  = std::exp(-reportRange / 45000.0);
//             double aspectScale = (track.velocity < 0.0) ? 1.2 : 0.8;

//             out.Pk = std::min(0.99,
//                               0.95 * rangeScale * aspectScale);

//             // -------------------------------------------------------
//             // TARGET MOTION PARAMETERS (TRACK BASED)
//             // -------------------------------------------------------

//             out.speedOverGround =
//                 std::sqrt(track.vx*track.vx +
//                           track.vy*track.vy +
//                           track.vz*track.vz);

//             out.heading =
//                 std::atan2(track.vy, track.vx) * (180.0 / M_PI);

//             if (out.heading < 0)
//                 out.heading += 360.0;

//             // Simple acceleration estimate
//             out.acceleration = 0.0;

//             // Aspect relative to radar
//             double losAngle =
//                 std::atan2(track.y, track.x) * (180.0 / M_PI);

//             if (losAngle < 0)
//                 losAngle += 360.0;

//             out.targetAspect = std::abs(out.heading - losAngle);

//             if (out.targetAspect > 180.0)
//                 out.targetAspect = 360.0 - out.targetAspect;

//             out.azimuth =
//                 std::atan2(track.y, track.x) * (180.0 / M_PI);

//             if (out.azimuth < 0)
//                 out.azimuth += 360.0;

//             // double ratio = track.z / reportRange;
//             // ratio = std::clamp(ratio, -1.0, 1.0);

//             // out.elevation =
//             //     std::asin(ratio) * (180.0 / M_PI);
//             if(reportRange > 1e-6)
//             {
//                 double ratio = std::clamp(track.z / reportRange, -1.0, 1.0);
//                 out.elevation = std::asin(ratio) * (180/M_PI);
//             }
//             else
//             {
//                 out.elevation = 0;
//             }
//             // out.elevation =
//             //     std::asin(track.z / reportRange) * (180.0 / M_PI);
//             //lastDetections.push_back(out);
//             static std::unordered_map<int,double> lastReportTime;

//            // double minReportInterval = 0.5;//.03

//            // if(currentTime - lastReportTime[track.id] > minReportInterval)
//            // {
//                 lastDetections.push_back(out);
//                // lastReportTime[track.id] = currentTime;
//             //}

//             track.isUpdated = false;
//         }
//     }
//     std::cout << "Track DB size: " << trackDatabase.size() << std::endl;

//     int validated = 0;
//     for(const auto& t : trackDatabase)
//     {
//         if(t.isValidated) validated++;
//     }

//     std::cout << "Validated tracks: " << validated << std::endl;
//     // --- 4. BREAK-LOCK LOGIC ---
//     if (config.mode == RadarMode::LOCK_ON && !lockedTargetVisible) {
//         config.mode = RadarMode::SURVEILLANCE;
//         config.lockedTargetID = 0;
//         std::cout << "\n[!] BREAK LOCK: Target lost at " << currentAzimuth << " deg\n";
//     }
// }

// double RadarModel::resolveRangeAmbiguity(
//     double measuredRange,
//     double predictedRange,
//     double Rmax)
// {
//     double bestRange = measuredRange;
//     double minError = 1e12;
//     if(Rmax < 1) return measuredRange;
//     for (int k = -5; k <= 5; k++)
//     {
//         double candidate = measuredRange + k * Rmax;
//         double error = std::abs(candidate - predictedRange);

//         if (error < minError)
//         {
//             minError = error;
//             bestRange = candidate;
//         }
//     }

//     return bestRange;
// }
// bool RadarModel::checkHorizon(double range, double targetZ)
// {
//     double Re = 6371000.0;
//     double effectiveRadius = Re * config.earthRadiusFactor;

//     double radarHeight = config.radarHeight;
//     double targetHeight = std::max(0.0, targetZ);

//     double d_radar = std::sqrt(2.0 * effectiveRadius * radarHeight);
//     double d_target = std::sqrt(2.0 * effectiveRadius * targetHeight);

//     double maxVisibleRange = d_radar + d_target;

//     return range <= maxVisibleRange;
// }

// bool RadarModel::checkBeamIntersection(double targetAz, double targetEl) {
//     // Target is hit if it falls within half the beamWidth of the current beam center
//     double azDiff = std::abs(currentAzimuth - targetAz);
//     if (azDiff > 180.0) azDiff = 360.0 - azDiff;
//     double elDiff = std::abs(currentElevation - targetEl);
//     double angularDist = std::sqrt(azDiff * azDiff + elDiff * elDiff);
//     // return (angularDist <= (config.beamWidth / 2.0));
//     return (angularDist <= config.beamWidth);

// }


// double RadarModel::calculateSignalStrength(double range, double rcs) {
//     if (range < 1.0) range = 1.0;

//     // 1. Determine Operating Frequency (handle Frequency Agility)
//     double freq = config.frequency_Hz;
//     if (config.frequencyAgility) {
//         // Frequency shifts based on hopRate and hopStep
//         // This ensures the radar "hops" across the spectrum
//         freq += (config.hopStepFrequency * config.hopRate);
//     }

//     // 2. Calculate wavelength (lambda = c / f)
//     double lambda = 299792458.0 / freq;

//     // 3. Convert units
//     double Pt = config.emissionPower_kW * 1000.0; // kW to Watts
//     double G_linear = std::pow(10.0, config.antennaGain / 10.0); // dB to Linear

//     // 4. Calculate Received Power (The Radar Equation)
//     // SNR is proportional to (Pt * G^2 * lambda^2 * sigma) / ((4*PI)^3 * R^4)
//     double numerator = Pt * std::pow(G_linear, 2) * std::pow(lambda, 2) * rcs;
//     double denominator = std::pow(4.0 * M_PI, 3) * std::pow(range, 4);

//     double snr = numerator / denominator;

//     double noiseFloor = 1e-18;

//     return std::max(0.0, snr + noiseFloor);
// }
// std::vector<DetectionOutput> RadarModel::getActiveDetections()
// {
//     std::lock_guard<std::mutex> lock(radarMutex);
//     return lastDetections;
// }

// std::vector<TrackFile> RadarModel::getActiveTracks()
// {
//     std::lock_guard<std::mutex> lock(radarMutex);

//     std::vector<TrackFile> active;

//     for(const auto& track : trackDatabase)
//     {
//         if(track.isValidated)
//             active.push_back(track);
//     }

//     return active;
// }
// void RadarModel::reset() {
//     std::lock_guard<std::mutex> lock(radarMutex);
//     currentAzimuth = 0.0;
//     currentElevation = 0.0;
//     trackDatabase.clear();
//     lastDetections.clear();
// }

// // Factory functions for the .so
// extern "C" {
// RadarModel* createRadar() { return new RadarModel(); }
// void destroyRadar(RadarModel* obj) { delete obj; }
// }
