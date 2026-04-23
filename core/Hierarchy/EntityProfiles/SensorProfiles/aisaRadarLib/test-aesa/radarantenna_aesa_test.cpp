// =============================================================================
// FILE:         radarantenna_aesa_test.cpp
// MODULE:       AESA Antenna Beam Steering — Unit Tests
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage — all true/false paths exercised
//
// DESCRIPTION:  Requirements-based unit tests for RadarAntenna_AESA.
//               Every test case is traceable to a specific requirement and
//               a specific function header comment in radarantenna_aesa.h.
//               Tests are structured as: Arrange -> Act -> Assert.
//
// REQUIREMENTS COVERED:
//   REQ-AESA-010  Antenna beam pointing and steering
//   REQ-AESA-011  Beam reachability validation
//   REQ-AESA-012  Array gain computation with element pattern
//   REQ-AESA-013  Beam spoiling factor support
//   REQ-AESA-014  Scan boundary state management
//
// TEST CASE INDEX:
//   TC-AESA-ANT-001  reset() sets azimuth to 0.0
//   TC-AESA-ANT-002  reset() sets elevation to 0.0
//   TC-AESA-ANT-003  reset() sets spoilFactor to 1.0
//   TC-AESA-ANT-004  reset() clears scanBoundaryOccurred
//   TC-AESA-ANT-005  pointBeam() NaN azimuth replaced with 0.0
//   TC-AESA-ANT-006  pointBeam() Inf elevation replaced with 0.0
//   TC-AESA-ANT-007  pointBeam() az=270 wraps to -90 then clamped to -60
//                    NOTE: wrap normalises 270->-90 but -90 exceeds
//                    maxSteeringAngle=60 so clamping applies. Final stored
//                    value is -60, not -90. Both wrap AND clamp are exercised.
//   TC-AESA-ANT-008  pointBeam() az=-270 wraps to +90 then clamped to +60
//                    NOTE: same rationale as TC-AESA-ANT-007. Wrap then clamp.
//   TC-AESA-ANT-009  pointBeam() out-of-FoV az clamped to maxSteeringAngle
//   TC-AESA-ANT-010  pointBeam() spoilFactor < 1.0 clamped to 1.0
//   TC-AESA-ANT-011  pointBeam() effectiveBeamWidth = beamWidth * spoilFactor
//   TC-AESA-ANT-012  isReachable() boresight (0,0) always reachable
//   TC-AESA-ANT-013  isReachable() angle exactly at maxSteeringAngle reachable
//   TC-AESA-ANT-014  isReachable() angle beyond maxSteeringAngle not reachable
//   TC-AESA-ANT-015  computeSteeringAngle() co-pointed beams return ~0.0 deg
//                    NOTE: epsilon relaxed to 1e-6 — trig at non-zero az/el
//                    cannot guarantee exact 0.0 due to floating-point rounding.
//   TC-AESA-ANT-016  computeSteeringAngle() orthogonal beams return 90.0 deg
//   TC-AESA-ANT-017  computeSteeringAngle() anti-parallel beams return 180.0 deg
//   TC-AESA-ANT-018  computeArrayGain() zero active elements returns 0.0
//   TC-AESA-ANT-019  computeArrayGain() boresight returns G_bore
//   TC-AESA-ANT-020  computeArrayGain() spoilFactor=2 reduces gain by factor 4
//   TC-AESA-ANT-021  setScanBoundary() sets flag true
//   TC-AESA-ANT-022  clearScanBoundary() clears flag to false
//   TC-AESA-ANT-023  computeArrayGain() gain floored >= 0 at extreme angle
//   TC-AESA-ANT-024  pointBeam() valid in-FoV position accepted unchanged
//   TC-AESA-ANT-025  computeArrayGain() 50% module failure degrades gain by 0.25
//   TC-AESA-ANT-026  pointBeam() az=180 exactly on wrap boundary accepted
//   TC-AESA-ANT-027  pointBeam() az=-180 exactly on wrap boundary accepted
//   TC-AESA-ANT-028  isReachable() negative az within FoV is reachable
//   TC-AESA-ANT-029  computeSteeringAngle() boresight self-angle is ~0.0 deg
//                    Uses (0,0) vs (0,0) — avoids trig rounding at non-zero angles
//   TC-AESA-ANT-030  computeArrayGain() single failed module degrades gain < full
//
// CHANGE HISTORY:
//   Rev 1  20 Apr 2026  Initial test suite — DO-178C DAL B compliant.
//   Rev 2  20 Apr 2026  Fixed TC-AESA-ANT-007/008: corrected expected value
//                       to account for wrap-then-clamp behaviour (not wrap only).
//                       Fixed TC-AESA-ANT-015: epsilon relaxed from 1e-9 to 1e-6
//                       to tolerate floating-point trig rounding at non-zero angles.
//                       Fixed TC-AESA-ANT-025: removed inline lambda, replaced
//                       with explicit local variable for portability.
//                       Added TC-AESA-ANT-026 through TC-AESA-ANT-030 for
//                       additional boundary and negative-path coverage.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
// =============================================================================

