#include <gtest/gtest.h>
#include <cmath>
#include "radartracker_aesa.h"

using namespace aesa;

static RadarConfig makeCfg(int minHits = 2, int dropMisses = 3)
{
    RadarConfig cfg;
    cfg.minHitsToValidate    = minHits;
    cfg.missedScansToDrop    = dropMisses;
    cfg.trackCoastSeconds    = 10.0;
    cfg.manoeuvreThreshold_m = 500.0;
    cfg.maxTrackSpeed        = 2000.0;
    cfg.noise.rangeStdDev    = 30.0;
    cfg.beamWidth            = 3.0f;
    cfg.useJPDA              = false;
    return cfg;
}

static TargetInput makeTgt(uint32_t id, double x, double y, double z,
                            double vx = -100.0)
{
    TargetInput t;
    t.id = id; t.x = x; t.y = y; t.z = z;
    t.vx = vx; t.vy = 0.0; t.vz = 0.0;
    t.surface = SurfaceType::AIR;
    t.swerlingCase = SwerlingCase::CASE_0;
    return t;
}

static DetectionOutput detFrom(const TargetInput& t)
{
    double r  = std::sqrt(t.x*t.x + t.y*t.y + t.z*t.z);
    double az = std::atan2(t.y, t.x) * (180.0/M_PI);
    double el = std::asin(std::clamp(t.z/std::max(r,1.0),-1.0,1.0)) * (180.0/M_PI);
    DetectionOutput d;
    d.targetID = t.id; d.range = r; d.azimuth = az; d.elevation = el;
    d.radialVelocity = -100.0; d.isAmbiguous = false; d.isDRFMGhost = false;
    d.snr = 20.0;
    return d;
}

// ── lifecycle ─────────────────────────────────────────────────────────────────
TEST(Tracker, MaxTracksCapEnforced)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(1, 100);
    // Insert more than MAX_TRACKS (2000)
    for (uint32_t i = 1; i <= 2100; ++i) {
        TargetInput t = makeTgt(i, 50000+i, 0, 5000);
        tr.createNewTrack(detFrom(t), t, 500000.0, 0.0, cfg);
    }
    EXPECT_LE(tr.database().size(), 2000u);
}

TEST(Tracker, PredictStableAfter1000Calls)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000, -100.0);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    for (int i = 0; i < 1000; ++i)
        tr.predict(0.05);
    // Position should still be finite
    EXPECT_TRUE(std::isfinite(tr.database()[0].x));
    EXPECT_TRUE(std::isfinite(tr.database()[0].predictedRange));
}
TEST(Tracker, ClearEmptiesDatabase)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    tr.clear();
    EXPECT_EQ(tr.database().size(), 0u);
}
TEST(Tracker, ClearOnEmptyDatabaseNoOp)
{
    RadarTracker_AESA tr;
    tr.clear();
    EXPECT_EQ(tr.database().size(), 0u);
}
TEST(Tracker, DoubleClearIsIdempotent)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    tr.clear();
    tr.clear();
    EXPECT_EQ(tr.database().size(), 0u);
}

// ── track creation ────────────────────────────────────────────────────────────
TEST(Tracker, CreateNewTrackAddsEntry)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    EXPECT_EQ(tr.database().size(), 1u);
    EXPECT_EQ(tr.database()[0].id, 1u);
}
TEST(Tracker, DuplicateIDNotInserted)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    auto cfg = makeCfg();
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, cfg);
    tr.createNewTrack(detFrom(t), t, 150000.0, 1.0, cfg);
    EXPECT_EQ(tr.database().size(), 1u);
}
TEST(Tracker, NewTrackNotValidatedWithMinHits3)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg(3));
    EXPECT_FALSE(tr.database()[0].isValidated);
}
TEST(Tracker, HitCountInitialisedToOne)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    EXPECT_EQ(tr.database()[0].hitCount, 1);
}
TEST(Tracker, NewTrackPositionMatchesDetection)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    auto d = detFrom(t);
    tr.createNewTrack(d, t, 150000.0, 0.0, makeCfg());
    EXPECT_NEAR(tr.database()[0].range, d.range, 10.0);
}
TEST(Tracker, MultipleDistinctTracksCreated)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg();
    TargetInput t1 = makeTgt(1, 50000, 0, 5000);
    TargetInput t2 = makeTgt(2, 60000, 10000, 5000);
    TargetInput t3 = makeTgt(3, 70000, 5000, 5000);
    tr.createNewTrack(detFrom(t1), t1, 150000.0, 0.0, cfg);
    tr.createNewTrack(detFrom(t2), t2, 150000.0, 0.0, cfg);
    tr.createNewTrack(detFrom(t3), t3, 150000.0, 0.0, cfg);
    EXPECT_EQ(tr.database().size(), 3u);
}

