#include "sensorprofile.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/ECS/Struct/sensor_component.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/adsbsensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aissensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/csm.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/customsensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/eosensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/irsensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/esm.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"


SensorProfile::SensorProfile():Component() {
    Active = true;
    // sensors =  new std::unordered_map<std::string, Sensor*>();
}


ECS::sensorComponent& SensorProfile::getSensorData() const{
    return *parentEntity->root->world.GetComponent<ECS::sensorComponent>(parentEntity->ecsID);
}
SensorProfile::~SensorProfile(){
    while (!sensors.empty()) {
        removeSubComponent(sensors.begin()->second->ID);
    }
    // delete sensors;
    // sensors = nullptr;
}
void SensorProfile::init(Entity *Entity){
    parentEntity = Entity;
    parentEntity->root->world.AddComponent<ECS::sensorComponent>(parentEntity->ecsID);
}
void SensorProfile::Awake(){}
void SensorProfile::Start(){}

// void SensorProfile::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){
//     // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//     if(!parentEntity->root)return;
//     if(data1 == "Generic"){
//         Radar* radar = new Radar(parentEntity->root);
//         // if(!data2.isEmpty()){
//         //     std::string id = radar->ID;
//         //     radar->fromJson(data3);
//         //     radar->ID = id;
//         //     radar->parentID = parentID;
//         // }
//         if(!data2.isEmpty()){
//             radar->fromJson(data3);
//             radar->ID = data2.toStdString();
//             radar->parentID = parentID;
//         }
//         radar->parentEntity = parentEntity;
//         radar->Name = name;
//         sensors.insert({radar->ID,radar});
//         parentEntity->root->Sensors.insert({radar->ID,radar});
//         emit parentEntity->root->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(radar->ID),QString::fromStdString(name));
//         emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(radar->ID));
//     }else
//         if(data1 == "CSM"){
//             CSM* csm = new CSM(parentEntity->root);
//             if(!data2.isEmpty()){
//                 std::string id = csm->ID;
//                 csm->fromJson(data3);
//                 csm->ID = id;
//                 csm->parentID = parentID;
//             }
//             csm->parentEntity = parentEntity;
//             csm->Name = name;
//             sensors.insert({csm->ID,csm});
//             parentEntity->root->Sensors.insert({csm->ID,csm});
//             emit parentEntity->root->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(csm->ID),QString::fromStdString(name));
//             emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(csm->ID));
//         }else
//             if(data1 == "ESM"){
//                 ESM* esm = new ESM(parentEntity->root);
//                 if(!data2.isEmpty()){
//                     std::string id = esm->ID;
//                     esm->fromJson(data3);
//                     esm->ID = id;
//                     esm->parentID = parentID;
//                 }
//                 esm->parentEntity = parentEntity;
//                 esm->Name = name;
//                 sensors.insert({esm->ID,esm});
//                 parentEntity->root->Sensors.insert({esm->ID,esm});
//                 emit parentEntity->root->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(esm->ID),QString::fromStdString(name));
//                 emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(esm->ID));


//             } else if(data1 == "EO"){
//                 EOSensor* ais = new EOSensor(parentEntity->root);
//                 ais->parentEntity = parentEntity;
//                 ais->Name = name;
//                 sensors.insert({ais->ID, ais});
//                 parentEntity->root->Sensors.insert({ais->ID, ais});
//                 emit parentEntity->root->subComponentAdded(
//                     QString::fromStdString(ID),
//                     QString::fromStdString(ais->ID),
//                     QString::fromStdString(name));
//                 emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(ais->ID));

//             } else if(data1 == "IR"){
//                 IRSensor* ais = new IRSensor(parentEntity->root);
//                 ais->parentEntity = parentEntity;
//                 ais->Name = name;
//                 sensors.insert({ais->ID, ais});
//                 parentEntity->root->Sensors.insert({ais->ID, ais});
//                 emit parentEntity->root->subComponentAdded(
//                     QString::fromStdString(ID),
//                     QString::fromStdString(ais->ID),
//                     QString::fromStdString(name));
//                 emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(ais->ID));

