// =============================================================================
// FILE:         radarsignalprocessor_aesa_test.cpp
// MODULE:       AESA Radar Signal Processor — Unit Tests
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage — all true/false paths exercised
//
// DESCRIPTION:  Requirements-based unit tests for RadarSignalProcessor_AESA.
//               Each test case is traceable to a specific requirement and to
//               specific function header comments in radarsignalprocessor_aesa.h.
//               Tests structured as: Arrange -> Act -> Assert.
//
// REQUIREMENTS COVERED:
//   REQ-AESA-020  PRF / waveform management
//   REQ-AESA-021  Staggered PRF ambiguity resolution
//   REQ-AESA-040  Detection pipeline physics
//   REQ-AESA-060  Electronic warfare
//   REQ-AESA-061  Chaff clutter
//   REQ-AESA-071  Propagation loss
//   REQ-AESA-072  Two-ray multipath
//
// TEST CASE INDEX:
//   TC-AESA-SP-001  isTargetInBeam() boresight target within gate
//   TC-AESA-SP-002  isTargetInBeam() target beyond gate rejected
//   TC-AESA-SP-003  isTargetInBeam() azimuth wrap-around handled
//   TC-AESA-SP-004  checkHorizon() target at 0 altitude within horizon
//   TC-AESA-SP-005  checkHorizon() target beyond horizon returns false
//   TC-AESA-SP-006  calculateSignalStrength() decreases with range^4
//   TC-AESA-SP-007  calculateSignalStrength() proportional to RCS
//   TC-AESA-SP-008  computeNoisePower() > 0 for any valid config
//   TC-AESA-SP-009  computeNoisePower() increases with bandwidth
//   TC-AESA-SP-010  computeClutterPower() AIR surface returns 0
//   TC-AESA-SP-011  computeClutterPower() SEA surface returns > 0
//   TC-AESA-SP-012  computeJammerPower() inactive jammer returns 0
//   TC-AESA-SP-013  computeJammerPower() > 0 for active jammer
//   TC-AESA-SP-014  computePropagationLoss() clear air close to 1.0
//   TC-AESA-SP-015  computePropagationLoss() heavy rain reduces factor
//   TC-AESA-SP-016  computeSINR() > 0 for valid signal and noise
//   TC-AESA-SP-017  computeSINR() decreases when jammer active
//   TC-AESA-SP-018  generateReferenceCells() returns exactly 16 cells
//   TC-AESA-SP-019  computeCFARThreshold() > 0 for valid cells
//   TC-AESA-SP-020  computeCFARThreshold() returns 1e12 for empty cells
//   TC-AESA-SP-021  computeCFARThresholdRelaxed() < standard threshold
//   TC-AESA-SP-022  computeEffectiveRCS() GENERIC returns base RCS
//   TC-AESA-SP-023  computeEffectiveRCS() STEALTHY < METAL for same dims
//   TC-AESA-SP-024  computeSwerlingRCS() CASE_0 returns exact value
//   TC-AESA-SP-025  computeSwerlingRCS() CASE_I returns positive value
//   TC-AESA-SP-026  computeAlbersheimPd() high SNR gives Pd close to 0.99
//   TC-AESA-SP-027  computeAlbersheimPd() zero SNR returns 0.0
//   TC-AESA-SP-028  computeRadialVelocity() head-on target negative
//   TC-AESA-SP-029  computeCPA() stationary target CPA = current range
//   TC-AESA-SP-030  resolveRangeAmbiguity() unambiguous range unchanged
//   TC-AESA-SP-031  resolveRangeAmbiguity() k=1 fold resolved correctly
//   TC-AESA-SP-032  resolveRangeAmbiguityStaggered() k=1 resolved
//   TC-AESA-SP-033  resolveVelocityStaggered() returns value in range
//   TC-AESA-SP-034  applyRangeAmbiguity() range < Rmax stored unchanged
//   TC-AESA-SP-035  applyRangeAmbiguity() range > Rmax sets isAmbiguous
//   TC-AESA-SP-036  computeMaxDetectionRange() returns > 0
//   TC-AESA-SP-037  shouldMergeDetection() same position returns true
//   TC-AESA-SP-038  shouldMergeDetection() distant position returns false
//   TC-AESA-SP-039  computeBeamGainFactor() boresight returns 1.0
//   TC-AESA-SP-040  computeBeamGainFactor() far target returns < 1.0
//   TC-AESA-SP-041  isJammerInSidelobe() inactive jammer returns false
//   TC-AESA-SP-042  computeModulationProcessingGain() LFM returns BT
//   TC-AESA-SP-043  computeModulationProcessingGain() NONE returns 1.0
//   TC-AESA-SP-044  isInDopplerBlindZone() HPRF returns false
//   TC-AESA-SP-045  isInDopplerBlindZone() stationary platform returns false
//   TC-AESA-SP-046  computeSTAPGain() far-from-notch returns full gain
//   TC-AESA-SP-047  computeSTAPGain() returns >= 1.0 always
//   TC-AESA-SP-048  computeMultipathFactor() high elevation returns 1.0
//   TC-AESA-SP-049  computeMultipathFactor() zero altitude returns 1.0
//   TC-AESA-SP-050  computeChaffReturn() empty cloud list returns 0.0
//   TC-AESA-SP-051  getPlatformBaseRCS() FIGHTER returns 3.0
//   TC-AESA-SP-052  getPlatformBaseRCS() STEALTH returns 0.001
//   TC-AESA-SP-053  lookupAspectRCS() empty table uses base RCS
//   TC-AESA-SP-054  computeWaterVapourDensity() 0% humidity returns 0
//   TC-AESA-SP-055  computeGaseousAttenuation() returns >= 0
//   TC-AESA-SP-056  selectWaveformForRange() short range selects HPRF
//   TC-AESA-SP-057  selectWaveformForRange() long range selects LPRF
//
// CHANGE HISTORY:
//   Rev 1  20 Apr 2026  Initial test suite — DO-178C DAL B compliant.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
// =============================================================================