#include "issst_test_framework.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib/radarantenna_aesa.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib/radarmodel_aesa.h"
#include <iostream>
#include <cmath>
#include <limits>

// Counter variables defined in the legacy build — declare extern here
// exactly as all other test translation units in the project do.
extern int testsPassed;
extern int testsFailed;
extern int tests;

// =============================================================================
// HELPER: buildDefaultAntennaConfig
//
// DESCRIPTION: Constructs a RadarConfig with known, deterministic defaults
//              for use in antenna unit tests. All fields are set explicitly
//              so test results are fully reproducible regardless of what
//              RadarConfig's own defaults may be.
//
// RETURNS:     Fully initialised RadarConfig suitable for antenna testing.
// REQUIREMENT: REQ-AESA-TEST-001
// =============================================================================
static aesa::RadarConfig buildDefaultAntennaConfig()
{
    aesa::RadarConfig cfg;

    // Array parameters — 1000 elements, none failed, standard efficiency
    cfg.numElements           = 1000;
    cfg.failedModules         = 0;
    cfg.peakPowerPerElement_W = 10.0f;
    cfg.moduleEfficiency      = 0.7f;

    // Antenna parameters — standard X-band AESA defaults
    cfg.antennaGain          = 34.0f;   // dBi — boresight peak gain
    cfg.beamWidth            = 3.0f;    // degrees — natural aperture beamwidth
    cfg.maxSteeringAngle_deg = 60.0f;   // degrees — electronic FoV half-angle

    // Elevation field of view
    cfg.minElevation = -10.0f;          // degrees
    cfg.maxElevation =  60.0f;          // degrees

    return cfg;
}

// =============================================================================
// TEST SUITE: test_radarantenna_reset
// Covers: TC-AESA-ANT-001 through TC-AESA-ANT-004
// Requirements: REQ-AESA-010, REQ-AESA-014
// =============================================================================
void test_radarantenna_reset()
{
    std::cout << "\n--- TC-AESA-ANT-001..004: reset() Tests ---" << std::endl;

    // Arrange: create antenna and put it in a non-default state to prove
    // reset() actually changes something, not just passes by doing nothing
    aesa::RadarAntenna_AESA ant;
    aesa::RadarConfig cfg = buildDefaultAntennaConfig();
    ant.pointBeam(30.0, 10.0, cfg, 2.0f);
    ant.setScanBoundary();

    // Act: reset to known safe state
    ant.reset();

    // TC-AESA-ANT-001: azimuth must be 0.0 after reset
    // REQ-AESA-010: beam state defined and safe after reset
    ASSERT_NEAR(ant.currentAzimuth(), 0.0, 1e-9,
                "TC-AESA-ANT-001: reset() sets currentAzimuth to 0.0");

    // TC-AESA-ANT-002: elevation must be 0.0 after reset
    ASSERT_NEAR(ant.currentElevation(), 0.0, 1e-9,
                "TC-AESA-ANT-002: reset() sets currentElevation to 0.0");

    // TC-AESA-ANT-003: spoilFactor must be 1.0 after reset
    // REQ-AESA-013: minimum spoil factor is 1.0 (no widening)
    ASSERT_NEAR((double)ant.currentSpoilFactor(), 1.0, 1e-6,
                "TC-AESA-ANT-003: reset() sets currentSpoilFactor to 1.0");

    // TC-AESA-ANT-004: scanBoundaryOccurred must be false after reset
    // REQ-AESA-014: boundary flag cleared at reset — no stale boundary event
    ASSERT_FALSE(ant.scanBoundaryOccurred(),
                 "TC-AESA-ANT-004: reset() clears scanBoundaryOccurred to false");
}

