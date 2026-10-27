// // csmspectrum.cpp - see csmspectrum.h for the model, equations and limits.

// #include "csmspectrum.h"
// #include "csmsignalmodel.h"
// #include <algorithm>
// #include <cmath>

// namespace csm {
// namespace spectrum {

// namespace {

// const double kPi = 3.14159265358979323846;
// const double kNaN = std::numeric_limits<double>::quiet_NaN();

// bool positiveFinite(double v) { return std::isfinite(v) && v > 0.0; }
// double dbmToMw(double dbm) { return std::pow(10.0, dbm / 10.0); }
// double mwToDbm(double mw) { return 10.0 * std::log10(mw); }

// // A drawn signal, reduced to what the PSD integral needs.
// // struct Signal {
// //     double centreHz;
// //     double powerMw;
// //     double symbolRateHz;
// //     double occupiedBwHz;
// // };

// using Signal = DrawnSignal;   // see csmspectrum.h

// Trace makeTrace(double startHz, double stopHz, int bins, double n0MwPerHz,
//                 const std::vector<Signal>& sigs)
// {
//     Trace t;
//     t.startHz = startHz;
//     t.binHz = (stopHz - startHz) / static_cast<double>(bins);
//     t.psdDbmHz.assign(static_cast<size_t>(bins), kNaN);

//     std::vector<double> prev(sigs.size());
//     for (size_t k = 0; k < sigs.size(); ++k)
//         prev[k] = rcCumulative(startHz - sigs[k].centreHz, sigs[k].symbolRateHz, kRollOff);

//     for (int i = 0; i < bins; ++i) {
//         const double edge = startHz + t.binHz * static_cast<double>(i + 1);
//         double powerMw = n0MwPerHz * t.binHz;
//         for (size_t k = 0; k < sigs.size(); ++k) {
//             const double cur = rcCumulative(edge - sigs[k].centreHz, sigs[k].symbolRateHz, kRollOff);
//             powerMw += sigs[k].powerMw * std::max(0.0, cur - prev[k]);
//             prev[k] = cur;
//         }
//         t.psdDbmHz[static_cast<size_t>(i)] = mwToDbm(powerMw / t.binHz);
//     }
//     return t;
// }

// bool numEq(double a, double b)
// {
//     if (std::isnan(a) && std::isnan(b)) return true;
//     return a == b;
// }

// bool levelEq(double a, double b)
// {
//     if (std::isnan(a) && std::isnan(b)) return true;
//     if (std::isnan(a) || std::isnan(b)) return false;
//     return std::fabs(a - b) <= kPowerCacheToleranceDb;
// }
// bool supportsLinearRcPsd(const std::string& mod)
// {
//     return mod == "BPSK" || mod == "QPSK" || mod == "PSK8" ||
//            mod == "QAM16" || mod == "QAM64";
// }

// Result unavailable(const char* why)
// {
//     Result r;
//     r.reason = why;
//     return r;
// }

// } // namespace

// double rcCumulative(double offsetHz, double rs, double beta)
// {
//     const double f1 = 0.5 * (1.0 - beta) * rs;   // end of flat part
//     const double f2 = 0.5 * (1.0 + beta) * rs;   // = B/2, end of roll-off
//     const double ax = std::fabs(offsetHz);
//     double half;                                  // power in [0, ax]
//     if (ax <= f1) {
//         half = ax / rs;
//     } else if (ax >= f2) {
//         half = 0.5;
//     } else {
//         const double u = ax - f1;
//         const double a = kPi / (beta * rs);
//         half = f1 / rs + (u + std::sin(a * u) / a) / (2.0 * rs);
//     }
//     return offsetHz < 0.0 ? 0.5 - half : 0.5 + half;
// }