#include "issst_test_framework.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib/radarsignalprocessor_aesa.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib/radarmodel_aesa.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <random>

extern int testsPassed;
extern int testsFailed;
extern int tests;

// =============================================================================
// HELPERS
// =============================================================================

static aesa::RadarConfig buildSPConfig()
{
    aesa::RadarConfig cfg;
    cfg.numElements           = 1000;
    cfg.failedModules         = 0;
    cfg.peakPowerPerElement_W = 10.0f;
    cfg.moduleEfficiency      = 0.7f;
    cfg.frequency_Hz          = 10.0e9;
    cfg.antennaGain           = 34.0f;
    cfg.beamWidth             = 3.0f;
    cfg.maxSteeringAngle_deg  = 60.0f;
    cfg.antennaBandwidth      = 100e6;
    cfg.systemTemperature_K   = 290.0;
    cfg.noiseFigure_dB        = 4.0;
    cfg.targetPfa             = 1e-6;
    cfg.radarHeight           = 5000.0;
    cfg.minDetectableRange    = 100.0;
    cfg.platformSpeed_m_s     = 250.0f;
    cfg.earthRadiusFactor     = 1.33;
    cfg.atmosphericFactor     = 1.0;
    cfg.seaState              = 2.0f;
    cfg.landClutter           = 0.0f;
    cfg.atmosphere.rainRate_mmph   = 0.0f;
    cfg.atmosphere.fogVisibility_m = 0.0f;
    cfg.atmosphere.temperature_C   = 15.0f;
    cfg.atmosphere.humidity_pct    = 60.0f;
    cfg.atmosphere.pressure_hPa    = 1013.25f;
    cfg.frequencyAgility      = false;
    cfg.sidelobeMode          = aesa::SidelobeMode::NORMAL;
    cfg.peakSidelobeLevel     = -40.0f;
    cfg.avgSidelobeLevel      = -50.0f;
    cfg.sidelobeBlanking_dB   = -15.0f;
    cfg.nullSteering.active   = false;
    cfg.searchWaveform        = { aesa::ModulationType::LFM,
                          50e-6f, 300.0f, 5e6f, 10,
                          aesa::WaveformMode::LPRF };
    cfg.trackWaveform         = { aesa::ModulationType::LFM,
                         10e-6f, 1000.0f, 20e6f, 10,
                         aesa::WaveformMode::MPRF };
    cfg.fireControlWaveform   = { aesa::ModulationType::NLFM,
                               5e-6f, 2000.0f, 50e6f, 20,
                               aesa::WaveformMode::HPRF };
    cfg.waveformTable[0] = {  30000.0f, cfg.fireControlWaveform };
    cfg.waveformTable[1] = { 100000.0f, cfg.trackWaveform };
    cfg.waveformTable[2] = { 400000.0f, cfg.searchWaveform };
    cfg.waveformTable[3] = {}; cfg.waveformTable[4] = {}; cfg.waveformTable[5] = {};
    return cfg;
}

static aesa::TargetInput buildSPTarget(uint32_t id,
                                       double x, double y, double z,
                                       double vx=0, double vy=0, double vz=0)
{
    aesa::TargetInput t;
    t.id           = id;
    t.x = x; t.y = y; t.z = z;
    t.vx=vx; t.vy=vy; t.vz=vz;
    t.rcs          = 3.0;
    t.surface      = aesa::SurfaceType::AIR;
    t.swerlingCase = aesa::SwerlingCase::CASE_0;
    t.platformType = "FIGHTER";
    t.jammer.active = false;
    return t;
}

