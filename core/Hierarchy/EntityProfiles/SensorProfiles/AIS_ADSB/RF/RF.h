#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace rf {

struct RfPosition {
    double x = 0.0;
    double y = 0.0;
    double altitude = 0.0;
};

enum class RfMode {
    RECEIVER_ONLY,
    TRANSMITTER_ONLY,
    TRANSCEIVER
};

enum class RfProtocol {
    GENERIC,
    AIS,
    ADSB
};

enum class RfPolarization {
    VERTICAL,
    HORIZONTAL,
    CIRCULAR_LEFT,
    CIRCULAR_RIGHT
};

enum class RfScanType {
    FIXED,
    SECTOR_SCAN,
    CONICAL_SCAN
};

enum class RfCommsMode {
    LINE_OF_SIGHT,
    BEYOND_LINE_OF_SIGHT,
    SATCOM,
    TROPOSCATTER
};

enum class RfSpreadSpectrum {
    NONE,
    FHSS,
    DSSS
};

enum class RfModulationClass {
    AM,
    FM,
    PSK,
    QAM,
    FSK,
    OFDM
};

enum class RfModulationScheme {
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

struct RfAntennaConfig {
    double gain_dbi = 0.0;
    double beamwidth_deg = 360.0;
    RfPolarization polarization = RfPolarization::VERTICAL;
    RfScanType scan_type = RfScanType::FIXED;
    double azimuth_min_deg = 0.0;
    double azimuth_max_deg = 360.0;
    uint32_t scan_period_ms = 0;
};

struct RfReceiverConfig {
    double sensitivity_dbm = -107.0;
    double noise_figure_db = 5.0;
    double squelch_threshold_db = 3.0;
    double frequency_tolerance_hz = 10000.0;
    double frequency_disconnect_hz = 300000.0;
    double detune_noise_max_db = 30.0;
};

struct RfPropagationConfig {
    bool enable_fspl = true;
    bool enable_log_distance = false;
    bool enable_two_ray = false;
    bool enable_los_horizon = false;
    bool enable_comms_mode_losses = true;
    bool enable_fixed_path_loss_override = true;
    bool enable_scan_beam = false;
    bool enable_scan_timing = false;
    bool enable_range_limit = false;
    bool enable_noise_floor = true;
    bool enable_snr_threshold = true;
    bool enable_sensitivity = true;
    bool enable_squelch = true;
    bool enable_interference = false;
    bool enable_shadowing = false;
    bool enable_fading = false;
    bool enable_doppler = false;
    bool enable_environmental_attenuation = false;
    bool enable_sea_attenuation = false;
    bool enable_polarization_loss = true;

    double los_blocked_loss_db = 200.0;
    double bLOS_diffraction_db_per_m = 0.1;
    double satcom_extra_loss_db = 150.0;
    double troposcatter_log_loss_factor_db = 40.0;
    double log_distance_path_loss_exp = 2.0;
    double log_distance_ref_distance_m = 1.0;
    double shadowing_sigma_db = 1.5;
    double fading_sigma_db = 1.0;
    double polarization_mismatch_loss_db = 3.0;
    double temperature_c = 20.0;
    double pressure_hpa = 1005.0;
    double humidity_percent = 60.0;
    double gas_attenuation_db_per_km_at_1ghz = 0.005;
    double gas_attenuation_freq_exponent = 1.0;
    double humidity_attenuation_factor_per_percent = 0.002;
    double rain_rate_mm_per_hr = 10.0;
    double rain_attenuation_db_per_km_per_mmhr = 0.004;
    bool use_itu_rain_model = true;
    double rain_coverage = 0.4;
    double rain_rate_sigma_frac = 0.15;
    double wind_speed_mps = 8.0;
    double wind_attenuation_db_per_km_per_mps = 0.0005;
    double sea_attenuation_db_per_km = 0.003;
    double interference_power_dbm = -120.0;
};

struct RfConfig {
    std::string id;
    std::string parent_name;
    RfMode mode = RfMode::TRANSCEIVER;
    RfProtocol protocol = RfProtocol::GENERIC;
    RfCommsMode comms_mode = RfCommsMode::LINE_OF_SIGHT;

