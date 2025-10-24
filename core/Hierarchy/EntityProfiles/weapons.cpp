

#include "weapons.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>

// Missile Implementation
Missile::Missile(Hierarchy* h) : Entity(h) {
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "missile_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["missile_param"] = par;
}

void Missile::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

std::vector<std::string> Missile::getSupportedComponents() {
    return std::vector<std::string>{};
}

void Missile::addComponent(std::string name) {
    Console::error("Missile does not support components: " + name);
}

void Missile::removeComponent(std::string name) {
    Console::error("Missile does not support components: " + name);
}

QJsonObject Missile::getComponent(std::string name) {
    Console::error("Missile does not support components: " + name);
    return QJsonObject();
}

void Missile::updateComponent(QString name, const QJsonObject& /*obj*/) {
    Console::error(name.toStdString() + ": Missile does not support components");
}

QJsonObject Missile::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;

    // Serialize parameters
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

    // Serialize missile attributes
    QJsonObject locObj;
    locObj["x"] = locationOffset.x;
    locObj["y"] = locationOffset.y;
    locObj["z"] = locationOffset.z;
    obj["locationOffset"] = locObj;
    obj["thrust"] = thrust;
    obj["burnTime"] = burnTime;
    obj["maxSpeed"] = maxSpeed;
    obj["acceleration"] = acceleration;
    obj["payload"] = payload;
    obj["proximityRange"] = proximityRange;
    obj["range"] = range;
    obj["guidingSensor"] = guidingSensorToString(guidingSensor);
    obj["seekerLockOnTime"] = seekerLockOnTime;
    obj["eccmCapability"] = eccmCapability;
    obj["launchPlatformType"] = launchPlatformTypeToString(launchPlatformType);
    obj["controlSystem"] = controlSystemToString(controlSystem);

    return obj;
}

void Missile::fromJson(const QJsonObject& obj) {
    Name = obj["name"].toString().toStdString();
    ID = obj["id"].toString().toStdString();
    parentID = obj["parent_id"].toString().toStdString();
    Active = obj["active"].toBool();

    // Deserialize parameters
    if (obj.contains("parameters")) {
        QJsonObject parObj = obj["parameters"].toObject();
        if (parObj.contains("value")) {
            QJsonObject paramMap = parObj["value"].toObject();
            for (const QString& key : paramMap.keys()) {
                QJsonObject paramObj = paramMap[key].toObject();
                std::shared_ptr<Parameter> param = std::make_shared<Parameter>();
                param->fromJson(paramObj);
                parameters[key.toStdString()] = param;
            }
        }
    }

    // Deserialize missile attributes
    if (obj.contains("locationOffset") && obj["locationOffset"].isObject()) {
        QJsonObject locObj = obj["locationOffset"].toObject();
        locationOffset.x = static_cast<float>(locObj["x"].toDouble());
        locationOffset.y = static_cast<float>(locObj["y"].toDouble());
        locationOffset.z = static_cast<float>(locObj["z"].toDouble());
    }
    thrust = static_cast<float>(obj["thrust"].toDouble());
    burnTime = static_cast<float>(obj["burnTime"].toDouble());
    maxSpeed = static_cast<float>(obj["maxSpeed"].toDouble());
    acceleration = static_cast<float>(obj["acceleration"].toDouble());
    payload = static_cast<float>(obj["payload"].toDouble());
    proximityRange = static_cast<float>(obj["proximityRange"].toDouble());
    range = static_cast<float>(obj["range"].toDouble());
    guidingSensor = stringToGuidingSensor(obj["guidingSensor"].toString());
    seekerLockOnTime = static_cast<float>(obj["seekerLockOnTime"].toDouble());
    eccmCapability = obj["eccmCapability"].toBool();
    launchPlatformType = stringToLaunchPlatformType(obj["launchPlatformType"].toString());
    controlSystem = stringToControlSystem(obj["controlSystem"].toString());
}

QString Missile::guidingSensorToString(GuidingSensor gs) const {
    switch (gs) {
    case GuidingSensor::ActiveRadar: return "ActiveRadar";
    case GuidingSensor::Infrared: return "Infrared";
    case GuidingSensor::PassiveRadar: return "PassiveRadar";
    case GuidingSensor::GPS: return "GPS";
    case GuidingSensor::Optical: return "Optical";
    default: return "ActiveRadar";
    }
}

