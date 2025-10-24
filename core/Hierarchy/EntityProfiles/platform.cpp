

#include "platform.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include "qjsonarray.h"
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

void Platform::update(){
    //qDebug()<<"update";
    for (Sensor* s : sensorList) {
        if (s) {
            s->scan(ID,transform);
            s->ewscan(ID,transform);
           // qDebug()<<"found sensor";
        }
    }
}


void Platform::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));

}

void Platform::addParam(std::string key,std::string value){
    customParameters[QString::fromStdString(key)] = QString::fromStdString(value);
}

void Platform::editParam(std::string key,std::string value){
    customParameters[QString::fromStdString(key)] = QString::fromStdString(value);
}

std::string Platform::getParam(std::string key){
    return customParameters[QString::fromStdString(key)].toString().toStdString();
}

void Platform::removeParam(std::string key){
    customParameters.remove(QString::fromStdString(key));
}


std::vector<std::string> Platform::getSupportedComponents() {
    std::vector<std::string> supported;
    supported.push_back("transform");
    supported.push_back("trajectory");
    supported.push_back("rigidbody");
    supported.push_back("dynamicModel");
    supported.push_back("collider");
    supported.push_back("networkObject");
    supported.push_back("meshRenderer2d");
    supported.push_back("mission");
    supported.push_back("radios");
    supported.push_back("sensors");
    supported.push_back("iff");
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
    QJsonArray radioArray;
    for (Radio* r : radioList) {
        if (r) radioArray.append(r->toJson());
    }
    obj["radios"] = radioArray;

    QJsonArray sensorArray;
    for (Sensor* s : sensorList) {
        if (s) sensorArray.append(s->toJson());
    }
    obj["sensors"] = sensorArray;

    QJsonArray iffArray;
    for (IFF* i : iffList) {
        if (i) iffArray.append(i->toJson());
    }
    obj["iffs"] = iffArray;

    QJsonObject entityObj;
    entityObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : entityTypeOptions())
        optionsArray.append(opt);

    entityObj["options"] = optionsArray;
    entityObj["value"] = entityTypeToString(type);
    obj["type"] = entityObj;

    // Include custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    return obj;
}


void Platform::fromJson(const QJsonObject& obj) {

    Name = obj["name"].toString().toStdString();
    ID = obj["id"].toString().toStdString();
    parentID = obj["parent_id"].toString().toStdString();
    Active = obj["active"].toBool();
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (obj.contains("parameters")) {
        QJsonObject parObj = obj["parameters"].toObject();
        if (parObj.contains("value")) { // Fix: Check "value" instead of "array"
            QJsonObject paramMap = parObj["value"].toObject();
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

    if (obj.contains("meshRenderer2d") && obj["meshRenderer2d"].isObject()) { // Fix: Correct key
        if (!meshRenderer2d) addComponent("meshRenderer2d");
        meshRenderer2d->fromJson(obj["meshRenderer2d"].toObject());
    }

    if (obj.contains("radios") && obj["radios"].isArray()) {
        QJsonArray arr = obj["radios"].toArray();
        for (const QJsonValue& val : arr) {
            Radio* r = new Radio(parent);
            r->fromJson(val.toObject());
            radioList.push_back(r);
        }

        addComponent("radios");
    }

    if (obj.contains("sensors") && obj["sensors"].isArray()) {
        QJsonArray arr = obj["sensors"].toArray();
        for (const QJsonValue& val : arr) {
            Sensor* s = new Sensor(parent);
            s->fromJson(val.toObject());
            sensorList.push_back(s);
        }

        addComponent("sensors");
    }

    if (obj.contains("iffs") && obj["iffs"].isArray()) {
        QJsonArray arr = obj["iffs"].toArray();
        for (const QJsonValue& val : arr) {
            IFF* i = new IFF(parent);
            i->fromJson(val.toObject());
            iffList.push_back(i);
        }
        addComponent( "iff");

    }

    // Merge custom parameters
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key() != "name" &&
            it.key() != "id" &&
            it.key() != "parent_id" &&
            it.key() != "active" &&
            it.key() != "parameters" &&
            it.key() != "type" &&
            it.key() != "transform" &&
            it.key() != "trajectory" &&
            it.key() != "rigidbody" &&
            it.key() != "dynamicModel" &&
            it.key() != "collider" &&
            it.key() != "meshRenderer2d" &&
            it.key() != "radios" &&
            it.key() != "sensors" &&
            it.key() != "iffs" &&
            it.key() != "parent_id") {
            customParameters[it.key()] = it.value();
        }
    }

}