// Result compute(const Input& in)
// {
//     // --- Monitor ---------------------------------------------------------
//     if (!positiveFinite(in.monitorCentreHz))
//         return unavailable("monitor centre frequency invalid");
//     if (!std::isfinite(in.monitorSpanHz) || in.monitorSpanHz < 0.0)
//         return unavailable("monitor bandwidth invalid");
//     if (in.monitorSpanHz == 0.0)
//         return unavailable("monitor bandwidth is 0 (tuned point): no frequency span to display");
//     const double lo = in.monitorCentreHz - 0.5 * in.monitorSpanHz;
//     const double hi = in.monitorCentreHz + 0.5 * in.monitorSpanHz;
//     if (lo < 0.0)
//         return unavailable("monitor span extends below 0 Hz");

//     // --- Noise (existing thermal-noise logic, 1 Hz reference bandwidth) ----
//     const double n0Dbm = signal::thermalNoiseFloorDbm(1.0, in.noiseFigureDb, in.temperatureC);
//     if (!std::isfinite(n0Dbm))
//         return unavailable("noise figure or temperature unavailable");
//     const double n0Mw = dbmToMw(n0Dbm);

//     Result r;
//     r.available = true;
//     r.noisePsdDbmHz = n0Dbm;

//     // --- Emitters ----------------------------------------------------------
//     std::vector<Signal> sigs;
//     for (const EmitterSignal& e : in.emitters) {
//         EmitterNote n;
//         n.label = e.label;
//         n.frequencyHz = e.frequencyHz;
//         n.powerDbm = e.powerDbm;

//         if (e.modulation.empty()) {
//             n.note = "modulation not configured";
//         } else if (!supportsLinearRcPsd(e.modulation)) {
//             n.note = "modulation " + e.modulation + " unsupported (not drawn)";
//         } else if (!positiveFinite(e.frequencyHz)) {
//             n.note = "frequency unavailable";
//         } else if (!positiveFinite(e.bandwidthHz)) {
//             n.note = "bandwidth not configured";
//         } else if (!std::isfinite(e.powerDbm)) {
//             n.note = "received level unavailable";
//         } else {
//             const double rs = e.bandwidthHz / (1.0 + kRollOff);
//             const double fraction = rcCumulative(hi - e.frequencyHz, rs, kRollOff) -
//                                     rcCumulative(lo - e.frequencyHz, rs, kRollOff);
//             n.occupiedBandwidthHz = e.bandwidthHz;
//             n.symbolRateHz = rs;
//             n.peakPsdDbmHz = e.powerDbm - 10.0 * std::log10(rs);
//             n.inSpanFraction = fraction;
//             if (fraction <= 1e-12) {
//                 n.note = "outside monitor span (not drawn)";
//             } else {
//                 n.drawn = true;
//                 n.note = fraction < 1.0 - 1e-9 ? "partly outside monitor span" : "";
//                 sigs.push_back({ e.frequencyHz, dbmToMw(e.powerDbm), rs, e.bandwidthHz });
//             }
//         }
//         if (n.drawn) ++r.drawnCount; else ++r.skippedCount;
//         r.emitters.push_back(n);
//     }
//     r.drawnSignals = sigs;
//     r.noiseMwPerHz = n0Mw;

//     // --- Display trace over the whole monitor span ---------------------------
//     r.span = makeTrace(lo, hi, kDisplayBins, n0Mw, sigs);

//     // --- Zoom trace around the strongest in-span signal ----------------------
//     size_t best = 0;
//     double bestPower = -1.0;
//     for (size_t k = 0; k < sigs.size(); ++k) {
//         const double p = sigs[k].powerMw *
//                          (rcCumulative(hi - sigs[k].centreHz, sigs[k].symbolRateHz, kRollOff) -
//                           rcCumulative(lo - sigs[k].centreHz, sigs[k].symbolRateHz, kRollOff));
//         if (p > bestPower) { bestPower = p; best = k; }
//     }
//     if (!sigs.empty()) {
//         const double half = kZoomHalfWidthInBandwidths * sigs[best].occupiedBwHz;
//         const double zlo = std::max(lo, sigs[best].centreHz - half);
//         const double zhi = std::min(hi, sigs[best].centreHz + half);
//         if (zhi > zlo) {
//             r.zoom = makeTrace(zlo, zhi, kZoomBins, n0Mw, sigs);
//             r.hasZoom = true;
//             r.zoomCentreHz = sigs[best].centreHz;
//         }
//     }
//     return r;
// }