//             } else if(data1 == "Sonar"){
//                 Sonar* sonar = new Sonar(parentEntity->root);
//                 sonar->parentEntity = parentEntity;
//                 sonar->Name = name;
//                 sensors.insert({sonar->ID, sonar});
//                 parentEntity->root->Sensors.insert({sonar->ID, sonar});
//                 emit parentEntity->root->subComponentAdded(
//                     QString::fromStdString(ID),
//                     QString::fromStdString(sonar->ID),
//                     QString::fromStdString(name));
//                 emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(sonar->ID));

//             }else if(data1 == "AIS"){
//                 AISSensor* ais = new AISSensor(parentEntity->root);
//                 ais->parentEntity = parentEntity;
//                 ais->Name = name;
//                 sensors.insert({ais->ID, ais});
//                 parentEntity->root->Sensors.insert({ais->ID, ais});
//                 emit parentEntity->root->subComponentAdded(
//                     QString::fromStdString(ID),
//                     QString::fromStdString(ais->ID),
//                     QString::fromStdString(name));
//                 emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(ais->ID));

//             }else if(data1 == "ADSB"){
//                 ADSBSensor* ais = new ADSBSensor(parentEntity->root);
//                 ais->parentEntity = parentEntity;
//                 ais->Name = name;
//                 sensors.insert({ais->ID, ais});
//                 parentEntity->root->Sensors.insert({ais->ID, ais});
//                 emit parentEntity->root->subComponentAdded(
//                     QString::fromStdString(ID),
//                     QString::fromStdString(ais->ID),
//                     QString::fromStdString(name));
//                 emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(ais->ID));

//             } else if (data1 == "AESA") {
//                 AESARadar* aesa = new AESARadar(parentEntity->root);
//                 // if (!data2.isEmpty()) {
//                 //     std::string id = aesa->ID;
//                 //     QJsonObject obj = (parentEntity->root->Sensors)[data2.toStdString()]->toJson();
//                 //     aesa->fromJson(obj);
//                 //     aesa->ID = id;
//                 //     aesa->parentID = parentID;
//                 // }
//                 aesa->parentEntity = parentEntity;
//                 aesa->Name = name;
//                 sensors.insert({aesa->ID, aesa});
//                 parentEntity->root->Sensors.insert({aesa->ID, aesa});
//                 emit parentEntity->root->subComponentAdded(
//                     QString::fromStdString(ID),
//                     QString::fromStdString(aesa->ID),
//                     QString::fromStdString(name));
//                 emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(aesa->ID));

//             } else{
//                 CustomSensorInterface* csensor = nullptr;
//                 if(parentEntity&&parentEntity->root){
//                     std::string iD = Uuid::generateShortUniqueID();
//                     emit parentEntity->root->getCustomSensor(QString::fromStdString(parentEntity->ID),parentEntity->ecsID,QString::fromStdString(iD),csensor);
//                     if(csensor){
//                         CustomSensor* sensor = new CustomSensor(parentEntity->root);
//                         sensor->ID = iD;
//                         sensor->parentEntity = parentEntity;
//                         sensor->sensor = csensor;
//                         sensor->type = data1;
//                         sensor->Name = name;
//                         sensors.insert({sensor->ID, sensor});
//                         parentEntity->root->Sensors.insert({sensor->ID, sensor});
//                         emit parentEntity->root->subComponentAdded(
//                             QString::fromStdString(ID),
//                             QString::fromStdString(sensor->ID),
//                             QString::fromStdString(name));
//                         emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(sensor->ID));
//                         csensor->registerDestroyCallback([this,iD,sensor](){
//                             if(sensor){
//                                 sensor->sensor = nullptr;
//                             }
//                             this->removeSubComponent(iD);
//                         });

//                     }
//                 }

//             }

// }


