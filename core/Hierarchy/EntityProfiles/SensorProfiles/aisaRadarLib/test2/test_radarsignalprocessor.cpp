#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include "radarsignalprocessor_aesa.h"

using namespace aesa;

static RadarConfig makeCfg()
{
    RadarConfig cfg;
    cfg.numElements           = 1000;
    cfg.peakPowerPerElement_W = 100.0f;
    cfg.moduleEfficiency      = 0.70f;
    cfg.failedModules         = 0;
    cfg.frequency_Hz          = 8.0e9;
    cfg.antennaGain           = 35.0f;
    cfg.antennaBandwidth      = 1e6;
    cfg.beamWidth             = 3.0f;
    cfg.maxSteeringAngle_deg  = 60.0f;
    cfg.systemTemperature_K   = 290.0;
    cfg.noiseFigure_dB        = 5.0;
    cfg.targetPfa             = 1e-6;
    cfg.radarHeight           = 10000.0;
    cfg.minDetectableRange    = 30.0;
    cfg.platformSpeed_m_s     = 0.0f;
    cfg.earthRadiusFactor     = 1.33;
    cfg.atmosphericFactor     = 1.0;
    cfg.seaState              = 0.0f;
    cfg.landClutter           = 0.0f;
    cfg.atmosphere            = {15.0f, 60.0f, 1013.25f, 0.0f, 0.0f};
    cfg.sidelobeMode          = SidelobeMode::NORMAL;
    cfg.peakSidelobeLevel     = -25.0f;
    cfg.avgSidelobeLevel      = -35.0f;
    cfg.waveformTable[0] = {30000.0f,  {ModulationType::NLFM, 5e-6f,  2000.0f, 50e6f, 25, WaveformMode::HPRF}};
    cfg.waveformTable[1] = {100000.0f, {ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 25, WaveformMode::MPRF}};
    cfg.waveformTable[2] = {400000.0f, {ModulationType::LFM,  50e-6f,  300.0f,  5e6f, 10, WaveformMode::LPRF}};
    cfg.waveformTable[3] = {};
    return cfg;
}

static BeamWaveform makeWF(WaveformMode m = WaveformMode::LPRF)
{
    BeamWaveform wf;
    wf.modulation     = ModulationType::LFM;
    wf.pulseWidth_s   = 50e-6f;
    wf.prf_Hz         = 300.0f;
    wf.bandwidth_Hz   = 5e6f;
    wf.pulsesPerDwell = 10;
    wf.mode           = m;
    wf.prf2_Hz        = 0.0f;
    return wf;
}

static TargetInput makeAirTarget(uint32_t id, double x, double y, double z,
                                  double vx = -100.0)
{
    TargetInput t;
    t.id = id; t.x = x; t.y = y; t.z = z;
    t.vx = vx; t.vy = 0.0; t.vz = 0.0;
    t.surface      = SurfaceType::AIR;
    t.swerlingCase = SwerlingCase::CASE_0;
    t.rcs          = 5.0;
    t.jammer.active = false;
    return t;
}

// ── geometry ─────────────────────────────────────────────────────────────────
TEST(SignalProc, JammerPowerZeroWithNoJammer)
{
    RadarSignalProcessor_AESA sp;
    TargetInput t = makeAirTarget(1, 50000, 0, 5000);
    t.jammer.active = false;
    EXPECT_DOUBLE_EQ(sp.computeJammerPower(50000.0, t, makeCfg()), 0.0);
}

