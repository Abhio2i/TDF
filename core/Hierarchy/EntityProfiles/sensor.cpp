#include "sensor.h"
#include <core/Hierarchy/hierarchy.h> // Include full Hierarchy definition
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>
#include <cmath>       // For std::atan2, std::abs
#include <QVector3D>   // For QVector3D
#include <vector>      // For targets (assuming std::vector)
#include <unordered_set> // For detects (for fast Contains/Add/Remove)

// M_PI को अधिकांश सिस्टम में डिफाइन किया जाता है, लेकिन इसकी गारंटी नहीं है।
// इसलिए, आप इसे मैन्युअल रूप से डिफाइन कर सकते हैं:
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// C# Mathf.Rad2Deg के बराबर
const float RAD2DEG = 180.0f / M_PI;

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

void Sensor::scan(std::string id , Transform *source)
{
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : *parent->Entities)
    {
        if(key == id) continue;
        Platform* platform = dynamic_cast<Platform*>(entity);
        if (platform) {
            //qDebug()<< "platform";
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            float distance = localPos.length();
            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            // InverseTransformPoint(tr.position) (Transform method assumed to exist)
            if (detectCheck(localPos)) // .position() is assumed
            {
                //qDebug()<< "detect";
                // C# !detects.Contains(tr) -> C++ detects.count(tr) == 0
                if (detects.count(platform) == 0)
                {
                    // C# detects.Add(tr) -> C++ detects.insert(tr)
                    detects.insert(platform);
                    Target target;
                    target.entity = platform;
                    target.angle = yAngle;
                    target.radius = distance;
                    targets.append(target);
                    //qDebug()<< "detect :"<<&entity->Name;
                }else{
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            targets[i].angle = yAngle;
                            targets[i].radius = distance;
                            //qDebug()<< localPos;
                            break; // एक बार मिल जाने पर लूप से बाहर निकल जाएँ
                        }
                    }
                    //qDebug()<< "detect :"<< QString::fromStdString(entity->Name);
                }
            }
            else
            {
                // C# detects.Contains(tr) -> C++ detects.count(tr) > 0
                if (detects.count(platform) > 0)
                {
                    // C# detects.Remove(tr) -> C++ detects.erase(tr)
                    // 1. target को खोजने के लिए iterate करें
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            // 2. मिल जाने पर, उसे index द्वारा हटा दें
                            targets.removeAt(i);
                            break; // एक बार मिल जाने पर लूप से बाहर निकल जाएँ
                        }
                    }
                    detects.erase(platform);
                    qDebug()<< "vanish :"<<&entity->Name;
                }
            }
        }
    }
}

void Sensor::ewscan(std::string id , Transform *source)
{
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : *parent->Entities)
    {
        if(key == id) continue;
        Platform* platform = dynamic_cast<Platform*>(entity);
        if (platform) {
            //qDebug()<< "platform";
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            float distance = localPos.length();
            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            // InverseTransformPoint(tr.position) (Transform method assumed to exist)
            if (distance<ewrange) // .position() is assumed
            {
                //qDebug()<< "detect";
                // C# !detects.Contains(tr) -> C++ detects.count(tr) == 0
                if (ewdetects.count(platform) == 0)
                {
                    // C# detects.Add(tr) -> C++ detects.insert(tr)
                    ewdetects.insert(platform);
                    Target target;
                    target.entity = platform;
                    target.angle = yAngle;
                    target.radius = distance;
                    ewtargets.append(target);
                    //qDebug()<< "detect :"<<&entity->Name;
                }else{
                    for (int i = 0; i < ewtargets.size(); ++i) {
                        if (ewtargets.at(i).entity == platform) {
                            ewtargets[i].angle = yAngle;
                            ewtargets[i].radius = distance;
                            //qDebug()<< localPos;
                            break; // एक बार मिल जाने पर लूप से बाहर निकल जाएँ
                        }
                    }
                    //qDebug()<< "detect :"<< QString::fromStdString(entity->Name);
                }
            }
            else
            {
                // C# detects.Contains(tr) -> C++ detects.count(tr) > 0
                if (ewdetects.count(platform) > 0)
                {
                    // C# detects.Remove(tr) -> C++ detects.erase(tr)
                    // 1. target को खोजने के लिए iterate करें
                    for (int i = 0; i < ewtargets.size(); ++i) {
                        if (ewtargets.at(i).entity == platform) {
                            // 2. मिल जाने पर, उसे index द्वारा हटा दें
                            ewtargets.removeAt(i);
                            break; // एक बार मिल जाने पर लूप से बाहर निकल जाएँ
                        }
                    }
                    ewdetects.erase(platform);
                    qDebug()<< "vanish :"<<&entity->Name;
                }
            }
        }
    }
}


// Sensor.cpp

bool Sensor::detectCheck(QVector3D localPos)
{
    // localPos.magnitude  ->  localPos.length() (QVector3D method)
    float distance = localPos.length();

    // C# Mathf.Atan2(y, x) -> std::atan2(y, x)
    // C# Mathf.Rad2Deg -> RAD2DEG constant

    // horizontal angle (Y axis) : x vs z
    float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;

    // vertical angle (X axis) : y vs z
    float xAngle = std::atan2(localPos.y(), localPos.z()) * RAD2DEG;

    // abs le lo taki left/right aur up/down dono sides cover ho
    // C# Mathf.Abs() -> std::abs()
    yAngle = std::abs(yAngle);
    xAngle = std::abs(xAngle);
    //qDebug()<< xAngle<<","<<yAngle<<","<<distance;
    //qDebug()<< maxDetectionAngle<<","<<range;
    //qDebug()<< (distance < range)<<","<<(xAngle < maxDetectionAngle)<<","<<(yAngle < maxDetectionAngle);
    // Assuming 'range' and 'maxDetectionAngle' are member variables of Sensor
    return (distance < range && xAngle < maxDetectionAngle && yAngle < maxDetectionAngle);
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
    obj["ewrange"] = ewrange;
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
        type = obj["sensorType"].toString() == "Active" ? Type::Active : Type::Passive;
    }
    if (obj.contains("mode")){
        mode = stringToMode(obj["mode"].toString());
    }
    if (obj.contains("emissionPower")){
        emissionPower = static_cast<float>(obj["emissionPower"].toDouble());
    }
    if (obj.contains("emissionFrequency")){
        emissionFrequency = static_cast<float>(obj["emissionFrequency"].toDouble());
    }
    bandwidth = static_cast<float>(obj["bandwidth"].toDouble());
    pulseWidth = static_cast<float>(obj["pulseWidth"].toDouble());
    prf = static_cast<float>(obj["prf"].toDouble());
    scanningRate = static_cast<float>(obj["scanningRate"].toDouble());
    beamWidth = static_cast<float>(obj["beamWidth"].toDouble());
    antennaGain = static_cast<float>(obj["antennaGain"].toDouble());
    detectionCapabilities = static_cast<float>(obj["detectionCapabilities"].toDouble());
    if (obj.contains("maxDetectionAngle")){
        maxDetectionAngle = static_cast<float>(obj["maxDetectionAngle"].toDouble());
    }
    if (obj.contains("range")){
        range = static_cast<float>(obj["range"].toDouble());
    }
    if (obj.contains("ewrange")){
        ewrange = static_cast<float>(obj["ewrange"].toDouble());
    }
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
