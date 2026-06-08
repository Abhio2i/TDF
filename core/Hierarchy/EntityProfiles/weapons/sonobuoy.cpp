#include "sonobuoy.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Simulation/simulation.h"

const float RAD2DEG = 180.0f / M_PI;

Sonobuoy::Sonobuoy(Hierarchy* h) : Weapon(h){
    weaponType     = WeaponType::Sonobuoy;
    Active = false;
    if (meshRenderer2d && meshRenderer2d->Sprite) {
        meshRenderer2d->Sprite->clear();
        meshRenderer2d->Sprite->append(":/sea/images/sea/sonobuoy.png");
    }

    if (collider) {
        collider->CollideRadius = 10000;
    }


}

Sonobuoy::~Sonobuoy(){

}

void Sonobuoy::dropped(){
    drop = true;
    Active = true;
    starttime = Simulation::simulationTime;
    if(parentEntity && transform){
        Transform* source = root->Platforms[parentEntity->ID]->transform;
        transform->setTranslation(source->translation());
        transform->setAltitude(Depth*3.281f);
    }

}

void Sonobuoy::Update(){
    if(!drop) return;



    if(!Active)return;
    time =  -starttime + Simulation::simulationTime;
    if((time/3600.f) > Life ){
        detects.clear();
        detection.clear();
        collider->CollideRadius = 1000;
        return;
    }
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
    if(!parentEntity || !transform || !collider) return;
    Transform* source = transform;
    if(!source) return;
    if(collider && !detection.empty()){
        if(collider->CollideRadius>5000){
            collider->CollideRadius = 5000;
        }else{
            collider->CollideRadius = 10000;
        }
    }else{
        collider->CollideRadius = 1000;
    }
    detects.clear();
    detection.clear();
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : root->Platforms)
    {
        if(!entity || !entity->Active) continue;
        if (entity->category == Entity::Category::Marine) {
            Platform* platform = entity;
            // qDebug() << "[Sensor::ewscan] iterating entity:" << QString::fromStdString(key);
            if(platform->ID == parentEntity->ID || !platform || !platform->transform) continue;
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;

            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;

            // qDebug()<<yAngle<<","<<detectCheck(localPos,metredis)<<","<<(fre1 < frequency && fre2> frequency);
            if (entity->Active && metredis<range)  // .position() is assumed
            {
                //qDebug()<< "detect";
                if (detects.count(platform) == 0)
                {
                    detects.insert(platform);
                    SonobuoyOutput target;
                    target.entity = platform;
                    target.bearing = yAngle;
                    target.radius = metredis;
                    detection.append(target);
                }else{
                    for (int i = 0; i < detection.size(); ++i) {
                        if (detection.at(i).entity == platform) {
                            detection[i].bearing = yAngle;
                            detection[i].radius = metredis;
                            break;
                        }
                    }
                }
            }
            else
            {
                if (detects.count(platform) > 0)
                {
                    for (int i = 0; i < detection.size(); ++i) {
                        if (detection.at(i).entity == platform) {
                            detection.removeAt(i);
                            break;
                        }
                    }
                    detects.erase(platform);
                }
            }
        }
    }


}

QJsonObject Sonobuoy::toJson() const
{
    QJsonObject obj;
    obj["active"] = Active;
    obj["drop"] = drop;
    obj["name"] = QString::fromStdString(Name);
    obj["id"] = QString::fromStdString(ID);
    obj["weaponTypeName"]     = "Sunobuoy";
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km",0, 120);
    // defaultObj["TransmissionRange"] = toParm(TransmissionRange,"km", 0.1,  500);
    defaultObj["Depth"] = toParm(Depth,"m", 10, 700);
    defaultObj["Life"] = toParm(Life,"h", 1, 10);
    obj["default"] = defaultObj;

    if (transform)      obj["transform"]    = transform->toJson();
    if (rigidbody)      obj["rigidbody"]    = rigidbody->toJson();
    if (collider)       obj["collider"]     = collider->toJson();
    if (trajectory)     obj["trajectory"]   = trajectory->toJson();
    if (meshRenderer2d) obj["bitmap"]       = meshRenderer2d->toJson();
    if (dynamicModel)   obj["dynamicModel"] = dynamicModel->toJson();
    if (crossSection)   obj["crossSection"] = crossSection->toJson();
    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    return obj;

}

void Sonobuoy::fromJson(const QJsonObject& obj)
{
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();

    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();

    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();

    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("drop"))
        drop = obj["drop"].toBool();

    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();

        if (defaultObj.contains("range"))
            range = valueFromParm(defaultObj["range"].toObject());

        if (defaultObj.contains("TransmissionRange"))
            TransmissionRange = valueFromParm(defaultObj["TransmissionRange"].toObject());

        if (defaultObj.contains("Depth"))
            Depth = valueFromParm(defaultObj["Depth"].toObject());

        if (defaultObj.contains("Life"))
            Life = valueFromParm(defaultObj["Life"].toObject());

    }
    // 7 auto-components
    if (obj.contains("transform")    && transform)     transform->fromJson(obj["transform"].toObject());
    if (obj.contains("rigidbody")    && rigidbody)     rigidbody->fromJson(obj["rigidbody"].toObject());
    if (obj.contains("collider")     && collider)      collider->fromJson(obj["collider"].toObject());
    if (obj.contains("trajectory")   && trajectory)    trajectory->fromJson(obj["trajectory"].toObject());
    if (obj.contains("bitmap")       && meshRenderer2d) meshRenderer2d->fromJson(obj["bitmap"].toObject());
    if (obj.contains("dynamicModel") && dynamicModel)  dynamicModel->fromJson(obj["dynamicModel"].toObject());
    if (obj.contains("crossSection") && crossSection)  crossSection->fromJson(obj["crossSection"].toObject());

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }
}