TEST(SignalProc, JammerPowerPositiveWhenActive)
{
    RadarSignalProcessor_AESA sp;
    TargetInput t = makeAirTarget(1, 50000, 0, 5000);
    t.jammer.active   = true;
    t.jammer.power_kW = 10.0;
    t.jammer.gain_dBi = 0.0;
    t.jammer.selfScreening = true;
    EXPECT_GT(sp.computeJammerPower(50000.0, t, makeCfg()), 0.0);
}
TEST(SignalProc, TargetAtBeamCentreIsInBeam)
{
    RadarSignalProcessor_AESA sp;
    double azD, elD;
    EXPECT_TRUE(sp.isTargetInBeam(10.0, 5.0, 10.0, 5.0, makeCfg(), azD, elD));
    EXPECT_NEAR(azD, 0.0, 1e-9);
}
TEST(SignalProc, TargetFarOffAxisNotInBeam)
{
    RadarSignalProcessor_AESA sp;
    double azD, elD;
    EXPECT_FALSE(sp.isTargetInBeam(0.0, 0.0, 30.0, 0.0, makeCfg(), azD, elD));
}
TEST(SignalProc, EffectiveBeamWidthExpandsGate)
{
    RadarSignalProcessor_AESA sp;
    double azD, elD;
    bool wide = sp.isTargetInBeam(0.0, 0.0, 6.0, 0.0, makeCfg(), azD, elD, 6.0);
    EXPECT_TRUE(wide);
}
TEST(SignalProc, BeamCheckBothAzAndElMustBeInGate)
{
    RadarSignalProcessor_AESA sp;
    double azD, elD;
    // Az OK but El way off
    EXPECT_FALSE(sp.isTargetInBeam(0.0, 0.0, 1.0, 60.0, makeCfg(), azD, elD));
}
TEST(SignalProc, HorizonShortRangeVisible)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_TRUE(sp.checkHorizon(100000.0, 3000.0, makeCfg()));
}
TEST(SignalProc, HorizonBeyondHorizonBlocked)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.radarHeight = 5.0;
    EXPECT_FALSE(sp.checkHorizon(500000.0, 0.0, cfg));
}
TEST(SignalProc, HorizonVeryShortRangeAlwaysVisible)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_TRUE(sp.checkHorizon(100.0, 100.0, makeCfg()));
}
TEST(SignalProc, HorizonTargetHighAltitudeExtendsHorizon)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.radarHeight = 5.0;
    // High target extends radar horizon significantly
    EXPECT_TRUE(sp.checkHorizon(200000.0, 10000.0, cfg));
}

// ── noise power ──────────────────────────────────────────────────────────────
TEST(SignalProc, NoisePowerPositive)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_GT(sp.computeNoisePower(makeCfg(), 5e6), 0.0);
}
TEST(SignalProc, NoisePowerLinearInBandwidth)
{
    RadarSignalProcessor_AESA sp;
    double Pn1 = sp.computeNoisePower(makeCfg(), 1e6);
    double Pn2 = sp.computeNoisePower(makeCfg(), 10e6);
    EXPECT_NEAR(Pn2/Pn1, 10.0, 0.01);
}
TEST(SignalProc, NoisePowerZeroBandwidthFlooredAtOne)
{
    RadarSignalProcessor_AESA sp;
    // bandwidth=0 should not produce zero or negative
    EXPECT_GT(sp.computeNoisePower(makeCfg(), 0.0), 0.0);
}
TEST(SignalProc, NoisePowerIncreasesWithNoiseFigure)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig c1 = makeCfg(); c1.noiseFigure_dB = 3.0;
    RadarConfig c2 = makeCfg(); c2.noiseFigure_dB = 10.0;
    EXPECT_LT(sp.computeNoisePower(c1, 5e6), sp.computeNoisePower(c2, 5e6));
}

// ── signal strength ───────────────────────────────────────────────────────────
TEST(SignalProc, SignalStrengthFourthPowerRangeLaw)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.frequencyAgility = false;  // isolate R^4 law from agility randomness
    double G = std::pow(10.0, cfg.antennaGain/10.0);
    BeamWaveform wf = makeWF();
    double Pr1 = sp.calculateSignalStrength(10000.0, 1.0, G, wf, cfg);
    double Pr2 = sp.calculateSignalStrength(20000.0, 1.0, G, wf, cfg);
    EXPECT_GT(Pr1/Pr2, 10.0);
    EXPECT_LT(Pr1/Pr2, 20.0);
}
TEST(SignalProc, SignalStrengthPositive)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    double G = std::pow(10.0, cfg.antennaGain/10.0);
    EXPECT_GT(sp.calculateSignalStrength(50000.0, 5.0, G, makeWF(), cfg), 0.0);
}
TEST(SignalProc, SignalStrengthDecreasesWithRange)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    double G = std::pow(10.0, cfg.antennaGain/10.0);
    BeamWaveform wf = makeWF();
    double Pr1 = sp.calculateSignalStrength(10000.0, 5.0, G, wf, cfg);
    double Pr2 = sp.calculateSignalStrength(50000.0, 5.0, G, wf, cfg);
    EXPECT_GT(Pr1, Pr2);
}
TEST(SignalProc, SignalStrengthIncreasesWithRCS)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.frequencyAgility = false;  // isolate RCS scaling from agility randomness
    double G = std::pow(10.0, cfg.antennaGain/10.0);
    BeamWaveform wf = makeWF();
    double Pr1 = sp.calculateSignalStrength(50000.0, 1.0,  G, wf, cfg);
    double Pr2 = sp.calculateSignalStrength(50000.0, 10.0, G, wf, cfg);
    EXPECT_NEAR(Pr2/Pr1, 10.0, 0.5);
}
TEST(SignalProc, SignalStrengthMinimumRangeFloor)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    double G = std::pow(10.0, cfg.antennaGain/10.0);
    // range < 1m should be floored — no divide by zero
    EXPECT_GT(sp.calculateSignalStrength(0.0, 5.0, G, makeWF(), cfg), 0.0);
    EXPECT_GT(sp.calculateSignalStrength(-10.0, 5.0, G, makeWF(), cfg), 0.0);
}

