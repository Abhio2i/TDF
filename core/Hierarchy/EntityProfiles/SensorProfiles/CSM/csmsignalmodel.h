// // CSMSignalModel.h - received power and thermal noise calculations.
// // Units: power dBm, gain/loss dB, bandwidth Hz, temperature Celsius.
// //
// // MODEL (scalar link budget; same relationships as the radio engine's
// // evaluate_link(), without polarization loss). Results are CALCULATED model
// // values, not measurements, and are NOT a power spectral density:
// //
// //   P_rx[dBm]  = P_tx - L_degradation + 10log10(duty) (0<duty<1 only)
// //                + G_tx[dBi] + G_rx[dBi] - L_path[dB]
// //   N[dBm]     = -174 + 10log10(T_K/290) + 10log10(B_Hz) + NF[dB]
// //                (T_K clamped to 173.15..373.15 K as in the radio engine)
// //   SNR[dB]    = P_rx - N     (only when both are valid)
// //
// // L_path comes from the existing propagation model (PropagationModel::
// // pathLossDb), which includes its fixed override and stochastic terms when
// // enabled. NOT modelled: polarization mismatch, antenna pattern/beam gating
// // toward the observer, receiver filter shape, partial-band overlap power,
// // multipath, Doppler on level, interference (unless in the propagation model).

// #pragma once
// namespace csm {
// namespace signal {

// // Calculate received signal power in dBm.
// // Returns NaN when required inputs are unavailable or invalid.
// double receivedPowerDbm(
//     double txPowerDbm,
//     double txPowerDegradationDb,
//     double txDutyCycle,
//     double txAntennaGainDbi,
//     double rxGainDbi,
//     double pathLossDb);

// // Calculate thermal noise floor in dBm.
// // Bandwidth is in Hz, temperature is in Celsius,
// // and noise figure is in dB.
// double thermalNoiseFloorDbm(
//     double bandwidthHz,
//     double noiseFigureDb,
//     double temperatureC);

// } // namespace signal
// } // namespace csm




// CSMSignalModel.h - received power and thermal noise calculations.
// Units: power dBm, gain/loss dB, bandwidth Hz, temperature Celsius.
//
// MODEL (scalar link budget; same relationships as the radio engine's
// evaluate_link(), without polarization loss). Results are CALCULATED model
// values, not measurements, and are NOT a power spectral density:
//
//   P_rx[dBm]  = P_tx - L_degradation + 10log10(duty) (0<duty<1 only)
//                + G_tx[dBi] + G_rx[dBi] - L_path[dB]
//   N[dBm]     = -174 + 10log10(T_K/290) + 10log10(B_Hz) + NF[dB]
//                (T_K clamped to 173.15..373.15 K as in the radio engine)
//   SNR[dB]    = P_rx - N     (only when both are valid)
//
// L_path comes from the existing propagation model (PropagationModel::
// pathLossDb), which includes its fixed override and stochastic terms when
// enabled. NOT modelled: polarization mismatch, antenna pattern/beam gating
// toward the observer, receiver filter shape, partial-band overlap power,
// multipath, Doppler on level, interference (unless in the propagation model).

#pragma once
#include "csmtypes.h"
namespace csm {
namespace signal {

// Calculate received signal power in dBm.
// Returns NaN when required inputs are unavailable or invalid.
double receivedPowerDbm(
    double txPowerDbm,
    double txPowerDegradationDb,
    double txDutyCycle,
    double txAntennaGainDbi,
    double rxGainDbi,
    double pathLossDb);

// Calculate thermal noise floor in dBm.
// Bandwidth is in Hz, temperature is in Celsius,
// and noise figure is in dB.
double thermalNoiseFloorDbm(
    double bandwidthHz,
    double noiseFigureDb,
    double temperatureC);

// Build an interference source from TRANSMIT-mode configuration (GHz / MHz /
// dBm / dBi). Returns false (and leaves `out` untouched) when the CSM is not
// transmitting or any value is non-finite / non-positive where required.
bool makeInterferer(bool transmitting, double freqGhz, double bandwidthMhz,
                    double txPowerDbm, double txGainDbi, InterfererInput& out);

// dBm <-> mW (absolute power only; never use on dB ratios).
double dbmToMw(double dbm);
double mwToDbm(double mw);       // NaN for mw <= 0 or non-finite

// Length of the intersection of [aLo,aHi] and [bLo,bHi] [Hz]; 0 if disjoint.
double overlapHz(double aLo, double aHi, double bLo, double bHi);

// Interference power [mW] from one source inside [bandLoHz, bandHiHz] at a
// receiver with gain rxGainDbi. Returns 0 when the source is out of coverage
// or does not overlap the band; NaN when its parameters are invalid.
double interferenceInBandMw(const InterfererInput& j, double rxGainDbi,
                            double bandLoHz, double bandHiHz);

// Sum of interferenceInBandMw over all sources (invalid sources are skipped).
double totalInterferenceInBandMw(const std::vector<InterfererInput>& js,
                                 double rxGainDbi, double bandLoHz, double bandHiHz);

// S/(N+I) in dB from absolute powers. interferenceMw may be 0.
// NaN when signal or noise is unavailable.
double sinrDb(double signalDbm, double noiseDbm, double interferenceMw);

} // namespace signal
} // namespace csm