// ── prediction ───────────────────────────────────────────────────────────────
TEST(Tracker, PredictMovesPositionWithVelocity)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000, -100.0);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    double x0 = tr.database()[0].x;
    tr.predict(1.0);
    EXPECT_NEAR(tr.database()[0].x - x0, -100.0, 15.0);
}
TEST(Tracker, PredictUpdatesPredictedRange)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000, -100.0);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    double r0 = tr.database()[0].predictedRange;
    tr.predict(1.0);
    EXPECT_NE(tr.database()[0].predictedRange, r0);
    EXPECT_GT(tr.database()[0].predictedRange, 0.0);
}
TEST(Tracker, PredictClearsIsUpdatedFlag)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    tr.predict(0.1);
    EXPECT_FALSE(tr.database()[0].isUpdated);
}
TEST(Tracker, PredictZeroDtNoPositionChange)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000, -100.0);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    double x0 = tr.database()[0].x;
    tr.predict(0.0);
    EXPECT_NEAR(tr.database()[0].x, x0, 1e-6);
}
TEST(Tracker, PredictMultipleTracksAllUpdated)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg();
    tr.createNewTrack(detFrom(makeTgt(1, 50000, 0, 5000)), makeTgt(1, 50000, 0, 5000), 150000.0, 0.0, cfg);
    tr.createNewTrack(detFrom(makeTgt(2, 60000, 0, 5000)), makeTgt(2, 60000, 0, 5000), 150000.0, 0.0, cfg);
    tr.predict(0.1);
    for (const auto& tf : tr.database())
        EXPECT_FALSE(tf.isUpdated);
}

// ── association ───────────────────────────────────────────────────────────────
TEST(Tracker, FindBestMatchReturnsNearbyTrack)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    DetectionOutput d = detFrom(t);
    tr.createNewTrack(d, t, 150000.0, 0.0, makeCfg());
    double prob = 0.0;
    TrackFile* m = tr.findBestTrackMatch(d, 150000.0, prob);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->id, 1u);
    EXPECT_GT(prob, 0.0);
}
TEST(Tracker, FindBestMatchNullForFarDetection)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg());
    DetectionOutput dFar;
    dFar.targetID = 99; dFar.range = 300000.0; dFar.azimuth = 45.0; dFar.elevation = 10.0;
    double prob = 0.0;
    EXPECT_EQ(tr.findBestTrackMatch(dFar, 150000.0, prob), nullptr);
}
TEST(Tracker, FindBestMatchNullOnEmptyDatabase)
{
    RadarTracker_AESA tr;
    DetectionOutput d = detFrom(makeTgt(1, 50000, 0, 5000));
    double prob = 0.0;
    EXPECT_EQ(tr.findBestTrackMatch(d, 150000.0, prob), nullptr);
}
TEST(Tracker, FindBestMatchSkipsAlreadyUpdated)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    DetectionOutput d = detFrom(t);
    auto cfg = makeCfg();
    tr.createNewTrack(d, t, 150000.0, 0.0, cfg);
    double prob = 0.0;
    TrackFile* m = tr.findBestTrackMatch(d, 150000.0, prob);
    ASSERT_NE(m, nullptr);
    tr.performKalmanUpdate(*m, d, 1.0, 0.05, 150000.0, cfg);
    // After update isUpdated=true → should not match again
    TrackFile* m2 = tr.findBestTrackMatch(d, 150000.0, prob);
    EXPECT_EQ(m2, nullptr);
}
TEST(Tracker, FindBestMatchPicksClosestTrack)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg();
    TargetInput t1 = makeTgt(1, 50000, 0, 5000);
    TargetInput t2 = makeTgt(2, 90000, 0, 5000);
    tr.createNewTrack(detFrom(t1), t1, 300000.0, 0.0, cfg);
    tr.createNewTrack(detFrom(t2), t2, 300000.0, 0.0, cfg);
    // Detection close to t1
    DetectionOutput d = detFrom(t1);
    double prob = 0.0;
    TrackFile* m = tr.findBestTrackMatch(d, 300000.0, prob);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->id, 1u);
}

