// =============================================================================
// radarsignalprocessor.cpp  —  Pure radar physics implementation
//
// All original methods are preserved exactly.
// Changes:
//   • computeSINR() now multiplies by computeModulationProcessingGain() —
//     the only change to an existing method body.
//   • computeBeamGainFactor()        added (§I)
//   • computeModulationProcessingGain() added (§J)
// =============================================================================

#include "radarsignalprocessor.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Physical constants
// =============================================================================

static constexpr double BOLTZMANN      = 1.380649e-23;
static constexpr double SPEED_OF_LIGHT = 299792458.0;
static constexpr double MERGE_RANGE_GATE       = 150.0;
static constexpr double SEA_SIGMA0_PER_SS      = 3e-3;
static constexpr double LAND_SIGMA0            = 1e-2;
static constexpr double STATIONARY_VEL_THRESHOLD = 5.0;

static thread_local std::default_random_engine tl_rng{ std::random_device{}() };

// =============================================================================
// §A  Beam / horizon geometry  (unchanged)
// =============================================================================

bool RadarSignalProcessor::isTargetInBeam(
    double currentAzimuth, double currentElevation,
    double targetAz, double targetEl,
    double dt, const RadarConfig& cfg,
    double& outAzDiff, double& outElDiff, double& outScanMargin) const
{
    outAzDiff = std::abs(currentAzimuth - targetAz);
    if (outAzDiff > 180.0) outAzDiff = 360.0 - outAzDiff;

    outElDiff = std::abs(currentElevation - targetEl);

    double scanSpeed  = (cfg.scanningRate_RPM / 60.0) * 360.0;
    outScanMargin     = (cfg.beamWidth * 2.5) + (scanSpeed * dt);

    return (outAzDiff <= outScanMargin && outElDiff <= outScanMargin);
}

bool RadarSignalProcessor::checkHorizon(
    double range, double targetZ, const RadarConfig& cfg) const
{
    double Re   = 6371000.0 * cfg.earthRadiusFactor * cfg.atmosphericFactor;
    double dRadar = std::sqrt(2.0 * Re * cfg.radarHeight);
    double dTgt   = std::sqrt(2.0 * Re * std::max(0.0, targetZ));
    return range <= (dRadar + dTgt);
}

// =============================================================================
// §B  Signal chain  (unchanged except computeSINR — see below)
// =============================================================================

double RadarSignalProcessor::calculateSignalStrength(
    double range, double rcs, const RadarConfig& cfg) const
{
    if (range < 1.0) range = 1.0;

    double freq = cfg.frequency_Hz;
    // if (cfg.frequencyAgility)
    //     freq += static_cast<double>(cfg.hopStepFrequency * cfg.hopRate);
    if (cfg.frequencyAgility) {
        freq += static_cast<double>(cfg.hopStepFrequency * cfg.hopRate);
        // clamp to declared hop band
        if (cfg.hopStopFrequency > cfg.hopStartFrequency && cfg.hopStopFrequency > 0.0f)
            freq = std::clamp(freq,
                              static_cast<double>(cfg.hopStartFrequency),
                              static_cast<double>(cfg.hopStopFrequency));
    }
    double lambda = SPEED_OF_LIGHT / freq;
    double Pt     = cfg.emissionPower_kW * 1000.0;
    double G      = std::pow(10.0, cfg.antennaGain / 10.0);

    double Pr = (Pt * G * G * lambda * lambda * rcs)
                / (std::pow(4.0 * M_PI, 3) * std::pow(range, 4));

    return std::max(0.0, Pr * computePropagationLoss(range, cfg));
}

double RadarSignalProcessor::computeNoisePower(const RadarConfig& cfg) const
{
    double B = std::max(1.0, cfg.antennaBandwidth);
    double F = std::pow(10.0, cfg.noiseFigure_dB / 10.0);
    return BOLTZMANN * cfg.systemTemperature_K * B * F;
}

double RadarSignalProcessor::computeClutterPower(
    double range, SurfaceType surface, const RadarConfig& cfg) const
{
    if (surface == SurfaceType::AIR || range < 1.0) return 0.0;

    double sigma0 = 0.0;
    if (surface == SurfaceType::SEA)
        sigma0 = cfg.seaState   * SEA_SIGMA0_PER_SS;
    else if (surface == SurfaceType::LAND)
        sigma0 = cfg.landClutter * LAND_SIGMA0;

    if (sigma0 <= 0.0) return 0.0;

    double tau   = (cfg.pulseWidth > 0.0f)
                     ? static_cast<double>(cfg.pulseWidth) : 1e-6;
    double bwRad = static_cast<double>(cfg.beamWidth) * M_PI / 180.0;
    double patch = (SPEED_OF_LIGHT * tau / 2.0) * (range * bwRad);

    double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;
    double Pt     = cfg.emissionPower_kW * 1000.0;
    double G      = std::pow(10.0, cfg.antennaGain / 10.0);

    double Pc = (Pt * G * G * lambda * lambda * sigma0 * patch)
                / (std::pow(4.0 * M_PI, 3) * std::pow(range, 3));

    thread_local std::exponential_distribution<double> fluct(1.0);
    return Pc * fluct(tl_rng);
}

