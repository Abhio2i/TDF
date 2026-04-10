#ifndef AISSENSOR_H
#define AISSENSOR_H

#include <QObject>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/AIS/AIS.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

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
    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

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
