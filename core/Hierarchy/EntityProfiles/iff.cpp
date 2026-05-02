/**
 * @file iff.cpp
 * @brief Implementation of the IFF (Identification Friend or Foe) entity.
 */

#include "iff.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <core/Hierarchy/Components/transform.h>
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include <unordered_set>
#include <QVector3D>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float RAD2DEG = 180.0f / M_PI;

/**
 * @brief Constructs an IFF entity.
 * @param h Pointer to the parent Hierarchy.
 */
IFF::IFF(Hierarchy* h) : Entity(h) {
    type = Constants::EntityType::IFF;
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "iff_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["iff_param"] = par;
}

/**
 * @brief Emits signals to notify the hierarchy that this IFF has been added.
 */
void IFF::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

/**
 * @brief Returns an empty list – IFF does not support child components.
 * @return Empty vector.
 */
std::vector<std::string> IFF::getSupportedComponents() {
    return std::vector<std::string>{};
}

/**
 * @brief Adds a component – not supported for IFF.
 * @param name Component name (ignored).
 */
void IFF::addComponent(std::string name) {
    Console::error("IFF does not support components: " + name);
}

/**
 * @brief Removes a component – not supported for IFF.
 * @param name Component name (ignored).
 */
void IFF::removeComponent(std::string name) {
    Console::error("IFF does not support components: " + name);
}

/**
 * @brief Retrieves component JSON – not supported.
 * @param name Component name.
 * @return Empty JSON object.
 */
QJsonObject IFF::getComponent(std::string name) {
    Console::error("IFF does not support components: " + name);
    return QJsonObject();
}

/**
 * @brief Updates the IFF entity from JSON data (only basic fields and IFF-specific parameters).
 * @param name Component name (unused).
 * @param obj JSON object containing update data.
 */
void IFF::updateComponent(QString name, const QJsonObject& obj) {
    Console::error(name.toStdString() + ": IFF does not support components");

    // Update basic entity fields
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();

    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();

    // Update IFF-specific fields
    if (obj.contains("transponder")) {
        transponder = obj["transponder"].toBool();
        qDebug() << "[IFF] Updated transponder to" << transponder
                 << "for:" << QString::fromStdString(Name);
    } else {
        qDebug() << "[IFF] No 'transponder' key in JSON for:"
                 << QString::fromStdString(Name)
                 << "| Keys are:" << obj.keys();
    }

    if (obj.contains("emittingRange"))
        emittingRange = static_cast<float>(obj["emittingRange"].toDouble());

    if (obj.contains("emittingFrequency"))
        emittingFrequency = static_cast<float>(obj["emittingFrequency"].toDouble());

    if (obj.contains("disType"))
        disType = obj["disType"].toString().toStdString();

    if (obj.contains("disName"))
        disName = obj["disName"].toString().toStdString();

    if (obj.contains("operationalMode"))
        operationalMode = stringToOperationalMode(obj["operationalMode"].toString());

    if (obj.contains("modeConfiguration") && obj["modeConfiguration"].isObject()) {
        QJsonObject modeConfigObj = obj["modeConfiguration"].toObject();
        if (modeConfigObj.contains("mode1"))
            modeConfiguration.mode1 = modeConfigObj["mode1"].toString().toStdString();
        if (modeConfigObj.contains("mode2"))
            modeConfiguration.mode2 = modeConfigObj["mode2"].toString().toStdString();
        if (modeConfigObj.contains("mode3A"))
            modeConfiguration.mode3A = modeConfigObj["mode3A"].toString().toStdString();
        if (modeConfigObj.contains("mode4"))
            modeConfiguration.mode4 = modeConfigObj["mode4"].toString().toStdString();
        if (modeConfigObj.contains("modeC"))
            modeConfiguration.modeC = modeConfigObj["modeC"].toString().toStdString();
    }

    if (obj.contains("codeSystem"))
        codeSystem = stringToCodeSystem(obj["codeSystem"].toString());

    if (obj.contains("encryptionType"))
        encryptionType = stringToEncryptionType(obj["encryptionType"].toString());

    if (obj.contains("spoofable"))
        spoofable = obj["spoofable"].toBool();

    if (obj.contains("responseDelay"))
        responseDelay = static_cast<float>(obj["responseDelay"].toDouble());

    if (obj.contains("lastInterrogationTime"))
        lastInterrogationTime = obj["lastInterrogationTime"].toString().toStdString();
}

