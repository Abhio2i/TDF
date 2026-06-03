#include "esm.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
const float RAD2DEG = 180.0f / M_PI;
ESM::ESM(Hierarchy* h) : Sensor(h) {
    subType = SubType::ESM;
    azimuth = 360;
    // if(h){
    //     connect(h,&Hierarchy::broadCast,[](Hierarchy::BroadcastMsg msg){

    //     }
    // );
}

void ESM::scan(){
    if(!Active) return;
    if(!parentEntity) return;
    if(!root) return;
    Transform* source = root->Platforms.count(parentEntity->ID)
                            ? root->Platforms[parentEntity->ID]->transform
                            : nullptr;
    if(!source) return;
    detect.clear();
    ewdetects.clear();
    for (auto& [key, entity] : root->Sensors)
    {
        if(!entity) continue;
        if(!entity->parentEntity) continue;
        if(!entity->parentEntity->Active) continue;
        if(entity->subType != Sensor::SubType::Generic) continue;
        if(!entity->Active) continue;
        if(entity->parentEntity->ID == parentEntity->ID) continue;
        auto it = root->Platforms.find(entity->parentEntity->ID);
        if(it == root->Platforms.end()) continue;
        Platform* platform = it->second;
        if(!platform) continue;
        if(!platform->transform) continue;
        float metredis = distanceBetween(
                             source->getLatitude(),  source->getLongitude(),
                             platform->transform->getLatitude(), platform->transform->getLongitude()
                             ) / 1000.0f;
        float dLon  = platform->transform->getLongitude() - source->getLongitude();
        float dLat  = platform->transform->getLatitude()  - source->getLatitude();
        float yAngle = std::atan2(dLon, dLat) * RAD2DEG;
        bool inRange = (range * 1000.0f) > metredis;
        if(inRange)
        {
            ewdetects.insert(platform);
            ESMTarget target;
            target.entity  = platform;
            target.angle   = yAngle;
            target.radius  = metredis;
            detect.append(target);
        }
    }
}

QJsonObject ESM::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["name"] = QString::fromStdString(Name);
    obj["active"] = Active;
    obj["SensorType"] = "ESM";
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km",  0,   500);
    defaultObj["frequency"] = toParm(frequency,"Ghz", 0.1, 100);
    defaultObj["azimuth"] = toParm(azimuth,"deg", 0,   360);
    obj["default"] = defaultObj;
    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;
    return obj;
}


void ESM::fromJson(const QJsonObject& obj) {
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
    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }
}
