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
    if(parentEntity){
        Transform* source = root->Platforms[parentEntity->ID]->transform;
        transform->setTranslation(source->translation());
        transform->setAltitude(Depth*3.281f);
    }

}

void Sonobuoy::Update(){
    // if(!drop) return;

    // time += Simulation::simulationTime;
    // if((time/3600.f) > Life ){
    //     return;
    // }

    if(!Active)return;
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
    if(!parentEntity) return;
    Transform* source = transform;
    if(!source) return;
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : root->Platforms)
    {
        if(!entity || !entity->Active) continue;
        if (entity->category == Entity::Category::Submarine) {
            Platform* platform = entity;
            // qDebug() << "[Sensor::ewscan] iterating entity:" << QString::fromStdString(key);
            if(platform->ID == parentEntity->ID || !platform || !platform->transform) continue;
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;

            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;

            // qDebug()<<yAngle<<","<<detectCheck(localPos,metredis)<<","<<(fre1 < frequency && fre2> frequency);
            if (entity->Active && metredis<range*1000.0f)  // .position() is assumed
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
    obj["name"] = QString::fromStdString(Name);
    obj["id"] = QString::fromStdString(ID);
    obj["weaponTypeName"]     = "Sunobuoy";
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km",0, 120);
    defaultObj["TransmissionRange"] = toParm(TransmissionRange,"km", 0.1,  500);
    defaultObj["Depth"] = toParm(Depth,"m", 10, 700);
    defaultObj["Life"] = toParm(Life,"h", 1, 10);
    obj["default"] = defaultObj;

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    return obj;

}

void Sonobuoy::fromJson(const QJsonObject& obj)
{


}
