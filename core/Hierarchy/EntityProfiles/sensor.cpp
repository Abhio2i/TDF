#include "sensor.h"
#include <core/Hierarchy/hierarchy.h> // Include full Hierarchy definition
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>
#include <cmath>       // For std::atan2, std::abs
#include <QVector3D>   // For QVector3D
#include <vector>      // For targets (assuming std::vector)
#include <unordered_set> // For detects (for fast Contains/Add/Remove)
#include "core/Hierarchy/EntityProfiles/radio.h"

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

void Sensor::scan(std::string id , Transform *source)
{
    // 🔹 Step 1: entry debug
    qDebug() << "[Sensor::scan] called for ID:" << QString::fromStdString(id)
             << " | parent:" << (GlobalRegistry::getParentHierarchy(this) ? "valid" : "null");

    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : *parent->Entities)
    {
        qDebug() << "[Sensor::scan] iterating entity:" << QString::fromStdString(key);
        if(key == id) continue;
        Platform* platform = dynamic_cast<Platform*>(entity);
        if (platform) {
            qDebug() << "[Sensor::scan] found Platform entity:"
                     << QString::fromStdString(platform->Name);
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            float distance = localPos.length();
            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            // 🔹 Step 3: debug detection conditions
            qDebug() << "[Sensor::scan] distance:" << distance
                     << " range:" << range
                     << " maxAngle:" << maxDetectionAngle;
            // InverseTransformPoint(tr.position) (Transform method assumed to exist)
            if (detectCheck(localPos)) // .position() is assumed
            {
                qDebug() << "[Sensor::scan] DETECTED target:"
                         << QString::fromStdString(platform->Name);
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
    qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
             << " | parent:" << (GlobalRegistry::getParentHierarchy(this) ? "valid" : "null");
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parent) {
        qDebug() << "[Sensor::ewscan] parent hierarchy is null";
        return;
    }
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : *parent->Entities)
    {
        qDebug() << "[Sensor::ewscan] iterating entity:" << QString::fromStdString(key);
        if(key == id) continue;
        Platform* platform = dynamic_cast<Platform*>(entity);
        if (platform) {
            qDebug() << "[Sensor::ewscan] found Platform entity:"
                     << QString::fromStdString(platform->Name);
            //qDebug()<< "platform";
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            float distance = localPos.length();
            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            // InverseTransformPoint(tr.position) (Transform method assumed to exist)
            qDebug() << "[Sensor::ewscan] distance:" << distance << " ewrange:" << ewrange;
            if (distance<ewrange) // .position() is assumed
            {
                //qDebug()<< "detect";
                // C# !detects.Contains(tr) -> C++ detects.count(tr) == 0
                if (ewdetects.count(platform) == 0)
                {
                    // C# detects.Add(tr) -> C++ detects.insert(tr)
                    qDebug() << "[Sensor::ewscan] DETECTED (EW) target:"
                             << QString::fromStdString(platform->Name);
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

void Sensor::csmScan(std::string id, Transform* source)
{
    if (subType != SubType::CSM) return;

    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    qDebug() << "[CSM] scan started for ID:" << QString::fromStdString(id)
             << "| total entities:" << parent->Entities->size();

    for (auto& [key, entity] : *parent->Entities)
    {
        qDebug() << "[CSM] checking entity:" << QString::fromStdString(key);

        if (key == id) continue;

        Platform* platform = dynamic_cast<Platform*>(entity);
        if (!platform) {
            qDebug() << "   [CSM] skipped - not a Platform";
            continue;
        }

        QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
        float distance = localPos.length();
        float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;

        qDebug() << "   [CSM] found Platform:" << QString::fromStdString(platform->Name)
                 << "| distance:" << distance << "| range:" << csmrange;

        bool hasRadio = !platform->radioList.empty();
        qDebug() << "   [CSM] hasRadio:" << hasRadio;

        if (hasRadio && distance < csmrange)
        {
            // ✅ Collect radio frequency data
            for (Radio* radio : platform->radioList)
            {
                if (!radio) continue;

                float freqUsed = radio->frequencyUsed;
                if (freqUsed <= 0.0f)
                {
                    // fallback: average of min and max if not used
                    freqUsed = (radio->frequencyMin + radio->frequencyMax) / 2.0f;
                }

                // Detect new platform
                if (csmdetects.count(platform) == 0)
                {
                    csmdetects.insert(platform);

                    Target target;
                    target.entity = platform;
                    target.angle = yAngle;
                    target.radius = distance;
                    csmtargets.append(target);

                    Message msg;
                    msg.timeStamp = QDateTime::currentDateTime().toString("hh:mm:ss").toStdString();
                    msg.source = Name;
                    msg.destination = platform->Name;
                    msg.content =
                        "Detected radio emission from " + platform->Name +
                        " | Frequency: " + std::to_string(freqUsed) + " MHz"
                                                                      " | Bandwidth: " + std::to_string(radio->bandwidth) + " kHz"
                                                             " | Power: " + std::to_string(radio->emittingPower) + " W";

                    messages.push_back(msg);

                    // ✅ Emit to UI
                    QJsonArray msgArray;
                    for (const auto& m : messages) {
                        QJsonObject o;
                        o["timeStamp"] = QString::fromStdString(m.timeStamp);
                        o["source"] = QString::fromStdString(m.source);
                        o["destination"] = QString::fromStdString(m.destination);
                        o["content"] = QString::fromStdString(m.content);
                        msgArray.append(o);
                    }
                    emit availableConnectionsUpdated(msgArray);

                    qDebug() << "📡 [CSM] Detected radio from:"
                             << QString::fromStdString(platform->Name)
                             << "| Frequency:" << freqUsed << "MHz"
                             << "| Power:" << radio->emittingPower << "W"
                             << "| BW:" << radio->bandwidth << "kHz";
                }
                else {
                    // update tracking if already known
                    for (int i = 0; i < csmtargets.size(); ++i) {
                        if (csmtargets.at(i).entity == platform) {
                            csmtargets[i].angle = yAngle;
                            csmtargets[i].radius = distance;
                            break;
                        }
                    }
                }
            }
        }
        else if (csmdetects.count(platform) > 0)
        {
            // Lost contact
            for (int i = 0; i < csmtargets.size(); ++i) {
                if (csmtargets.at(i).entity == platform) {
                    csmtargets.removeAt(i);
                    break;
                }
            }
            csmdetects.erase(platform);
            qDebug() << "❌ [CSM] lost radio contact with:"
                     << QString::fromStdString(platform->Name);
        }
    }

    qDebug() << "[CSM] scan completed — total detections:" << csmdetects.size();
}

void Sensor::esmScan(std::string id, Transform* source)
{
    if (subType != SubType::ESM) return;

    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parent) {
        qDebug() << "[ESM] No valid parent hierarchy!";
        return;
    }

    qDebug() << "[ESM] scan started for ID:" << QString::fromStdString(id)
             << "| total entities:" << parent->Entities->size();

    for (auto& [key, entity] : *parent->Entities)
    {
        if (key == id) continue; // skip self

        Platform* platform = dynamic_cast<Platform*>(entity);
        if (!platform) continue;

        QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
        float distance = localPos.length();
        float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;

        bool hasEmitter = false;
        QString emitterType;
        float detectedFreq = 0.0f;

        // ✅ Detect any active Generic (Radar/EW) emitters
        for (auto* s : platform->sensorList) {
            if (!s) continue;

            if (s->subType == SubType::Generic) {
                hasEmitter = true;
                emitterType = "Radar / EW emission";
                detectedFreq = s->emissionFrequency;  // use the existing field
                break;
            }
        }

        qDebug() << "[ESM] checking entity:" << QString::fromStdString(platform->Name)
                 << "| distance:" << distance
                 << "| inRange:" << (distance < esrange)
                 << "| hasEmitter:" << hasEmitter
                 << "| emitterType:" << emitterType;

        // ✅ Detected new emitter
        if (hasEmitter && distance < esrange)
        {
            if (esmdetects.count(platform) == 0)
            {
                esmdetects.insert(platform);

                Target target;
                target.entity = platform;
                target.angle = yAngle;
                target.radius = distance;
                esmtargets.append(target);

                // 🛰️ Create detection message for Inspector
                Message msg;
                msg.timeStamp = QDateTime::currentDateTime().toString("hh:mm:ss").toStdString();
                msg.source = Name;
                msg.destination = platform->Name;
                msg.content = "Detected " + emitterType.toStdString() +
                              " from " + platform->Name +
                              (detectedFreq > 0 ? (" @ " + std::to_string(detectedFreq) + " MHz") : "");
                messages.push_back(msg);

                // Emit JSON update
                QJsonArray msgArray;
                for (const auto& m : messages) {
                    QJsonObject o;
                    o["timeStamp"] = QString::fromStdString(m.timeStamp);
                    o["source"] = QString::fromStdString(m.source);
                    o["destination"] = QString::fromStdString(m.destination);
                    o["content"] = QString::fromStdString(m.content);
                    msgArray.append(o);
                }
                emit availableConnectionsUpdated(msgArray);

                qDebug() << "🎯 [ESM] Detected" << emitterType
                         << "from:" << QString::fromStdString(platform->Name);
            }
            else {
                // Update tracking for existing detection
                for (int i = 0; i < esmtargets.size(); ++i) {
                    if (esmtargets.at(i).entity == platform) {
                        esmtargets[i].angle = yAngle;
                        esmtargets[i].radius = distance;
                        break;
                    }
                }
            }
        }
        else if (esmdetects.count(platform) > 0)
        {
            // ❌ Lost contact
            for (int i = 0; i < esmtargets.size(); ++i) {
                if (esmtargets.at(i).entity == platform) {
                    esmtargets.removeAt(i);
                    break;
                }
            }
            esmdetects.erase(platform);
            qDebug() << "❌ [ESM] Lost contact with:" << QString::fromStdString(platform->Name);
        }
    }

    qDebug() << "[ESM] scan completed — total detections:" << esmdetects.size();
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
    obj["subType"] = subTypeToString(subType);

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

    // --- SubType-specific serialization ---
    if (subType == SubType::Generic) {
        obj["componentType"] = "Generic";
        obj["description"] = "Generic Radar Sensor";

        // ✅ Only Generic sensors show radar attributes
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
    }

    else if (subType == SubType::CSM) {
        obj["componentType"] = "CSM";
        obj["description"] = "Communication Support Measure Sensor";
        obj["active"] = Active;
        obj["detectionRange"] = csmrange;
        obj["detectedRadios"] = static_cast<int>(csmdetects.size());
    }

    else if (subType == SubType::ESM) {
        obj["componentType"] = "ESM";
        obj["description"] = "Electronic Support Measure Sensor";
        obj["active"] = Active;
        obj["detectionRange"] = esrange;
        obj["detectedRadars"] = static_cast<int>(esmdetects.size());
    }

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
    // --- Serialize Messages for CSM/ESM ---
    QJsonArray msgArray;
    for (const auto& msg : messages) {
        QJsonObject m;
        m["timeStamp"] = QString::fromStdString(msg.timeStamp);
        m["source"] = QString::fromStdString(msg.source);
        m["destination"] = QString::fromStdString(msg.destination);
        m["content"] = QString::fromStdString(msg.content);
        msgArray.append(m);
    }
    obj["messages"] = msgArray;
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
    // --- Deserialize Messages for CSM/ESM ---
    messages.clear();
    if (obj.contains("messages") && obj["messages"].isArray()) {
        QJsonArray msgArray = obj["messages"].toArray();
        for (const auto& msgVal : msgArray) {
            QJsonObject m = msgVal.toObject();
            Message msg;
            msg.timeStamp = m["timeStamp"].toString().toStdString();
            msg.source = m["source"].toString().toStdString();
            msg.destination = m["destination"].toString().toStdString();
            msg.content = m["content"].toString().toStdString();
            messages.push_back(msg);
        }
    }
    if (obj.contains("componentType")) {
        QString type = obj["componentType"].toString();
        if (type == "CSM")
            subType = SubType::CSM;
        else if (type == "ESM")
            subType = SubType::ESM;
        else
            subType = SubType::Generic;
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
