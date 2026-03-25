#include "csm.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
const float RAD2DEG = 180.0f / M_PI;
CSM::CSM(Hierarchy* h) : Sensor(h) {
    subType = SubType::CSM;
    azimuth = 360;
    frequency = 10.0f;
}

void CSM::scan(){
    if(!Active)return;
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
    if(!parentEntity) return;
    Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
    if(!source) return;
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : *root->Radios)
    {
        if(!entity || !entity->parentEntity || !entity->parentEntity->Active) continue;
        auto it = root->Platforms->find(entity->parentEntity->ID);
        if (it != root->Platforms->end()) {
            Platform* platform = it->second;
            // qDebug() << "[Sensor::ewscan] iterating entity:" << QString::fromStdString(key);
            if(platform->ID == parentEntity->ID || !platform || !platform->transform) continue;
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;

            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            float fre1 = entity->minFrequency;
            float fre2 = entity->maxFrequency;
            // qDebug()<<yAngle<<","<<detectCheck(localPos,metredis)<<","<<(fre1 < frequency && fre2> frequency);
            if (entity->Active && detectCheck(localPos,metredis) && fre1 < frequency && fre2> frequency)  // .position() is assumed
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

QJsonObject CSM::toJson() const {
    QJsonObject obj;
    obj["active"] = Active;
    obj["name"] = QString::fromStdString(Name);
    obj["SensorType"] = "CSM";
    obj["id"] = QString::fromStdString(ID);
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km",0,    500);
    defaultObj["frequency"] = toParm(frequency,"Ghz", 0.1,  100);
    defaultObj["azimuth"] = toParm(azimuth,"deg", 0,    360);
    obj["default"] = defaultObj;
    return obj;
}

void CSM::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")){
        ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("active"))
        Active = obj["active"].toBool();
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
