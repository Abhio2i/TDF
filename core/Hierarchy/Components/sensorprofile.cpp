#include "sensorprofile.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/adsbsensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aissensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/csm.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/eosensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/esm.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"
SensorProfile::SensorProfile(Hierarchy* h):Component(h) {
    Active = true;
    sensors =  new std::unordered_map<std::string, Sensor*>();
}

void SensorProfile::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if(data1 == "Generic"){
        Radar* radar = new Radar(parent);
        if(!data2.isEmpty()){
            std::string id = radar->ID;
            radar->fromJson(data3);
            radar->ID = id;
            radar->parentID = parentID;
        }
        radar->parentEntity = parentEntity;
        radar->Name = name;
        sensors->insert({radar->ID,radar});
        parent->Sensors.insert({radar->ID,radar});
        emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(radar->ID),QString::fromStdString(name));
    }else
    if(data1 == "CSM"){
        CSM* csm = new CSM(parent);
        if(!data2.isEmpty()){
            std::string id = csm->ID;
            csm->fromJson(data3);
            csm->ID = id;
            csm->parentID = parentID;
        }
        csm->parentEntity = parentEntity;
        csm->Name = name;
        sensors->insert({csm->ID,csm});
        parent->Sensors.insert({csm->ID,csm});
        emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(csm->ID),QString::fromStdString(name));
    }else
    if(data1 == "ESM"){
        ESM* esm = new ESM(parent);
        if(!data2.isEmpty()){
            std::string id = esm->ID;
            esm->fromJson(data3);
            esm->ID = id;
            esm->parentID = parentID;
        }
        esm->parentEntity = parentEntity;
        esm->Name = name;
        sensors->insert({esm->ID,esm});
        parent->Sensors.insert({esm->ID,esm});
        emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(esm->ID),QString::fromStdString(name));

    } else if(data1 == "EO"){
        EOSensor* ais = new EOSensor(parent);
        ais->parentEntity = parentEntity;
        ais->Name = name;
        sensors->insert({ais->ID, ais});
        parent->Sensors.insert({ais->ID, ais});
        emit parent->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(ais->ID),
            QString::fromStdString(name));
    } else if(data1 == "Sonar"){
        Sonar* sonar = new Sonar(parent);
        sonar->parentEntity = parentEntity;
        sonar->Name = name;
        sensors->insert({sonar->ID, sonar});
        parent->Sensors.insert({sonar->ID, sonar});
        emit parent->subComponentAdded(
        QString::fromStdString(ID),
        QString::fromStdString(sonar->ID),
        QString::fromStdString(name));
    }else if(data1 == "AIS"){
        AISSensor* ais = new AISSensor(parent);
        ais->parentEntity = parentEntity;
        ais->Name = name;
        sensors->insert({ais->ID, ais});
        parent->Sensors.insert({ais->ID, ais});
        emit parent->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(ais->ID),
            QString::fromStdString(name));
    }else if(data1 == "ADSB"){
        ADSBSensor* ais = new ADSBSensor(parent);
        ais->parentEntity = parentEntity;
        ais->Name = name;
        sensors->insert({ais->ID, ais});
        parent->Sensors.insert({ais->ID, ais});
        emit parent->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(ais->ID),
            QString::fromStdString(name));
    } else if (data1 == "AESA") {
        AESARadar* aesa = new AESARadar(parent);
        if (!data2.isEmpty()) {
            std::string id = aesa->ID;
            QJsonObject obj = (parent->Sensors)[data2.toStdString()]->toJson();
            aesa->fromJson(obj);
            aesa->ID = id;
            aesa->parentID = parentID;
        }
        aesa->parentEntity = parentEntity;
        aesa->Name = name;
        sensors->insert({aesa->ID, aesa});
        parent->Sensors.insert({aesa->ID, aesa});
        emit parent->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(aesa->ID),
            QString::fromStdString(name));
    }


}

void SensorProfile::removeSubComponent(std::string ID){
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    auto it = sensors->find(ID);
    if (it != sensors->end() && it->second != nullptr) {
        Sensor *sensor = it->second;
        emit parent->subComponentRemoved(QString::fromStdString(this->ID),QString::fromStdString(ID),QString::fromStdString(sensor->Name));
        sensors->erase(sensor->ID);
        parent->Sensors.erase(sensor->ID);
        delete sensor;
        sensor = nullptr;
    }
}

void SensorProfile::updateSubComponent(std::string ID, const QJsonObject& obj){
    auto it = sensors->find(ID);
    if (it != sensors->end() && it->second != nullptr) {
        it->second->fromJson(obj);
    }

}

QJsonObject SensorProfile::getsubComponentData(std::string ID) const{
    auto it = sensors->find(ID);
    if (it != sensors->end() && it->second != nullptr) {
        return it->second->toJson();
    }
    return QJsonObject(); // Return empty object if not found
}

QJsonObject SensorProfile::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    QJsonObject sensorObj;
    for (const auto& [key, sensorPtr] : *sensors) {
        if (sensorPtr) {
            sensorObj[QString::fromStdString(key)] = sensorPtr->toJson();
        }
    }
    obj["sensors"] = sensorObj;
    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;
    return obj;
}
void SensorProfile::fromJson(const QJsonObject& obj) {
    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("sensors") && obj["sensors"].isObject()) {
        QJsonObject sensorsObj = obj["sensors"].toObject();
        for (const QString& key : sensorsObj.keys()) {
            QJsonObject sensorObj = sensorsObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            QString type = sensorObj.value("SensorType").toString();
            std::string id = sensorObj["id"].toString().toStdString();
            Sensor* sensor = nullptr;
            bool exists = sensors->count(id);

            if (exists) {
                sensor = (*sensors)[id];
            } else if (type == "Radar") {
                sensor = new Radar(parent);
            } else if (type == "CSM") {
                sensor = new CSM(parent);
            } else if (type == "ESM") {
                sensor = new ESM(parent);
            } else if (type == "EO") {
                sensor = new EOSensor(parent);
            } else if (type == "Sonar") {
                sensor = new Sonar(parent);
            } else if (type == "AIS") {
                sensor = new AISSensor(parent);
            } else if (type == "ADSB") {
                sensor = new ADSBSensor(parent);
            } else if (type == "AESA" || type == "AESARadar") {
                sensor = new AESARadar(parent);
            } else {
                sensor = new Radar(parent);      // default
            }

            sensor->parentEntity = parentEntity;
            sensor->Name = sensorObj["name"].toString().toStdString();
            sensor->ID = sensorObj["id"].toString().toStdString();
            sensor->fromJson(sensorObj);

            if (!exists) {
                sensors->insert({sensor->ID, sensor});
                parent->Sensors.insert({sensor->ID, sensor});
                emit parent->subComponentAdded(
                    QString::fromStdString(ID),
                    QString::fromStdString(sensor->ID),
                    QString::fromStdString(sensor->Name));
            }
        }
    }

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }

}
Sensor* SensorProfile::getSensor(const std::string& id) const
{
    if (!sensors) return nullptr;

    auto it = sensors->find(id);
    if (it == sensors->end())
        return nullptr;

    return it->second;
}
void SensorProfile::renameSubComponent(std::string id, QString newName) {
    auto it = sensors->find(id);
    if (it != sensors->end() && it->second != nullptr) {
        Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
        it->second->Name = newName.toStdString();
        emit parent->subComponentRenamed(
            QString::fromStdString(ID),
            QString::fromStdString(id),
            newName);
    }
}
