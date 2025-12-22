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
    // QJsonObject modObj;
    // modObj["spreadSpectrum"] = spreadSpectrumToString(modulation.spreadSpectrum);
    // modObj["majorModulation"] = majorModulationToString(modulation.majorModulation);
    // modObj["detailModulation"] = QString::fromStdString(modulation.detailModulation);
    // obj["modulation"] = modObj;
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["frequencyMin"] = toParm(frequencyMin,"Mhz");
    defaultObj["frequencyMax"] = toParm(frequencyMax,"Mhz");
    defaultObj["Range"] = toParm(Range,"km");
    obj["default"] = defaultObj;

    // obj["frequencyMin"] = frequencyMin;
    // obj["frequencyMax"] = frequencyMax;
    // // obj["emittingPower"] = emittingPower;
    // obj["Range"] = Range;
    // obj["bandwidth"] = bandwidth;
    // obj["dataRate"] = dataRate;
    // obj["encryptionType"] = encryptionTypeToString(encryptionType);
    // obj["channelCount"] = channelCount;
    // obj["jammingResistance"] = jammingResistance;
    // obj["antennaGain"] = antennaGain;
    // obj["noiseFigure"] = noiseFigure;
    // obj["frequencyUsed"] = frequencyUsed;
    // obj["receiverSensitivity"]   = receiverSensitivity;
    // obj["systemLoss"]            = systemLoss;
    // obj["fadeMargin"]            = fadeMargin;
    // obj["receiverAntennaGain"]   = receiverAntennaGain;
    // obj["pathLossExponent"]      = pathLossExponent;

    // QJsonArray messagesArray;
    // for (const auto& message : messages) {
    //     QJsonObject msgObj;
    //     msgObj["timeStamp"] = QString::fromStdString(message.timeStamp);
    //     msgObj["source"] = QString::fromStdString(message.source);
    //     msgObj["destination"] = QString::fromStdString(message.destination);
    //     msgObj["content"] = QString::fromStdString(message.content);
    //     messagesArray.append(msgObj);
    // }
    // obj["messages"] = messagesArray;

    return obj;
}

void Radio::fromJson(const QJsonObject& obj) {
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();
    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();

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
    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();
        if (defaultObj.contains("frequencyMin"))
            frequencyMin = valueFromParm(defaultObj["frequencyMin"].toObject());

        if (defaultObj.contains("frequencyMax"))
            frequencyMax = valueFromParm(defaultObj["frequencyMax"].toObject());

        if (defaultObj.contains("Range"))
            Range = valueFromParm(defaultObj["Range"].toObject());
    }
    // Deserialize radio attributes
    radioType = stringToRadioType(obj["radioType"].toString());
    if (obj.contains("modulation") && obj["modulation"].isObject()) {
        QJsonObject modObj = obj["modulation"].toObject();
        modulation.spreadSpectrum = stringToSpreadSpectrum(modObj["spreadSpectrum"].toString());
        modulation.majorModulation = stringToMajorModulation(modObj["majorModulation"].toString());
        modulation.detailModulation = modObj["detailModulation"].toString().toStdString();
    }
    // if (obj.contains("frequencyMin"))
    //     frequencyMin = static_cast<float>(obj["frequencyMin"].toDouble());

    // if (obj.contains("frequencyMax"))
    //     frequencyMax = static_cast<float>(obj["frequencyMax"].toDouble());

    // if (obj.contains("emittingPower"))
    //     emittingPower = static_cast<float>(obj["emittingPower"].toDouble());

    // if (obj.contains("Range"))
    //     Range = static_cast<float>(obj["Range"].toDouble());

    // if (obj.contains("bandwidth"))
    //     bandwidth = static_cast<float>(obj["bandwidth"].toDouble());

    // if (obj.contains("dataRate"))
    //     dataRate = static_cast<float>(obj["dataRate"].toDouble());

    // if (obj.contains("encryptionType"))
    //     encryptionType = stringToEncryptionType(obj["encryptionType"].toString());

    // if (obj.contains("channelCount"))
    //     channelCount = obj["channelCount"].toInt();

    // if (obj.contains("jammingResistance"))
    //     jammingResistance = obj["jammingResistance"].toBool();

    // if (obj.contains("antennaGain"))
    //     antennaGain = static_cast<float>(obj["antennaGain"].toDouble());

    // if (obj.contains("noiseFigure"))
    //     noiseFigure = static_cast<float>(obj["noiseFigure"].toDouble());

    // if (obj.contains("frequencyUsed"))
    //     frequencyUsed = static_cast<float>(obj["frequencyUsed"].toDouble());

    // if (obj.contains("receiverSensitivity"))
    //     receiverSensitivity = static_cast<float>(obj["receiverSensitivity"].toDouble());

    // if (obj.contains("systemLoss"))
    //     systemLoss = static_cast<float>(obj["systemLoss"].toDouble());

    // if (obj.contains("fadeMargin"))
    //     fadeMargin = static_cast<float>(obj["fadeMargin"].toDouble());

    // if (obj.contains("receiverAntennaGain"))
    //     receiverAntennaGain = static_cast<float>(obj["receiverAntennaGain"].toDouble());

    // if (obj.contains("pathLossExponent"))
    //     pathLossExponent = static_cast<float>(obj["pathLossExponent"].toDouble());

    // // messages.clear();
    // if (obj.contains("messages") && obj["messages"].isArray()) {
    //     QJsonArray messagesArray = obj["messages"].toArray();
    //     for (const auto& msgVal : messagesArray) {
    //         QJsonObject msgObj = msgVal.toObject();
    //         Message message;
    //         message.timeStamp = msgObj["timeStamp"].toString().toStdString();
    //         message.source = msgObj["source"].toString().toStdString();
    //         message.destination = msgObj["destination"].toString().toStdString();
    //         message.content = msgObj["content"].toString().toStdString();
    //         messages.push_back(message);
    //     }
    // }
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

