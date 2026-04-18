#include <gtest/gtest.h>
#include <cmath>
#include "radarscheduler.h"

using namespace aesa;

static RadarConfig makeCfg()
{
    RadarConfig cfg;
    cfg.beamWidth               = 3.0f;
    cfg.minAzimuth              = -30.0f; cfg.maxAzimuth   = 30.0f;
    cfg.minElevation            =   0.0f; cfg.maxElevation =  6.0f;
    cfg.searchDwellTime_ms      = 2.0f;
    cfg.trackDwellTime_ms       = 5.0f;
    cfg.fireControlDwellTime_ms = 5.0f;
    cfg.maxDutyCycle            = 0.5f;
    cfg.mode                    = RadarMode::TWS;
    cfg.lockedTargetID          = 0;
    cfg.searchWaveform      = {ModulationType::LFM,  50e-6f,  300.0f,  5e6f, 10, WaveformMode::LPRF};
    cfg.trackWaveform       = {ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 25, WaveformMode::MPRF};
    cfg.fireControlWaveform = {ModulationType::NLFM,  5e-6f, 2000.0f, 50e6f, 25, WaveformMode::HPRF};
    return cfg;
}

static TrackFile makeTrack(uint32_t id, double x, double y, double z,
                            bool validated = true, bool manoeuver = false)
{
    TrackFile tr;
    tr.id             = id;
    tr.x = x; tr.y = y; tr.z = z;
    tr.predictedRange = std::sqrt(x*x + y*y + z*z);
    tr.isValidated    = validated;
    tr.isManoeuvring  = manoeuver;
    return tr;
}

// ── lifecycle ────────────────────────────────────────────────────────────────
TEST(Scheduler, InvertedAzimuthRangeDoesNotCrash)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.minAzimuth =  60.0f;
    cfg.maxAzimuth = -60.0f;  // inverted
    s.buildSchedule(cfg, {}); // should not crash or infinite loop
}

TEST(Scheduler, ZeroMissedScansToDrop)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.searchDwellTime_ms = 0.0f; // zero dwell
    s.buildSchedule(cfg, {});
    s.advance(0.001); // should not crash
}
TEST(Scheduler, ResetClearsAll)
{
    RadarScheduler s;
    s.buildSchedule(makeCfg(), {});
    s.reset();
    EXPECT_EQ(s.scheduleSize(), 0);
    EXPECT_EQ(s.searchGridSize(), 0);
    EXPECT_FALSE(s.scanCompleted());
}
TEST(Scheduler, ResetOnFreshObjectNoOp)
{
    RadarScheduler s;
    s.reset();
    EXPECT_EQ(s.scheduleSize(), 0);
}

// ── build ────────────────────────────────────────────────────────────────────
TEST(Scheduler, BuildScheduleNotEmpty)
{
    RadarScheduler s;
    s.buildSchedule(makeCfg(), {});
    EXPECT_GT(s.scheduleSize(), 0);
}
TEST(Scheduler, SearchGridSizeReasonable)
{
    RadarScheduler s;
    s.buildSchedule(makeCfg(), {});
    EXPECT_GT(s.searchGridSize(), 0);
    EXPECT_LT(s.searchGridSize(), 500);
}
TEST(Scheduler, FirstBeamInTWSIsSearch)
{
    RadarScheduler s;
    s.buildSchedule(makeCfg(), {});
    EXPECT_EQ(s.currentBeam().task, BeamRequest::Task::SEARCH);
}
TEST(Scheduler, RebuildResetsIndex)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    s.buildSchedule(cfg, {});
    // advance a few beams
    double dwell = static_cast<double>(cfg.searchDwellTime_ms)/1000.0 + 0.001;
    for (int i = 0; i < 3; ++i) s.advance(dwell);
    s.buildSchedule(cfg, {});
    EXPECT_EQ(s.currentIndex(), 0);
}