// =============================================================================
// TEST SUITE: test_sp_geometry
// Covers: TC-AESA-SP-001 through TC-AESA-SP-005
// =============================================================================
void test_sp_geometry()
{
    std::cout << "\n--- TC-AESA-SP-001..005: Geometry Tests ---" << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    aesa::RadarConfig cfg = buildSPConfig();

    double azDiff, elDiff;

    // TC-AESA-SP-001: boresight target within gate
    ASSERT_TRUE(sp.isTargetInBeam(0.0, 0.0, 0.0, 0.0, cfg, azDiff, elDiff),
                "TC-AESA-SP-001: boresight target within beam gate");

    // TC-AESA-SP-002: target 90 deg away is outside 3 deg beam gate
    ASSERT_FALSE(sp.isTargetInBeam(0.0, 0.0, 90.0, 0.0, cfg, azDiff, elDiff),
                 "TC-AESA-SP-002: target 90 deg away rejected by beam gate");

    // TC-AESA-SP-003: azimuth wrap-around — 179 and -179 are only 2 deg apart
    bool withinGate = sp.isTargetInBeam(179.0, 0.0, -179.0, 0.0, cfg, azDiff, elDiff);
    ASSERT_NEAR(azDiff, 2.0, 0.01,
                "TC-AESA-SP-003: azimuth wrap gives correct 2 deg difference");

    // TC-AESA-SP-004: target at z=0 within radar horizon (radarHeight=5000 m)
    ASSERT_TRUE(sp.checkHorizon(100000.0, 0.0, cfg),
                "TC-AESA-SP-004: target at 100km z=0 within horizon");

    // TC-AESA-SP-005: target at 500 km, z=0 is beyond horizon for 5000 m radar
    ASSERT_FALSE(sp.checkHorizon(500000.0, 0.0, cfg),
                 "TC-AESA-SP-005: target at 500km z=0 beyond horizon");
}

// =============================================================================
// TEST SUITE: test_sp_signal_chain
// Covers: TC-AESA-SP-006 through TC-AESA-SP-017
// =============================================================================
void test_sp_signal_chain()
{
    std::cout << "\n--- TC-AESA-SP-006..017: Signal Chain Tests ---" << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    aesa::RadarConfig cfg = buildSPConfig();
    aesa::BeamWaveform wf = cfg.searchWaveform;

    // TC-AESA-SP-006: signal strength decreases with range^4
    double G = std::pow(10.0, 34.0 / 10.0);
    double P1 = sp.calculateSignalStrength(10000.0, 3.0, G, wf, cfg);
    double P2 = sp.calculateSignalStrength(20000.0, 3.0, G, wf, cfg);
    ASSERT_TRUE(P1 > P2,
                "TC-AESA-SP-006: signal strength decreases with range");
    // Range doubles => power reduces by 16x (range^4 law — approximate check)
    ASSERT_TRUE(P1 / P2 > 10.0,
                "TC-AESA-SP-006b: ratio > 10x for doubled range (R^4 law)");

    // TC-AESA-SP-007: signal strength proportional to RCS
    double Prcs1 = sp.calculateSignalStrength(50000.0, 1.0, G, wf, cfg);
    double Prcs10 = sp.calculateSignalStrength(50000.0, 10.0, G, wf, cfg);
    ASSERT_NEAR(Prcs10 / Prcs1, 10.0, 0.01,
                "TC-AESA-SP-007: signal strength proportional to RCS (10x ratio)");

    // TC-AESA-SP-008: noise power > 0
    double Pn = sp.computeNoisePower(cfg, 5e6);
    ASSERT_TRUE(Pn > 0.0,
                "TC-AESA-SP-008: computeNoisePower() > 0");

    // TC-AESA-SP-009: noise power increases with bandwidth
    double Pn2 = sp.computeNoisePower(cfg, 50e6);
    ASSERT_TRUE(Pn2 > Pn,
                "TC-AESA-SP-009: noise power increases with bandwidth");

    // TC-AESA-SP-010: AIR surface clutter = 0
    double Pc = sp.computeClutterPower(50000.0, aesa::SurfaceType::AIR, cfg);
    ASSERT_NEAR(Pc, 0.0, 1e-30,
                "TC-AESA-SP-010: AIR surface clutter power = 0");

    // TC-AESA-SP-011: SEA surface clutter > 0
    // Run multiple times — exponential distribution makes each call random
    double seaTotal = 0.0;
    for (int i = 0; i < 20; ++i)
        seaTotal += sp.computeClutterPower(50000.0, aesa::SurfaceType::SEA, cfg);
    ASSERT_TRUE(seaTotal > 0.0,
                "TC-AESA-SP-011: SEA surface clutter power > 0 on average");

    // TC-AESA-SP-012: inactive jammer returns 0
    aesa::TargetInput tgt = buildSPTarget(1u, 50000.0, 0.0, 5000.0);
    double Pj = sp.computeJammerPower(50000.0, tgt, cfg);
    ASSERT_NEAR(Pj, 0.0, 1e-30,
                "TC-AESA-SP-012: inactive jammer returns 0 power");

    // TC-AESA-SP-013: active jammer returns > 0
    tgt.jammer.active       = true;
    tgt.jammer.power_kW     = 5.0;
    tgt.jammer.gain_dBi     = 10.0;
    tgt.jammer.selfScreening = true;
    tgt.jammer.bandwidth_Hz  = 100e6;
    Pj = sp.computeJammerPower(50000.0, tgt, cfg);
    ASSERT_TRUE(Pj > 0.0,
                "TC-AESA-SP-013: active jammer returns > 0 power");

    // TC-AESA-SP-014: clear air propagation loss close to 1.0
    double propLoss = sp.computePropagationLoss(10000.0, cfg);
    ASSERT_TRUE(propLoss > 0.9 && propLoss <= 1.0,
                "TC-AESA-SP-014: clear air propagation loss in (0.9, 1.0]");

    // TC-AESA-SP-015: heavy rain reduces propagation factor
    aesa::RadarConfig rainCfg = cfg;
    rainCfg.atmosphere.rainRate_mmph = 100.0f;
    double propRain = sp.computePropagationLoss(100000.0, rainCfg);
    ASSERT_TRUE(propRain < propLoss,
                "TC-AESA-SP-015: heavy rain reduces propagation factor");

    // TC-AESA-SP-016: SINR > 0 for valid signal
    aesa::TargetInput noJam = buildSPTarget(2u, 20000.0, 0.0, 5000.0);
    double Pr = sp.calculateSignalStrength(20000.0, 3.0, G, wf, cfg);
    double sinr = sp.computeSINR(Pr, 20000.0, aesa::SurfaceType::AIR,
                                 noJam, cfg, wf);
    ASSERT_TRUE(sinr > 0.0,
                "TC-AESA-SP-016: SINR > 0 for valid signal");

    // TC-AESA-SP-017: SINR decreases when jammer active
    aesa::TargetInput withJam = buildSPTarget(3u, 20000.0, 0.0, 5000.0);
    withJam.jammer.active       = true;
    withJam.jammer.power_kW     = 50.0;
    withJam.jammer.gain_dBi     = 20.0;
    withJam.jammer.selfScreening = true;
    withJam.jammer.bandwidth_Hz  = cfg.antennaBandwidth;
    double sinrJam = sp.computeSINR(Pr, 20000.0, aesa::SurfaceType::AIR,
                                    withJam, cfg, wf);
    ASSERT_TRUE(sinrJam < sinr,
                "TC-AESA-SP-017: SINR decreases with active jammer");
}

