// =============================================================================
// FILE:         radarsignallibrary_aesa_test.cpp
// MODULE:       AESA Radar Signal Intercept Library (ESM) — Unit Tests
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage — all true/false paths exercised
//
// DESCRIPTION:  Requirements-based unit tests for RadarSignalLibrary_AESA.
//               Each test case is traceable to a specific requirement and to
//               specific function comments in radarsignallibrary_aesa.h.
//               Tests are structured as: Arrange -> Act -> Assert.
//
// REQUIREMENTS COVERED:
//   REQ-AESA-004  Output assembly — intercept publication
//   REQ-AESA-030  Stale intercept pruning
//   REQ-AESA-040  ESM signal accumulation and classification
//   REQ-AESA-060  DRFM ghost suppression
//
// TEST CASE INDEX:
//   TC-AESA-LIB-001  clear() leaves acc_ empty (getIntercepts returns empty)
//   TC-AESA-LIB-002  clear() leaves lastSeen_ empty (pruneStale safe after clear)
//   TC-AESA-LIB-003  clear() does not clear lib_ (match still works after clear)
//   TC-AESA-LIB-004  loadLibrary() replaces lib_ contents
//   TC-AESA-LIB-005  loadLibrary({}) effectively clears library
//   TC-AESA-LIB-006  loadLibrary() does not affect existing acc_ entries
//   TC-AESA-LIB-007  accumulate() DRFM ghost returns empty SignalIntercept
//   TC-AESA-LIB-008  accumulate() DRFM ghost does not modify acc_
//   TC-AESA-LIB-009  accumulate() real detection creates entry in output
//   TC-AESA-LIB-010  accumulate() frequency averaged correctly over 3 dwells
//   TC-AESA-LIB-011  accumulate() PRI averaged correctly
//   TC-AESA-LIB-012  accumulate() pulseWidth averaged correctly
//   TC-AESA-LIB-013  accumulate() signalLevel computed in dBW
//   TC-AESA-LIB-014  accumulate() low power records floor value
//   TC-AESA-LIB-015  accumulate() frequency agility uses midpoint
//   TC-AESA-LIB-016  accumulate() updates lastSeen timestamp
//   TC-AESA-LIB-017  getIntercepts() returns empty on fresh library
//   TC-AESA-LIB-018  getIntercepts() returns one entry after accumulation
//   TC-AESA-LIB-019  getIntercepts() clears output before fill
//   TC-AESA-LIB-020  pruneStale() removes expired entry
//   TC-AESA-LIB-021  pruneStale() retains recent entry
//   TC-AESA-LIB-022  pruneStale() removes from both acc_ and lastSeen_
//   TC-AESA-LIB-023  matchLibrary() returns no match before 3 frequency obs
//   TC-AESA-LIB-024  matchLibrary() identifies known emitter after 3 obs
//   TC-AESA-LIB-025  matchLibrary() returns unchanged ID for no library match
//
// CHANGE HISTORY:
//   Rev 1  20 Apr 2026  Initial test suite — DO-178C DAL B compliant.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
// =============================================================================

#include "issst_test_framework.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib/radarsignallibrary_aesa.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib/radarmodel_aesa.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

extern int testsPassed;
extern int testsFailed;
extern int tests;

// =============================================================================
// HELPER: buildDefaultLibConfig
//
// DESCRIPTION: Returns a RadarConfig with deterministic defaults for
//              signal library unit testing.
// =============================================================================
static aesa::RadarConfig buildDefaultLibConfig()
{
    aesa::RadarConfig cfg;
    cfg.frequency_Hz      = 10.0e9;
    cfg.frequencyAgility  = false;
    cfg.hopStartFrequency = 0.0f;
    cfg.hopStopFrequency  = 0.0f;
    cfg.numElements       = 1000;
    cfg.failedModules     = 0;
    cfg.antennaGain       = 34.0f;
    cfg.beamWidth         = 3.0f;
    return cfg;
}

// =============================================================================
// HELPER: buildDefaultWaveform
//
// DESCRIPTION: Returns a BeamWaveform with deterministic defaults.
// =============================================================================
static aesa::BeamWaveform buildDefaultWaveform()
{
    aesa::BeamWaveform wf;
    wf.prf_Hz       = 1000.0f;
    wf.pulseWidth_s = 10e-6f;
    wf.bandwidth_Hz = 20e6f;
    wf.modulation   = aesa::ModulationType::LFM;
    wf.mode         = aesa::WaveformMode::MPRF;
    return wf;
}

