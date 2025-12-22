#include "sensorprofile.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/csm.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/esm.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"
#include "core/Hierarchy/hierarchy.h"

SensorProfile::SensorProfile(Hierarchy* h):Component(h) {
    Active = true;
    sensors =  new std::unordered_map<std::string, Sensor*>();
}

void SensorProfile::addSubComponent(std::string name, QString data1, QString data2, QString data3){
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if(data1 == "Generic"){
        Radar* radar = new Radar(parent);
        radar->parentEntity = parentEntity;
        radar->Name = name;
        sensors->insert({radar->ID,radar});
        parent->Sensors->insert({radar->ID,radar});
        emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(radar->ID),QString::fromStdString(name));
    }else
    if(data1 == "CSM"){
        CSM* csm = new CSM(parent);
        csm->parentEntity = parentEntity;
        csm->Name = name;
        sensors->insert({csm->ID,csm});
        parent->Sensors->insert({csm->ID,csm});
        emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(csm->ID),QString::fromStdString(name));
    }else
    if(data1 == "ESM"){
        ESM* esm = new ESM(parent);
        esm->parentEntity = parentEntity;
        esm->Name = name;
        sensors->insert({esm->ID,esm});
        parent->Sensors->insert({esm->ID,esm});
        emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(esm->ID),QString::fromStdString(name));
    }

}

void SensorProfile::removeSubComponent(std::string ID){

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
    return obj;
}

void SensorProfile::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")){
        // ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("sensors") && obj["sensors"].isObject()) {
        QJsonObject sensorsObj = obj["sensors"].toObject();
        for (const QString& key : sensorsObj.keys()) {
            QJsonObject sensorObj = sensorsObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            QString type = sensorObj.value("SensorType").toString();
            std::string id = sensorObj["id"].toString().toStdString();
            Sensor*  sensor;
            bool is = sensors->count(id);
            if ( is > 0) {
                sensor = (*sensors)[id];   // OR iffs->at(id)

            } else
            if(type == "Radar"){
                sensor = new Radar(parent);
            }else if(type == "CSM"){
                sensor = new CSM(parent);
            }else if(type == "ESM"){
                sensor = new ESM(parent);
            }else{
                sensor = new Radar(parent);
            }

            sensor->parentEntity = parentEntity;
            sensor->Name = sensorObj["name"].toString().toStdString();
            sensor->ID = sensorObj["id"].toString().toStdString();
            sensor->fromJson(sensorObj);
            if(!is){
                sensors->insert({sensor->ID,sensor});
                parent->Sensors->insert({sensor->ID,sensor});
                emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(sensor->ID),QString::fromStdString(sensor->Name));
            }

        }
    }
    //Console::log("Collider::fromJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
}
