

#include "iff.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>

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

std::vector<Component*> IFF::getSupportedComponents() {
    return std::vector<Component*>{};
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

void IFF::updateComponent(QString name, const QJsonObject& /*obj*/) {
    Console::error(name.toStdString() + ": IFF does not support components");
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

    return obj;
}

void IFF::fromJson(const QJsonObject& obj) {
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

    // Deserialize IFF attributes
    transponder = obj["transponder"].toBool();
    emittingRange = static_cast<float>(obj["emittingRange"].toDouble());
    emittingFrequency = static_cast<float>(obj["emittingFrequency"].toDouble());
    disType = obj["disType"].toString().toStdString();
    disName = obj["disName"].toString().toStdString();
    operationalMode = stringToOperationalMode(obj["operationalMode"].toString());
    if (obj.contains("modeConfiguration") && obj["modeConfiguration"].isObject()) {
        QJsonObject modeConfigObj = obj["modeConfiguration"].toObject();
        modeConfiguration.mode1 = modeConfigObj["mode1"].toString().toStdString();
        modeConfiguration.mode2 = modeConfigObj["mode2"].toString().toStdString();
        modeConfiguration.mode3A = modeConfigObj["mode3A"].toString().toStdString();
        modeConfiguration.mode4 = modeConfigObj["mode4"].toString().toStdString();
        modeConfiguration.modeC = modeConfigObj["modeC"].toString().toStdString();
    }
    codeSystem = stringToCodeSystem(obj["codeSystem"].toString());
    encryptionType = stringToEncryptionType(obj["encryptionType"].toString());
    spoofable = obj["spoofable"].toBool();
    responseDelay = static_cast<float>(obj["responseDelay"].toDouble());
    lastInterrogationTime = obj["lastInterrogationTime"].toString().toStdString();
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
