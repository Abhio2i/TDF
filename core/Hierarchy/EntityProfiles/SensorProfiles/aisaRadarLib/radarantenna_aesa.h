// =============================================================================
// FILE:         radarantenna_aesa.h
// MODULE:       AESA Antenna Beam Steering
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Declares the RadarAntenna_AESA class which implements
//               electronic beam steering, reachability validation, steering
//               angle computation, and array gain computation for an Active
//               Electronically Scanned Array (AESA) radar antenna model.
//               Beam spoiling (widening) is supported via spoilFactor.
//               All methods are pure or near-pure — no external I/O.
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
//                       pattern with cosine-squared array factor roll-off.
//                       Dead array guard added. Azimuth normalisation added.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Commented-out code removed per NS-05.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#pragma once
#ifndef RADARANTENNA_AESA_H
#define RADARANTENNA_AESA_H

#include "radarmodel_aesa.h"

namespace aesa {

// =============================================================================
// CLASS: RadarAntenna_AESA
//
// DESCRIPTION:  Models the electronic beam steering and gain characteristics
//               of an AESA radar antenna. Maintains current beam pointing
//               state (azimuth, elevation, spoil factor, effective beam width)
//               and provides computation methods for reachability checking,
//               steering angle measurement between two pointing directions,
//               and array gain at a given off-boresight angle.
//
//               All gain computations use the incoherent mean Physical Optics
//               model appropriate for entity-level simulation time steps.
//
// REQUIREMENTS: REQ-AESA-010 through REQ-AESA-014
//
// THREAD SAFETY: Not thread-safe. The owning RadarModel_AESA serialises
//                all access via its internal mutex. Do not call from multiple
//                threads without external synchronisation.
//
// TRACEABILITY:
//   Test suite:  test_radarantenna_aesa (radarantenna_aesa_test.cpp)
//   Test cases:  TC-AESA-ANT-001 through TC-AESA-ANT-020
// =============================================================================
class RadarAntenna_AESA
{
public:

    // =========================================================================
    // CONSTRUCTOR: RadarAntenna_AESA (default)
    //
    // DESCRIPTION: Default constructor. All member variables are initialised
    //              by their in-class initialisers. No resources are acquired.
    //              Call reset() before first operational use if the object is
    //              reused across radar init/start cycles.
    //
    // REQUIREMENT: REQ-AESA-010
    // =========================================================================
    RadarAntenna_AESA() = default;

    // =========================================================================
    // FUNCTION:    reset
    //
    // DESCRIPTION: Resets all beam state variables to their safe default
    //              values. Must be called during radar initialisation and
    //              restart to guarantee a known antenna state before the
    //              first beam command is issued.
    //
    // REQUIREMENT: REQ-AESA-010
    //              REQ-AESA-014
    //
    // PARAMETERS:  None.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Writes currentAzimuth_, currentElevation_,
    //               currentSpoilFactor_, effectiveBeamWidth_,
    //               scanBoundaryOccurred_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-ANT-001  reset() sets azimuth to 0.0
    //               TC-AESA-ANT-002  reset() sets elevation to 0.0
    //               TC-AESA-ANT-003  reset() sets spoilFactor to 1.0
    //               TC-AESA-ANT-004  reset() clears scanBoundaryOccurred
    // =========================================================================
    void reset();

    // =========================================================================
    // FUNCTION:    pointBeam
    //
    // DESCRIPTION: Commands the antenna to point its beam at the specified
    //              azimuth and elevation in body-frame coordinates. Input
    //              values are sanitised (NaN/Inf rejection, azimuth wrap
    //              normalisation) before reachability is evaluated. If the
    //              commanded position is outside the electronically steerable
    //              field of view, it is clamped to the nearest valid position.
    //              Effective beam width is updated from beamWidth * spoilFactor.
    //
    // REQUIREMENT: REQ-AESA-010  Beam pointing command
    //              REQ-AESA-011  Reachability clamping
    //              REQ-AESA-013  Beam spoiling factor application
    //
    // PARAMETERS:
    //   az_deg     [in/modified]  Commanded azimuth in body frame (degrees).
    //                             Valid range: any finite double.
    //                             Non-finite values are replaced with 0.0.
    //                             Wrapped to [-180, +180] before reachability
    //                             check. Must not be NaN or Inf on entry —
    //                             guarded internally per REQ-AESA-010.
    //
    //   el_deg     [in/modified]  Commanded elevation in body frame (degrees).
    //                             Valid range: any finite double.
    //                             Non-finite values are replaced with 0.0.
    //
    //   cfg        [in]           Radar configuration struct. Must be valid
    //                             and fully initialised. Key fields used:
    //                               cfg.maxSteeringAngle_deg — FoV limit
    //                               cfg.minElevation         — elevation floor
    //                               cfg.maxElevation         — elevation ceiling
    //                               cfg.beamWidth            — natural beamwidth
    //
    //   spoilFactor [in]          Beam spoiling multiplier. Range: >= 1.0.
    //                             Values < 1.0 are clamped to 1.0.
    //                             Dimensionless. Default: 1.0 (no spoiling).
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Updates currentAzimuth_, currentElevation_,
    //               currentSpoilFactor_, effectiveBeamWidth_.
    //
    // ASSUMPTIONS: cfg is valid and initialised by caller. No internal
    //              validation of cfg fields is performed beyond what
    //              isReachable() requires.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-ANT-005  NaN azimuth clamped to 0.0
    //               TC-AESA-ANT-006  Inf elevation clamped to 0.0
    //               TC-AESA-ANT-007  Az > 180 normalised correctly
    //               TC-AESA-ANT-008  Az < -180 normalised correctly
    //               TC-AESA-ANT-009  Out-of-FoV az clamped to maxSteeringAngle
    //               TC-AESA-ANT-010  spoilFactor < 1.0 clamped to 1.0
    //               TC-AESA-ANT-011  effectiveBeamWidth = beamWidth * spoilFactor
    // =========================================================================
    void pointBeam(double az_deg, double el_deg,
                   const RadarConfig& cfg,
                   float spoilFactor = 1.0f);

