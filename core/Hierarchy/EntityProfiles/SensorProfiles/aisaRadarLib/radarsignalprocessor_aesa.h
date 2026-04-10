#pragma once
#ifndef RADARSIGNALPROCESSOR_AESA_H
#define RADARSIGNALPROCESSOR_AESA_H
// =============================================================================
// radarsignalprocessor_aesa.h  —  Rev 3
// All FIX-01..11 physics methods declared here.
// =============================================================================

#include "radarmodel_aesa.h"
#include <random>
#include <utility>
#include <vector>

namespace aesa {

class RadarSignalProcessor_AESA
{
public:
    RadarSignalProcessor_AESA() = default;

    // ---- Geometry -----------------------------------------------------------
    bool   isTargetInBeam(double beamAz, double beamEl,
                          double targetAz, double targetEl,
                          const RadarConfig& cfg,
                          double& outAzDiff, double& outElDiff,
                          double effectiveBeamWidth = -1.0) const;

    bool   checkHorizon(double range, double targetZ,
                        const RadarConfig& cfg) const;

    // ---- Signal chain -------------------------------------------------------
    double calculateSignalStrength(double range, double rcs,
                                   double arrayGain,
                                   const BeamWaveform& waveform,
                                   const RadarConfig& cfg) const;

    double computeNoisePower    (const RadarConfig& cfg, double bandwidth_Hz) const;
    double computeClutterPower  (double range, SurfaceType surface,
                                 const RadarConfig& cfg) const;
    double computeJammerPower   (double targetRange_m, const TargetInput& target,
                                 const RadarConfig& cfg) const;
    double computePropagationLoss(double range_m, const RadarConfig& cfg) const;

    double computeSINR(double receivedPower, double range,
                       SurfaceType surface, const TargetInput& target,
                       const RadarConfig& cfg, const BeamWaveform& waveform) const;

    // ---- CFAR ---------------------------------------------------------------
    std::vector<double> generateReferenceCells(SurfaceType surface,
                                               const RadarConfig& cfg) const;
    double computeCFARThreshold        (const std::vector<double>& cells,
                                        const RadarConfig& cfg) const;
    double computeCFARThresholdRelaxed (const std::vector<double>& cells,
                                        const RadarConfig& cfg) const;

    // ---- RCS / Swerling  (FIX-07) ------------------------------------------
    double computeEffectiveRCS(const TargetInput& target,
                               double range,
                               double frequency_Hz) const;
    //double computeEffectiveRCS(const TargetInput& target, double range) const;
    double computeSwerlingRCS (double nominalRCS, SwerlingCase sc,
                               bool coherentDwell) const;

    // ---- Target motion ------------------------------------------------------
    void   computeTargetMotionParams(DetectionOutput& det,
                                     const TargetInput& target,
                                     double range) const;
    double computeRadialVelocity(const TargetInput& target, double range,
                                 std::normal_distribution<double>& noise) const;
    void   computeCPA(DetectionOutput& det, const TargetInput& target,
                      double range) const;

    // ---- FIX-07  Albersheim Pd + Pk replacement ----------------------------
    double computeAlbersheimPd(double snr_linear, double Pfa, int N,
                                SwerlingCase sc) const;
    double computePk           (double sinr_linear, double Pfa, int N,
                                SwerlingCase sc = SwerlingCase::CASE_I) const;

    // ---- Range ambiguity ----------------------------------------------------
   // void   applyRangeAmbiguity(DetectionOutput& det, double range, double Rmax,
                               //std::normal_distribution<double>& noise) const;
    void   applyRangeAmbiguity(DetectionOutput& det, double range,
                             double Rmax, double Rmax2,
                             std::normal_distribution<double>& noise) const;
    void   resolveRangeForLockOn(DetectionOutput& det, double range, double Rmax,
                                 uint32_t targetId,
                                 const std::vector<TrackFile>& db) const;
   // double resolveRangeAmbiguity(double measured, double predicted, double Rmax) const;
    double resolveRangeAmbiguity(double measured, double predicted, double Rmax) const;

    // Staggered PRF resolvers — only called when wf.prf2_Hz > 0
    double resolveRangeAmbiguityStaggered(double measured1, double measured2,
                                          double Rmax1,     double Rmax2,
                                          double predicted) const;

    double resolveVelocityStaggered(double foldedVel1, double foldedVel2,
                                    double Vmax1,      double Vmax2,
                                    double predictedVel) const;
    // ---- Max detection range -----------------------------------------------
    double computeMaxDetectionRange(double rcs, const RadarConfig& cfg) const;

    // ---- Detection merge guard ---------------------------------------------
    bool shouldMergeDetection(const DetectionOutput& det,
                              const std::vector<DetectionOutput>& existing,
                              const RadarConfig& cfg) const;

    // ---- Beam gain + FIX-11 sidelobe blanking ------------------------------
    double computeBeamGainFactor(double azDiff, double elDiff,
                                 const RadarConfig& cfg,
                                 double effectiveBeamWidth = -1.0) const;

    bool   isJammerInSidelobe(double azDiff, double elDiff,
                              const TargetInput& target,
                              const RadarConfig& cfg) const;

    // ---- Modulation processing gain ----------------------------------------
    double computeModulationProcessingGain(const BeamWaveform& waveform) const;

    // ---- FIX-01  Doppler clutter notch -------------------------------------
    bool isInDopplerBlindZone(double radVel_m_s,
                              const RadarConfig& cfg,
                              const BeamWaveform& waveform) const;
    std::pair<double,double> computeClutterNotch(const RadarConfig& cfg,
                                                  const BeamWaveform& wf) const;

    // ---- FIX-02  Monopulse angle error -------------------------------------
    void computeMonopulseAngleError(double azDiff_deg, double elDiff_deg,
                                    double sinr, const RadarConfig& cfg,
                                    double& outAzError_deg,
                                    double& outElError_deg) const;

    // ---- FIX-06  Range-based waveform selection ----------------------------
    BeamWaveform selectWaveformForRange(double range_m,
                                        const RadarConfig& cfg) const;

    // ---- FIX-09  Two-ray multipath factor ----------------------------------
    double computeMultipathFactor(double range_m, double elevation_deg,
                                  double targetHeight_m,
                                  const RadarConfig& cfg) const;

    // ---- FIX-10  Chaff return ----------------------------------------------
    double computeChaffReturn(double beamAz, double beamEl,
                              const std::vector<ChaffCloud>& clouds,
                              double simTime,
                              const RadarConfig& cfg) const;
    // ADD to RadarSignalProcessor_AESA class public section:
    double lookupAspectRCS(const TargetInput& target, double aspectAngle_deg) const;
    double getPlatformBaseRCS(const std::string& platformType) const;
    // ADD — MTI/STAP improvement factor
    double computeSTAPGain(double radialVelocity_m_s,
                           double platformSpeed_m_s,
                           const BeamWaveform& wf,
                           const RadarConfig& cfg) const;

    bool isInClutterNotchSTAP(double radVel_m_s,
                              const RadarConfig& cfg,
                              const BeamWaveform& wf) const;
    double computeWaterVapourDensity(const AtmosphericConditions& atm) const;

    double computeGaseousAttenuation(double frequency_Hz,
                                     const AtmosphericConditions& atm,
                                     double range_m) const;
};

} // namespace aesa
#endif
