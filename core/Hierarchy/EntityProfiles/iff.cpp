#include "iff.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <core/Hierarchy/Components/transform.h>
#include "core/Hierarchy/Utils/entityutils.h"
#include <unordered_set>
#include <QVector3D>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// C# Mathf.Rad2Deg के बराबर
const float RAD2DEG = 180.0f / M_PI;
// std::unordered_set<std::string> IFF::iffSeen;

IFF::IFF(Hierarchy* h) : Entity(h) {
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "iff_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["iff_param"] = par;
}

void IFF::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

std::vector<std::string>IFF:: getSupportedComponents() {
    return std::vector<std::string>{};
}

void IFF::addComponent(std::string name) {
    Console::error("IFF does not support components: " + name);
}

void IFF::removeComponent(std::string name) {
    Console::error("IFF does not support components: " + name);
}

QJsonObject IFF::getComponent(std::string name) {
    Console::error("IFF does not support components: " + name);
    return QJsonObject();
}

void IFF::updateComponent(QString name, const QJsonObject& obj) {
    Console::error(name.toStdString() + ": IFF does not support components");

    // Update basic entity fields
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();

    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();

    // Update parameters
    if (obj.contains("parameters")) {
        QJsonObject parObj = obj["parameters"].toObject();
        if (parObj.contains("value")) {
            QJsonObject paramMap = parObj["value"].toObject();
            for (const QString& key : paramMap.keys()) {
                QJsonObject paramObj = paramMap[key].toObject();

                auto it = parameters.find(key.toStdString());
                if (it != parameters.end()) {
                    // Existing parameter → update
                    it->second->fromJson(paramObj);
                } else {
                    // New parameter → create and insert
                    std::shared_ptr<Parameter> param = std::make_shared<Parameter>();
                    param->fromJson(paramObj);
                    parameters[key.toStdString()] = param;
                }
            }
        }
    }

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


QJsonObject IFF::toJson() const {
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

    // Serialize IFF attributes
    obj["transponder"] = transponder;
    obj["emittingRange"] = emittingRange;
    obj["emittingFrequency"] = emittingFrequency;
    obj["disType"] = QString::fromStdString(disType);
    obj["disName"] = QString::fromStdString(disName);
    obj["operationalMode"] = operationalModeToString(operationalMode);
    QJsonObject modeConfigObj;
    modeConfigObj["mode1"] = QString::fromStdString(modeConfiguration.mode1);
    modeConfigObj["mode2"] = QString::fromStdString(modeConfiguration.mode2);
    modeConfigObj["mode3A"] = QString::fromStdString(modeConfiguration.mode3A);
    modeConfigObj["mode4"] = QString::fromStdString(modeConfiguration.mode4);
    modeConfigObj["modeC"] = QString::fromStdString(modeConfiguration.modeC);
    obj["modeConfiguration"] = modeConfigObj;
    obj["codeSystem"] = codeSystemToString(codeSystem);
    obj["encryptionType"] = encryptionTypeToString(encryptionType);
    obj["spoofable"] = spoofable;
    obj["responseDelay"] = responseDelay;
    obj["lastInterrogationTime"] = QString::fromStdString(lastInterrogationTime);
    // --- Serialize message history ---
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

    // --- Deserialize message history ---
    if (obj.contains("messages") && obj["messages"].isArray()) {
        QJsonArray messagesArray = obj["messages"].toArray();
        messages.clear();
        for (const auto& msgVal : messagesArray) {
            QJsonObject msgObj = msgVal.toObject();
            Message msg;
            msg.timeStamp = msgObj["timeStamp"].toString().toStdString();
            msg.source = msgObj["source"].toString().toStdString();
            msg.destination = msgObj["destination"].toString().toStdString();
            msg.content = msgObj["content"].toString().toStdString();
            messages.push_back(msg);
        }
    }

}

QString IFF::operationalModeToString(OperationalMode om) const {
    switch (om) {
    case OperationalMode::Active: return "Active";
    case OperationalMode::Passive: return "Passive";
    case OperationalMode::Off: return "Off";
    case OperationalMode::Simulation: return "Simulation";
    default: return "Off";
    }
}

IFF::OperationalMode IFF::stringToOperationalMode(const QString& str) const {
    if (str == "Active") return OperationalMode::Active;
    if (str == "Passive") return OperationalMode::Passive;
    if (str == "Simulation") return OperationalMode::Simulation;
    return OperationalMode::Off;
}

QString IFF::codeSystemToString(CodeSystem cs) const {
    switch (cs) {
    case CodeSystem::NoPulse: return "NoPulse";
    case CodeSystem::FivePulses: return "FivePulses";
    case CodeSystem::SixPulses: return "SixPulses";
    case CodeSystem::TwelvePulses: return "TwelvePulses";
    default: return "NoPulse";
    }
}

IFF::CodeSystem IFF::stringToCodeSystem(const QString& str) const {
    if (str == "FivePulses") return CodeSystem::FivePulses;
    if (str == "SixPulses") return CodeSystem::SixPulses;
    if (str == "TwelvePulses") return CodeSystem::TwelvePulses;
    return CodeSystem::NoPulse;
}

QString IFF::encryptionTypeToString(EncryptionType et) const {
    switch (et) {
    case EncryptionType::None: return "None";
    case EncryptionType::NATO: return "NATO";
    case EncryptionType::SecureID: return "SecureID";
    default: return "None";
    }
}

IFF::EncryptionType IFF::stringToEncryptionType(const QString& str) const {
    if (str == "NATO") return EncryptionType::NATO;
    if (str == "SecureID") return EncryptionType::SecureID;
    return EncryptionType::None;
}
// Helper to get ISO timestamp string
static std::string nowIsoString() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
}
void IFF::interrogateTargets(Transform* source)
{
    // Critical call trace — keep
    qWarning() << "IFF::interrogateTargets for:" << QString::fromStdString(Name);

    if (!transponder) {
        qWarning() << "[IFF] Transponder OFF – interrogation aborted.";
        return;
    }

    if (!(operationalMode == OperationalMode::Active || operationalMode == OperationalMode::Simulation)) {
        qWarning() << "[IFF] Operational mode prevents interrogation:" << operationalModeToString(operationalMode);
        return;
    }

    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parent) {
        qWarning() << "[IFF] ERROR: Parent hierarchy missing.";
        return;
    }

    Platform* sourcePlatform = nullptr;
    for (auto& [key, entity] : *parent->Entities) {
        if (Platform* plat = dynamic_cast<Platform*>(entity)) {
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
            qWarning() << "[IFF] Removing invalid target record.";
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
    for (auto& [key, entity] : *parent->Entities) {
        Platform* platform = dynamic_cast<Platform*>(entity);
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

            responsesArray.append(resp);
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
                // UPDATED EXISTING TARGET (OPTIMIZED + PER-FRAME MOVEMENT)
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

                        QJsonObject obj;
                        obj["responderId"]   = QString::fromStdString(target.responderId);
                        obj["responderName"] = QString::fromStdString(target.responderName);
                        obj["status"]        = (target.status == 1 ? "Friend" : "Foe");
                        obj["mode"]          = QString::fromStdString(target.mode);
                        obj["code"]          = QString::fromStdString(target.code);

                        QJsonArray arr;
                        arr.append(obj);

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

    result["interrogatorId"] = QString::fromStdString(interrogator ? interrogator->ID : std::string(""));
    result["interrogatorName"] = QString::fromStdString(interrogator ? interrogator->Name : std::string(""));
    result["responderId"] = QString::fromStdString(this->ID);
    result["responderName"] = QString::fromStdString(this->Name);
    result["mode"] = modeStr;
    result["code"] = codeStr;
    result["distanceMeters"] = distanceMeters;
    result["responseDelayMs"] = responseDelay;
    result["status"] = status;
    result["timestamp"] = QString::fromStdString(nowIsoString());

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
