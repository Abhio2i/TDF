#include "customsensor.h"
#include "core/Hierarchy/hierarchy.h"

CustomSensor::CustomSensor(Hierarchy* h) : Sensor(h)
{
    subType = SubType::Generic;
}

CustomSensor::~CustomSensor(){

    if(parentEntity&&parentEntity->root){
        emit parentEntity->root->RemoveCustomSensor(QString::fromStdString(parentEntity->ID),parentEntity->ecsID,QString::fromStdString(ID));
    }
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
    if(sensor){
        obj = sensor->toJson();
        obj["id"] = QString::fromStdString(ID);
    }
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
    if(sensor)
    sensor->fromJson(obj);


}
