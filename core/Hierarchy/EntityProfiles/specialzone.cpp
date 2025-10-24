#include "specialzone.h"

#include "qjsonarray.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/GlobalRegistry.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>
Specialzone::Specialzone(Hierarchy* h) : Entity(h) {

}

void Specialzone::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));

}


std::vector<std::string> Specialzone::getSupportedComponents() {
    std::vector<std::string> supported;

    return supported;
}

QJsonObject Specialzone::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;



    if (transform) obj["transform"] = transform->toJson();
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

void Specialzone::fromJson(const QJsonObject& obj) {
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


    if (obj.contains("meshRenderer2d") && obj["meshRenderer2d"].isObject()) { // Fix: Correct key
        if (!meshRenderer2d) addComponent("meshRenderer2d");
        meshRenderer2d->fromJson(obj["meshRenderer2d"].toObject());
    }

}

void Specialzone::addComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform){
            transform = new Transform();
            emit parent->componentAdded(QString::fromStdString(ID), "transform");
        }
    }else if (name == "collider") {
        if (!collider) {
            if (!transform)
                addComponent("transform");
            collider = new Collider();
            emit parent->componentAdded(QString::fromStdString(ID), "collider");
        }

    }else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) {
            if (!transform)
                addComponent("transform");
            if (!collider)
                addComponent("collider");
            meshRenderer2d = new MeshRenderer2D();
            meshRenderer2d->Sprite = new std::string(":/texture/images/Texture/zone.png");
            meshRenderer2d->Meshes[0]->Sprite = meshRenderer2d->Sprite;
            meshRenderer2d->Meshes[0]->clear();
            emit parent->componentAdded(QString::fromStdString(ID), "meshRenderer2d");
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
    } else if (name == "meshRenderer2d") {
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
    } else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) { Console::error(name.toStdString() + ": not exist"); return; }
        meshRenderer2d->fromJson(obj);
    }
}
