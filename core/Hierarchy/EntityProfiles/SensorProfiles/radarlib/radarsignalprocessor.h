#pragma once
// =============================================================================
// radarsignalprocessor.h  —  Pure radar physics and signal-chain calculations
//
// All original methods are UNCHANGED in signature and semantics.
// Added:
//   §I  computeBeamGainFactor()       — main-beam vs sidelobe gain factor
//   §J  computeModulationProcessingGain() — pulse-compression / waveform gain
//
// These two methods are called from RadarModel::processTargetDetection() to
// apply sidelobe attenuation and modulation processing gain without touching
// any existing call sites.
// =============================================================================

#ifndef RADARSIGNALPROCESSOR_H
#define RADARSIGNALPROCESSOR_H

#include "radarmodel.h"
#include <random>
#include <vector>

class RadarSignalProcessor
{
public:
    RadarSignalProcessor() = default;

    // -------------------------------------------------------------------------
    // §A  Beam / horizon geometry  (unchanged)
    // -------------------------------------------------------------------------
    bool isTargetInBeam(double currentAzimuth,
                        double currentElevation,
                        double targetAz,
                        double targetEl,
                        double dt,
                        const RadarConfig& cfg,
                        double& outAzDiff,
                        double& outElDiff,
                        double& outScanMargin) const;

    bool checkHorizon(double range,
                      double targetZ,
                      const RadarConfig& cfg) const;

    // -------------------------------------------------------------------------
    // §B  Signal chain  (unchanged)
    // -------------------------------------------------------------------------
    double calculateSignalStrength(double range,
                                   double rcs,
                                   const RadarConfig& cfg) const;

    double computeNoisePower(const RadarConfig& cfg) const;

    double computeClutterPower(double range,
                               SurfaceType surface,
                               const RadarConfig& cfg) const;

    double computeJammerPower(double targetRange_m,
                              const TargetInput& target,
                              const RadarConfig& cfg) const;

    double computePropagationLoss(double range_m,
                                  const RadarConfig& cfg) const;

    double computeSINR(double receivedPower,
                       double range,
                       SurfaceType surface,
                       const TargetInput& target,
                       const RadarConfig& cfg) const;

    // -------------------------------------------------------------------------
    // §C  CFAR  (unchanged)
    // -------------------------------------------------------------------------
    std::vector<double> generateReferenceCells(SurfaceType surface,
                                               const RadarConfig& cfg) const;

    double computeCFARThreshold(const std::vector<double>& cells,
                                const RadarConfig& cfg) const;

    double computeCFARThresholdRelaxed(const std::vector<double>& cells,
                                       const RadarConfig& cfg) const;

    // -------------------------------------------------------------------------
    // §D  Effective RCS  (unchanged)
    // -------------------------------------------------------------------------
    double computeEffectiveRCS(const TargetInput& target,
                               double range) const;

    // -------------------------------------------------------------------------
    // §E  Target motion  (unchanged)
    // -------------------------------------------------------------------------
    void   computeTargetMotionParams(DetectionOutput& det,
                                   const TargetInput& target,
                                   double range) const;

    double computeRadialVelocity(const TargetInput& target,
                                 double range,
                                 std::normal_distribution<double>& dopplerNoise) const;

    void   computeCPA(DetectionOutput& det,
                    const TargetInput& target,
                    double range) const;

    double computePk(double range, double radialVelocity) const;

    // -------------------------------------------------------------------------
    // §F  Range ambiguity  (unchanged)
    // -------------------------------------------------------------------------
    void   applyRangeAmbiguity(DetectionOutput& det,
                             double range,
                             double maxUnambiguousRange,
                             std::normal_distribution<double>& rangeNoise) const;

    void   resolveRangeForLockOn(DetectionOutput& det,
                               double range,
                               double maxUnambiguousRange,
                               uint32_t targetId,
                               const std::vector<TrackFile>& trackDatabase) const;

