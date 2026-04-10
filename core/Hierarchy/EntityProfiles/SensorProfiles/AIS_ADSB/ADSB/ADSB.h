#pragma once
#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/RF/RF.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace adsb {

using RfPosition = rf::RfPosition;
using RfMode = rf::RfMode;
using RfConfig = rf::RfConfig;
using RfDevice = rf::RfDevice;
using RfPropagationConfig = rf::RfPropagationConfig;
using RfPropagationModel = rf::RfPropagationModel;
using RfReceiveReport = rf::RfReceiveReport;
using RfScanHit = rf::RfScanHit;

enum class AdsbMessageKind {
    IDENTIFICATION,
    AIRBORNE_POSITION,
    AIRBORNE_VELOCITY,
    STATUS
};

enum class AdsbEmergencyState {
    NONE,
    GENERAL,
    NO_COMMUNICATION,
    UNLAWFUL_INTERFERENCE,
    DOWNED_AIRCRAFT
};

struct AdsbStaticData {
    uint32_t icao_address = 0;
    std::string flight_id;
    uint8_t emitter_category = 0;
    uint16_t aircraft_length_m = 0;
    uint16_t aircraft_width_m = 0;
};

struct AdsbDynamicData {
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
    double altitude_baro_ft = 0.0;
    double altitude_geometric_ft = 0.0;
    double ground_speed_kt = 0.0;
    double track_angle_deg = 0.0;
    double vertical_rate_fpm = 0.0;
};

struct AdsbStatusData {
    uint8_t nacp = 0;
    uint8_t nic = 0;
    uint8_t sil = 0;
    uint8_t adsb_version = 2;
    uint8_t geometric_vertical_accuracy = 0;
    uint8_t system_design_assurance = 0;
    uint16_t capability_class = 0;
    uint16_t operational_mode = 0;
    bool nic_supplement_a = false;
    bool nic_baro = false;
    bool horizontal_reference_true_north = true;
    bool sil_supplement = false;
    AdsbEmergencyState emergency_state = AdsbEmergencyState::NONE;
    uint16_t squawk = 0;
    uint32_t utc_second = 0;
};

struct AdsbConfig {
    bool enabled = true;
    bool tx_enabled = true;
    bool rx_enabled = true;
    bool use_1090es = true;
    double identification_interval_s = 5.0;
    double position_interval_s = 1.0;
    double velocity_interval_s = 1.0;
    double status_interval_s = 5.0;
    double track_stale_timeout_s = 30.0;
    AdsbStaticData static_data;
    AdsbDynamicData dynamic_data;
    AdsbStatusData status_data;
};

struct AdsbTrack {
    uint32_t icao_address = 0;
    AdsbStaticData static_data;
    AdsbDynamicData dynamic_data;
    AdsbStatusData status_data;
    RfReceiveReport last_report;
    AdsbMessageKind last_message_kind = AdsbMessageKind::IDENTIFICATION;
    uint64_t last_update_ms = 0;
    bool stale = false;
};

struct AdsbReceiveReport {
    AdsbTrack track;
    std::vector<std::byte> raw_frame;
};

class AdsbSensor {
public:
    using TrackUpdateCallback = std::function<void(const AdsbTrack&)>;
    using ReceiveCallback = std::function<void(const AdsbReceiveReport&)>;

    struct CprFrame {
        bool valid = false;
        bool odd = false;
        uint32_t lat_cpr = 0;
        uint32_t lon_cpr = 0;
        double altitude_ft = 0.0;
        uint64_t timestamp_ms = 0;
    };

    AdsbSensor();
    ~AdsbSensor();

    void attachToModel(RfPropagationModel* model, const RfPosition& pos);
    void detachFromModel();
    void updatePosition(const RfPosition& pos);

    void configureAdsb(const AdsbConfig& cfg);
    AdsbConfig getAdsbConfiguration() const;

    void configureRf(const RfConfig& cfg);
    RfConfig getRfConfiguration() const;

    void tick(double sim_time_s);
    bool transmitIdentification();
    bool transmitPosition();
    bool transmitVelocity();
    bool transmitStatus();

    std::vector<AdsbTrack> getTracks() const;
    std::optional<AdsbTrack> getTrack(uint32_t icao_address) const;
    void clearTracks();

    void setTrackUpdateCallback(TrackUpdateCallback cb);
    void setReceiveCallback(ReceiveCallback cb);

    RfDevice& rfDevice();
    const RfDevice& rfDevice() const;

private:
    void ingest(const std::vector<std::byte>& data, const RfReceiveReport& report);
    void rebuildRfDefaults(bool reset_schedule = true);

    std::unique_ptr<RfDevice> device_;
    mutable std::mutex mutex_;
    AdsbConfig adsb_config_;
    std::unordered_map<uint32_t, AdsbTrack> tracks_;
    std::unordered_map<uint32_t, CprFrame> even_cpr_frames_;
    std::unordered_map<uint32_t, CprFrame> odd_cpr_frames_;
    TrackUpdateCallback update_cb_;
    ReceiveCallback receive_cb_;
    double last_ident_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    double last_position_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    double last_velocity_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    double last_status_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    bool next_position_odd_ = false;
};

RfConfig makeAdsbRfConfig(const AdsbConfig& cfg,
                          RfMode mode = RfMode::TRANSCEIVER);

} // namespace adsb
