/**
 * @file fixedpoints.cpp
 * @brief Implementation of the FixedPoints entity for static geographic markers.
 */

#include "fixedpoints.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "qjsonarray.h"
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>

/**
 * @brief Constructs a FixedPoints entity.
 * @param h Pointer to the parent Hierarchy.
 */
FixedPoints::FixedPoints(Hierarchy *h):Entity(h) {
    type = Constants::EntityType::FixedPoint;
}

/**
 * @brief Emits signals to notify the hierarchy that this entity has been added.
 */
void FixedPoints::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

/**
 * @brief Returns a list of component names supported by FixedPoints.
 * @return Vector containing "transform", "collider", "bitmap".
 */
std::vector<std::string> FixedPoints::getSupportedComponents() {
    std::vector<std::string> supported;
    supported.push_back("transform");
    supported.push_back("collider");
    supported.push_back("bitmap");
    return supported;
}

/**
 * @brief Serializes the FixedPoints entity to JSON.
 * @return QJsonObject containing transform, collider, and bitmap data.
 */
QJsonObject FixedPoints::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;

    if (transform) obj["transform"] = transform->toJson();
    if (collider) obj["collider"] = collider->toJson();
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

/**
 * @brief Deserializes the FixedPoints entity from JSON.
 * @param obj JSON object containing entity data.
 */
void FixedPoints::fromJson(const QJsonObject& obj) {
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

    if (obj.contains("transform") && obj["transform"].isObject()) {
        if (!transform) addComponent("transform");
        transform->fromJson(obj["transform"].toObject());
    }

    if (obj.contains("collider") && obj["collider"].isObject()) {
        if (!collider) addComponent("collider");
        collider->fromJson(obj["collider"].toObject());
    }

    if (obj.contains("bitmap") && obj["bitmap"].isObject()) { // Correct key
        if (!meshRenderer2d) addComponent("bitmap");
        meshRenderer2d->fromJson(obj["bitmap"].toObject());
    }
}

/**
 * @brief Adds a component by name to the FixedPoints entity.
 * @param name Component name ("transform", "collider", or "bitmap").
 */
void FixedPoints::addComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform){
            transform = new Transform();
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(transform->ID), "transform");
        }
    }
    else if (name == "collider") {
        if (!collider) {
            if (!transform)
                addComponent("transform");
            collider = new Collider(parent);
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(collider->ID), "collider");
        }

    }
    else if (name == "bitmap") {
        if (!meshRenderer2d) {
            if (!transform)
                addComponent("transform");
            if (!collider)
                addComponent("collider");
            meshRenderer2d = new MeshRenderer2D();
            meshRenderer2d->Sprite = new std::string(":/texture/images/Texture/marker.png");
            meshRenderer2d->Meshes[0]->Sprite = meshRenderer2d->Sprite;
            meshRenderer2d->Meshes[0]->clear();
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(meshRenderer2d->ID), "bitmap");
            emit parent->entityMeshAdded(QString::fromStdString(parentID), this);
        }

    }
}

/**
 * @brief Removes a component by name.
 * @param name Component name.
 */
void FixedPoints::removeComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform) return;
        parent->Components.erase(transform->ID);
        delete transform;
        transform = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "transform");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "collider") {
        if (!collider) return;
        parent->Components.erase(collider->ID);
        delete collider;
        collider = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "collider");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "bitmap") {
        if (!meshRenderer2d) return;
        parent->Components.erase(meshRenderer2d->ID);
        delete meshRenderer2d;
        meshRenderer2d = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "bitmap");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
    }
}

/**
 * @brief Retrieves a component's JSON data by name.
 * @param name Component name.
 * @return QJsonObject representing the component, or empty if not found.
 */
QJsonObject FixedPoints::getComponent(std::string name) {
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

/**
 * @brief Updates a component from JSON data.
 * @param name Component name.
 * @param obj JSON object containing new data.
 */
void FixedPoints::updateComponent(QString name, const QJsonObject& obj) {
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