    double min_freq_hz = 161.95e6;
    double max_freq_hz = 162.05e6;
    double frequency_hz = 161.975e6;
    std::vector<double> receive_frequencies_hz;
    double bandwidth_hz = 25e3;
    double rx_bandwidth_hz = 25e3;

    double tx_power_dbm = 40.97;
    double power_degradation_db = 0.0;
    double tx_duty_cycle = 1.0;
    double max_range_m = 0.0;
    double required_snr_threshold_db = 0.0;
    RfModulationClass modulation_class = RfModulationClass::FSK;
    RfModulationScheme modulation_scheme = RfModulationScheme::GMSK;

    double heading_deg = 0.0;
    double velocity_mps = 0.0;
    bool is_naval = true;
    RfSpreadSpectrum spread_spectrum = RfSpreadSpectrum::NONE;
    double processing_gain_db = 0.0;

    RfAntennaConfig antenna;
    RfReceiverConfig receiver;

    // Optional per-device propagation override. When disabled, the shared
    // RfPropagationModel config is used as the fallback/default.
    bool use_local_propagation_config = false;
    RfPropagationConfig propagation;

    double fixed_path_loss_db = 0.0;
};

struct RfReceiveReport {
    std::string sender_id;
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
    bool protocol_match = false;
    bool frequency_match = false;
    bool range_ok = false;
    bool beam_ok = false;
    bool sensitivity_ok = false;
    bool squelch_ok = false;
    bool snr_ok = false;
};

struct RfScanHit {
    std::string id;
    std::string target_name;
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
    bool protocol_match = false;
    bool frequency_match = false;
    bool range_ok = false;
    bool beam_ok = false;
    bool sensitivity_ok = false;
    bool squelch_ok = false;
    bool snr_ok = false;
    bool link_ok = false;
};

class RfPropagationModel;

class RfDevice {
public:
    using ReceiveCallback = std::function<void(const std::vector<std::byte>&, const RfReceiveReport&)>;

    RfDevice();
    ~RfDevice();

    void configure(const RfConfig& config);
    RfConfig getConfiguration() const;

    void setPowerOn(bool on);
    bool isPoweredOn() const;

    void setReceiveCallback(ReceiveCallback cb);

    void attachToModel(RfPropagationModel* model, const RfPosition& pos);
    void detachFromModel();
    void updatePosition(const RfPosition& pos);

    bool transmit(const std::vector<std::byte>& data);
    std::vector<RfScanHit> scan() const;

private:
    friend class RfPropagationModel;
    void deliver(const std::vector<std::byte>& data, const RfReceiveReport& report);

    mutable std::mutex mutex_;
    RfConfig config_;
    bool powered_on_ = true;
    ReceiveCallback receive_cb_;
    RfPropagationModel* model_ = nullptr;
};

class RfPropagationModel {
public:
    explicit RfPropagationModel(const RfPropagationConfig& cfg = {});
    ~RfPropagationModel();

    void addDevice(RfDevice* device, const RfPosition& pos);
    void removeDevice(RfDevice* device);
    void updatePosition(RfDevice* device, const RfPosition& pos);
    std::vector<RfScanHit> scan(const RfDevice* scanner) const;
    void transmit(RfDevice* sender, const std::vector<std::byte>& data);

    void setConfig(const RfPropagationConfig& cfg);
    RfPropagationConfig getConfig() const;

private:
    struct DeviceEntry {
        RfPosition pos;
    };

    mutable std::mutex mutex_;
    std::unordered_map<const RfDevice*, DeviceEntry> devices_;
    RfPropagationConfig config_;
};

} // namespace rf