    double resolveRangeAmbiguity(double measuredRange,
                                 double predictedRange,
                                 double Rmax) const;

    // -------------------------------------------------------------------------
    // §G  Maximum detection range  (unchanged)
    // -------------------------------------------------------------------------
    double computeMaxDetectionRange(double rcs,
                                    const RadarConfig& cfg) const;

    // -------------------------------------------------------------------------
    // §H  Detection merge guard  (unchanged)
    // -------------------------------------------------------------------------
    bool shouldMergeDetection(const DetectionOutput& det,
                              const std::vector<DetectionOutput>& existing,
                              const RadarConfig& cfg) const;

    // -------------------------------------------------------------------------
    // §I  Beam gain factor  (NEW)
    //
    // Returns a linear gain multiplier [sidelobeLinear, 1.0]:
    //   1.0            — target is inside the main beam (azDiff ≤ beamWidth/2
    //                    AND elDiff ≤ beamWidth/2)
    //   peakSidelobe   — target is in the sidelobe region
    //
    // Call this after isTargetInBeam() returns true.
    // Multiply the effective RCS by this factor before calculateSignalStrength().
    // -------------------------------------------------------------------------
    double computeBeamGainFactor(double azDiff,
                                 double elDiff,
                                 const RadarConfig& cfg) const;

    // -------------------------------------------------------------------------
    // §J  Modulation processing gain  (NEW)
    //
    // Returns the linear signal-processing gain conferred by pulse compression
    // or CW integration.  Multiplied into the numerator of SINR.
    //   NONE / CW   → 1.0   (no gain)
    //   LFM / NLFM  → B·τ   (time-bandwidth product)
    //   FMCW        → B·τ   (same formula, different physical origin)
    // -------------------------------------------------------------------------
    double computeModulationProcessingGain(const RadarConfig& cfg) const;
};

#endif // RADARSIGNALPROCESSOR_H
// #pragma once
// // =============================================================================
// // radarsignalprocessor.h  —  Pure radar physics and signal-chain calculations
// //
// // Responsibility:
// //   Everything that transforms a (range, RCS, config) tuple into a detection
// //   decision.  This covers:
// //     • Received signal power      (radar range equation)
// //     • Noise power                (thermal + noise figure)
// //     • Clutter power              (sea state / land backscatter)
// //     • Jammer power               (self-screening and stand-off)
// //     • Propagation loss           (rain attenuation, fog)
// //     • SINR computation
// //     • CFAR threshold generation
// //     • Maximum detection range    (iterative radar equation)
// //     • Effective RCS              (aspect-angle model)
// //     • Target motion parameters   (speed, heading, aspect, CPA)
// //     • Radial velocity            (Doppler with noise)
// //     • Range ambiguity            (fold / unfold)
// //     • Probability of kill        (simple engagement model)
// //     • Beam / horizon geometry    (isTargetInBeam, checkHorizon)
// //
// // Design rules:
// //   1. STATELESS — no member variables beyond a const ref to RadarConfig.
// //      All methods are const (and most could be static; they take config
// //      by reference so they don't need to be).
// //   2. NO MUTEX — called only from RadarModel::update() which already holds
// //      the master mutex.
// //   3. NO Qt types, no engine types.  Only std C++17 + radarmodel.h.
// //   4. Thread-local RNG is used for noise injection to avoid contention.
// //
// // Adding a new propagation model or detection algorithm = add a method here.
// // Nothing else in the codebase needs to change.
// // =============================================================================

// #ifndef RADARSIGNALPROCESSOR_H
// #define RADARSIGNALPROCESSOR_H

// #include "radarmodel.h"
// #include <random>
// #include <vector>

// class RadarSignalProcessor
// {
// public:
//     // -------------------------------------------------------------------------
//     // Construction
//     // -------------------------------------------------------------------------

//     RadarSignalProcessor() = default;