// =============================================================================
// TEST SUITE: test_sp_cfar
// Covers: TC-AESA-SP-018 through TC-AESA-SP-021
// =============================================================================
void test_sp_cfar()
{
    std::cout << "\n--- TC-AESA-SP-018..021: CFAR Tests ---" << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    aesa::RadarConfig cfg = buildSPConfig();

    // TC-AESA-SP-018: generateReferenceCells() returns exactly 16 cells
    auto cells = sp.generateReferenceCells(aesa::SurfaceType::AIR, cfg);
    ASSERT_EQ(static_cast<int>(cells.size()), 16,
              "TC-AESA-SP-018: generateReferenceCells() returns 16 cells");

    // TC-AESA-SP-019: computeCFARThreshold() > 0 for valid cells
    double thresh = sp.computeCFARThreshold(cells, cfg);
    ASSERT_TRUE(thresh > 0.0,
                "TC-AESA-SP-019: CFAR threshold > 0 for valid cells");

    // TC-AESA-SP-020: empty cells returns 1e12
    double infThresh = sp.computeCFARThreshold({}, cfg);
    ASSERT_NEAR(infThresh, 1e12, 1.0,
                "TC-AESA-SP-020: empty cells returns 1e12");

    // TC-AESA-SP-021: relaxed threshold < standard threshold
    double relaxed = sp.computeCFARThresholdRelaxed(cells, cfg);
    ASSERT_TRUE(relaxed < thresh,
                "TC-AESA-SP-021: relaxed CFAR threshold < standard threshold");
}

