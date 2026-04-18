#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include "radarantenna_aesa.h"
#include <limits>

using namespace aesa;

static RadarConfig makeCfg()
{
    RadarConfig cfg;
    cfg.beamWidth             = 3.0f;
    cfg.maxSteeringAngle_deg  = 60.0f;
    cfg.minElevation          = -10.0f;
    cfg.maxElevation          =  60.0f;
    cfg.minAzimuth            = -60.0f;
    cfg.maxAzimuth            =  60.0f;
    cfg.numElements           = 1000;
    cfg.failedModules         = 0;
    cfg.antennaGain           = 34.0f;
    return cfg;
}

TEST(RadarAntenna, PointBeamNaNDoesNotCrash)
{
    RadarAntenna_AESA ant;
    double nan = std::numeric_limits<double>::quiet_NaN();
    ant.pointBeam(nan, nan, makeCfg(), 1.0f);
    // result clamped — just must not crash or produce NaN state
    EXPECT_FALSE(std::isnan(ant.currentAzimuth()));
}

TEST(RadarAntenna, GainNaNSteeringDoesNotCrash)
{
    RadarAntenna_AESA ant;
    double nan = std::numeric_limits<double>::quiet_NaN();
    double gain = ant.computeArrayGain(nan, makeCfg(), 1.0f);
    EXPECT_FALSE(std::isnan(gain));
    EXPECT_GE(gain, 0.0);
}
// ── reset ────────────────────────────────────────────────────────────────────
TEST(RadarAntenna, ResetZerosAllState)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(30.0, 10.0, makeCfg(), 2.5f);
    ant.setScanBoundary();
    ant.reset();
    EXPECT_DOUBLE_EQ(ant.currentAzimuth(),   0.0);
    EXPECT_DOUBLE_EQ(ant.currentElevation(), 0.0);
    EXPECT_FLOAT_EQ (ant.currentSpoilFactor(), 1.0f);
    EXPECT_NEAR     (ant.effectiveBeamWidth(), 0.0, 1e-9);
    EXPECT_FALSE    (ant.scanBoundaryOccurred());
}
TEST(RadarAntenna, ResetOnFreshObjectNoOp)
{
    RadarAntenna_AESA ant;
    ant.reset();
    EXPECT_DOUBLE_EQ(ant.currentAzimuth(), 0.0);
}
TEST(RadarAntenna, DoubleResetIsIdempotent)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(45.0, 5.0, makeCfg(), 1.0f);
    ant.reset();
    ant.reset();
    EXPECT_DOUBLE_EQ(ant.currentAzimuth(), 0.0);
}

// ── pointBeam normal ─────────────────────────────────────────────────────────
TEST(RadarAntenna, PointBeamStoresAngles)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(20.0, 5.0, makeCfg(), 1.0f);
    EXPECT_DOUBLE_EQ(ant.currentAzimuth(),   20.0);
    EXPECT_DOUBLE_EQ(ant.currentElevation(),  5.0);
}
TEST(RadarAntenna, PointBeamAtBoresight)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(0.0, 0.0, makeCfg(), 1.0f);
    EXPECT_DOUBLE_EQ(ant.currentAzimuth(),   0.0);
    EXPECT_DOUBLE_EQ(ant.currentElevation(), 0.0);
}
TEST(RadarAntenna, PointBeamNegativeAzimuth)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(-30.0, 0.0, makeCfg(), 1.0f);
    EXPECT_DOUBLE_EQ(ant.currentAzimuth(), -30.0);
}
TEST(RadarAntenna, PointBeamNegativeElevation)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(0.0, -5.0, makeCfg(), 1.0f);
    EXPECT_DOUBLE_EQ(ant.currentElevation(), -5.0);
}

// ── pointBeam clamping ───────────────────────────────────────────────────────
TEST(RadarAntenna, ClampsAzimuthBeyondMaxSteering)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    ant.pointBeam(85.0, 0.0, cfg, 1.0f);
    EXPECT_LE(ant.currentAzimuth(), static_cast<double>(cfg.maxAzimuth));
}
TEST(RadarAntenna, ClampsElevationAboveMax)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    ant.pointBeam(0.0, 80.0, cfg, 1.0f);
    EXPECT_LE(ant.currentElevation(), static_cast<double>(cfg.maxElevation));
}
TEST(RadarAntenna, ClampsElevationBelowMin)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    ant.pointBeam(0.0, -90.0, cfg, 1.0f);
    EXPECT_GE(ant.currentElevation(), static_cast<double>(cfg.minElevation));
}
TEST(RadarAntenna, ExtremeAzimuth360DoesNotCrash)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    ant.pointBeam(360.0, 0.0, cfg, 1.0f);
    EXPECT_LE(ant.currentAzimuth(), static_cast<double>(cfg.maxAzimuth));
}
TEST(RadarAntenna, ExtremeNegativeAzimuthDoesNotCrash)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    ant.pointBeam(-360.0, 0.0, cfg, 1.0f);
    EXPECT_GE(ant.currentAzimuth(), static_cast<double>(cfg.minAzimuth));
}