double RadarSignalProcessor::computeJammerPower(
    double targetRange_m, const TargetInput& target, const RadarConfig& cfg) const
{
    const auto& j = target.jammer;
    if (!j.active || j.power_kW <= 0.0) return 0.0;

    double Pj  = j.power_kW * 1000.0;
    double Gj  = std::pow(10.0, j.gain_dBi / 10.0);
    double Gr  = std::pow(10.0, cfg.antennaGain / 10.0);
    double lam = SPEED_OF_LIGHT / cfg.frequency_Hz;

    double Rj = j.selfScreening ? targetRange_m
                                : (j.range_m > 1.0 ? j.range_m : targetRange_m);

    double Pr_j = (Pj * Gj * Gr * lam * lam)
                  / (std::pow(4.0 * M_PI, 2) * Rj * Rj);

    double B_r = std::max(1.0, cfg.antennaBandwidth);
    double B_j = std::max(1.0, j.bandwidth_Hz);
    return Pr_j * std::min(1.0, B_r / B_j);
}

double RadarSignalProcessor::computePropagationLoss(
    double range_m, const RadarConfig& cfg) const
{
    double loss_dB = 0.0;

    if (cfg.rainRate_mmph > 0.0)
    {
        constexpr double k = 0.00887, a = 1.255;
        double gamma = k * std::pow(cfg.rainRate_mmph, a);
        loss_dB += 2.0 * gamma * (range_m / 1000.0);
    }

    if (cfg.fogVisibility_m > 1.0 && cfg.fogVisibility_m < 2000.0)
    {
        double M     = 0.0367 * std::pow(1000.0 / cfg.fogVisibility_m, 1.43);
        double gamma = 0.0157 * std::pow(M, 1.05);
        loss_dB += 2.0 * gamma * (range_m / 1000.0);
    }

    return std::pow(10.0, -loss_dB / 10.0);
}

// computeSINR — only change vs original: multiply signal by processing gain
double RadarSignalProcessor::computeSINR(
    double receivedPower, double range,
    SurfaceType surface, const TargetInput& target, const RadarConfig& cfg) const
{
    double Pn = computeNoisePower(cfg);
    double Pc = computeClutterPower(range, surface, cfg);
    double Pj = computeJammerPower(range, target, cfg);

    // Apply modulation processing gain (pulse compression / CW integration)
    double pg = computeModulationProcessingGain(cfg);

    return std::max(0.0, (receivedPower * pg) / (Pn + Pc + Pj));
}

// =============================================================================
// §C  CFAR  (unchanged)
// =============================================================================

std::vector<double> RadarSignalProcessor::generateReferenceCells(
    SurfaceType surface, const RadarConfig& cfg) const
{
    thread_local std::exponential_distribution<double> cellDist(1.0);
    std::vector<double> cells;
    cells.reserve(16);
    for (int i = 0; i < 16; ++i)
    {
        double c = cellDist(tl_rng);
        if (surface == SurfaceType::SEA)  c *= (1.0 + cfg.seaState    * 0.3);
        if (surface == SurfaceType::LAND) c *= (1.0 + cfg.landClutter * 0.5);
        cells.push_back(c);
    }
    return cells;
}

double RadarSignalProcessor::computeCFARThreshold(
    const std::vector<double>& cells, const RadarConfig& cfg) const
{
    if (cells.empty()) return 1e12;
    double sum = 0.0;
    for (double v : cells) sum += v;
    double N     = static_cast<double>(cells.size());
    double alpha = N * (std::pow(cfg.targetPfa, -1.0 / N) - 1.0);
    return (sum / N) * alpha;
}

double RadarSignalProcessor::computeCFARThresholdRelaxed(
    const std::vector<double>& cells, const RadarConfig& cfg) const
{
    if (cells.empty()) return 1e12;
    double sum = 0.0;
    for (double v : cells) sum += v;
    double N     = static_cast<double>(cells.size());
    double pfa   = std::min(1e-4, cfg.targetPfa * 100.0);
    double alpha = N * (std::pow(pfa, -1.0 / N) - 1.0);
    return (sum / N) * alpha;
}

// =============================================================================
// §D  Effective RCS  (unchanged)
// =============================================================================

double RadarSignalProcessor::computeEffectiveRCS(
    const TargetInput& target, double range) const
{
    double velMag = std::sqrt(
        target.vx*target.vx + target.vy*target.vy + target.vz*target.vz);

    if (velMag < 0.01) return target.rcs * 0.6;

    double dot = std::clamp(
        (target.vx / velMag) * (target.x / range) +
            (target.vy / velMag) * (target.y / range) +
            (target.vz / velMag) * (target.z / range),
        -1.0, 1.0);

    double aspectRad    = std::acos(dot);
    double aspectFactor = 0.2 + 0.8 * std::abs(std::sin(aspectRad));
    return target.rcs * aspectFactor;
}

