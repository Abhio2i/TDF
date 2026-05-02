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
    if(!Active)return;
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
    if(!parentEntity) return;
    detect.clear();
    Transform* source = root->Platforms[parentEntity->ID]->transform;
    if(!source) return;
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : root->Sensors)
    {
        if(!entity || !entity->parentEntity|| !entity->parentEntity->Active) continue;
        auto it = root->Platforms.find(entity->parentEntity->ID);
        if (it != root->Platforms.end()) {
            Platform* platform = it->second;
            // qDebug() << "[Sensor::ewscan] iterating entity:" << QString::fromStdString(key);
            if(platform->ID == parentEntity->ID || !platform || !platform->transform || entity->subType != Sensor::SubType::Generic ) continue;
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;

            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;

            //Radar side
            QVector3D radarlocalpos = platform->transform->inverseTransformPoint(source->matrix->translation());

            if (entity->Active && (range*1000.f) > metredis/*detectCheck(localPos,metredis) && entity->detectCheck(radarlocalpos,metredis,2) && entity->frequency==frequency*/) // .position() is assumed
            {
                //qDebug()<< "detect";
                if (ewdetects.count(platform) == 0)
                {
                    ewdetects.insert(platform);
                    ESMTarget target;
                    target.entity = platform;
                    target.angle = yAngle;
                    target.radius = metredis;
                    detect.append(target);
                }else{
                    for (int i = 0; i < detect.size(); ++i) {
                        if (detect.at(i).entity == platform) {
                            detect[i].angle = yAngle;
                            detect[i].radius = metredis;
                            break;
                        }
                    }
                }
            }
            else
            {
                if (ewdetects.count(platform) > 0)
                {
                    for (int i = 0; i < detect.size(); ++i) {
                        if (detect.at(i).entity == platform) {
                            detect.removeAt(i);
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
