#pragma once
// Radio configuration structures.
// These are consumed by RadioImpl (src/radiolib.cpp) and by the propagation model
// math in src/propagation_model.cpp. Middleware typically fills these from UI.
#include "radio_types.h"
#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace radio {

struct AntennaConfig {
    double gain_dbi = 0.0;
    double bandwidth_hz = 1e6;
    double beamwidth_deg = 360.0;          // 360 = omnidirectional
    Polarization polarization = Polarization::VERTICAL;
    double azimuth_min_deg = 0.0;
    double azimuth_max_deg = 360.0;
    double elevation_min_deg = -90.0;
    double elevation_max_deg = 90.0;
    ScanType scan_type = ScanType::FIXED;
    uint32_t scan_time1_ms = 0;
    uint32_t scan_time2_ms = 0;
    double peak_sidelobe_db = 0.0;
    double avg_sidelobe_db = 0.0;
};

struct ReceiverConfig {
    double sensitivity_dbm = -110.0;          // minimum detectable signal (dBm)
    double noise_figure_db = 5.0;
    double squelch_threshold_db = 3.0;

    // Frequency capture / detuning model:
    // inside frequency_tolerance_hz: no detune penalty
    // between tolerance and disconnect: link stays possible but gets noisier
    // beyond disconnect: receiver cannot capture the signal
    double frequency_tolerance_hz = 10000.0;      // 10 kHz clean region
    double frequency_disconnect_hz = 300000.0;    // 300 kHz hard disconnect
    double detune_noise_max_db = 30.0;            // extra noise added near disconnect
};


// All fields are optional with safe defaults; the propagation model uses only
// what is enabled via PropagationModelConfig toggles.
struct RadioConfig {
    // Basic identity (optional, for logging)
    std::string id;
    std::string parent_platform_name;
    bool is_naval = false;

    // Mode
    RadioMode mode = RadioMode::TRANSCEIVER;
    CommsMode comms_mode = CommsMode::LINE_OF_SIGHT;

    // Frequency
    double min_freq_hz = 30e6;
    double max_freq_hz = 512e6;
    double frequency_hz = 100e6;          // current operating frequency
    double bandwidth_hz = 25e3;
    double rx_bandwidth_hz = 0.0;         // 0 = use bandwidth_hz

    // Transmit
    double tx_power_dbm = 30.0;            // 1 Watt
    double power_degradation_db = 0.0;     // optional loss factor
    double tx_duty_cycle = 0.0;            // 0 = ignore, otherwise 0..1
    ModulationClass modulation_class = ModulationClass::PSK;
    ModulationScheme modulation_scheme = ModulationScheme::QPSK;
    bool required_snr_override = false;
    double required_snr_db = 0.0;
    double data_rate_bps = 256000;
    DuplexMode duplex_mode = DuplexMode::HALF_DUPLEX;
    double pulse_width_us = 0.0;           // 0 = continuous wave

    // Spread spectrum & LPI/LPD
    SpreadSpectrum spread_spectrum = SpreadSpectrum::NONE;
    double processing_gain_db = 0.0;
    bool frequency_hopping_enabled = false;
    bool lpi_enabled = false;
    bool lpd_enabled = false;
    bool aj_enabled = false;

    // Encryption
    EncryptionType encryption_type = EncryptionType::NONE;
    std::vector<std::byte> encryption_key; // AES: 16/24/32 bytes, DES: 8 bytes
    std::vector<std::byte> encryption_iv;  // AES: 16 bytes, DES: 8 bytes

    // Network
    uint32_t network_id = 0;
    uint32_t channel = 0;
    double max_range_m = 0.0;              // 0 = ignore

    // Antenna
    AntennaConfig antenna;
    double heading_deg = 0.0;              // optional orientation
    double velocity_mps = 0.0;             // optional (for doppler later)

    // Receiver (only used if mode != TRANSMITTER_ONLY)
    ReceiverConfig receiver;

    // Optional override for testing (if >0, overrides calculated path loss)
    double fixed_path_loss_db = 0.0;
};

} // namespace radio