Missile::GuidingSensor Missile::stringToGuidingSensor(const QString& str) const {
    if (str == "Infrared") return GuidingSensor::Infrared;
    if (str == "PassiveRadar") return GuidingSensor::PassiveRadar;
    if (str == "GPS") return GuidingSensor::GPS;
    if (str == "Optical") return GuidingSensor::Optical;
    return GuidingSensor::ActiveRadar;
}

QString Missile::launchPlatformTypeToString(LaunchPlatformType lpt) const {
    switch (lpt) {
    case LaunchPlatformType::Air: return "Air";
    case LaunchPlatformType::Ground: return "Ground";
    case LaunchPlatformType::Sea: return "Sea";
    default: return "Air";
    }
}

Missile::LaunchPlatformType Missile::stringToLaunchPlatformType(const QString& str) const {
    if (str == "Ground") return LaunchPlatformType::Ground;
    if (str == "Sea") return LaunchPlatformType::Sea;
    return LaunchPlatformType::Air;
}

QString Missile::controlSystemToString(ControlSystem cs) const {
    switch (cs) {
    case ControlSystem::Canard: return "Canard";
    case ControlSystem::ThrustVectoring: return "ThrustVectoring";
    case ControlSystem::Fins: return "Fins";
    default: return "Fins";
    }
}

Missile::ControlSystem Missile::stringToControlSystem(const QString& str) const {
    if (str == "Canard") return ControlSystem::Canard;
    if (str == "ThrustVectoring") return ControlSystem::ThrustVectoring;
    return ControlSystem::Fins;
}

// Gun Implementation
Gun::Gun(Hierarchy* h) : Entity(h) {
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "gun_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["gun_param"] = par;
}

void Gun::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

std::vector<std::string> Gun::getSupportedComponents() {
    return std::vector<std::string>{};
}

void Gun::addComponent(std::string name) {
    Console::error("Gun does not support components: " + name);
}

void Gun::removeComponent(std::string name) {
    Console::error("Gun does not support components: " + name);
}

QJsonObject Gun::getComponent(std::string name) {
    Console::error("Gun does not support components: " + name);
    return QJsonObject();
}

void Gun::updateComponent(QString name, const QJsonObject& /*obj*/) {
    Console::error(name.toStdString() + ": Gun does not support components");
}

QJsonObject Gun::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;

    // Serialize parameters
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

    // Serialize gun attributes
    obj["muzzleSpeed"] = muzzleSpeed;
    obj["maximumRate"] = maximumRate;
    obj["barrelLength"] = barrelLength;
    obj["caliber"] = caliber;
    obj["magazineCapacity"] = magazineCapacity;
    obj["effectiveRange"] = effectiveRange;
    obj["burstMode"] = burstMode;
    obj["reloadTime"] = reloadTime;
    obj["recoilForce"] = recoilForce;

    return obj;
}

void Gun::fromJson(const QJsonObject& obj) {
    Name = obj["name"].toString().toStdString();
    ID = obj["id"].toString().toStdString();
    parentID = obj["parent_id"].toString().toStdString();
    Active = obj["active"].toBool();

    // Deserialize parameters
    if (obj.contains("parameters")) {
        QJsonObject parObj = obj["parameters"].toObject();
        if (parObj.contains("value")) {
            QJsonObject paramMap = parObj["value"].toObject();
            for (const QString& key : paramMap.keys()) {
                QJsonObject paramObj = paramMap[key].toObject();
                std::shared_ptr<Parameter> param = std::make_shared<Parameter>();
                param->fromJson(paramObj);
                parameters[key.toStdString()] = param;
            }
        }
    }

    // Deserialize gun attributes
    muzzleSpeed = static_cast<float>(obj["muzzleSpeed"].toDouble());
    maximumRate = static_cast<float>(obj["maximumRate"].toDouble());
    barrelLength = static_cast<float>(obj["barrelLength"].toDouble());
    caliber = static_cast<float>(obj["caliber"].toDouble());
    magazineCapacity = obj["magazineCapacity"].toInt();
    effectiveRange = static_cast<float>(obj["effectiveRange"].toDouble());
    burstMode = obj["burstMode"].toBool();
    reloadTime = static_cast<float>(obj["reloadTime"].toDouble());
    recoilForce = static_cast<float>(obj["recoilForce"].toDouble());
}

