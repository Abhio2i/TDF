#include "specialzone.h"

#include "qjsonarray.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/GlobalRegistry.h"
#include "qmath.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>
Specialzone::Specialzone(Hierarchy* h) : Entity(h) {
    type = Constants::EntityType::SpecialZone;
}

void Specialzone::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));

}

void Specialzone::Update(float delta){
    time += delta;
    if(root){
        if(!transform || !collider)return;
        for (auto& [key, entity] : *root->Platforms){
            if(!entity || !entity->Active || entity->isDestroy || !entity->transform || !entity->dynamicModel) continue;
            float distance = transform->matrix->translation().distanceToPoint(entity->transform->matrix->translation())*1000;
            // qDebug()<<distance;
            float alt = entity->dynamicModel->currentAltitude * 1;//KMtoFT;
            if(distance < collider->CollideRadius && alt > (MinAltitude-100) && alt < MaxAltitude){
                entity->dynamicModel->windAngle = direction;
                entity->dynamicModel->windDierction = getDynamicWind(direction,  time);
                entity->dynamicModel->windSpeed = Speed;//getDynamicSpeed(Speed,  time);
            }else{
                entity->dynamicModel->windSpeed = 0;
            }
        }
    }
}

QVector3D Specialzone::getDynamicWind(float baseAngle, float time) {

    float angleNoise = qCos(time * 0.8f) * 10.0f; // 10 degree tak ka variation
    float finalAngle = qDegreesToRadians(baseAngle + angleNoise);

    return QVector3D(
        qSin(finalAngle),
        0.0f,
        qCos(finalAngle)
        );
}

float Specialzone::getDynamicSpeed(float baseSpeed, float time){
    float speedNoise = qSin(time * 1.2f) * 0.7f + qSin(time * 3.5f) * 0.3f;
    float finalSpeed = baseSpeed + (speedNoise * (baseSpeed * 0.3f)); // 30% fluctuation
    float kmPerSec = finalSpeed / 3600.0f;
    return kmPerSec;
}

std::vector<std::string> Specialzone::getSupportedComponents() {
    std::vector<std::string> supported;
    supported.push_back("transform");
    supported.push_back("collider");
    supported.push_back("bitmap");
    return supported;
}

QJsonObject Specialzone::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;

    QJsonObject dimensionObj;
    dimensionObj["type"] = "Section";
    dimensionObj["direction"] = toParm(direction,"deg");
    dimensionObj["MinAltitude"] = toParm(MinAltitude,"ft");
    dimensionObj["MaxAltitude"] = toParm(MaxAltitude,"ft");
    dimensionObj["Speed"] = toParm(Speed,"km/hr");
    obj["dimension"] = dimensionObj;

    if (transform) obj["transform"] = transform->toJson();
    if (meshRenderer2d) obj["bitmap"] = meshRenderer2d->toJson();

    QJsonObject entityObj;
    entityObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : entityTypeOptions())
        optionsArray.append(opt);

    entityObj["options"] = optionsArray;
    entityObj["value"] = entityTypeToString(type);
    obj["type"] = entityObj;
    return obj;
}

void Specialzone::fromJson(const QJsonObject& obj) {
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();
    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();

    if (obj.contains("type") && obj["type"].isObject()) {
        QJsonObject entityObj = obj["type"].toObject();
        if (entityObj.contains("value"))
            type = stringToEntityType(entityObj["value"].toString());
    }


    if (obj.contains("dimension") && obj["dimension"].isObject()) {
        QJsonObject dimensionObj = obj["dimension"].toObject();
        if (dimensionObj.contains("direction") && dimensionObj["direction"].isObject())
            direction = valueFromParm(dimensionObj["direction"].toObject());
        if (dimensionObj.contains("MinAltitude") && dimensionObj["MinAltitude"].isObject())
            MinAltitude = valueFromParm(dimensionObj["MinAltitude"].toObject());
        if (dimensionObj.contains("MaxAltitude") && dimensionObj["MaxAltitude"].isObject())
            MaxAltitude = valueFromParm(dimensionObj["MaxAltitude"].toObject());
        if (dimensionObj.contains("Speed") && dimensionObj["Speed"].isObject())
            Speed = valueFromParm(dimensionObj["Speed"].toObject());
    }

    if (obj.contains("transform") && obj["transform"].isObject()) {
        if (!transform) addComponent("transform");
        transform->fromJson(obj["transform"].toObject());
    }


    if (obj.contains("bitmap") && obj["bitmap"].isObject()) { // Fix: Correct key
        if (!meshRenderer2d) addComponent("bitmap");
        meshRenderer2d->fromJson(obj["bitmap"].toObject());
    }

}

void Specialzone::addComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform){
            transform = new Transform();
            transform->parentEntity = this;
            parent->Components->insert({transform->ID, transform});
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(transform->ID), "transform");
        }
    }else if (name == "collider") {
        if (!collider) {
            if (!transform)
                addComponent("transform");
            collider = new Collider(parent);
            collider->CollideRadius = 20000;
            collider->parentEntity = this;
            collider->parentID = ID;
            parent->Components->insert({collider->ID, collider});
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(collider->ID), "collider");
            emit parent->entityPhysicsAdded(QString::fromStdString(parentID), this);
        }

    }else if (name == "bitmap") {
        if (!meshRenderer2d) {
            if (!transform)
                addComponent("transform");
            if (!collider)
                addComponent("collider");
            meshRenderer2d = new MeshRenderer2D();
            meshRenderer2d->parentEntity = this;
            parent->Components->insert({meshRenderer2d->ID, meshRenderer2d});
            meshRenderer2d->Sprite = new std::string(":/texture/images/Texture/zone.png");
            meshRenderer2d->Meshes[0]->Sprite = meshRenderer2d->Sprite;
            meshRenderer2d->Meshes[0]->clear();
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(meshRenderer2d->ID), "bitmap");
            emit parent->entityMeshAdded(QString::fromStdString(parentID), this);
        }

    }
}

void Specialzone::removeComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
}

QJsonObject Specialzone::getComponent(std::string name) {
    if (name == "transform") {
        if (!transform) { Console::error(name + ": not exist"); return QJsonObject(); }
        return transform->toJson();
    } else if (name == "collider") {
        if (!collider) { Console::error(name + ": not exist"); return QJsonObject(); }
        return collider->toJson();
    } else if (name == "bitmap") {
        if (!meshRenderer2d) { Console::error(name + ": not exist"); return QJsonObject(); }
        return meshRenderer2d->toJson();
    }

    return QJsonObject();
}

void Specialzone::updateComponent(QString name, const QJsonObject& obj) {
    if (name == "transform") {
        if (!transform) { Console::error(name.toStdString() + ": not exist"); return; }
        transform->fromJson(obj);
    } else if (name == "collider") {
        if (!collider) { Console::error(name.toStdString() + ": not exist"); return; }
        collider->fromJson(obj);
    } else if (name == "bitmap") {
        if (!meshRenderer2d) { Console::error(name.toStdString() + ": not exist"); return; }
        meshRenderer2d->fromJson(obj);
    }
}