// ── Kalman update ─────────────────────────────────────────────────────────────
TEST(Tracker, KalmanUpdateIncrementsHitCount)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(3);
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    DetectionOutput d = detFrom(t);
    tr.createNewTrack(d, t, 150000.0, 0.0, cfg);
    int hc0 = tr.database()[0].hitCount;
    double prob = 0.0;
    TrackFile* m = tr.findBestTrackMatch(d, 150000.0, prob);
    ASSERT_NE(m, nullptr);
    tr.performKalmanUpdate(*m, d, 1.0, 0.05, 150000.0, cfg);
    EXPECT_EQ(tr.database()[0].hitCount, hc0+1);
}
TEST(Tracker, KalmanUpdateValidatesAfterMinHits)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(2);
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    DetectionOutput d = detFrom(t);
    tr.createNewTrack(d, t, 150000.0, 0.0, cfg);
    EXPECT_FALSE(tr.database()[0].isValidated);
    double prob = 0.0;
    TrackFile* m = tr.findBestTrackMatch(d, 150000.0, prob);
    ASSERT_NE(m, nullptr);
    tr.performKalmanUpdate(*m, d, 1.0, 0.05, 150000.0, cfg);
    EXPECT_TRUE(tr.database()[0].isValidated);
}
TEST(Tracker, KalmanUpdateSetsIsUpdated)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg();
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    DetectionOutput d = detFrom(t);
    tr.createNewTrack(d, t, 150000.0, 0.0, cfg);
    tr.predict(0.05);
    double prob = 0.0;
    TrackFile* m = tr.findBestTrackMatch(d, 150000.0, prob);
    ASSERT_NE(m, nullptr);
    tr.performKalmanUpdate(*m, d, 1.0, 0.05, 150000.0, cfg);
    EXPECT_TRUE(tr.database()[0].isUpdated);
}
TEST(Tracker, KalmanUpdateResetsMissCount)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(1, 5);
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    DetectionOutput d = detFrom(t);
    tr.createNewTrack(d, t, 150000.0, 0.0, cfg);
    // Simulate a miss
    tr.applyScanMissLogic(1.0, cfg);
    tr.applyScanMissLogic(2.0, cfg);
    EXPECT_GT(tr.database()[0].missCount, 0);
    // Now update
    tr.predict(0.05);
    double prob = 0.0;
    TrackFile* m = tr.findBestTrackMatch(d, 150000.0, prob);
    ASSERT_NE(m, nullptr);
    tr.performKalmanUpdate(*m, d, 3.0, 0.05, 150000.0, cfg);
    EXPECT_EQ(tr.database()[0].missCount, 0);
}

// ── scan miss ─────────────────────────────────────────────────────────────────
TEST(Tracker, ScanMissCountIncrementsWhenNotUpdated)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(1, 5);
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, cfg);
    tr.applyScanMissLogic(1.0, cfg); // clears creation flag
    tr.applyScanMissLogic(2.0, cfg); // now increments
    EXPECT_EQ(tr.database()[0].scanMissCount, 1);
}
TEST(Tracker, TrackDroppedAfterMissLimit)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(1, 2);
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, cfg);
    for (int i = 1; i <= 5; ++i)
        tr.applyScanMissLogic(static_cast<double>(i), cfg);
    EXPECT_EQ(tr.database().size(), 0u);
}
TEST(Tracker, TrackKeptWhenUpdatedThisScan)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(1, 2);
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    DetectionOutput d = detFrom(t);
    tr.createNewTrack(d, t, 150000.0, 0.0, cfg);
    double prob = 0.0;
    TrackFile* m = tr.findBestTrackMatch(d, 150000.0, prob);
    if (m) tr.performKalmanUpdate(*m, d, 1.0, 0.05, 150000.0, cfg);
    tr.applyScanMissLogic(1.0, cfg);
    EXPECT_EQ(tr.database().size(), 1u);
    EXPECT_EQ(tr.database()[0].scanMissCount, 0);
}
TEST(Tracker, TrackDroppedAfterCoastTimeout)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(1, 100); // high miss limit so coast triggers first
    cfg.trackCoastSeconds = 2.0;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, cfg);
    tr.applyScanMissLogic(100.0, cfg); // far in future → coast expired
    EXPECT_EQ(tr.database().size(), 0u);
}