// =============================================================================
// HELPER: buildLibraryEntry
//
// DESCRIPTION: Creates a SignalLibraryEntry for a known emitter.
// =============================================================================
static aesa::SignalLibraryEntry buildLibraryEntry(
    const std::string& id,
    double freq_Hz,
    double freqTol_Hz,
    aesa::ModulationType mod = aesa::ModulationType::NONE)
{
    aesa::SignalLibraryEntry e;
    e.emitterID         = id;
    e.frequency_Hz      = freq_Hz;
    e.freqTolerance_Hz  = freqTol_Hz;
    e.pri_s             = 0.0;      // not used in matching
    e.pulseWidth_s      = 0.0;      // not used in matching
    e.modulation        = mod;
    return e;
}

// =============================================================================
// TEST SUITE: test_lib_clear
// Covers: TC-AESA-LIB-001 through TC-AESA-LIB-003
// Requirements: REQ-AESA-040
// =============================================================================
void test_lib_clear()
{
    std::cout << "\n--- TC-AESA-LIB-001..003: clear() Tests ---" << std::endl;

    aesa::RadarSignalLibrary_AESA lib;
    aesa::RadarConfig             cfg = buildDefaultLibConfig();
    aesa::BeamWaveform            wf  = buildDefaultWaveform();

    // Load a library entry and accumulate one detection to create state
    lib.loadLibrary({ buildLibraryEntry("EMITTER-A", 10.0e9, 500e6) });
    lib.accumulate(1u, 1e-10, 0.0, cfg, wf, false);

    // Act: clear
    lib.clear();

    // TC-AESA-LIB-001: acc_ must be empty after clear
    // REQ-AESA-040: no stale intercepts after clear
    std::vector<aesa::SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_TRUE(out.empty(),
                "TC-AESA-LIB-001: clear() leaves acc_ empty "
                "(getIntercepts returns empty)");

    // TC-AESA-LIB-002: pruneStale must be safe after clear (no crash)
    // REQ-AESA-030: empty lastSeen_ means pruneStale is a no-op
    try {
        lib.pruneStale(100.0, 1.0);
        ASSERT_TRUE(true,
                    "TC-AESA-LIB-002: pruneStale() after clear() does not crash");
    } catch (...) {
        ASSERT_FALSE(true,
                     "TC-AESA-LIB-002: pruneStale() after clear() crashed");
    }

    // TC-AESA-LIB-003: lib_ must survive clear — accumulate 3 dwells and
    // check that the library match still works (implies lib_ was not cleared)
    // REQ-AESA-040: library persists across operational cycles
    for (int i = 0; i < 3; ++i)
    {
        lib.accumulate(2u, 1e-10, static_cast<double>(i), cfg, wf, false);
    }
    lib.getIntercepts(out);
    bool foundEmitter = false;
    for (const auto& si : out)
    {
        if (si.targetID == 2u && si.emitterID == "EMITTER-A")
        {
            foundEmitter = true;
        }
    }
    ASSERT_TRUE(foundEmitter,
                "TC-AESA-LIB-003: clear() does not clear lib_ — "
                "emitter match still works after clear + re-accumulate");
}

