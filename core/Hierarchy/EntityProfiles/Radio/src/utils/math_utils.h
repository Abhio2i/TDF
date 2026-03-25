#pragma once
// Small math helpers used by the propagation model (src/propagation_model.cpp).
#include <cmath>

namespace radio {
namespace utils {

// Convert dBm to Watts
inline double dbmToWatt(double dbm) {
    return std::pow(10.0, dbm / 10.0) / 1000.0;
}

// Convert Watts to dBm
inline double wattToDbm(double watt) {
    return 10.0 * std::log10(watt * 1000.0);
}

// Free-space path loss in dB given distance (meters) and frequency (Hz)
inline double freeSpacePathLoss(double distance_m, double freq_hz) {
    // 20*log10(4πd/λ) = 20*log10(d) + 20*log10(f) + 20*log10(4π/c)
    // constant = 20*log10(4π/299792458) ≈ -147.55
    return 20.0 * std::log10(distance_m) + 20.0 * std::log10(freq_hz) - 147.55;
}

} // namespace utils
} // namespace radio
