// // csmspectrum.h - MODELLED CSM frequency spectrum (Qt-free, pure logic).
// //
// // WHAT THIS IS
// //   A deterministic, analytic model of what a CSM monitor receiver would see:
// //   thermal noise plus the power spectral density (PSD) of every detected
// //   radio whose modulation is supported by the linear raised-cosine model
// //   (BPSK, QPSK, and other constant-envelope / linear digital schemes listed
// //   below). It is NOT a measurement and NOT an FFT of generated samples.
// //   Every result carries the label
// //   "MODELLED / SIMULATED - NOT MEASURED".
// //
// // MODEL (units: power dBm / mW, PSD dBm/Hz, frequency Hz, temperature Celsius)
// //   Signal  : Linear digital modulation with root-raised-cosine (RRC) pulse
// //             shaping, roll-off beta. Supported scheme names (exact match of
// //             the configured radio modulation string):
// //               BPSK, QPSK, PSK8, QAM16, QAM64
// //             The continuous-time PSD shape for random independent symbols
// //             with the same RRC pulse is identical for these schemes; only
// //             the bits-per-symbol differs. The configured radio bandwidth B
// //             is taken as the occupied RF bandwidth:
// //                 B = Rs (1 + beta)   =>   Rs = B / (1 + beta)
// //             beta = kRollOff = 0.35 is an ASSUMPTION (not a radio setting).
// //             The PSD is proportional to |H_rrc(f)|^2 (raised-cosine shape),
// //             normalised so that
// //                 integral S(f) df = P_rx          (received power, mW)
// //             Flat part:  S = P / Rs            for |f - fc| <= (1-beta) Rs/2
// //             Roll-off:   S = P/(2Rs) [1 + cos(pi (|f-fc| - (1-beta)Rs/2) / (beta Rs))]
// //             Zero beyond |f - fc| >= (1+beta) Rs/2 = B/2.
// //             The cumulative power has a closed form, so power in ANY
// //             frequency interval is exact (no sampling, no FFT, no grid).
// //             ASSUMPTION: the radio's configured bandwidth is the occupied
// //             bandwidth after pulse shaping; data rate is NOT used as symbol
// //             rate. Real transmitter filters may differ; this model does not
// //             claim to match a specific hardware filter response.
// //   Noise   : expected thermal PSD  N0 = kT * NF, evaluated with the existing
// //             csm::signal::thermalNoiseFloorDbm() for a 1 Hz bandwidth:
// //                 N0[dBm/Hz] = -174 + 10log10(T_K/290) + NF[dB]
// //             The EXPECTED (mean) PSD is shown; no random noise realisation
// //             is generated.
// //   Level   : P_rx is the existing calculated link-budget level
// //             (Observation::levelDbm). No propagation maths is repeated here.
// //
// // TWO VIEWS, ONE MODEL (internal resolution != display resolution)
// //   span : kDisplayBins bins across the configured monitor span. Each bin holds
// //          PSD_bin = (power in bin) / (bin width), where the power is the exact
// //          integral of the model over the bin. Power is conserved; a narrow
// //          signal is NOT stretched or boosted, so its bin PSD is lower than its
// //          true peak PSD when the bin is wider than the signal.
// //   zoom : kZoomBins bins over +/- one occupied bandwidth around the strongest
// //          drawn signal (clipped to the monitor span). Fine resolution is only
// //          spent where the signal is.
// //   The true signal peak PSD (P/Rs) is reported separately per emitter.
// //
// // NOT MODELLED: AM/FM/FSK/GMSK/OFDM and any other unsupported scheme
// // (reported as unsupported), spread spectrum, receiver filter shape,
// // adjacent-channel leakage of real hardware, Doppler, multipath,
// // interference, phase noise, spurs.

// #pragma once

// #include <limits>
// #include <memory>
// #include <string>
// #include <vector>

// namespace csm {
// namespace spectrum {

// constexpr double kRollOff = 0.35;               // assumed RRC roll-off
// constexpr int    kDisplayBins = 512;            // bins across monitor span
// constexpr int    kZoomBins = 256;               // bins across zoom window
// constexpr double kZoomHalfWidthInBandwidths = 1.0;
// constexpr double kPowerCacheToleranceDb = 0.01; // level change ignored by cache

// inline const char* provenanceLabel() { return "MODELLED / SIMULATED - NOT MEASURED"; }