void SensorProfile::addSubComponent(
    std::string name,
    QString data1,
    QString data2,
    QJsonObject data3)
{
    if (!parentEntity || !parentEntity->root)
        return;

    // ============================================================
    // IMPORTANT:
    // Replay supplies the recorded/stable sensor ID in data2.
    //
    // If this sensor already exists, DO NOT create another sensor.
    // Update the existing sensor instead.
    // ============================================================

    if (!data2.isEmpty())
    {
        auto existingIt =
            sensors.find(data2.toStdString());

        if (existingIt != sensors.end() &&
            existingIt->second != nullptr)
        {
            Sensor* existingSensor = existingIt->second;

            existingSensor->fromJson(data3);
            existingSensor->ID = data2.toStdString();
            existingSensor->Name = name;
            existingSensor->parentEntity = parentEntity;
            if(existingSensor->subType == Sensor::SubType::Generic || existingSensor->subType == Sensor::SubType::AESA){

                existingSensor->dataid = getSensorData().activeCount;
                getSensorData().activeCount++;
            }

            // Keep the global hierarchy registry pointing to
            // the same existing object.
            parentEntity->root->Sensors[data2.toStdString()] =
                existingSensor;

            qDebug()
                << "[SENSOR PROFILE REUSE]"
                << "ID =" << data2
                << "Name =" << QString::fromStdString(name)
                << "Type =" << data1;

            return;
        }
    }


    // ============================================================
    // GENERIC / RADAR
    // ============================================================

    if (data1 == "Generic")
    {
        Radar* radar =
            new Radar(parentEntity->root);

        if (!data2.isEmpty())
        {
            radar->fromJson(data3);
            // radar->ID = data2.toStdString();
            radar->parentID = parentID;
        }

        radar->parentEntity = parentEntity;
        radar->Name = name;

        radar->dataid = getSensorData().activeCount;
        getSensorData().activeCount++;

        sensors[radar->ID] = radar;
        parentEntity->root->Sensors[radar->ID] = radar;

        emit parentEntity->root->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(radar->ID),
            QString::fromStdString(name));

        emit parentEntity->root->sensorAdded(
            QString::fromStdString(parentID),
            QString::fromStdString(radar->ID));
    }


    // ============================================================
    // CSM
    // ============================================================

    else if (data1 == "CSM")
    {
        CSM* csm =
            new CSM(parentEntity->root);

        if (!data2.isEmpty())
        {
            csm->fromJson(data3);
            // csm->ID = data2.toStdString();
            csm->parentID = parentID;
        }

        csm->parentEntity = parentEntity;
        csm->Name = name;

        sensors[csm->ID] = csm;
        parentEntity->root->Sensors[csm->ID] = csm;

        emit parentEntity->root->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(csm->ID),
            QString::fromStdString(name));

        emit parentEntity->root->sensorAdded(
            QString::fromStdString(parentID),
            QString::fromStdString(csm->ID));
    }


    // ============================================================
    // ESM
    // ============================================================

    else if (data1 == "ESM")
    {
        ESM* esm =
            new ESM(parentEntity->root);

        if (!data2.isEmpty())
        {
            esm->fromJson(data3);
            // esm->ID = data2.toStdString();
            esm->parentID = parentID;
        }

        esm->parentEntity = parentEntity;
        esm->Name = name;

        sensors[esm->ID] = esm;
        parentEntity->root->Sensors[esm->ID] = esm;

        emit parentEntity->root->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(esm->ID),
            QString::fromStdString(name));

        emit parentEntity->root->sensorAdded(
            QString::fromStdString(parentID),
            QString::fromStdString(esm->ID));
    }


    // ============================================================
    // EO
    // ============================================================

    else if (data1 == "EO")
    {
        EOSensor* ais =
            new EOSensor(parentEntity->root);

        if (!data2.isEmpty())
        {
            ais->fromJson(data3);
            // ais->ID = data2.toStdString();
        }

        ais->parentEntity = parentEntity;
        ais->Name = name;

        sensors[ais->ID] = ais;
        parentEntity->root->Sensors[ais->ID] = ais;

        emit parentEntity->root->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(ais->ID),
            QString::fromStdString(name));

        emit parentEntity->root->sensorAdded(
            QString::fromStdString(parentID),
            QString::fromStdString(ais->ID));
    }


    // ============================================================
    // IR
    // ============================================================

    else if (data1 == "IR")
    {
        IRSensor* ais =
            new IRSensor(parentEntity->root);

        if (!data2.isEmpty())
        {
            ais->fromJson(data3);
            // ais->ID = data2.toStdString();
        }

        ais->parentEntity = parentEntity;
        ais->Name = name;

        sensors[ais->ID] = ais;
        parentEntity->root->Sensors[ais->ID] = ais;

        emit parentEntity->root->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(ais->ID),
            QString::fromStdString(name));

        emit parentEntity->root->sensorAdded(
            QString::fromStdString(parentID),
            QString::fromStdString(ais->ID));
    }


    // ============================================================
    // Sonar
    // ============================================================

    else if (data1 == "Sonar")
    {
        Sonar* sonar =
            new Sonar(parentEntity->root);

        if (!data2.isEmpty())
        {
            sonar->fromJson(data3);
            // sonar->ID = data2.toStdString();
        }

        sonar->parentEntity = parentEntity;
        sonar->Name = name;

        sensors[sonar->ID] = sonar;
        parentEntity->root->Sensors[sonar->ID] = sonar;

        emit parentEntity->root->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(sonar->ID),
            QString::fromStdString(name));

        emit parentEntity->root->sensorAdded(
            QString::fromStdString(parentID),
            QString::fromStdString(sonar->ID));
    }


    // ============================================================
    // AIS
    // ============================================================

    else if (data1 == "AIS")
    {
        AISSensor* ais =
            new AISSensor(parentEntity->root);

        if (!data2.isEmpty())
        {
            ais->fromJson(data3);
            // ais->ID = data2.toStdString();
        }

        ais->parentEntity = parentEntity;
        ais->Name = name;

        sensors[ais->ID] = ais;
        parentEntity->root->Sensors[ais->ID] = ais;

        emit parentEntity->root->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(ais->ID),
            QString::fromStdString(name));

        emit parentEntity->root->sensorAdded(
            QString::fromStdString(parentID),
            QString::fromStdString(ais->ID));
    }


    // ============================================================
    // ADSB
    // ============================================================

    else if (data1 == "ADSB")
    {
        ADSBSensor* ais =
            new ADSBSensor(parentEntity->root);

        if (!data2.isEmpty())
        {
            ais->fromJson(data3);
            // ais->ID = data2.toStdString();
        }

        ais->parentEntity = parentEntity;
        ais->Name = name;

        sensors[ais->ID] = ais;
        parentEntity->root->Sensors[ais->ID] = ais;

        emit parentEntity->root->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(ais->ID),
            QString::fromStdString(name));

        emit parentEntity->root->sensorAdded(
            QString::fromStdString(parentID),
            QString::fromStdString(ais->ID));
    }


    // ============================================================
    // AESA
    // ============================================================

    else if (data1 == "AESA")
    {
        AESARadar* aesa =
            new AESARadar(parentEntity->root);

        if (!data2.isEmpty())
        {
            aesa->fromJson(data3);
            // aesa->ID = data2.toStdString();
            aesa->parentID = parentID;
        }

        // aesa->parentEntity = parentEntity;
        // aesa->Name = name;

        // sensors[aesa->ID] = aesa;
        aesa->parentEntity = parentEntity;
        aesa->Name = name;

        aesa->dataid = getSensorData().activeCount;
        getSensorData().activeCount++;
        if (aesa->emitterIndex <= 0) {
            aesa->emitterIndex = m_nextEmitterIndex++;
        } else {
            m_nextEmitterIndex = std::max(m_nextEmitterIndex, aesa->emitterIndex + 1);
        }
        sensors[aesa->ID] = aesa;
        parentEntity->root->Sensors[aesa->ID] = aesa;
        qDebug() << "[INDEX-CHECK] aesa" << QString::fromStdString(aesa->ID)
                 << "assigned emitterIndex=" << aesa->emitterIndex;
        emit parentEntity->root->subComponentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(aesa->ID),
            QString::fromStdString(name));

        emit parentEntity->root->sensorAdded(
            QString::fromStdString(parentID),
            QString::fromStdString(aesa->ID));
    }


    // ============================================================
    // CUSTOM SENSOR
    // ============================================================

    else
    {
        CustomSensorInterface* csensor = nullptr;

        if (parentEntity && parentEntity->root)
        {
            std::string iD =
                data2.isEmpty()
                    ? Uuid::generateShortUniqueID()
                    : data2.toStdString();

            emit parentEntity->root->getCustomSensor(
                QString::fromStdString(parentEntity->ID),
                parentEntity->ecsID,
                QString::fromStdString(iD),
                csensor);

            if (csensor)
            {
                CustomSensor* sensor =
                    new CustomSensor(parentEntity->root);

                sensor->ID = iD;
                sensor->parentEntity = parentEntity;
                sensor->sensor = csensor;
                sensor->type = data1;
                sensor->Name = name;

                sensors[sensor->ID] = sensor;
                parentEntity->root->Sensors[sensor->ID] = sensor;

                emit parentEntity->root->subComponentAdded(
                    QString::fromStdString(ID),
                    QString::fromStdString(sensor->ID),
                    QString::fromStdString(name));

                emit parentEntity->root->sensorAdded(
                    QString::fromStdString(parentID),
                    QString::fromStdString(sensor->ID));

                csensor->registerDestroyCallback(
                    [this, iD, sensor]()
                    {
                        if (sensor)
                            sensor->sensor = nullptr;

                        this->removeSubComponent(iD);
                    });
            }
        }
    }
}

