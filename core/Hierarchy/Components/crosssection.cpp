/**
 * @file crosssection.cpp
 * @brief Implementation of the CrossSection component for radar, visual, infrared, sonar, and laser cross‑section data.
 */

#include "crosssection.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qjsondocument.h"
#include "core/Debug/console.h"

/**
 * @brief Constructs a CrossSection component.
 */
CrossSection::CrossSection():Component(nullptr) {

}

/**
 * @brief Helper: Converts a float value with unit to a JSON parameter object.
 * @param value Numerical value.
 * @param unit Unit string (e.g., "%", "m²").
 * @return QJsonObject with type "unitParam", value, and unit.
 */
QJsonObject toParms(float value,QString unit){
    QJsonObject parm;
    parm["type"] = "unitParam";
    parm["value"] = value;
    parm["unit"] = unit;
    return parm;
}

/**
 * @brief Helper: Extracts a float value from a JSON parameter object.
 * @param parm JSON object created by toParms().
 * @return The numeric value, or 0.0f if missing.
 */
float valueFromParms(const QJsonObject& parm) {
    if (parm.contains("value") ) {
        return parm["value"].toVariant().toDouble();
    }
    return 0.0f; // Default value if key is missing or not a double
}

/**
 * @brief Converts a CrossSection::data structure to a JSON "Section" object.
 * @param d The data structure (uniformedValue, modulationValue).
 * @param type Identifier for the section type (e.g., "Radar", "Visual").
 * @return QJsonObject representing the section.
 */
QJsonObject toSection(const CrossSection::data& d, const QString& type) {
    QJsonObject section;
    section["type"] = "Section";
    section["uniformedValue"] = toParm(d.uniformedValue,"%",0,100);
    section["modulationValue"] = toParm(d.modulationValue,"%",0,100);
    // section["dataType"] = type; // Uncomment to store data type identifier
    return section;
}

/**
 * @brief Populates a CrossSection::data structure from a JSON "Section" object.
 * @param d Reference to the data structure to fill.
 * @param section JSON object containing the section data.
 */
void fromSection(CrossSection::data& d, const QJsonObject& section) {
    if (section.contains("uniformedValue") && section["uniformedValue"].isObject())
        d.uniformedValue = valueFromParm(section["uniformedValue"].toObject());
    if (section.contains("modulationValue") && section["modulationValue"].isObject())
        d.modulationValue = valueFromParm(section["modulationValue"].toObject());
}

/**
 * @brief Adds a sub‑component (unused for CrossSection).
 */
void CrossSection::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){

}

/**
 * @brief Removes a sub‑component (unused).
 */
void CrossSection::removeSubComponent(std::string ID){

}

/**
 * @brief Updates a sub‑component (unused).
 */
void CrossSection::updateSubComponent(std::string ID, const QJsonObject& obj){

}

/**
 * @brief Gets sub‑component data (unused).
 */
QJsonObject CrossSection::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

/**
 * @brief Serializes the CrossSection component to JSON.
 * @return QJsonObject containing radar, visual, infrared, sonar, and laser cross‑section data.
 */
QJsonObject CrossSection::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["type"] = "component"; // Assuming the Component class does not have a 'type' member

    // --- Data Sections ---
    obj["Radar"] = toSection(Radar, "Radar");
    obj["Visual"] = toSection(Visual, "Visual");
    obj["Infrared"] = toSection(Infrared, "Infrared");
    obj["Sonar"] = toSection(Sonar, "Sonar");
    obj["Laser"] = toSection(Laser, "Laser");

    // Include custom parameters
    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    //Console::log("CrossSection::toJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
    //Console::log("CrossSection::toJson output: " + QString(QJsonDocument(obj).toJson()).toStdString());
    return obj;
}

/**
 * @brief Deserializes the CrossSection component from JSON.
 * @param obj JSON object containing cross‑section data.
 */
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
    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }

    //Console::log("Collider::fromJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
}
