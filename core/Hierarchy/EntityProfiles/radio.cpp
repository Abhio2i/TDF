#include "radio.h"
#include <core/Hierarchy/hierarchy.h> // Include full Hierarchy definition
#include <core/Debug/console.h>
#include "core/Hierarchy/Utils/entityutils.h"
#include <core/GlobalRegistry.h>
#include <cmath>
#include <QtMath>
#include <QDebug>
std::unordered_set<std::string> Radio::radioSeen;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float RAD2DEG = 180.0f / M_PI;

Radio::Radio(Hierarchy* h) : Entity(h) {
    // Initialize default parameter (similar to Platform and Sensor)
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "radio_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["radio_param"] = par;
}

void Radio::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

std::vector<std::string>Radio:: getSupportedComponents(){
    return std::vector<std::string>{};
}

void Radio::addComponent(std::string name) {
    Console::error("Radio does not support components: " + name);
}

void Radio::removeComponent(std::string name) {
    Console::error("Radio does not support components: " + name);
}

QJsonObject Radio::getComponent(std::string name) {
    Console::error("Radio does not support components: " + name);
    return QJsonObject();
}

void Radio::updateComponent(QString name, const QJsonObject& /*obj*/) {
    Console::error(name.toStdString() + ": Radio does not support components");
}

QJsonObject Radio::toJson() const {
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

    // Serialize radio attributes
    obj["radioType"] = radioTypeToString(radioType);
    QJsonObject modObj;
    modObj["spreadSpectrum"] = spreadSpectrumToString(modulation.spreadSpectrum);
    modObj["majorModulation"] = majorModulationToString(modulation.majorModulation);
    modObj["detailModulation"] = QString::fromStdString(modulation.detailModulation);
    obj["modulation"] = modObj;
    obj["frequencyMin"] = frequencyMin;
    obj["frequencyMax"] = frequencyMax;
    obj["emittingPower"] = emittingPower;
    obj["Range"] = Range;
    obj["bandwidth"] = bandwidth;
    obj["dataRate"] = dataRate;
    obj["encryptionType"] = encryptionTypeToString(encryptionType);
    obj["channelCount"] = channelCount;
    obj["jammingResistance"] = jammingResistance;
    obj["antennaGain"] = antennaGain;
    obj["noiseFigure"] = noiseFigure;
    obj["frequencyUsed"] = frequencyUsed;
    obj["receiverSensitivity"]   = receiverSensitivity;
    obj["systemLoss"]            = systemLoss;
    obj["fadeMargin"]            = fadeMargin;
    obj["receiverAntennaGain"]   = receiverAntennaGain;
    obj["pathLossExponent"]      = pathLossExponent;

    QJsonArray messagesArray;
    for (const auto& message : messages) {
        QJsonObject msgObj;
        msgObj["timeStamp"] = QString::fromStdString(message.timeStamp);
        msgObj["source"] = QString::fromStdString(message.source);
        msgObj["destination"] = QString::fromStdString(message.destination);
        msgObj["content"] = QString::fromStdString(message.content);
        messagesArray.append(msgObj);
    }
    obj["messages"] = messagesArray;

    return obj;
}

