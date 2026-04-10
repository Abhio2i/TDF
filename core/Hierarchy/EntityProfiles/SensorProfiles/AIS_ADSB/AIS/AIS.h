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

namespace ais {

using rf::RfAntennaConfig;
using rf::RfConfig;
using rf::RfDevice;
using rf::RfMode;
using rf::RfPolarization;
using rf::RfPosition;
using rf::RfPropagationConfig;
using rf::RfPropagationModel;
using rf::RfProtocol;
using rf::RfReceiveReport;
using rf::RfReceiverConfig;
using rf::RfScanHit;
using rf::RfScanType;

enum class AisClass {
    CLASS_A,
    CLASS_B
};

enum class AisChannelMode {
    AIS1_ONLY,
    AIS2_ONLY,
    DUAL
};

enum class AisNavStatus {
    UNDER_WAY = 0,
    AT_ANCHOR = 1,
    NOT_UNDER_COMMAND = 2,
    RESTRICTED_MANOEUVRABILITY = 3,
    CONSTRAINED_BY_DRAFT = 4,
    MOORED = 5,
    AGROUND = 6,
    ENGAGED_IN_FISHING = 7,
    UNDER_WAY_SAILING = 8,
    UNKNOWN = 15
};

enum class AisMessageKind {
    POSITION_CLASS_A = 1,
    BASE_STATION = 4,
    STATIC_AND_VOYAGE = 5,
    POSITION_CLASS_B = 18,
    STATIC_DATA_REPORT = 24
};

struct AisStaticData {
    uint32_t mmsi = 0;
    uint32_t imo = 0;
    std::string name;
    std::string callsign;
    std::string vendor_id;
    std::string destination;
    uint16_t ship_type = 0;
    uint16_t dim_bow_m = 0;
    uint16_t dim_stern_m = 0;
    uint16_t dim_port_m = 0;
    uint16_t dim_starboard_m = 0;
    float draught_m = 0.0f;
};

struct AisDynamicData {
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
    double sog_kn = 0.0;
    double cog_deg = 0.0;
    double heading_deg = 0.0;
    double rot_deg_per_min = 0.0;
    bool position_accuracy = false;
    AisNavStatus nav_status = AisNavStatus::UNKNOWN;
};

struct AisVoyageData {
    uint8_t eta_month = 0;
    uint8_t eta_day = 0;
    uint8_t eta_hour = 0;
    uint8_t eta_minute = 0;
};

struct AisConfig {
    bool enabled = true;
    bool tx_enabled = true;
    bool rx_enabled = true;
    AisClass ais_class = AisClass::CLASS_A;
    AisChannelMode channel_mode = AisChannelMode::DUAL;
    double ais1_frequency_hz = 161.975e6;
    double ais2_frequency_hz = 162.025e6;
    double dynamic_interval_s = 0.0;
    double static_interval_s = 0.0;
    double track_stale_timeout_s = 900.0;
    bool prefer_aivdo_for_ownship = false;
    AisStaticData static_data;
    AisDynamicData dynamic_data;
    AisVoyageData voyage_data;
};

struct AisTrack {
    uint32_t mmsi = 0;
    AisStaticData static_data;
    AisDynamicData dynamic_data;
    AisVoyageData voyage_data;
    RfReceiveReport last_report;
    AisMessageKind last_message_kind = AisMessageKind::POSITION_CLASS_A;
    uint64_t last_update_ms = 0;
    bool stale = false;
};

struct AisReceiveReport {
    AisTrack track;
    std::string raw_sentence;
};

class AisSensor {
public:
    using TrackUpdateCallback = std::function<void(const AisTrack&)>;
    using ReceiveCallback = std::function<void(const AisReceiveReport&)>;

    AisSensor();
    ~AisSensor();

    void attachToModel(RfPropagationModel* model, const RfPosition& pos);
    void detachFromModel();
    void updatePosition(const RfPosition& pos);

    void configureAis(const AisConfig& cfg);
    AisConfig getAisConfiguration() const;

    void configureRf(const RfConfig& cfg);
    RfConfig getRfConfiguration() const;

    void tick(double sim_time_s);
    bool transmitDynamicReport();
    bool transmitStaticVoyageReport();

    std::vector<AisTrack> getTracks() const;
    std::optional<AisTrack> getTrack(uint32_t mmsi) const;
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
    AisConfig ais_config_;
    std::unordered_map<uint32_t, AisTrack> tracks_;
    TrackUpdateCallback update_cb_;
    ReceiveCallback receive_cb_;
    double last_dynamic_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    double last_static_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    double next_dynamic_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    double next_static_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    int next_tx_channel_ = 1;
};

RfConfig makeAisRfConfig(const AisConfig& cfg,
                         bool use_ais2,
                         RfMode mode = RfMode::TRANSCEIVER);

} // namespace ais
