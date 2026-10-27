// #include "csmsignalmodel.h"
// #include "csmtypes.h"
// #include <algorithm>
// #include <cmath>

// namespace csm {
// namespace signal {

// double receivedPowerDbm(
//     double txPowerDbm,
//     double txDegradationDb,
//     double txDutyCycle,
//     double txGainDbi,
//     double rxGainDbi,
//     double pathLossDb)
// {
//     if (!std::isfinite(txPowerDbm) ||
//         !std::isfinite(txGainDbi) ||
//         !std::isfinite(rxGainDbi) ||
//         !std::isfinite(pathLossDb) ||
//         !std::isfinite(txDegradationDb)) {
//         return kNaN;
//     }

//     double p = txPowerDbm - txDegradationDb;

//     if (std::isfinite(txDutyCycle) &&
//         txDutyCycle > 0.0 &&
//         txDutyCycle < 1.0) {
//         p += 10.0 * std::log10(txDutyCycle);
//     }

//     return p + txGainDbi + rxGainDbi - pathLossDb;
// }

// double thermalNoiseFloorDbm(
//     double bandwidthHz,
//     double noiseFigureDb,
//     double temperatureC)
// {
//     if (!std::isfinite(bandwidthHz) ||
//         bandwidthHz <= 0.0 ||
//         !std::isfinite(noiseFigureDb) ||
//         !std::isfinite(temperatureC)) {
//         return kNaN;
//     }

//     const double tK = std::clamp(
//         temperatureC + 273.15,
//         173.15,
//         373.15);

//     return -174.0
//            + 10.0 * std::log10(tK / 290.0)
//            + 10.0 * std::log10(bandwidthHz)
//            + noiseFigureDb;
// }

// } // namespace signal
// } // namespace csm














#include "csmsignalmodel.h"
#include "csmtypes.h"
#include <algorithm>
#include <cmath>

namespace csm {
namespace signal {

double receivedPowerDbm(
    double txPowerDbm,
    double txDegradationDb,
    double txDutyCycle,
    double txGainDbi,
    double rxGainDbi,
    double pathLossDb)
{
    if (!std::isfinite(txPowerDbm) ||
        !std::isfinite(txGainDbi) ||
        !std::isfinite(rxGainDbi) ||
        !std::isfinite(pathLossDb) ||
        !std::isfinite(txDegradationDb)) {
        return kNaN;
    }

    double p = txPowerDbm - txDegradationDb;

    if (std::isfinite(txDutyCycle) &&
        txDutyCycle > 0.0 &&
        txDutyCycle < 1.0) {
        p += 10.0 * std::log10(txDutyCycle);
    }

    return p + txGainDbi + rxGainDbi - pathLossDb;
}

double thermalNoiseFloorDbm(
    double bandwidthHz,
    double noiseFigureDb,
    double temperatureC)
{
    if (!std::isfinite(bandwidthHz) ||
        bandwidthHz <= 0.0 ||
        !std::isfinite(noiseFigureDb) ||
        !std::isfinite(temperatureC)) {
        return kNaN;
    }

    const double tK = std::clamp(
        temperatureC + 273.15,
        173.15,
        373.15);

    return -174.0
           + 10.0 * std::log10(tK / 290.0)
           + 10.0 * std::log10(bandwidthHz)
           + noiseFigureDb;
}

bool makeInterferer(bool transmitting, double freqGhz, double bandwidthMhz,
                    double txPowerDbm, double txGainDbi, InterfererInput& out)
{
    if (!transmitting) return false;
    if (!std::isfinite(freqGhz) || freqGhz <= 0.0) return false;
    if (!std::isfinite(bandwidthMhz) || bandwidthMhz <= 0.0) return false;
    if (!std::isfinite(txPowerDbm) || !std::isfinite(txGainDbi)) return false;
    out = InterfererInput();
    out.frequencyHz = freqGhz * 1.0e9;
    out.bandwidthHz = bandwidthMhz * 1.0e6;
    out.txPowerDbm = txPowerDbm;
    out.txGainDbi = txGainDbi;
    return true;
}

double dbmToMw(double dbm) { return std::pow(10.0, dbm / 10.0); }

double mwToDbm(double mw)
{
    if (!std::isfinite(mw) || mw <= 0.0) return kNaN;
    return 10.0 * std::log10(mw);
}

double overlapHz(double aLo, double aHi, double bLo, double bHi)
{
    return std::max(0.0, std::min(aHi, bHi) - std::max(aLo, bLo));
}

double interferenceInBandMw(const InterfererInput& j, double rxGainDbi,
                            double bandLoHz, double bandHiHz)
{
    if (!j.inCoverage) return 0.0;
    if (!std::isfinite(j.frequencyHz) || j.frequencyHz <= 0.0 ||
        !std::isfinite(j.bandwidthHz) || j.bandwidthHz <= 0.0)
        return kNaN;
    // Jammer: no degradation/duty terms; link budget as for the desired signal.
    const double prx = receivedPowerDbm(j.txPowerDbm, 0.0, 1.0, j.txGainDbi,
                                        rxGainDbi, j.pathLossDb);
    if (!std::isfinite(prx)) return kNaN;
    const double half = 0.5 * j.bandwidthHz;
    const double ov = overlapHz(bandLoHz, bandHiHz, j.frequencyHz - half, j.frequencyHz + half);
    if (ov <= 0.0) return 0.0;
    return dbmToMw(prx) * (ov / j.bandwidthHz);
}

double totalInterferenceInBandMw(const std::vector<InterfererInput>& js,
                                 double rxGainDbi, double bandLoHz, double bandHiHz)
{
    double sum = 0.0;
    for (const InterfererInput& j : js) {
        const double mw = interferenceInBandMw(j, rxGainDbi, bandLoHz, bandHiHz);
        if (std::isfinite(mw)) sum += mw;     // linear aggregation
    }
    return sum;
}

double sinrDb(double signalDbm, double noiseDbm, double interferenceMw)
{
    if (!std::isfinite(signalDbm) || !std::isfinite(noiseDbm)) return kNaN;
    const double i = (std::isfinite(interferenceMw) && interferenceMw > 0.0) ? interferenceMw : 0.0;
    return signalDbm - mwToDbm(dbmToMw(noiseDbm) + i);
}

} // namespace signal
} // namespace csm