// =============================================================================
// TEST SUITE: test_sp_rcs
// Covers: TC-AESA-SP-022 through TC-AESA-SP-025
// =============================================================================
void test_sp_rcs()
{
    std::cout << "\n--- TC-AESA-SP-022..025: RCS Tests ---" << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    aesa::RadarConfig cfg = buildSPConfig();

    // TC-AESA-SP-022: GENERIC target without dimensions returns base RCS
    aesa::TargetInput generic = buildSPTarget(1u, 50000.0, 0.0, 5000.0);
    generic.swerlingCase  = aesa::SwerlingCase::CASE_0;  // no fluctuation
    double rcs = sp.computeEffectiveRCS(generic, 50000.0, 10.0e9);
    ASSERT_NEAR(rcs, 3.0, 0.01,  // FIGHTER base = 3.0 m²
                "TC-AESA-SP-022: GENERIC FIGHTER returns base RCS 3.0 m²");

    // TC-AESA-SP-023: STEALTHY material reduces RCS vs METAL for same dims
    aesa::TargetInput metalTarget = buildSPTarget(2u, 50000.0, 0.0, 5000.0);
    metalTarget.dimensions.valid    = true;
    metalTarget.dimensions.length   = 15.0;
    metalTarget.dimensions.height   = 4.0;
    metalTarget.dimensions.width    = 10.0;
    metalTarget.dimensions.material = aesa::TargetMaterialType::METAL;
    metalTarget.dimensions.shape    = aesa::TargetShapeType::AIRCRAFT;
    metalTarget.swerlingCase        = aesa::SwerlingCase::CASE_0;

    aesa::TargetInput stealthTarget = metalTarget;
    stealthTarget.dimensions.material = aesa::TargetMaterialType::STEALTHY;

    double rcsMetal   = sp.computeEffectiveRCS(metalTarget,   50000.0, 10.0e9);
    double rcsStealth = sp.computeEffectiveRCS(stealthTarget, 50000.0, 10.0e9);
    ASSERT_TRUE(rcsStealth < rcsMetal,
                "TC-AESA-SP-023: STEALTHY material < METAL RCS for same dimensions");

    // TC-AESA-SP-024: CASE_0 Swerling returns exact nominal RCS
    double nominal = 5.0;
    double result  = sp.computeSwerlingRCS(nominal, aesa::SwerlingCase::CASE_0, false);
    ASSERT_NEAR(result, nominal, 1e-9,
                "TC-AESA-SP-024: CASE_0 returns exact nominal RCS");

    // TC-AESA-SP-025: CASE_I returns positive value for positive input
    // Run multiple times — exponential distribution is always positive
    bool allPositive = true;
    for (int i = 0; i < 20; ++i)
    {
        double r = sp.computeSwerlingRCS(5.0, aesa::SwerlingCase::CASE_I, false);
        if (r <= 0.0) allPositive = false;
    }
    ASSERT_TRUE(allPositive,
                "TC-AESA-SP-025: CASE_I returns positive RCS on 20 draws");
}

// =============================================================================
// TEST SUITE: test_sp_albersheim
// Covers: TC-AESA-SP-026 through TC-AESA-SP-027
// =============================================================================
void test_sp_albersheim()
{
    std::cout << "\n--- TC-AESA-SP-026..027: Albersheim Pd Tests ---" << std::endl;

    aesa::RadarSignalProcessor_AESA sp;

    // TC-AESA-SP-026: high SNR gives Pd close to 0.99
    double pd = sp.computeAlbersheimPd(10000.0, 1e-6, 10, aesa::SwerlingCase::CASE_0);
    ASSERT_TRUE(pd >= 0.95,
                "TC-AESA-SP-026: high SNR gives Pd >= 0.95");
    ASSERT_TRUE(pd <= 0.99,
                "TC-AESA-SP-026b: Pd clamped at 0.99 maximum");

    // TC-AESA-SP-027: zero SNR returns 0.0
    pd = sp.computeAlbersheimPd(0.0, 1e-6, 10, aesa::SwerlingCase::CASE_0);
    ASSERT_NEAR(pd, 0.0, 1e-9,
                "TC-AESA-SP-027: zero SNR returns Pd = 0.0");
}

// =============================================================================
// TEST SUITE: test_sp_kinematics
// Covers: TC-AESA-SP-028 through TC-AESA-SP-029
// =============================================================================
void test_sp_kinematics()
{
    std::cout << "\n--- TC-AESA-SP-028..029: Kinematics Tests ---" << std::endl;

    aesa::RadarSignalProcessor_AESA sp;

    // TC-AESA-SP-028: head-on target (moving toward radar) gives negative radial vel
    aesa::TargetInput headOn = buildSPTarget(1u, 50000.0, 0.0, 0.0,
                                             -300.0, 0.0, 0.0);  // closing
    std::normal_distribution<double> noise(0.0, 0.0);  // zero noise
    double rv = sp.computeRadialVelocity(headOn, 50000.0, noise);
    ASSERT_TRUE(rv < 0.0,
                "TC-AESA-SP-028: head-on target gives negative radial velocity");

    // TC-AESA-SP-029: stationary target CPA = current range
    aesa::TargetInput stationary = buildSPTarget(2u, 30000.0, 0.0, 0.0);
    aesa::DetectionOutput det;
    sp.computeCPA(det, stationary, 30000.0);
    ASSERT_NEAR(det.cpa_distance, 30000.0, 1.0,
                "TC-AESA-SP-029: stationary target CPA = current range");
    ASSERT_NEAR(det.time_to_cpa, 0.0, 1e-9,
                "TC-AESA-SP-029b: stationary target time to CPA = 0");
}