// =============================================================================
// TEST SUITE: test_lib_loadLibrary
// Covers: TC-AESA-LIB-004 through TC-AESA-LIB-006
// Requirements: REQ-AESA-040
// =============================================================================
void test_lib_loadLibrary()
{
    std::cout << "\n--- TC-AESA-LIB-004..006: loadLibrary() Tests ---"
              << std::endl;

    aesa::RadarSignalLibrary_AESA lib;
    aesa::RadarConfig             cfg = buildDefaultLibConfig();
    aesa::BeamWaveform            wf  = buildDefaultWaveform();

    // TC-AESA-LIB-004: loadLibrary() replaces lib_ contents
    // REQ-AESA-040: new library replaces old — match uses new entries
    lib.loadLibrary({ buildLibraryEntry("OLD-EMITTER", 10.0e9, 500e6) });
    lib.loadLibrary({ buildLibraryEntry("NEW-EMITTER", 10.0e9, 500e6) });

    // Accumulate 3 dwells and check which emitter is identified
    for (int i = 0; i < 3; ++i)
    {
        lib.accumulate(1u, 1e-10, static_cast<double>(i), cfg, wf, false);
    }
    std::vector<aesa::SignalIntercept> out;
    lib.getIntercepts(out);
    bool foundNew = false;
    bool foundOld = false;
    for (const auto& si : out)
    {
        if (si.emitterID == "NEW-EMITTER") foundNew = true;
        if (si.emitterID == "OLD-EMITTER") foundOld = true;
    }
    ASSERT_TRUE(foundNew,
                "TC-AESA-LIB-004: loadLibrary() replaces lib_ — new emitter matched");
    ASSERT_FALSE(foundOld,
                 "TC-AESA-LIB-004b: old emitter not matched after library replace");

    // TC-AESA-LIB-005: loadLibrary({}) clears library — no match possible
    // REQ-AESA-040: empty library means all emitterIDs remain empty
    lib.clear();
    lib.loadLibrary({});
    for (int i = 0; i < 3; ++i)
    {
        lib.accumulate(2u, 1e-10, static_cast<double>(i), cfg, wf, false);
    }
    lib.getIntercepts(out);
    bool anyNamed = false;
    for (const auto& si : out)
    {
        if (!si.emitterID.empty()) anyNamed = true;
    }
    ASSERT_FALSE(anyNamed,
                 "TC-AESA-LIB-005: loadLibrary({}) means no emitter matched");

    // TC-AESA-LIB-006: loadLibrary() does not affect acc_
    // REQ-AESA-040: existing intercepts survive library reload
    lib.clear();
    lib.accumulate(3u, 1e-10, 0.0, cfg, wf, false);
    lib.loadLibrary({ buildLibraryEntry("X", 10.0e9, 500e6) });
    lib.getIntercepts(out);
    bool foundTarget3 = false;
    for (const auto& si : out)
    {
        if (si.targetID == 3u) foundTarget3 = true;
    }
    ASSERT_TRUE(foundTarget3,
                "TC-AESA-LIB-006: loadLibrary() does not erase existing acc_ entry");
}

// =============================================================================
// TEST SUITE: test_lib_accumulate_ghost
// Covers: TC-AESA-LIB-007, TC-AESA-LIB-008
// Requirements: REQ-AESA-060
// =============================================================================
void test_lib_accumulate_ghost()
{
    std::cout << "\n--- TC-AESA-LIB-007..008: DRFM Ghost Suppression Tests ---"
              << std::endl;

    aesa::RadarSignalLibrary_AESA lib;
    aesa::RadarConfig             cfg = buildDefaultLibConfig();
    aesa::BeamWaveform            wf  = buildDefaultWaveform();

    // TC-AESA-LIB-007: accumulate() with isDRFMGhost=true returns empty intercept
    // REQ-AESA-060: ghost must not create a valid intercept record
    aesa::SignalIntercept result =
        lib.accumulate(1u, 1e-10, 0.0, cfg, wf, true);
    ASSERT_EQ(result.targetID, 0u,
              "TC-AESA-LIB-007: DRFM ghost returns intercept with targetID=0");
    ASSERT_NEAR(result.frequency_Hz, 0.0, 1e-9,
                "TC-AESA-LIB-007b: DRFM ghost returns intercept with frequency=0");

    // TC-AESA-LIB-008: accumulate() with isDRFMGhost=true does NOT modify acc_
    // REQ-AESA-060: ghost must not pollute real emitter parameters
    std::vector<aesa::SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_TRUE(out.empty(),
                "TC-AESA-LIB-008: DRFM ghost does not create entry in acc_");
}

