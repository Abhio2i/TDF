#include "radio.h"
#include <core/Hierarchy/hierarchy.h> // Include full Hierarchy definition
#include <core/Debug/console.h>
#include "core/Hierarchy/Utils/entityutils.h"
#include <core/GlobalRegistry.h>
#include <cmath>
#include <QtMath>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float RAD2DEG = 180.0f / M_PI;

Radio::Radio(Hierarchy* h) : Entity(h) {
    type = Constants::EntityType::Radio;
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

    QJsonObject Transmitter;
    Transmitter["type"] = "Section";
    Transmitter["minFrequency"] = toParm(minFrequency,"Mhz", 0,    30000);
    Transmitter["maxFrequency"] = toParm(maxFrequency,"Mhz", 0,    30000);
    Transmitter["Range"] = toParm(Range,"");
    Transmitter["powerDegradation"] = toParm(powerDegradation,"");
    obj["Transmitter"] = Transmitter;

    QJsonObject Envolope;
    Envolope["type"] = "Section";
    Envolope["minAzimuth"] = toParm(minAzimuth,"deg", -180, 0);
    Envolope["maxAzimuth"] = toParm(maxAzimuth,"deg", 0, 180);
    Envolope["minElevation"] = toParm(minElevation,"deg", -90, 0);
    Envolope["maxElevation"] = toParm(maxElevation,"deg", 0, 90);
    obj["Envolope"] = Envolope;

    QJsonObject Modulation;
    Modulation["type"] = "Section";
    Modulation["spreadSpecturm"] = toParm(spreadSpecturm,"");
    Modulation["majorModulation"] = toParm(majorModulation,"");
    Modulation["detail"] = toParm(detail,"");
    Modulation["detailModulation"] = toParm(detailModulation,"");
    obj["Modulation"] = Modulation;

    QJsonObject Pulse;
    Pulse["type"] = "Section";
    Pulse["pulseWidth"] = toParm(pulseWidth,"Mhz",0,500);
    obj["Pulse"] = Pulse;

    QJsonObject Antenna;
    Antenna["type"] = "Section";
    Antenna["AntennaGain"] = toParm(AntennaGain,"");
    Antenna["AntennaBandwidth"] = toParm(AntennaBandwidth,"");
    Antenna["beamWidth"] = toParm(beamWidth,"");
    Antenna["scanType"] = toParm(scanType,"");
    Antenna["scanTime1"] = toParm(scanTime1,"");
    Antenna["scanTime2"] = toParm(scanTime2,"");
    Antenna["peakSideLobLevel"] = toParm(peakSideLobLevel,"");
    Antenna["avgSideLobLevel"] = toParm(avgSideLobLevel,"");
    obj["Antenna"] = Antenna;



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

    if (obj.contains("Transmitter") && obj["Transmitter"].isObject()) {
        QJsonObject Transmitter = obj["Transmitter"].toObject();
        if (Transmitter.contains("minFrequency"))
            minFrequency = valueFromParm(Transmitter["minFrequency"].toObject());
        if (Transmitter.contains("maxFrequency"))
            maxFrequency = valueFromParm(Transmitter["maxFrequency"].toObject());
        if (Transmitter.contains("Range"))
            Range = valueFromParm(Transmitter["Range"].toObject());
        if (Transmitter.contains("powerDegradation"))
            powerDegradation = valueFromParm(Transmitter["powerDegradation"].toObject());
    }

    if (obj.contains("Envolope") && obj["Envolope"].isObject()) {
        QJsonObject Envolope = obj["Envolope"].toObject();
        if (Envolope.contains("minAzimuth"))
            minAzimuth = valueFromParm(Envolope["minAzimuth"].toObject());
        if (Envolope.contains("maxAzimuth"))
            maxAzimuth = valueFromParm(Envolope["maxAzimuth"].toObject());
        if (Envolope.contains("minElevation"))
            minElevation = valueFromParm(Envolope["minElevation"].toObject());
        if (Envolope.contains("maxElevation"))
            maxElevation = valueFromParm(Envolope["maxElevation"].toObject());
    }

    if (obj.contains("Modulation") && obj["Modulation"].isObject()) {
        QJsonObject Modulation = obj["Modulation"].toObject();
        if (Modulation.contains("minAzimuth"))
            spreadSpecturm = valueFromParm(Modulation["minAzimuth"].toObject());
        if (Modulation.contains("maxAzimuth"))
            majorModulation = valueFromParm(Modulation["maxAzimuth"].toObject());
        if (Modulation.contains("minElevation"))
            detail = valueFromParm(Modulation["minElevation"].toObject());
        if (Modulation.contains("maxElevation"))
            detailModulation = valueFromParm(Modulation["maxElevation"].toObject());
    }

    if (obj.contains("Pulse") && obj["Pulse"].isObject()) {
        QJsonObject Pulse = obj["Pulse"].toObject();
        if (Pulse.contains("pulseWidth"))
            pulseWidth = valueFromParm(Pulse["pulseWidth"].toObject());
    }

    if (obj.contains("Antenna") && obj["Antenna"].isObject()) {
        QJsonObject Antenna = obj["Antenna"].toObject();
        if (Antenna.contains("AntennaGain"))
            AntennaGain = valueFromParm(Antenna["AntennaGain"].toObject());
        if (Antenna.contains("AntennaBandwidth"))
            AntennaBandwidth = valueFromParm(Antenna["AntennaBandwidth"].toObject());
        if (Antenna.contains("beamWidth"))
            beamWidth = valueFromParm(Antenna["beamWidth"].toObject());
        if (Antenna.contains("scanType"))
            scanType = valueFromParm(Antenna["scanType"].toObject());
        if (Antenna.contains("scanTime1"))
            scanTime1 = valueFromParm(Antenna["scanTime1"].toObject());
        if (Antenna.contains("scanTime2"))
            scanTime2 = valueFromParm(Antenna["scanTime2"].toObject());
        if (Antenna.contains("peakSideLobLevel"))
            peakSideLobLevel = valueFromParm(Antenna["peakSideLobLevel"].toObject());
        if (Antenna.contains("avgSideLobLevel"))
            avgSideLobLevel = valueFromParm(Antenna["avgSideLobLevel"].toObject());
    }

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

}