// =============================================================================
// TEST SUITE: test_radarantenna_pointBeam_sanitisation
// Covers: TC-AESA-ANT-005 through TC-AESA-ANT-011
// Requirements: REQ-AESA-010, REQ-AESA-011, REQ-AESA-013
// =============================================================================
void test_radarantenna_pointBeam_sanitisation()
{
    std::cout << "\n--- TC-AESA-ANT-005..011: pointBeam() Sanitisation Tests ---"
              << std::endl;

    aesa::RadarAntenna_AESA ant;
    aesa::RadarConfig cfg = buildDefaultAntennaConfig();

    // TC-AESA-ANT-005: NaN azimuth replaced with 0.0
    // REQ-AESA-010 acceptance criterion 1: non-finite az rejected, beam stays safe
    ant.reset();
    ant.pointBeam(std::numeric_limits<double>::quiet_NaN(), 5.0, cfg, 1.0f);
    ASSERT_NEAR(ant.currentAzimuth(), 0.0, 1e-9,
                "TC-AESA-ANT-005: NaN azimuth replaced with 0.0");

    // TC-AESA-ANT-006: Inf elevation replaced with 0.0
    // REQ-AESA-010 acceptance criterion 1: non-finite el rejected
    ant.reset();
    ant.pointBeam(10.0, std::numeric_limits<double>::infinity(), cfg, 1.0f);
    ASSERT_NEAR(ant.currentElevation(), 0.0, 1e-9,
                "TC-AESA-ANT-006: Inf elevation replaced with 0.0");

    // TC-AESA-ANT-007: az=270 wraps to -90, then clamped to -60
    // REQ-AESA-010 acceptance criterion 2: wrap normalisation applied
    // REQ-AESA-011: clamping applied after wrap when outside FoV
    // Sequence: 270 -> (270-360) = -90 -> clamp(-90, -60, +60) = -60
    // This test exercises BOTH the wrap branch AND the clamp branch.
    ant.reset();
    ant.pointBeam(270.0, 0.0, cfg, 1.0f);
    ASSERT_NEAR(ant.currentAzimuth(), -60.0, 1e-9,
                "TC-AESA-ANT-007: az=270 wraps to -90 then clamped to -60");

    // TC-AESA-ANT-008: az=-270 wraps to +90, then clamped to +60
    // Sequence: -270 -> (-270+360) = +90 -> clamp(+90, -60, +60) = +60
    ant.reset();
    ant.pointBeam(-270.0, 0.0, cfg, 1.0f);
    ASSERT_NEAR(ant.currentAzimuth(), 60.0, 1e-9,
                "TC-AESA-ANT-008: az=-270 wraps to +90 then clamped to +60");

    // TC-AESA-ANT-009: az=80 exceeds maxSteeringAngle=60, clamped to 60
    // REQ-AESA-011: unreachable positions clamped to nearest valid position
    // No wrap needed (80 is already in [-180,+180]).
    ant.reset();
    ant.pointBeam(80.0, 0.0, cfg, 1.0f);
    ASSERT_NEAR(ant.currentAzimuth(), 60.0, 1e-9,
                "TC-AESA-ANT-009: out-of-FoV az=80 clamped to maxSteeringAngle=60");

    // TC-AESA-ANT-010: spoilFactor=0.5 clamped to 1.0
    // REQ-AESA-013: beam cannot be narrower than natural beamwidth
    ant.reset();
    ant.pointBeam(0.0, 0.0, cfg, 0.5f);
    ASSERT_NEAR((double)ant.currentSpoilFactor(), 1.0, 1e-6,
                "TC-AESA-ANT-010: spoilFactor=0.5 clamped to 1.0");

    // TC-AESA-ANT-011: effectiveBeamWidth = beamWidth * spoilFactor
    // REQ-AESA-013: signal processor beam gate uses effectiveBeamWidth
    ant.reset();
    ant.pointBeam(0.0, 0.0, cfg, 3.0f);
    double expectedBW = static_cast<double>(cfg.beamWidth) * 3.0;
    ASSERT_NEAR(ant.effectiveBeamWidth(), expectedBW, 1e-9,
                "TC-AESA-ANT-011: effectiveBeamWidth = beamWidth * spoilFactor");
}