void SensorProfile::removeSubComponent(std::string ID){
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parentEntity->root) {
        return; // Guard against null parent pointer
    }
    auto it = sensors.find(ID);
    if (it != sensors.end() && it->second != nullptr) {
        Sensor *sensor = it->second;

        // Disconnect all Qt signals FIRST — stops updateRadar() firing on dead pointer
        QObject* sensorObj = dynamic_cast<QObject*>(sensor);
        if (sensorObj)
            QObject::disconnect(sensorObj, nullptr, nullptr, nullptr);

        // Remove from maps before emit
        sensors.erase(sensor->ID);
        parentEntity->root->Sensors.erase(sensor->ID);
        emit parentEntity->root->sensorRemoved(QString::fromStdString(parentID), QString::fromStdString(ID));
        // Now safe to emit — sensor pointer still valid, signals already cut
        emit parentEntity->root->subComponentRemoved(QString::fromStdString(this->ID),QString::fromStdString(ID),QString::fromStdString(sensor->Name));

        // Safe to delete — nothing can call into it anymore
        delete sensor;
        sensor = nullptr;
    }
}

void SensorProfile::updateSubComponent(std::string ID, const QJsonObject& obj){
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this); // << ADD
    auto it = sensors.find(ID);
    if (it != sensors.end() && it->second != nullptr) {
        it->second->fromJson(obj);
        emit parentEntity->root->sensorUpdated(QString::fromStdString(ID)); // << ADD
    }
}

