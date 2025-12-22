#include "crosssection.h"
#include "qjsondocument.h"
#include "core/Debug/console.h"
CrossSection::CrossSection():Component(nullptr) {

}
QJsonObject toParms(float value,QString unit){
    QJsonObject parm;
    parm["type"] = "unitParam";
    parm["value"] = value;
    parm["unit"] = unit;
    return parm;
}

float valueFromParms(const QJsonObject& parm) {
    if (parm.contains("value") ) {
        return parm["value"].toVariant().toDouble();
    }
    return 0.0f; // Default value if key is missing or not a double
}

// toSection: data स्ट्रक्चर को JSON ऑब्जेक्ट में बदलता है
QJsonObject toSection(const CrossSection::data& d, const QString& type) {
    QJsonObject section;
    section["type"] = "Section";
    section["uniformedValue"] = toParms(d.uniformedValue,"%");
    section["modulationValue"] = toParms(d.modulationValue,"%");
    // section["dataType"] = type; // सेक्शन के डेटा टाइप को पहचानें
    return section;
}

// fromSection: JSON ऑब्जेक्ट से data स्ट्रक्चर में मान (values) सेट करता है
void fromSection(CrossSection::data& d, const QJsonObject& section) {
    if (section.contains("uniformedValue") && section["uniformedValue"].isObject())
        d.uniformedValue = valueFromParms(section["uniformedValue"].toObject());
    if (section.contains("modulationValue") && section["modulationValue"].isObject())
        d.modulationValue = valueFromParms(section["modulationValue"].toObject());
}

void CrossSection::addSubComponent(std::string name, QString data1, QString data2, QString data3){

}

void CrossSection::removeSubComponent(std::string ID){

}

void CrossSection::updateSubComponent(std::string ID, const QJsonObject& obj){

}

QJsonObject CrossSection::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

QJsonObject CrossSection::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["type"] = "component"; // यह मानते हुए कि Component क्लास में 'type' नहीं है

    // --- Data Sections ---
    obj["Radar"] = toSection(Radar, "Radar");
    obj["Visual"] = toSection(Visual, "Visual");
    obj["Infrared"] = toSection(Infrared, "Infrared");
    obj["Sonar"] = toSection(Sonar, "Sonar");
    obj["Laser"] = toSection(Laser, "Laser");

    // Include custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    //Console::log("Collider::toJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
    Console::log("Collider::toJson output: " + QString(QJsonDocument(obj).toJson()).toStdString());
    return obj;
}

void CrossSection::fromJson(const QJsonObject& obj) {

    // --- Data Sections ---
    // Radar
    if (obj.contains("Radar") && obj["Radar"].isObject()) {
        fromSection(Radar, obj["Radar"].toObject());
    }

    // Visual
    if (obj.contains("Visual") && obj["Visual"].isObject()) {
        fromSection(Visual, obj["Visual"].toObject());
    }

    // Infrared
    if (obj.contains("Infrared") && obj["Infrared"].isObject()) {
        fromSection(Infrared, obj["Infrared"].toObject());
    }

    // Sonar
    if (obj.contains("Sonar") && obj["Sonar"].isObject()) {
        fromSection(Sonar, obj["Sonar"].toObject());
    }

    // Laser
    if (obj.contains("Laser") && obj["Laser"].isObject()) {
        fromSection(Laser, obj["Laser"].toObject());
    }
    // Merge custom parameters
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key() != "Radar" && it.key() != "Visual" && it.key() != "Infrared" &&
            it.key() != "Sonar" && it.key() != "Laser" ) {
            customParameters[it.key()] = it.value();
        }
    }
    //Console::log("Collider::fromJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
}