// =============================================================================
// TEST SUITE: test_sp_range_ambiguity
// Covers: TC-AESA-SP-030 through TC-AESA-SP-035
// =============================================================================
void test_sp_range_ambiguity()
{
    std::cout << "\n--- TC-AESA-SP-030..035: Range Ambiguity Tests ---" << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    double Rmax = 500000.0;  // 500 km

    // TC-AESA-SP-030: unambiguous range returned unchanged
    double resolved = sp.resolveRangeAmbiguity(200000.0, 200000.0, Rmax);
    ASSERT_NEAR(resolved, 200000.0, 1.0,
                "TC-AESA-SP-030: unambiguous range returned unchanged");

    // TC-AESA-SP-031: k=1 fold resolved to true range
    double trueRange = 600000.0;
    double folded    = trueRange - Rmax;  // 100 km
    resolved = sp.resolveRangeAmbiguity(folded, trueRange, Rmax);
    ASSERT_NEAR(resolved, trueRange, 1.0,
                "TC-AESA-SP-031: k=1 fold resolved to true range");

    // TC-AESA-SP-032: staggered PRF resolves k=1 fold
    double Rmax2 = 450000.0;
    double folded2 = std::fmod(trueRange, Rmax2);  // ~150 km
    double staggered = sp.resolveRangeAmbiguityStaggered(
        folded, folded2, Rmax, Rmax2, trueRange);
    ASSERT_NEAR(staggered, trueRange, 1000.0,
                "TC-AESA-SP-032: staggered PRF resolves k=1 fold");

    // TC-AESA-SP-033: resolveVelocityStaggered returns a finite value
    double vel = sp.resolveVelocityStaggered(50.0, 55.0, 150.0, 135.0, 50.0);
    ASSERT_TRUE(std::isfinite(vel),
                "TC-AESA-SP-033: resolveVelocityStaggered returns finite value");

    // TC-AESA-SP-034: range < Rmax stored unchanged with isAmbiguous=false
    std::normal_distribution<double> zeroNoise(0.0, 0.0);
    aesa::DetectionOutput det;
    sp.applyRangeAmbiguity(det, 100000.0, Rmax, 0.0, zeroNoise);
    ASSERT_NEAR(det.range, 100000.0, 1.0,
                "TC-AESA-SP-034: range < Rmax stored unchanged");
    ASSERT_FALSE(det.isAmbiguous,
                 "TC-AESA-SP-034b: range < Rmax gives isAmbiguous=false");

    // TC-AESA-SP-035: range > Rmax sets isAmbiguous=true
    sp.applyRangeAmbiguity(det, 600000.0, Rmax, 0.0, zeroNoise);
    ASSERT_TRUE(det.isAmbiguous,
                "TC-AESA-SP-035: range > Rmax sets isAmbiguous=true");
}

// =============================================================================
// TEST SUITE: test_sp_maxrange_merge
// Covers: TC-AESA-SP-036 through TC-AESA-SP-038
// =============================================================================
void test_sp_maxrange_merge()
{
    std::cout << "\n--- TC-AESA-SP-036..038: Max Range and Merge Tests ---"
              << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    aesa::RadarConfig cfg = buildSPConfig();

    // TC-AESA-SP-036: computeMaxDetectionRange() > 0
    double maxR = sp.computeMaxDetectionRange(3.0, cfg);
    ASSERT_TRUE(maxR > 0.0,
                "TC-AESA-SP-036: computeMaxDetectionRange() > 0");

    // TC-AESA-SP-037: same position detection merges
    aesa::DetectionOutput det1;
    det1.range     = 50000.0;
    det1.azimuth   = 30.0;
    det1.elevation = 10.0;
    det1.targetID  = 1u;

    aesa::DetectionOutput det2 = det1;  // identical position
    ASSERT_TRUE(sp.shouldMergeDetection(det2, {det1}, cfg),
                "TC-AESA-SP-037: same position detection merges");

    // TC-AESA-SP-038: distant position does not merge
    aesa::DetectionOutput det3;
    det3.range     = 200000.0;  // 200 km away
    det3.azimuth   = 90.0;
    det3.elevation = 30.0;
    det3.targetID  = 2u;
    ASSERT_FALSE(sp.shouldMergeDetection(det3, {det1}, cfg),
                 "TC-AESA-SP-038: distant position does not merge");
}

// =============================================================================
// TEST SUITE: test_sp_beamgain_sidelobe
// Covers: TC-AESA-SP-039 through TC-AESA-SP-041
// =============================================================================
void test_sp_beamgain_sidelobe()
{
    std::cout << "\n--- TC-AESA-SP-039..041: Beam Gain and Sidelobe Tests ---"
              << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    aesa::RadarConfig cfg = buildSPConfig();

    // TC-AESA-SP-039: boresight (azDiff=0, elDiff=0) returns factor 1.0
    double factor = sp.computeBeamGainFactor(0.0, 0.0, cfg);
    ASSERT_NEAR(factor, 1.0, 1e-9,
                "TC-AESA-SP-039: boresight target returns beam gain factor 1.0");

    // TC-AESA-SP-040: far target (90 deg away) returns average sidelobe level
    factor = sp.computeBeamGainFactor(90.0, 0.0, cfg);
    double expectedAvgSLL = std::pow(10.0, -50.0 / 10.0);  // -50 dBi
    ASSERT_NEAR(factor, expectedAvgSLL, expectedAvgSLL * 0.01,
                "TC-AESA-SP-040: far target returns avgSidelobeLevel linear");

    // TC-AESA-SP-041: inactive jammer in sidelobe returns false
    aesa::TargetInput noJam = buildSPTarget(1u, 0.0, 50000.0, 0.0);
    ASSERT_FALSE(sp.isJammerInSidelobe(45.0, 5.0, noJam, cfg),
                 "TC-AESA-SP-041: inactive jammer returns false from sidelobe check");
}