// bool windowTrace(const Result& r, double loHz, double hiHz, int bins, Trace& out)
// {
//     if (!r.available || bins < 2 || !(r.noiseMwPerHz > 0.0))
//         return false;

//     if (!std::isfinite(loHz) ||
//         !std::isfinite(hiHz) ||
//         !(hiHz > loHz) ||
//         loHz < 0.0)
//         return false;

//     out = makeTrace(loHz, hiHz, bins, r.noiseMwPerHz, r.drawnSignals);
//     return true;
// }

// bool axisRange(const Trace& t, double& loDb, double& hiDb)
// {
//     double mn = std::numeric_limits<double>::infinity();
//     double mx = -std::numeric_limits<double>::infinity();
//     for (double v : t.psdDbmHz) {
//         if (!std::isfinite(v)) continue;
//         mn = std::min(mn, v);
//         mx = std::max(mx, v);
//     }
//     if (!std::isfinite(mn) || !std::isfinite(mx)) return false;
//     loDb = std::floor((mn - 5.0) / 10.0) * 10.0;
//     hiDb = std::ceil((mx + 5.0) / 10.0) * 10.0;
//     if (hiDb - loDb < 20.0) hiDb = loDb + 20.0;
//     return true;
// }

// bool sameInput(const Input& a, const Input& b)
// {
//     if (!numEq(a.monitorCentreHz, b.monitorCentreHz) ||
//         !numEq(a.monitorSpanHz, b.monitorSpanHz) ||
//         !numEq(a.noiseFigureDb, b.noiseFigureDb) ||
//         !numEq(a.temperatureC, b.temperatureC) ||
//         a.emitters.size() != b.emitters.size())
//         return false;
//     for (size_t i = 0; i < a.emitters.size(); ++i) {
//         const EmitterSignal& x = a.emitters[i];
//         const EmitterSignal& y = b.emitters[i];
//         if (x.label != y.label || x.modulation != y.modulation ||
//             !numEq(x.frequencyHz, y.frequencyHz) ||
//             !numEq(x.bandwidthHz, y.bandwidthHz) ||
//             !levelEq(x.powerDbm, y.powerDbm))
//             return false;
//     }
//     return true;
// }

// std::shared_ptr<const Result> Cache::update(const Input& in)
// {
//     if (result_ && sameInput(key_, in)) return result_;
//     key_ = in;
//     result_ = std::make_shared<const Result>(compute(in));
//     ++computeCount_;
//     return result_;
// }

// void Cache::clear()
// {
//     result_.reset();
//     key_ = Input();
// }

// } // namespace spectrum
// } // namespace csm
















// csmspectrum.cpp - see csmspectrum.h for the model, equations and limits.

#include "csmspectrum.h"
#include "csmsignalmodel.h"
#include <algorithm>
#include <cmath>