// ── spoil factor ─────────────────────────────────────────────────────────────
TEST(RadarAntenna, SpoilFactorBelowOneClampedToOne)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(0.0, 0.0, makeCfg(), 0.1f);
    EXPECT_GE(ant.currentSpoilFactor(), 1.0f);
}
TEST(RadarAntenna, SpoilFactorZeroClampedToOne)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(0.0, 0.0, makeCfg(), 0.0f);
    EXPECT_GE(ant.currentSpoilFactor(), 1.0f);
}
TEST(RadarAntenna, SpoilFactorNegativeClampedToOne)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(0.0, 0.0, makeCfg(), -5.0f);
    EXPECT_GE(ant.currentSpoilFactor(), 1.0f);
}
TEST(RadarAntenna, SpoilFactorLargeValueAccepted)
{
    RadarAntenna_AESA ant;
    ant.pointBeam(0.0, 0.0, makeCfg(), 10.0f);
    EXPECT_FLOAT_EQ(ant.currentSpoilFactor(), 10.0f);
}
TEST(RadarAntenna, EffectiveBeamWidthScalesWithSpoil)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    ant.pointBeam(0.0, 0.0, cfg, 3.0f);
    EXPECT_NEAR(ant.effectiveBeamWidth(), static_cast<double>(cfg.beamWidth)*3.0, 1e-6);
}
TEST(RadarAntenna, EffectiveBeamWidthSpoilOneEqualsBeamWidth)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    ant.pointBeam(0.0, 0.0, cfg, 1.0f);
    EXPECT_NEAR(ant.effectiveBeamWidth(), static_cast<double>(cfg.beamWidth), 1e-6);
}

// ── isReachable ───────────────────────────────────────────────────────────────
TEST(RadarAntenna, BoresightAlwaysReachable)
{
    RadarAntenna_AESA ant;
    EXPECT_TRUE(ant.isReachable(0.0, 0.0, makeCfg()));
}
TEST(RadarAntenna, JustInsideLimitReachable)
{
    RadarAntenna_AESA ant;
    EXPECT_TRUE(ant.isReachable(58.0, 0.0, makeCfg()));
}
TEST(RadarAntenna, BeyondLimitNotReachable)
{
    RadarAntenna_AESA ant;
    EXPECT_FALSE(ant.isReachable(75.0, 0.0, makeCfg()));
}
TEST(RadarAntenna, WellBeyondLimitNotReachable)
{
    RadarAntenna_AESA ant;
    EXPECT_FALSE(ant.isReachable(89.0, 89.0, makeCfg()));
}
TEST(RadarAntenna, NegativeAzimuthWithinLimitReachable)
{
    RadarAntenna_AESA ant;
    EXPECT_TRUE(ant.isReachable(-45.0, 0.0, makeCfg()));
}
TEST(RadarAntenna, ZeroMaxSteeringOnlyBoresightReachable)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    cfg.maxSteeringAngle_deg = 0.0f;
    EXPECT_TRUE (ant.isReachable(0.0, 0.0, cfg));
    EXPECT_FALSE(ant.isReachable(1.0, 0.0, cfg));
}

