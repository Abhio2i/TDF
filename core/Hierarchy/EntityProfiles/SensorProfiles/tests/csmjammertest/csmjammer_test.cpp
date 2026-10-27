// csmjammer_test.cpp - focused, Qt-free tests of the CSM RECEIVE/TRANSMIT
// interference model (detector, signal model, spectrum). Builds against the
// real CSM/*.cpp sources:
//   g++ -std=c++17 -Wall -Wextra -I<repo root> csmjammer_test.cpp
//       <repo>/core/Hierarchy/EntityProfiles/SensorProfiles/CSM/*.cpp -o csmjammer_test
// Not covered here (needs Qt / Hierarchy): CSM::scan wiring, JSON round trip,
// entity removal. See the delivery notes.
#include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmdetector.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmsignalmodel.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmspectrum.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <type_traits>

using namespace csm;

static int g_fail = 0, g_checks = 0;
#define CHECK(c) do { ++g_checks; if (!(c)) { ++g_fail; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
#define NEAR(a, b, tol) do { ++g_checks; const double _a=(a), _b=(b); if (!(std::fabs(_a-_b) <= (tol))) { ++g_fail; \
    std::printf("  FAIL %s:%d  %s=%.6f vs %s=%.6f\n", __FILE__, __LINE__, #a, _a, #b, _b); } } while (0)

// Free-space path loss [dB], d in km, f in Hz (stand-in for the propagation model).
static double fspl(double dKm, double fHz) { return 32.44 + 20.0 * std::log10(dKm) + 20.0 * std::log10(fHz / 1e6); }

// ---- Scenario: E1 radio -> E2 CSM; E3 CSM transmit --------------------------
static const double kF = 150e6, kSigBw = 25e3, kRxGain = 0.0;
static const double kD12 = 10.0, kD32 = 5.0;       // km

static EmitterInput e1Radio()
{
    EmitterInput e;
    e.platformId = "E1"; e.platformName = "Entity1"; e.radioId = "R1"; e.radioName = "Radio1";
    e.platformActive = e.radioActive = e.canTransmit = e.inCoverage = true;
    e.distanceM = kD12 * 1000; e.bearingDeg = 45;
    e.frequencyHz = kF; e.bandwidthHz = kSigBw; e.minFreqHz = 30e6; e.maxFreqHz = 512e6;
    e.modulation = "QPSK"; e.txPowerDbm = 30; e.txDutyCycle = 0; e.txAntennaGainDbi = 0;
    return e;
}
static MonitorConfig e2Monitor(double minSnr = kNaN)
{
    MonitorConfig m;
    m.centreHz = kF; m.bandwidthHz = 1e6;            // 1 MHz span around 150 MHz
    m.rxGainDbi = kRxGain; m.noiseFigureDb = 6; m.temperatureC = 16.85; m.minSnrDb = minSnr;
    return m;
}
static InterfererInput e3Jammer(double fHz = kF, double bwHz = 10e6, double pDbm = 40)
{
    InterfererInput j;
    CHECK(signal::makeInterferer(true, fHz / 1e9, bwHz / 1e6, pDbm, 0.0, j));
    j.inCoverage = true;
    j.pathLossDb = fspl(kD32, fHz);
    return j;
}
static Detector::PathLossFn pl12 = [](const EmitterInput&) { return fspl(kD12, kF); };

static ScanResult run(const std::vector<InterfererInput>* js, double minSnr = kNaN,
                      std::vector<EmitterInput> em = { e1Radio() })
{
    return Detector::evaluate(em, "E2", e2Monitor(minSnr), pl12, 1.0, js);
}

static spectrum::Input specInput(const std::vector<InterfererInput>& js)
{
    spectrum::Input in;
    in.monitorCentreHz = kF; in.monitorSpanHz = 1e6; in.noiseFigureDb = 6; in.temperatureC = 16.85;
    const EmitterInput e = e1Radio();
    spectrum::EmitterSignal s;
    s.label = "E1/R1"; s.frequencyHz = e.frequencyHz; s.bandwidthHz = e.bandwidthHz; s.modulation = "QPSK";
    s.powerDbm = signal::receivedPowerDbm(e.txPowerDbm, 0, 0, 0, kRxGain, fspl(kD12, kF));
    in.emitters.push_back(s);
    for (const InterfererInput& j : js) {
        spectrum::InterfererSignal is;
        is.centreHz = j.frequencyHz; is.bandwidthHz = j.bandwidthHz;
        is.powerDbm = signal::receivedPowerDbm(j.txPowerDbm, 0, 1, j.txGainDbi, kRxGain, j.pathLossDb);
        in.interferers.push_back(is);
    }
    return in;
}
static double psdAt(const spectrum::Result& r, double hz)
{
    const int i = static_cast<int>((hz - r.span.startHz) / r.span.binHz);
    return r.span.psdDbmHz.at(static_cast<size_t>(i));
}
static double traceMaxPsd(const spectrum::Result& r)
{
    double m = -1e9; for (double v : r.span.psdDbmHz) m = std::fmax(m, v); return m;
}
// Total power [mW] in the span trace (sum of PSD * binHz).
static double tracePowerMw(const spectrum::Trace& t)
{
    double s = 0; for (double v : t.psdDbmHz) s += std::pow(10.0, v / 10.0) * t.binHz; return s;
}

// ---------------------------------------------------------------------------
static void testA_receiveRegression()
{
    std::puts("TEST A  receive-mode regression");
    CHECK(OperatingMode::Receive != OperatingMode::Transmit);
    CHECK(std::string(operatingModeName(OperatingMode::Receive)) == "RECEIVE");
    const ScanResult base = run(nullptr);
    const std::vector<InterfererInput> none;
    const ScanResult empty = run(&none);
    CHECK(base.observations.size() == 1 && empty.observations.size() == 1);
    const Observation& o = base.observations[0];
    // Hand-computed link budget: 30 - FSPL(10 km, 150 MHz) = -65.96 dBm
    NEAR(o.levelDbm.value, 30.0 - fspl(kD12, kF), 1e-9);
    NEAR(o.noiseFloorDbm.value, -174 + 10*std::log10((16.85+273.15)/290.0) + 10*std::log10(kSigBw) + 6, 1e-9);
    NEAR(o.snrDb.value, o.levelDbm.value - o.noiseFloorDbm.value, 1e-12);
    CHECK(!o.interferenceDbm.valid());
    NEAR(o.sinrDb.value, o.snrDb.value, 0.0);               // SINR == SNR, bit-exact
    CHECK(!base.interference.present);
    CHECK(base.stats == empty.stats);
    NEAR(o.levelDbm.value, empty.observations[0].levelDbm.value, 0.0);
    // Spectrum: no interferers => identical to the pre-existing model.
    const spectrum::Result r0 = spectrum::compute(specInput({}));
    CHECK(r0.available && r0.interferenceDrawnCount == 0 && r0.drawnCount == 1);
}

static void testB_transmitMode()
{
    std::puts("TEST B  transmitter mode / source availability");
    InterfererInput j;
    CHECK(!signal::makeInterferer(false, 0.15, 10, 40, 0, j));          // RECEIVE / txEnabled off => no source
    CHECK(signal::makeInterferer(true, 0.15, 10, 40, 3, j));
    NEAR(j.frequencyHz, 150e6, 1e-3); NEAR(j.bandwidthHz, 10e6, 1e-3);
    NEAR(j.txPowerDbm, 40, 0); NEAR(j.txGainDbi, 3, 0);
    InterfererInput k = j;
    CHECK(!signal::makeInterferer(true, 0.15, 0, 40, 0, k));            // invalid bandwidth
    CHECK(!signal::makeInterferer(true, -1, 10, 40, 0, k));             // invalid frequency
    CHECK(!signal::makeInterferer(true, 0.15, 10, kNaN, 0, k));         // NaN power
    NEAR(k.txGainDbi, 3, 0);                                            // out untouched on failure
}

static void testC_receiverInterference()
{
    std::puts("TEST C  receiver interference (E1 -> E2, E3 jamming)");
    const EmitterInput e1 = e1Radio();
    const EmitterInput e1Before = e1;
    const std::vector<InterfererInput> js{ e3Jammer() };
    const ScanResult base = run(nullptr), jam = run(&js);
    CHECK(jam.observations.size() == 1);
    const Observation& b = base.observations[0];
    const Observation& o = jam.observations[0];

    // Desired level and noise are untouched; snrDb keeps S/N semantics.
    NEAR(o.levelDbm.value, b.levelDbm.value, 0.0);
    NEAR(o.noiseFloorDbm.value, b.noiseFloorDbm.value, 0.0);
    NEAR(o.snrDb.value, b.snrDb.value, 0.0);

    // Hand calculation: P_j = 40 - FSPL(5 km) ; in-band = P_j * 25k/10M (linear)
    const double pj = 40.0 - fspl(kD32, kF);
    const double iDbm = pj + 10*std::log10(kSigBw / 10e6);
    NEAR(o.interferenceDbm.value, iDbm, 1e-9);
    const double nMw = std::pow(10.0, o.noiseFloorDbm.value/10), iMw = std::pow(10.0, iDbm/10);
    NEAR(o.sinrDb.value, o.levelDbm.value - 10*std::log10(nMw + iMw), 1e-9);
    CHECK(o.sinrDb.value < o.snrDb.value - 40.0);           // strongly degraded
    CHECK(jam.interference.present);
    NEAR(jam.interference.inMonitorDbm.value, pj + 10*std::log10(1e6/10e6), 1e-9);  // 1 MHz of 10 MHz

    // Detection consequence with a 20 dB gate: detected without jammer, lost with it
    // - and the loss is attributed to interference, not to noise.
    const ScanResult baseG = run(nullptr, 20.0), jamG = run(&js, 20.0);
    CHECK(baseG.observations.size() == 1);
    CHECK(jamG.observations.empty());
    CHECK(jamG.stats.belowSinrThreshold == 1 && jamG.stats.belowSnrThreshold == 0);
    // Not "everything suppressed": a weak gate still detects with a jammer.
    CHECK(run(&js, -30.0).observations.size() == 1);

    // Spectrum: jammer raises PSD over the jammed band, exactly P_j / B_j.
    const spectrum::Result r0 = spectrum::compute(specInput({}));
    const spectrum::Result r1 = spectrum::compute(specInput(js));
    CHECK(r1.interferenceDrawnCount == 1);
    const double jamPsd = pj - 10*std::log10(10e6);
    const double off = kF + 400e3;                           // away from the 25 kHz signal
    NEAR(psdAt(r0, off), r0.noisePsdDbmHz, 1e-6);
    const double expect = 10*std::log10(std::pow(10, r0.noisePsdDbmHz/10) + std::pow(10, jamPsd/10));
    NEAR(psdAt(r1, off), expect, 1e-6);
    CHECK(psdAt(r1, off) > psdAt(r0, off) + 40.0);
    // Power conservation: added trace power == jammer power inside the 1 MHz span.
    NEAR((tracePowerMw(r1.span) - tracePowerMw(r0.span)) / std::pow(10, (pj + 10*std::log10(0.1))/10), 1.0, 1e-6);

    // E1's radio configuration is not touched (snapshots are by value).
    CHECK(e1.txPowerDbm == e1Before.txPowerDbm && e1.frequencyHz == e1Before.frequencyHz &&
          e1.bandwidthHz == e1Before.bandwidthHz && e1.modulation == e1Before.modulation);
}

static void testD_noJammer()
{
    std::puts("TEST D  jammer removed => baseline restored");
    const std::vector<InterfererInput> js{ e3Jammer() };
    const std::vector<InterfererInput> none;
    spectrum::Cache cache;
    const double jammed = traceMaxPsd(*cache.update(specInput(js)));
    const int c1 = cache.computeCount();
    const spectrum::Result base = spectrum::compute(specInput({}));
    const auto after = cache.update(specInput(none));           // cache MUST invalidate
    CHECK(cache.computeCount() == c1 + 1);
    CHECK(after->interferenceDrawnCount == 0);
    NEAR(psdAt(*after, kF + 400e3), psdAt(base, kF + 400e3), 0.0);
    CHECK(jammed >= traceMaxPsd(*after));
    const ScanResult r = run(&none, 20.0);
    CHECK(r.observations.size() == 1 && !r.interference.present && !r.observations[0].interferenceDbm.valid());
    // Out of coverage is equivalent to no jammer.
    InterfererInput far = e3Jammer(); far.inCoverage = false;
    const std::vector<InterfererInput> farv{ far };
    CHECK(!run(&farv, 20.0).interference.present && run(&farv, 20.0).observations.size() == 1);
}

static void testE_frequencyRelevance()
{
    std::puts("TEST E  frequency relevance");
    // Disjoint from the monitor span (monitor 149.5-150.5 MHz, jammer 200 MHz +/- 5 MHz).
    const std::vector<InterfererInput> outside{ e3Jammer(200e6, 10e6, 40) };
    const ScanResult o = run(&outside);
    CHECK(!o.interference.present && !o.observations[0].interferenceDbm.valid());
    NEAR(o.observations[0].sinrDb.value, o.observations[0].snrDb.value, 0.0);
    const spectrum::Result r = spectrum::compute(specInput(outside));
    CHECK(r.interferenceDrawnCount == 0);
    NEAR(psdAt(r, kF + 400e3), r.noisePsdDbmHz, 1e-6);
    // Overlaps the monitor span but NOT the signal's band: spectrum + summary change,
    // the signal's SINR does not.
    const std::vector<InterfererInput> adj{ e3Jammer(kF + 450e3, 100e3, 40) };   // 150.40-150.50 MHz
    const ScanResult a = run(&adj);
    CHECK(a.interference.present && !a.observations[0].interferenceDbm.valid());
    CHECK(spectrum::compute(specInput(adj)).interferenceDrawnCount == 1);
    // Partial overlap: jammer edge covers half of the signal band -> half the power.
    const std::vector<InterfererInput> part{ e3Jammer(kF + 50e3, 100e3, 40) };   // spans kF..kF+100k: covers the upper half of the signal band
    const ScanResult p = run(&part);
    // Touching-edge case: a jammer starting exactly at the signal's upper edge adds nothing.
    const std::vector<InterfererInput> edge{ e3Jammer(kF + 12.5e3 + 50e3, 100e3, 40) };
    CHECK(!run(&edge).observations[0].interferenceDbm.valid());
    const double pjPart = 40.0 - fspl(kD32, kF + 50e3);   // path loss at the jammer's own centre frequency
    NEAR(p.observations[0].interferenceDbm.value, pjPart + 10*std::log10(12.5e3 / 100e3), 1e-9);
    // Fully overlapping -> includes whole band.
    const std::vector<InterfererInput> ov{ e3Jammer() };
    CHECK(run(&ov).observations[0].interferenceDbm.valid());
}

static void testF_privacy()
{
    std::puts("TEST F  source identity privacy");
    // Structural: the interference types carry no identity, range or bearing.
    static_assert(std::is_standard_layout<InterfererInput>::value, "plain data");
    static_assert(sizeof(InterfererInput) == 6 * sizeof(double),
                  "InterfererInput must hold only bool + 5 doubles (no strings/ids)");
    const std::vector<InterfererInput> js{ e3Jammer() };
    const ScanResult r = run(&js);
    // Only E1 appears in the observation table; E3 contributes no row/platform.
    CHECK(r.observations.size() == 1 && r.observations[0].platformId == "E1");
    CHECK(r.platforms.size() == 1 && r.platforms[0].platformId == "E1");
    for (const Observation& o : r.observations) {
        CHECK(o.platformId != "E3" && o.platformName != "Entity3");
        // Observation distance/bearing remain E1's own geometry.
        NEAR(o.distanceM.value, kD12 * 1000, 1e-9); NEAR(o.bearingDeg.value, 45, 1e-9);
    }
    CHECK(r.stats.emittersSeen == 1 && r.stats.detectedRadios == 1);
    // Summary says "present + level", nothing else.
    CHECK(r.interference.present && r.interference.inMonitorDbm.valid());
    // Spectrum notes list emitters only (no interferer label/notes).
    const spectrum::Result sp = spectrum::compute(specInput(js));
    CHECK(sp.emitters.size() == 1 && sp.emitters[0].label == "E1/R1");
}

static void testI_multipleSources()
{
    std::puts("TEST I  multiple sources, linear aggregation");
    InterfererInput j1 = e3Jammer(kF, 10e6, 40), j2 = e3Jammer(kF, 10e6, 40);   // identical
    const std::vector<InterfererInput> one{ j1 }, two{ j1, j2 };
    const double i1 = run(&one).observations[0].interferenceDbm.value;
    const double i2 = run(&two).observations[0].interferenceDbm.value;
    NEAR(i2 - i1, 10*std::log10(2.0), 1e-9);                  // +3.01 dB, not +40 dB / not dB-added
    // Two normal radios do not overwrite each other.
    EmitterInput a = e1Radio(), b = e1Radio();
    b.platformId = "E4"; b.radioId = "R4"; b.frequencyHz = kF + 300e3; b.txPowerDbm = 20;
    const ScanResult both = Detector::evaluate({ a, b }, "E2", e2Monitor(),
        [](const EmitterInput& e) { return fspl(kD12, e.frequencyHz); }, 1.0, &two);
    CHECK(both.observations.size() == 2);
    CHECK(both.observations[0].interferenceDbm.valid() && both.observations[1].interferenceDbm.valid());
    NEAR(both.observations[0].levelDbm.value, 30 - fspl(kD12, kF), 1e-9);
    NEAR(both.observations[1].levelDbm.value, 20 - fspl(kD12, kF + 300e3), 1e-9);
    // Different-frequency jammers: each only affects the signal it overlaps.
    InterfererInput jb = e3Jammer(kF + 300e3, 50e3, 40);      // covers only radio b's band
    const std::vector<InterfererInput> sep{ jb };
    const ScanResult s = Detector::evaluate({ a, b }, "E2", e2Monitor(),
        [](const EmitterInput& e) { return fspl(kD12, e.frequencyHz); }, 1.0, &sep);
    CHECK(!s.observations[0].interferenceDbm.valid() && s.observations[1].interferenceDbm.valid());
    // Spectrum: two jammers -> exact power sum (linear), sources can't hide each other.
    const spectrum::Result r1 = spectrum::compute(specInput(one)), r2 = spectrum::compute(specInput(two));
    const double off = kF + 400e3;
    NEAR(psdAt(r2, off) - psdAt(r1, off), 3.0103, 0.01);      // jam dominates noise there
    // dBm helpers round trip; invalid -> NaN.
    NEAR(signal::mwToDbm(signal::dbmToMw(-73.5)), -73.5, 1e-12);
    CHECK(std::isnan(signal::mwToDbm(0.0)));
    // Determinism.
    const spectrum::Result r2b = spectrum::compute(specInput(two));
    CHECK(r2b.span.psdDbmHz == r2.span.psdDbmHz);
}

static void testH_statelessSources()
{
    std::puts("TEST H  no stale state in the pure layer");
    // The detector and spectrum keep no history: results depend only on inputs,
    // so toggling a source off immediately yields the baseline again.
    const std::vector<InterfererInput> js{ e3Jammer() }, none;
    const ScanResult on1 = run(&js), off = run(&none), on2 = run(&js);
    CHECK(on1.stats == on2.stats && on1.observations[0].sinrDb.value == on2.observations[0].sinrDb.value);
    CHECK(!off.interference.present);
    // Double-processing guard: evaluating the same input twice gives identical,
    // non-accumulating results.
    CHECK(run(&js).observations[0].interferenceDbm.value == run(&js).observations[0].interferenceDbm.value);
}

int main()
{
    testA_receiveRegression();
    testB_transmitMode();
    testC_receiverInterference();
    testD_noJammer();
    testE_frequencyRelevance();
    testF_privacy();
    testH_statelessSources();
    testI_multipleSources();
    std::printf("\n%d checks, %d failed\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