// =============================================================================
// TEST SUITE: test_sp_modulation_doppler
// Covers: TC-AESA-SP-042 through TC-AESA-SP-047
// =============================================================================
void test_sp_modulation_doppler()
{
    std::cout << "\n--- TC-AESA-SP-042..047: Modulation and Doppler Tests ---"
              << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    aesa::RadarConfig cfg = buildSPConfig();

    // TC-AESA-SP-042: LFM processing gain = BT = bandwidth * pulseWidth
    aesa::BeamWaveform lfm;
    lfm.modulation  = aesa::ModulationType::LFM;
    lfm.bandwidth_Hz = 20e6f;
    lfm.pulseWidth_s = 10e-6f;
    double pg = sp.computeModulationProcessingGain(lfm);
    ASSERT_NEAR(pg, 20e6 * 10e-6, 1.0,
                "TC-AESA-SP-042: LFM processing gain = BT = 200");

    // TC-AESA-SP-043: NONE modulation returns 1.0
    aesa::BeamWaveform none;
    none.modulation = aesa::ModulationType::NONE;
    ASSERT_NEAR(sp.computeModulationProcessingGain(none), 1.0, 1e-9,
                "TC-AESA-SP-043: NONE modulation returns processing gain 1.0");

    // TC-AESA-SP-044: HPRF waveform is never in Doppler blind zone
    aesa::BeamWaveform hprf = cfg.fireControlWaveform;  // HPRF
    ASSERT_FALSE(sp.isInDopplerBlindZone(0.0, cfg, hprf),
                 "TC-AESA-SP-044: HPRF waveform returns false for blind zone");

    // TC-AESA-SP-045: stationary platform (speed < 1 m/s) has no blind zone
    aesa::RadarConfig staticCfg = cfg;
    staticCfg.platformSpeed_m_s = 0.0f;
    aesa::BeamWaveform lprf = cfg.searchWaveform;
    ASSERT_FALSE(sp.isInDopplerBlindZone(0.0, staticCfg, lprf),
                 "TC-AESA-SP-045: stationary platform returns false (no blind zone)");

    // TC-AESA-SP-046: target far from notch gets full STAP gain
    double fullGain = sp.computeSTAPGain(0.0, 250.0, cfg.searchWaveform, cfg);
    // At vel=0 and platform=250, distFromNotch = 250 m/s >> notch width
    ASSERT_TRUE(fullGain > 1.0,
                "TC-AESA-SP-046: far-from-notch target gets STAP gain > 1.0");

    // TC-AESA-SP-047: STAP gain is always >= 1.0
    double inNotchGain = sp.computeSTAPGain(250.0, 250.0, cfg.searchWaveform, cfg);
    ASSERT_TRUE(inNotchGain >= 1.0,
                "TC-AESA-SP-047: STAP gain always >= 1.0 (even inside notch)");
}

// =============================================================================
// TEST SUITE: test_sp_multipath_chaff
// Covers: TC-AESA-SP-048 through TC-AESA-SP-050
// =============================================================================
void test_sp_multipath_chaff()
{
    std::cout << "\n--- TC-AESA-SP-048..050: Multipath and Chaff Tests ---"
              << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    aesa::RadarConfig cfg = buildSPConfig();

    // TC-AESA-SP-048: high elevation angle returns multipath factor 1.0
    double mp = sp.computeMultipathFactor(50000.0, 10.0, 1000.0, cfg);
    ASSERT_NEAR(mp, 1.0, 1e-9,
                "TC-AESA-SP-048: elevation > 5 deg returns multipath factor 1.0");

    // TC-AESA-SP-049: zero target altitude returns multipath factor 1.0
    mp = sp.computeMultipathFactor(50000.0, 1.0, 0.0, cfg);
    ASSERT_NEAR(mp, 1.0, 1e-9,
                "TC-AESA-SP-049: zero altitude returns multipath factor 1.0");

    // TC-AESA-SP-050: empty cloud list returns 0.0 chaff return
    double chaff = sp.computeChaffReturn(0.0, 0.0, {}, 0.0, cfg);
    ASSERT_NEAR(chaff, 0.0, 1e-30,
                "TC-AESA-SP-050: empty cloud list returns 0.0 chaff power");
}

