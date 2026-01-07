#include "radar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
const float RAD2DEG = 180.0f / M_PI;
Radar::Radar(Hierarchy* h) : Sensor(h)  {
    subType = SubType::Generic;
}

void Radar::scan(){
    // qDebug()<<"Radar";
    Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
    for (auto& [key, entity] : *root->Platforms)
    {
        if(key == parentEntity->ID) continue;
        Platform* platform = entity;
        if (platform) {
            QVector3D localPos = source->inverseTransformPoint(platform->transform->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            if (detectCheck(localPos,metredis)) // .position() is assumed
            {
                if (detects.count(platform) == 0)
                {
                    detects.insert(platform);
                    Target target;
                    target.entity = platform;
                    if(platform->dynamicModel)
                        target.speed = platform->dynamicModel->currentSpeed;
                    target.angle = yAngle;
                    target.radius = metredis;
                    targets.append(target);
                }else{
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            targets[i].angle = yAngle;
                            if(platform->dynamicModel)
                                targets[i].speed = platform->dynamicModel->currentSpeed;
                            targets[i].radius = metredis;
                            break;
                        }
                    }
                }
            }
            else
            {
                // C# detects.Contains(tr) -> C++ detects.count(tr) > 0
                if (detects.count(platform) > 0)
                {
                    // C# detects.Remove(tr) -> C++ detects.erase(tr)
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            targets.removeAt(i);
                            break;
                        }
                    }
                    detects.erase(platform);
                    // qDebug()<< "vanish :"<<&entity->Name;
                }
            }
        }
    }
    // qDebug()<<targets.size();
}

QJsonObject Radar::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["name"] = QString::fromStdString(Name);
    obj["on"] = on;
    obj["SensorType"] = "Radar";

    QJsonObject capabilitiesObj;
    capabilitiesObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : DetectionCapabilitiesTypeOptions())
        optionsArray.append(opt);
    capabilitiesObj["options"] = optionsArray;
    capabilitiesObj["value"] = detectionCapabilitiesToString(capabilities);


    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km");
    defaultObj["frequency"] = toParm(frequency,"Ghz");
    defaultObj["azimuth"] = toParm(azimuth,"deg");
    defaultObj["DetectionCapabilities"] = capabilitiesObj;
    obj["default"] = defaultObj;
    return obj;
}

void Radar::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")){
        ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("on"))
        on = obj["on"].toBool();
    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();
        if (defaultObj.contains("range"))
            range = valueFromParm(defaultObj["range"].toObject());

        if (defaultObj.contains("frequency"))
            frequency = valueFromParm(defaultObj["frequency"].toObject());

        if (defaultObj.contains("azimuth"))
            azimuth = valueFromParm(defaultObj["azimuth"].toObject());

        if (defaultObj.contains("DetectionCapabilities") && defaultObj["DetectionCapabilities"].isObject()) {
            QJsonObject capabilitiesObj = defaultObj["DetectionCapabilities"].toObject();
            if (capabilitiesObj.contains("value"))
                capabilities = stringTodetectionCapabilities(capabilitiesObj["value"].toString());
        }
    }
}
