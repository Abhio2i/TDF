
#include "radio.h"
#include <core/Hierarchy/hierarchy.h> // Include full Hierarchy definition
#include <core/Debug/console.h>
#include <core/GlobalRegistry.h>

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
    obj["bandwidth"] = bandwidth;
    obj["dataRate"] = dataRate;
    obj["encryptionType"] = encryptionTypeToString(encryptionType);
    obj["channelCount"] = channelCount;
    obj["jammingResistance"] = jammingResistance;
    obj["antennaGain"] = antennaGain;
    obj["noiseFigure"] = noiseFigure;
    obj["frequencyUsed"] = frequencyUsed;

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
    frequencyMin = static_cast<float>(obj["frequencyMin"].toDouble());
    frequencyMax = static_cast<float>(obj["frequencyMax"].toDouble());
    emittingPower = static_cast<float>(obj["emittingPower"].toDouble());
    bandwidth = static_cast<float>(obj["bandwidth"].toDouble());
    dataRate = static_cast<float>(obj["dataRate"].toDouble());
    encryptionType = stringToEncryptionType(obj["encryptionType"].toString());
    channelCount = obj["channelCount"].toInt();
    jammingResistance = obj["jammingResistance"].toBool();
    antennaGain = static_cast<float>(obj["antennaGain"].toDouble());
    noiseFigure = static_cast<float>(obj["noiseFigure"].toDouble());
    frequencyUsed = static_cast<float>(obj["frequencyUsed"].toDouble());

    messages.clear();
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