// =============================================================================
// §E  Target motion  (unchanged)
// =============================================================================

void RadarSignalProcessor::computeTargetMotionParams(
    DetectionOutput& det, const TargetInput& target, double range) const
{
    det.speedOverGround = std::sqrt(target.vx*target.vx + target.vy*target.vy);
    det.heading = std::atan2(target.vy, target.vx) * (180.0 / M_PI);
    if (det.heading < 0.0) det.heading += 360.0;

    det.acceleration = 0.0;

    if (det.speedOverGround > 0.01)
    {
        double vxn = target.vx / det.speedOverGround;
        double vzn = target.vz / det.speedOverGround;
        double dot = std::clamp(
            vxn*(target.x/range) + vzn*(target.z/range), -1.0, 1.0);
        det.targetAspect = std::acos(dot) * 180.0 / M_PI;
    }
    else
        det.targetAspect = 0.0;
}

double RadarSignalProcessor::computeRadialVelocity(
    const TargetInput& target, double range,
    std::normal_distribution<double>& dopplerNoise) const
{
    double dot = target.vx*target.x + target.vy*target.y + target.vz*target.z;
    double rv  = (range > 1e-6) ? (dot / range) : 0.0;
    return rv + dopplerNoise(tl_rng);
}

void RadarSignalProcessor::computeCPA(
    DetectionOutput& det, const TargetInput& target, double range) const
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

double RadarSignalProcessor::computePk(
    double range, double radialVelocity) const
{
    return std::min(0.99,
                    0.95 * std::exp(-range / 45000.0) * (radialVelocity < 0.0 ? 1.2 : 0.8));
}

// =============================================================================
// §F  Range ambiguity  (unchanged)
// =============================================================================

double RadarSignalProcessor::resolveRangeAmbiguity(
    double measured, double predicted, double Rmax) const
{
    if (Rmax < 1.0) return measured;
    double best   = measured;
    double minErr = 1e12;
    for (int k = -5; k <= 5; ++k)
    {
        double cand = measured + k * Rmax;
        double err  = std::abs(cand - predicted);
        if (err < minErr) { minErr = err; best = cand; }
    }
    return best;
}

