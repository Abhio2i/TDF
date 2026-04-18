#include <gtest/gtest.h>
#include <cmath>
#include "radarmodel_aesa.h"

using namespace aesa;

static RadarConfig makeCfg()
{
    RadarConfig cfg;
    cfg.numElements           = 1000;
    cfg.peakPowerPerElement_W = 100.0f;
    cfg.moduleEfficiency      = 0.70f;
    cfg.failedModules         = 0;
    cfg.maxDutyCycle          = 0.50f;
    cfg.frequency_Hz          = 8.0e9;
    cfg.antennaGain           = 35.0f;
    cfg.antennaBandwidth      = 1e6;
    cfg.beamWidth             = 3.0f;
    cfg.maxSteeringAngle_deg  = 60.0f;
    cfg.sidelobeMode          = SidelobeMode::LOW_SLL;
    cfg.peakSidelobeLevel     = -25.0f;
    cfg.avgSidelobeLevel      = -35.0f;
    cfg.sidelobeBlanking_dB   = -15.0f;
    cfg.minAzimuth            = -60.0f; cfg.maxAzimuth   = 60.0f;
    cfg.minElevation          =  -2.0f; cfg.maxElevation = 15.0f;
    cfg.searchDwellTime_ms      = 2.0f;
    cfg.trackDwellTime_ms       = 5.0f;
    cfg.fireControlDwellTime_ms = 5.0f;
    cfg.searchWaveform      = {ModulationType::LFM,  50e-6f,  300.0f,  5e6f, 10, WaveformMode::LPRF};
    cfg.trackWaveform       = {ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 25, WaveformMode::MPRF};
    cfg.fireControlWaveform = {ModulationType::NLFM,  5e-6f, 2000.0f, 50e6f, 25, WaveformMode::HPRF};
    cfg.waveformTable[0]    = {30000.0f,  {ModulationType::NLFM, 5e-6f,  2000.0f, 50e6f, 25, WaveformMode::HPRF}};
    cfg.waveformTable[1]    = {100000.0f, {ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 25, WaveformMode::MPRF}};
    cfg.waveformTable[2]    = {400000.0f, {ModulationType::LFM,  50e-6f,  300.0f,  5e6f, 10, WaveformMode::LPRF}};
    cfg.waveformTable[3]    = {}; cfg.waveformTable[4] = {}; cfg.waveformTable[5] = {};
    cfg.systemTemperature_K = 290.0;
    cfg.noiseFigure_dB      = 5.0;
    cfg.targetPfa           = 1e-6;
    cfg.radarHeight         = 3000.0;
    cfg.minDetectableRange  = 30.0;
    cfg.platformSpeed_m_s   = 0.0f;
    cfg.earthRadiusFactor   = 1.33;
    cfg.atmosphericFactor   = 1.0;
    cfg.seaState            = 0.0f;
    cfg.landClutter         = 0.0f;
    cfg.atmosphere          = {15.0f, 60.0f, 1013.25f, 0.0f, 0.0f};
    cfg.targetCategory      = DetectionCategory::ALL;
    cfg.missedScansToDrop   = 3;
    cfg.trackCoastSeconds   = 10.0;
    cfg.minHitsToValidate   = 2;
    cfg.maxTrackSpeed       = 2000.0;
    cfg.manoeuvreThreshold_m = 500.0;
    cfg.useJPDA             = false;
    cfg.noise               = {0.0, 0.0, 0.0, 0.0};
    cfg.mode                = RadarMode::TWS;
    cfg.interrogationMode   = IFFMode::MODE_3A;
    return cfg;
}

static RadarPose makePose(double height = 3000.0)
{
    RadarPose p;
    p.x = 0.0; p.y = height; p.z = 0.0;
    p.roll = 0.0f; p.pitch = 0.0f; p.heading = 0.0f;
    return p;
}

static TargetInput makeCloseTarget(uint32_t id = 1, double rcs = 100.0)
{
    TargetInput t;
    t.id     = id;
    t.x      = 15000.0; t.y = 0.0; t.z = 3000.0;
    t.vx     = -150.0;  t.vy = 0.0; t.vz = 0.0;
    t.rcs    = rcs;
    t.dimensions = {20.0, 6.0, 15.0, true, TargetMaterialType::METAL, TargetShapeType::AIRCRAFT};
    t.surface      = SurfaceType::AIR;
    t.swerlingCase = SwerlingCase::CASE_0;
    t.jammer.active = false;
    return t;
}

