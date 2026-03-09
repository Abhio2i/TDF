#include "radar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
const float RAD2DEG = 180.0f / M_PI;
Radar::Radar(Hierarchy* h) : Sensor(h)  {
    subType = SubType::Generic;
}

void Radar::scan(){
     if(!parentEntity) return;
    Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
    if(!source) return;
    for (auto& [key, entity] : *root->Platforms)
    {
        if(!entity || key == parentEntity->ID) continue;
        Platform* platform = entity;
        if (platform && platform->transform) {
            QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
            //float distance = localPos.length();
            float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;
            float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
            if (detectCheck(localPos,metredis,2)) // .position() is assumed
            {
                if (detects.count(platform) == 0)
                {
                    detects.insert(platform);
                    Target target;
                    target.entity = platform;
                    if(platform->dynamicModel)
                        target.speed = platform->dynamicModel->currentSpeed;
                    target.angle = yAngle;
                    target.radius = metredis;
                    targets.append(target);
                }else{
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            targets[i].angle = yAngle;
                            if(platform->dynamicModel)
                                targets[i].speed = platform->dynamicModel->currentSpeed;
                            targets[i].radius = metredis;
                            break;
                        }
                    }
                }
            }
            else
            {
                // C# detects.Contains(tr) -> C++ detects.count(tr) > 0
                if (detects.count(platform) > 0)
                {
                    // C# detects.Remove(tr) -> C++ detects.erase(tr)
                    for (int i = 0; i < targets.size(); ++i) {
                        if (targets.at(i).entity == platform) {
                            targets.removeAt(i);
                            break;
                        }
                    }
                    detects.erase(platform);
                    // qDebug()<< "vanish :"<<&entity->Name;
                }
            }
        }
    }
    // qDebug()<<targets.size();
}

QJsonObject Radar::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["name"] = QString::fromStdString(Name);
    obj["Active"] = Active;
    obj["SensorType"] = "Radar";

    QJsonObject capabilitiesObj;
    capabilitiesObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : DetectionCapabilitiesTypeOptions())
        optionsArray.append(opt);
    capabilitiesObj["options"] = optionsArray;
    capabilitiesObj["value"] = detectionCapabilitiesToString(capabilities);

    QJsonObject Emmison;
    Emmison["type"] = "Section";
    Emmison["power"] = toParm(power,"kw",  0,    1000);
    Emmison["frequency"] = toParm(frequency,"Ghz", 0.1,  100);
    obj["Emmison"] = Emmison;

    QJsonObject Envolope;
    Envolope["type"] = "Section";
    Envolope["minAzimuth"] = toParm(minAzimuth,"deg", -180, 0);
    Envolope["maxAzimuth"] = toParm(maxAzimuth,"deg", 0,    180);
    Envolope["minElevation"] = toParm(minElevation,"deg", -90,  0);
    Envolope["maxElevation"] = toParm(maxElevation,"deg", 0,    90);
    obj["Envolope"] = Envolope;

    QJsonObject Scanning;
    Scanning["type"] = "Section";
    Scanning["rate"] = toParm(rate,"hz", 0, 100);
    Scanning["hits"] = toParm(hits,"",   0, 100);
    obj["Scanning"] = Scanning;

    QJsonObject Antenna;
    Antenna["type"] = "Section";
    Antenna["AntennaGain"] = toParm(AntennaGain,"db",  0,    60);
    Antenna["AntennaBandwidth"] = toParm(AntennaBandwidth,"ghz", 0,    10);
    Antenna["beamWidth"] = toParm(beamWidth,"deg", 0,    360);
    Antenna["scanType"] = toParm(scanType,"");
    Antenna["scanTime1"] = toParm(scanTime1,"");
    Antenna["scanTime2"] = toParm(scanTime2,"");
    Antenna["peakSideLobLevel"] = toParm(peakSideLobLevel,"");
    Antenna["avgSideLobLevel"] = toParm(avgSideLobLevel,"");
    obj["Antenna"] = Antenna;

    QJsonObject Pulse;
    Pulse["type"] = "Section";
    Pulse["pulseWidth"] = toParm(pulseWidth,"us", 0, 1000);
    obj["Pulse"] = Pulse;

    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km", 0, 1000);
    defaultObj["frequency"] = toParm(frequency,"Ghz", 0, 1000);
    defaultObj["azimuth"] = toParm(azimuth,"deg", 0,   360);
    defaultObj["DetectionCapabilities"] = capabilitiesObj;
    obj["default"] = defaultObj;
    return obj;
}

void Radar::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")){
        ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("Active"))
        Active = obj["Active"].toBool();

    if (obj.contains("Emmison") && obj["Emmison"].isObject()) {
        QJsonObject Emmison = obj["Emmison"].toObject();
        if (Emmison.contains("power"))
            power = valueFromParm(Emmison["power"].toObject());
        if (Emmison.contains("frequency"))
            frequency = valueFromParm(Emmison["frequency"].toObject());
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

    if (obj.contains("Scanning") && obj["Scanning"].isObject()) {
        QJsonObject Scanning = obj["Scanning"].toObject();
        if (Scanning.contains("rate"))
            rate = valueFromParm(Scanning["rate"].toObject());
        if (Scanning.contains("hits"))
            hits = valueFromParm(Scanning["hits"].toObject());
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

    if (obj.contains("Pulse") && obj["Pulse"].isObject()) {
        QJsonObject Pulse = obj["Pulse"].toObject();
        if (Pulse.contains("pulseWidth"))
            pulseWidth = valueFromParm(Pulse["pulseWidth"].toObject());
    }

    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();
        if (defaultObj.contains("range"))
            range = valueFromParm(defaultObj["range"].toObject());

        if (defaultObj.contains("frequency"))
            frequency = valueFromParm(defaultObj["frequency"].toObject());

        if (defaultObj.contains("azimuth"))
            azimuth = valueFromParm(defaultObj["azimuth"].toObject());

        if (defaultObj.contains("DetectionCapabilities") && defaultObj["DetectionCapabilities"].isObject()) {
            QJsonObject capabilitiesObj = defaultObj["DetectionCapabilities"].toObject();
            if (capabilitiesObj.contains("value"))
                capabilities = stringTodetectionCapabilities(capabilitiesObj["value"].toString());
        }
    }
}