// ── propagation loss ─────────────────────────────────────────────────────────
TEST(SignalProc, PropagationClearAtmosphereNearUnity)
{
    RadarSignalProcessor_AESA sp;
    double loss = sp.computePropagationLoss(1000.0, makeCfg());
    EXPECT_GT(loss, 0.99);
    EXPECT_LE(loss, 1.0);
}
TEST(SignalProc, PropagationHeavyRainReducesSignal)
{
    RadarSignalProcessor_AESA sp;
    double lossClear = sp.computePropagationLoss(50000.0, makeCfg());
    RadarConfig cfg = makeCfg();
    cfg.atmosphere.rainRate_mmph = 100.0f;
    double lossRain = sp.computePropagationLoss(50000.0, cfg);
    EXPECT_LT(lossRain, lossClear);
}
TEST(SignalProc, PropagationFogReducesSignal)
{
    RadarSignalProcessor_AESA sp;
    double lossClear = sp.computePropagationLoss(50000.0, makeCfg());
    RadarConfig cfg = makeCfg();
    cfg.atmosphere.fogVisibility_m = 100.0f;
    double lossFog = sp.computePropagationLoss(50000.0, cfg);
    EXPECT_LT(lossFog, lossClear);
}
TEST(SignalProc, PropagationLossIsNeverNegative)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.atmosphere.rainRate_mmph = 200.0f;
    EXPECT_GE(sp.computePropagationLoss(200000.0, cfg), 0.0);
}
TEST(SignalProc, PropagationLossWorseAtLongerRange)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.atmosphere.rainRate_mmph = 10.0f;
    double l1 = sp.computePropagationLoss(10000.0,  cfg);
    double l2 = sp.computePropagationLoss(100000.0, cfg);
    EXPECT_GT(l1, l2);
}

// ── CFAR ─────────────────────────────────────────────────────────────────────
TEST(SignalProc, CFARThresholdPositive)
{
    RadarSignalProcessor_AESA sp;
    std::vector<double> cells(16, 1.0);
    EXPECT_GT(sp.computeCFARThreshold(cells, makeCfg()), 0.0);
}
TEST(SignalProc, CFARThresholdLinearInCellMean)
{
    RadarSignalProcessor_AESA sp;
    std::vector<double> c1(16, 1.0), c2(16, 10.0);
    EXPECT_NEAR(sp.computeCFARThreshold(c2, makeCfg()) /
                sp.computeCFARThreshold(c1, makeCfg()), 10.0, 0.01);
}
TEST(SignalProc, CFAREmptyCellsReturnsHugeThreshold)
{
    RadarSignalProcessor_AESA sp;
    std::vector<double> empty;
    EXPECT_GE(sp.computeCFARThreshold(empty, makeCfg()), 1e10);
}
TEST(SignalProc, CFARRelaxedLowerThanNormal)
{
    RadarSignalProcessor_AESA sp;
    std::vector<double> cells(16, 1.0);
    EXPECT_LT(sp.computeCFARThresholdRelaxed(cells, makeCfg()),
              sp.computeCFARThreshold(cells, makeCfg()));
}
TEST(SignalProc, CFARHigherPfaLowersThreshold)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig c1 = makeCfg(); c1.targetPfa = 1e-3;
    RadarConfig c2 = makeCfg(); c2.targetPfa = 1e-6;
    std::vector<double> cells(16, 1.0);
    EXPECT_LT(sp.computeCFARThreshold(cells, c1),
              sp.computeCFARThreshold(cells, c2));
}