// ── computeSteeringAngle ─────────────────────────────────────────────────────
TEST(RadarAntenna, SteeringAngleSamePointIsZero)
{
    RadarAntenna_AESA ant;
    EXPECT_NEAR(ant.computeSteeringAngle(0.0, 0.0, 0.0, 0.0), 0.0, 1e-9);
}
TEST(RadarAntenna, SteeringAngle90DegAz)
{
    RadarAntenna_AESA ant;
    EXPECT_NEAR(ant.computeSteeringAngle(0.0, 0.0, 90.0, 0.0), 90.0, 1e-5);
}
TEST(RadarAntenna, SteeringAngleSymmetric)
{
    RadarAntenna_AESA ant;
    double aPos = ant.computeSteeringAngle(0.0, 0.0,  30.0, 0.0);
    double aNeg = ant.computeSteeringAngle(0.0, 0.0, -30.0, 0.0);
    EXPECT_NEAR(aPos, aNeg, 1e-9);
}
TEST(RadarAntenna, SteeringAngleAlwaysNonNegative)
{
    RadarAntenna_AESA ant;
    EXPECT_GE(ant.computeSteeringAngle( 10.0,  5.0, -10.0, -5.0), 0.0);
    EXPECT_GE(ant.computeSteeringAngle(-30.0, -5.0,  30.0,  5.0), 0.0);
}
TEST(RadarAntenna, SteeringAngle180OppositeHemisphere)
{
    RadarAntenna_AESA ant;
    EXPECT_NEAR(ant.computeSteeringAngle(0.0, 0.0, 180.0, 0.0), 180.0, 1e-5);
}

// ── computeArrayGain ─────────────────────────────────────────────────────────
TEST(RadarAntenna, GainAtBoresightMatchesAntennaGain)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    double G_bore = std::pow(10.0, static_cast<double>(cfg.antennaGain)/10.0);
    EXPECT_NEAR(ant.computeArrayGain(0.0, cfg, 1.0f)/G_bore, 1.0, 0.02);
}
TEST(RadarAntenna, GainMonotonicallyDecreasesWithSteering)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    double g0  = ant.computeArrayGain( 0.0, cfg, 1.0f);
    double g30 = ant.computeArrayGain(30.0, cfg, 1.0f);
    double g60 = ant.computeArrayGain(60.0, cfg, 1.0f);
    EXPECT_GT(g0,  g30);
    EXPECT_GT(g30, g60);
}
TEST(RadarAntenna, GainAlwaysPositive)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    for (double a : {0.0, 15.0, 30.0, 45.0, 59.0})
        EXPECT_GT(ant.computeArrayGain(a, cfg, 1.0f), 0.0);
}
TEST(RadarAntenna, GainHalvedModulesReducesToQuarter)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    double gFull = ant.computeArrayGain(0.0, cfg, 1.0f);
    cfg.failedModules = 500;
    double gHalf = ant.computeArrayGain(0.0, cfg, 1.0f);
    EXPECT_NEAR(gHalf/gFull, 0.25, 0.03);
}
TEST(RadarAntenna, GainAllModulesFailedNearZero)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    cfg.failedModules = cfg.numElements;
    EXPECT_NEAR(ant.computeArrayGain(0.0, cfg, 1.0f), 0.0, 1e-6);
}
TEST(RadarAntenna, GainSpoilTwoReducesToQuarter)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    double g1 = ant.computeArrayGain(0.0, cfg, 1.0f);
    double g2 = ant.computeArrayGain(0.0, cfg, 2.0f);
    EXPECT_NEAR(g2*4.0, g1, g1*0.02);
}
TEST(RadarAntenna, GainZeroElementsIsZero)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    cfg.numElements  = 0;
    cfg.failedModules = 0;
    EXPECT_NEAR(ant.computeArrayGain(0.0, cfg, 1.0f), 0.0, 1e-6);
}
TEST(RadarAntenna, GainMoreFailedThanTotalDoesNotCrash)
{
    RadarAntenna_AESA ant;
    RadarConfig cfg = makeCfg();
    cfg.failedModules = cfg.numElements + 100;
    EXPECT_GE(ant.computeArrayGain(0.0, cfg, 1.0f), 0.0);
}

// ── scan boundary ────────────────────────────────────────────────────────────
TEST(RadarAntenna, ScanBoundaryInitiallyFalse)
{
    RadarAntenna_AESA ant;
    EXPECT_FALSE(ant.scanBoundaryOccurred());
}
TEST(RadarAntenna, ScanBoundarySetAndClear)
{
    RadarAntenna_AESA ant;
    ant.setScanBoundary();
    EXPECT_TRUE(ant.scanBoundaryOccurred());
    ant.clearScanBoundary();
    EXPECT_FALSE(ant.scanBoundaryOccurred());
}
TEST(RadarAntenna, ScanBoundaryDoubleSet)
{
    RadarAntenna_AESA ant;
    ant.setScanBoundary();
    ant.setScanBoundary();
    EXPECT_TRUE(ant.scanBoundaryOccurred());
}
TEST(RadarAntenna, ScanBoundaryClearedByReset)
{
    RadarAntenna_AESA ant;
    ant.setScanBoundary();
    ant.reset();
    EXPECT_FALSE(ant.scanBoundaryOccurred());
}
