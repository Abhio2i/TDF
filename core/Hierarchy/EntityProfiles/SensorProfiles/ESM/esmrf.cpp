#include "esmrf.h"
#include <algorithm>
#include <cmath>

static const double kNaN = std::numeric_limits<double>::quiet_NaN();

bool RFEmitterParams::isValid() const
{
    return std::isfinite(frequencyMHz) && frequencyMHz > 0.0
           && std::isfinite(bandwidthHz) && bandwidthHz > 0.0
           && std::isfinite(txPowerDbm) && std::isfinite(txGainDbi)
           && std::isfinite(txLossDb) && txLossDb >= 0.0;
}


bool ESMReceiverConfig::isValid() const
{
    const int gm = static_cast<int>(gainMode);

    return
        // Receiver frequency limits: MHz
        std::isfinite(fMinMHz) &&
        std::isfinite(fMaxMHz) &&
        fMinMHz >= 1.0 &&
        fMaxMHz <= 100000.0 &&
        fMaxMHz > fMinMHz &&

        // Receiver bandwidth: Hz; Inspector stores MHz
        std::isfinite(bandwidthHz) &&
        bandwidthHz >= 1.0e3 &&
        bandwidthHz <= 1.0e10 &&

        // Noise and detection thresholds
        std::isfinite(noiseFigureDb) &&
        noiseFigureDb >= 0.0 &&
        noiseFigureDb <= 60.0 &&

        std::isfinite(sensitivityDbm) &&
        sensitivityDbm >= -150.0 &&
        sensitivityDbm <= 0.0 &&

        std::isfinite(minSnrDb) &&
        minSnrDb >= -20.0 &&
        minSnrDb <= 60.0 &&

        // Antenna and propagation losses
        std::isfinite(rxGainDbi) &&
        rxGainDbi >= -30.0 &&
        rxGainDbi <= 60.0 &&

        std::isfinite(rainDbPerKm) &&
        rainDbPerKm >= 0.0 &&
        rainDbPerKm <= 50.0 &&

        std::isfinite(atmosphericDbPerKm) &&
        atmosphericDbPerKm >= 0.0 &&
        atmosphericDbPerKm <= 50.0 &&

        std::isfinite(otherLossDb) &&
        otherLossDb >= 0.0 &&
        otherLossDb <= 100.0 &&

        // Horizon model
        std::isfinite(earthRadiusFactor) &&
        earthRadiusFactor >= 1.0 &&
        earthRadiusFactor <= 2.0 &&

            // Enum and discrete settings
            gm >= 0 && gm <= 2 &&
        spectrumPoints >= 2 &&
        spectrumPoints <= 4096 &&
        averageWindow >= 1 &&
        averageWindow <= 1000;
}