void Radio::scan(){
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
    if(!parentEntity) return;
    Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
    if(!source) return;
    // C# foreach (Transform tr in targets) -> C++ range-based for loop
    for (auto& [key, entity] : *root->Radios)
    {
        if(!entity || !entity->parentEntity) continue;
        auto it = root->Platforms->find(entity->parentEntity->ID);
        if (it != root->Platforms->end()) {
            Platform* platform = it->second;
            // Aapka aage ka logic yahan aaye
            // qDebug() << "[Sensor::ewscan] iterating entity:" << QString::fromStdString(key);
            if(platform->ID == parentEntity->ID || !platform || !platform->transform) continue;
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;
            bool connect = false;
            for(int f = minFrequency;f<=maxFrequency;f++){
                if(f<=entity->maxFrequency && f>=entity->minFrequency){
                    connect = true;
                    break;
                }
            }

            // horizontal angle (Y axis) : x vs z
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            //qDebug()<<localPos<<","<<yAngle;
            if (metredis<Range&&connect) // .position() is assumed
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
int Radio::getRadioTargetCount() const
{
    return targets.size();
}

bool Radio::getRadioTarget(
    int index,
    std::string& outName,
    float& outRadius,
    float& outAngle,
    float& outRange,
    float& outFrequency
    ) const
{
    if (index < 0 || index >= targets.size())
        return false;

    const RadioTarget& t = targets[index];

    outName      = t.name;
    outRadius    = t.radius;
    outAngle     = t.angle;
    outRange     = t.range;
    outFrequency = t.frequency;

    return true;
}

