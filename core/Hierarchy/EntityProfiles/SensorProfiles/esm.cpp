#include "esm.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
const float RAD2DEG = 180.0f / M_PI;
ESM::ESM(Hierarchy* h) : Sensor(h) {
    subType = SubType::ESM;
}

void ESM::scan(){
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)

    Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : *root->Platforms)
    {
        // qDebug() << "[Sensor::ewscan] iterating entity:" << QString::fromStdString(key);
        if(key == parentEntity->ID) continue;
        Platform* platform = entity;
        if (platform) {
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->translation().x(),source->translation().z(),platform->transform->matrix->translation().x(),platform->transform->matrix->translation().z())/1000;

            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            if (metredis<range) // .position() is assumed
            {
                //qDebug()<< "detect";
                if (ewdetects.count(platform) == 0)
                {
                    ewdetects.insert(platform);
                    Target target;
                    target.entity = platform;
                    target.angle = yAngle;
                    target.radius = metredis;
                    ewtargets.append(target);
                }else{
                    for (int i = 0; i < ewtargets.size(); ++i) {
                        if (ewtargets.at(i).entity == platform) {
                            ewtargets[i].angle = yAngle;
                            ewtargets[i].radius = metredis;
                            break;
                        }
                    }
                }
            }
            else
            {
                if (ewdetects.count(platform) > 0)
                {
                    for (int i = 0; i < ewtargets.size(); ++i) {
                        if (ewtargets.at(i).entity == platform) {
                            ewtargets.removeAt(i);
                            break;
                        }
                    }
                    ewdetects.erase(platform);
                }
            }
        }
    }
}

QJsonObject ESM::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["name"] = QString::fromStdString(Name);
    obj["on"] = on;
    obj["SensorType"] = "ESM";
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km");
    defaultObj["frequency"] = toParm(frequency,"Ghz");
    defaultObj["azimuth"] = toParm(azimuth,"deg");
    obj["default"] = defaultObj;
    return obj;
}

void ESM::fromJson(const QJsonObject& obj) {
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