void Radio::fromJson(const QJsonObject& obj) {
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

    // Deserialize radio attributes
    radioType = stringToRadioType(obj["radioType"].toString());
    if (obj.contains("modulation") && obj["modulation"].isObject()) {
        QJsonObject modObj = obj["modulation"].toObject();
        modulation.spreadSpectrum = stringToSpreadSpectrum(modObj["spreadSpectrum"].toString());
        modulation.majorModulation = stringToMajorModulation(modObj["majorModulation"].toString());
        modulation.detailModulation = modObj["detailModulation"].toString().toStdString();
    }
    if (obj.contains("frequencyMin"))
        frequencyMin = static_cast<float>(obj["frequencyMin"].toDouble());

    if (obj.contains("frequencyMax"))
        frequencyMax = static_cast<float>(obj["frequencyMax"].toDouble());

    if (obj.contains("emittingPower"))
        emittingPower = static_cast<float>(obj["emittingPower"].toDouble());

    if (obj.contains("Range"))
        Range = static_cast<float>(obj["Range"].toDouble());

    if (obj.contains("bandwidth"))
        bandwidth = static_cast<float>(obj["bandwidth"].toDouble());

    if (obj.contains("dataRate"))
        dataRate = static_cast<float>(obj["dataRate"].toDouble());

    if (obj.contains("encryptionType"))
        encryptionType = stringToEncryptionType(obj["encryptionType"].toString());

    if (obj.contains("channelCount"))
        channelCount = obj["channelCount"].toInt();

    if (obj.contains("jammingResistance"))
        jammingResistance = obj["jammingResistance"].toBool();

    if (obj.contains("antennaGain"))
        antennaGain = static_cast<float>(obj["antennaGain"].toDouble());

    if (obj.contains("noiseFigure"))
        noiseFigure = static_cast<float>(obj["noiseFigure"].toDouble());

    if (obj.contains("frequencyUsed"))
        frequencyUsed = static_cast<float>(obj["frequencyUsed"].toDouble());

    if (obj.contains("receiverSensitivity"))
        receiverSensitivity = static_cast<float>(obj["receiverSensitivity"].toDouble());

    if (obj.contains("systemLoss"))
        systemLoss = static_cast<float>(obj["systemLoss"].toDouble());

    if (obj.contains("fadeMargin"))
        fadeMargin = static_cast<float>(obj["fadeMargin"].toDouble());

    if (obj.contains("receiverAntennaGain"))
        receiverAntennaGain = static_cast<float>(obj["receiverAntennaGain"].toDouble());

    if (obj.contains("pathLossExponent"))
        pathLossExponent = static_cast<float>(obj["pathLossExponent"].toDouble());

    // messages.clear();
    if (obj.contains("messages") && obj["messages"].isArray()) {
        QJsonArray messagesArray = obj["messages"].toArray();
        for (const auto& msgVal : messagesArray) {
            QJsonObject msgObj = msgVal.toObject();
            Message message;
            message.timeStamp = msgObj["timeStamp"].toString().toStdString();
            message.source = msgObj["source"].toString().toStdString();
            message.destination = msgObj["destination"].toString().toStdString();
            message.content = msgObj["content"].toString().toStdString();
            messages.push_back(message);
        }
    }
}

QString Radio::radioTypeToString(RadioType rt) const {
    switch (rt) {
    case RadioType::Transmitter: return "Transmitter";
    case RadioType::Receiver: return "Receiver";
    case RadioType::Transceiver: return "Transceiver";
    default: return "Transceiver";
    }
}

Radio::RadioType Radio::stringToRadioType(const QString& str) const {
    if (str == "Transmitter") return RadioType::Transmitter;
    if (str == "Receiver") return RadioType::Receiver;
    return RadioType::Transceiver;
}

QString Radio::spreadSpectrumToString(SpreadSpectrum ss) const {
    switch (ss) {
    case SpreadSpectrum::FHSS: return "FHSS";
    case SpreadSpectrum::DSSS: return "DSSS";
    case SpreadSpectrum::None: return "None";
    default: return "None";
    }
}

Radio::SpreadSpectrum Radio::stringToSpreadSpectrum(const QString& str) const {
    if (str == "FHSS") return SpreadSpectrum::FHSS;
    if (str == "DSSS") return SpreadSpectrum::DSSS;
    return SpreadSpectrum::None;
}

QString Radio::majorModulationToString(MajorModulation mm) const {
    switch (mm) {
    case MajorModulation::AM: return "AM";
    case MajorModulation::FM: return "FM";
    case MajorModulation::PSK: return "PSK";
    case MajorModulation::QAM: return "QAM";
    default: return "AM";
    }
}

Radio::MajorModulation Radio::stringToMajorModulation(const QString& str) const {
    if (str == "FM") return MajorModulation::FM;
    if (str == "PSK") return MajorModulation::PSK;
    if (str == "QAM") return MajorModulation::QAM;
    return MajorModulation::AM;
}

QString Radio::encryptionTypeToString(EncryptionType et) const {
    switch (et) {
    case EncryptionType::AES: return "AES";
    case EncryptionType::DES: return "DES";
    case EncryptionType::None: return "None";
    default: return "None";
    }
}

Radio::EncryptionType Radio::stringToEncryptionType(const QString& str) const {
    if (str == "AES") return EncryptionType::AES;
    if (str == "DES") return EncryptionType::DES;
    return EncryptionType::None;
}

