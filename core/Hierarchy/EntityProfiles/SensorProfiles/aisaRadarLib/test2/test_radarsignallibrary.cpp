#include <gtest/gtest.h>
#include "radarsignallibrary_aesa.h"

using namespace aesa;

static RadarConfig makeCfg(bool agile = false)
{
    RadarConfig cfg;
    cfg.frequency_Hz      = 8.0e9;
    cfg.frequencyAgility  = agile;
    cfg.hopStartFrequency = agile ? 7.0e9f : 0.0f;
    cfg.hopStopFrequency  = agile ? 9.0e9f : 0.0f;
    return cfg;
}

static BeamWaveform makeWF(float prf = 300.0f, ModulationType mod = ModulationType::LFM)
{
    BeamWaveform wf;
    wf.modulation     = mod;
    wf.pulseWidth_s   = 50e-6f;
    wf.prf_Hz         = prf;
    wf.bandwidth_Hz   = 5e6f;
    wf.pulsesPerDwell = 10;
    return wf;
}

// ── accumulate / basic ────────────────────────────────────────────────────────
TEST(SignalLib, AccumulateCreatesIntercept)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 1e-10, 0.0, makeCfg(), makeWF(), false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].targetID, 1u);
}
TEST(SignalLib, AccumulateMultipleTargets)
{
    RadarSignalLibrary_AESA lib;
    auto cfg = makeCfg(); auto wf = makeWF();
    lib.accumulate(1, 1e-10, 0.0, cfg, wf, false);
    lib.accumulate(2, 1e-10, 0.0, cfg, wf, false);
    lib.accumulate(3, 1e-10, 0.0, cfg, wf, false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out.size(), 3u);
}
TEST(SignalLib, DRFMGhostNotAccumulated)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 1e-10, 0.0, makeCfg(), makeWF(), true);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out.size(), 0u);
}
TEST(SignalLib, DRFMGhostDoesNotPolluteLegitEntry)
{
    RadarSignalLibrary_AESA lib;
    auto cfg = makeCfg(); auto wf = makeWF();
    lib.accumulate(1, 1e-10, 0.0, cfg, wf, false); // real
    lib.accumulate(1, 1e-10, 0.1, cfg, wf, true);  // ghost — should not add
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].signalDepth, 1); // only one real accumulation
}
TEST(SignalLib, ZeroPowerAccumulatedWithoutCrash)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 0.0, 0.0, makeCfg(), makeWF(), false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out.size(), 1u);
}

// ── running averages ──────────────────────────────────────────────────────────
TEST(SignalLib, SignalDepthIncrementsWithEachCall)
{
    RadarSignalLibrary_AESA lib;
    auto cfg = makeCfg(); auto wf = makeWF();
    for (int i = 0; i < 5; ++i)
        lib.accumulate(1, 1e-10, static_cast<double>(i)*0.1, cfg, wf, false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out[0].signalDepth, 5);
}
TEST(SignalLib, PRICountIncrementsPerCall)
{
    RadarSignalLibrary_AESA lib;
    auto cfg = makeCfg(); auto wf = makeWF();
    lib.accumulate(1, 1e-10, 0.0, cfg, wf, false);
    lib.accumulate(1, 1e-10, 0.1, cfg, wf, false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out[0].priCount, 2);
}
TEST(SignalLib, AccumulatedPRIMatchesWaveform)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 1e-10, 0.0, makeCfg(), makeWF(500.0f), false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_NEAR(out[0].pri_s, 1.0/500.0, 1e-9);
}
TEST(SignalLib, AccumulatedFrequencyMatchesConfig)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 1e-10, 0.0, makeCfg(), makeWF(), false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_NEAR(out[0].frequency_Hz, 8.0e9, 1.0e6);
}
TEST(SignalLib, FrequencyAgilityUsesHopMidpoint)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 1e-10, 0.0, makeCfg(true), makeWF(), false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_NEAR(out[0].frequency_Hz, 8.0e9, 1.0e8);
}
TEST(SignalLib, ModulationTypeStored)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 1e-10, 0.0, makeCfg(), makeWF(300.0f, ModulationType::NLFM), false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out[0].modulation, ModulationType::NLFM);
}