void Platform::addComponent(std::string name) {
     Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform){
            transform = new Transform();
            emit parent->componentAdded(QString::fromStdString(ID), "transform");
        }
    } else if (name == "trajectory") {
        if (!trajectory){
            trajectory = new Trajectory();
            emit parent->componentAdded(QString::fromStdString(ID), "trajectory");
        }
    } else if (name == "rigidbody") {
        if (!rigidbody) {
            if (!transform)
                addComponent("transform");
            rigidbody = new Rigidbody();
            emit parent->componentAdded(QString::fromStdString(ID), "rigidbody");
        }

    } else if (name == "dynamicModel") {
        if (!dynamicModel) {
            if (!transform)
                addComponent("transform");
            if (!rigidbody)
                addComponent("rigidbody");
            if (!collider)
                addComponent("collider");
            if (!trajectory)
                addComponent("trajectory");
            dynamicModel = new DynamicModel();
            dynamicModel->transform = transform;
            dynamicModel->rigidbody = rigidbody;
            dynamicModel->trajectory = trajectory;
            emit parent->componentAdded(QString::fromStdString(ID), "dynamicModel");
            emit parent->entityPhysicsAdded(QString::fromStdString(parentID), this);
        }

    } else if (name == "collider") {
        if (!collider) {
            if (!transform)
                addComponent("transform");
            collider = new Collider();
            emit parent->componentAdded(QString::fromStdString(ID), "collider");
        }

    } else if (name == "meshRenderer2d") {
        if (!meshRenderer2d) {
            if (!transform)
                addComponent("transform");
            if (!collider)
                addComponent("collider");
            meshRenderer2d = new MeshRenderer2D();
            emit parent->componentAdded(QString::fromStdString(ID), "meshRenderer2d");
            emit parent->entityMeshAdded(QString::fromStdString(parentID), this);
        }

    }else if (name == "radios") {
        emit parent->componentAdded(QString::fromStdString(ID), "radios");
    }else if (name == "sensors") {
        emit parent->componentAdded(QString::fromStdString(ID), "sensors");
    }else if (name == "iff") {
        emit parent->componentAdded(QString::fromStdString(ID), "iff");
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
    }
    else if (name == "iff") {
       //if (iffList) { Console::error(name + ": not exist"); return QJsonObject(); }
       //return ififfListfs->toJson();
       QJsonObject obj;
       obj["id"] = QString::fromStdString(ID);
       obj["active"] = Active;
       obj["type"] = "component";
       QJsonArray iffArray;
       for (IFF* i : iffList) {
           if (i) iffArray.append(i->toJson());
       }
       obj["iffs"] = iffArray;
       return obj;
    }
    else if (name == "radios") {

        QJsonObject obj;
        obj["id"] = QString::fromStdString(ID);
        obj["active"] = Active;
        obj["type"] = "component";
        QJsonArray radioArray;
        for (Radio* r : radioList) {
            if (r) radioArray.append(r->toJson());
        }
        obj["radios"] = radioArray;
        return obj;
    }


    else if (name == "sensors") {

        QJsonObject obj;
        obj["id"] = QString::fromStdString(ID);
        obj["active"] = Active;
        obj["type"] = "component";
        QJsonArray sensorArray;
        for (Sensor* r : sensorList) {
            if (r) sensorArray.append(r->toJson());
        }
        obj["sensors"] = sensorArray;
        return obj;
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
    }
}