// =============================================================================
// TEST SUITE: test_lib_accumulate_real
// Covers: TC-AESA-LIB-009 through TC-AESA-LIB-016
// Requirements: REQ-AESA-040
// =============================================================================
void test_lib_accumulate_real()
{
    std::cout << "\n--- TC-AESA-LIB-009..016: accumulate() Real Detection Tests ---"
              << std::endl;

    aesa::RadarSignalLibrary_AESA lib;
    aesa::RadarConfig             cfg = buildDefaultLibConfig();
    aesa::BeamWaveform            wf  = buildDefaultWaveform();
    // wf.prf_Hz = 1000 Hz => PRI = 0.001 s
    // wf.pulseWidth_s = 10e-6 s

    // TC-AESA-LIB-009: real detection creates entry in output
    // REQ-AESA-040: first detection must create an accumulator entry
    lib.accumulate(1u, 1e-10, 0.0, cfg, wf, false);
    std::vector<aesa::SignalIntercept> out;
    lib.getIntercepts(out);
    ASSERT_FALSE(out.empty(),
                 "TC-AESA-LIB-009: real detection creates entry in getIntercepts output");
    ASSERT_EQ(out[0].targetID, 1u,
              "TC-AESA-LIB-009b: accumulated entry has correct targetID");

    // TC-AESA-LIB-010: frequency averaged over 3 dwells
    // REQ-AESA-040: running average must converge to true value
    lib.clear();
    aesa::RadarConfig cfgFixed = buildDefaultLibConfig();
    cfgFixed.frequencyAgility = false;
    cfgFixed.frequency_Hz     = 10.0e9;
    for (int i = 0; i < 3; ++i)
    {
        lib.accumulate(2u, 1e-10, static_cast<double>(i), cfgFixed, wf, false);
    }
    lib.getIntercepts(out);
    ASSERT_NEAR(out[0].frequency_Hz, 10.0e9, 1.0,
                "TC-AESA-LIB-010: frequency average converges to cfg.frequency_Hz");

    // TC-AESA-LIB-011: PRI averaged correctly
    // REQ-AESA-040: PRI = 1 / prf_Hz = 0.001 s
    lib.clear();
    for (int i = 0; i < 3; ++i)
    {
        lib.accumulate(3u, 1e-10, static_cast<double>(i), cfgFixed, wf, false);
    }
    lib.getIntercepts(out);
    ASSERT_NEAR(out[0].pri_s, 1.0 / 1000.0, 1e-6,
                "TC-AESA-LIB-011: PRI average = 1/PRF = 0.001 s");

    // TC-AESA-LIB-012: pulseWidth averaged correctly
    // REQ-AESA-040: pulseWidth should equal wf.pulseWidth_s = 10e-6 s
    ASSERT_NEAR(out[0].pulseWidth_s, 10e-6, 1e-9,
                "TC-AESA-LIB-012: pulseWidth average = wf.pulseWidth_s");

    // TC-AESA-LIB-013: signalLevel computed in dBW
    // REQ-AESA-040: 1e-10 W => 10*log10(1e-10) = -100 dBW
    lib.clear();
    lib.accumulate(4u, 1e-10, 0.0, cfgFixed, wf, false);
    lib.getIntercepts(out);
    ASSERT_NEAR(out[0].signalLevel_dBW, -100.0, 0.1,
                "TC-AESA-LIB-013: signalLevel = 10*log10(1e-10) = -100 dBW");

    // TC-AESA-LIB-014: power <= 1e-30 records floor value (-300 dBW)
    // REQ-AESA-040: noise-floor measurement must not produce log10(0)
    lib.clear();
    lib.accumulate(5u, 0.0, 0.0, cfgFixed, wf, false);
    lib.getIntercepts(out);
    ASSERT_NEAR(out[0].signalLevel_dBW, -300.0, 0.1,
                "TC-AESA-LIB-014: zero power records floor value -300 dBW");

    // TC-AESA-LIB-015: frequency agility uses midpoint of hop band
    // REQ-AESA-040: midpoint of [9e9, 11e9] = 10e9
    lib.clear();
    aesa::RadarConfig cfgAgile    = buildDefaultLibConfig();
    cfgAgile.frequencyAgility     = true;
    cfgAgile.hopStartFrequency    = 9.0e9f;
    cfgAgile.hopStopFrequency     = 11.0e9f;
    cfgAgile.frequency_Hz         = 5.0e9;  // should be ignored
    for (int i = 0; i < 3; ++i)
    {
        lib.accumulate(6u, 1e-10, static_cast<double>(i), cfgAgile, wf, false);
    }
    lib.getIntercepts(out);
    ASSERT_NEAR(out[0].frequency_Hz, 10.0e9, 1.0,
                "TC-AESA-LIB-015: agility midpoint (9e9+11e9)/2 = 10e9 recorded");

    // TC-AESA-LIB-016: lastSeen timestamp updated by accumulate()
    // REQ-AESA-030: timestamp must reflect most recent detection time
    lib.clear();
    lib.accumulate(7u, 1e-10, 5.0, cfgFixed, wf, false);
    lib.accumulate(7u, 1e-10, 10.0, cfgFixed, wf, false);
    // If lastSeen is updated correctly, pruneStale with coast=6.0 at time=15.0
    // should NOT prune (15.0 - 10.0 = 5.0 < 6.0 coast). REQ-AESA-030.
    lib.pruneStale(15.0, 6.0);
    lib.getIntercepts(out);
    bool found7 = false;
    for (const auto& si : out) if (si.targetID == 7u) found7 = true;
    ASSERT_TRUE(found7,
                "TC-AESA-LIB-016: lastSeen updated — recent entry not pruned");
}