// =============================================================================
// TEST SUITE: test_radarantenna_isReachable
// Covers: TC-AESA-ANT-012 through TC-AESA-ANT-014, TC-AESA-ANT-028
// Requirements: REQ-AESA-011
// =============================================================================
void test_radarantenna_isReachable()
{
    std::cout << "\n--- TC-AESA-ANT-012..014,028: isReachable() Tests ---"
              << std::endl;

    aesa::RadarAntenna_AESA ant;
    aesa::RadarConfig cfg = buildDefaultAntennaConfig();
    // cfg.maxSteeringAngle_deg = 60.0

    // TC-AESA-ANT-012: boresight (0,0) is always reachable
    // REQ-AESA-011: steering angle at boresight is 0 deg — always within FoV
    ASSERT_TRUE(ant.isReachable(0.0, 0.0, cfg),
                "TC-AESA-ANT-012: boresight (0,0) is reachable");

    // TC-AESA-ANT-013: position at exactly maxSteeringAngle is reachable
    // REQ-AESA-011: boundary is inclusive (steer <= maxSteeringAngle)
    ASSERT_TRUE(ant.isReachable(60.0, 0.0, cfg),
                "TC-AESA-ANT-013: az=60.0 (=maxSteeringAngle) is reachable");

    // TC-AESA-ANT-014: position just beyond maxSteeringAngle is not reachable
    // REQ-AESA-011: positions outside FoV correctly rejected
    ASSERT_FALSE(ant.isReachable(61.0, 0.0, cfg),
                 "TC-AESA-ANT-014: az=61.0 (>maxSteeringAngle=60) is not reachable");

    // TC-AESA-ANT-028: negative azimuth within FoV is reachable
    // REQ-AESA-011: symmetric FoV — negative az treated identically to positive
    ASSERT_TRUE(ant.isReachable(-45.0, 0.0, cfg),
                "TC-AESA-ANT-028: az=-45 (within FoV) is reachable");
}

