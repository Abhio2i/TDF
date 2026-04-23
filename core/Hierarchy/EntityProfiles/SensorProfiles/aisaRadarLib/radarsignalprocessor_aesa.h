// =============================================================================
// FILE:         radarsignalprocessor_aesa.h
// MODULE:       AESA Radar Signal Processor
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Declares the RadarSignalProcessor_AESA class which implements
//               all physics-layer signal processing for the AESA radar model.
//               This class contains no state — all methods are pure const
//               computations. It is owned by RadarModel_AESA and called from
//               within update() while the model mutex is held.
//
//               Subsystems implemented:
//                 §A  Geometry    — beam gate, horizon check
//                 §B  Signal chain — radar equation, noise, clutter, jammer,
//                                   propagation loss (rain, fog, gaseous)
//                 §C  CFAR        — CA-CFAR threshold generation
//                 §D  RCS         — Physical Optics 6-facet model, Swerling
//                 §E  Target motion — kinematic parameters, CPA, Albersheim Pd
//                 §F  Range ambiguity — staggered PRF coincidence detector
//                 §G  Max detection range — iterative radar equation solver
//                 §H  Detection merge guard
//                 §I  Beam gain + sidelobe blanking
//                 §J  Modulation processing gain
//                 §K  Doppler / STAP — clutter notch, STAP gain
//                 §L  Monopulse angle refinement
//                 §M  Waveform selection
//                 §N  Two-ray multipath
//                 §O  Chaff return
//                 §P  Atmospheric propagation (ITU-R P.676-12, P.838-3, Kunkel)
//
// THREAD SAFETY: All methods are const and stateless. Thread-safe for
//               concurrent reads. The thread_local RNG in the .cpp file
//               is per-thread — no shared mutable state.
//
// REQUIREMENTS: REQ-AESA-040  Detection pipeline physics
//               REQ-AESA-021  Staggered PRF ambiguity resolution
//               REQ-AESA-060  Electronic warfare (jammer, chaff)
//               REQ-AESA-071  Propagation loss (ITU-R P.676-12 / P.838-3)
//               REQ-AESA-072  Two-ray multipath
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-SP-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation.
//   Rev 2  15 Feb 2026  FIX-01 through FIX-11 applied.
//   Rev 3  01 Apr 2026  STAP gain, staggered PRF resolvers, Physical Optics
//                       RCS, ITU-R P.676-12 gaseous attenuation added.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#pragma once
#ifndef RADARSIGNALPROCESSOR_AESA_H
#define RADARSIGNALPROCESSOR_AESA_H

#include "radarmodel_aesa.h"
#include <random>
#include <utility>
#include <vector>

namespace aesa {

// =============================================================================
// CLASS: RadarSignalProcessor_AESA
//
// DESCRIPTION:  Stateless physics engine for the AESA radar model.
//               All methods are const — no member variables, no side effects
//               except through output parameters explicitly documented.
//               The thread_local RNG in the .cpp is the only shared state,
//               and it is per-thread so no synchronisation is required.
//
// REQUIREMENTS: REQ-AESA-040, REQ-AESA-021, REQ-AESA-060,
//               REQ-AESA-071, REQ-AESA-072
//
// TRACEABILITY:
//   Test suite:  radarSignalProcessor_test
//                (radarsignalprocessor_aesa_test.cpp)
//   Test cases:  TC-AESA-SP-001 through TC-AESA-SP-050
// =============================================================================
class RadarSignalProcessor_AESA
{
public:

    // =========================================================================
    // CONSTRUCTOR
    // =========================================================================

    // =========================================================================
    // FUNCTION:    RadarSignalProcessor_AESA (default constructor)
    // DESCRIPTION: No resources acquired. Stateless class. REQ-AESA-040.
    // =========================================================================
    RadarSignalProcessor_AESA() = default;

    // =========================================================================
    // §A  GEOMETRY
    // REQ-AESA-010, REQ-AESA-040, REQ-AESA-071
    // =========================================================================

    // =========================================================================
    // FUNCTION:    isTargetInBeam
    //
    // DESCRIPTION: Determines whether a target at (targetAz, targetEl) falls
    //              within the current beam gate centred at (beamAz, beamEl).
    //              The gate width is 2.5 * effectiveBeamWidth in both azimuth
    //              and elevation. Also computes the absolute angular differences
    //              azDiff and elDiff for downstream gain and monopulse use.
    //
    //              Azimuth difference is computed with wrap-around protection:
    //              if |beamAz - targetAz| > 180 deg, the shorter arc is used.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   beamAz/beamEl       [in]  Current beam pointing (degrees, body frame).
    //   targetAz/targetEl   [in]  Target angular position (degrees, body frame).
    //   cfg                 [in]  Uses cfg.beamWidth as fallback if
    //                             effectiveBeamWidth <= 0.
    //   outAzDiff           [out] Unsigned azimuth difference (degrees, [0, 180]).
    //   outElDiff           [out] Unsigned elevation difference (degrees).
    //   effectiveBeamWidth  [in]  Spoiled beamwidth (degrees). Use -1.0 or 0.0
    //                             to fall back to cfg.beamWidth. Default: -1.0.
    //
    // RETURNS:    true  = target is within the beam gate.
    //             false = target is outside the beam gate.
    //
    // SIDE EFFECTS: Writes outAzDiff, outElDiff.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-001  Boresight target within beam (az=0, el=0)
    //   TC-AESA-SP-002  Target beyond gate rejected
    //   TC-AESA-SP-003  Azimuth wrap-around handled correctly
    // =========================================================================
    bool isTargetInBeam(double beamAz, double beamEl,
                        double targetAz, double targetEl,
                        const RadarConfig& cfg,
                        double& outAzDiff, double& outElDiff,
                        double effectiveBeamWidth = -1.0) const;

    // =========================================================================
    // FUNCTION:    checkHorizon
    //
    // DESCRIPTION: Returns true if a target at slant range (range) and altitude
    //              (targetZ) is within the combined radar + target horizon using
    //              the 4/3 earth radius model (standard atmospheric refraction).
    //
    //              Horizon check: range <= dRadar + dTarget
    //              where dRadar = sqrt(2 * Re * radarHeight)
    //                    dTarget = sqrt(2 * Re * targetZ)
    //                    Re = 6371000 * earthRadiusFactor * atmosphericFactor
    //
    // REQUIREMENT: REQ-AESA-071
    //
    // PARAMETERS:
    //   range    [in]  Slant range to target (metres).
    //   targetZ  [in]  Target altitude (metres). Negative values clamped to 0.
    //   cfg      [in]  Uses radarHeight, earthRadiusFactor, atmosphericFactor.
    //
    // RETURNS:    true = target is within combined horizon.
    //             false = target is beyond horizon — reject from pipeline.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-004  Target at 0 altitude always within radar horizon
    //   TC-AESA-SP-005  Target beyond combined horizon returns false
    // =========================================================================
    bool checkHorizon(double range, double targetZ,
                      const RadarConfig& cfg) const;

