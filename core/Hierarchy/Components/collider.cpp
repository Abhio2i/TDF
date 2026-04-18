
#include "collider.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qjsonarray.h"
#include <QMetaEnum>
#include <core/Debug/console.h>
#include "core/Hierarchy/hierarchy.h"

Collider::Collider(Hierarchy* h):Component(h) {
    Active = true;
    CollideRadius = 200;
    WarningRadius = 100;
    Width = 1;
    Length = 1;
    Height = 1;
    collider = Constants::ColliderType::Box;
    vector = nullptr; // Initialize pointer
    type = Constants::EntityType::Platform; // Use Platform as default
}

void Collider::Update(float deltaTime){
    if(parentEntity && parentEntity->type == Constants::EntityType::Platform && parentEntity->root){
        Transform* source =parentEntity->root->Platforms[parentEntity->ID]->transform;
        if(!source)return;
        for (auto& [key, entity] : parentEntity->root->Platforms){
            if(!entity || !entity->Active || entity->isDestroy || key == parentEntity->ID || !entity->transform) continue;
            float distance = source->matrix->translation().distanceToPoint(entity->transform->matrix->translation())*1000;
            // qDebug()<<distance;
            if(distance<WarningRadius){
                parentEntity->collisionWarning = true;
                // entity->isDestroy = true;
            }else{
                parentEntity->collisionWarning = false;
            }
        }
    }
}

QString colliderTypeToString(Constants::ColliderType type) {
    switch (type) {
    case Constants::ColliderType::Box: return "Box";
    case Constants::ColliderType::Sphere: return "Sphere";
    case Constants::ColliderType::Cyclinder: return "Cyclinder"; // Note: Typo, consider fixing to Cylinder
    default: return "Box";
    }
}

Constants::ColliderType stringToColliderType(const QString& str) {
    if (str == "Box") return Constants::ColliderType::Box;
    if (str == "Sphere") return Constants::ColliderType::Sphere;
    if (str == "Cyclinder") return Constants::ColliderType::Cyclinder;
    return Constants::ColliderType::Box; // Default
}

QStringList colliderTypeOptions() {
    QStringList list;
    int index = Constants::staticMetaObject.indexOfEnumerator("ColliderType");
    QMetaEnum metaEnum = Constants::staticMetaObject.enumerator(index);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        list << QString::fromLatin1(metaEnum.key(i));
    }
    return list;
}

void Collider::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){

}

void Collider::removeSubComponent(std::string ID){

}

void Collider::updateSubComponent(std::string ID, const QJsonObject& obj){

}

QJsonObject Collider::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

QJsonObject Collider::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    obj["type"] = "component";

    QJsonObject dimensionObj;
    dimensionObj["type"] = "Section";
    dimensionObj["CollideRadius"] = toParm(CollideRadius,"m", 0, 10000);
    dimensionObj["WarningRadius"] = toParm(WarningRadius,"m", 0, 10000);
    dimensionObj["width"] = toParm(Width,"m", 0, 500);
    dimensionObj["length"] = toParm(Length,"m", 0, 500);
    dimensionObj["height"] = toParm(Height,"m", 0, 500);
    obj["dimension"] = dimensionObj;


    QJsonObject colliderObj;
    colliderObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : colliderTypeOptions())
        optionsArray.append(opt);
    colliderObj["options"] = optionsArray;
    colliderObj["value"] = colliderTypeToString(collider);
    obj["collider"] = colliderObj;

    // Include custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    //Console::log("Collider::toJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
    //Console::log("Collider::toJson output: " + QString(QJsonDocument(obj).toJson()).toStdString());
    return obj;
}

void Collider::fromJson(const QJsonObject& obj) {
    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("dimension") && obj["dimension"].isObject()) {
        QJsonObject dimensionObj = obj["dimension"].toObject();
        if (dimensionObj.contains("CollideRadius") && dimensionObj["CollideRadius"].isObject())
            CollideRadius = valueFromParm(dimensionObj["CollideRadius"].toObject());
        if (dimensionObj.contains("WarningRadius") && dimensionObj["WarningRadius"].isObject())
            WarningRadius = valueFromParm(dimensionObj["WarningRadius"].toObject());
        if (dimensionObj.contains("width") && dimensionObj["width"].isObject())
            Width = valueFromParm(dimensionObj["width"].toObject());
        if (dimensionObj.contains("length") && dimensionObj["length"].isObject())
            Length = valueFromParm(dimensionObj["length"].toObject());
        if (dimensionObj.contains("height") && dimensionObj["height"].isObject())
            Height = valueFromParm(dimensionObj["height"].toObject());
    }

    if (obj.contains("collider") && obj["collider"].isObject()) {
        QJsonObject colliderObj = obj["collider"].toObject();
        if (colliderObj.contains("value"))
            collider = stringToColliderType(colliderObj["value"].toString());
    }

    // Merge custom parameters
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key() != "active" && it.key() != "radius" && it.key() != "width" &&
            it.key() != "length" && it.key() != "height" && it.key() != "collider" && it.key() != "dimension") {
            customParameters[it.key()] = it.value();
        }
    }
    //Console::log("Collider::fromJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
}