// =============================================================================
// TEST SUITE: test_radarantenna_computeSteeringAngle
// Covers: TC-AESA-ANT-015 through TC-AESA-ANT-017, TC-AESA-ANT-029
// Requirements: REQ-AESA-012
// =============================================================================
void test_radarantenna_computeSteeringAngle()
{
    std::cout << "\n--- TC-AESA-ANT-015..017,029: computeSteeringAngle() Tests ---"
              << std::endl;

    aesa::RadarAntenna_AESA ant;

    // TC-AESA-ANT-029: boresight self-angle at (0,0) vs (0,0) returns exactly 0.0
    // REQ-AESA-012 acceptance criterion 1: cos(0)*cos(0)*cos(0)+sin(0)*sin(0)=1.0
    // acos(1.0) = 0.0 exactly — no floating-point trig rounding at pure zero.
    // This is the reliable co-pointed test. TC-AESA-ANT-015 below tests at
    // non-zero angles where trig rounding requires a wider epsilon.
    double angle = ant.computeSteeringAngle(0.0, 0.0, 0.0, 0.0);
    ASSERT_NEAR(angle, 0.0, 1e-9,
                "TC-AESA-ANT-029: boresight self-angle (0,0)vs(0,0) returns 0.0 deg");

    // TC-AESA-ANT-015: co-pointed beams at non-zero az/el return ~0.0 degrees
    // REQ-AESA-012 acceptance criterion 1: same direction has zero separation
    // Epsilon is 1e-6 (not 1e-9) because cos/sin/acos chain at non-zero angles
    // accumulates floating-point rounding of order 1e-15 to 1e-7.
    angle = ant.computeSteeringAngle(30.0, 10.0, 30.0, 10.0);
    ASSERT_NEAR(angle, 0.0, 1e-6,
                "TC-AESA-ANT-015: co-pointed beams (30,10)vs(30,10) return ~0.0 deg");

    // TC-AESA-ANT-016: beam at (0,0) vs target at (90,0) returns 90.0 degrees
    // REQ-AESA-012 acceptance criterion 1: orthogonal unit vectors give 90 deg
    // cos(0)*cos(0)*cos(0-90) + sin(0)*sin(0) = 1*1*0 + 0 = 0 -> acos(0) = 90
    angle = ant.computeSteeringAngle(0.0, 0.0, 90.0, 0.0);
    ASSERT_NEAR(angle, 90.0, 1e-6,
                "TC-AESA-ANT-016: orthogonal beams (0,0)vs(90,0) return 90.0 deg");

    // TC-AESA-ANT-017: beam at (0,0) vs target at (180,0) returns 180.0 degrees
    // REQ-AESA-012 acceptance criterion 2: anti-parallel unit vectors give 180 deg
    // cos(0)*cos(0)*cos(0-180) + sin(0)*sin(0) = 1*1*(-1) + 0 = -1 -> acos(-1) = 180
    angle = ant.computeSteeringAngle(0.0, 0.0, 180.0, 0.0);
    ASSERT_NEAR(angle, 180.0, 1e-6,
                "TC-AESA-ANT-017: anti-parallel beams return 180.0 deg");
}