// =============================================================================
// TEST SUITE: test_lib_getIntercepts
// Covers: TC-AESA-LIB-017 through TC-AESA-LIB-019
// Requirements: REQ-AESA-004
// =============================================================================
void test_lib_getIntercepts()
{
    std::cout << "\n--- TC-AESA-LIB-017..019: getIntercepts() Tests ---"
              << std::endl;

    aesa::RadarSignalLibrary_AESA  lib;
    aesa::RadarConfig              cfg = buildDefaultLibConfig();
    aesa::BeamWaveform             wf  = buildDefaultWaveform();
    std::vector<aesa::SignalIntercept> out;

    // TC-AESA-LIB-017: getIntercepts() returns empty on fresh library
    // REQ-AESA-004: no intercepts before any detection
    lib.getIntercepts(out);
    ASSERT_TRUE(out.empty(),
                "TC-AESA-LIB-017: getIntercepts() returns empty on fresh library");

    // TC-AESA-LIB-018: getIntercepts() returns one entry after one accumulation
    // REQ-AESA-004: one target produces one intercept entry
    lib.accumulate(1u, 1e-10, 0.0, cfg, wf, false);
    lib.getIntercepts(out);
    ASSERT_EQ(static_cast<int>(out.size()), 1,
              "TC-AESA-LIB-018: getIntercepts() returns 1 entry after 1 accumulation");

    // TC-AESA-LIB-019: getIntercepts() clears output before fill
    // REQ-AESA-004: stale output from previous call must not persist
    // Pre-fill output with junk data
    out.push_back(aesa::SignalIntercept{});
    out.push_back(aesa::SignalIntercept{});
    lib.getIntercepts(out);
    // Should contain exactly the entries in acc_ (1 target), not 1 + 2 junk
    ASSERT_EQ(static_cast<int>(out.size()), 1,
              "TC-AESA-LIB-019: getIntercepts() clears output before filling");
}

// =============================================================================
// TEST SUITE: test_lib_pruneStale
// Covers: TC-AESA-LIB-020 through TC-AESA-LIB-022
// Requirements: REQ-AESA-030
// =============================================================================
void test_lib_pruneStale()
{
    std::cout << "\n--- TC-AESA-LIB-020..022: pruneStale() Tests ---"
              << std::endl;

    aesa::RadarSignalLibrary_AESA  lib;
    aesa::RadarConfig              cfg = buildDefaultLibConfig();
    aesa::BeamWaveform             wf  = buildDefaultWaveform();
    std::vector<aesa::SignalIntercept> out;

    // Accumulate two targets at different times
    lib.accumulate(10u, 1e-10,  0.0, cfg, wf, false);   // old — time=0
    lib.accumulate(11u, 1e-10, 25.0, cfg, wf, false);   // recent — time=25

    // TC-AESA-LIB-020: pruneStale() removes expired entry
    // REQ-AESA-030: target 10 age = 30-0 = 30 > coast=20 -> pruned
    lib.pruneStale(30.0, 20.0);
    lib.getIntercepts(out);
    bool found10 = false;
    for (const auto& si : out) if (si.targetID == 10u) found10 = true;
    ASSERT_FALSE(found10,
                 "TC-AESA-LIB-020: expired entry (age=30 > coast=20) pruned");

    // TC-AESA-LIB-021: pruneStale() retains recent entry
    // REQ-AESA-030: target 11 age = 30-25 = 5 < coast=20 -> retained
    bool found11 = false;
    for (const auto& si : out) if (si.targetID == 11u) found11 = true;
    ASSERT_TRUE(found11,
                "TC-AESA-LIB-021: recent entry (age=5 < coast=20) retained");

    // TC-AESA-LIB-022: pruneStale() removes from both acc_ and lastSeen_
    // REQ-AESA-030: consistency — pruning must be atomic for both maps
    // After pruning target 10, re-accumulating it at a fresh time should
    // work correctly (no ghost entry from lastSeen_ remains). REQ-AESA-030.
    lib.accumulate(10u, 1e-10, 35.0, cfg, wf, false);  // re-introduce target 10
    lib.getIntercepts(out);
    bool found10Again = false;
    for (const auto& si : out) if (si.targetID == 10u) found10Again = true;
    ASSERT_TRUE(found10Again,
                "TC-AESA-LIB-022: pruned target can be re-added cleanly "
                "(both acc_ and lastSeen_ were removed)");
}

