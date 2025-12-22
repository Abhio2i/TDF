#include "radar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
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
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->translation().x(),source->translation().z(),platform->transform->matrix->translation().x(),platform->transform->matrix->translation().z())/1000;
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            if (detectCheck(localPos,metredis)) // .position() is assumed
            {
                if (detects.count(platform) == 0)
                {
                    detects.insert(platform);
                    Target target;
                    target.entity = platform;
                    target.angle = yAngle;
                    target.radius = metredis;
                    targets.append(target);
                }else{
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            targets[i].angle = yAngle;
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
}

QJsonObject Radar::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["name"] = QString::fromStdString(Name);
    obj["on"] = on;
    obj["SensorType"] = "Radar";
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km");
    defaultObj["frequency"] = toParm(frequency,"Ghz");
    defaultObj["azimuth"] = toParm(azimuth,"deg");
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
    }
}