//     // -------------------------------------------------------------------------
//     // §A  Beam / horizon geometry
//     // -------------------------------------------------------------------------

//     /// Returns true if the target at (targetAz, targetEl) falls within the
//     /// current beam footprint (widened by scan margin for the current dt).
//     ///
//     /// @param currentAzimuth   Current antenna azimuth (degrees)
//     /// @param currentElevation Current antenna elevation (degrees)
//     /// @param targetAz         Target azimuth in radar-local frame (degrees)
//     /// @param targetEl         Target elevation in radar-local frame (degrees)
//     /// @param dt               Tick duration (seconds) — used to compute margin
//     /// @param cfg              Current radar configuration
//     /// @param outAzDiff        [out] Azimuth separation in degrees
//     /// @param outElDiff        [out] Elevation separation in degrees
//     /// @param outScanMargin    [out] Total beam margin applied (degrees)
//     bool isTargetInBeam(double currentAzimuth,
//                         double currentElevation,
//                         double targetAz,
//                         double targetEl,
//                         double dt,
//                         const RadarConfig& cfg,
//                         double& outAzDiff,
//                         double& outElDiff,
//                         double& outScanMargin) const;

//     /// Returns true if the target is within radar line-of-sight.
//     /// Uses the 4/3 Earth effective-radius model adjusted by atmosphericFactor.
//     ///
//     /// @param range    Slant range to target (metres)
//     /// @param targetZ  Target altitude (metres above sea level)
//     /// @param cfg      Current radar configuration (provides radarHeight, factors)
//     bool checkHorizon(double range,
//                       double targetZ,
//                       const RadarConfig& cfg) const;

//     // -------------------------------------------------------------------------
//     // §B  Signal chain — received power and interference
//     // -------------------------------------------------------------------------

//     /// Radar range equation — returns received signal power (Watts).
//     /// Accounts for frequency agility if enabled in cfg.
//     ///
//     /// @param range  Slant range (metres, clamped to ≥ 1 m internally)
//     /// @param rcs    Effective RCS in m² (already aspect-corrected)
//     /// @param cfg    Current radar configuration
//     double calculateSignalStrength(double range,
//                                    double rcs,
//                                    const RadarConfig& cfg) const;

//     /// Thermal noise power (Watts) = k·T·B·F
//     double computeNoisePower(const RadarConfig& cfg) const;

//     /// Sea / land clutter power (Watts).  Returns 0 for AIR targets.
//     /// Applies exponential fluctuation (Swerling I model via thread-local RNG).
//     double computeClutterPower(double range,
//                                SurfaceType surface,
//                                const RadarConfig& cfg) const;

//     /// Jammer power received at the radar (Watts).
//     /// Handles both self-screening (jammer on target) and stand-off geometries.
//     double computeJammerPower(double targetRange_m,
//                               const TargetInput& target,
//                               const RadarConfig& cfg) const;

//     /// Multiplicative propagation loss factor [0,1] — rain and fog attenuation.
//     /// Returns 1.0 (no loss) when rainRate and fogVisibility are both 0.
//     double computePropagationLoss(double range_m,
//                                   const RadarConfig& cfg) const;

//     /// SINR = Pr / (Pn + Pc + Pj).
//     /// Calls computeNoisePower, computeClutterPower, computeJammerPower
//     /// internally — caller only needs the received power and range.
//     double computeSINR(double receivedPower,
//                        double range,
//                        SurfaceType surface,
//                        const TargetInput& target,
//                        const RadarConfig& cfg) const;

//     // -------------------------------------------------------------------------
//     // §C  CFAR detection threshold
//     // -------------------------------------------------------------------------

//     /// Generate 16 reference cells for CA-CFAR (exponential fluctuation).
//     /// Sea / land clutter scaling applied via cfg.
//     std::vector<double> generateReferenceCells(SurfaceType surface,
//                                                const RadarConfig& cfg) const;