/**
 * @brief Serializes the IFF entity to JSON.
 * @return QJsonObject containing IFF parameters.
 */
QJsonObject IFF::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;

    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["transponder"] = transponder;
    defaultObj["emittingRange"] = toParm(emittingRange,"km",  0,  500);
    defaultObj["emittingFrequency"] = toParm(emittingFrequency,"Mhz", 0,  3000);
    defaultObj["code"] = toParm(code,"code");
    obj["default"] = defaultObj;

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    return obj;
}

/**
 * @brief Deserializes the IFF entity from JSON.
 * @param obj JSON object containing IFF data.
 */
void IFF::fromJson(const QJsonObject& obj) {
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();

    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();

    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();

    if (obj.contains("active"))
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

    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();
        if (defaultObj.contains("transponder"))
            transponder = defaultObj["transponder"].isBool();

        if (defaultObj.contains("emittingRange"))
            emittingRange = valueFromParm(defaultObj["emittingRange"].toObject());

        if (defaultObj.contains("emittingFrequency"))
            emittingFrequency = valueFromParm(defaultObj["emittingFrequency"].toObject());

        if (defaultObj.contains("code"))
            code = valueFromParm(defaultObj["code"].toObject());
    }

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }
}

/**
 * @brief Performs IFF scan to detect other IFF transponders within range.
 */
