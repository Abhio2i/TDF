
#include "parameter.h"
#include <QJsonObject>
#include <QJsonValue>

Parameter::Parameter() {}



QJsonObject Parameter::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    //obj["type"] = static_cast<int>(type);
    obj["type"] = "paravalue";
    if (std::holds_alternative<int>(value)) obj["value"] = std::get<int>(value);
    else if (std::holds_alternative<float>(value)) obj["value"] = std::get<float>(value);
    else if (std::holds_alternative<double>(value)) obj["value"] = std::get<double>(value);
    else if (std::holds_alternative<std::string>(value)) obj["value"] = QString::fromStdString(std::get<std::string>(value));
    else if (std::holds_alternative<bool>(value)) obj["value"] = std::get<bool>(value);
    else if (std::holds_alternative<char>(value)) obj["value"] = QString(QChar(std::get<char>(value)));

    return obj;
}

void Parameter::fromJson(const QJsonObject& obj) {
    Name = obj["name"].toString().toStdString();
    type = static_cast<Constants::ParameterType>(obj["type"].toInt());

    QJsonValue val = obj["value"];
    switch (type) {
    case Constants::ParameterType::INT: value = val.toInt(); break;
    case Constants::ParameterType::FLOAT: value = static_cast<float>(val.toDouble()); break;
    case Constants::ParameterType::DOUBLE: value = val.toDouble(); break;
    case Constants::ParameterType::STRING: value = val.toString().toStdString(); break;
    case Constants::ParameterType::BOOL: value = val.toBool(); break;
    case Constants::ParameterType::CHAR: value = val.toString().toStdString()[0]; break;
    case Constants::LIST:
    case Constants::ENUM:
        break;
    }
}