// ── output ────────────────────────────────────────────────────────────────────
TEST(Tracker, GetValidatedTracksFiltersUnvalidated)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(3);
    TargetInput t1 = makeTgt(1, 50000, 0, 5000);
    TargetInput t2 = makeTgt(2, 60000, 10000, 5000);
    tr.createNewTrack(detFrom(t1), t1, 150000.0, 0.0, cfg);
    tr.createNewTrack(detFrom(t2), t2, 150000.0, 0.0, cfg);
    DetectionOutput d1 = detFrom(t1);
    for (int i = 0; i < 3; ++i) {
        tr.predict(0.05);
        double prob = 0.0;
        TrackFile* m = tr.findBestTrackMatch(d1, 150000.0, prob);
        if (m) tr.performKalmanUpdate(*m, d1, static_cast<double>(i)*0.05, 0.05, 150000.0, cfg);
    }
    std::vector<TrackOutput> out;
    tr.getValidatedTracks(out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].id, 1u);
}
TEST(Tracker, TrackOutputRangePositive)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg(1));
    EXPECT_GT(tr.buildTrackOutput(tr.database()[0]).range, 0.0);
}
TEST(Tracker, TrackOutputAzimuthReasonable)
{
    RadarTracker_AESA tr;
    TargetInput t = makeTgt(1, 50000, 10000, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, makeCfg(1));
    auto out = tr.buildTrackOutput(tr.database()[0]);
    EXPECT_GE(out.azimuth, -180.0);
    EXPECT_LE(out.azimuth,  180.0);
}
TEST(Tracker, GetValidatedTracksEmptyWhenNoneValidated)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg(5);
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, cfg);
    std::vector<TrackOutput> out;
    tr.getValidatedTracks(out);
    EXPECT_TRUE(out.empty());
}

// ── external track injection ──────────────────────────────────────────────────
TEST(Tracker, InjectExternalTrackValidatedAndFlagged)
{
    RadarTracker_AESA tr;
    TrackOutput ext;
    ext.id = 42; ext.x = 30000; ext.y = 0; ext.z = 5000;
    ext.vx = -100; ext.vy = 0; ext.vz = 0;
    ext.range = std::sqrt(30000.0*30000.0 + 5000.0*5000.0);
    ext.isValidated = true;
    tr.injectExternalTrack(ext, 0.0, makeCfg());
    ASSERT_EQ(tr.database().size(), 1u);
    EXPECT_TRUE(tr.database()[0].isValidated);
    EXPECT_TRUE(tr.database()[0].isExternalTrack);
}
TEST(Tracker, InjectExternalTrackDuplicateIgnored)
{
    RadarTracker_AESA tr;
    TrackOutput ext; ext.id = 42; ext.range = 30000.0;
    auto cfg = makeCfg();
    tr.injectExternalTrack(ext, 0.0, cfg);
    tr.injectExternalTrack(ext, 1.0, cfg);
    EXPECT_EQ(tr.database().size(), 1u);
}
TEST(Tracker, InjectMultipleExternalTracks)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg();
    for (uint32_t i = 1; i <= 5; ++i) {
        TrackOutput ext; ext.id = i; ext.range = 50000.0 + i*1000.0;
        tr.injectExternalTrack(ext, 0.0, cfg);
    }
    EXPECT_EQ(tr.database().size(), 5u);
}

// ── JPDA ─────────────────────────────────────────────────────────────────────
TEST(Tracker, JPDAUpdateSingleDetectionIncrementsHit)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg();
    cfg.useJPDA = true; cfg.jpdaFalseAlarmDensity = 1e-6f;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    DetectionOutput d = detFrom(t);
    tr.createNewTrack(d, t, 150000.0, 0.0, cfg);
    tr.predict(0.05);
    tr.performJPDAUpdate({d}, 1.0, 150000.0, cfg);
    EXPECT_EQ(tr.database()[0].hitCount, 2);
}
TEST(Tracker, JPDAUpdateEmptyDetectionsNoChange)
{
    RadarTracker_AESA tr;
    auto cfg = makeCfg();
    cfg.useJPDA = true;
    TargetInput t = makeTgt(1, 50000, 0, 5000);
    tr.createNewTrack(detFrom(t), t, 150000.0, 0.0, cfg);
    int hc0 = tr.database()[0].hitCount;
    tr.predict(0.05);
    tr.performJPDAUpdate({}, 1.0, 150000.0, cfg);
    EXPECT_EQ(tr.database()[0].hitCount, hc0);
}