void IFF::scan(){
    if(!Active)return;
    if(!parentEntity) return;
    Transform* source = (root->Platforms)[parentEntity->ID]->transform;
    if(!source) return;

    for (auto& [key, entity] : root->Iffs)
    {
        if(!entity || !entity->parentEntity) continue;
        auto it = root->Platforms.find(entity->parentEntity->ID);
        if (it != root->Platforms.end()) {
            Platform* platform = it->second;
            if(platform->ID == parentEntity->ID || !platform || !platform->transform) continue;
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;

            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            if (entity->Active && metredis < emittingRange && entity->emittingFrequency == emittingFrequency)
            {
                if (detects.count(platform) == 0)
                {
                    detects.insert(platform);
                    IFFTarget target;
                    target.entity = platform;
                    target.angle = yAngle;
                    target.ally = entity->code == code;
                    target.radius = metredis;
                    targets.append(target);
                }else{
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            targets[i].angle = yAngle;
                            targets[i].ally = entity->code == code;
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

/**
 * @brief Converts OperationalMode enum to string.
 * @param om Operational mode.
 * @return String representation.
 */
QString IFF::operationalModeToString(OperationalMode om) const {
    switch (om) {
    case OperationalMode::Active: return "Active";
    case OperationalMode::Passive: return "Passive";
    case OperationalMode::Off: return "Off";
    case OperationalMode::Simulation: return "Simulation";
    default: return "Off";
    }
}

/**
 * @brief Converts string to OperationalMode enum.
 * @param str String representation.
 * @return Corresponding OperationalMode.
 */
IFF::OperationalMode IFF::stringToOperationalMode(const QString& str) const {
    if (str == "Active") return OperationalMode::Active;
    if (str == "Passive") return OperationalMode::Passive;
    if (str == "Simulation") return OperationalMode::Simulation;
    return OperationalMode::Off;
}

/**
 * @brief Converts CodeSystem enum to string.
 * @param cs Code system.
 * @return String representation.
 */
QString IFF::codeSystemToString(CodeSystem cs) const {
    switch (cs) {
    case CodeSystem::NoPulse: return "NoPulse";
    case CodeSystem::FivePulses: return "FivePulses";
    case CodeSystem::SixPulses: return "SixPulses";
    case CodeSystem::TwelvePulses: return "TwelvePulses";
    default: return "NoPulse";
    }
}

/**
 * @brief Converts string to CodeSystem enum.
 * @param str String representation.
 * @return Corresponding CodeSystem.
 */
IFF::CodeSystem IFF::stringToCodeSystem(const QString& str) const {
    if (str == "FivePulses") return CodeSystem::FivePulses;
    if (str == "SixPulses") return CodeSystem::SixPulses;
    if (str == "TwelvePulses") return CodeSystem::TwelvePulses;
    return CodeSystem::NoPulse;
}

/**
 * @brief Converts EncryptionType enum to string.
 * @param et Encryption type.
 * @return String representation.
 */
QString IFF::encryptionTypeToString(EncryptionType et) const {
    switch (et) {
    case EncryptionType::None: return "None";
    case EncryptionType::NATO: return "NATO";
    case EncryptionType::SecureID: return "SecureID";
    default: return "None";
    }
}

/**
 * @brief Converts string to EncryptionType enum.
 * @param str String representation.
 * @return Corresponding EncryptionType.
 */
IFF::EncryptionType IFF::stringToEncryptionType(const QString& str) const {
    if (str == "NATO") return EncryptionType::NATO;
    if (str == "SecureID") return EncryptionType::SecureID;
    return EncryptionType::None;
}

/**
 * @brief Returns current UTC time as ISO string.
 * @return ISO timestamp.
 */
static std::string nowIsoString() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
}

/**
 * @brief Interrogates nearby platforms and processes IFF responses.
 * @param source Transform component of the interrogating platform.
 */
void IFF::interrogateTargets(Transform* source)
{
    if (!transponder) {
        return;
    }

    if (!(operationalMode == OperationalMode::Active || operationalMode == OperationalMode::Simulation)) {
        return;
    }

    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parent) {
        return;
    }

    Platform* sourcePlatform = nullptr;
    for (auto& [key, entity] : parent->Platforms) {
        if (Platform* plat = entity) {
            for (IFF* iff : plat->iffList) {
                if (iff == this) {
                    sourcePlatform = plat;
                    break;
                }
            }
            if (sourcePlatform) break;
        }
    }

    QJsonArray responsesArray;
    float range_m = (emittingRange > 0.0f) ? emittingRange : 5.0f;

    // ================================
    //   CLEANUP OUT-OF-RANGE TARGETS
    // ================================
    for (int i = iffTargets.size() - 1; i >= 0; --i) {
        IFFTarget &t = iffTargets[i];
        if (!t.entity || !t.entity->transform || !t.entity->transform->matrix) {
            localIffSeen.erase(t.responderId);
            iffTargets.removeAt(i);
            continue;
        }

        QVector3D localPos = source->inverseTransformPoint(t.entity->transform->matrix->translation());
        float dist = localPos.length();

        if (dist > range_m) {
            localIffSeen.erase(t.responderId);
            iffTargets.removeAt(i);
        }
    }

    // ======================================
    //   MAIN INTERROGATION LOOP
    // ======================================
    for (auto& [key, entity] : parent->Platforms) {
        Platform* platform = entity;
        if (!platform || platform == sourcePlatform || platform->iffList.empty()) continue;
        if (!platform->transform || !platform->transform->matrix) continue;

        bool anyTransponderOn = false;
        for (IFF* other : platform->iffList) {
            if (other && other->transponder) { anyTransponderOn = true; break; }
        }
        if (!anyTransponderOn) continue;

        QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
        float distance = localPos.length();
        float metredis = distanceBetween(
                             source->translation().x(), source->translation().z(),
                             platform->transform->matrix->translation().x(), platform->transform->matrix->translation().z()
                             ) / 1000;

        if (distance > range_m)
            continue;

        bool responded = false;

        for (IFF* other : platform->iffList) {
            if (!other || !other->transponder) continue;
            if (!(other->operationalMode == OperationalMode::Active ||
                  other->operationalMode == OperationalMode::Passive ||
                  other->operationalMode == OperationalMode::Simulation))
                continue;

            if (responded) continue;

            QJsonObject resp = other->respondToInterrogation(this, distance);
            if (resp.isEmpty()) continue;
            responded = true;
            std::string uid = resp["responderId"].toString().toStdString();

            // =========================
            // NEW TARGET
            // =========================
            if (localIffSeen.find(uid) == localIffSeen.end()) {
                localIffSeen.insert(uid);

                float angle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
                if (angle < 0.0f) angle += 360.0f;
                int status = (resp["status"].toString().compare("friend", Qt::CaseInsensitive) == 0) ? 1 : 0;

                IFFTarget tgt;
                tgt.distance      = metredis;
                tgt.radius        = metredis;
                tgt.angle         = angle;
                tgt.responderId   = uid;
                tgt.responderName = resp["responderName"].toString().toStdString();
                tgt.status        = status;
                tgt.mode          = resp["mode"].toString().toStdString();
                tgt.code          = resp["code"].toString().toStdString();
                tgt.entity        = platform;

                // Avoid duplicates
                bool exists = false;
                for (const IFFTarget& t : iffTargets)
                    if (t.responderId == uid) { exists = true; break; }

                if (!exists) iffTargets.append(tgt);
            }
            else {
                // =========================
                // UPDATED EXISTING TARGET
                // =========================
                for (int i = 0; i < iffTargets.size(); ++i) {
                    IFFTarget &target = iffTargets[i];
                    if (target.responderId != uid) continue;

                    // --- ALWAYS UPDATE POSITION (movement per frame) ---
                    float angle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
                    if (angle < 0.0f) angle += 360.0f;

                    target.distance = metredis;
                    target.radius   = metredis;
                    target.angle    = angle;

                    // --- NEW IDENTITY DATA FROM RESPONSE ---
                    int newStatus = (resp["status"].toString().compare("friend", Qt::CaseInsensitive) == 0) ? 1 : 0;
                    std::string newMode = resp["mode"].toString().toStdString();
                    std::string newCode = resp["code"].toString().toStdString();

                    // Check for identity change
                    bool changed =
                        target.status != newStatus ||
                        target.mode   != newMode   ||
                        target.code   != newCode;

                    // Only emit UI update if identity changed
                    if (changed) {
                        target.status = newStatus;
                        target.mode   = newMode;
                        target.code   = newCode;

                        QJsonArray arr;
                        emit iffContactsUpdated(arr);
                    }
                    break;
                }
            }
        }
    }

    if (!responsesArray.isEmpty())
        emit iffContactsUpdated(responsesArray);
}

/**
 * @brief Responds to an IFF interrogation.
 * @param interrogator The IFF entity that sent the interrogation.
 * @param distanceMeters Distance to the interrogator.
 * @return JSON object containing response data.
 */
QJsonObject IFF::respondToInterrogation(IFF* interrogator, float distanceMeters)
{
    QJsonObject result;

    if (!transponder) return result;

    if (!(operationalMode == OperationalMode::Active ||
          operationalMode == OperationalMode::Passive ||
          operationalMode == OperationalMode::Simulation))
        return result;

    QString modeStr = "Unknown";
    QString codeStr = "0000";
    QString status = "Unknown";

    if (interrogator) {
        const auto& intrMode = interrogator->modeConfiguration;

        bool mode3AMatch = !modeConfiguration.mode3A.empty() && !intrMode.mode3A.empty() &&
                           modeConfiguration.mode3A == intrMode.mode3A;
        bool mode4Match = !modeConfiguration.mode4.empty() && !intrMode.mode4.empty() &&
                          modeConfiguration.mode4 == intrMode.mode4;

        if ((modeConfiguration.mode3A.empty() || mode3AMatch) &&
            (modeConfiguration.mode4.empty() || mode4Match)) {

            if (mode3AMatch) {
                modeStr = "Mode3A";
                codeStr = modeConfiguration.mode3A.c_str();
            } else if (mode4Match) {
                modeStr = "Mode4";
                codeStr = modeConfiguration.mode4.c_str();
            }
            status = "Friend";
        }
    }

    lastInterrogationTime = nowIsoString();

    // Add message to local history
    Message msg;
    msg.timeStamp = nowIsoString();
    msg.source = interrogator ? interrogator->Name : "Unknown";
    msg.destination = this->Name;
    msg.content = "Responded with status: " + status.toStdString() +
                  ", Mode: " + modeStr.toStdString() +
                  ", Code: " + codeStr.toStdString();
    messages.push_back(msg);

    return result;
}

/**
 * @brief Returns the number of current IFF targets.
 * @return Target count.
 */
int IFF::getIFFTargetCount() const
{
    return targets.size();
}

/**
 * @brief Retrieves IFF target data by index.
 * @param index Target index.
 * @param outResponderId Output responder ID.
 * @param outResponderName Output responder name.
 * @param outMode Output mode.
 * @param outCode Output code.
 * @param outDistance Output distance.
 * @param outAngle Output angle.
 * @param outStatus Output status (1 = friend, 0 = foe).
 * @return True if index valid, false otherwise.
 */
bool IFF::getIFFTarget(
    int index,
    std::string& outResponderId,
    std::string& outResponderName,
    std::string& outMode,
    std::string& outCode,
    float& outDistance,
    float& outAngle,
    int& outStatus
    ) const
{
    if (index < 0 || index >= targets.size())
        return false;

    const IFFTarget& t = targets[index];

    outResponderId   = t.responderId;
    outResponderName = t.responderName;
    outMode          = t.mode;
    outCode          = t.code;
    outDistance      = t.distance;
    outAngle         = t.angle;
    outStatus        = t.status;

    return true;
}
