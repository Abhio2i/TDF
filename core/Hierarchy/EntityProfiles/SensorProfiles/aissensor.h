#ifndef AISSENSOR_H
#define AISSENSOR_H

#include <QObject>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/AIS/AIS.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

// AIS Navigation Status
enum class NavStatus : uint8_t {
    UnderWayEngine = 0,
    AtAnchor = 1,
    NotUnderCommand = 2,
    RestrictedManoeuvrability = 3,
    ConstrainedByDraft = 4,
    Moored = 5,
    Aground = 6,
    EngagedInFishing = 7,
    UnderWaySailing = 8,
    Reserved = 15 // Unknown
};

// Ship and Cargo Types
enum class ShipType : uint8_t {
    Unknown = 0,
    Fishing = 30,
    Tug = 52,
    Passenger = 60,
    Cargo = 70,
    Tanker = 80,
    PleasureCraft = 37
};

struct AISVesselData {
    Platform *entity;
    double angle = 0.0;
    double radius = 0.0;
    // --- Static Information (Every 6 mins) ---
    uint32_t mmsi;                // Unique ID
    uint32_t imo_number;          // Official IMO ID
    char name[21];                // Vessel Name (Max 20 chars)
    char call_sign[8];            // Radio Call Sign
    char destination[21];         // Destination (Max 20 chars)

    struct Dimensions {
        uint16_t length;          // Meters
        uint16_t beam;            // Width in meters
        float draught;            // Current depth in water
    } dims;

    ShipType ship_type;

    // --- Dynamic Information (Every 2-10 seconds) ---
    struct Position {
        double latitude;          // Decimal degrees
        double longitude;         // Decimal degrees
        uint32_t timestamp;       // UTC Unix Timestamp
    } pos;

    float sog;                    // Speed Over Ground (Knots)
    float cog;                    // Course Over Ground (Degrees)
    uint16_t heading;             // True Heading (0-359)
    int8_t rot;                   // Rate of Turn (deg/min)

    NavStatus nav_status;

    // --- Voyage Related ---
    struct ETA {
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
    } eta;

    // --- Metadata / System Level ---
    uint8_t repeat_indicator;     // AIS Message repeat count
    uint8_t ais_channel;          // 0 for A, 1 for B
    bool is_verified;             // Checksum pass/fail
};



class AISSensor: public Sensor
{
    Q_OBJECT
public:
    struct AisRxEvent {
        float simTime = 0.0f;
        ais::AisReceiveReport report;
    };
    explicit AISSensor(Hierarchy* h);
    ais::AisSensor ownship;
    static rf::RfPropagationModel* model;
    QVector<AisRxEvent> rxEvents;
    float lastPrintTime = 0.0f;
     QVector<AISVesselData> detect;
    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
    QJsonObject AdditionalParameters;   //!< User-defined extension parameters


    private:
        // AIS input fields
        bool enabled = true;
        bool tx_enabled = true;
        bool rx_enabled = true;

        uint32_t mmsi = 419123456;
        std::string ais_name = "OWN_SHIP";
        std::string ais_class = "CLASS_A";
        std::string channel_mode = "DUAL";

        double latitude_deg = 18.95000;
        double longitude_deg = 72.82000;
        double sog_kn = 12.5;
        double cog_deg = 90.0;
        double heading_deg = 90.0;
        double rot_deg_per_min = 0.0;

        double dynamic_interval_s = 2.0;
        double static_interval_s = 10.0;
        double track_stale_timeout_s = 30.0;

        std::string rf_id = "OWN_AIS";
        std::string parent_name = "OWN_PLATFORM";
        std::string rf_mode = "TRANSCEIVER";

};

#endif // AISSENSOR_H
