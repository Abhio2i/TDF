#ifndef RADAR_H
#define RADAR_H

#include "core/Hierarchy/EntityProfiles/sensor.h"

class Radar : public Sensor
{
    Q_OBJECT
public:
    explicit Radar(Hierarchy* h);
    bool on = false;
    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
};

#endif // RADAR_H
