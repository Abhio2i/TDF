

#include "platform.h"
#include "qjsonarray.h"
#include "qmetaobject.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/Struct/parameter.h"
#include "core/GlobalRegistry.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>

Platform::Platform(Hierarchy* h) : Entity(h) {
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "roll";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 1.0f;
    parameters["roll"] = par;
}

void Platform::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));

}

std::vector<Component*> Platform::getSupportedComponents() {
    std::vector<Component*> supported;
    if (transform) supported.push_back(transform);
    if (trajectory) supported.push_back(trajectory);
    if (rigidbody) supported.push_back(rigidbody);
    if (dynamicModel) supported.push_back(dynamicModel);
    if (collider) supported.push_back(collider);
    if (networkObject) supported.push_back(networkObject);
    if (meshRenderer2d) supported.push_back(meshRenderer2d);
    if (mission) supported.push_back(mission);
    if (radios) supported.push_back(radios);
    if (weopons) supported.push_back(weopons);
    if (radar) supported.push_back(radar);
    return supported;
}

QJsonObject Platform::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;
    QJsonObject paramMap;
    for (const auto& [key, param] : parameters) {
        if (param) {
            paramMap[QString::fromStdString(key)] = param->toJson();
        }
    }

    QJsonObject parObj;
    parObj["type"] = "parameter";
    parObj["value"] = paramMap;
    obj["parameters"] = parObj;

    if (transform) obj["transform"] = transform->toJson();
    if (trajectory) obj["trajectory"] = trajectory->toJson();
    if (rigidbody) obj["rigidbody"] = rigidbody->toJson();
    if (dynamicModel) obj["dynamicModel"] = dynamicModel->toJson();
    if (collider) obj["collider"] = collider->toJson();
    if (meshRenderer2d) obj["meshRenderer2d"] = meshRenderer2d->toJson();
    if (radios) obj["radios"] = radios->toJson();
    if (weopons) obj["weopons"] = weopons->toJson();
    if (radar) obj["radar"] = radar->toJson();

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

void Platform::fromJson(const QJsonObject& obj) {
    Name = obj["name"].toString().toStdString();
    ID = obj["id"].toString().toStdString();
    parentID = obj["parent_id"].toString().toStdString();
    Active = obj["active"].toBool();

    if (obj.contains("parameters")) {
        QJsonObject parObj = obj["parameters"].toObject();
        if (parObj.contains("array")) {
            QJsonObject paramMap = obj["array"].toObject();
            for (const QString& key : paramMap.keys()) {
                QJsonObject paramObj = paramMap[key].toObject();
                std::shared_ptr<Parameter> param = std::make_shared<Parameter>();
                param->fromJson(paramObj);
                parameters[key.toStdString()] = param;
            }
        }
    }

    if (obj.contains("type") && obj["type"].isObject()) {
        QJsonObject entityObj = obj["type"].toObject();
        if (entityObj.contains("value"))
            type = stringToEntityType(entityObj["value"].toString());
    }

    if (obj.contains("transform") && obj["transform"].isObject()) {
        if (!transform) addComponent("transform");
        transform->fromJson(obj["transform"].toObject());
    }

    if (obj.contains("trajectory") && obj["trajectory"].isObject()) {
        if (!trajectory) addComponent("trajectory");
        trajectory->fromJson(obj["trajectory"].toObject());
    }

    if (obj.contains("rigidbody") && obj["rigidbody"].isObject()) {
        if (!rigidbody) addComponent("rigidbody");
        rigidbody->fromJson(obj["rigidbody"].toObject());
    }

    if (obj.contains("dynamicModel") && obj["dynamicModel"].isObject()) {
        if (!dynamicModel) addComponent("dynamicModel");
        dynamicModel->fromJson(obj["dynamicModel"].toObject());
    }

    if (obj.contains("collider") && obj["collider"].isObject()) {
        if (!collider) addComponent("collider");
        collider->fromJson(obj["collider"].toObject());
    }

    if (obj.contains("meshRenderer2d") && obj["meshrenderer2d"].isObject()) {
        if (!meshRenderer2d) addComponent("meshRenderer2d");
        meshRenderer2d->fromJson(obj["meshRenderer2d"].toObject());
    }

    if (obj.contains("radios") && obj["radios"].isObject()) {
        if (!radios) addComponent("radios");
        radios->fromJson(obj["radios"].toObject());
    }

    if (obj.contains("weopons") && obj["weopons"].isObject()) {
        if (!weopons) addComponent("weopons");
        weopons->fromJson(obj["weopons"].toObject());
    }

    if (obj.contains("radar") && obj["radar"].isObject()) {
        if (!radar) addComponent("radar");
        radar->fromJson(obj["radar"].toObject());
    }
}