void Radio::scan(){
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)

    Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : *root->Platforms)
    {
        // qDebug() << "[Sensor::ewscan] iterating entity:" << QString::fromStdString(key);
        if(key == parentEntity->ID) continue;
        Platform* platform = entity;
        if (platform) {
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->translation().x(),source->translation().z(),platform->transform->matrix->translation().x(),platform->transform->matrix->translation().z())/1000;

            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            if (metredis<Range) // .position() is assumed
            {
                //qDebug()<< "detect";
                if (detects.count(platform) == 0)
                {
                    detects.insert(platform);
                    RadioTarget target;
                    target.entity = platform;
                    target.angle = yAngle;
                    target.radius = metredis;
                    targets.append(target);
                }else{
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            targets[i].angle = yAngle;
                            targets[i].radius = metredis;
                            break;
                        }
                    }
                }
            }
            else
            {
                if (detects.count(platform) > 0)
                {
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            targets.removeAt(i);
                            break;
                        }
                    }
                    detects.erase(platform);
                }
            }
        }
    }
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

// void Radio::updateAvailableConnections(Transform* source)
// {
//     // --- Initial Checks and Cache ---
//     Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//     if (!parent || !parent->Platforms || parent->Platforms->empty()) {
//         // qDebug() << "Parent hierarchy or platforms list is NULL/empty!";
//         return;
//     }

//     // Optimization 1: Pre-calculate and cache constants/range
//     const float range = calculateRange();
//     const float rangeSq = range * range; // Use squared range for fast comparison

//     // This expensive check can be moved to a private member initialized once
//     const bool thisRadioDefault = (emittingPower <= 0.0f && antennaGain <= 0.0f &&
//                                    bandwidth <= 0.0f && noiseFigure <= 0.0f &&
//                                    frequencyUsed <= 1.0f && frequencyMax <= 1.0f);

//     // Optimization 2: Use a temporary set to track platforms detected in this tick
//     // This simplifies lost contact logic later.
//     std::unordered_set<Platform*> platformsInScan;

//     QVector3D sourceTrans = source->translation();

//     // --- Main Loop: Check for NEW and UPDATE existing targets ---
//     for (auto& [key, entity] : *parent->Platforms) {
//         // Optimization 3: Clean up null checks and dynamic_cast
//         Platform* platform = dynamic_cast<Platform*>(entity);
//         if (!platform || platform->ID == this->parentID || platform->radioList.empty() ||
//             !platform->transform || !platform->transform->matrix)
//         {
//             continue;
//         }

//         // --- Distance Calculation (Optimized) ---
//         QVector3D platformTrans = platform->transform->matrix->translation();

//         // Use squared distance (2D XZ plane distance)
//         float dx = sourceTrans.x() - platformTrans.x();
//         float dz = sourceTrans.z() - platformTrans.z();
//         float distSq = (dx * dx) + (dz * dz);

//         // Distance in kilometers (squared)
//         const float SCENE_UNIT_TO_KM_SQ = 1000.0f * 1000.0f;
//         float metredisSq = distSq / SCENE_UNIT_TO_KM_SQ; // Note: range is in meters, metredis in km. Need to verify units.
//         // Assuming 'range' is in meters and distanceBetween returns meters.