// =============================================================================
// TEST SUITE: test_radarantenna_computeArrayGain
// Covers: TC-AESA-ANT-018 through TC-AESA-ANT-020, TC-AESA-ANT-023,
//         TC-AESA-ANT-025, TC-AESA-ANT-030
// Requirements: REQ-AESA-012, REQ-AESA-013
// =============================================================================
void test_radarantenna_computeArrayGain()
{
    std::cout << "\n--- TC-AESA-ANT-018..020,023,025,030: computeArrayGain() Tests ---"
              << std::endl;

    aesa::RadarAntenna_AESA ant;
    aesa::RadarConfig cfg = buildDefaultAntennaConfig();

    // TC-AESA-ANT-018: zero active elements returns 0.0 exactly
    // REQ-AESA-012 acceptance criterion 1: dead array produces no gain
    // This guards the downstream SINR computation from a non-zero gain
    // value being returned when the array is physically inoperable.
    cfg.numElements   = 1000;
    cfg.failedModules = 1000;
    double gain = ant.computeArrayGain(0.0, cfg, 1.0f);
    ASSERT_NEAR(gain, 0.0, 1e-12,
                "TC-AESA-ANT-018: all modules failed returns gain=0.0");

    // TC-AESA-ANT-019: boresight (0 deg steering) with full array returns G_bore
    // REQ-AESA-012 acceptance criterion 3: at boresight, element pattern=1,
    // steerLoss=1, moduleLoss=1, spoil=1 so gain = G_bore exactly.
    cfg.numElements   = 1000;
    cfg.failedModules = 0;
    cfg.antennaGain   = 34.0f;
    double G_bore_expected = std::pow(10.0, 34.0 / 10.0);
    gain = ant.computeArrayGain(0.0, cfg, 1.0f);
    ASSERT_NEAR(gain, G_bore_expected, G_bore_expected * 0.001,
                "TC-AESA-ANT-019: boresight gain equals G_bore (linear)");

    // TC-AESA-ANT-020: spoilFactor=2 reduces gain by exactly factor 4 (1/sf^2)
    // REQ-AESA-013: gain reduction from spoiling = 1/(sf*sf)
    double gainNoSpoil   = ant.computeArrayGain(0.0, cfg, 1.0f);
    double gainWithSpoil = ant.computeArrayGain(0.0, cfg, 2.0f);
    ASSERT_NEAR(gainNoSpoil / gainWithSpoil, 4.0, 0.001,
                "TC-AESA-ANT-020: spoilFactor=2 reduces gain by factor 4");

    // TC-AESA-ANT-023: gain at extreme steering angle is non-negative
    // REQ-AESA-012 acceptance criterion 3: GAIN_FLOOR_LINEAR prevents zero/negative
    // At 89 deg the steerLoss floor at 1e-4 (-40 dB) is active.
    double gainExtreme = ant.computeArrayGain(89.0, cfg, 1.0f);
    ASSERT_TRUE(gainExtreme >= 0.0,
                "TC-AESA-ANT-023: gain at extreme angle is >= 0.0 (floor applied)");

    // TC-AESA-ANT-025: 50% module failure degrades boresight gain by factor 0.25
    // REQ-AESA-012 acceptance criterion 4: coherent loss = ratio^2
    // ratio = 500/1000 = 0.5, moduleLoss = 0.5^2 = 0.25
    // All other gain factors identical at boresight, so ratio of gains = 0.25.
    cfg.numElements   = 1000;
    cfg.failedModules = 0;
    double gainFullArray = ant.computeArrayGain(0.0, cfg, 1.0f);

    // Use explicit local variable — avoids inline lambda portability issues
    aesa::RadarConfig cfgHalfFailed = cfg;
    cfgHalfFailed.failedModules = 500;
    double gainHalfFailed = ant.computeArrayGain(0.0, cfgHalfFailed, 1.0f);

    ASSERT_NEAR(gainHalfFailed / gainFullArray, 0.25, 0.001,
                "TC-AESA-ANT-025: 50% module failure reduces gain by factor 0.25");

    // TC-AESA-ANT-030: single failed module produces gain slightly less than full
    // REQ-AESA-012: any module failure must reduce gain, not leave it unchanged
    aesa::RadarConfig cfgOneFailed = cfg;
    cfgOneFailed.failedModules = 1;
    double gainOneFailed = ant.computeArrayGain(0.0, cfgOneFailed, 1.0f);
    ASSERT_TRUE(gainOneFailed < gainFullArray,
                "TC-AESA-ANT-030: single failed module produces gain < full array gain");
}

// =============================================================================
// TEST SUITE: test_radarantenna_scanBoundary
// Covers: TC-AESA-ANT-021 through TC-AESA-ANT-022
// Requirements: REQ-AESA-014
// =============================================================================
void test_radarantenna_scanBoundary()
{
    std::cout << "\n--- TC-AESA-ANT-021..022: Scan Boundary Tests ---" << std::endl;

    aesa::RadarAntenna_AESA ant;

    // Pre-condition: flag is false before any set — confirms reset state
    ant.reset();
    ASSERT_FALSE(ant.scanBoundaryOccurred(),
                 "TC-AESA-ANT-021 (pre): scanBoundaryOccurred is false before set");

    // TC-AESA-ANT-021: setScanBoundary() sets flag to true
    // REQ-AESA-014: scheduler sets this flag when scan cycle completes
    ant.setScanBoundary();
    ASSERT_TRUE(ant.scanBoundaryOccurred(),
                "TC-AESA-ANT-021: setScanBoundary() sets flag to true");

    // TC-AESA-ANT-022: clearScanBoundary() resets flag to false
    // REQ-AESA-014: output assembly stage clears flag after consuming it
    ant.clearScanBoundary();
    ASSERT_FALSE(ant.scanBoundaryOccurred(),
                 "TC-AESA-ANT-022: clearScanBoundary() clears flag to false");
}

