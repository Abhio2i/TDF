#ifndef ESM_H
#define ESM_H
#include "core/Hierarchy/EntityProfiles/sensor.h"

class ESM: public Sensor
{
    Q_OBJECT
public:
    explicit ESM(Hierarchy* h);
    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
};

#endif // ESM_H