// ── lock-on ──────────────────────────────────────────────────────────────────
TEST(Scheduler, LockOnScheduleHasSingleFCBeam)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.mode = RadarMode::LOCK_ON; cfg.lockedTargetID = 42;
    s.buildSchedule(cfg, {});
    EXPECT_EQ(s.scheduleSize(), 1);
    EXPECT_EQ(s.currentBeam().task,     BeamRequest::Task::FIRE_CONTROL);
    EXPECT_EQ(s.currentBeam().targetID, 42u);
}
TEST(Scheduler, LockOnBeamUsesTrackPosition)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.mode = RadarMode::LOCK_ON; cfg.lockedTargetID = 7;
    TrackFile tr = makeTrack(7, 50000, 10000, 5000);
    s.buildSchedule(cfg, {tr});
    double expectedAz = std::atan2(tr.y, tr.x) * (180.0/M_PI);
    EXPECT_NEAR(s.currentBeam().azimuth_deg, expectedAz, 0.5);
}
TEST(Scheduler, LockOnWithNoTrackUsesDefault)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.mode = RadarMode::LOCK_ON; cfg.lockedTargetID = 99;
    s.buildSchedule(cfg, {}); // no matching track
    EXPECT_EQ(s.currentBeam().task, BeamRequest::Task::FIRE_CONTROL);
}
TEST(Scheduler, LockOnFCBeamHasHighestPriority)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.mode = RadarMode::LOCK_ON; cfg.lockedTargetID = 1;
    s.buildSchedule(cfg, {makeTrack(1, 50000, 0, 5000)});
    EXPECT_GE(s.currentBeam().priority, 100);
}

// ── track interleaving ────────────────────────────────────────────────────────
TEST(Scheduler, ValidatedTrackExpandsSchedule)
{
    RadarScheduler s1, s2;
    RadarConfig cfg = makeCfg();
    s1.buildSchedule(cfg, {});
    int noTrack = s1.scheduleSize();
    s2.buildSchedule(cfg, {makeTrack(1, 50000, 0, 5000)});
    EXPECT_GT(s2.scheduleSize(), noTrack);
}
TEST(Scheduler, UnvalidatedTrackNotInterleaved)
{
    RadarScheduler s1, s2;
    RadarConfig cfg = makeCfg();
    s1.buildSchedule(cfg, {});
    s2.buildSchedule(cfg, {makeTrack(1, 50000, 0, 5000, false)}); // not validated
    EXPECT_EQ(s1.scheduleSize(), s2.scheduleSize());
}
TEST(Scheduler, ManoeuveringTargetDoublesTrackBeams)
{
    RadarScheduler s1, s2;
    RadarConfig cfg = makeCfg();
    s1.buildSchedule(cfg, {makeTrack(1, 50000, 0, 5000, true, false)});
    s2.buildSchedule(cfg, {makeTrack(1, 50000, 0, 5000, true, true)});
    EXPECT_GT(s2.scheduleSize(), s1.scheduleSize());
}
TEST(Scheduler, MultipleTracksAllInterleaved)
{
    RadarScheduler s1, s2;
    RadarConfig cfg = makeCfg();
    s1.buildSchedule(cfg, {});
    s2.buildSchedule(cfg, {
        makeTrack(1, 50000, 0,     5000),
        makeTrack(2, 60000, 10000, 5000),
        makeTrack(3, 70000, 5000,  5000)
    });
    EXPECT_GT(s2.scheduleSize(), s1.scheduleSize() + 2);
}