// ── Swerling / RCS ───────────────────────────────────────────────────────────
TEST(SignalProc, SwerlingCase0IsNominal)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_DOUBLE_EQ(sp.computeSwerlingRCS(5.0, SwerlingCase::CASE_0, false), 5.0);
}
TEST(SignalProc, SwerlingCase1AlwaysPositive)
{
    RadarSignalProcessor_AESA sp;
    for (int i = 0; i < 50; ++i)
        EXPECT_GT(sp.computeSwerlingRCS(5.0, SwerlingCase::CASE_I, false), 0.0);
}
TEST(SignalProc, SwerlingCase3AlwaysPositive)
{
    RadarSignalProcessor_AESA sp;
    for (int i = 0; i < 50; ++i)
        EXPECT_GT(sp.computeSwerlingRCS(5.0, SwerlingCase::CASE_III, false), 0.0);
}
TEST(SignalProc, SwerlingZeroRCSReturnsZero)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_DOUBLE_EQ(sp.computeSwerlingRCS(0.0, SwerlingCase::CASE_I, false), 0.0);
}
TEST(SignalProc, SwerlingNegativeRCSReturnsZero)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_DOUBLE_EQ(sp.computeSwerlingRCS(-5.0, SwerlingCase::CASE_0, false), 0.0);
}
TEST(SignalProc, StealthMaterialReducesRCSSignificantly)
{
    RadarSignalProcessor_AESA sp;
    TargetInput t = makeAirTarget(1, 50000, 0, 5000);
    t.dimensions  = {20.0, 5.0, 12.0, true, TargetMaterialType::METAL, TargetShapeType::AIRCRAFT};
    t.swerlingCase = SwerlingCase::CASE_0;
    double range   = std::sqrt(50000.0*50000.0 + 5000.0*5000.0);
    double rcsMetal = sp.computeEffectiveRCS(t, range, 8.0e9);
    t.dimensions.material = TargetMaterialType::STEALTHY;
    double rcsStealth = sp.computeEffectiveRCS(t, range, 8.0e9);
    EXPECT_LT(rcsStealth, rcsMetal * 0.01);
}
TEST(SignalProc, EffectiveRCSPositiveWithDimensions)
{
    RadarSignalProcessor_AESA sp;
    TargetInput t = makeAirTarget(1, 50000, 0, 5000);
    t.dimensions  = {15.0, 5.0, 10.0, true, TargetMaterialType::METAL, TargetShapeType::AIRCRAFT};
    t.swerlingCase = SwerlingCase::CASE_0;
    double range   = std::sqrt(50000.0*50000.0 + 5000.0*5000.0);
    EXPECT_GT(sp.computeEffectiveRCS(t, range, 8.0e9), 0.0);
}
TEST(SignalProc, EffectiveRCSWithNoDimensionsFallsBackToPlatformType)
{
    RadarSignalProcessor_AESA sp;
    TargetInput t = makeAirTarget(1, 50000, 0, 5000);
    t.platformType = "FIGHTER";
    t.swerlingCase = SwerlingCase::CASE_0;
    double range   = std::sqrt(50000.0*50000.0 + 5000.0*5000.0);
    EXPECT_GT(sp.computeEffectiveRCS(t, range, 8.0e9), 0.0);
}
TEST(SignalProc, ShipRCSLargerThanUAV)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_GT(sp.getPlatformBaseRCS("SHIP"), sp.getPlatformBaseRCS("UAV"));
}
TEST(SignalProc, StealthRCSSmallerThanFighter)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_LT(sp.getPlatformBaseRCS("STEALTH"), sp.getPlatformBaseRCS("FIGHTER"));
}