float Radio::calculateRange() const
{
    bool isDefault = (emittingPower <= 0.0f &&
                      antennaGain <= 0.0f &&
                      bandwidth <= 0.0f &&
                      noiseFigure <= 0.0f &&
                      frequencyUsed <= 1.0f &&
                      frequencyMax <= 1.0f);

    if (isDefault||true) {
        return Range;
    }

    // --- simplified FSPL-based range ---
    float txGain = antennaGain;
    float rxGain = (receiverAntennaGain >= 0.0f) ? receiverAntennaGain : antennaGain;
    float Pt_dBm = 10.0f * log10(emittingPower * 1000.0f);

    const float k = 1.38064852e-23f;
    const float T = 290.0f;
    float B_Hz = bandwidth * 1000.0f;
    float N_dBm = 10.0f * log10(k * T * B_Hz * 1000.0f);
    float Prx_dBm = N_dBm + noiseFigure + fadeMargin;

    float freqMHz = (frequencyUsed > 0.0f) ? frequencyUsed : frequencyMax;
    float FSPL_dB = Pt_dBm + txGain + rxGain - Prx_dBm - systemLoss;
    float exponent = (FSPL_dB - 32.44f - 20.0f * log10(freqMHz)) / 20.0f;
    float range_m = pow(10.0f, exponent);

    // --- clamp range to reasonable values ---
    if (!std::isfinite(range_m) || range_m <= 0.0f) range_m = 7000.0f; // fallback 7 km
    if (range_m > 50000.0f) range_m = 50000.0f; // max ~50 km
    if (range_m < 100.0f) range_m = 100.0f;     // min ~100 m

    qDebug() << "Radio range calculated:"
             << "Frequency(MHz):" << freqMHz
             << "Range(m):" << range_m;

    return range_m;
}