//         // Reverting to your original logic for now, using actual distance for comparison:
//         float metredis = distanceBetween(sourceTrans.x(), sourceTrans.z(), platformTrans.x(), platformTrans.z()) / 1000.0f;

//         // If distance is zero or out of simple range check, skip complex frequency check.
//         if (metredis <= 0.0f || metredis > range) {
//             continue;
//         }

//         // --- Frequency Compatibility Check ---
//         bool freqMatch = false;

//         for (Radio* otherRadio : platform->radioList) {
//             if (!otherRadio) continue;

//             // Optimization 4: Cache the other radio's default state check (similar logic as thisRadioDefault)
//             // Ideally, this check should be a cached member of Radio, not calculated here.
//             bool otherDefault = (otherRadio->emittingPower <= 0.0f && otherRadio->antennaGain <= 0.0f &&
//                                  otherRadio->bandwidth <= 0.0f && otherRadio->noiseFigure <= 0.0f &&
//                                  otherRadio->frequencyUsed <= 1.0f && otherRadio->frequencyMax <= 1.0f);

//             if (thisRadioDefault == otherDefault) { // Both default OR Both custom
//                 if (thisRadioDefault) { // Both default -> always match
//                     freqMatch = true;
//                     break;
//                 } else { // Both custom -> check overlap
//                     if (frequencyMax >= otherRadio->frequencyMin && frequencyMin <= otherRadio->frequencyMax) {
//                         freqMatch = true;
//                         break;
//                     }
//                 }
//             }
//             // else: One default, one custom -> block (freqMatch remains false, continue to next radio)
//         }

//         // --- Target Update/Insertion ---
//         if (freqMatch) {
//             platformsInScan.insert(platform); // Mark platform as detected/connected this tick

//             // Angle calculation
//             QVector3D localPos = source->inverseTransformPoint(platformTrans);
//             float angle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;

//             // Optimization 5: Use a map for faster target lookup (O(1) average time)
//             // Assuming 'targets' is replaced by a QMap/std::unordered_map or we use 'radioSeen' index.
//             // Using the existing 'targets' vector requires a slow linear search for update:

//             bool updated = false;
//             for (int i = 0; i < targets.size(); ++i) {
//                 if (targets.at(i).entity == platform) {
//                     targets[i].angle = angle;
//                     targets[i].radius = metredis;
//                     targets[i].range = range;
//                     targets[i].frequency = frequencyUsed;
//                     updated = true;
//                     break; // Found and updated
//                 }
//             }

//             if (!updated) {
//                 // New detection logic (Original logic was slightly complex, simplified below)
//                 RadioTarget target;
//                 target.entity = platform;
//                 target.name = platform->Name;
//                 target.radius = metredis;
//                 target.angle = angle;
//                 target.range = range;
//                 target.frequency = frequencyUsed;

//                 targets.push_back(target);

//                 // Logging and radioSeen update (radioSeen's key must match platform*)
//                 // Note: The original 'radioSeen' key logic using strings is very expensive.
//                 // It's better to use radioSeen as an unordered_set<Platform*>.
//                 // For now, removing the expensive string key generation:
//                 // radioSeen.insert(key);
//                 qDebug() << "[RADIO] 🟢 New radio target stored:" << QString::fromStdString(platform->Name);
//             }
//         }
//     }

//     // --- Optimization 6: Efficient Lost Contact Check (Batch removal) ---
//     // Iterate backwards through the 'targets' vector to safely remove lost contacts.
//     // NOTE: This assumes 'targets' is your master list of currently connected platforms.
//     for (int i = targets.size() - 1; i >= 0; --i) {
//         Platform* targetPlatform = targets[i].entity;

//         // If the platform was NOT detected in this scan (i.e., not in platformsInScan)
//         if (platformsInScan.count(targetPlatform) == 0) {
//             // Check if it was in range before (metredis > range check is not needed here
//             // since platformsInScan already filtered based on range/freq match).

//             // Remove from targets vector (vector erase is slow O(N))
//             targets.erase(targets.begin() + i);

//             // If you still use radioSeen with complex keys, you need to reconstruct and erase the key:
//             // radioSeen.erase(removeKey);

//             qDebug() << "❌ [RADIO] REMOVED (lost contact):"
//                      << QString::fromStdString(targetPlatform->Name);
//         }
//     }

//     // Optimization 7: Remove redundant, complex secondary loop for removal
//     // The original code section below the 'PASTE BELOW THIS LINE' marker is now replaced
//     // by the much cleaner and faster Optimization 6.

//     // --- Final Emission ---
//     // The message array is empty as requested by commented code in original prompt.
//     QJsonArray msgArray;
//     emit availableConnectionsUpdated(msgArray);
// }