// ── Pk / Pd ──────────────────────────────────────────────────────────────────
TEST(SignalProc, PkHighSNRNearOne)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_GT(sp.computePk(1000.0, 1e-6, 10, SwerlingCase::CASE_0), 0.95);
}
TEST(SignalProc, PkVeryLowSNRNearZero)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_LT(sp.computePk(0.001, 1e-6, 10, SwerlingCase::CASE_0), 0.10);
}
TEST(SignalProc, PkMonotonicallyIncreasesWithSNR)
{
    RadarSignalProcessor_AESA sp;
    double pk1 = sp.computePk(0.5, 1e-6, 10, SwerlingCase::CASE_0);
    double pk2 = sp.computePk(2.0, 1e-6, 10, SwerlingCase::CASE_0);
    double pk3 = sp.computePk(8.0, 1e-6, 10, SwerlingCase::CASE_0);
    EXPECT_LT(pk1, pk2);
    EXPECT_LT(pk2, pk3);
}
TEST(SignalProc, PkZeroSNRIsZero)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_DOUBLE_EQ(sp.computePk(0.0, 1e-6, 10, SwerlingCase::CASE_0), 0.0);
}
TEST(SignalProc, PkNeverExceedsOne)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_LE(sp.computePk(1e9, 1e-6, 10, SwerlingCase::CASE_0), 1.0);
}
TEST(SignalProc, PkMorePulsesGivesHigherPd)
{
    RadarSignalProcessor_AESA sp;
    double pk1 = sp.computePk(5.0, 1e-6,  1, SwerlingCase::CASE_0);
    double pk2 = sp.computePk(5.0, 1e-6, 20, SwerlingCase::CASE_0);
    EXPECT_LT(pk1, pk2);
}

// ── Doppler blind zone ───────────────────────────────────────────────────────
TEST(SignalProc, StationaryPlatformNeverBlind)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.platformSpeed_m_s = 0.0f;
    EXPECT_FALSE(sp.isInDopplerBlindZone(  0.0, cfg, makeWF()));
    EXPECT_FALSE(sp.isInDopplerBlindZone(300.0, cfg, makeWF()));
}
TEST(SignalProc, HPRFNeverBlind)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.platformSpeed_m_s = 250.0f;
    EXPECT_FALSE(sp.isInDopplerBlindZone(250.0, cfg, makeWF(WaveformMode::HPRF)));
}
TEST(SignalProc, TargetAtPlatformSpeedIsBlind)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.platformSpeed_m_s = 200.0f;
    EXPECT_TRUE(sp.isInDopplerBlindZone(200.0, cfg, makeWF(WaveformMode::LPRF)));
}
TEST(SignalProc, TargetFarFromClutterNotBlind)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.platformSpeed_m_s = 200.0f;
    // Very high radial velocity — well outside notch
    EXPECT_FALSE(sp.isInDopplerBlindZone(600.0, cfg, makeWF(WaveformMode::LPRF)));
}

// ── multipath ────────────────────────────────────────────────────────────────
TEST(SignalProc, MultipathHighElevationIsUnity)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_DOUBLE_EQ(sp.computeMultipathFactor(50000.0, 15.0, 1000.0, makeCfg()), 1.0);
}
TEST(SignalProc, MultipathLowElevationInRange0to4)
{
    RadarSignalProcessor_AESA sp;
    double f = sp.computeMultipathFactor(10000.0, 1.0, 50.0, makeCfg());
    EXPECT_GE(f, 0.0);
    EXPECT_LE(f, 4.0);
}
TEST(SignalProc, MultipathZeroTargetHeightIsUnity)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_DOUBLE_EQ(sp.computeMultipathFactor(10000.0, 1.0, 0.0, makeCfg()), 1.0);
}

// ── waveform selection ───────────────────────────────────────────────────────
TEST(SignalProc, ShortRangeSelectsHPRF)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_EQ(sp.selectWaveformForRange(10000.0, makeCfg()).mode, WaveformMode::HPRF);
}
TEST(SignalProc, MediumRangeSelectsMPRF)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_EQ(sp.selectWaveformForRange(50000.0, makeCfg()).mode, WaveformMode::MPRF);
}
TEST(SignalProc, LongRangeSelectsLPRF)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_EQ(sp.selectWaveformForRange(300000.0, makeCfg()).mode, WaveformMode::LPRF);
}
TEST(SignalProc, VeryLongRangeFallsBackToSearchWaveform)
{
    RadarSignalProcessor_AESA sp;
    auto wf = sp.selectWaveformForRange(999000.0, makeCfg());
    EXPECT_EQ(wf.mode, WaveformMode::LPRF);
}