namespace esmrf {

double mwToDbm(double mw) { return 10.0 * std::log10(std::max(mw, 1e-60)); }
double dbmToMw(double dbm) { return std::pow(10.0, dbm / 10.0); }

//Path Loss due to frequency and Distance
double fsplDb(double fMHz, double dKm)
{
    if (!std::isfinite(fMHz) || fMHz <= 0.0 || !std::isfinite(dKm) || dKm < 0.0) return kNaN;
    return 32.44 + 20.0 * std::log10(fMHz) + 20.0 * std::log10(std::max(dKm, kMinDistanceKm));
}

double noisePowerDbm(double bwHz, double nfDb)
{
    if (!std::isfinite(bwHz) || bwHz <= 0.0 || !std::isfinite(nfDb)) return kNaN;
    return kThermalNoiseDbmHz + 10.0 * std::log10(bwHz) + nfDb;
}

double losHorizonKm(double h1m, double h2m, double k)
{
    constexpr double R = 6371000.0;
    const double a = std::sqrt(2.0 * k * R * std::max(h1m, 0.0));
    const double b = std::sqrt(2.0 * k * R * std::max(h2m, 0.0));
    return (a + b) / 1000.0;
}

namespace {
bool effectiveGain(const RFEmitterParams& e, const ESMReceiverConfig& rx, double& gt)
{
    gt = e.txGainDbi;
    switch (rx.gainMode) {
    case GainMode::MainBeam: return true;
    case GainMode::PeakSidelobe:    if (!std::isfinite(e.sidelobePeakDb)) return false; gt += e.sidelobePeakDb; return true;
    case GainMode::AverageSidelobe: if (!std::isfinite(e.sidelobeAvgDb))  return false; gt += e.sidelobeAvgDb;  return true;
    }
    return false;
}

double totalLossDb(double fMHz, double dKm, const ESMReceiverConfig& rx)
{
    return fsplDb(fMHz, dKm) + (rx.rainDbPerKm + rx.atmosphericDbPerKm) * dKm + rx.otherLossDb;
}
} // namespace

RFLink computeLink(const RFEmitterParams& e, double dKm, const ESMReceiverConfig& rx, double horizonKm)
{
    RFLink L;
    double gt = 0.0;
    if (!e.isValid() || !rx.isValid() || !std::isfinite(dKm) || dKm < 0.0 || !effectiveGain(e, rx, gt)) {
        L.reason = LinkReason::InvalidInput; return L;
    }
    if (e.frequencyMHz < rx.fMinMHz || e.frequencyMHz > rx.fMaxMHz) { L.reason = LinkReason::OutOfBand; return L; }
    if (horizonKm > 0.0 && dKm > horizonKm)                       { L.reason = LinkReason::BeyondHorizon; return L; }

    L.pathLossDb = totalLossDb(e.frequencyMHz, dKm, rx);
    L.rxPowerDbm = e.txPowerDbm + gt - e.txLossDb + rx.rxGainDbi - L.pathLossDb;

    // Flat-spectrum assumption: only Brx/Bem of the emitter power is inside the channel.
    const double frac = std::min(1.0, rx.bandwidthHz / e.bandwidthHz);
    L.channelPowerDbm = L.rxPowerDbm + 10.0 * std::log10(frac);
    L.noiseDbm = noisePowerDbm(rx.bandwidthHz, rx.noiseFigureDb);
    L.snrDb = L.channelPowerDbm - L.noiseDbm;       // same (receiver) bandwidth basis

    //Detection conditions
    if (L.channelPowerDbm < rx.sensitivityDbm)            L.reason = LinkReason::BelowSensitivity;
    else if (rx.useSnrThreshold && L.snrDb < rx.minSnrDb) L.reason = LinkReason::BelowSnr;
    else { L.reason = LinkReason::Detected; L.detected = true; }
    return L;
}

double maxRangeKm(const RFEmitterParams& e, const ESMReceiverConfig& rx)
{
    double gt = 0.0;
    if (!e.isValid() || !rx.isValid() || !effectiveGain(e, rx, gt)) return kNaN;
    if (e.frequencyMHz < rx.fMinMHz || e.frequencyMHz > rx.fMaxMHz) return kNaN;

    const double frac = std::min(1.0, rx.bandwidthHz / e.bandwidthHz);
    double reqChannel = rx.sensitivityDbm;
    if (rx.useSnrThreshold)
        reqChannel = std::max(reqChannel, noisePowerDbm(rx.bandwidthHz, rx.noiseFigureDb) + rx.minSnrDb);
    const double reqRx = reqChannel - 10.0 * std::log10(frac);

    auto margin = [&](double d) {
        return e.txPowerDbm + gt - e.txLossDb + rx.rxGainDbi - totalLossDb(e.frequencyMHz, d, rx) - reqRx;
    };
    double lo = kMinDistanceKm, hi = 1.0e5;
    if (margin(lo) < 0.0) return 0.0;
    if (margin(hi) > 0.0) return hi;
    for (int i = 0; i < 80; ++i) { const double mid = 0.5 * (lo + hi); (margin(mid) >= 0.0 ? lo : hi) = mid; }
    return lo;
}

bool synthesizeSpectrum(const ESMReceiverConfig& rx, const QVector<SpectrumContribution>& em,
                        QVector<double>& freq, QVector<double>& psd)
{
    if (!rx.isValid()) return false;
    const int n = rx.spectrumPoints;
    const double dfMHz = (rx.fMaxMHz - rx.fMinMHz) / (n - 1);
    const double noisePsd = dbmToMw(kThermalNoiseDbmHz + rx.noiseFigureDb);  // mW/Hz

    freq.resize(n);
    psd.fill(noisePsd, n);
    for (int i = 0; i < n; ++i) freq[i] = rx.fMinMHz + i * dfMHz;

    for (const SpectrumContribution& c : em) {
        if (!std::isfinite(c.freqMHz) || !std::isfinite(c.rxPowerDbm) || !(c.bandwidthHz > 0.0)) continue;
        const double bwMHz = c.bandwidthHz / 1.0e6;
        const double lo = c.freqMHz - bwMHz / 2.0, hi = c.freqMHz + bwMHz / 2.0;
        const double pMw = dbmToMw(c.rxPowerDbm);
        const int i0 = std::max(0,     int(std::floor((lo - rx.fMinMHz) / dfMHz)) - 1);
        const int i1 = std::min(n - 1, int(std::ceil ((hi - rx.fMinMHz) / dfMHz)) + 1);
        for (int i = i0; i <= i1; ++i) {
            const double overlap = std::min(hi, freq[i] + dfMHz / 2.0) - std::max(lo, freq[i] - dfMHz / 2.0);
            if (overlap <= 0.0) continue;
            psd[i] += (pMw * overlap / bwMHz) / (dfMHz * 1.0e6);   // power-conserving, mW/Hz
        }
    }
    return true;
}

void SpectrumAccumulator::reset() { count_ = 0; freq_.clear(); psd_.clear(); }

void SpectrumAccumulator::update(const QVector<double>& f, const QVector<double>& p)
{
    if (f.size() != p.size() || f.size() < 2) { reset(); return; }
    const bool same = (freq_.size() == f.size())
                      && std::fabs(freq_.first() - f.first()) < 1e-9 && std::fabs(freq_.last() - f.last()) < 1e-9;
    if (!same) reset();

    if (psd_.isEmpty() || mode_ == SpectrumMode::Live) { freq_ = f; psd_ = p; count_ = 1; return; }
    if (mode_ == SpectrumMode::MaxHold) {
        for (int i = 0; i < psd_.size(); ++i) psd_[i] = std::max(psd_[i], p[i]);
    } else {  // Average, linear power domain
        count_ = std::min(count_ + 1, window_);
        for (int i = 0; i < psd_.size(); ++i) psd_[i] += (p[i] - psd_[i]) / count_;
    }
}

QVector<double> SpectrumAccumulator::psdDbmPerHz() const
{
    QVector<double> out(psd_.size());
    for (int i = 0; i < psd_.size(); ++i) out[i] = mwToDbm(psd_[i]);
    return out;
}

} // namespace esmrf