//     /// Standard CA-CFAR threshold — used for moving targets.
//     double computeCFARThreshold(const std::vector<double>& cells,
//                                 const RadarConfig& cfg) const;

//     /// Relaxed CA-CFAR threshold — applied when radial velocity < 5 m/s
//     /// (near-stationary / slow-movers are harder to detect above clutter).
//     double computeCFARThresholdRelaxed(const std::vector<double>& cells,
//                                        const RadarConfig& cfg) const;

//     // -------------------------------------------------------------------------
//     // §D  Effective RCS
//     // -------------------------------------------------------------------------

//     /// Aspect-angle correction on the nominal RCS.
//     /// Stationary targets get a 40 % reduction (no Doppler discrimination).
//     /// Moving targets are weighted by |sin(aspect)| — broad-side = full RCS.
//     double computeEffectiveRCS(const TargetInput& target,
//                                double range) const;

//     // -------------------------------------------------------------------------
//     // §E  Target motion parameters
//     // -------------------------------------------------------------------------

//     /// Fill speed-over-ground, heading, and target-aspect fields in det.
//     /// Does NOT touch range or azimuth — caller fills those separately.
//     void computeTargetMotionParams(DetectionOutput& det,
//                                    const TargetInput& target,
//                                    double range) const;

//     /// Compute radial velocity (Doppler) with optional noise injection.
//     /// dot(v, r̂) / |r| + noise sample.
//     double computeRadialVelocity(const TargetInput& target,
//                                  double range,
//                                  std::normal_distribution<double>& dopplerNoise) const;

//     /// Fill cpa_distance and time_to_cpa fields in det.
//     void computeCPA(DetectionOutput& det,
//                     const TargetInput& target,
//                     double range) const;

//     /// Simple probability-of-kill model: 0.95 · exp(−R/45 km) × aspect factor.
//     double computePk(double range, double radialVelocity) const;

//     // -------------------------------------------------------------------------
//     // §F  Range ambiguity
//     // -------------------------------------------------------------------------

//     /// Fold an unambiguous range into the PRF interval [0, Rmax).
//     /// Sets det.isAmbiguous and adds range noise.
//     void applyRangeAmbiguity(DetectionOutput& det,
//                              double range,
//                              double maxUnambiguousRange,
//                              std::normal_distribution<double>& rangeNoise) const;

//     /// Unfold det.range using a predicted range from the track database.
//     /// Used in LOCK_ON mode to remove the ambiguity flag.
//     ///
//     /// @param trackDatabase  Read-only view of tracks to find predictedRange
//     void resolveRangeForLockOn(DetectionOutput& det,
//                                double range,
//                                double maxUnambiguousRange,
//                                uint32_t targetId,
//                                const std::vector<TrackFile>& trackDatabase) const;

//     /// Unfold a measured range by finding the k·Rmax offset that minimises
//     /// |measured + k·Rmax − predicted|.  Pure utility, no side-effects.
//     double resolveRangeAmbiguity(double measuredRange,
//                                  double predictedRange,
//                                  double Rmax) const;

//     // -------------------------------------------------------------------------
//     // §G  Maximum detection range
//     // -------------------------------------------------------------------------

//     /// Iterative radar range equation — converges in ~5 steps.
//     /// Accounts for current clutter, jamming, propagation, and RCS.
//     /// Caller must already hold RadarModel::mutex_.
//     double computeMaxDetectionRange(double rcs,
//                                     const RadarConfig& cfg) const;

//     // -------------------------------------------------------------------------
//     // §H  Detection merge guard
//     // -------------------------------------------------------------------------

//     /// Returns true if det is close enough to an existing detection that it
//     /// should be suppressed (two platforms very close together).
//     bool shouldMergeDetection(const DetectionOutput& det,
//                               const std::vector<DetectionOutput>& existing,
//                               const RadarConfig& cfg) const;
// };

// #endif // RADARSIGNALPROCESSOR_H