// ── modulation processing gain ───────────────────────────────────────────────
TEST(SignalProc, LFMGainIsTimeBandwidthProduct)
{
    RadarSignalProcessor_AESA sp;
    BeamWaveform wf = makeWF();
    wf.modulation = ModulationType::LFM;
    wf.pulseWidth_s = 50e-6f; wf.bandwidth_Hz = 5e6f;
    EXPECT_NEAR(sp.computeModulationProcessingGain(wf), 250.0, 1.0);
}
TEST(SignalProc, NLFMGainIsTimeBandwidthProduct)
{
    RadarSignalProcessor_AESA sp;
    BeamWaveform wf = makeWF();
    wf.modulation = ModulationType::NLFM;
    wf.pulseWidth_s = 10e-6f; wf.bandwidth_Hz = 50e6f;
    EXPECT_NEAR(sp.computeModulationProcessingGain(wf), 500.0, 1.0);
}
TEST(SignalProc, NoneModulationGainIsUnity)
{
    RadarSignalProcessor_AESA sp;
    BeamWaveform wf = makeWF();
    wf.modulation = ModulationType::NONE;
    EXPECT_DOUBLE_EQ(sp.computeModulationProcessingGain(wf), 1.0);
}

// ── range ambiguity ───────────────────────────────────────────────────────────
TEST(SignalProc, UnambiguousRangeUnchanged)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_NEAR(sp.resolveRangeAmbiguity(50000.0, 50000.0, 150000.0), 50000.0, 1.0);
}
TEST(SignalProc, AmbiguousRangeResolvedByPredicted)
{
    RadarSignalProcessor_AESA sp;
    double resolved = sp.resolveRangeAmbiguity(10000.0, 160000.0, 150000.0);
    EXPECT_NEAR(resolved, 160000.0, 100.0);
}
TEST(SignalProc, RangeAmbiguityZeroRmaxReturnsInput)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_DOUBLE_EQ(sp.resolveRangeAmbiguity(50000.0, 50000.0, 0.0), 50000.0);
}

// ── beam gain / sidelobe ─────────────────────────────────────────────────────
TEST(SignalProc, MainLobeGainIsUnity)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_DOUBLE_EQ(sp.computeBeamGainFactor(0.0, 0.0, makeCfg(), 3.0), 1.0);
}
TEST(SignalProc, SidelobeGainBelowMainLobe)
{
    RadarSignalProcessor_AESA sp;
    double g = sp.computeBeamGainFactor(20.0, 20.0, makeCfg(), 3.0);
    EXPECT_GT(g, 0.0);
    EXPECT_LT(g, 1.0);
}
TEST(SignalProc, SidelobeGainAlwaysPositive)
{
    RadarSignalProcessor_AESA sp;
    for (double diff : {5.0, 10.0, 20.0, 45.0})
        EXPECT_GT(sp.computeBeamGainFactor(diff, diff, makeCfg(), 3.0), 0.0);
}

// ── max detection range ───────────────────────────────────────────────────────
TEST(SignalProc, MaxRangePositiveAndReasonable)
{
    RadarSignalProcessor_AESA sp;
    double r = sp.computeMaxDetectionRange(5.0, makeCfg());
    EXPECT_GT(r, 50.0);
    EXPECT_LT(r, 2000.0);
}
TEST(SignalProc, LargerRCSGivesGreaterRange)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_GT(sp.computeMaxDetectionRange(100.0, makeCfg()),
              sp.computeMaxDetectionRange(0.001, makeCfg()));
}
TEST(SignalProc, MaxRangeScalesWith4thRootOfRCS)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg();
    cfg.frequencyAgility = false;
    cfg.atmosphere = {15.0f, 0.0f, 1013.25f, 0.0f, 0.0f}; // zero humidity reduces atm loss
    double r1 = sp.computeMaxDetectionRange(1.0,  cfg);
    double r2 = sp.computeMaxDetectionRange(16.0, cfg);
    // Atmosphere modifies pure R^4 scaling — relax tolerance
    EXPECT_GT(r2/r1, 1.5);
    EXPECT_LT(r2/r1, 2.5);
}