QJsonObject SensorProfile::getsubComponentData(std::string ID) const{
    auto it = sensors.find(ID);
    if (it != sensors.end() && it->second != nullptr) {
        return it->second->toJson();
    }
    return QJsonObject(); // Return empty object if not found
}

QJsonObject SensorProfile::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    QJsonObject sensorObj;
    for (const auto& [key, sensorPtr] : sensors) {
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
        // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);

        for (const QString& key : sensorsObj.keys()) {
            if(!parentEntity->root)break;
            QJsonObject sensorObj = sensorsObj[key].toObject();

            QString type = sensorObj.value("SensorType").toString();
            std::string id = sensorObj["id"].toString().toStdString();
            Sensor* sensor = nullptr;
            bool exists = sensors.count(id);

            if (exists) {
                sensor = (sensors)[id];
            } else if (type == "Radar") {
                sensor = new Radar(parentEntity->root);
            } else if (type == "CSM") {
                sensor = new CSM(parentEntity->root);
            } else if (type == "ESM") {
                sensor = new ESM(parentEntity->root);
            } else if (type == "EO") {
                sensor = new EOSensor(parentEntity->root);
            } else if (type == "IR") {
                sensor = new IRSensor(parentEntity->root);
            } else if (type == "Sonar") {
                sensor = new Sonar(parentEntity->root);
            } else if (type == "AIS") {
                sensor = new AISSensor(parentEntity->root);
            } else if (type == "ADSB") {
                sensor = new ADSBSensor(parentEntity->root);
            } else if (type == "AESA" || type == "AESARadar") {
                sensor = new AESARadar(parentEntity->root);
            } else {
                sensor = new CustomSensor(parentEntity->root);      // default
            }

            // sensor->parentEntity = parentEntity;
            // sensor->Name = sensorObj["name"].toString().toStdString();
            // std::string newId = sensor->ID;
            // //sensor->ID = sensorObj["id"].toString().toStdString();
            // sensor->fromJson(sensorObj);
            // // sensor->ID = newId;

            // // if (!exists) {
            // sensor->ID = newId;
            // if (AESARadar* aesaSensor = dynamic_cast<AESARadar*>(sensor)) {
            //     if (aesaSensor->emitterIndex <= 0) {
            //         aesaSensor->emitterIndex = m_nextEmitterIndex++;
            //     } else {
            //         m_nextEmitterIndex = std::max(m_nextEmitterIndex, aesaSensor->emitterIndex + 1);
            //     }
            // }
            // if (!exists) {
            //     sensors.insert({sensor->ID, sensor});
            //     parentEntity->root->Sensors.insert({sensor->ID, sensor});
            //     emit parentEntity->root->subComponentAdded(
            //         QString::fromStdString(ID),
            //         QString::fromStdString(sensor->ID),
            //         QString::fromStdString(sensor->Name));
            // }
            sensor->parentEntity = parentEntity;
            sensor->Name = sensorObj["name"].toString().toStdString();
            std::string newId = sensor->ID;
            //sensor->ID = sensorObj["id"].toString().toStdString();
            sensor->fromJson(sensorObj);
            sensor->ID = newId;
            if (!exists) {
                if (AESARadar* aesaSensor = dynamic_cast<AESARadar*>(sensor)) {
                    if (aesaSensor->emitterIndex <= 0) {
                        aesaSensor->emitterIndex = m_nextEmitterIndex++;
                    } else {
                        m_nextEmitterIndex = std::max(m_nextEmitterIndex, aesaSensor->emitterIndex + 1);
                    }
                }
                sensors.insert({sensor->ID, sensor});
                if(sensor->subType == Sensor::SubType::Generic || sensor->subType == Sensor::SubType::AESA){

                    sensor->dataid = getSensorData().activeCount;
                    getSensorData().activeCount++;
                }
                parentEntity->root->Sensors.insert({sensor->ID, sensor});
                emit parentEntity->root->subComponentAdded(
                    QString::fromStdString(ID),
                    QString::fromStdString(sensor->ID),
                    QString::fromStdString(sensor->Name));
                // Same notification as addSubComponent()/addSensor(), so
                // listeners (DIS emission wiring, plugins) also see sensors
                // created by scenario load.
                emit parentEntity->root->sensorAdded(
                    QString::fromStdString(parentID),
                    QString::fromStdString(sensor->ID));
            }
        }
    }

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }

}
Sensor* SensorProfile::getSensor(const std::string& id) const
{
    // if (!sensors) return nullptr;

    auto it = sensors.find(id);
    if (it == sensors.end())
        return nullptr;

    return it->second;
}
void SensorProfile::renameSubComponent(std::string id, QString newName) {
    auto it = sensors.find(id);
    if (it != sensors.end() && it->second != nullptr) {
        // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
        it->second->Name = newName.toStdString();
        emit parentEntity->root->subComponentRenamed(
            QString::fromStdString(ID),
            QString::fromStdString(id),
            newName);
    }
}
void SensorProfile::addSensor(Sensor* sensor)
{
    if (!sensor) return;

    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);

    // Agar already exist karta hai toh skip
    if (sensors.count(sensor->ID)) return;

    sensor->parentEntity = parentEntity;
    if (AESARadar* aesaSensor = dynamic_cast<AESARadar*>(sensor)) {
        if (aesaSensor->emitterIndex <= 0) {
            aesaSensor->emitterIndex = m_nextEmitterIndex++;
        } else {
            m_nextEmitterIndex = std::max(m_nextEmitterIndex, aesaSensor->emitterIndex + 1);
        }
    }
    sensors.insert({sensor->ID, sensor});
    parentEntity->root->Sensors.insert({sensor->ID, sensor});

    emit parentEntity->root->subComponentAdded(
        QString::fromStdString(ID),
        QString::fromStdString(sensor->ID),
        QString::fromStdString(sensor->Name));

    emit parentEntity->root->sensorAdded(
        QString::fromStdString(parentID),
        QString::fromStdString(sensor->ID));
}