// =============================================================================
// TEST SUITE: test_radarantenna_pointBeam_validPosition
// Covers: TC-AESA-ANT-024, TC-AESA-ANT-026, TC-AESA-ANT-027
// Requirements: REQ-AESA-010
// =============================================================================
void test_radarantenna_pointBeam_validPosition()
{
    std::cout << "\n--- TC-AESA-ANT-024,026,027: pointBeam() Valid Position Tests ---"
              << std::endl;

    aesa::RadarAntenna_AESA ant;
    aesa::RadarConfig cfg = buildDefaultAntennaConfig();

    // TC-AESA-ANT-024: valid in-FoV position stored unchanged
    // REQ-AESA-010: valid commands accepted without modification
    // az=30, el=10 — both within FoV (maxSteeringAngle=60, maxEl=60)
    ant.pointBeam(30.0, 10.0, cfg, 1.0f);
    ASSERT_NEAR(ant.currentAzimuth(),   30.0, 1e-9,
                "TC-AESA-ANT-024: valid az=30 stored unchanged");
    ASSERT_NEAR(ant.currentElevation(), 10.0, 1e-9,
                "TC-AESA-ANT-024: valid el=10 stored unchanged");

    // TC-AESA-ANT-026: az=180 exactly on the wrap boundary accepted as-is
    // REQ-AESA-010 acceptance criterion 2: 180 does not trigger the wrap
    // (condition is az > 180, not az >= 180). Then isReachable rejects it
    // (steer at az=180 el=0 is 180 deg >> maxSteeringAngle=60), so it gets
    // clamped. Final value should be 60 (max azimuth clamp).
    ant.reset();
    ant.pointBeam(180.0, 0.0, cfg, 1.0f);
    ASSERT_NEAR(ant.currentAzimuth(), 60.0, 1e-9,
                "TC-AESA-ANT-026: az=180 at wrap boundary clamped to maxSteeringAngle");

    // TC-AESA-ANT-027: az=-180 exactly on the lower wrap boundary
    // Symmetric to TC-AESA-ANT-026. -180 does not trigger wrap (condition
    // is az < -180). isReachable rejects it, clamp gives -60.
    ant.reset();
    ant.pointBeam(-180.0, 0.0, cfg, 1.0f);
    ASSERT_NEAR(ant.currentAzimuth(), -60.0, 1e-9,
                "TC-AESA-ANT-027: az=-180 at wrap boundary clamped to -maxSteeringAngle");
}

// =============================================================================
// ENTRY POINT: radarAntenna_test
//
// DESCRIPTION: Called from Core_Test::Core_Test() alongside all other test
//              suites. Resets counters before running, prints suite summary
//              after. Counter accumulation into totals is handled by the
//              caller in core_test.cpp — do not accumulate here.
//
// REQUIREMENT: REQ-AESA-TEST-001
// =============================================================================
void radarAntenna_test()
{
    // Reset counters at suite entry — same pattern as aesaRadar_test()
    testsPassed = 0;
    testsFailed = 0;
    tests       = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "   RADARANTENNA_AESA UNIT TESTS          " << std::endl;
    std::cout << "   Standard: DO-178C DAL B               " << std::endl;
    std::cout << "   Project:  ISSST                       " << std::endl;
    std::cout << "   Org:      Oxygen to Innovation Pvt. Ltd." << std::endl;
    std::cout << "=========================================" << std::endl;

    test_radarantenna_reset();
    test_radarantenna_pointBeam_sanitisation();
    test_radarantenna_isReachable();
    test_radarantenna_computeSteeringAngle();
    test_radarantenna_computeArrayGain();
    test_radarantenna_scanBoundary();
    test_radarantenna_pointBeam_validPosition();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "ANTENNA TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
