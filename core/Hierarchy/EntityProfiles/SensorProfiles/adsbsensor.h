#ifndef ADSBSENSOR_H
#define ADSBSENSOR_H

#include <QObject>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/ADSB/ADSB.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/AIS/AIS.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

class ADSBSensor: public Sensor
{
    Q_OBJECT
public:
    struct AdsbRxEvent {
        float simTime = 0.0f;
        adsb::AdsbReceiveReport report;
    };


    explicit ADSBSensor(Hierarchy* h);

    adsb::AdsbSensor ownship;
    static rf::RfPropagationModel* model;
    QVector<AdsbRxEvent> rxEvents;
    float lastPrintTime = 0.0f;
    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

        // ADS-B config fields
        bool enabled = true;
        bool tx_enabled = true;
        bool rx_enabled = true;

        uint32_t icao_address = 0xABC001;
        std::string flight_id = "OWN123";

        double latitude_deg = 18.95000;
        double longitude_deg = 72.82000;
        double altitude_baro_ft = 32000.0;
        double ground_speed_kt = 440.0;
        double track_angle_deg = 90.0;
        double vertical_rate_fpm = 0.0;

        double identification_interval_s = 2.0;
        double position_interval_s = 1.0;
        double velocity_interval_s = 1.0;
        double status_interval_s = 5.0;
        double track_stale_timeout_s = 30.0;

        std::string rf_id;
        std::string parent_name;
        std::string rf_mode = "TRANSCEIVER";

};

#endif // ADSBSENSOR_H
