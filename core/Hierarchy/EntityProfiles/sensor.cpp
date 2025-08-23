


#include "sensor.h"
#include <core/Hierarchy/hierarchy.h> // Include full Hierarchy definition
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>

Sensor::Sensor(Hierarchy* h) : Entity(h) {
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "sensor_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["sensor_param"] = par;
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

QJsonObject Sensor::toJson() const {
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

    // Serialize sensor attributes
    obj["sensorType"] = type == Type::Active ? "Active" : "Passive";
    obj["mode"] = modeToString(mode);
    obj["emissionPower"] = emissionPower;
    obj["emissionFrequency"] = emissionFrequency;
    obj["bandwidth"] = bandwidth;
    obj["pulseWidth"] = pulseWidth;
    obj["prf"] = prf;
    obj["scanningRate"] = scanningRate;
    obj["beamWidth"] = beamWidth;
    obj["antennaGain"] = antennaGain;
    obj["detectionCapabilities"] = detectionCapabilities;
    obj["maxDetectionAngle"] = maxDetectionAngle;
    obj["range"] = range;
    obj["refreshRate"] = refreshRate;
    obj["noiseFigure"] = noiseFigure;
    obj["clutterRejection"] = clutterRejection;
    obj["eccmCapability"] = eccmCapability;

    QJsonArray detectionsArray;
    for (const auto& detection : detections) {
        QJsonObject detObj;
        detObj["latitude"] = detection.geoCoords.latitude;
        detObj["longitude"] = detection.geoCoords.longitude;
        detObj["altitude"] = detection.geoCoords.altitude;
        detObj["heading"] = detection.geoCoords.heading;
        detObj["velocityX"] = detection.velocity.x;
        detObj["velocityY"] = detection.velocity.y;
        detObj["velocityZ"] = detection.velocity.z;
        detObj["entityReference"] = QString::fromStdString(detection.entityReference);
        detObj["signalStrength"] = detection.signalStrength;
        detObj["detectionConfidence"] = detection.detectionConfidence;
        detectionsArray.append(detObj);
    }
    obj["detections"] = detectionsArray;

    return obj;
}

void Sensor::fromJson(const QJsonObject& obj) {
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

    // Deserialize sensor attributes
    type = obj["sensorType"].toString() == "Active" ? Type::Active : Type::Passive;
    mode = stringToMode(obj["mode"].toString());
    emissionPower = static_cast<float>(obj["emissionPower"].toDouble());
    emissionFrequency = static_cast<float>(obj["emissionFrequency"].toDouble());
    bandwidth = static_cast<float>(obj["bandwidth"].toDouble());
    pulseWidth = static_cast<float>(obj["pulseWidth"].toDouble());
    prf = static_cast<float>(obj["prf"].toDouble());
    scanningRate = static_cast<float>(obj["scanningRate"].toDouble());
    beamWidth = static_cast<float>(obj["beamWidth"].toDouble());
    antennaGain = static_cast<float>(obj["antennaGain"].toDouble());
    detectionCapabilities = static_cast<float>(obj["detectionCapabilities"].toDouble());
    maxDetectionAngle = static_cast<float>(obj["maxDetectionAngle"].toDouble());
    range = static_cast<float>(obj["range"].toDouble());
    refreshRate = static_cast<float>(obj["refreshRate"].toDouble());
    noiseFigure = static_cast<float>(obj["noiseFigure"].toDouble());
    clutterRejection = obj["clutterRejection"].toBool();
    eccmCapability = obj["eccmCapability"].toBool();

    detections.clear();
    if (obj.contains("detections") && obj["detections"].isArray()) {
        QJsonArray detectionsArray = obj["detections"].toArray();
        for (const auto& detVal : detectionsArray) {
            QJsonObject detObj = detVal.toObject();
            Detection detection;
            detection.geoCoords.latitude = detObj["latitude"].toDouble();
            detection.geoCoords.longitude = detObj["longitude"].toDouble();
            detection.geoCoords.altitude = detObj["altitude"].toDouble();
            detection.geoCoords.heading = detObj["heading"].toDouble();
            detection.velocity.x = detObj["velocityX"].toDouble();
            detection.velocity.y = detObj["velocityY"].toDouble();
            detection.velocity.z = detObj["velocityZ"].toDouble();
            detection.entityReference = detObj["entityReference"].toString().toStdString();
            detection.signalStrength = static_cast<float>(detObj["signalStrength"].toDouble());
            detection.detectionConfidence = static_cast<float>(detObj["detectionConfidence"].toDouble());
            detections.push_back(detection);
        }
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