// // One detected radio, as configured/calculated by the existing pipeline.
// // NaN / empty means "not available".
// struct EmitterSignal {
//     std::string label;          // e.g. "Platform/Radio"
//     double frequencyHz = std::numeric_limits<double>::quiet_NaN();
//     double bandwidthHz = std::numeric_limits<double>::quiet_NaN();
//     double powerDbm    = std::numeric_limits<double>::quiet_NaN(); // received (calculated)
//     std::string modulation;     // configured scheme name, e.g. "BPSK"
// };

// struct Input {
//     double monitorCentreHz = std::numeric_limits<double>::quiet_NaN();
//     double monitorSpanHz   = 0.0;   // 0 = tuned point (no span)
//     double noiseFigureDb   = std::numeric_limits<double>::quiet_NaN();
//     double temperatureC    = std::numeric_limits<double>::quiet_NaN();
//     std::vector<EmitterSignal> emitters;
// };

// // Uniform-bin PSD trace. Bin i covers [startHz + i*binHz, startHz + (i+1)*binHz).
// struct Trace {
//     double startHz = 0.0;
//     double binHz = 0.0;
//     std::vector<double> psdDbmHz;
//     double stopHz() const { return startHz + binHz * static_cast<double>(psdDbmHz.size()); }
//     double binCentreHz(int i) const { return startHz + binHz * (static_cast<double>(i) + 0.5); }
// };

// struct EmitterNote {
//     std::string label;
//     bool drawn = false;
//     std::string note;               // why it is not drawn, or a short remark
//     double frequencyHz = 0.0;
//     double powerDbm = 0.0;
//     double occupiedBandwidthHz = 0.0;
//     double symbolRateHz = 0.0;
//     double peakPsdDbmHz = 0.0;      // true signal peak PSD = P / Rs (not a bin value)
//     double inSpanFraction = 0.0;    // share of signal power inside the monitor span
// };


// struct DrawnSignal {
//     double centreHz;
//     double powerMw;
//     double symbolRateHz;
//     double occupiedBwHz;
// };



// struct Result {
//     bool available = false;
//     std::string reason;             // set when !available
//     Trace span;
//     Trace zoom;
//     bool hasZoom = false;
//     double zoomCentreHz = 0.0;
//     double noisePsdDbmHz = 0.0;
//     double rollOff = kRollOff;
//     int drawnCount = 0;
//     int skippedCount = 0;
//     std::vector<EmitterNote> emitters;
//     std::vector<DrawnSignal> drawnSignals;
//     double noiseMwPerHz = 0.0;
// };

// // Closed-form cumulative power fraction of the unit-power raised-cosine PSD
// // below carrier offset x [Hz] (0 at -inf, 0.5 at x = 0, 1 at +inf).
// double rcCumulative(double offsetHz, double symbolRateHz, double rollOff);

// // Compute the modelled spectrum. Pure function: same input -> identical output.
// Result compute(const Input& in);
// bool windowTrace(const Result& r,double loHz,double hiHz,int bins,Trace& out);

// // Suggested Y-axis range [dBm/Hz] (multiples of 10 dB, at least 20 dB tall).
// // Returns false when the trace is empty or has no finite values.
// bool axisRange(const Trace& t, double& loDb, double& hiDb);

// // Caches the last result; recomputes only if a relevant input changed.
// // Level changes below kPowerCacheToleranceDb do not invalidate the cache.
// class Cache {
// public:
//     std::shared_ptr<const Result> update(const Input& in);
//     std::shared_ptr<const Result> result() const { return result_; }
//     void clear();
//     int computeCount() const { return computeCount_; }

// private:
//     Input key_;
//     std::shared_ptr<const Result> result_;
//     int computeCount_ = 0;
// };

// // True if both inputs describe the same spectrum (NaN == NaN; levels compared
// // within kPowerCacheToleranceDb). Exposed for tests.
// bool sameInput(const Input& a, const Input& b);

// } // namespace spectrum
// } // namespace csm















