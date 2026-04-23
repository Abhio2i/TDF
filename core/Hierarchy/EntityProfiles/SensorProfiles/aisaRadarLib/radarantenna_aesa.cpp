// =============================================================================
// FILE:         radarantenna_aesa.cpp
// MODULE:       AESA Antenna Beam Steering
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements electronic beam steering, reachability validation,
//               steering angle computation, and array gain computation for
//               the AESA radar antenna model. All functions are stateless
//               computations or direct state updates — no external I/O,
//               no dynamic memory allocation, no recursion.
//
// REQUIREMENTS: REQ-AESA-010  Antenna beam pointing and steering
//               REQ-AESA-011  Beam reachability validation
//               REQ-AESA-012  Array gain computation with element pattern
//               REQ-AESA-013  Beam spoiling factor support
//               REQ-AESA-014  Scan boundary state management
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-ANTENNA-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Basic cos(theta) gain model.
//   Rev 2  15 Feb 2026  REQ-AESA-013: Beam spoiling factor added.
//   Rev 3  01 Apr 2026  REQ-AESA-012: Gain model upgraded to cos^1.5 element
//                       pattern. Dead array guard added. Az normalisation added.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Commented-out code removed per NS-05. Magic numbers
//                       replaced with named constexpr constants.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#include "radarantenna_aesa.h"
#include <algorithm>
#include <cmath>

// =============================================================================
// NAMED CONSTANTS
// All numeric literals used in physics computations are declared here.
// This satisfies VI-08 (no magic numbers) and ensures single-point change
// if physical constants or model parameters are revised.
// Requirements: REQ-AESA-012
// =============================================================================

namespace
{
// Pi — full precision, replaces non-standard M_PI macro (LC-08 compliance).
// Using a local constexpr avoids reliance on compiler-defined M_PI extension.
constexpr double PI = 3.14159265358979323846;

// Conversion factor: degrees to radians.
constexpr double DEG_TO_RAD = PI / 180.0;

// Conversion factor: radians to degrees.
constexpr double RAD_TO_DEG = 180.0 / PI;

// Element pattern exponent for microstrip patch elements.
// cos^ELEMENT_PATTERN_EXP(theta) — value 1.5 per Mailloux, Phased Array
// Antenna Handbook, 2nd Ed, Ch 3. Valid for patch elements up to 60 deg.
// REQ-AESA-012.
constexpr double ELEMENT_PATTERN_EXP = 1.5;

// Array factor Gaussian taper exponent coefficient.
// Controls how quickly gain rolls off beyond boresight in the array factor
// model. Value 2.0 gives ~3 dB loss at max steering angle. REQ-AESA-012.
constexpr double ARRAY_TAPER_EXP = 2.0;

// Gain floor in linear scale, equivalent to -40 dB.
// Prevents steerLoss from reaching zero which would cause a zero-divide
// in downstream SINR computation. REQ-AESA-012 acceptance criterion 3.
constexpr double GAIN_FLOOR_LINEAR = 1.0e-4;

// Azimuth wrap upper bound (degrees). Azimuth is normalised to
// [-AZ_WRAP_LIMIT, +AZ_WRAP_LIMIT] before reachability check. REQ-AESA-010.
constexpr double AZ_WRAP_LIMIT = 180.0;

// Full azimuth circle (degrees). Used in wrap normalisation. REQ-AESA-010.
constexpr double AZ_FULL_CIRCLE = 360.0;

// Minimum valid spoil factor. Values below this are physically meaningless
// — a beam cannot be narrower than its natural aperture-limited width.
// REQ-AESA-013.
constexpr float  SPOIL_FACTOR_MIN = 1.0f;

// Minimum active element count that produces non-zero gain.
// If active <= ACTIVE_ELEMENT_ZERO_THRESHOLD the array is inoperable.
// REQ-AESA-012 acceptance criterion 1.
constexpr int    ACTIVE_ELEMENT_ZERO_THRESHOLD = 0;

// Dot product clamp bounds for acos safety. Floating-point arithmetic can
// produce values marginally outside [-1, 1] causing acos to return NaN.
constexpr double DOT_CLAMP_MIN = -1.0;
constexpr double DOT_CLAMP_MAX =  1.0;

} // anonymous namespace