    // =========================================================================
    // FUNCTION:    isReachable
    //
    // DESCRIPTION: Determines whether a given azimuth/elevation position is
    //              within the electronically steerable field of view of the
    //              AESA array. Uses the cosine space steering angle metric:
    //              the true off-boresight angle is computed from the direction
    //              cosine of the beam unit vector, and compared against
    //              cfg.maxSteeringAngle_deg.
    //
    //              This method uses the direction cosine bx = cos(el)*cos(az)
    //              which is the projection of the beam unit vector onto the
    //              boresight axis. The steering angle is acos(bx).
    //
    // REQUIREMENT: REQ-AESA-011
    //
    // PARAMETERS:
    //   az_deg  [in]  Azimuth to test (degrees, body frame).
    //                 Expected range: [-180, +180]. Values outside this range
    //                 produce correct results due to trigonometric periodicity
    //                 but should be normalised by caller for clarity.
    //
    //   el_deg  [in]  Elevation to test (degrees, body frame).
    //                 Expected range: [-90, +90].
    //
    //   cfg     [in]  Radar configuration. cfg.maxSteeringAngle_deg must be
    //                 in range [0, 90] degrees. Values outside this range
    //                 produce undefined comparison behaviour.
    //
    // RETURNS:    true  — beam position is within the steerable FoV.
    //             false — beam position exceeds the steering angle limit.
    //
    // SIDE EFFECTS: None. Pure const query.
    //
    // ALGORITHM:  1. Convert az_deg, el_deg to radians.
    //             2. Compute direction cosine bx = cos(el) * cos(az).
    //             3. Compute steering angle = acos(clamp(bx, -1, 1)) * 180/pi.
    //             4. Return steer <= maxSteeringAngle_deg.
    //             clamp guards against floating-point values marginally
    //             outside [-1, 1] which would cause acos to return NaN.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-ANT-012  Boresight (0,0) always reachable
    //               TC-AESA-ANT-013  Angle at exactly maxSteeringAngle reachable
    //               TC-AESA-ANT-014  Angle beyond maxSteeringAngle not reachable
    // =========================================================================
    bool isReachable(double az_deg, double el_deg,
                     const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    computeSteeringAngle
    //
    // DESCRIPTION: Computes the angular separation (degrees) between two
    //              pointing directions expressed as azimuth/elevation pairs.
    //              Used to determine how far off-boresight a target is relative
    //              to the current beam position, for use in gain computation.
    //
    //              Uses the spherical dot-product formula for angular separation
    //              between two unit vectors on the unit sphere.
    //
    // REQUIREMENT: REQ-AESA-012
    //
    // PARAMETERS:
    //   beamAz    [in]  Current beam azimuth (degrees, body frame).
    //   beamEl    [in]  Current beam elevation (degrees, body frame).
    //   targetAz  [in]  Target azimuth (degrees, body frame).
    //   targetEl  [in]  Target elevation (degrees, body frame).
    //                   All parameters: finite doubles expected.
    //                   Non-finite inputs produce NaN output — caller
    //                   must validate inputs per REQ-AESA-010.
    //
    // RETURNS:    Angular separation in degrees. Range: [0.0, 180.0].
    //             Returns 0.0 when beam and target are co-pointed.
    //             Returns 180.0 when they are anti-parallel.
    //
    // SIDE EFFECTS: None. Pure const computation.
    //
    // ALGORITHM:  dot = cos(e1)*cos(e2)*cos(a1-a2) + sin(e1)*sin(e2)
    //             angle = acos(clamp(dot, -1, 1)) * 180/pi
    //             This is the great-circle angular distance formula.
    //             clamp prevents NaN from floating-point rounding at
    //             dot = +/-1.0 exactly.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-ANT-015  Co-pointed beams return 0.0 degrees
    //               TC-AESA-ANT-016  Orthogonal beams return 90.0 degrees
    //               TC-AESA-ANT-017  Anti-parallel beams return 180.0 degrees
    // =========================================================================
    double computeSteeringAngle(double beamAz, double beamEl,
                                double targetAz, double targetEl) const;

    // =========================================================================
    // FUNCTION:    computeArrayGain
    //
    // DESCRIPTION: Computes the effective array gain (linear, not dB) of the
    //              AESA antenna at a given off-boresight steering angle.
    //              Models three gain reduction mechanisms:
    //
    //              1. Element pattern roll-off: cos^1.5(theta) — standard
    //                 for microstrip patch elements (Mailloux, Ch 3).
    //
    //              2. Array factor roll-off: cosine-squared roll-off combined
    //                 with Gaussian taper — models realistic phased array
    //                 gain reduction as beam is steered off boresight.
    //                 At max steering angle (typically 60 deg), gain drops
    //                 approximately 3 dB vs boresight.
    //
    //              3. Failed module degradation: coherent loss model where
    //                 gain scales as (active/total)^2, representing the
    //                 reduction in both aperture and coherent combining.
    //
    //              Beam spoiling by spoilFactor sf reduces gain by 1/(sf*sf)
    //              because spoiling widens the beam — energy is spread over
    //              a larger solid angle.
    //
    // REQUIREMENT: REQ-AESA-012  Array gain computation
    //              REQ-AESA-013  Beam spoiling gain reduction
    //
    // PARAMETERS:
    //   steeringAngle_deg [in]  Off-boresight angle from current beam centre
    //                           to the target (degrees). Range: [0, 90].
    //                           Values beyond maxSteeringAngle produce gain
    //                           at or near the floor value (REQ-AESA-012).
    //                           Must be finite — non-finite input produces
    //                           undefined output.
    //
    //   cfg               [in]  Radar configuration struct. Key fields:
    //                             cfg.numElements           — total T/R count
    //                             cfg.failedModules         — failed T/R count
    //                             cfg.antennaGain           — boresight gain (dBi)
    //                             cfg.maxSteeringAngle_deg  — FoV limit (deg)
    //                           cfg.numElements must be >= 0.
    //                           cfg.failedModules must be <= cfg.numElements.
    //
    //   spoilFactor       [in]  Beam spoiling multiplier. Range: >= 1.0.
    //                           Values < 1.0 are clamped to 1.0 internally.
    //                           Dimensionless. Default: 1.0.
    //
    // RETURNS:    Effective array gain (linear, dimensionless).
    //             Range: [0.0, G_boresight].
    //             Returns 0.0 exactly when all T/R modules have failed
    //             (active == 0). This is a hard guarantee per REQ-AESA-012.
    //             Minimum non-zero gain is floored at GAIN_FLOOR_LINEAR
    //             (-40 dB) via steerLoss floor, preventing division by zero
    //             in downstream SINR computation.
    //
    // SIDE EFFECTS: None. Pure const computation.
    //
    // ASSUMPTIONS: cfg.failedModules does not exceed cfg.numElements.
    //              If it does, active is clamped to 0 via std::max, and
    //              the function returns 0.0 safely.
    //
    // LIMITATIONS: Gain model is valid for patch element arrays steered
    //              up to 60 degrees off boresight. Beyond 60 degrees the
    //              cos^1.5 element pattern approximation loses accuracy.
    //              Documented in ICD-AESA-ANTENNA-001.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-ANT-018  Zero active elements returns 0.0 gain
    //               TC-AESA-ANT-019  Boresight (0 deg) returns G_boresight * moduleLoss
    //               TC-AESA-ANT-020  spoilFactor=2 reduces gain by factor of 4
    // =========================================================================
    double computeArrayGain(double steeringAngle_deg,
                            const RadarConfig& cfg,
                            float spoilFactor = 1.0f) const;

    // =========================================================================
    // ACCESSORS — read-only state queries
    // All return by value. No side effects. Thread-safe for reading if
    // no concurrent write is in progress (caller must ensure this).
    // Requirements: REQ-AESA-010, REQ-AESA-014
    // =========================================================================

    // Returns current beam azimuth in body frame (degrees, range [-180, +180])
    double currentAzimuth()       const { return currentAzimuth_;      }

    // Returns current beam elevation in body frame (degrees)
    double currentElevation()     const { return currentElevation_;     }

    // Returns current beam spoil factor (dimensionless, >= 1.0)
    float  currentSpoilFactor()   const { return currentSpoilFactor_;   }

    // Returns effective beam width after spoiling (degrees)
    // effectiveBeamWidth = cfg.beamWidth * currentSpoilFactor
    double effectiveBeamWidth()   const { return effectiveBeamWidth_;   }

    // Returns true if a scan boundary event occurred since last clearScanBoundary()
    // REQ-AESA-014: scan boundary state is set by scheduler, cleared after output
    bool   scanBoundaryOccurred() const { return scanBoundaryOccurred_; }

    // =========================================================================
    // FUNCTION:    setScanBoundary
    // DESCRIPTION: Marks that a scan boundary event has occurred. Called by
    //              RadarScheduler when the beam completes one full scan cycle.
    // REQUIREMENT: REQ-AESA-014
    // SIDE EFFECTS: Sets scanBoundaryOccurred_ = true.
    // TRACEABILITY: TC-AESA-ANT-004
    // =========================================================================
    void setScanBoundary()   { scanBoundaryOccurred_ = true;  }

    // =========================================================================
    // FUNCTION:    clearScanBoundary
    // DESCRIPTION: Clears the scan boundary flag after it has been consumed
    //              by the output assembly stage. Must be called once per tick
    //              after the flag has been read.
    // REQUIREMENT: REQ-AESA-014
    // SIDE EFFECTS: Sets scanBoundaryOccurred_ = false.
    // TRACEABILITY: TC-AESA-ANT-004
    // =========================================================================
    void clearScanBoundary() { scanBoundaryOccurred_ = false; }

private:

    // =========================================================================
    // PRIVATE MEMBER VARIABLES
    // All initialised by in-class initialisers. reset() must be called
    // before operational use to guarantee clean state after construction.
    // Requirements: REQ-AESA-010, REQ-AESA-014
    // =========================================================================

    // Current beam azimuth in body frame (degrees). Range: [-180, +180].
    // Updated only by pointBeam(). Default: 0.0 (boresight).
    double currentAzimuth_       = 0.0;

    // Current beam elevation in body frame (degrees).
    // Updated only by pointBeam(). Default: 0.0 (horizontal).
    double currentElevation_     = 0.0;

    // Current beam spoiling factor (dimensionless). Range: [1.0, inf).
    // 1.0 = no spoiling (natural beamwidth). Updated only by pointBeam().
    float  currentSpoilFactor_   = 1.0f;

    // Effective beam width after spoiling (degrees).
    // = cfg.beamWidth * currentSpoilFactor_. Updated only by pointBeam().
    double effectiveBeamWidth_   = 0.0;

    // Scan boundary flag. Set by setScanBoundary(), cleared by clearScanBoundary().
    // true = beam has completed one full search scan cycle this tick.
    bool   scanBoundaryOccurred_ = false;
};

} // namespace aesa

#endif // RADARANTENNA_AESA_H
// #pragma once
// #ifndef RADARANTENNA_AESA_H
// #define RADARANTENNA_AESA_H
// // =============================================================================
// // radarantenna_aesa.h  —  Electronic beam steering + FIX-13 beam spoiling
// // =============================================================================

// #include "radarmodel_aesa.h"

// namespace aesa {

// class RadarAntenna_AESA
// {
// public:
//     RadarAntenna_AESA() = default;

//     void reset();

//     // FIX-13: spoilFactor > 1 widens effective beam
//     void   pointBeam(double az_deg, double el_deg,
//                      const RadarConfig& cfg,
//                      float spoilFactor = 1.0f);

//     bool   isReachable(double az_deg, double el_deg,
//                        const RadarConfig& cfg) const;

//     double computeSteeringAngle(double beamAz, double beamEl,
//                                 double targetAz, double targetEl) const;

//     // FIX-13: spoilFactor reduces gain by 1/sf²
//     double computeArrayGain(double steeringAngle_deg,
//                             const RadarConfig& cfg,
//                             float spoilFactor = 1.0f) const;

//     double currentAzimuth()       const { return currentAzimuth_;      }
//     double currentElevation()     const { return currentElevation_;     }
//     float  currentSpoilFactor()   const { return currentSpoilFactor_;   }
//     double effectiveBeamWidth()   const { return effectiveBeamWidth_;   }
//     bool   scanBoundaryOccurred() const { return scanBoundaryOccurred_; }

//     void setScanBoundary()   { scanBoundaryOccurred_ = true;  }
//     void clearScanBoundary() { scanBoundaryOccurred_ = false; }

// private:
//     double currentAzimuth_       = 0.0;
//     double currentElevation_     = 0.0;
//     float  currentSpoilFactor_   = 1.0f;
//     double effectiveBeamWidth_   = 0.0;
//     bool   scanBoundaryOccurred_ = false;
// };

// } // namespace aesa
// #endif