// ── scan completion ───────────────────────────────────────────────────────────
TEST(Scheduler, ScanCompleteAfterAllSearchBeams)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.minAzimuth = -6.0f; cfg.maxAzimuth = 6.0f;
    cfg.minElevation = 0.0f; cfg.maxElevation = 3.0f;
    s.buildSchedule(cfg, {});
    double dwell = static_cast<double>(cfg.searchDwellTime_ms)/1000.0 + 0.001;
    for (int i = 0; i < s.scheduleSize()*4; ++i)
    {
        s.advance(dwell);
        if (s.scanCompleted()) break;
    }
    EXPECT_TRUE(s.scanCompleted());
}
TEST(Scheduler, ScanCompleteFlagClearsOnNextTick)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.minAzimuth = -3.0f; cfg.maxAzimuth = 3.0f;
    cfg.minElevation = 0.0f; cfg.maxElevation = 3.0f;
    s.buildSchedule(cfg, {});
    double dwell = static_cast<double>(cfg.searchDwellTime_ms)/1000.0 + 0.001;
    for (int i = 0; i < 40; ++i) s.advance(dwell);
    EXPECT_TRUE(s.scanCompleted());
    s.advance(0.001);
    EXPECT_FALSE(s.scanCompleted());
}
TEST(Scheduler, ScanNotCompleteInitially)
{
    RadarScheduler s;
    s.buildSchedule(makeCfg(), {});
    EXPECT_FALSE(s.scanCompleted());
}

// ── duty cycle ────────────────────────────────────────────────────────────────
TEST(Scheduler, DutyCycleEnforcedOnOverBudgetWaveform)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.maxDutyCycle = 0.1f;
    cfg.searchWaveform.pulseWidth_s = 0.01f;
    cfg.searchWaveform.prf_Hz       = 1000.0f;
    s.buildSchedule(cfg, {});
    float dc = s.currentBeam().waveform.pulseWidth_s
             * s.currentBeam().waveform.prf_Hz;
    EXPECT_LE(dc, cfg.maxDutyCycle + 0.01f);
}
TEST(Scheduler, DutyCycleQueryInRange)
{
    RadarScheduler s;
    s.buildSchedule(makeCfg(), {});
    EXPECT_GE(s.currentDutyCycle(), 0.0);
    EXPECT_LE(s.currentDutyCycle(), 1.0);
}
TEST(Scheduler, DutyCycleUpdatedAfterAdvance)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    s.buildSchedule(cfg, {});
    s.advance(static_cast<double>(cfg.searchDwellTime_ms)/1000.0 + 0.001);
    EXPECT_GE(s.currentDutyCycle(), 0.0);
}

// ── advance / index ───────────────────────────────────────────────────────────
TEST(Scheduler, AdvanceBeyondDwellMovesIndex)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    s.buildSchedule(cfg, {});
    int idx0 = s.currentIndex();
    s.advance(static_cast<double>(cfg.searchDwellTime_ms)/1000.0 + 0.01);
    EXPECT_NE(s.currentIndex(), idx0);
}
TEST(Scheduler, PartialDwellDoesNotAdvance)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    s.buildSchedule(cfg, {});
    int idx0 = s.currentIndex();
    s.advance(0.0001);
    EXPECT_EQ(s.currentIndex(), idx0);
}
TEST(Scheduler, ZeroDtDoesNotAdvance)
{
    RadarScheduler s;
    s.buildSchedule(makeCfg(), {});
    int idx0 = s.currentIndex();
    s.advance(0.0);
    EXPECT_EQ(s.currentIndex(), idx0);
}
TEST(Scheduler, IndexWrapsAroundSchedule)
{
    RadarScheduler s;
    RadarConfig cfg = makeCfg();
    cfg.minAzimuth = -3.0f; cfg.maxAzimuth = 3.0f;
    cfg.minElevation = 0.0f; cfg.maxElevation = 3.0f;
    s.buildSchedule(cfg, {});
    int size = s.scheduleSize();
    double dwell = static_cast<double>(cfg.searchDwellTime_ms)/1000.0 + 0.001;
    // advance through entire schedule twice — should not crash
    for (int i = 0; i < size*2 + 1; ++i) s.advance(dwell);
    EXPECT_GE(s.currentIndex(), 0);
    EXPECT_LT(s.currentIndex(), size);
}