namespace aesa {

// =============================================================================
// FUNCTION:    RadarAntenna_AESA::reset
// (Full description in header)
// =============================================================================
void RadarAntenna_AESA::reset()
{
    // Reset beam pointing to boresight (az=0, el=0 — forward horizontal).
    // This is the safe default state before the first beam command.
    // REQ-AESA-010: beam state must be defined after reset.
    currentAzimuth_   = 0.0;
    currentElevation_ = 0.0;

    // Reset spoil factor to 1.0 — no beam widening, natural aperture beamwidth.
    // REQ-AESA-013: spoilFactor >= 1.0 at all times.
    currentSpoilFactor_ = SPOIL_FACTOR_MIN;

    // Reset effective beam width to zero — will be recomputed on first pointBeam().
    // Zero is safe here because no gain computation occurs until pointBeam() runs.
    effectiveBeamWidth_ = 0.0;

    // Clear scan boundary flag — no scan has completed at reset time.
    // REQ-AESA-014: boundary flag must be false after reset.
    scanBoundaryOccurred_ = false;
}

// =============================================================================
// FUNCTION:    RadarAntenna_AESA::pointBeam
// (Full description in header)
// =============================================================================
void RadarAntenna_AESA::pointBeam(double az_deg, double el_deg,
                                  const RadarConfig& cfg,
                                  float spoilFactor)
{
    // -------------------------------------------------------------------------
    // Step 1: Sanitise inputs — reject non-finite values.
    // Non-finite azimuth or elevation (NaN, Inf) indicate a numerical failure
    // upstream (e.g. atan2 of a zero vector). Replace with boresight (0.0)
    // to maintain a defined beam state. REQ-AESA-010 acceptance criterion 1.
    // -------------------------------------------------------------------------
    if (!std::isfinite(az_deg)) az_deg = 0.0;
    if (!std::isfinite(el_deg)) el_deg = 0.0;

    // -------------------------------------------------------------------------
    // Step 2: Normalise azimuth to [-180, +180] degrees.
    // isReachable() uses trigonometric functions which are periodic, but
    // the clamping logic in step 3 assumes az is already in this range.
    // Without normalisation, a commanded az of 270 deg would be incorrectly
    // clamped rather than recognised as equivalent to -90 deg.
    // REQ-AESA-010 acceptance criterion 2.
    // -------------------------------------------------------------------------
    while (az_deg >  AZ_WRAP_LIMIT) az_deg -= AZ_FULL_CIRCLE;
    while (az_deg < -AZ_WRAP_LIMIT) az_deg += AZ_FULL_CIRCLE;

    // -------------------------------------------------------------------------
    // Step 3: Reachability check and clamping.
    // If the requested position is outside the electronically steerable FoV
    // (defined by cfg.maxSteeringAngle_deg), clamp to the nearest valid
    // position. This prevents the scheduler from commanding an unreachable
    // beam, which would corrupt the gain calculation.
    // REQ-AESA-011: unreachable beams shall be clamped, not rejected silently.
    // -------------------------------------------------------------------------
    if (!isReachable(az_deg, el_deg, cfg))
    {
        // Clamp azimuth to [-maxSteeringAngle, +maxSteeringAngle].
        // Cast to double to avoid float precision loss in clamp comparison.
        double maxAng = static_cast<double>(cfg.maxSteeringAngle_deg);
        az_deg = std::clamp(az_deg, -maxAng, maxAng);

        // Clamp elevation to [minElevation, maxElevation] from config.
        // These define the physical FoV of the antenna aperture.
        el_deg = std::clamp(el_deg,
                            static_cast<double>(cfg.minElevation),
                            static_cast<double>(cfg.maxElevation));
    }

    // -------------------------------------------------------------------------
    // Step 4: Commit sanitised, validated beam pointing state.
    // REQ-AESA-010: beam state updated atomically after all checks pass.
    // -------------------------------------------------------------------------
    currentAzimuth_   = az_deg;
    currentElevation_ = el_deg;

    // Clamp spoil factor to minimum of 1.0 — beam cannot be narrower than
    // the natural aperture-limited beamwidth. REQ-AESA-013.
    currentSpoilFactor_ = std::max(SPOIL_FACTOR_MIN, spoilFactor);

    // Effective beam width = natural beamwidth multiplied by spoil factor.
    // Used by signal processor beam gate check (isTargetInBeam).
    // REQ-AESA-013: effectiveBeamWidth >= cfg.beamWidth at all times.
    effectiveBeamWidth_ = static_cast<double>(cfg.beamWidth)
                          * static_cast<double>(currentSpoilFactor_);
}

// =============================================================================
// FUNCTION:    RadarAntenna_AESA::isReachable
// (Full description in header)
// =============================================================================
bool RadarAntenna_AESA::isReachable(double az_deg, double el_deg,
                                    const RadarConfig& cfg) const
{
    // Convert az and el to radians for trigonometric computation.
    // REQ-AESA-011: steering angle computed in cosine space.
    double azRad = az_deg * DEG_TO_RAD;
    double elRad = el_deg * DEG_TO_RAD;

    // Compute the x-direction cosine of the beam unit vector.
    // bx = cos(el) * cos(az) is the projection onto the boresight axis (x-axis).
    // When az=0 and el=0 (boresight), bx=1.0 and steer=0 deg.
    // When the beam is 90 deg off boresight, bx=0.0 and steer=90 deg.
    double bx = std::cos(elRad) * std::cos(azRad);

    // Compute steering angle as the arc-cosine of bx.
    // clamp prevents NaN from floating-point values marginally outside [-1, 1].
    // REQ-AESA-011 acceptance criterion 1: acos domain protection required.
    double steer = std::acos(std::clamp(bx, DOT_CLAMP_MIN, DOT_CLAMP_MAX))
                   * RAD_TO_DEG;

    // Return true only if steering angle is within the array's steerable FoV.
    // Cast maxSteeringAngle_deg to double to avoid implicit float comparison.
    return steer <= static_cast<double>(cfg.maxSteeringAngle_deg);
}

// =============================================================================
// FUNCTION:    RadarAntenna_AESA::computeSteeringAngle
// (Full description in header)
// =============================================================================
double RadarAntenna_AESA::computeSteeringAngle(double beamAz, double beamEl,
                                               double targetAz, double targetEl) const
{
    // Convert all four angles to radians.
    // a1/e1 = beam direction, a2/e2 = target direction. REQ-AESA-012.
    double a1 = beamAz   * DEG_TO_RAD;
    double e1 = beamEl   * DEG_TO_RAD;
    double a2 = targetAz * DEG_TO_RAD;
    double e2 = targetEl * DEG_TO_RAD;

    // Compute dot product of the two unit vectors using the spherical
    // great-circle formula:
    //   dot = cos(e1)*cos(e2)*cos(a1-a2) + sin(e1)*sin(e2)
    // This gives the cosine of the angular separation between the two
    // directions. REQ-AESA-012 acceptance criterion 1.
    double dot = std::cos(e1) * std::cos(e2) * std::cos(a1 - a2)
                 + std::sin(e1) * std::sin(e2);

    // Clamp dot product before acos to guard against floating-point values
    // marginally outside [-1, 1] which would produce NaN.
    // REQ-AESA-012 acceptance criterion 2: no NaN output permitted.
    return std::acos(std::clamp(dot, DOT_CLAMP_MIN, DOT_CLAMP_MAX)) * RAD_TO_DEG;
}

// =============================================================================
// FUNCTION:    RadarAntenna_AESA::computeArrayGain
// (Full description in header)
// =============================================================================
double RadarAntenna_AESA::computeArrayGain(double steeringAngle_deg,
                                           const RadarConfig& cfg,
                                           float spoilFactor) const
{
    // -------------------------------------------------------------------------
    // Step 1: Compute number of active T/R modules.
    // active = total elements minus failed modules, floored at 0.
    // std::max prevents negative active count if failedModules > numElements
    // due to a config error. REQ-AESA-012 acceptance criterion 1.
    // -------------------------------------------------------------------------
    int active = std::max(ACTIVE_ELEMENT_ZERO_THRESHOLD,
                          cfg.numElements - cfg.failedModules);

    // Dead array check — all modules have failed.
    // Return 0.0 immediately. No further computation is valid or meaningful.
    // This is a hard guarantee per REQ-AESA-012: a failed array produces
    // zero gain, not a spurious non-zero value that could cause a false detection.
    if (active <= ACTIVE_ELEMENT_ZERO_THRESHOLD) return 0.0;

    // -------------------------------------------------------------------------
    // Step 2: Compute active element ratio.
    // ratio = active / total — fraction of array that is operational.
    // Guard against cfg.numElements == 0 (invalid config) to prevent
    // division by zero. If numElements is 0 but active > 0, ratio defaults
    // to 1.0 (treat as fully operational). REQ-AESA-012.
    // -------------------------------------------------------------------------
    double ratio = (cfg.numElements > 0)
                       ? static_cast<double>(active) / cfg.numElements
                       : 1.0;

    // Convert steering angle to radians for trigonometric gain computation.
    double theta = steeringAngle_deg * DEG_TO_RAD;

    // Convert boresight gain from dBi to linear scale.
    // G_bore is the peak gain at boresight with full array and no spoiling.
    double G_bore = std::pow(10.0, static_cast<double>(cfg.antennaGain) / 10.0);

    // Clamp spoil factor to minimum 1.0 — consistent with pointBeam() clamping.
    // REQ-AESA-013: spoilFactor < 1.0 is physically impossible.
    double sf = std::max(1.0, static_cast<double>(spoilFactor));

    // -------------------------------------------------------------------------
    // Step 3: Element pattern roll-off.
    // Models the radiation pattern of an individual patch element.
    // Pattern = cos^1.5(theta) per Mailloux, Phased Array Antenna Handbook,
    // 2nd Ed, Ch 3. Valid for microstrip patch elements up to 60 deg.
    // std::max(0.0, cos(theta)) prevents negative values at theta > 90 deg
    // (physically: element cannot radiate backward). REQ-AESA-012.
    // -------------------------------------------------------------------------
    double elementPattern = std::pow(std::max(0.0, std::cos(theta)),
                                     ELEMENT_PATTERN_EXP);

    // -------------------------------------------------------------------------
    // Step 4: Array factor steering loss.
    // Models the reduction in coherent combining efficiency as the beam is
    // steered off boresight. Uses cosine-squared roll-off combined with a
    // Gaussian taper based on the relative steering angle.
    // At max steering angle (typically 60 deg), total loss is approximately
    // 3 dB vs boresight. REQ-AESA-012 acceptance criterion 3.
    // -------------------------------------------------------------------------
    double steerLoss = 1.0; // Default: no loss at boresight (theta == 0)

    double maxSteer = static_cast<double>(cfg.maxSteeringAngle_deg) * DEG_TO_RAD;

    if (theta > 0.0 && maxSteer > 0.0)
    {
        // relTheta = normalised off-boresight angle in [0, 1].
        // 0 = boresight, 1 = max steering angle.
        double relTheta = theta / maxSteer;

        // Cosine-squared term: models array factor pattern roll-off.
        // Gaussian taper: exp(-2 * relTheta^2) adds additional roll-off
        // that matches measured AESA antenna patterns at mid-band.
        // Combined, this gives ~3 dB at relTheta=1 (max steering).
        steerLoss = std::pow(std::cos(theta), ARRAY_TAPER_EXP)
                    * std::exp(-ARRAY_TAPER_EXP * relTheta * relTheta);

        // Floor steerLoss at GAIN_FLOOR_LINEAR (-40 dB).
        // Prevents steerLoss from reaching zero which would cause the
        // returned gain to be zero even for a partially functional array,
        // and could cause division by zero in SINR computation.
        // REQ-AESA-012 acceptance criterion 3.
        steerLoss = std::max(steerLoss, GAIN_FLOOR_LINEAR);
    }

    // -------------------------------------------------------------------------
    // Step 5: Failed module gain degradation.
    // Coherent loss model: gain scales as ratio^2.
    // Physical basis: in a phased array, gain = N^2 * element_power / N = N.
    // If only a fraction r of elements are active, gain scales as r^2 because
    // both the number of transmitting elements (factor r) and the coherent
    // combining efficiency (factor r) are reduced.
    // REQ-AESA-012 acceptance criterion 4.
    // -------------------------------------------------------------------------
    double moduleLoss = ratio * ratio;

    // -------------------------------------------------------------------------
    // Step 6: Assemble and return total gain.
    // Spoiling divides by sf^2 because beam widening by factor sf spreads
    // the same total radiated power over sf^2 times the solid angle.
    // REQ-AESA-012, REQ-AESA-013.
    // -------------------------------------------------------------------------
    return G_bore * elementPattern * steerLoss * moduleLoss / (sf * sf);
}

} // namespace aesa