// csmspectrum.h - MODELLED CSM frequency spectrum (Qt-free, pure logic).
//
// WHAT THIS IS
//   A deterministic, analytic model of what a CSM monitor receiver would see:
//   thermal noise plus the power spectral density (PSD) of every detected
//   radio whose modulation is supported by the linear raised-cosine model
//   (BPSK, QPSK, and other constant-envelope / linear digital schemes listed
//   below). It is NOT a measurement and NOT an FFT of generated samples.
//   Every result carries the label
//   "MODELLED / SIMULATED - NOT MEASURED".
//
// MODEL (units: power dBm / mW, PSD dBm/Hz, frequency Hz, temperature Celsius)
//   Signal  : Linear digital modulation with root-raised-cosine (RRC) pulse
//             shaping, roll-off beta. Supported scheme names (exact match of
//             the configured radio modulation string):
//               BPSK, QPSK, PSK8, QAM16, QAM64
//             The continuous-time PSD shape for random independent symbols
//             with the same RRC pulse is identical for these schemes; only
//             the bits-per-symbol differs. The configured radio bandwidth B
//             is taken as the occupied RF bandwidth:
//                 B = Rs (1 + beta)   =>   Rs = B / (1 + beta)
//             beta = kRollOff = 0.35 is an ASSUMPTION (not a radio setting).
//             The PSD is proportional to |H_rrc(f)|^2 (raised-cosine shape),
//             normalised so that
//                 integral S(f) df = P_rx          (received power, mW)
//             Flat part:  S = P / Rs            for |f - fc| <= (1-beta) Rs/2
//             Roll-off:   S = P/(2Rs) [1 + cos(pi (|f-fc| - (1-beta)Rs/2) / (beta Rs))]
//             Zero beyond |f - fc| >= (1+beta) Rs/2 = B/2.
//             The cumulative power has a closed form, so power in ANY
//             frequency interval is exact (no sampling, no FFT, no grid).
//             ASSUMPTION: the radio's configured bandwidth is the occupied
//             bandwidth after pulse shaping; data rate is NOT used as symbol
//             rate. Real transmitter filters may differ; this model does not
//             claim to match a specific hardware filter response.
//   Noise   : expected thermal PSD  N0 = kT * NF, evaluated with the existing
//             csm::signal::thermalNoiseFloorDbm() for a 1 Hz bandwidth:
//                 N0[dBm/Hz] = -174 + 10log10(T_K/290) + NF[dB]
//             The EXPECTED (mean) PSD is shown; no random noise realisation
//             is generated.
//   Level   : P_rx is the existing calculated link-budget level
//             (Observation::levelDbm). No propagation maths is repeated here.
//
// TWO VIEWS, ONE MODEL (internal resolution != display resolution)
//   span : kDisplayBins bins across the configured monitor span. Each bin holds
//          PSD_bin = (power in bin) / (bin width), where the power is the exact
//          integral of the model over the bin. Power is conserved; a narrow
//          signal is NOT stretched or boosted, so its bin PSD is lower than its
//          true peak PSD when the bin is wider than the signal.
//   zoom : kZoomBins bins over +/- one occupied bandwidth around the strongest
//          drawn signal (clipped to the monitor span). Fine resolution is only
//          spent where the signal is.
//   The true signal peak PSD (P/Rs) is reported separately per emitter.
//
//   Interference (added): each jammer is a barrage-noise source with a FLAT PSD
//             over its bandwidth B_j centred on f_j, carrying the received
//             power P_j [mW] (link budget computed by the caller):
//                 S_j(f) = P_j / B_j   for |f - f_j| <= B_j/2, else 0.
//             Bin power is the exact integral, summed in LINEAR mW with the
//             signals and noise before conversion to dBm/Hz. Interferers are
//             ANONYMOUS here: no label, id, range or bearing is carried.
//
// NOT MODELLED: AM/FM/FSK/GMSK/OFDM and any other unsupported scheme
// (reported as unsupported), spread spectrum, receiver filter shape,
// adjacent-channel leakage of real hardware, Doppler, multipath,
// jammer waveform detail (hopping, pulsing, adaptation), phase noise, spurs.

#pragma once