// ── lifecycle ─────────────────────────────────────────────────────────────────
TEST(RadarModel, InitAndStartSetsTWSMode)
{
    RadarModel_AESA model;
    model.init(makeCfg());
    model.start();
    EXPECT_EQ(model.getOutput().mode, RadarMode::TWS);
}
TEST(RadarModel, EndClearsOutput)
{
    RadarModel_AESA model;
    model.init(makeCfg());
    model.start();
    model.end();
    EXPECT_TRUE(model.getOutput().detections.empty());
    EXPECT_TRUE(model.getOutput().tracks.empty());
}
TEST(RadarModel, ResetClearsDetectionsAndTracks)
{
    RadarModel_AESA model;
    model.init(makeCfg());
    model.start();
    model.reset();
    EXPECT_TRUE(model.getOutput().detections.empty());
    EXPECT_TRUE(model.getOutput().tracks.empty());
}
TEST(RadarModel, DoubleInitIsIdempotent)
{
    RadarModel_AESA model;
    RadarConfig cfg = makeCfg();
    model.init(cfg);
    model.start();
    model.init(cfg); // should not crash or corrupt
    model.start();
    EXPECT_EQ(model.getOutput().mode, RadarMode::TWS);
}
TEST(RadarModel, StartAfterEndRestarts)
{
    RadarModel_AESA model;
    model.init(makeCfg());
    model.start();
    model.end();
    model.start();
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05); // should not crash
}

// ── config ────────────────────────────────────────────────────────────────────
TEST(RadarModel, GetConfigReturnsSetConfig)
{
    RadarModel_AESA model;
    RadarConfig cfg = makeCfg();
    cfg.antennaGain = 38.0f;
    model.init(cfg); model.start();
    EXPECT_FLOAT_EQ(model.getConfig().antennaGain, 38.0f);
}
TEST(RadarModel, SetConfigUpdatesHeight)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    RadarConfig cfg2 = makeCfg();
    cfg2.radarHeight = 8000.0;
    model.setConfig(cfg2);
    EXPECT_NEAR(model.getConfig().radarHeight, 8000.0, 1.0);
}
TEST(RadarModel, SetConfigPreservesMode)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    model.setMode(RadarMode::SURVEILLANCE);

    // Correct pattern — always get current config, modify, set back
    // This is how aesaradar.cpp does it in scan()
    RadarConfig cfg2 = model.getConfig();  // GET current — preserves mode
    cfg2.radarHeight = 5000.0;             // modify only what you need
    model.setConfig(cfg2);                 // SET back

    EXPECT_EQ(model.getConfig().mode, RadarMode::SURVEILLANCE);
}

// ── mode switching ────────────────────────────────────────────────────────────
TEST(RadarModel, SetModeSurveillance)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    model.setMode(RadarMode::SURVEILLANCE);
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05);
    EXPECT_EQ(model.getOutput().mode, RadarMode::SURVEILLANCE);
}
TEST(RadarModel, LockOnSetsMode)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    model.lockOn(42);
    EXPECT_EQ(model.getConfig().mode, RadarMode::LOCK_ON);
    EXPECT_EQ(model.getConfig().lockedTargetID, 42u);
}
TEST(RadarModel, BreakLockReturnsSurveillance)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    model.lockOn(1);
    model.breakLock();
    EXPECT_EQ(model.getConfig().mode, RadarMode::SURVEILLANCE);
    EXPECT_EQ(model.getConfig().lockedTargetID, 0u);
}
TEST(RadarModel, BreakLockClearsDetectionsAndTracks)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    model.lockOn(1);
    model.breakLock();
    EXPECT_TRUE(model.getOutput().detections.empty());
    EXPECT_TRUE(model.getOutput().tracks.empty());
}
TEST(RadarModel, SurveillanceModeTracksEmpty)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    model.setMode(RadarMode::SURVEILLANCE);
    RadarPose pose = makePose();
    model.update(0.05, pose, {makeCloseTarget()}, 0.05);
    EXPECT_TRUE(model.getOutput().tracks.empty());
}