// Bomb Implementation
Bomb::Bomb(Hierarchy* h) : Entity(h) {
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "bomb_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["bomb_param"] = par;
}

void Bomb::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

std::vector<std::string> Bomb::getSupportedComponents() {
    return std::vector<std::string>{};
}

void Bomb::addComponent(std::string name) {
    Console::error("Bomb does not support components: " + name);
}

void Bomb::removeComponent(std::string name) {
    Console::error("Bomb does not support components: " + name);
}

QJsonObject Bomb::getComponent(std::string name) {
    Console::error("Bomb does not support components: " + name);
    return QJsonObject();
}

void Bomb::updateComponent(QString name, const QJsonObject& /*obj*/) {
    Console::error(name.toStdString() + ": Bomb does not support components");
}

QJsonObject Bomb::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;

    // Serialize parameters
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

    // Serialize bomb attributes
    QJsonObject locObj;
    locObj["x"] = locationOffset.x;
    locObj["y"] = locationOffset.y;
    locObj["z"] = locationOffset.z;
    obj["locationOffset"] = locObj;
    obj["payload"] = payload;
    obj["proximityRange"] = proximityRange;
    obj["guidingSensor"] = guidingSensorToString(guidingSensor);
    obj["fallTime"] = fallTime;
    obj["armingDelay"] = armingDelay;
    obj["fuseType"] = fuseTypeToString(fuseType);
    obj["glideCapability"] = glideCapability;

    return obj;
}

void Bomb::fromJson(const QJsonObject& obj) {
    Name = obj["name"].toString().toStdString();
    ID = obj["id"].toString().toStdString();
    parentID = obj["parent_id"].toString().toStdString();
    Active = obj["active"].toBool();

    // Deserialize parameters
    if (obj.contains("parameters")) {
        QJsonObject parObj = obj["parameters"].toObject();
        if (parObj.contains("value")) {
            QJsonObject paramMap = parObj["value"].toObject();
            for (const QString& key : paramMap.keys()) {
                QJsonObject paramObj = paramMap[key].toObject();
                std::shared_ptr<Parameter> param = std::make_shared<Parameter>();
                param->fromJson(paramObj);
                parameters[key.toStdString()] = param;
            }
        }
    }

    // Deserialize bomb attributes
    if (obj.contains("locationOffset") && obj["locationOffset"].isObject()) {
        QJsonObject locObj = obj["locationOffset"].toObject();
        locationOffset.x = static_cast<float>(locObj["x"].toDouble());
        locationOffset.y = static_cast<float>(locObj["y"].toDouble());
        locationOffset.z = static_cast<float>(locObj["z"].toDouble());
    }
    payload = static_cast<float>(obj["payload"].toDouble());
    proximityRange = static_cast<float>(obj["proximityRange"].toDouble());
    guidingSensor = stringToGuidingSensor(obj["guidingSensor"].toString());
    fallTime = static_cast<float>(obj["fallTime"].toDouble());
    armingDelay = static_cast<float>(obj["armingDelay"].toDouble());
    fuseType = stringToFuseType(obj["fuseType"].toString());
    glideCapability = obj["glideCapability"].toBool();
}

QString Bomb::guidingSensorToString(GuidingSensor gs) const {
    switch (gs) {
    case GuidingSensor::ActiveRadar: return "ActiveRadar";
    case GuidingSensor::Infrared: return "Infrared";
    default: return "Infrared";
    }
}

Bomb::GuidingSensor Bomb::stringToGuidingSensor(const QString& str) const {
    if (str == "ActiveRadar") return GuidingSensor::ActiveRadar;
    return GuidingSensor::Infrared;
}

QString Bomb::fuseTypeToString(FuseType ft) const {
    switch (ft) {
    case FuseType::Contact: return "Contact";
    case FuseType::Airburst: return "Airburst";
    case FuseType::Proximity: return "Proximity";
    default: return "Contact";
    }
}

Bomb::FuseType Bomb::stringToFuseType(const QString& str) const {
    if (str == "Airburst") return FuseType::Airburst;
    if (str == "Proximity") return FuseType::Proximity;
    return FuseType::Contact;
}
