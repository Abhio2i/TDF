#pragma once
// Shared data types for RadioLib.
// Used by both the public API (include/radio/radio_interface.h) and the
// internal implementation (src/radiolib.cpp, src/propagation_model.cpp).
#include <cstdint>
#include <string>

namespace radio {

class Radiolib;

// 2D position with altitude (meters)
struct Position {
    double x = 0.0;
    double y = 0.0;
    double altitude = 0.0;   // above ground/sea level
};

// Radio operational mode
enum class RadioMode {
    RECEIVER_ONLY,
    TRANSMITTER_ONLY,
    TRANSCEIVER
};

// Communication mode (propagation model selector)
enum class CommsMode {
    LINE_OF_SIGHT,
    BEYOND_LINE_OF_SIGHT,
    SATCOM,
    TROPOSCATTER
};

// Spread spectrum type
enum class SpreadSpectrum {
    NONE,
    FHSS,      // Frequency Hopping Spread Spectrum
    DSSS       // Direct Sequence Spread Spectrum
};

// Modulation class
enum class ModulationClass {
    AM,
    FM,
    PSK,
    QAM
};

// Specific modulation schemes (extend as needed)
enum class ModulationScheme {
    AM,
    FM,
    BPSK,
    QPSK,
    PSK8,
    QAM16,
    QAM64,
    FSK2,
    FSK4,
    GMSK,
    OFDM_BPSK,
    OFDM_QPSK,
    OFDM_QAM16,
    OFDM_QAM64
};

// Encryption type
enum class EncryptionType {
    NONE,
    AES,
    DES
};

// Polarization
enum class Polarization {
    VERTICAL,
    HORIZONTAL,
    CIRCULAR_LEFT,
    CIRCULAR_RIGHT
};

// Duplex mode
enum class DuplexMode {
    SIMPLEX,
    HALF_DUPLEX,
    FULL_DUPLEX
};

// Antenna scan type
enum class ScanType {
    FIXED,
    SECTOR_SCAN,
    CONICAL_SCAN
};

// Scan result record returned by Radiolib::radiolibscan().
// Filled in PropagationModelImpl::radiolibscan() (src/propagation_model.cpp).
struct ScanHit {
    Radiolib* radio = nullptr;
    std::string id;
    std::string target_platform_name;

    double distance_m = 0.0;
    double radius_m = 0.0;
    double azimuth_deg = 0.0;

    double scanner_max_range_m = 0.0;
    double scanner_frequency_hz = 0.0;

    double path_loss_db = 0.0;
    double rx_power_dbm = 0.0;
    double noise_floor_dbm = 0.0;
    double snr_db = 0.0;
    double rain_attenuation_db = 0.0;
    double wind_attenuation_db_per_km = 0.0;
    double los_horizon_distance_m = 0.0;
    double polarization_loss_db = 0.0;
    double required_snr_threshold_db = 0.0;

    bool network_match = false;
    bool frequency_match = false;
    bool range_ok = false;
    bool beam_ok = false;
    bool sensitivity_ok = false;
    bool squelch_ok = false;
    bool snr_ok = false;

    // New: transmitter-only scan result can return a footprint record
    bool is_footprint = false;
    double heading_deg = 0.0;
    double beamwidth_deg = 0.0;
};


// Receive report for delivered payloads.
// Filled in PropagationModelImpl::transmit() and passed to RadioImpl::receive().
struct ReceiveReport {
    Radiolib* sender = nullptr;
    std::string sender_id;
    std::string sender_platform_name;
    double distance_m = 0.0;
    double azimuth_deg = 0.0;
    double path_loss_db = 0.0;
    double rx_power_dbm = 0.0;
    double noise_floor_dbm = 0.0;
    double snr_db = 0.0;
    double frequency_hz = 0.0;
    double rain_attenuation_db = 0.0;
    double wind_attenuation_db_per_km = 0.0;
    double los_horizon_distance_m = 0.0;
    double polarization_loss_db = 0.0;
    double required_snr_threshold_db = 0.0;
    bool frequency_match = false;
    bool range_ok = false;
    bool sensitivity_ok = false;
    bool squelch_ok = false;
};

} // namespace radio