void Platform::addComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform) transform = new Transform();
        emit parent->componentAdded(QString::fromStdString(ID), "transform");
    } else if (name == "trajectory") {
        if (!trajectory) trajectory = new Trajectory();
        emit parent->componentAdded(QString::fromStdString(ID), "trajectory");
    } else if (name == "rigidbody") {
        if (!rigidbody) {
            if (!transform) addComponent("transform");
            rigidbody = new Rigidbody();
        }
        emit parent->componentAdded(QString::fromStdString(ID), "rigidbody");
    } else if (name == "dynamicModel") {
        if (!dynamicModel) {
            if (!transform) addComponent("transform");
            if (!rigidbody) addComponent("rigidbody");
            if (!collider) addComponent("collider");
            dynamicModel = new DynamicModel();
            dynamicModel->transform = transform;
            dynamicModel->rigidbody = rigidbody;
        }
        emit parent->componentAdded(QString::fromStdString(ID), "dynamicModel");
        emit parent->entityPhysicsAdded(QString::fromStdString(parentID), this);
    } else if (name == "collider") {
        if (!collider) {
            if (!transform) addComponent("transform");
            collider = new Collider();
        }
        emit parent->componentAdded(QString::fromStdString(ID), "collider");
    } else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) {
            if (!transform) addComponent("transform");
            if (!collider) addComponent("collider");
            meshRenderer2d = new MeshRenderer2D();
        }
        emit parent->componentAdded(QString::fromStdString(ID), "meshRenderer2d");
        emit parent->entityMeshAdded(QString::fromStdString(parentID), this);
    } else if (name == "radios") {
        if (!radios) {
            if (!transform) addComponent("transform");
            radios = new AttachedEnitities();
        }
        emit parent->componentAdded(QString::fromStdString(ID), "radios");
    } else if (name == "weopons") {
        if (!weopons) {
            if (!transform) addComponent("transform");
            weopons = new AttachedEnitities();
        }
        emit parent->componentAdded(QString::fromStdString(ID), "weopons");
    } else if (name == "radar") {
        if (!radar) {
            if (!transform) addComponent("transform");
            radar = new AttachedEnitities();
        }
        emit parent->componentAdded(QString::fromStdString(ID), "radar");
    }
}

void Platform::removeComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform) return;
        delete transform;
        transform = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "transform");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "trajectory") {
        if (!trajectory) return;
        delete trajectory;
        trajectory = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "trajectory");
    } else if (name == "rigidbody") {
        if (!rigidbody) return;
        delete rigidbody;
        rigidbody = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "rigidbody");
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "dynamicModel") {
        if (!dynamicModel) return;
        delete dynamicModel;
        dynamicModel = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "dynamicModel");
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "collider") {
        if (!collider) return;
        delete collider;
        collider = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "collider");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) return;
        delete meshRenderer2d;
        meshRenderer2d = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "meshRenderer2d");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
    }
}

QJsonObject Platform::getComponent(std::string name) {
    if (name == "transform") {
        if (!transform) { Console::error(name + ": not exist"); return QJsonObject(); }
        return transform->toJson();
    } else if (name == "trajectory") {
        if (!trajectory) { Console::error(name + ": not exist"); return QJsonObject(); }
        return trajectory->toJson();
    } else if (name == "rigidbody") {
        if (!rigidbody) { Console::error(name + ": not exist"); return QJsonObject(); }
        return rigidbody->toJson();
    } else if (name == "dynamicModel") {
        if (!dynamicModel) { Console::error(name + ": not exist"); return QJsonObject(); }
        return dynamicModel->toJson();
    } else if (name == "collider") {
        if (!collider) { Console::error(name + ": not exist"); return QJsonObject(); }
        return collider->toJson();
    } else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) { Console::error(name + ": not exist"); return QJsonObject(); }
        return meshRenderer2d->toJson();
    } else if (name == "radios") {
        if (!radios) { Console::error(name + ": not exist"); return QJsonObject(); }
        return radios->toJson();
    } else if (name == "weopons") {
        if (!weopons) { Console::error(name + ": not exist"); return QJsonObject(); }
        return weopons->toJson();
    } else if (name == "radar") {
        if (!radar) { Console::error(name + ": not exist"); return QJsonObject(); }
        return radar->toJson();
    }
    return QJsonObject();
}

void Platform::updateComponent(QString name, const QJsonObject& obj) {
    if (name == "transform") {
        if (!transform) { Console::error(name.toStdString() + ": not exist"); return; }
        transform->fromJson(obj);
    } else if (name == "trajectory") {
        if (!trajectory) { Console::error(name.toStdString() + ": not exist"); return; }
        trajectory->fromJson(obj);
    } else if (name == "rigidbody") {
        if (!rigidbody) { Console::error(name.toStdString() + ": not exist"); return; }
        rigidbody->fromJson(obj);
    } else if (name == "dynamicModel") {
        if (!dynamicModel) { Console::error(name.toStdString() + ": not exist"); return; }
        dynamicModel->fromJson(obj);
    } else if (name == "collider") {
        if (!collider) { Console::error(name.toStdString() + ": not exist"); return; }
        collider->fromJson(obj);
    } else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) { Console::error(name.toStdString() + ": not exist"); return; }
        meshRenderer2d->fromJson(obj);
    } else if (name == "radios") {
        if (!radios) { Console::error(name.toStdString() + ": not exist"); return; }
        radios->fromJson(obj);
    } else if (name == "weopons") {
        if (!weopons) { Console::error(name.toStdString() + ": not exist"); return; }
        weopons->fromJson(obj);
    } else if (name == "radar") {
        if (!radar) { Console::error(name.toStdString() + ": not exist"); return; }
        radar->fromJson(obj);
    }
}