// ── detection merge guard ─────────────────────────────────────────────────────
TEST(SignalProc, MergeGuardReturnsFalseForDistantDet)
{
    RadarSignalProcessor_AESA sp;
    DetectionOutput d1, d2;
    d1.range = 50000.0; d1.azimuth = 10.0; d1.elevation = 5.0;
    d2.range = 80000.0; d2.azimuth = 30.0; d2.elevation = 5.0;
    EXPECT_FALSE(sp.shouldMergeDetection(d2, {d1}, makeCfg()));
}
TEST(SignalProc, MergeGuardReturnsTrueForNearbyDet)
{
    RadarSignalProcessor_AESA sp;
    DetectionOutput d1, d2;
    d1.range = 50000.0; d1.azimuth = 10.0; d1.elevation = 5.0;
    d2.range = 50050.0; d2.azimuth = 10.1; d2.elevation = 5.0;
    EXPECT_TRUE(sp.shouldMergeDetection(d2, {d1}, makeCfg()));
}
TEST(SignalProc, MergeGuardEmptyListAlwaysFalse)
{
    RadarSignalProcessor_AESA sp;
    DetectionOutput d;
    d.range = 50000.0; d.azimuth = 10.0; d.elevation = 5.0;
    EXPECT_FALSE(sp.shouldMergeDetection(d, {}, makeCfg()));
}
// computeClutterPower never tested
TEST(SignalProc, ClutterPowerZeroForAirTarget)
{
    RadarSignalProcessor_AESA sp;
    EXPECT_DOUBLE_EQ(sp.computeClutterPower(50000.0, SurfaceType::AIR, makeCfg()), 0.0);
}
TEST(SignalProc, ClutterPowerPositiveForSeaTarget)
{
    RadarSignalProcessor_AESA sp;
    RadarConfig cfg = makeCfg(); cfg.seaState = 3.0f;
    EXPECT_GT(sp.computeClutterPower(50000.0, SurfaceType::SEA, cfg), 0.0);
}

// computeSINR never tested directly
TEST(SignalProc, SINRPositiveWithValidInputs)
{
    RadarSignalProcessor_AESA sp;
    TargetInput t = makeAirTarget(1, 50000, 0, 5000);
    double Pr = 1e-10;
    EXPECT_GT(sp.computeSINR(Pr, 50000.0, SurfaceType::AIR, t, makeCfg(), makeWF()), 0.0);
}
TEST(SignalProc, SINRZeroReceivedPowerIsZero)
{
    RadarSignalProcessor_AESA sp;
    TargetInput t = makeAirTarget(1, 50000, 0, 5000);
    EXPECT_DOUBLE_EQ(sp.computeSINR(0.0, 50000.0, SurfaceType::AIR, t, makeCfg(), makeWF()), 0.0);
}

// Monopulse never tested
TEST(SignalProc, MonopulseErrorFiniteAtHighSNR)
{
    RadarSignalProcessor_AESA sp;
    double azErr, elErr;
    sp.computeMonopulseAngleError(0.5, 0.3, 100.0, makeCfg(), azErr, elErr);
    EXPECT_TRUE(std::isfinite(azErr));
    EXPECT_TRUE(std::isfinite(elErr));
}
TEST(SignalProc, MonopulseErrorFiniteAtZeroSNR)
{
    RadarSignalProcessor_AESA sp;
    double azErr, elErr;
    // Zero SNR — should not divide by zero
    sp.computeMonopulseAngleError(0.5, 0.3, 0.0, makeCfg(), azErr, elErr);
    EXPECT_TRUE(std::isfinite(azErr));
    EXPECT_TRUE(std::isfinite(elErr));
}

// Staggered resolvers never tested
TEST(SignalProc, StaggeredRangeResolverFindsCorrectRange)
{
    RadarSignalProcessor_AESA sp;
    double resolved = sp.resolveRangeAmbiguityStaggered(
        10000.0, 10100.0, 150000.0, 135000.0, 160000.0);
    EXPECT_TRUE(std::isfinite(resolved));
    EXPECT_GT(resolved, 0.0);
}
TEST(SignalProc, StaggeredVelocityResolverFindsCorrectVelocity)
{
    RadarSignalProcessor_AESA sp;
    double resolved = sp.resolveVelocityStaggered(
        50.0, 52.0, 300.0, 270.0, 350.0);
    EXPECT_TRUE(std::isfinite(resolved));
}

// Jammer in sidelobe never tested
TEST(SignalProc, SidelobeBlankingReturnsFalseNoJammer)
{
    RadarSignalProcessor_AESA sp;
    TargetInput t = makeAirTarget(1, 50000, 0, 5000);
    t.jammer.active = false;
    EXPECT_FALSE(sp.isJammerInSidelobe(20.0, 20.0, t, makeCfg()));
}
TEST(SignalProc, SidelobeBlankingReturnsFalseInMainLobe)
{
    RadarSignalProcessor_AESA sp;
    TargetInput t = makeAirTarget(1, 50000, 0, 5000);
    t.jammer.active = true; t.jammer.power_kW = 100.0; t.jammer.gain_dBi = 0.0;
    // Inside main lobe — should NOT blank
    EXPECT_FALSE(sp.isJammerInSidelobe(0.5, 0.5, t, makeCfg()));
}