void RadarSignalProcessor::applyRangeAmbiguity(
    DetectionOutput& det, double range,
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

void RadarSignalProcessor::resolveRangeForLockOn(
    DetectionOutput& det, double range,
    double maxUnambiguousRange,
    uint32_t targetId,
    const std::vector<TrackFile>& trackDatabase) const
{
    double predicted = range;
    for (const auto& t : trackDatabase)
        if (t.id == targetId) { predicted = t.predictedRange; break; }

    det.range       = resolveRangeAmbiguity(det.range, predicted, maxUnambiguousRange);
    det.isAmbiguous = false;
}

// =============================================================================
// §G  Maximum detection range  (unchanged)
// =============================================================================

double RadarSignalProcessor::computeMaxDetectionRange(
    double rcs, const RadarConfig& cfg) const
{
    double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;
    double Pt     = cfg.emissionPower_kW * 1000.0;
    double G      = std::pow(10.0, cfg.antennaGain / 10.0);
    double Pn     = computeNoisePower(cfg);

    double N     = 16.0;
    double alpha = N * (std::pow(cfg.targetPfa, -1.0 / N) - 1.0);

    double R_est = 50000.0;
    for (int i = 0; i < 5; ++i)
    {
        double Pc  = std::max(computeClutterPower(R_est, SurfaceType::SEA,  cfg),
                             computeClutterPower(R_est, SurfaceType::LAND, cfg));
        double Pnt = Pn + Pc;
        double prop = computePropagationLoss(R_est, cfg);
        double Pt_e = Pt * prop * prop;

        double num = Pt_e * G * G * lambda * lambda * (rcs * 0.6);
        double den = std::pow(4.0 * M_PI, 3) * Pnt * alpha;
        if (den <= 0.0) break;
        R_est = std::pow(num / den, 0.25);
    }

    double Re      = 6371000.0 * cfg.earthRadiusFactor * cfg.atmosphericFactor;
    double horizon = std::sqrt(2.0 * Re * cfg.radarHeight) / 1000.0;
    double R_km    = std::min(R_est / 1000.0, horizon);
    return std::max(R_km, cfg.minDetectableRange / 1000.0 * 2.0);
}

// =============================================================================
// §H  Detection merge guard  (unchanged)
// =============================================================================

bool RadarSignalProcessor::shouldMergeDetection(
    const DetectionOutput& det,
    const std::vector<DetectionOutput>& existing,
    const RadarConfig& cfg) const
{
    for (const auto& ex : existing)
    {
        double azDiff = std::abs(ex.azimuth - det.azimuth);
        if (azDiff > 180.0) azDiff = 360.0 - azDiff;

        if (std::abs(ex.range - det.range) < MERGE_RANGE_GATE &&
            azDiff  < cfg.beamWidth &&
            std::abs(ex.elevation - det.elevation) < cfg.beamWidth)
            return true;
    }
    return false;
}

// =============================================================================
// §I  Beam gain factor  (NEW)
// =============================================================================

double RadarSignalProcessor::computeBeamGainFactor(
    double azDiff, double elDiff, const RadarConfig& cfg) const
{
    double halfBeam = static_cast<double>(cfg.beamWidth) / 2.0;

    // Target is inside the main beam — full gain
    if (azDiff <= halfBeam && elDiff <= halfBeam)
        return 1.0;

    // Target is in the sidelobe region.
    // Use peak sidelobe level (dB, negative value) as the gain factor.
    // Average sidelobe level used for targets further than 2 beam widths out.
    double sidelobedB = (azDiff <= static_cast<double>(cfg.beamWidth) * 2.0 &&
                         elDiff <= static_cast<double>(cfg.beamWidth) * 2.0)
                            ? static_cast<double>(cfg.peakSidelobeLevel)
                            : static_cast<double>(cfg.avgSidelobeLevel);

    return std::pow(10.0, sidelobedB / 10.0);
}

// =============================================================================
// §J  Modulation processing gain  (NEW)
// =============================================================================

double RadarSignalProcessor::computeModulationProcessingGain(
    const RadarConfig& cfg) const
{
    switch (cfg.modulation)
    {
    case ModulationType::LFM:
    case ModulationType::NLFM:
    case ModulationType::FMCW:
    {
        // Time-bandwidth product:  TBP = B · τ
        // For LFM / NLFM / FMCW this is the pulse-compression gain.
        // Clamped at 1.0 minimum — no anti-gain from degenerate parameters.
        double tau = static_cast<double>(cfg.pulseWidth);
        double B   = cfg.antennaBandwidth;
        return std::max(1.0, B * tau);
    }
    case ModulationType::CW:
        // CW has no range resolution but Doppler integration gain.
        // Model as 1.0 here — a proper CW integration time would need its
        // own config parameter; leave as extension point.
        return 1.0;

    case ModulationType::NONE:
    default:
        return 1.0;
    }
}
// // =============================================================================
// // radarsignalprocessor.cpp  —  Pure radar physics implementation
// //
// // All methods are const / stateless.  No mutex needed — this class is only
// // ever called from RadarModel::update() which already holds the master lock.
// //
// // Physical constants are file-scoped; they do not belong in the header.
// // Thread-local RNG avoids contention when multiple simulation instances run
// // on different threads.
// // =============================================================================

// #include "radarsignalprocessor.h"

// #include <algorithm>
// #include <cmath>

// #ifndef M_PI
// #define M_PI 3.14159265358979323846
// #endif

// // =============================================================================
// // Physical constants  (file-scope — not part of the public API)
// // =============================================================================

// static constexpr double BOLTZMANN      = 1.380649e-23;  ///< J/K
// static constexpr double SPEED_OF_LIGHT = 299792458.0;   ///< m/s

// /// Association gate radius for detection-merge test (metres)
// static constexpr double MERGE_RANGE_GATE = 150.0;

// /// Clutter backscatter coefficient per Beaufort unit (sea)
// static constexpr double SEA_SIGMA0_PER_SS = 3e-3;

// /// Clutter backscatter coefficient for nominal land cover
// static constexpr double LAND_SIGMA0       = 1e-2;

// /// Radial-velocity threshold below which relaxed CFAR is used (m/s)
// static constexpr double STATIONARY_VEL_THRESHOLD = 5.0;

// // Thread-local RNG — each CPU thread that calls into the processor gets its
// // own engine; avoids mutex contention on the RNG without sacrificing
// // per-thread repeatability.
// // Declared static so the TLS init symbol is internal to this translation unit.
// static thread_local std::default_random_engine tl_rng{ std::random_device{}() };

// // =============================================================================
// // §A  Beam / horizon geometry
// // =============================================================================

// bool RadarSignalProcessor::isTargetInBeam(
//     double currentAzimuth, double currentElevation,
//     double targetAz, double targetEl,
//     double dt, const RadarConfig& cfg,
//     double& outAzDiff, double& outElDiff, double& outScanMargin) const
// {
//     // Compute signed azimuth difference, wrapped to [0, 180]
//     outAzDiff = std::abs(currentAzimuth - targetAz);
//     if (outAzDiff > 180.0) outAzDiff = 360.0 - outAzDiff;

//     outElDiff = std::abs(currentElevation - targetEl);

//     // Scan margin = 2.5× beam width + distance swept in one tick.
//     // The generous multiplier handles the transition from last tick's position
//     // to this tick's position without dropping targets near the beam edge.
//     double scanSpeed  = (cfg.scanningRate_RPM / 60.0) * 360.0; // deg/s
//     outScanMargin     = (cfg.beamWidth * 2.5) + (scanSpeed * dt);

//     return (outAzDiff <= outScanMargin && outElDiff <= outScanMargin);
// }

// bool RadarSignalProcessor::checkHorizon(
//     double range, double targetZ, const RadarConfig& cfg) const
// {
//     // 4/3 Earth effective radius adjusted by atmospheric refraction factor.
//     // atmosphericFactor > 1 → super-refraction (ducting); < 1 → sub-refraction.
//     double Re     = 6371000.0 * cfg.earthRadiusFactor * cfg.atmosphericFactor;

//     // Geometric horizon distance for the radar platform
//     double dRadar = std::sqrt(2.0 * Re * cfg.radarHeight);

//     // Geometric horizon distance for the target (altitude clipped at 0 m)
//     double dTgt   = std::sqrt(2.0 * Re * std::max(0.0, targetZ));

//     // Target is visible if slant range ≤ sum of both horizon distances
//     return range <= (dRadar + dTgt);
// }

// // =============================================================================
// // §B  Signal chain
// // =============================================================================

// double RadarSignalProcessor::calculateSignalStrength(
//     double range, double rcs, const RadarConfig& cfg) const
// {
//     // Guard against divide-by-zero at point-blank range
//     if (range < 1.0) range = 1.0;

//     // If frequency agility is enabled, shift centre frequency by hop offset
//     double freq = cfg.frequency_Hz;
//     if (cfg.frequencyAgility)
//         freq += static_cast<double>(cfg.hopStepFrequency * cfg.hopRate);

//     double lambda = SPEED_OF_LIGHT / freq;
//     double Pt     = cfg.emissionPower_kW * 1000.0;             // kW → W
//     double G      = std::pow(10.0, cfg.antennaGain / 10.0);    // dBi → linear

//     // Standard monostatic radar range equation:
//     //   Pr = (Pt · G² · λ² · σ) / ((4π)³ · R⁴)
//     double Pr = (Pt * G * G * lambda * lambda * rcs)
//                 / (std::pow(4.0 * M_PI, 3) * std::pow(range, 4));

//     // Multiply by two-way propagation loss (rain + fog attenuation)
//     return std::max(0.0, Pr * computePropagationLoss(range, cfg));
// }

// double RadarSignalProcessor::computeNoisePower(const RadarConfig& cfg) const
// {
//     // Receiver noise power:  Pn = k · T · B · F
//     //   k  = Boltzmann constant
//     //   T  = system temperature (K)
//     //   B  = receiver bandwidth (Hz), clipped at 1 Hz minimum
//     //   F  = noise figure (linear)
//     double B = std::max(1.0, cfg.antennaBandwidth);
//     double F = std::pow(10.0, cfg.noiseFigure_dB / 10.0); // dB → linear
//     return BOLTZMANN * cfg.systemTemperature_K * B * F;
// }

// double RadarSignalProcessor::computeClutterPower(
//     double range, SurfaceType surface, const RadarConfig& cfg) const
// {
//     // Air targets produce no surface clutter return
//     if (surface == SurfaceType::AIR || range < 1.0) return 0.0;

//     // Select normalised clutter coefficient σ₀ based on surface type
//     double sigma0 = 0.0;
//     if (surface == SurfaceType::SEA)
//         sigma0 = cfg.seaState   * SEA_SIGMA0_PER_SS;
//     else if (surface == SurfaceType::LAND)
//         sigma0 = cfg.landClutter * LAND_SIGMA0;

//     if (sigma0 <= 0.0) return 0.0;

//     // Illuminated clutter patch area:
//     //   radial extent = c·τ/2  (half pulse-length in range)
//     //   azimuthal extent = R·θ  (range × beam width in radians)
//     double tau    = (cfg.pulseWidth > 0.0f)
//                      ? static_cast<double>(cfg.pulseWidth) : 1e-6;
//     double bwRad  = static_cast<double>(cfg.beamWidth) * M_PI / 180.0;
//     double patch  = (SPEED_OF_LIGHT * tau / 2.0) * (range * bwRad);

//     double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;
//     double Pt     = cfg.emissionPower_kW * 1000.0;
//     double G      = std::pow(10.0, cfg.antennaGain / 10.0);

//     // Clutter power (range³ denominator — clutter fills the beam)
//     double Pc = (Pt * G * G * lambda * lambda * sigma0 * patch)
//                 / (std::pow(4.0 * M_PI, 3) * std::pow(range, 3));

//     // Swerling I amplitude fluctuation — exponential distribution, mean = 1
//     thread_local std::exponential_distribution<double> fluct(1.0);
//     return Pc * fluct(tl_rng);
// }

// double RadarSignalProcessor::computeJammerPower(
//     double targetRange_m, const TargetInput& target, const RadarConfig& cfg) const
// {
//     const auto& j = target.jammer;
//     if (!j.active || j.power_kW <= 0.0) return 0.0;

//     double Pj  = j.power_kW * 1000.0;
//     double Gj  = std::pow(10.0, j.gain_dBi / 10.0);
//     double Gr  = std::pow(10.0, cfg.antennaGain / 10.0); // radar receive gain
//     double lam = SPEED_OF_LIGHT / cfg.frequency_Hz;

//     // Jammer range:
//     //   selfScreening  → jammer is on the target, so use targetRange_m
//     //   stand-off      → use configured stand-off range (j.range_m)
//     double Rj = j.selfScreening ? targetRange_m
//                                 : (j.range_m > 1.0 ? j.range_m : targetRange_m);

//     // One-way jammer power at the radar receiver
//     double Pr_j = (Pj * Gj * Gr * lam * lam)
//                   / (std::pow(4.0 * M_PI, 2) * Rj * Rj);

//     // Bandwidth ratio: only the fraction of jammer bandwidth overlapping
//     // the receiver bandwidth contributes (partial noise jamming)
//     double B_r = std::max(1.0, cfg.antennaBandwidth);
//     double B_j = std::max(1.0, j.bandwidth_Hz);
//     return Pr_j * std::min(1.0, B_r / B_j);
// }

// double RadarSignalProcessor::computePropagationLoss(
//     double range_m, const RadarConfig& cfg) const
// {
//     double loss_dB = 0.0;

//     // Rain attenuation — ITU-R P.838 approximation (specific attenuation)
//     //   γ = k · R^α  [dB/km, one-way]
//     //   Two-way loss = 2 · γ · range_km
//     if (cfg.rainRate_mmph > 0.0)
//     {
//         constexpr double k = 0.00887, a = 1.255;
//         double gamma = k * std::pow(cfg.rainRate_mmph, a);
//         loss_dB += 2.0 * gamma * (range_m / 1000.0);
//     }

//     // Fog attenuation — empirical model based on liquid water content
//     //   Valid only for dense fog (visibility 1 m … 2 km)
//     if (cfg.fogVisibility_m > 1.0 && cfg.fogVisibility_m < 2000.0)
//     {
//         double M     = 0.0367 * std::pow(1000.0 / cfg.fogVisibility_m, 1.43);
//         double gamma = 0.0157 * std::pow(M, 1.05);
//         loss_dB += 2.0 * gamma * (range_m / 1000.0);
//     }

//     // Convert total dB loss to a linear multiplicative factor [0,1]
//     return std::pow(10.0, -loss_dB / 10.0);
// }

// double RadarSignalProcessor::computeSINR(
//     double receivedPower, double range,
//     SurfaceType surface, const TargetInput& target, const RadarConfig& cfg) const
// {
//     double Pn = computeNoisePower(cfg);
//     double Pc = computeClutterPower(range, surface, cfg);
//     double Pj = computeJammerPower(range, target, cfg);

//     // SINR = signal / (noise + clutter + jamming)
//     // Clipped at 0 — negative SINR has no physical meaning here
//     return std::max(0.0, receivedPower / (Pn + Pc + Pj));
// }

// // =============================================================================
// // §C  CFAR detection threshold
// // =============================================================================

// std::vector<double> RadarSignalProcessor::generateReferenceCells(
//     SurfaceType surface, const RadarConfig& cfg) const
// {
//     thread_local std::exponential_distribution<double> cellDist(1.0);

//     std::vector<double> cells;
//     cells.reserve(16);

//     for (int i = 0; i < 16; ++i)
//     {
//         double c = cellDist(tl_rng);

//         // Scale reference cells by local clutter level
//         if (surface == SurfaceType::SEA)
//             c *= (1.0 + cfg.seaState    * 0.3);
//         if (surface == SurfaceType::LAND)
//             c *= (1.0 + cfg.landClutter * 0.5);

//         cells.push_back(c);
//     }
//     return cells;
// }

// double RadarSignalProcessor::computeCFARThreshold(
//     const std::vector<double>& cells, const RadarConfig& cfg) const
// {
//     if (cells.empty()) return 1e12;

//     double sum = 0.0;
//     for (double v : cells) sum += v;

//     double N     = static_cast<double>(cells.size());
//     // CA-CFAR multiplier: α = N · (Pfa^(−1/N) − 1)
//     double alpha = N * (std::pow(cfg.targetPfa, -1.0 / N) - 1.0);
//     return (sum / N) * alpha;
// }

// double RadarSignalProcessor::computeCFARThresholdRelaxed(
//     const std::vector<double>& cells, const RadarConfig& cfg) const
// {
//     if (cells.empty()) return 1e12;

//     double sum = 0.0;
//     for (double v : cells) sum += v;

//     double N = static_cast<double>(cells.size());

//     // Relax Pfa by a factor of 100 for near-stationary / slow targets.
//     // These targets have reduced Doppler discrimination, so a stricter
//     // threshold would miss them entirely; we accept a higher false-alarm
//     // rate in exchange for improved slow-mover detection.
//     double pfa   = std::min(1e-4, cfg.targetPfa * 100.0);
//     double alpha = N * (std::pow(pfa, -1.0 / N) - 1.0);
//     return (sum / N) * alpha;
// }

// // =============================================================================
// // §D  Effective RCS
// // =============================================================================

// double RadarSignalProcessor::computeEffectiveRCS(
//     const TargetInput& target, double range) const
// {
//     double velMag = std::sqrt(
//         target.vx*target.vx + target.vy*target.vy + target.vz*target.vz);

//     // Stationary targets: no aspect information available from Doppler;
//     // apply a conservative 40 % reduction to nominal RCS.
//     if (velMag < 0.01) return target.rcs * 0.6;

//     // Compute cosine of the angle between velocity vector and LOS vector.
//     // LOS unit vector = (x, y, z) / range
//     double dot = std::clamp(
//         (target.vx / velMag) * (target.x / range) +
//             (target.vy / velMag) * (target.y / range) +
//             (target.vz / velMag) * (target.z / range),
//         -1.0, 1.0);

//     // Aspect angle relative to velocity vector
//     double aspectRad    = std::acos(dot);

//     // |sin(aspect)| peaks at broad-side (90°) and is near 0 for head-on/tail-on.
//     // The 0.2 floor prevents the RCS from going to zero at exactly 0° aspect.
//     double aspectFactor = 0.2 + 0.8 * std::abs(std::sin(aspectRad));
//     return target.rcs * aspectFactor;
// }

// // =============================================================================
// // §E  Target motion parameters
// // =============================================================================

// void RadarSignalProcessor::computeTargetMotionParams(
//     DetectionOutput& det, const TargetInput& target, double range) const
// {
//     // Speed-over-ground: horizontal component only (vx = East, vz = North)
//     // det.speedOverGround = std::sqrt(target.vx*target.vx + target.vz*target.vz);
//     // det.heading = std::atan2(target.vx, target.vz) * (180.0 / M_PI);
//     // vx=North, vy=East now
//     det.speedOverGround = std::sqrt(target.vx*target.vx + target.vy*target.vy);
//     det.heading = std::atan2(target.vy, target.vx) * (180.0 / M_PI);
//     if (det.heading < 0.0) det.heading += 360.0;

//     det.acceleration    = 0.0; // Not modelled in current version

//     // Target aspect: angle between target's velocity vector and LOS
//     if (det.speedOverGround > 0.01)
//     {
//         double vxn = target.vx / det.speedOverGround;
//         double vzn = target.vz / det.speedOverGround;
//         double dot = std::clamp(
//             vxn*(target.x/range) + vzn*(target.z/range), -1.0, 1.0);
//         det.targetAspect = std::acos(dot) * 180.0 / M_PI;
//     }
//     else
//         det.targetAspect = 0.0;
// }

// double RadarSignalProcessor::computeRadialVelocity(
//     const TargetInput& target, double range,
//     std::normal_distribution<double>& dopplerNoise) const
// {
//     // Radial velocity = dot(velocity, LOS unit vector)
//     //   positive = receding, negative = closing
//     double dot = target.vx*target.x + target.vy*target.y + target.vz*target.z;
//     double rv  = (range > 1e-6) ? (dot / range) : 0.0;
//     return rv + dopplerNoise(tl_rng);
// }

// void RadarSignalProcessor::computeCPA(
//     DetectionOutput& det, const TargetInput& target, double range) const
// {
//     // Default: CPA is at current position
//     det.cpa_distance = range;
//     det.time_to_cpa  = 0.0;

//     double v2 = target.vx*target.vx + target.vy*target.vy + target.vz*target.vz;
//     if (v2 > 0.01)
//     {
//         // Time of CPA: t = −(r · v) / |v|²
//         double t = -(target.x*target.vx + target.y*target.vy + target.z*target.vz) / v2;
//         det.time_to_cpa = std::max(0.0, t);

//         // CPA position
//         double cx = target.x + target.vx * det.time_to_cpa;
//         double cy = target.y + target.vy * det.time_to_cpa;
//         double cz = target.z + target.vz * det.time_to_cpa;
//         det.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
//     }
// }

// double RadarSignalProcessor::computePk(
//     double range, double radialVelocity) const
// {
//     // Simple engagement model:
//     //   • Exponential decay with range (characteristic scale 45 km)
//     //   • 20 % bonus for closing targets (radialVelocity < 0)
//     return std::min(0.99,
//                     0.95 * std::exp(-range / 45000.0) * (radialVelocity < 0.0 ? 1.2 : 0.8));
// }

// // =============================================================================
// // §F  Range ambiguity
// // =============================================================================

// double RadarSignalProcessor::resolveRangeAmbiguity(
//     double measured, double predicted, double Rmax) const
// {
//     if (Rmax < 1.0) return measured;

//     // Search k ∈ [−5, 5] to find the fold that best matches the prediction
//     double best   = measured;
//     double minErr = 1e12;
//     for (int k = -5; k <= 5; ++k)
//     {
//         double cand = measured + k * Rmax;
//         double err  = std::abs(cand - predicted);
//         if (err < minErr) { minErr = err; best = cand; }
//     }
//     return best;
// }

// void RadarSignalProcessor::applyRangeAmbiguity(
//     DetectionOutput& det, double range,
//     double maxUnambiguousRange,
//     std::normal_distribution<double>& rangeNoise) const
// {
//     if (range > maxUnambiguousRange)
//     {
//         // Range folds back into [0, Rmax) — report is ambiguous
//         det.range       = std::fmod(range, maxUnambiguousRange) + rangeNoise(tl_rng);
//         det.isAmbiguous = true;
//     }
//     else
//     {
//         det.range       = range + rangeNoise(tl_rng);
//         det.isAmbiguous = false;
//     }
// }

// void RadarSignalProcessor::resolveRangeForLockOn(
//     DetectionOutput& det, double range,
//     double maxUnambiguousRange,
//     uint32_t targetId,
//     const std::vector<TrackFile>& trackDatabase) const
// {
//     // Find the predicted range from the Kalman track for this target.
//     // Falls back to the true range if no track exists yet.
//     double predicted = range;
//     for (const auto& t : trackDatabase)
//         if (t.id == targetId) { predicted = t.predictedRange; break; }

//     // Unfold measured range using the prediction, then clear ambiguity flag
//     det.range       = resolveRangeAmbiguity(det.range, predicted, maxUnambiguousRange);
//     det.isAmbiguous = false;
// }

// // =============================================================================
// // §G  Maximum detection range
// // =============================================================================

// double RadarSignalProcessor::computeMaxDetectionRange(
//     double rcs, const RadarConfig& cfg) const
// {
//     double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;
//     double Pt     = cfg.emissionPower_kW * 1000.0;
//     double G      = std::pow(10.0, cfg.antennaGain / 10.0);
//     double Pn     = computeNoisePower(cfg);

//     // CFAR multiplier α for N = 16 reference cells
//     double N     = 16.0;
//     double alpha = N * (std::pow(cfg.targetPfa, -1.0 / N) - 1.0);

//     // Iterative solution: start with a 50 km estimate and converge in ~5 steps.
//     // Each iteration updates the clutter and jamming contributions, which depend
//     // on range, making a direct closed-form solution impractical.
//     double R_est = 50000.0;
//     for (int i = 0; i < 5; ++i)
//     {
//         // Worst-case clutter at current estimate
//         // Worst-case clutter at current estimate
//         double Pc   = std::max(computeClutterPower(R_est, SurfaceType::SEA,  cfg),
//                              computeClutterPower(R_est, SurfaceType::LAND, cfg));
//         double Pj   = 0.0;  // Jammer is per-target, not global — no penalty on display range
//         double Pnt  = Pn + Pc + Pj;
//         // Effective transmit power after two-way propagation loss
//         double prop = computePropagationLoss(R_est, cfg);
//         double Pt_e = Pt * prop * prop; // two-way

//         // Radar range equation solved for R:
//         //   R = ( Pt·G²·λ²·σ / ((4π)³·Pnt·α) )^(1/4)
//         // Apply a 0.6 RCS factor to approximate a non-favourable aspect
//         double num = Pt_e * G * G * lambda * lambda * (rcs * 0.6);
//         double den = std::pow(4.0 * M_PI, 3) * Pnt * alpha;
//         if (den <= 0.0) break;
//         R_est = std::pow(num / den, 0.25);
//     }

//     // Horizon constraint: maximum line-of-sight range in km
//     double Re      = 6371000.0 * cfg.earthRadiusFactor * cfg.atmosphericFactor;
//     double horizon = std::sqrt(2.0 * Re * cfg.radarHeight) / 1000.0; // km

//     double R_km = std::min(R_est / 1000.0, horizon);

//     // Floor at twice the minimum detectable range
//     return std::max(R_km, cfg.minDetectableRange / 1000.0 * 2.0);
// }

// // =============================================================================
// // §H  Detection merge guard
// // =============================================================================

// bool RadarSignalProcessor::shouldMergeDetection(
//     const DetectionOutput& det,
//     const std::vector<DetectionOutput>& existing,
//     const RadarConfig& cfg) const
// {
//     for (const auto& ex : existing)
//     {
//         double azDiff = std::abs(ex.azimuth - det.azimuth);
//         if (azDiff > 180.0) azDiff = 360.0 - azDiff;

//         // Merge if within 150 m range AND within one beam width in both angles
//         if (std::abs(ex.range - det.range) < MERGE_RANGE_GATE &&
//             azDiff  < cfg.beamWidth &&
//             std::abs(ex.elevation - det.elevation) < cfg.beamWidth)
//             return true;
//     }
//     return false;
// }