#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace csm {
namespace spectrum {

constexpr double kRollOff = 0.35;               // assumed RRC roll-off
constexpr int    kDisplayBins = 512;            // bins across monitor span
constexpr int    kZoomBins = 256;               // bins across zoom window
constexpr double kZoomHalfWidthInBandwidths = 1.0;
constexpr double kPowerCacheToleranceDb = 0.01; // level change ignored by cache

inline const char* provenanceLabel() { return "MODELLED / SIMULATED - NOT MEASURED"; }

// One detected radio, as configured/calculated by the existing pipeline.
// NaN / empty means "not available".
struct EmitterSignal {
    std::string label;          // e.g. "Platform/Radio"
    double frequencyHz = std::numeric_limits<double>::quiet_NaN();
    double bandwidthHz = std::numeric_limits<double>::quiet_NaN();
    double powerDbm    = std::numeric_limits<double>::quiet_NaN(); // received (calculated)
    std::string modulation;     // configured scheme name, e.g. "BPSK"
};

// One interference source at the receiver (identity-free). powerDbm is the
// total power received over the whole bandwidth.
struct InterfererSignal {
    double centreHz = std::numeric_limits<double>::quiet_NaN();
    double bandwidthHz = std::numeric_limits<double>::quiet_NaN();
    double powerDbm = std::numeric_limits<double>::quiet_NaN();
};

struct Input {
    double monitorCentreHz = std::numeric_limits<double>::quiet_NaN();
    double monitorSpanHz   = 0.0;   // 0 = tuned point (no span)
    double noiseFigureDb   = std::numeric_limits<double>::quiet_NaN();
    double temperatureC    = std::numeric_limits<double>::quiet_NaN();
    std::vector<EmitterSignal> emitters;
    std::vector<InterfererSignal> interferers;   // anonymous; may be empty
};

// Uniform-bin PSD trace. Bin i covers [startHz + i*binHz, startHz + (i+1)*binHz).
struct Trace {
    double startHz = 0.0;
    double binHz = 0.0;
    std::vector<double> psdDbmHz;
    double stopHz() const { return startHz + binHz * static_cast<double>(psdDbmHz.size()); }
    double binCentreHz(int i) const { return startHz + binHz * (static_cast<double>(i) + 0.5); }
};

struct EmitterNote {
    std::string label;
    bool drawn = false;
    std::string note;               // why it is not drawn, or a short remark
    double frequencyHz = 0.0;
    double powerDbm = 0.0;
    double occupiedBandwidthHz = 0.0;
    double symbolRateHz = 0.0;
    double peakPsdDbmHz = 0.0;      // true signal peak PSD = P / Rs (not a bin value)
    double inSpanFraction = 0.0;    // share of signal power inside the monitor span
};


struct DrawnSignal {
    double centreHz;
    double powerMw;
    double symbolRateHz;
    double occupiedBwHz;
};



// A drawn interference band (flat PSD), reduced to what the integral needs.
struct InterferenceBand {
    double centreHz;
    double bandwidthHz;
    double powerMw;
};

struct Result {
    bool available = false;
    std::string reason;             // set when !available
    Trace span;
    Trace zoom;
    bool hasZoom = false;
    double zoomCentreHz = 0.0;
    double noisePsdDbmHz = 0.0;
    double rollOff = kRollOff;
    int drawnCount = 0;
    int skippedCount = 0;
    std::vector<EmitterNote> emitters;
    std::vector<DrawnSignal> drawnSignals;
    double noiseMwPerHz = 0.0;
    std::vector<InterferenceBand> drawnInterferers;
    int interferenceDrawnCount = 0;   // interferers overlapping the monitor span
};

// Closed-form cumulative power fraction of the unit-power raised-cosine PSD
// below carrier offset x [Hz] (0 at -inf, 0.5 at x = 0, 1 at +inf).
double rcCumulative(double offsetHz, double symbolRateHz, double rollOff);

// Compute the modelled spectrum. Pure function: same input -> identical output.
Result compute(const Input& in);
bool windowTrace(const Result& r,double loHz,double hiHz,int bins,Trace& out);

// Suggested Y-axis range [dBm/Hz] (multiples of 10 dB, at least 20 dB tall).
// Returns false when the trace is empty or has no finite values.
bool axisRange(const Trace& t, double& loDb, double& hiDb);

// Caches the last result; recomputes only if a relevant input changed.
// Level changes below kPowerCacheToleranceDb do not invalidate the cache.
class Cache {
public:
    std::shared_ptr<const Result> update(const Input& in);
    std::shared_ptr<const Result> result() const { return result_; }
    void clear();
    int computeCount() const { return computeCount_; }

private:
    Input key_;
    std::shared_ptr<const Result> result_;
    int computeCount_ = 0;
};

// True if both inputs describe the same spectrum (NaN == NaN; levels compared
// within kPowerCacheToleranceDb). Exposed for tests.
bool sameInput(const Input& a, const Input& b);

} // namespace spectrum
} // namespace csm
