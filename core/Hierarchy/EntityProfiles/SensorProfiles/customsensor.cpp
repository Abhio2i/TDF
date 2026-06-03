#include "customsensor.h"

CustomSensor::CustomSensor(Hierarchy* h) : Sensor(h)
{
    subType = SubType::Generic;

}

void CustomSensor::scan()
{
    qDebug()<< "sensor working";
}

QJsonObject CustomSensor::toJson() const
{
    QJsonObject obj;

    // --- Identity (unchanged) ---
    obj["id"]         = QString::fromStdString(ID);
    obj["name"]       = QString::fromStdString(Name);
    obj["Active"]     = Active;
    obj["SensorType"] = type;

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    return obj;
}


void CustomSensor::fromJson(const QJsonObject& obj)
{

    if(obj.contains("SensorType")){
        type = obj["SensorType"].toString();
    }
    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }


}