namespace csm {
namespace spectrum {

namespace {

const double kPi = 3.14159265358979323846;
const double kNaN = std::numeric_limits<double>::quiet_NaN();

bool positiveFinite(double v) { return std::isfinite(v) && v > 0.0; }
double dbmToMw(double dbm) { return std::pow(10.0, dbm / 10.0); }
double mwToDbm(double mw) { return 10.0 * std::log10(mw); }

// A drawn signal, reduced to what the PSD integral needs.
// struct Signal {
//     double centreHz;
//     double powerMw;
//     double symbolRateHz;
//     double occupiedBwHz;
// };

using Signal = DrawnSignal;   // see csmspectrum.h

// Cumulative power fraction of a flat band [lo,hi] below x (0..1).
double flatCumulative(double x, double lo, double hi)
{
    if (x <= lo) return 0.0;
    if (x >= hi) return 1.0;
    return (x - lo) / (hi - lo);
}

Trace makeTrace(double startHz, double stopHz, int bins, double n0MwPerHz,
                const std::vector<Signal>& sigs,
                const std::vector<InterferenceBand>& jams)
{
    Trace t;
    t.startHz = startHz;
    t.binHz = (stopHz - startHz) / static_cast<double>(bins);
    t.psdDbmHz.assign(static_cast<size_t>(bins), kNaN);

    std::vector<double> prev(sigs.size());
    for (size_t k = 0; k < sigs.size(); ++k)
        prev[k] = rcCumulative(startHz - sigs[k].centreHz, sigs[k].symbolRateHz, kRollOff);

    std::vector<double> jprev(jams.size());
    for (size_t k = 0; k < jams.size(); ++k)
        jprev[k] = flatCumulative(startHz, jams[k].centreHz - 0.5 * jams[k].bandwidthHz,
                                  jams[k].centreHz + 0.5 * jams[k].bandwidthHz);

    for (int i = 0; i < bins; ++i) {
        const double edge = startHz + t.binHz * static_cast<double>(i + 1);
        double powerMw = n0MwPerHz * t.binHz;
        for (size_t k = 0; k < jams.size(); ++k) {
            const double cur = flatCumulative(edge, jams[k].centreHz - 0.5 * jams[k].bandwidthHz,
                                              jams[k].centreHz + 0.5 * jams[k].bandwidthHz);
            powerMw += jams[k].powerMw * std::max(0.0, cur - jprev[k]);
            jprev[k] = cur;
        }
        for (size_t k = 0; k < sigs.size(); ++k) {
            const double cur = rcCumulative(edge - sigs[k].centreHz, sigs[k].symbolRateHz, kRollOff);
            powerMw += sigs[k].powerMw * std::max(0.0, cur - prev[k]);
            prev[k] = cur;
        }
        t.psdDbmHz[static_cast<size_t>(i)] = mwToDbm(powerMw / t.binHz);
    }
    return t;
}

bool numEq(double a, double b)
{
    if (std::isnan(a) && std::isnan(b)) return true;
    return a == b;
}

bool levelEq(double a, double b)
{
    if (std::isnan(a) && std::isnan(b)) return true;
    if (std::isnan(a) || std::isnan(b)) return false;
    return std::fabs(a - b) <= kPowerCacheToleranceDb;
}
bool supportsLinearRcPsd(const std::string& mod)
{
    return mod == "BPSK" || mod == "QPSK" || mod == "PSK8" ||
           mod == "QAM16" || mod == "QAM64";
}

Result unavailable(const char* why)
{
    Result r;
    r.reason = why;
    return r;
}

} // namespace

double rcCumulative(double offsetHz, double rs, double beta)
{
    const double f1 = 0.5 * (1.0 - beta) * rs;   // end of flat part
    const double f2 = 0.5 * (1.0 + beta) * rs;   // = B/2, end of roll-off
    const double ax = std::fabs(offsetHz);
    double half;                                  // power in [0, ax]
    if (ax <= f1) {
        half = ax / rs;
    } else if (ax >= f2) {
        half = 0.5;
    } else {
        const double u = ax - f1;
        const double a = kPi / (beta * rs);
        half = f1 / rs + (u + std::sin(a * u) / a) / (2.0 * rs);
    }
    return offsetHz < 0.0 ? 0.5 - half : 0.5 + half;
}

Result compute(const Input& in)
{
    // --- Monitor ---------------------------------------------------------
    if (!positiveFinite(in.monitorCentreHz))
        return unavailable("monitor centre frequency invalid");
    if (!std::isfinite(in.monitorSpanHz) || in.monitorSpanHz < 0.0)
        return unavailable("monitor bandwidth invalid");
    if (in.monitorSpanHz == 0.0)
        return unavailable("monitor bandwidth is 0 (tuned point): no frequency span to display");
    const double lo = in.monitorCentreHz - 0.5 * in.monitorSpanHz;
    const double hi = in.monitorCentreHz + 0.5 * in.monitorSpanHz;
    if (lo < 0.0)
        return unavailable("monitor span extends below 0 Hz");

    // --- Noise (existing thermal-noise logic, 1 Hz reference bandwidth) ----
    const double n0Dbm = signal::thermalNoiseFloorDbm(1.0, in.noiseFigureDb, in.temperatureC);
    if (!std::isfinite(n0Dbm))
        return unavailable("noise figure or temperature unavailable");
    const double n0Mw = dbmToMw(n0Dbm);

    Result r;
    r.available = true;
    r.noisePsdDbmHz = n0Dbm;

    // --- Emitters ----------------------------------------------------------
    std::vector<Signal> sigs;
    for (const EmitterSignal& e : in.emitters) {
        EmitterNote n;
        n.label = e.label;
        n.frequencyHz = e.frequencyHz;
        n.powerDbm = e.powerDbm;

        if (e.modulation.empty()) {
            n.note = "modulation not configured";
        } else if (!supportsLinearRcPsd(e.modulation)) {
            n.note = "modulation " + e.modulation + " unsupported (not drawn)";
        } else if (!positiveFinite(e.frequencyHz)) {
            n.note = "frequency unavailable";
        } else if (!positiveFinite(e.bandwidthHz)) {
            n.note = "bandwidth not configured";
        } else if (!std::isfinite(e.powerDbm)) {
            n.note = "received level unavailable";
        } else {
            const double rs = e.bandwidthHz / (1.0 + kRollOff);
            const double fraction = rcCumulative(hi - e.frequencyHz, rs, kRollOff) -
                                    rcCumulative(lo - e.frequencyHz, rs, kRollOff);
            n.occupiedBandwidthHz = e.bandwidthHz;
            n.symbolRateHz = rs;
            n.peakPsdDbmHz = e.powerDbm - 10.0 * std::log10(rs);
            n.inSpanFraction = fraction;
            if (fraction <= 1e-12) {
                n.note = "outside monitor span (not drawn)";
            } else {
                n.drawn = true;
                n.note = fraction < 1.0 - 1e-9 ? "partly outside monitor span" : "";
                sigs.push_back({ e.frequencyHz, dbmToMw(e.powerDbm), rs, e.bandwidthHz });
            }
        }
        if (n.drawn) ++r.drawnCount; else ++r.skippedCount;
        r.emitters.push_back(n);
    }
    r.drawnSignals = sigs;
    r.noiseMwPerHz = n0Mw;

    // --- Interference: only sources that overlap the monitor span ----------
    for (const InterfererSignal& j : in.interferers) {
        if (!positiveFinite(j.centreHz) || !positiveFinite(j.bandwidthHz) ||
            !std::isfinite(j.powerDbm))
            continue;
        const double jlo = j.centreHz - 0.5 * j.bandwidthHz;
        const double jhi = j.centreHz + 0.5 * j.bandwidthHz;
        if (std::min(hi, jhi) - std::max(lo, jlo) <= 0.0) continue;
        r.drawnInterferers.push_back({ j.centreHz, j.bandwidthHz, dbmToMw(j.powerDbm) });
    }
    r.interferenceDrawnCount = static_cast<int>(r.drawnInterferers.size());

    // --- Display trace over the whole monitor span ---------------------------
    r.span = makeTrace(lo, hi, kDisplayBins, n0Mw, sigs, r.drawnInterferers);

    // --- Zoom trace around the strongest in-span signal ----------------------
    size_t best = 0;
    double bestPower = -1.0;
    for (size_t k = 0; k < sigs.size(); ++k) {
        const double p = sigs[k].powerMw *
                         (rcCumulative(hi - sigs[k].centreHz, sigs[k].symbolRateHz, kRollOff) -
                          rcCumulative(lo - sigs[k].centreHz, sigs[k].symbolRateHz, kRollOff));
        if (p > bestPower) { bestPower = p; best = k; }
    }
    if (!sigs.empty()) {
        const double half = kZoomHalfWidthInBandwidths * sigs[best].occupiedBwHz;
        const double zlo = std::max(lo, sigs[best].centreHz - half);
        const double zhi = std::min(hi, sigs[best].centreHz + half);
        if (zhi > zlo) {
            r.zoom = makeTrace(zlo, zhi, kZoomBins, n0Mw, sigs, r.drawnInterferers);
            r.hasZoom = true;
            r.zoomCentreHz = sigs[best].centreHz;
        }
    }
    return r;
}

bool windowTrace(const Result& r, double loHz, double hiHz, int bins, Trace& out)
{
    if (!r.available || bins < 2 || !(r.noiseMwPerHz > 0.0))
        return false;

    if (!std::isfinite(loHz) ||
        !std::isfinite(hiHz) ||
        !(hiHz > loHz) ||
        loHz < 0.0)
        return false;

    out = makeTrace(loHz, hiHz, bins, r.noiseMwPerHz, r.drawnSignals, r.drawnInterferers);
    return true;
}

bool axisRange(const Trace& t, double& loDb, double& hiDb)
{
    double mn = std::numeric_limits<double>::infinity();
    double mx = -std::numeric_limits<double>::infinity();
    for (double v : t.psdDbmHz) {
        if (!std::isfinite(v)) continue;
        mn = std::min(mn, v);
        mx = std::max(mx, v);
    }
    if (!std::isfinite(mn) || !std::isfinite(mx)) return false;
    loDb = std::floor((mn - 5.0) / 10.0) * 10.0;
    hiDb = std::ceil((mx + 5.0) / 10.0) * 10.0;
    if (hiDb - loDb < 20.0) hiDb = loDb + 20.0;
    return true;
}

bool sameInput(const Input& a, const Input& b)
{
    if (!numEq(a.monitorCentreHz, b.monitorCentreHz) ||
        !numEq(a.monitorSpanHz, b.monitorSpanHz) ||
        !numEq(a.noiseFigureDb, b.noiseFigureDb) ||
        !numEq(a.temperatureC, b.temperatureC) ||
        a.emitters.size() != b.emitters.size() ||
        a.interferers.size() != b.interferers.size())
        return false;
    for (size_t i = 0; i < a.interferers.size(); ++i) {
        const InterfererSignal& x = a.interferers[i];
        const InterfererSignal& y = b.interferers[i];
        if (!numEq(x.centreHz, y.centreHz) || !numEq(x.bandwidthHz, y.bandwidthHz) ||
            !levelEq(x.powerDbm, y.powerDbm))
            return false;
    }
    for (size_t i = 0; i < a.emitters.size(); ++i) {
        const EmitterSignal& x = a.emitters[i];
        const EmitterSignal& y = b.emitters[i];
        if (x.label != y.label || x.modulation != y.modulation ||
            !numEq(x.frequencyHz, y.frequencyHz) ||
            !numEq(x.bandwidthHz, y.bandwidthHz) ||
            !levelEq(x.powerDbm, y.powerDbm))
            return false;
    }
    return true;
}

std::shared_ptr<const Result> Cache::update(const Input& in)
{
    if (result_ && sameInput(key_, in)) return result_;
    key_ = in;
    result_ = std::make_shared<const Result>(compute(in));
    ++computeCount_;
    return result_;
}

void Cache::clear()
{
    result_.reset();
    key_ = Input();
}

} // namespace spectrum
} // namespace csm