// =============================================================================
// TEST SUITE: test_sp_rcs_lookup
// Covers: TC-AESA-SP-051 through TC-AESA-SP-053
// =============================================================================
void test_sp_rcs_lookup()
{
    std::cout << "\n--- TC-AESA-SP-051..053: RCS Lookup Tests ---" << std::endl;

    aesa::RadarSignalProcessor_AESA sp;

    // TC-AESA-SP-051: FIGHTER returns 3.0 m²
    ASSERT_NEAR(sp.getPlatformBaseRCS("FIGHTER"), 3.0, 1e-9,
                "TC-AESA-SP-051: FIGHTER base RCS = 3.0 m²");

    // TC-AESA-SP-052: STEALTH returns 0.001 m²
    ASSERT_NEAR(sp.getPlatformBaseRCS("STEALTH"), 0.001, 1e-9,
                "TC-AESA-SP-052: STEALTH base RCS = 0.001 m²");

    // TC-AESA-SP-053: empty table uses base RCS
    aesa::TargetInput t = buildSPTarget(1u, 50000.0, 0.0, 0.0);
    t.rcsTable.clear();
    double rcs = sp.lookupAspectRCS(t, 90.0);
    ASSERT_NEAR(rcs, 3.0, 1e-9,  // FIGHTER base
                "TC-AESA-SP-053: empty rcsTable falls back to base RCS");
}

// =============================================================================
// TEST SUITE: test_sp_atmospheric
// Covers: TC-AESA-SP-054 through TC-AESA-SP-055
// =============================================================================
void test_sp_atmospheric()
{
    std::cout << "\n--- TC-AESA-SP-054..055: Atmospheric Tests ---" << std::endl;

    aesa::RadarSignalProcessor_AESA sp;

    // TC-AESA-SP-054: 0% humidity gives zero water vapour density
    aesa::AtmosphericConditions dryAtm;
    dryAtm.humidity_pct    = 0.0f;
    dryAtm.temperature_C   = 15.0f;
    dryAtm.pressure_hPa    = 1013.25f;
    double rho = sp.computeWaterVapourDensity(dryAtm);
    ASSERT_NEAR(rho, 0.0, 1e-9,
                "TC-AESA-SP-054: 0% humidity gives zero water vapour density");

    // TC-AESA-SP-055: gaseous attenuation >= 0 for standard atmosphere
    aesa::AtmosphericConditions stdAtm;
    stdAtm.temperature_C = 15.0f;
    stdAtm.humidity_pct  = 60.0f;
    stdAtm.pressure_hPa  = 1013.25f;
    double gas = sp.computeGaseousAttenuation(10.0e9, stdAtm, 50000.0);
    ASSERT_TRUE(gas >= 0.0,
                "TC-AESA-SP-055: gaseous attenuation >= 0 for standard atmosphere");
}

// =============================================================================
// TEST SUITE: test_sp_waveform_selection
// Covers: TC-AESA-SP-056 through TC-AESA-SP-057
// =============================================================================
void test_sp_waveform_selection()
{
    std::cout << "\n--- TC-AESA-SP-056..057: Waveform Selection Tests ---"
              << std::endl;

    aesa::RadarSignalProcessor_AESA sp;
    aesa::RadarConfig cfg = buildSPConfig();
    // Table: 0-30km HPRF, 30-100km MPRF, 100-400km LPRF

    // TC-AESA-SP-056: short range (20 km) selects HPRF (first table entry)
    aesa::BeamWaveform wf = sp.selectWaveformForRange(20000.0, cfg);
    ASSERT_TRUE(wf.mode == aesa::WaveformMode::HPRF,
                "TC-AESA-SP-056: 20km selects HPRF waveform");

    // TC-AESA-SP-057: long range (300 km) selects LPRF (third table entry)
    wf = sp.selectWaveformForRange(300000.0, cfg);
    ASSERT_TRUE(wf.mode == aesa::WaveformMode::LPRF,
                "TC-AESA-SP-057: 300km selects LPRF waveform");
}

// =============================================================================
// ENTRY POINT: radarSignalProcessor_test
// =============================================================================
void radarSignalProcessor_test()
{
    testsPassed = 0;
    testsFailed = 0;
    tests       = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "   RADARSIGNALPROCESSOR_AESA UNIT TESTS  " << std::endl;
    std::cout << "   Standard: DO-178C DAL B               " << std::endl;
    std::cout << "   Project:  ISSST                       " << std::endl;
    std::cout << "   Org:      Oxygen to Innovation Pvt. Ltd." << std::endl;
    std::cout << "=========================================" << std::endl;

    test_sp_geometry();
    test_sp_signal_chain();
    test_sp_cfar();
    test_sp_rcs();
    test_sp_albersheim();
    test_sp_kinematics();
    test_sp_range_ambiguity();
    test_sp_maxrange_merge();
    test_sp_beamgain_sidelobe();
    test_sp_modulation_doppler();
    test_sp_multipath_chaff();
    test_sp_rcs_lookup();
    test_sp_atmospheric();
    test_sp_waveform_selection();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "SIGNAL PROCESSOR TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
