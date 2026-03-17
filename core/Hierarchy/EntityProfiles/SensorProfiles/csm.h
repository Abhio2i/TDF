#ifndef CSM_H
#define CSM_H

#include "core/Hierarchy/EntityProfiles/sensor.h"

class CSM: public Sensor
{
    Q_OBJECT
public:
    explicit CSM(Hierarchy* h);
    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
};

#endif // CSM_H