// ── pruning ───────────────────────────────────────────────────────────────────
TEST(SignalLib, PruneRemovesExpiredEntry)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 1e-10, 0.0, makeCfg(), makeWF(), false);
    lib.pruneStale(20.0, 10.0);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out.size(), 0u);
}
TEST(SignalLib, PruneKeepsRecentEntry)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 1e-10, 15.0, makeCfg(), makeWF(), false);
    lib.pruneStale(20.0, 10.0);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out.size(), 1u);
}
TEST(SignalLib, PruneSelectivelyRemovesOld)
{
    RadarSignalLibrary_AESA lib;
    auto cfg = makeCfg(); auto wf = makeWF();
    lib.accumulate(1, 1e-10,  0.0, cfg, wf, false);  // old
    lib.accumulate(2, 1e-10, 15.0, cfg, wf, false);  // recent
    lib.pruneStale(20.0, 10.0);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].targetID, 2u);
}
TEST(SignalLib, PruneOnEmptyLibraryNoOp)
{
    RadarSignalLibrary_AESA lib;
    lib.pruneStale(100.0, 10.0); // should not crash
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out.size(), 0u);
}
TEST(SignalLib, PruneZeroCoastRemovesAll)
{
    RadarSignalLibrary_AESA lib;
    auto cfg = makeCfg(); auto wf = makeWF();
    lib.accumulate(1, 1e-10, 0.0, cfg, wf, false);
    lib.accumulate(2, 1e-10, 0.0, cfg, wf, false);
    lib.pruneStale(1.0, 0.0); // coast=0 → everything expired
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out.size(), 0u);
}

// ── library matching ──────────────────────────────────────────────────────────
TEST(SignalLib, MatchesEmitterByFrequency)
{
    RadarSignalLibrary_AESA lib;
    SignalLibraryEntry entry;
    entry.emitterID        = "APG-68";
    entry.frequency_Hz     = 8.0e9;
    entry.freqTolerance_Hz = 500e6;
    entry.modulation       = ModulationType::NONE;
    lib.loadLibrary({entry});
    auto cfg = makeCfg(); auto wf = makeWF();
    for (int i = 0; i < 5; ++i)
        lib.accumulate(1, 1e-10, static_cast<double>(i)*0.1, cfg, wf, false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].emitterID, "APG-68");
}
TEST(SignalLib, NoMatchForWrongFrequency)
{
    RadarSignalLibrary_AESA lib;
    SignalLibraryEntry entry;
    entry.emitterID        = "WRONG";
    entry.frequency_Hz     = 3.0e9;
    entry.freqTolerance_Hz = 100e6;
    lib.loadLibrary({entry});
    auto cfg = makeCfg(); auto wf = makeWF();
    for (int i = 0; i < 5; ++i)
        lib.accumulate(1, 1e-10, static_cast<double>(i)*0.1, cfg, wf, false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_NE(out[0].emitterID, "WRONG");
}
TEST(SignalLib, EmptyLibraryNoMatch)
{
    RadarSignalLibrary_AESA lib;
    lib.loadLibrary({});
    auto cfg = makeCfg(); auto wf = makeWF();
    for (int i = 0; i < 5; ++i)
        lib.accumulate(1, 1e-10, static_cast<double>(i)*0.1, cfg, wf, false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_TRUE(out[0].emitterID.empty()); // no match
}
TEST(SignalLib, MultipleLibraryEntriesFirstMatchWins)
{
    RadarSignalLibrary_AESA lib;
    SignalLibraryEntry e1, e2;
    e1.emitterID = "FIRST"; e1.frequency_Hz = 8.0e9; e1.freqTolerance_Hz = 1.0e9;
    e2.emitterID = "SECOND"; e2.frequency_Hz = 8.0e9; e2.freqTolerance_Hz = 1.0e9;
    lib.loadLibrary({e1, e2});
    auto cfg = makeCfg(); auto wf = makeWF();
    for (int i = 0; i < 5; ++i)
        lib.accumulate(1, 1e-10, static_cast<double>(i)*0.1, cfg, wf, false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out[0].emitterID, "FIRST");
}

// ── clear ─────────────────────────────────────────────────────────────────────
TEST(SignalLib, ClearRemovesAllEntries)
{
    RadarSignalLibrary_AESA lib;
    auto cfg = makeCfg(); auto wf = makeWF();
    lib.accumulate(1, 1e-10, 0.0, cfg, wf, false);
    lib.accumulate(2, 1e-10, 0.0, cfg, wf, false);
    lib.clear();
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out.size(), 0u);
}
TEST(SignalLib, ClearThenAccumulateWorks)
{
    RadarSignalLibrary_AESA lib;
    auto cfg = makeCfg(); auto wf = makeWF();
    lib.accumulate(1, 1e-10, 0.0, cfg, wf, false);
    lib.clear();
    lib.accumulate(2, 1e-10, 0.0, cfg, wf, false);
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].targetID, 2u);
}
TEST(SignalLib, DoubleClearIsIdempotent)
{
    RadarSignalLibrary_AESA lib;
    lib.accumulate(1, 1e-10, 0.0, makeCfg(), makeWF(), false);
    lib.clear();
    lib.clear();
    std::vector<SignalIntercept> out;
    lib.getIntercepts(out);
    EXPECT_EQ(out.size(), 0u);
}