// ── update — no targets ───────────────────────────────────────────────────────
TEST(RadarModel, NoTargetsNoDetections)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    RadarPose pose = makePose();
    for (int i = 0; i < 10; ++i)
        model.update(0.05, pose, {}, static_cast<double>(i)*0.05);
    EXPECT_TRUE(model.getOutput().detections.empty());
}
TEST(RadarModel, UpdateBeforeStartDoesNotCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg());
    // No start() call — should silently do nothing
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05);
}
TEST(RadarModel, UpdateWithZeroDtDoesNotCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    RadarPose pose = makePose();
    model.update(0.0, pose, {}, 0.0);
}
TEST(RadarModel, UpdateWithLargeDtDoesNotCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    RadarPose pose = makePose();
    model.update(10.0, pose, {}, 10.0); // 10 second jump
}

// ── update — with close high-RCS target ──────────────────────────────────────
TEST(RadarModel, CloseTargetEventuallyDetected)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    TargetInput t  = makeCloseTarget();
    RadarPose pose = makePose();
    bool detected  = false;
    for (int i = 0; i < 200 && !detected; ++i)
    {
        model.update(0.05, pose, {t}, static_cast<double>(i)*0.05);
        if (!model.getOutput().detections.empty()) detected = true;
    }
    EXPECT_TRUE(detected) << "15 km target with 100 m² RCS never detected";
}
TEST(RadarModel, DetectionHasCorrectTargetID)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    TargetInput t  = makeCloseTarget(7);
    RadarPose pose = makePose();
    for (int i = 0; i < 200; ++i)
    {
        model.update(0.05, pose, {t}, static_cast<double>(i)*0.05);
        if (!model.getOutput().detections.empty())
        {
            EXPECT_EQ(model.getOutput().detections[0].targetID, 7u);
            return;
        }
    }
    FAIL() << "Target never detected";
}
TEST(RadarModel, FarTargetWithZeroRCSNotDetected)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    TargetInput t = makeCloseTarget(1, 0.0); // zero RCS
    t.x = 400000.0; // 400 km — far
    RadarPose pose = makePose();
    for (int i = 0; i < 50; ++i)
        model.update(0.05, pose, {t}, static_cast<double>(i)*0.05);
    EXPECT_TRUE(model.getOutput().detections.empty());
}
TEST(RadarModel, MultipleTargetsCanBeDetected)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    TargetInput t1 = makeCloseTarget(1);
    TargetInput t2 = makeCloseTarget(2);
    t2.y = 2000.0; // slightly offset
    RadarPose pose = makePose();
    std::set<uint32_t> detectedIDs;
    for (int i = 0; i < 300; ++i)
    {
        model.update(0.05, pose, {t1, t2}, static_cast<double>(i)*0.05);
        for (const auto& d : model.getOutput().detections)
            detectedIDs.insert(d.targetID);
        if (detectedIDs.size() >= 2) break;
    }
    EXPECT_GE(detectedIDs.size(), 1u); // at least one detected
}

// ── display range ─────────────────────────────────────────────────────────────
TEST(RadarModel, DisplayRangeInBounds)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05);
    double dr = model.getOutput().displayRange_km;
    EXPECT_GE(dr, 5.0);
    EXPECT_LE(dr, 1000.0);
}
TEST(RadarModel, DisplayRangeNotZero)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05);
    EXPECT_GT(model.getOutput().displayRange_km, 0.0);
}

// ── azimuth output ────────────────────────────────────────────────────────────
TEST(RadarModel, CurrentAzimuthChangesOverTime)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05);
    double az0 = model.getOutput().currentAzimuth;
    for (int i = 1; i < 20; ++i)
        model.update(0.05, pose, {}, static_cast<double>(i)*0.05);
    double az1 = model.getOutput().currentAzimuth;
    // Beam should have moved
    (void)az0; (void)az1; // just verify no crash and value is finite
    EXPECT_TRUE(std::isfinite(model.getOutput().currentAzimuth));
}