void Radio::updateAvailableConnections(Transform* source)
{
    //qDebug() << "=== updateAvailableConnections() called for Radio:" << QString::fromStdString(Name) << "===";

    // messages.clear();
    // qDebug() << "Messages cleared.";

    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parent) {
        qDebug() << "Parent hierarchy is NULL!";
        return;
    }

    //qDebug() << "Total entities in hierarchy:" << parent->Entities->size();

    float range = calculateRange();
    //qDebug() << "Calculated radio range:" << range << "meters";

    // Detect default/empty radio
    bool thisRadioDefault = (emittingPower <= 0.0f &&
                             antennaGain <= 0.0f &&
                             bandwidth <= 0.0f &&
                             noiseFigure <= 0.0f &&
                             frequencyUsed <= 1.0f &&
                             frequencyMax <= 1.0f);

    const float SCENE_UNIT_TO_METERS = 1000.0f;

    for (auto& [key, entity] : *parent->Entities) {
        if (!entity) continue;
        if (entity->ID == this->parentID) {
            //qDebug() << "Skipping own platform/entity:" << QString::fromStdString(entity->Name);
            continue;
        }


        Platform* platform = dynamic_cast<Platform*>(entity);
        if (!platform) {
            //qDebug() << "Skipping entity (not a Platform):" << QString::fromStdString(entity->ID);
            continue;
        }

        if (platform->radioList.empty()) {
            //qDebug() << "Platform has no radios:" << QString::fromStdString(platform->Name);
            continue;
        }

        if (!platform->transform || !platform->transform->matrix) {
            //qDebug() << "Invalid transform/matrix for platform:" << QString::fromStdString(platform->Name);
            continue;
        }

        // Distance calculation
        QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
        //float distance = localPos.length() * SCENE_UNIT_TO_METERS;
        float metredis = distanceBetween(source->translation().x(),source->translation().z(),platform->transform->matrix->translation().x(),platform->transform->matrix->translation().z())/1000;

        //qDebug() << "Platform:" << QString::fromStdString(platform->Name)
        //<< "| Distance (meters):" << distance;

        // Frequency compatibility check
        bool freqMatch = false;
        for (Radio* otherRadio : platform->radioList) {
            if (!otherRadio) continue;

            bool otherDefault = (otherRadio->emittingPower <= 0.0f &&
                                 otherRadio->antennaGain <= 0.0f &&
                                 otherRadio->bandwidth <= 0.0f &&
                                 otherRadio->noiseFigure <= 0.0f &&
                                 otherRadio->frequencyUsed <= 1.0f &&
                                 otherRadio->frequencyMax <= 1.0f);

            // Both default -> allow
            if (thisRadioDefault && otherDefault) {
                freqMatch = true;
                //qDebug() << "Both radios are default: communication allowed";
                break;
            }

            // One default and one custom -> block
            if (thisRadioDefault != otherDefault) {
                //qDebug() << "One radio default, one custom: communication blocked";
                continue;
            }

            // Both custom -> check overlap
            float thisMin = frequencyMin;
            float thisMax = frequencyMax;
            float otherMin = otherRadio->frequencyMin;
            float otherMax = otherRadio->frequencyMax;

            if (thisMax >= otherMin && thisMin <= otherMax) {
                freqMatch = true;
                //qDebug() << "Custom frequency ranges overlap:"
                //<< "this[" << thisMin << "," << thisMax << "]"
                //<< "other[" << otherMin << "," << otherMax << "]";
                break;
            } else {
                //qDebug() << "Custom frequency ranges do NOT overlap:"
                //<< "this[" << thisMin << "," << thisMax << "]"
                //<< "other[" << otherMin << "," << otherMax << "]";
            }
        }
        // If within range and frequency matches, store target
        if (metredis > 0.0f && metredis <= range && freqMatch)
        {
            // Calculate angle (horizontal azimuth)
            //QVector3D delta = platform->transform->matrix->translation() - source->matrix->translation();
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());

            float angle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            //if (angle < 0) angle += 360.0f;

            // Make key unique per radio and per platform
            std::string key = parentID + ":" + Name + "->" + platform->ID + ":" + platform->Name;

            if (radioSeen.find(key) == radioSeen.end())
            {
                radioSeen.insert(key);

                RadioTarget target;
                target.entity = platform;
                target.name = platform->Name;
                target.radius = metredis;
                target.angle = angle;
                target.range = range;
                target.frequency = frequencyUsed;

                targets.push_back(target);  // store in vector of RadioTarget

                qDebug() << "[RADIO] 🟢 New radio target stored:" << QString::fromStdString(key);
            }else{
                for (int i = 0; i < targets.size(); ++i) {
                    if (targets.at(i).entity == platform) {
                        targets[i].angle = angle;
                        targets[i].radius = metredis;
                        targets[i].range = range;
                        targets[i].frequency = frequencyUsed;
                        //qDebug()<< localPos;
                        break; // एक बार मिल जाने पर लूप से बाहर निकल जाएँ
                    }
                }
            }
        }
        else
        {
            if (metredis > range)"";
            //qDebug() << "Platform out of range:" << QString::fromStdString(platform->Name)
            //<< "| Distance:" << distance << ">" << range;

            if (!freqMatch)"";
            //qDebug() << "Frequency mismatch with platform:" << QString::fromStdString(platform->Name);
        }
        // --------- PASTE BELOW THIS LINE ---------

        for (int i = targets.size() - 1; i >= 0; --i) {
            bool stillExists = false;
            for (auto& [entityKey, entity] : *parent->Entities) { // renamed 'key' -> 'entityKey'
                Platform* platform = dynamic_cast<Platform*>(entity);
                if (!platform) continue;

                float metredis = distanceBetween(
                                     source->translation().x(), source->translation().z(),
                                     platform->transform->matrix->translation().x(),
                                     platform->transform->matrix->translation().z()
                                     ) / 1000;

                if (platform == targets[i].entity && metredis <= range) {
                    stillExists = true;
                    break;
                }
            }

            if (!stillExists) {
                std::string removeKey = parentID + ":" + Name + "->" +
                                        targets[i].entity->ID + ":" +
                                        targets[i].entity->Name;

                radioSeen.erase(removeKey);
                targets.erase(targets.begin() + i);

                qDebug() << "[RADIO] REMOVED (out of range):"
                         << QString::fromStdString(removeKey);
            }
        }
    }

    //qDebug() << "Total messages prepared:" << messages.size();

    QJsonArray msgArray;
    for (const auto& m : messages) {
        QJsonObject obj;
        obj["timeStamp"] = QString::fromStdString(m.timeStamp);
        obj["source"] = QString::fromStdString(m.source);
        obj["destination"] = QString::fromStdString(m.destination);
        obj["content"] = QString::fromStdString(m.content);
        msgArray.append(obj);
    }

    //qDebug() << "Emitting availableConnectionsUpdated with"
    //<< msgArray.size() << "messages.";
    emit availableConnectionsUpdated(msgArray);
}