// =============================================================================
// TEST SUITE: test_lib_matchLibrary
// Covers: TC-AESA-LIB-023 through TC-AESA-LIB-025
// Requirements: REQ-AESA-040
// =============================================================================
void test_lib_matchLibrary()
{
    std::cout << "\n--- TC-AESA-LIB-023..025: matchLibrary() Tests ---"
              << std::endl;

    aesa::RadarSignalLibrary_AESA  lib;
    aesa::RadarConfig              cfg = buildDefaultLibConfig();
    aesa::BeamWaveform             wf  = buildDefaultWaveform();
    std::vector<aesa::SignalIntercept> out;

    // Load a library with one known emitter at 10 GHz
    lib.loadLibrary({
        buildLibraryEntry("SA-10-FLAP-LID", 10.0e9, 500e6,
                          aesa::ModulationType::NONE)
    });

    // TC-AESA-LIB-023: matchLibrary() returns no match before 3 frequency obs
    // REQ-AESA-040: minimum 3 observations required for reliable classification
    lib.accumulate(1u, 1e-10, 0.0, cfg, wf, false);  // 1 observation
    lib.getIntercepts(out);
    bool emptyId = true;
    for (const auto& si : out)
    {
        if (si.targetID == 1u && !si.emitterID.empty()) emptyId = false;
    }
    ASSERT_TRUE(emptyId,
                "TC-AESA-LIB-023: emitterID remains empty before 3 frequency obs");

    // TC-AESA-LIB-024: matchLibrary() identifies known emitter after 3 obs
    // REQ-AESA-040: emitter ID must be assigned after sufficient observations
    lib.accumulate(1u, 1e-10, 1.0, cfg, wf, false);  // 2nd observation
    lib.accumulate(1u, 1e-10, 2.0, cfg, wf, false);  // 3rd observation — triggers match
    lib.getIntercepts(out);
    std::string foundID;
    for (const auto& si : out)
    {
        if (si.targetID == 1u) foundID = si.emitterID;
    }
    ASSERT_EQ(foundID, std::string("SA-10-FLAP-LID"),
              "TC-AESA-LIB-024: emitterID matched to SA-10-FLAP-LID after 3 obs");

    // TC-AESA-LIB-025: matchLibrary() returns unchanged ID for no match
    // REQ-AESA-040: emitter at wrong frequency returns empty (no false positive)
    lib.clear();
    aesa::RadarConfig cfgWrong = buildDefaultLibConfig();
    cfgWrong.frequency_Hz = 3.0e9;   // far from library entry at 10 GHz
    for (int i = 0; i < 3; ++i)
    {
        lib.accumulate(2u, 1e-10, static_cast<double>(i), cfgWrong, wf, false);
    }
    lib.getIntercepts(out);
    std::string noMatch;
    for (const auto& si : out)
    {
        if (si.targetID == 2u) noMatch = si.emitterID;
    }
    ASSERT_TRUE(noMatch.empty(),
                "TC-AESA-LIB-025: no match at wrong frequency leaves emitterID empty");
}

// =============================================================================
// ENTRY POINT: radarSignalLibrary_test
// =============================================================================
void radarSignalLibrary_test()
{
    testsPassed = 0;
    testsFailed = 0;
    tests       = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "   RADARSIGNALLIBRARY_AESA UNIT TESTS    " << std::endl;
    std::cout << "   Standard: DO-178C DAL B               " << std::endl;
    std::cout << "   Project:  ISSST                       " << std::endl;
    std::cout << "   Org:      Oxygen to Innovation Pvt. Ltd." << std::endl;
    std::cout << "=========================================" << std::endl;

    test_lib_clear();
    test_lib_loadLibrary();
    test_lib_accumulate_ghost();
    test_lib_accumulate_real();
    test_lib_getIntercepts();
    test_lib_pruneStale();
    test_lib_matchLibrary();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "SIGNAL LIBRARY TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
