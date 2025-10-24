#include "fixedpoints.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "qjsonarray.h"
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>

FixedPoints::FixedPoints(Hierarchy *h):Entity(h) {

}



void FixedPoints::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
    addComponent("transform");
    addComponent("collider");
    addComponent("meshRenderer2d");
}

std::vector<std::string>FixedPoints:: getSupportedComponents() {
    return std::vector<std::string>{};
}


QJsonObject FixedPoints::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;



    if (transform) obj["transform"] = transform->toJson();
    if (collider) obj["collider"] = collider->toJson();
    if (meshRenderer2d) obj["meshRenderer2d"] = meshRenderer2d->toJson();

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

void FixedPoints::fromJson(const QJsonObject& obj) {
    Name = obj["name"].toString().toStdString();
    ID = obj["id"].toString().toStdString();
    parentID = obj["parent_id"].toString().toStdString();
    Active = obj["active"].toBool();

    if (obj.contains("type") && obj["type"].isObject()) {
        QJsonObject entityObj = obj["type"].toObject();
        if (entityObj.contains("value"))
            type = stringToEntityType(entityObj["value"].toString());
    }

    if (obj.contains("transform") && obj["transform"].isObject()) {
        if (!transform) addComponent("transform");
        transform->fromJson(obj["transform"].toObject());
    }

    if (obj.contains("collider") && obj["collider"].isObject()) {
        if (!collider) addComponent("collider");
        collider->fromJson(obj["collider"].toObject());
    }

    if (obj.contains("meshRenderer2d") && obj["meshRenderer2d"].isObject()) { // Fix: Correct key
        if (!meshRenderer2d) addComponent("meshRenderer2d");
        meshRenderer2d->fromJson(obj["meshRenderer2d"].toObject());
    }
}

void FixedPoints::addComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform){
            transform = new Transform();
            emit parent->componentAdded(QString::fromStdString(ID), "transform");
        }
    }
    else if (name == "collider") {
        if (!collider) {
            if (!transform)
                addComponent("transform");
            collider = new Collider();
            emit parent->componentAdded(QString::fromStdString(ID), "collider");
        }

    }
    else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) {
            if (!transform)
                addComponent("transform");
            meshRenderer2d = new MeshRenderer2D();
            meshRenderer2d->Sprite = new std::string(":/texture/images/Texture/marker.png");
            meshRenderer2d->Meshes[0]->Sprite = meshRenderer2d->Sprite;
            meshRenderer2d->Meshes[0]->clear();
            emit parent->componentAdded(QString::fromStdString(ID), "meshRenderer2d");
            emit parent->entityMeshAdded(QString::fromStdString(parentID), this);
        }

    }
}

void FixedPoints::removeComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
}

QJsonObject FixedPoints::getComponent(std::string name) {
    if (name == "transform") {
        if (!transform) { Console::error(name + ": not exist"); return QJsonObject(); }
        return transform->toJson();
    } else if (name == "collider") {
        if (!collider) { Console::error(name + ": not exist"); return QJsonObject(); }
        return collider->toJson();
    } else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) { Console::error(name + ": not exist"); return QJsonObject(); }
        return meshRenderer2d->toJson();
    }

    return QJsonObject();
}

void FixedPoints::updateComponent(QString name, const QJsonObject& obj) {
    if (name == "transform") {
        if (!transform) { Console::error(name.toStdString() + ": not exist"); return; }
        transform->fromJson(obj);
    } else if (name == "collider") {
        if (!collider) { Console::error(name.toStdString() + ": not exist"); return; }
        collider->fromJson(obj);
    } else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) { Console::error(name.toStdString() + ": not exist"); return; }
        meshRenderer2d->fromJson(obj);
    }
}