    // =========================================================================
    // §B  SIGNAL CHAIN
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    calculateSignalStrength
    //
    // DESCRIPTION: Computes received signal power (Watts) using the radar range
    //              equation including array gain, wavelength, RCS, and
    //              propagation loss. Applies pulse-to-pulse frequency hopping
    //              for LPI operation when frequencyAgility is enabled.
    //
    //              Pr = (Pt * G^2 * lambda^2 * rcs) / ((4pi)^3 * R^4) * L_prop
    //
    //              where Pt = numActiveElements * peakPowerPerElement * efficiency.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   range     [in]  Slant range (metres). Clamped to 1.0 minimum.
    //   rcs       [in]  Effective RCS after all adjustments (m²).
    //   arrayGain [in]  Total array gain (linear) from computeArrayGain() *
    //                   computeBeamGainFactor(). REQ-AESA-012.
    //   waveform  [in]  Not used in computation — retained for API consistency
    //                   with future LFM-specific power models. REQ-AESA-020.
    //   cfg       [in]  Array and antenna parameters. REQ-AESA-040.
    //
    // RETURNS:    Received signal power (Watts). Returns 0.0 if result is
    //             negative (guards against numerical errors). REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-006  Signal strength decreases with range^4
    //   TC-AESA-SP-007  Signal strength proportional to RCS
    // =========================================================================
    double calculateSignalStrength(double range, double rcs,
                                   double arrayGain,
                                   const BeamWaveform& waveform,
                                   const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    computeNoisePower
    //
    // DESCRIPTION: Computes receiver thermal noise power (Watts).
    //              Pn = k * T * B * F
    //              where k = Boltzmann constant (1.380649e-23 J/K)
    //                    T = systemTemperature_K
    //                    B = bandwidth_Hz (clamped to >= 1 Hz)
    //                    F = 10^(noiseFigure_dB/10)  (linear noise figure)
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   cfg           [in]  Uses systemTemperature_K, noiseFigure_dB.
    //   bandwidth_Hz  [in]  Instantaneous receiver bandwidth (Hz).
    //                       Clamped to 1.0 minimum to prevent zero noise power.
    //
    // RETURNS:    Noise power (Watts, > 0). REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-008  Noise power > 0 for any valid config
    //   TC-AESA-SP-009  Noise power increases with bandwidth
    // =========================================================================
    double computeNoisePower(const RadarConfig& cfg, double bandwidth_Hz) const;

    // =========================================================================
    // FUNCTION:    computeClutterPower
    //
    // DESCRIPTION: Computes surface clutter power (Watts) using:
    //              - GIT sea clutter model (Horst et al. 1978) for SEA surface.
    //              - Billingsley low-relief terrain model for LAND surface.
    //              Returns 0.0 for AIR surface or range < 1 m.
    //              Applies exponential fluctuation (Rayleigh amplitude target).
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   range    [in]  Slant range (metres).
    //   surface  [in]  Target surface type. AIR returns 0.0 immediately.
    //   cfg      [in]  Uses seaState, landClutter, frequency_Hz, beamWidth,
    //                  radarHeight, searchWaveform.pulseWidth_s.
    //
    // RETURNS:    Clutter power (Watts). Includes random fluctuation.
    //             REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-010  AIR surface returns zero clutter
    //   TC-AESA-SP-011  SEA clutter increases with sea state
    // =========================================================================
    double computeClutterPower(double range, SurfaceType surface,
                               const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    computeJammerPower
    //
    // DESCRIPTION: Computes jammer power received at the radar antenna (Watts)
    //              using the one-way jamming equation:
    //              Pj_rx = (Pj * Gj * Gr * lambda^2) / ((4pi)^2 * Rj^2)
    //              Scaled by bandwidth efficiency ratio B_receiver / B_jammer.
    //              Returns 0.0 if jammer is inactive or has zero power.
    //
    // REQUIREMENT: REQ-AESA-040, REQ-AESA-060
    //
    // PARAMETERS:
    //   targetRange_m [in]  Range to the target platform (metres). Used when
    //                       jammer is self-screening.
    //   target        [in]  Contains jammer configuration. REQ-AESA-060.
    //   cfg           [in]  Uses antennaGain, frequency_Hz, antennaBandwidth.
    //
    // RETURNS:    Jammer power at radar receiver (Watts). REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-012  Inactive jammer returns 0.0
    //   TC-AESA-SP-013  Jammer power decreases with range^2
    // =========================================================================
    double computeJammerPower(double targetRange_m,
                              const TargetInput& target,
                              const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    computePropagationLoss
    //
    // DESCRIPTION: Computes two-way path propagation loss as a linear power
    //              reduction factor in (0.0, 1.0]. Combines:
    //                - Rain attenuation (ITU-R P.838-3)
    //                - Fog attenuation (Kunkel 1984)
    //                - Gaseous absorption O2 + H2O (ITU-R P.676-12 Annex 2)
    //
    //              Returns pow(10, -total_loss_dB / 10).
    //              A return of 1.0 means no propagation loss (clear air).
    //
    // REQUIREMENT: REQ-AESA-071
    //
    // PARAMETERS:
    //   range_m  [in]  One-way path length (metres).
    //   cfg      [in]  Uses atmosphere struct (rain, fog, temperature, humidity,
    //                  pressure) and frequency_Hz. REQ-AESA-071.
    //
    // RETURNS:    Linear propagation loss factor (0.0, 1.0]. REQ-AESA-071.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-014  Clear air returns value close to 1.0
    //   TC-AESA-SP-015  Heavy rain reduces propagation factor
    // =========================================================================
    double computePropagationLoss(double range_m,
                                  const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    computeSINR
    //
    // DESCRIPTION: Computes signal-to-interference-plus-noise ratio (linear).
    //              SINR = (Pr * pg * ig) / (Pn + Pc + Pj * jSuppress)
    //              where pg = modulation processing gain
    //                    ig = pulse integration gain (pulsesPerDwell)
    //                    jSuppress = null steering suppression factor
    //
    //              Null steering: if cfg.nullSteering.active and jammer is
    //              within the null cone, jammer power is multiplied by
    //              10^(nullDepth_dB/10) before entering the denominator.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   receivedPower [in]  Signal power from calculateSignalStrength() (W).
    //   range         [in]  Slant range (metres).
    //   surface       [in]  Target surface type for clutter model.
    //   target        [in]  Full target descriptor for jammer config.
    //   cfg           [in]  Full radar config.
    //   waveform      [in]  Current beam waveform.
    //
    // RETURNS:    SINR (linear, dimensionless). Clamped to >= 0. REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-016  SINR > 0 for valid signal and noise
    //   TC-AESA-SP-017  SINR decreases when jammer is active
    // =========================================================================
    double computeSINR(double receivedPower, double range,
                       SurfaceType surface, const TargetInput& target,
                       const RadarConfig& cfg,
                       const BeamWaveform& waveform) const;

    // =========================================================================
    // §C  CFAR
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    generateReferenceCells
    //
    // DESCRIPTION: Generates 16 CFAR reference cell power estimates using an
    //              exponential distribution (models Rayleigh amplitude clutter).
    //              Cell values are scaled by surface clutter factor:
    //                SEA:  cells * (1 + seaState * 0.3)
    //                LAND: cells * (1 + landClutter * 0.5)
    //              This models the higher variance of surface clutter versus
    //              thermal noise. REQ-AESA-040.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   surface  [in]  Target surface type. Determines cell scaling.
    //   cfg      [in]  Uses seaState, landClutter.
    //
    // RETURNS:    Vector of 16 reference cell power estimates. REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-018  Returns exactly 16 cells
    //   TC-AESA-SP-019  SEA cells > AIR cells on average
    // =========================================================================
    std::vector<double> generateReferenceCells(SurfaceType surface,
                                               const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    computeCFARThreshold
    //
    // DESCRIPTION: Computes CA-CFAR (Cell-Averaging CFAR) detection threshold
    //              for the configured false alarm probability.
    //              threshold = mean_cells * alpha
    //              alpha = N * (Pfa^(-1/N) - 1)  (CA-CFAR multiplier)
    //              Returns 1e12 (effectively infinite) if cells is empty.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   cells  [in]  Reference cells from generateReferenceCells().
    //   cfg    [in]  Uses targetPfa.
    //
    // RETURNS:    CA-CFAR threshold. SINR must exceed this for detection.
    //             REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-020  Threshold > 0 for any valid cells and Pfa
    //   TC-AESA-SP-021  Empty cells returns 1e12
    // =========================================================================
    double computeCFARThreshold(const std::vector<double>& cells,
                                const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    computeCFARThresholdRelaxed
    //
    // DESCRIPTION: Same as computeCFARThreshold() but uses a relaxed Pfa of
    //              min(1e-4, targetPfa * 100). Used for slow-moving or
    //              stationary targets near the clutter notch where residual
    //              clutter increases false alarm rate. The higher Pfa (less
    //              strict) threshold prevents false alarms from clutter
    //              without completely suppressing the target. REQ-AESA-040.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:  Same as computeCFARThreshold().
    // RETURNS:     Relaxed CA-CFAR threshold. REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-022  Relaxed threshold < standard threshold for same cells
    // =========================================================================
    double computeCFARThresholdRelaxed(const std::vector<double>& cells,
                                       const RadarConfig& cfg) const;

    // =========================================================================
    // §D  RCS AND SWERLING FLUCTUATION
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeEffectiveRCS
    //
    // DESCRIPTION: Computes the effective RCS of a target (m²) combining:
    //              1. Physical Optics 6-facet box model (when dimensions.valid).
    //                 Models aspect-angle, frequency, material, and shape.
    //              2. Aspect-angle table interpolation (when rcsTable non-empty).
    //              3. Platform type base RCS lookup (fallback).
    //              Then applies Swerling fluctuation (temporal decorrelation).
    //
    //              Physical Optics model references:
    //              [1] Knott et al — Radar Cross Section, Ch 4-5
    //              [2] Ruck et al — RCS Handbook, Ch 3
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   target        [in]  Full target descriptor.
    //   range         [in]  Slant range for LOS unit vector computation (m).
    //   frequency_Hz  [in]  Radar frequency for wavelength and regime selection.
    //
    // RETURNS:    Effective RCS after all adjustments and Swerling fluctuation
    //             (m²). Returns 0.0 for nominalRCS <= 0. REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-023  GENERIC target returns getPlatformBaseRCS() value
    //   TC-AESA-SP-024  STEALTHY material reduces RCS vs METAL
    //   TC-AESA-SP-025  CASE_0 Swerling returns deterministic value
    // =========================================================================
    double computeEffectiveRCS(const TargetInput& target,
                               double range,
                               double frequency_Hz) const;

    // =========================================================================
    // FUNCTION:    computeSwerlingRCS
    //
    // DESCRIPTION: Applies Swerling RCS fluctuation model to a nominal RCS
    //              value to produce one random realisation.
    //
    //              CASE_0:   no fluctuation — returns nominalRCS exactly.
    //              CASE_I/II: exponential distribution (chi-squared, 2 DOF).
    //                         Models many small independent scatterers.
    //              CASE_III/IV: sum of two exponentials (chi-squared, 4 DOF).
    //                         Models one dominant scatterer plus many small.
    //
    //              Returns 0.0 for nominalRCS <= 0. REQ-AESA-040.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   nominalRCS     [in]  Mean RCS (m²). REQ-AESA-040.
    //   sc             [in]  Swerling case. REQ-AESA-040.
    //   coherentDwell  [in]  Reserved for future use — not currently applied.
    //
    // RETURNS:    Fluctuated RCS realisation (m²). REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-025  CASE_0 returns exact nominalRCS
    //   TC-AESA-SP-026  CASE_I returns positive value for positive input
    // =========================================================================
    double computeSwerlingRCS(double nominalRCS, SwerlingCase sc,
                              bool coherentDwell) const;

    // =========================================================================
    // §E  TARGET MOTION AND ALBERSHEIM Pd
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeTargetMotionParams
    //
    // DESCRIPTION: Populates kinematic fields of a DetectionOutput from
    //              target velocity: speedOverGround, heading, acceleration (0),
    //              and targetAspect angle. REQ-AESA-040.
    //
    // PARAMETERS:
    //   det     [out]  DetectionOutput fields populated.
    //   target  [in]   Target velocity vector.
    //   range   [in]   Slant range (metres).
    //
    // SIDE EFFECTS: Writes det.speedOverGround, det.heading, det.acceleration,
    //               det.targetAspect.
    // TRACEABILITY: TC-AESA-SP-027  Stationary target gives speed=0, heading=0
    // =========================================================================
    void computeTargetMotionParams(DetectionOutput& det,
                                   const TargetInput& target,
                                   double range) const;

    // =========================================================================
    // FUNCTION:    computeRadialVelocity
    //
    // DESCRIPTION: Computes radial velocity (m/s, positive = closing) as the
    //              dot product of target velocity and LOS unit vector, plus
    //              Doppler noise from the supplied distribution. REQ-AESA-040.
    //
    // PARAMETERS:
    //   target  [in]      Target position and velocity.
    //   range   [in]      Slant range (metres).
    //   noise   [in/out]  Normal distribution for Doppler noise. Stateful.
    //
    // RETURNS:    Radial velocity (m/s). REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-028  Head-on target gives negative radial vel
    // =========================================================================
    double computeRadialVelocity(const TargetInput& target, double range,
                                 std::normal_distribution<double>& noise) const;

    // =========================================================================
    // FUNCTION:    computeCPA
    //
    // DESCRIPTION: Computes closest point of approach (CPA) distance and time
    //              by finding where d/dt(range^2) = 0. REQ-AESA-040.
    //
    // PARAMETERS:
    //   det     [out]  Writes cpa_distance, time_to_cpa.
    //   target  [in]   Target position and velocity.
    //   range   [in]   Current slant range (metres).
    //
    // SIDE EFFECTS: Writes det.cpa_distance, det.time_to_cpa. REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-029  Stationary target CPA = current range
    // =========================================================================
    void computeCPA(DetectionOutput& det, const TargetInput& target,
                    double range) const;

    // =========================================================================
    // FUNCTION:    computeAlbersheimPd
    //
    // DESCRIPTION: Computes probability of detection using the Albersheim
    //              closed-form approximation via 40-iteration bisection search.
    //              Applies Swerling loss factors:
    //                CASE_I/II:  5.72 dB additional required SNR
    //                CASE_III/IV: 2.36 dB additional required SNR
    //              Valid for N >= 1 and Pfa in (0, 1). REQ-AESA-040.
    //
    // PARAMETERS:
    //   snr_linear  [in]  SINR (linear). Returns 0.0 if <= 0.
    //   Pfa         [in]  False alarm probability.
    //   N           [in]  Number of integrated pulses.
    //   sc          [in]  Swerling case for loss factor.
    //
    // RETURNS:    Pd in [0.0, 0.99]. REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-030  High SNR gives Pd close to 0.99
    // =========================================================================
    double computeAlbersheimPd(double snr_linear, double Pfa, int N,
                               SwerlingCase sc) const;

    // =========================================================================
    // FUNCTION:    computePk
    //
    // DESCRIPTION: Wraps computeAlbersheimPd() as the probability of kill
    //              estimate. REQ-AESA-040.
    //
    // PARAMETERS:  Same as computeAlbersheimPd(). Default sc = CASE_I.
    // RETURNS:     Pk in [0.0, 0.99]. REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-030
    // =========================================================================
    double computePk(double sinr_linear, double Pfa, int N,
                     SwerlingCase sc = SwerlingCase::CASE_I) const;

    // =========================================================================
    // §F  RANGE AMBIGUITY
    // REQ-AESA-021
    // =========================================================================

    // =========================================================================
    // FUNCTION:    applyRangeAmbiguity
    //
    // DESCRIPTION: Applies range folding due to PRF ambiguity and optionally
    //              resolves it using staggered PRF coincidence detection.
    //
    //              Single PRF (Rmax2 <= 1.0):
    //                If range > Rmax: det.range = fmod(range, Rmax) + noise.
    //                                det.isAmbiguous = true.
    //                If range <= Rmax: det.range = range + noise.
    //                                  det.isAmbiguous = false.
    //
    //              Staggered PRF (Rmax2 > 1.0):
    //                Two independent folded measurements are computed and
    //                fed to resolveRangeAmbiguityStaggered(). On successful
    //                coincidence, det.isAmbiguous = false. REQ-AESA-021.
    //
    // REQUIREMENT: REQ-AESA-021
    //
    // PARAMETERS:
    //   det    [out]  Writes det.range, det.isAmbiguous.
    //   range  [in]   True slant range (metres).
    //   Rmax   [in]   Primary PRF unambiguous range (metres).
    //   Rmax2  [in]   Secondary PRF unambiguous range (metres). 0 = disabled.
    //   noise  [in/out] Range noise distribution. Stateful.
    //
    // SIDE EFFECTS: Writes det.range, det.isAmbiguous. REQ-AESA-021.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-031  Range < Rmax stored unchanged (no ambiguity)
    //   TC-AESA-SP-032  Range > Rmax folded correctly
    //   TC-AESA-SP-033  Staggered PRF resolves k=1 fold correctly
    // =========================================================================
    void applyRangeAmbiguity(DetectionOutput& det, double range,
                             double Rmax, double Rmax2,
                             std::normal_distribution<double>& noise) const;

    // =========================================================================
    // FUNCTION:    resolveRangeForLockOn
    //
    // DESCRIPTION: Resolves range ambiguity in LOCK_ON mode using the
    //              Kalman-predicted track range as the prior.
    //              Searches integer multiples of Rmax in [-5, +5].
    //              Sets det.isAmbiguous = false on completion. REQ-AESA-021.
    //
    // PARAMETERS:
    //   det       [in/out]  det.range is updated. det.isAmbiguous cleared.
    //   range     [in]      True slant range (metres, used as fallback prior).
    //   Rmax      [in]      Unambiguous range (metres).
    //   targetId  [in]      ID of locked target for track database lookup.
    //   db        [in]      Tracker database for predicted range prior.
    //
    // SIDE EFFECTS: Writes det.range, det.isAmbiguous. REQ-AESA-021.
    // TRACEABILITY: TC-AESA-SP-034  Lock-on range resolved to track predicted
    // =========================================================================
    void resolveRangeForLockOn(DetectionOutput& det, double range, double Rmax,
                               uint32_t targetId,
                               const std::vector<TrackFile>& db) const;

    // =========================================================================
    // FUNCTION:    resolveRangeAmbiguity
    //
    // DESCRIPTION: Resolves a folded range measurement using the Chinese
    //              Remainder approach: searches k in [-5, +5] for the
    //              candidate measured + k*Rmax closest to predicted.
    //              Returns measured unchanged if Rmax < 1.0. REQ-AESA-021.
    //
    // PARAMETERS:
    //   measured   [in]  Folded range measurement (metres).
    //   predicted  [in]  Kalman-predicted range prior (metres).
    //   Rmax       [in]  Unambiguous range (metres).
    //
    // RETURNS:    Resolved range (metres). REQ-AESA-021.
    // TRACEABILITY: TC-AESA-SP-033
    // =========================================================================
    double resolveRangeAmbiguity(double measured, double predicted,
                                 double Rmax) const;

    // =========================================================================
    // FUNCTION:    resolveRangeAmbiguityStaggered
    //
    // DESCRIPTION: Resolves range ambiguity using two folded measurements from
    //              two different PRFs. Searches n1, n2 in [0, 4] for
    //              candidates that agree within a 500 m coincidence gate.
    //              Returns the candidate closest to predicted. REQ-AESA-021.
    //
    // PARAMETERS:
    //   measured1/2  [in]  Folded range from PRF1 and PRF2 (metres).
    //   Rmax1/2      [in]  Unambiguous ranges for PRF1 and PRF2 (metres).
    //   predicted    [in]  Prior estimate for disambiguation (metres).
    //
    // RETURNS:    Resolved range (metres). REQ-AESA-021.
    // TRACEABILITY: TC-AESA-SP-033
    // =========================================================================
    double resolveRangeAmbiguityStaggered(double measured1, double measured2,
                                          double Rmax1, double Rmax2,
                                          double predicted) const;

    // =========================================================================
    // FUNCTION:    resolveVelocityStaggered
    //
    // DESCRIPTION: Resolves Doppler velocity ambiguity using two folded velocity
    //              measurements from two different PRFs. Both are folded into
    //              [0, Vmax] then searched for coincidence within 2 m/s gate.
    //              REQ-AESA-021.
    //
    // PARAMETERS:
    //   foldedVel1/2   [in]  Folded velocity from PRF1 and PRF2 (m/s).
    //   Vmax1/2        [in]  Unambiguous velocity limits (m/s).
    //   predictedVel   [in]  Prior velocity estimate for disambiguation (m/s).
    //
    // RETURNS:    Resolved radial velocity (m/s). REQ-AESA-021.
    // TRACEABILITY: TC-AESA-SP-035  Staggered velocity resolves k=1 fold
    // =========================================================================
    double resolveVelocityStaggered(double foldedVel1, double foldedVel2,
                                    double Vmax1, double Vmax2,
                                    double predictedVel) const;

    // =========================================================================
    // §G  MAX DETECTION RANGE
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeMaxDetectionRange
    //
    // DESCRIPTION: Iteratively solves the radar range equation for the maximum
    //              range at which the specified target RCS can be detected above
    //              the CFAR threshold. Uses up to 20 iterations converging when
    //              |R_new - R_prev| < 10 m. REQ-AESA-040.
    //
    // PARAMETERS:
    //   rcs  [in]  Target RCS (m²). Must be > 0.
    //   cfg  [in]  Full radar configuration.
    //
    // RETURNS:    Maximum detection range (km). REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-036  Returns > 0 for any valid config
    // =========================================================================
    double computeMaxDetectionRange(double rcs,
                                    const RadarConfig& cfg) const;

    // =========================================================================
    // §H  DETECTION MERGE GUARD
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    shouldMergeDetection
    //
    // DESCRIPTION: Returns true if the candidate detection is within the merge
    //              gate of any existing detection in the list. Prevents the same
    //              target appearing twice from adjacent beam positions.
    //              Merge gate: range within 150 m AND az/el within beamWidth.
    //              REQ-AESA-040.
    //
    // PARAMETERS:
    //   det       [in]  Candidate detection to test.
    //   existing  [in]  Current scan detection list.
    //   cfg       [in]  Uses beamWidth for angular gate.
    //
    // RETURNS:    true = merge (suppress candidate). false = add. REQ-AESA-040.
    // TRACEABILITY:
    //   TC-AESA-SP-037  Same position returns merge=true
    //   TC-AESA-SP-038  Distant position returns merge=false
    // =========================================================================
    bool shouldMergeDetection(const DetectionOutput& det,
                              const std::vector<DetectionOutput>& existing,
                              const RadarConfig& cfg) const;

    // =========================================================================
    // §I  BEAM GAIN AND SIDELOBE BLANKING
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeBeamGainFactor
    //
    // DESCRIPTION: Returns a gain reduction factor [0, 1] for targets at the
    //              edge of or outside the main beam. Targets within 2*bw of
    //              beam centre get factor 1.0 (full gain). Targets further out
    //              get the average sidelobe level converted to linear. REQ-AESA-040.
    //
    // PARAMETERS:
    //   azDiff/elDiff      [in]  Signed angular differences (degrees).
    //   cfg                [in]  Uses sidelobeMode, peakSidelobeLevel,
    //                            avgSidelobeLevel, beamWidth.
    //   effectiveBeamWidth [in]  Spoiled beamwidth. -1.0 = use cfg.beamWidth.
    //
    // RETURNS:    Linear gain factor [pow(10, SLL/10), 1.0]. REQ-AESA-040.
    // TRACEABILITY:
    //   TC-AESA-SP-039  Boresight target returns factor 1.0
    //   TC-AESA-SP-040  Far sidelobe target returns avgSLL linear factor
    // =========================================================================
    double computeBeamGainFactor(double azDiff, double elDiff,
                                 const RadarConfig& cfg,
                                 double effectiveBeamWidth = -1.0) const;

    // =========================================================================
    // FUNCTION:    isJammerInSidelobe
    //
    // DESCRIPTION: Returns true if a jammer located in the sidelobe region
    //              produces received power exceeding the sidelobeBlanking_dB
    //              threshold above noise. Used to blank detections that would
    //              be drowned by sidelobe interference. REQ-AESA-040.
    //
    // PARAMETERS:
    //   azDiff/elDiff  [in]  Target angular offsets from beam centre (degrees).
    //   target         [in]  Target with jammer configuration.
    //   cfg            [in]  Uses beamWidth, sidelobeBlanking_dB.
    //
    // RETURNS:    true = jammer in sidelobe above threshold → blank detection.
    //             false = no sidelobe blanking needed. REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-041  Inactive jammer returns false
    //   TC-AESA-SP-042  Main beam jammer returns false (not sidelobe)
    // =========================================================================
    bool isJammerInSidelobe(double azDiff, double elDiff,
                            const TargetInput& target,
                            const RadarConfig& cfg) const;

    // =========================================================================
    // §J  MODULATION PROCESSING GAIN
    // REQ-AESA-020
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeModulationProcessingGain
    //
    // DESCRIPTION: Returns pulse compression processing gain for LFM, NLFM,
    //              and FMCW waveforms: pg = bandwidth_Hz * pulseWidth_s.
    //              Returns 1.0 for unmodulated pulses (NONE).
    //              Clamped to >= 1.0 (pg < 1 would reduce SNR, not possible).
    //              REQ-AESA-020.
    //
    // PARAMETERS:
    //   waveform  [in]  Uses modulation, bandwidth_Hz, pulseWidth_s.
    //
    // RETURNS:    Processing gain (dimensionless, >= 1.0). REQ-AESA-020.
    // TRACEABILITY:
    //   TC-AESA-SP-043  LFM waveform returns bandwidth * pulseWidth
    //   TC-AESA-SP-044  NONE modulation returns 1.0
    // =========================================================================
    double computeModulationProcessingGain(const BeamWaveform& waveform) const;

    // =========================================================================
    // §K  DOPPLER CLUTTER NOTCH AND STAP
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    isInDopplerBlindZone
    //
    // DESCRIPTION: Returns true if the target's radial velocity falls within
    //              the MTI clutter notch. HPRF waveforms are immune (return
    //              false). Stationary platforms (platformSpeed < 1 m/s) have
    //              no meaningful clutter notch (return false). REQ-AESA-040.
    //
    // PARAMETERS:
    //   radVel_m_s  [in]  Target radial velocity (m/s).
    //   cfg         [in]  Uses platformSpeed_m_s.
    //   waveform    [in]  Uses mode (HPRF bypasses notch), prf_Hz, pulsesPerDwell.
    //
    // RETURNS:    true = target in MTI blind zone. false = target detectable.
    //             REQ-AESA-040.
    //
    // TRACEABILITY:
    //   TC-AESA-SP-045  HPRF waveform returns false always
    //   TC-AESA-SP-046  Stationary platform returns false
    // =========================================================================
    bool isInDopplerBlindZone(double radVel_m_s,
                              const RadarConfig& cfg,
                              const BeamWaveform& waveform) const;

    // =========================================================================
    // FUNCTION:    computeClutterNotch
    //
    // DESCRIPTION: Computes the MTI clutter notch velocity interval [lo, hi]
    //              in m/s centred at platformSpeed_m_s. Notch width =
    //              lambda * prf / (2 * N) clamped to [1, 15] m/s. REQ-AESA-040.
    //
    // PARAMETERS:
    //   cfg  [in]  Uses platformSpeed_m_s, frequency_Hz.
    //   wf   [in]  Uses prf_Hz, pulsesPerDwell.
    //
    // RETURNS:    {lo, hi} in m/s. REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-047  Notch centre at platformSpeed
    // =========================================================================
    std::pair<double,double> computeClutterNotch(const RadarConfig& cfg,
                                                  const BeamWaveform& wf) const;

    // =========================================================================
    // FUNCTION:    computeSTAPGain
    //
    // DESCRIPTION: Computes STAP (Space-Time Adaptive Processing) improvement
    //              factor over MTI. Full gain = N_pulses * sqrt(N_elements).
    //              Targets within the clutter notch get partial recovery
    //              proportional to their distance from notch centre. REQ-AESA-040.
    //
    // PARAMETERS:
    //   radialVelocity_m_s  [in]  Target radial velocity (m/s).
    //   platformSpeed_m_s   [in]  Platform speed for notch centre.
    //   wf                  [in]  Uses pulsesPerDwell.
    //   cfg                 [in]  Uses numElements, failedModules.
    //
    // RETURNS:    STAP gain factor (linear, >= 1.0, capped at 1000). REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-048  Far-from-notch target gets full STAP gain
    // =========================================================================
    double computeSTAPGain(double radialVelocity_m_s,
                           double platformSpeed_m_s,
                           const BeamWaveform& wf,
                           const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    isInClutterNotchSTAP
    //
    // DESCRIPTION: Returns true if the target is within the STAP-narrowed
    //              clutter notch. STAP narrows the notch by sqrt(N_spatial)
    //              compared to MTI. HPRF and stationary platforms return false.
    //              REQ-AESA-040.
    //
    // PARAMETERS:  Same as isInDopplerBlindZone().
    // RETURNS:     true = in STAP notch. false = STAP can recover target.
    // TRACEABILITY: TC-AESA-SP-048
    // =========================================================================
    bool isInClutterNotchSTAP(double radVel_m_s,
                              const RadarConfig& cfg,
                              const BeamWaveform& wf) const;

    // =========================================================================
    // §L  MONOPULSE ANGLE ERROR
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeMonopulseAngleError
    //
    // DESCRIPTION: Computes monopulse angle correction errors for azimuth and
    //              elevation. Combines a systematic term (azDiff / km^2) and a
    //              random noise term (bw / (km * sqrt(2*SINR))). REQ-AESA-040.
    //
    //              km = 1.606 (monopulse sensitivity slope for a sinc aperture).
    //
    // PARAMETERS:
    //   azDiff_deg/elDiff_deg  [in]   Angular offsets from beam centre (deg).
    //   sinr                   [in]   Current SINR (linear). Noise sigma = bw
    //                                 when sinr <= 0.
    //   cfg                    [in]   Uses beamWidth.
    //   outAzError_deg/outElError_deg [out] Angle correction terms (degrees).
    //
    // SIDE EFFECTS: Writes outAzError_deg, outElError_deg. REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-049  Boresight target gives near-zero error mean
    // =========================================================================
    void computeMonopulseAngleError(double azDiff_deg, double elDiff_deg,
                                    double sinr, const RadarConfig& cfg,
                                    double& outAzError_deg,
                                    double& outElError_deg) const;

    // =========================================================================
    // §M  WAVEFORM SELECTION
    // REQ-AESA-020
    // =========================================================================

    // =========================================================================
    // FUNCTION:    selectWaveformForRange
    //
    // DESCRIPTION: Selects the waveform from cfg.waveformTable whose maxRange_m
    //              first exceeds range_m. If no entry matches, returns
    //              cfg.searchWaveform. Table must be sorted ascending by
    //              maxRange_m with sentinel 0.0 at end. REQ-AESA-020.
    //
    // PARAMETERS:
    //   range_m  [in]  Target slant range (metres).
    //   cfg      [in]  Uses waveformTable, searchWaveform.
    //
    // RETURNS:    Selected BeamWaveform. REQ-AESA-020.
    // TRACEABILITY: TC-AESA-SP-050  Short range selects HPRF waveform
    // =========================================================================
    BeamWaveform selectWaveformForRange(double range_m,
                                        const RadarConfig& cfg) const;

    // =========================================================================
    // §N  TWO-RAY MULTIPATH
    // REQ-AESA-072
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeMultipathFactor
    //
    // DESCRIPTION: Computes the two-ray multipath interference factor for
    //              low-elevation targets over a flat reflecting surface.
    //              Factor = 4 * sin^2(dphi/2)
    //              where dphi = 4*pi*h_radar*h_target / (lambda*range).
    //              Returns 1.0 for elevation > 5 deg, zero altitude, or
    //              range < 1 m. REQ-AESA-072.
    //
    // PARAMETERS:
    //   range_m        [in]  Slant range (metres).
    //   elevation_deg  [in]  Target elevation (degrees).
    //   targetHeight_m [in]  Target altitude (metres).
    //   cfg            [in]  Uses radarHeight, frequency_Hz.
    //
    // RETURNS:    Multipath factor [0.0, 4.0]. REQ-AESA-072.
    // TRACEABILITY: TC-AESA-SP-051  High elevation returns 1.0
    // =========================================================================
    double computeMultipathFactor(double range_m, double elevation_deg,
                                  double targetHeight_m,
                                  const RadarConfig& cfg) const;

    // =========================================================================
    // §O  CHAFF RETURN
    // REQ-AESA-061
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeChaffReturn
    //
    // DESCRIPTION: Computes total received power from all chaff clouds in the
    //              current beam (Watts). For each cloud within 3*beamWidth of
    //              the beam centre, computes RCS decay and radar range equation.
    //              REQ-AESA-061.
    //
    // PARAMETERS:
    //   beamAz/beamEl  [in]  Current beam pointing (degrees, body frame).
    //   clouds         [in]  All active chaff clouds.
    //   simTime        [in]  Current simulation time for RCS decay (seconds).
    //   cfg            [in]  Full radar configuration.
    //
    // RETURNS:    Total chaff return power (Watts). REQ-AESA-061.
    // TRACEABILITY: TC-AESA-SP-052  Empty cloud list returns 0.0
    // =========================================================================
    double computeChaffReturn(double beamAz, double beamEl,
                              const std::vector<ChaffCloud>& clouds,
                              double simTime,
                              const RadarConfig& cfg) const;

    // =========================================================================
    // §P  ATMOSPHERIC PROPAGATION
    // REQ-AESA-071
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeWaterVapourDensity
    //
    // DESCRIPTION: Computes absolute water vapour density (g/m³) from
    //              temperature and relative humidity using the Magnus-Tetens
    //              saturation pressure formula (Buck 1981 / ITU-R P.836-6).
    //              Valid for -40 to +60 deg C and 0 to 100% RH. REQ-AESA-071.
    //
    // PARAMETERS:
    //   atm  [in]  Atmospheric conditions. Uses temperature_C, humidity_pct.
    //
    // RETURNS:    Water vapour density (g/m³, >= 0). REQ-AESA-071.
    // TRACEABILITY: TC-AESA-SP-053  0% humidity returns 0 density
    // =========================================================================
    double computeWaterVapourDensity(const AtmosphericConditions& atm) const;

    // =========================================================================
    // FUNCTION:    computeGaseousAttenuation
    //
    // DESCRIPTION: Computes two-way gaseous absorption loss (dB) using
    //              ITU-R P.676-12 Annex 2 for oxygen (γ_o) and water vapour
    //              (γ_w) components. Returns 2 * (γ_o + γ_w) * range_km.
    //              Valid for 1–350 GHz. REQ-AESA-071.
    //
    // PARAMETERS:
    //   frequency_Hz  [in]  Radar carrier frequency (Hz).
    //   atm           [in]  Atmospheric conditions.
    //   range_m       [in]  One-way path length (metres).
    //
    // RETURNS:    Two-way gaseous loss (dB, >= 0). REQ-AESA-071.
    // TRACEABILITY: TC-AESA-SP-054  Clear standard atmosphere gives positive loss
    // =========================================================================
    double computeGaseousAttenuation(double frequency_Hz,
                                     const AtmosphericConditions& atm,
                                     double range_m) const;

    // =========================================================================
    // UTILITY METHODS (RCS lookup)
    // REQ-AESA-040
    // =========================================================================

    // =========================================================================
    // FUNCTION:    lookupAspectRCS
    //
    // DESCRIPTION: Linearly interpolates target RCS from the aspect-angle table.
    //              Returns getPlatformBaseRCS() if table is empty. REQ-AESA-040.
    //
    // PARAMETERS:
    //   target          [in]  Contains rcsTable and platformType.
    //   aspectAngle_deg [in]  Angle between target heading and radar LOS (deg).
    //
    // RETURNS:    Interpolated RCS (m²). REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-055  Empty table returns base RCS
    // =========================================================================
    double lookupAspectRCS(const TargetInput& target,
                           double aspectAngle_deg) const;

    // =========================================================================
    // FUNCTION:    getPlatformBaseRCS
    //
    // DESCRIPTION: Returns a nominal median RCS (m²) for known platform types
    //              from open literature. Returns 5.0 m² for GENERIC.
    //              REQ-AESA-040.
    //
    // PARAMETERS:
    //   platformType  [in]  String key: "FIGHTER","BOMBER","UAV","MISSILE",
    //                       "HELO","SHIP","STEALTH","GENERIC".
    //
    // RETURNS:    Nominal RCS (m²). REQ-AESA-040.
    // TRACEABILITY: TC-AESA-SP-056  FIGHTER returns 3.0 m²
    // =========================================================================
    double getPlatformBaseRCS(const std::string& platformType) const;
};

} // namespace aesa

#endif // RADARSIGNALPROCESSOR_AESA_H

// #pragma once
// #ifndef RADARSIGNALPROCESSOR_AESA_H
// #define RADARSIGNALPROCESSOR_AESA_H
// // =============================================================================
// // radarsignalprocessor_aesa.h  —  Rev 3
// // All FIX-01..11 physics methods declared here.
// // =============================================================================

// #include "radarmodel_aesa.h"
// #include <random>
// #include <utility>
// #include <vector>

// namespace aesa {

// class RadarSignalProcessor_AESA
// {
// public:
//     RadarSignalProcessor_AESA() = default;

//     // ---- Geometry -----------------------------------------------------------
//     bool   isTargetInBeam(double beamAz, double beamEl,
//                           double targetAz, double targetEl,
//                           const RadarConfig& cfg,
//                           double& outAzDiff, double& outElDiff,
//                           double effectiveBeamWidth = -1.0) const;

//     bool   checkHorizon(double range, double targetZ,
//                         const RadarConfig& cfg) const;

//     // ---- Signal chain -------------------------------------------------------
//     double calculateSignalStrength(double range, double rcs,
//                                    double arrayGain,
//                                    const BeamWaveform& waveform,
//                                    const RadarConfig& cfg) const;

//     double computeNoisePower    (const RadarConfig& cfg, double bandwidth_Hz) const;
//     double computeClutterPower  (double range, SurfaceType surface,
//                                  const RadarConfig& cfg) const;
//     double computeJammerPower   (double targetRange_m, const TargetInput& target,
//                                  const RadarConfig& cfg) const;
//     double computePropagationLoss(double range_m, const RadarConfig& cfg) const;

//     double computeSINR(double receivedPower, double range,
//                        SurfaceType surface, const TargetInput& target,
//                        const RadarConfig& cfg, const BeamWaveform& waveform) const;

//     // ---- CFAR ---------------------------------------------------------------
//     std::vector<double> generateReferenceCells(SurfaceType surface,
//                                                const RadarConfig& cfg) const;
//     double computeCFARThreshold        (const std::vector<double>& cells,
//                                         const RadarConfig& cfg) const;
//     double computeCFARThresholdRelaxed (const std::vector<double>& cells,
//                                         const RadarConfig& cfg) const;

//     // ---- RCS / Swerling  (FIX-07) ------------------------------------------
//     double computeEffectiveRCS(const TargetInput& target,
//                                double range,
//                                double frequency_Hz) const;
//     //double computeEffectiveRCS(const TargetInput& target, double range) const;
//     double computeSwerlingRCS (double nominalRCS, SwerlingCase sc,
//                                bool coherentDwell) const;

//     // ---- Target motion ------------------------------------------------------
//     void   computeTargetMotionParams(DetectionOutput& det,
//                                      const TargetInput& target,
//                                      double range) const;
//     double computeRadialVelocity(const TargetInput& target, double range,
//                                  std::normal_distribution<double>& noise) const;
//     void   computeCPA(DetectionOutput& det, const TargetInput& target,
//                       double range) const;

//     // ---- FIX-07  Albersheim Pd + Pk replacement ----------------------------
//     double computeAlbersheimPd(double snr_linear, double Pfa, int N,
//                                 SwerlingCase sc) const;
//     double computePk           (double sinr_linear, double Pfa, int N,
//                                 SwerlingCase sc = SwerlingCase::CASE_I) const;

//     // ---- Range ambiguity ----------------------------------------------------
//    // void   applyRangeAmbiguity(DetectionOutput& det, double range, double Rmax,
//                                //std::normal_distribution<double>& noise) const;
//     void   applyRangeAmbiguity(DetectionOutput& det, double range,
//                              double Rmax, double Rmax2,
//                              std::normal_distribution<double>& noise) const;
//     void   resolveRangeForLockOn(DetectionOutput& det, double range, double Rmax,
//                                  uint32_t targetId,
//                                  const std::vector<TrackFile>& db) const;
//    // double resolveRangeAmbiguity(double measured, double predicted, double Rmax) const;
//     double resolveRangeAmbiguity(double measured, double predicted, double Rmax) const;

//     // Staggered PRF resolvers — only called when wf.prf2_Hz > 0
//     double resolveRangeAmbiguityStaggered(double measured1, double measured2,
//                                           double Rmax1,     double Rmax2,
//                                           double predicted) const;

//     double resolveVelocityStaggered(double foldedVel1, double foldedVel2,
//                                     double Vmax1,      double Vmax2,
//                                     double predictedVel) const;
//     // ---- Max detection range -----------------------------------------------
//     double computeMaxDetectionRange(double rcs, const RadarConfig& cfg) const;

//     // ---- Detection merge guard ---------------------------------------------
//     bool shouldMergeDetection(const DetectionOutput& det,
//                               const std::vector<DetectionOutput>& existing,
//                               const RadarConfig& cfg) const;

//     // ---- Beam gain + FIX-11 sidelobe blanking ------------------------------
//     double computeBeamGainFactor(double azDiff, double elDiff,
//                                  const RadarConfig& cfg,
//                                  double effectiveBeamWidth = -1.0) const;

//     bool   isJammerInSidelobe(double azDiff, double elDiff,
//                               const TargetInput& target,
//                               const RadarConfig& cfg) const;

//     // ---- Modulation processing gain ----------------------------------------
//     double computeModulationProcessingGain(const BeamWaveform& waveform) const;

//     // ---- FIX-01  Doppler clutter notch -------------------------------------
//     bool isInDopplerBlindZone(double radVel_m_s,
//                               const RadarConfig& cfg,
//                               const BeamWaveform& waveform) const;
//     std::pair<double,double> computeClutterNotch(const RadarConfig& cfg,
//                                                   const BeamWaveform& wf) const;

//     // ---- FIX-02  Monopulse angle error -------------------------------------
//     void computeMonopulseAngleError(double azDiff_deg, double elDiff_deg,
//                                     double sinr, const RadarConfig& cfg,
//                                     double& outAzError_deg,
//                                     double& outElError_deg) const;

//     // ---- FIX-06  Range-based waveform selection ----------------------------
//     BeamWaveform selectWaveformForRange(double range_m,
//                                         const RadarConfig& cfg) const;

//     // ---- FIX-09  Two-ray multipath factor ----------------------------------
//     double computeMultipathFactor(double range_m, double elevation_deg,
//                                   double targetHeight_m,
//                                   const RadarConfig& cfg) const;

//     // ---- FIX-10  Chaff return ----------------------------------------------
//     double computeChaffReturn(double beamAz, double beamEl,
//                               const std::vector<ChaffCloud>& clouds,
//                               double simTime,
//                               const RadarConfig& cfg) const;
//     // ADD to RadarSignalProcessor_AESA class public section:
//     double lookupAspectRCS(const TargetInput& target, double aspectAngle_deg) const;
//     double getPlatformBaseRCS(const std::string& platformType) const;
//     // ADD — MTI/STAP improvement factor
//     double computeSTAPGain(double radialVelocity_m_s,
//                            double platformSpeed_m_s,
//                            const BeamWaveform& wf,
//                            const RadarConfig& cfg) const;

//     bool isInClutterNotchSTAP(double radVel_m_s,
//                               const RadarConfig& cfg,
//                               const BeamWaveform& wf) const;
//     double computeWaterVapourDensity(const AtmosphericConditions& atm) const;

//     double computeGaseousAttenuation(double frequency_Hz,
//                                      const AtmosphericConditions& atm,
//                                      double range_m) const;
// };

// } // namespace aesa
// #endif
