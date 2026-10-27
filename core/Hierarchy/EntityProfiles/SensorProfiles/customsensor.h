#ifndef CUSTOMSENSOR_H
#define CUSTOMSENSOR_H

#include "core/Plugins/Interfaces/customSensorInterface.h"
#include <core/Hierarchy/EntityProfiles/sensor.h>


class CustomSensor: public Sensor
{
    Q_OBJECT

public:
    explicit CustomSensor(Hierarchy* h);
    ~CustomSensor();
    QString type = "";
    CustomSensorInterface*  sensor = nullptr;
    // ------------------------------------------------------------------
    // Engine tick
    // ------------------------------------------------------------------
    void scan() override;


    QJsonObject toJson()                     const override;
    void        fromJson(const QJsonObject&)       override;

    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

};

#endif // CUSTOMSENSOR_H
