#include "sensor.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qmetaobject.h"
#include <core/Hierarchy/hierarchy.h> // Include full Hierarchy definition
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>
#include <cmath>       // For std::atan2, std::abs
#include <QVector3D>   // For QVector3D
#include <vector>      // For targets (assuming std::vector)


// #ifndef M_PI
// #define M_PI 3.14159265358979323846
// #endif


const float RAD2DEG = 180.0f / M_PI;

Sensor::Sensor(Hierarchy* h) : Entity(h) {
    type = Constants::EntityType::Sensor;
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "sensor_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["sensor_param"] = par;
    subType = SubType::Generic;
}

void Sensor::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

std::vector<std::string>Sensor:: getSupportedComponents() {
    return std::vector<std::string>{};
}

void Sensor::addComponent(std::string name) {
    Console::error("Sensor does not support components: " + name);
}

void Sensor::removeComponent(std::string name) {
    Console::error("Sensor does not support components: " + name);
}

QJsonObject Sensor::getComponent(std::string name) {
    Console::error("Sensor does not support components: " + name);
    return QJsonObject();
}

void Sensor::updateComponent(QString name, const QJsonObject& /*obj*/) {
    Console::error(name.toStdString() + ": Sensor does not support components");
}
void Sensor::scan() {
    // Default implementation can be empty for now
    // This allows the class to be instantiated
    qDebug()<<"sensor";
}

void Sensor::clearTargets(){
    targets.clear();
    detects.clear();

    ewdetects.clear();
    ewtargets.clear();
}

bool Sensor::detectCheck(QVector3D localPos,float distance,float multi)
{
    // horizontal angle (Y axis) : x vs z
    float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;

    // vertical angle (X axis) : y vs z
    float xAngle = std::atan2(localPos.y(), localPos.z()) * RAD2DEG;

    // abs le lo taki left/right aur up/down dono sides cover ho
    // C# Mathf.Abs() -> std::abs()
    yAngle = std::abs(yAngle) * multi;
    xAngle = std::abs(xAngle);
    xAngle = yAngle;
    // qDebug()<< xAngle<<","<<yAngle<<","<<distance;
    //qDebug()<< maxDetectionAngle<<","<<range;
    //qDebug()<< (distance < range)<<","<<(xAngle < maxDetectionAngle)<<","<<(yAngle < maxDetectionAngle);
    // Assuming 'range' and 'maxDetectionAngle' are member variables of Sensor
    return (distance < range && xAngle < azimuth && yAngle < azimuth);
}

QStringList DetectionCapabilitiesTypeOptions() {
    QStringList list;
    int index = Sensor::staticMetaObject.indexOfEnumerator("DetectionCapabilities");
    QMetaEnum metaEnum = Sensor::staticMetaObject.enumerator(index);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        list << QString::fromLatin1(metaEnum.key(i));
    }
    return list;
}

QJsonObject Sensor::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;
    obj["subType"] = subTypeToString(subType);

    QJsonObject capabilitiesObj;
    capabilitiesObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : DetectionCapabilitiesTypeOptions())
        optionsArray.append(opt);
    capabilitiesObj["options"] = optionsArray;
    capabilitiesObj["value"] = detectionCapabilitiesToString(capabilities);


    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km");
    defaultObj["frequency"] = toParm(frequency,"Ghz");
    defaultObj["azimuth"] = toParm(azimuth,"deg");
    defaultObj["DetectionCapabilities"] = capabilitiesObj;
    obj["default"] = defaultObj;
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
    return obj;
}

void Sensor::fromJson(const QJsonObject& obj) {
    if (obj.contains("name")){
        Name = obj["name"].toString().toStdString();
    }
    if (obj.contains("id")){
        ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("parent_id")){
        parentID = obj["parent_id"].toString().toStdString();
    }
    if (obj.contains("active")){
        Active = obj["active"].toBool();
    }
    if (obj.contains("subType"))
        subType = stringToSubType(obj["subType"].toString());

    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();
        if (defaultObj.contains("range"))
            range = valueFromParm(defaultObj["range"].toObject());

        if (defaultObj.contains("frequency"))
            frequency = valueFromParm(defaultObj["frequency"].toObject());

        if (defaultObj.contains("azimuth"))
            azimuth = valueFromParm(defaultObj["azimuth"].toObject());

        if (defaultObj.contains("DetectionCapabilities") && defaultObj["DetectionCapabilities"].isObject()) {
            QJsonObject capabilitiesObj = defaultObj["DetectionCapabilities"].toObject();
            if (capabilitiesObj.contains("value"))
                capabilities = stringTodetectionCapabilities(capabilitiesObj["value"].toString());
        }
    }
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

    // Deserialize sensor attributes
    if (obj.contains("sensorType")){
        sensortype = obj["sensorType"].toString() == "Active" ? Type::Active : Type::Passive;
    }


}

QString Sensor::modeToString(Mode m) const {
    switch (m) {
    case Mode::Search: return "Search";
    case Mode::Track: return "Track";
    case Mode::TrackWhileScan: return "TrackWhileScan";
    case Mode::FireControl: return "FireControl";
    default: return "Search";
    }
}

Sensor::Mode Sensor::stringToMode(const QString& str) const {
    if (str == "Track") return Mode::Track;
    if (str == "TrackWhileScan") return Mode::TrackWhileScan;
    if (str == "FireControl") return Mode::FireControl;
    return Mode::Search;
}

QString Sensor::subTypeToString(SubType t) const {
    switch (t) {
    case SubType::CSM: return "CSM";
    case SubType::ESM: return "ESM";
    case SubType::Generic: return "Generic";
    default: return "Generic";
    }
}

Sensor::SubType Sensor::stringToSubType(const QString& str) const {
    QString lower = str.toLower();
    if (lower == "csm") return SubType::CSM;
    if (lower == "esm") return SubType::ESM;
    if (lower == "sensor" || lower == "generic") return SubType::Generic;
    return SubType::Generic;  // ✅ Default fallback
}

QString Sensor::detectionCapabilitiesToString(DetectionCapabilities t) const  {
    switch (t) {
    case DetectionCapabilities::All: return "All";
    case DetectionCapabilities::MovingOnly: return "MovingOnly";
    default: return "All";
    }
}

Sensor::DetectionCapabilities Sensor::stringTodetectionCapabilities(const QString& str) const {
    QString lower = str.toLower();
    if (lower == "all") return DetectionCapabilities::All;
    if (lower == "movingonly") return DetectionCapabilities::MovingOnly;
        return DetectionCapabilities::All;  // ✅ Default fallback
}

// ================= Generic Targets =================
int Sensor::getTargetCount() const {
    return targets.size();
}

Target Sensor::getTarget(int index) const {
    if (index < 0 || index >= targets.size()) {
        throw std::out_of_range("Generic Target index out of range");
    }
    return targets[index];
}

// ================= CSM Targets =================
int Sensor::getCSMTargetCount() const {
    return ewtargets.size();  // Changed from csmtargets
}

Target Sensor::getCSMTarget(int index) const {
    if (index < 0 || index >= csmtargets.size()) {
        throw std::out_of_range("CSM Target index out of range");
    }
    return ewtargets[index];  // Changed from csmtargets
}

// ================= ESM Targets =================
int Sensor::getESMTargetCount() const {
    return ewtargets.size();  // Changed from esmtargets
}

Target Sensor::getESMTarget(int index) const {
    if (index < 0 || index >= esmtargets.size()) {
        throw std::out_of_range("ESM Target index out of range");
    }
    return ewtargets[index];  // Changed from esmtargets
}