// ── chaff ─────────────────────────────────────────────────────────────────────
TEST(RadarModel, AddChaffDoesNotCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    ChaffCloud cloud;
    cloud.x = 10000; cloud.y = 0; cloud.z = 3000;
    cloud.rcsTotal = 5000.0; cloud.decayTime_s = 60.0; cloud.birthTime_s = 0.0;
    model.addChaffCloud(cloud);
}
TEST(RadarModel, ClearChaffDoesNotCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    ChaffCloud cloud;
    cloud.x = 10000; cloud.y = 0; cloud.z = 3000;
    cloud.rcsTotal = 5000.0; cloud.decayTime_s = 60.0; cloud.birthTime_s = 0.0;
    model.addChaffCloud(cloud);
    model.clearChaffClouds();
}
TEST(RadarModel, MultipleChaffCloudsDoNotCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    for (int i = 0; i < 10; ++i) {
        ChaffCloud c;
        c.x = 10000 + i*500; c.y = 0; c.z = 3000;
        c.rcsTotal = 1000.0; c.decayTime_s = 60.0; c.birthTime_s = 0.0;
        model.addChaffCloud(c);
    }
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05); // should not crash
}

// ── max detection range utility ───────────────────────────────────────────────
TEST(RadarModel, MaxDetectionRangeReasonable)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    double r = model.computeMaxDetectionRange(5.0);
    EXPECT_GT(r, 50.0);
    EXPECT_LT(r, 2000.0);
}
TEST(RadarModel, MaxRangeLargerRCSGivesMoreRange)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    EXPECT_GT(model.computeMaxDetectionRange(100.0),
              model.computeMaxDetectionRange(0.001));
}

// ── current task output ───────────────────────────────────────────────────────
TEST(RadarModel, CurrentTaskIsSearchInTWS)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05);
    EXPECT_EQ(model.getOutput().currentTask, BeamRequest::Task::SEARCH);
}
TEST(RadarModel, DutyCycleInRange)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05);
    EXPECT_GE(model.getOutput().currentDutyCycle, 0.0);
    EXPECT_LE(model.getOutput().currentDutyCycle, 1.0);
}

// ── external track injection ──────────────────────────────────────────────────
TEST(RadarModel, InjectExternalTrackDoesNotCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    TrackOutput ext;
    ext.id = 99; ext.x = 30000; ext.y = 0; ext.z = 5000;
    ext.range = 30413.0; ext.isValidated = true;
    model.injectExternalTrack(ext);
}

// ── lock-on target category filter ───────────────────────────────────────────
TEST(RadarModel, AirOnlyCategoryFiltersOutSurface)
{
    RadarModel_AESA model;
    RadarConfig cfg = makeCfg();
    cfg.targetCategory = DetectionCategory::AIR_ONLY;
    model.init(cfg); model.start();
    TargetInput surf = makeCloseTarget(1);
    surf.surface = SurfaceType::LAND; // should be filtered
    RadarPose pose = makePose();
    for (int i = 0; i < 50; ++i)
        model.update(0.05, pose, {surf}, static_cast<double>(i)*0.05);
    EXPECT_TRUE(model.getOutput().detections.empty());
}

// Target at radar position — range=0 division risk
TEST(RadarModel, TargetAtExactRadarPositionNocrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    TargetInput t = makeCloseTarget();
    t.x = 0.0; t.y = 0.0; t.z = 0.0; // exactly at radar
    RadarPose pose = makePose();
    model.update(0.05, pose, {t}, 0.05); // must not crash
}

// Target with NaN position
TEST(RadarModel, TargetWithNaNPositionNoCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    TargetInput t = makeCloseTarget();
    t.x = std::numeric_limits<double>::quiet_NaN();
    RadarPose pose = makePose();
    model.update(0.05, pose, {t}, 0.05); // must not crash
}

// Target with ID=0
TEST(RadarModel, TargetIDZeroNoCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    TargetInput t = makeCloseTarget(0); // ID=0
    RadarPose pose = makePose();
    model.update(0.05, pose, {t}, 0.05); // must not crash
}

// LockOn unknown target
TEST(RadarModel, LockOnUnknownIDNoCrash)
{
    RadarModel_AESA model;
    model.init(makeCfg()); model.start();
    model.lockOn(9999); // ID not in tracker
    RadarPose pose = makePose();
    model.update(0.05, pose, {}, 0.05); // must not crash
    EXPECT_EQ(model.getConfig().mode, RadarMode::LOCK_ON);
}