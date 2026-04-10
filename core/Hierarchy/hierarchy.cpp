#include "hierarchy.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Simulation/simulation.h"
#include "core/Config/scenarioconfig.h"
#include <core/Debug/console.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>

// Thread-local context pointer
thread_local Hierarchy* Hierarchy::currentContext = nullptr;

// Constructor
Hierarchy::Hierarchy()
{

    setCurrentContext(this);
}



Hierarchy::~Hierarchy()
{

    Entities.clear();  // Pehle clear karein

    Folders.clear();   // Pehle clear karein

    ProfileCategories.clear(); // Pehle clear karein

    // Baaki code same rakhein

}

ProfileCategaory* Hierarchy::addProfileCategaory(QString profileName)
{
    if(profileName.isEmpty()){
        return nullptr;
    }
    emit status("add");
    ProfileCategaory* profile = new ProfileCategaory(this);
    profile->setProfileType(Constants::EntityType::Platform);
    profile->Name = profileName.toStdString();
    ProfileCategories.insert({profile->ID, profile});
    emit profileAddedPointer(profile);
    emit profileAdded(QString::fromStdString(profile->ID), QString::fromStdString(profile->Name));
    return profile;
}

void Hierarchy::addProfileCategaoryWithObject(ProfileCategaory *profile)
{
    if(profile == nullptr){
        return;
    }
    emit status("add");
    ProfileCategories.insert({profile->ID, profile});
    emit profileAddedPointer(profile);
    emit profileAdded(QString::fromStdString(profile->ID), QString::fromStdString(profile->Name));
}

void Hierarchy::removeProfileCategaory(QString ID)
{
    if(ID.isEmpty()){
        return;
    }
    std::string key = ID.toStdString();
    auto it = ProfileCategories.find(key);

    if (it != ProfileCategories.end()) {
        emit status("delete");
        delete it->second;
        ProfileCategories.erase(it);
        emit profileRemoved(ID);
    }
}

void Hierarchy::renameProfileCategaory(QString Id, QString name)
{
    if(Id.isEmpty() || name.isEmpty()){
        return;
    }
    std::string key = Id.toStdString();
    auto it = ProfileCategories.find(key);

    if (it != ProfileCategories.end()) {
        emit status("rename");
        it->second->Name = name.toStdString();
        emit profileRenamed(Id, name);
    }
}

ProfileCategaory* Hierarchy::getProfileById(QString ID){
    if(ID.isEmpty()){
        return nullptr;
    }
    std::string key = ID.toStdString();
    auto it = ProfileCategories.find(key);
    if (it != ProfileCategories.end()) {
        return it->second;
    }else{
        return nullptr;
    }
}

ProfileCategaory* Hierarchy::getProfileByName(QString name){
    if(name.isEmpty()){
        return nullptr;
    }
    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr) {
            if(QString::fromStdString(profilePtr->Name).contains(name)){
                return profilePtr;
            }
        }
    }
    return nullptr;
}

Folder* Hierarchy::addFolder(QString parentId, QString FolderName, bool Profile)
{
    if(parentId.isEmpty() || FolderName.isEmpty()){
        return nullptr;
    }
    emit status("add");
    std::string pId = parentId.toStdString();
    std::string fName = FolderName.toStdString();
    if (Profile) {
        auto it = ProfileCategories.find(pId);
        if (it != ProfileCategories.end()) {
            return it->second->addFolder(fName);
        }
    } else {
        auto it = Folders.find(pId);
        if (it != Folders.end()) {
            return it->second->addFolder(fName);
        }
    }
    return nullptr;
}

void Hierarchy::addFolderViaNetwork(QString parentId, QString ID, QString FolderName, bool Profile)
{
    if(parentId.isEmpty() || ID.isEmpty() || FolderName.isEmpty()){
        return;
    }
    emit status("add");
    std::string pId = parentId.toStdString();
    std::string Id = ID.toStdString();
    std::string fName = FolderName.toStdString();
    if (Profile) {
        auto it = ProfileCategories.find(pId);
        if (it != ProfileCategories.end()) {
            it->second->addFolder(fName,Id);
        }
    } else {
        auto it = Folders.find(pId);
        if (it != Folders.end()) {
            it->second->addFolder(fName,Id);
        }
    }
}

void Hierarchy::removeFolder(QString parentId, QString ID, bool Profile)
{
    if(parentId.isEmpty() || ID.isEmpty()){
        return;
    }
    emit status("remove");
    std::string pId = parentId.toStdString();
    std::string Id = ID.toStdString();
    auto it = ProfileCategories.find(pId);
    if (it != ProfileCategories.end()) {
        it->second->removeFolder(Id);

    } else {
        auto it = Folders.find(pId);
        if (it != Folders.end()) {
            it->second->removeFolder(Id);
        }
    }

}

void Hierarchy::removeFolderViaNetwork(QString ID){
    if(ID.isEmpty()){
        return;
    }
    emit status("remove");
    std::string parentId = (Folders)[ID.toStdString()]->parentID;
    if (ProfileCategories.count(parentId)) {
        ProfileCategories[parentId]->removeFolder(ID.toStdString());
    } else {
        (Folders)[parentId]->removeFolder(ID.toStdString());
    }
}

void Hierarchy::renameFolder(QString Id, QString name)
{
    if(Id.isEmpty() || name.isEmpty()){
        return;
    }
    auto it = Folders.find(Id.toStdString());
    if (it != Folders.end()) {
        emit status("rename");
        it->second->Name = name.toStdString();
        emit folderRenamed(Id, name);
    }
}

Entity* Hierarchy::addEntity(QString parentId, QString EntityName, bool Profile)
{
    emit status("add");
    std::string pId = parentId.toStdString();
    std::string EName = EntityName.toStdString();
    if (Profile) {
        auto it = ProfileCategories.find(pId);
        if (it != ProfileCategories.end()) {
            return it->second->addEntity(EName);
        }
    } else {
        auto it = Folders.find(pId);
        if (it != Folders.end()) {
            return it->second->addEntity(EName);
        }
    }
    return nullptr;
}

void Hierarchy::addEntityViaNetwork(QString parentId, QString ID, QString EntityName, bool Profile)
{
    emit status("add");
    std::string pId = parentId.toStdString();
    std::string Id = ID.toStdString();
    std::string EName = EntityName.toStdString();
    if (Profile) {
        auto it = ProfileCategories.find(pId);
        if (it != ProfileCategories.end()) {
            it->second->addEntity(EName,Id);
        }
    } else {
        auto it = Folders.find(pId);
        if (it != Folders.end()) {
            it->second->addEntity(EName,Id);
        }
    }
}

void Hierarchy::addEntityViaLogger(QString parentId, QString ID, QString EntityName, bool Profile)
{
    emit status("add");
    std::string pId = parentId.toStdString();
    std::string Id = ID.toStdString();
    std::string EName = EntityName.toStdString();
    if (Profile) {
        auto it = ProfileCategories.find(pId);
        if (it != ProfileCategories.end()) {
            it->second->addEntity(EName,Id);
        }
    } else {
        auto it = Folders.find(pId);
        if (it != Folders.end()) {
            it->second->addEntity(EName,Id);
        }
    }
}

Entity* Hierarchy::addEntityFromJson(QString parentId, QJsonObject obj, bool Profile)
{
    emit status("add");
    QString EntityName = obj["name"].toString();
    Entity* entity = nullptr;

    std::string pId = parentId.toStdString();
    std::string EName = EntityName.toStdString();
    if (Profile) {
        auto it = ProfileCategories.find(pId);
        if (it != ProfileCategories.end()) {
            entity = it->second->addEntity(EName);
        }
    } else {
        auto it = Folders.find(pId);
        if (it != Folders.end()) {
            entity = it->second->addEntity(EName);
        }
    }

    if(entity){
        std::string id = entity->ID;
        std::string prntId = entity->parentID;
        obj["id"] = QString::fromStdString(id);
        obj["parent_id"] = QString::fromStdString(prntId);
        entity->fromJson(obj);
        // entity->ID = id;
        //entity->parentID = prntId;
        return entity;
    }
    return nullptr;
}

void Hierarchy::removeEntity(QString parentId, QString ID, bool Profile)
{
    emit status("remove");
    std::string pId = parentId.toStdString();
    std::string Id = ID.toStdString();
    auto it = ProfileCategories.find(pId);
    if (it != ProfileCategories.end()) {
        it->second->removeEntity(Id);

    } else {
        auto it = Folders.find(pId);
        if (it != Folders.end()) {
            it->second->removeEntity(Id);
        }
    }
}

void Hierarchy::renameEntity(QString Id, QString name)
{
    auto it = Entities.find(Id.toStdString());
    if (it != Entities.end()) {
        emit status("rename");
        it->second->Name = name.toStdString();
        emit entityRenamed(Id, name);
    }
}

Entity* Hierarchy::getEntityById(QString ID){
    auto it = Entities.find(ID.toStdString());
    if (it != Entities.end()) {
        return it->second;
    }else{
        return nullptr;
    }
}

void Hierarchy::addComponent(QString ID, QString componentName)
{
    auto it = Entities.find(ID.toStdString());
    if (it != Entities.end()) {
        emit status("add");
        it->second->addComponent(componentName.toStdString());
        //emit componentAdded(ID, componentName);
        //getCurrentJsonData(); // Emit updated JSON
        //Console::log("Hierarchy::addComponent emitted getJsonData for " + ID.toStdString() + ", component: " + componentName.toStdString());
    }
}

QJsonObject Hierarchy::getComponentData(QString ID, QString componentName)
{
    auto it = Entities.find(ID.toStdString());
    if (it != Entities.end()) {
        return it->second->getComponent(componentName.toStdString());
    }
    return QJsonObject();
}

void Hierarchy::UpdateComponent(QString ID, QString componentName, QJsonObject delta)
{
    if(componentName.contains("_sub")){
        if(Components.find(ID.toStdString()) != Components.end()){
            Component* component = (Components)[ID.toStdString()];
            QJsonObject currentData = component->getsubComponentData(delta["_id"].toString().toStdString());
            // Merge delta into current data, preserving existing keys
            QJsonObject mergedData = currentData;
            for (auto it = delta.begin(); it != delta.end(); ++it) {
                mergedData[it.key()] = it.value();
            }
            component->updateSubComponent(delta["_id"].toString().toStdString(),mergedData);
            //Console::log("Hierarchy::UpdateComponent merged data for " + componentName.toStdString() + ": " + QString(QJsonDocument(mergedData).toJson()).toStdString());

        }
    }else
        if (Entities.find(ID.toStdString()) != Entities.end()) {
            Entity* entity = (Entities)[ID.toStdString()];
            QJsonObject currentData = entity->getComponent(componentName.toStdString());
            if(componentName.contains("_self")){
                currentData = entity->toJson();
            }

            // Merge delta into current data, preserving existing keys
            QJsonObject mergedData = currentData;
            for (auto it = delta.begin(); it != delta.end(); ++it) {
                mergedData[it.key()] = it.value();
            }
            if(componentName.contains("_self")){
                entity->fromJson(delta);
            }
            QJsonObject obj = delta["ref"].toObject();
            if(!obj.isEmpty()){
                QString ID = obj["id"].toString();
                QString type = obj["subtype"].toString();
                Entity* en = (Entities)[ID.toStdString()];

                if(type == "sensors"){
                    Sensor* sensor = dynamic_cast<Sensor*>(en);
                    Sensor* newsensor = new Sensor(this);
                    newsensor->fromJson(sensor->toJson());
                    entity->addSensor(newsensor);
                }

                if(type == "iffs"){
                    IFF* iff = dynamic_cast<IFF*>(en);
                    IFF* newiff = new IFF(this);
                    newiff->fromJson(iff->toJson());
                    entity->addIFF(newiff);
                }
                if(type == "radios"){
                    Radio* radio = dynamic_cast<Radio*>(en);
                    Radio* newradio = new Radio(this);
                    newradio->fromJson(radio->toJson());
                    entity->addRadio(newradio);
                }
                if(type == "weapons"){
                    Weapon* weapon = dynamic_cast<Weapon*>(en);
                    Weapon* newweapon = new Weapon(this);
                    newweapon->fromJson(weapon->toJson());
                    entity->addWeapon(newweapon);
                }
            }
            entity->updateComponent(componentName, mergedData);
            emit entityUpdate(ID);
            emit entityComponentUpdate(ID,componentName,delta);
            //getCurrentJsonData(); // Emit updated JSON
            //Console::log("Hierarchy::UpdateComponent merged data for " + componentName.toStdString() + ": " + QString(QJsonDocument(mergedData).toJson()).toStdString());
            //Console::log("Hierarchy::UpdateComponent emitted getJsonData for " + ID.toStdString());
        } else {
            //Console::log("Entity not found: " + ID.toStdString());
        }
}

void Hierarchy::removeComponent(QString entityId, QString componentName)
{
    auto it = Entities.find(entityId.toStdString());
    if (it != Entities.end()) {
        emit status("remove");
        it->second->removeComponent(componentName.toStdString());
        emit componentRemoved(entityId, componentName);
        emit entityUpdate(entityId);
    }
}

void Hierarchy::addSubComponent(QString ID, ComponentType type, QString subComponentName, QString data1 , QString data2, QString data3){
    auto it = Components.find(ID.toStdString());
    if (it != Components.end()) {
        emit status("add");
        it->second->addSubComponent(subComponentName.toStdString(),data1,data2);
        //emit componentAdded(ID, componentName);
        //getCurrentJsonData(); // Emit updated JSON
        //Console::log("Hierarchy::addSubComponent emitted getJsonData for " + ID.toStdString() + ", component: " + subComponentName.toStdString());
    }
}

void Hierarchy::renameSubComponent(QString ID, QString subComponentID, QString newName)
{
    auto it = Components.find(ID.toStdString());
    if (it != Components.end()) {
        emit status("rename");
        it->second->renameSubComponent(subComponentID.toStdString(), newName);
        //Console::log("Hierarchy::RemoveSubComponent emitted getJsonData for " + ID.toStdString() + ", component: " + subComponentName.toStdString());
    }
}

void Hierarchy::removeSubComponent(QString ID, QString subComponentID, QString subComponentName){
    auto it = Components.find(ID.toStdString());
    if (it != Components.end()) {
        emit status("remove");
        it->second->removeSubComponent(subComponentID.toStdString());
        //Console::log("Hierarchy::RemoveSubComponent emitted getJsonData for " + ID.toStdString() + ", component: " + subComponentName.toStdString());
    }
}

//------------------IFF------------------------

void Hierarchy::attchedIff(QString ID, QString name)
{
    if (Entities.find(ID.toStdString()) == Entities.end()) {
        return;
    }

    // IFF* iff = new IFF(this);
    // iff->Name = name.toStdString();
    QString iffProfileId;

    bool foundIffProfile = false;
    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr->type == Constants::EntityType::IFF) {
            iffProfileId = QString::fromStdString(key);
            foundIffProfile = true;
            break;
        }
    }
    if (!foundIffProfile) {
        ProfileCategaory* iffProfile = addProfileCategaory("IFF");
        iffProfile->setProfileType(Constants::EntityType::IFF);
        iffProfileId = QString::fromStdString(iffProfile->ID);
        dictionry[iffProfile->ID] = {iffProfile->ID};
    }

    Entity* entity = addEntity(iffProfileId, name, true);
    IFF* iff = dynamic_cast<IFF*>(entity);
    iff->Name = name.toStdString();
    if (iff) {
        (Entities)[ID.toStdString()]->addIFF(iff);
    } else {
        delete iff;
    }
}

void Hierarchy::attachSensors(QString ID, QString name, QString sensorType)
{
    if (Entities.find(ID.toStdString()) == Entities.end()) {
        return;
    }
    QString sensorsProfileId;
    bool foundSensorsProfile = false;
    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr->type == Constants::EntityType::Sensor) {
            sensorsProfileId = QString::fromStdString(key);
            foundSensorsProfile = true;
            break;
        }
    }

    if (!foundSensorsProfile) {
        ProfileCategaory* sensorsProfile = addProfileCategaory("Sensors");
        sensorsProfile->setProfileType(Constants::EntityType::Sensor);
        sensorsProfileId = QString::fromStdString(sensorsProfile->ID);
        dictionry[sensorsProfile->ID] = {sensorsProfile->ID};
    }

    // Create the sensor entity
    Entity* entity = addEntity(sensorsProfileId, name, true);
    Sensor* sensor = dynamic_cast<Sensor*>(entity);

    if (!sensor) {
        qWarning() << "❌ Failed to cast entity to Sensor for:" << name;
        delete entity;
        return;
    }

    sensor->Name = name.toStdString();

    // Map string to SubType safely
    if (sensorType.compare("CSM", Qt::CaseInsensitive) == 0) {
        sensor->subType = Sensor::SubType::CSM;
    }
    else if (sensorType.compare("ESM", Qt::CaseInsensitive) == 0) {
        sensor->subType = Sensor::SubType::ESM;
    }
    else {
        sensor->subType = Sensor::SubType::Generic;
    }

    //qDebug() << "✅ Sensor attached:" << name << "Subtype:" << sensorType;

    // ✅ Attach to entity
    (Entities)[ID.toStdString()]->addSensor(sensor);
}

void Hierarchy::attachRadios(QString ID, QString name)
{
    if (Entities.find(ID.toStdString()) == Entities.end()) {
        return;
    }

    // Radio* radio = new Radio(this);
    // radio->Name = name.toStdString();
    QString radiosProfileId;

    bool foundRadiosProfile = false;
    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr->type == Constants::EntityType::Radio) {
            radiosProfileId = QString::fromStdString(key);
            foundRadiosProfile = true;
            break;
        }
    }

    if (!foundRadiosProfile) {
        ProfileCategaory* radiosProfile = addProfileCategaory("Radios");
        radiosProfile->setProfileType(Constants::EntityType::Radio);
        radiosProfileId = QString::fromStdString(radiosProfile->ID);
        dictionry[radiosProfile->ID] = {radiosProfile->ID};
    }


    Entity* entity = addEntity(radiosProfileId, name, true);
    Radio* radio = dynamic_cast<Radio*>(entity);
    radio->Name = name.toStdString();
    if (radio) {
        // ✅ The only required fix
        radio->parentID = ID.toStdString();
        (Entities)[ID.toStdString()]->addRadio(radio);
    } else {
        delete radio;
    }
}

void Hierarchy::attachWeapons(QString ID, QString name)
{
    if (Entities.find(ID.toStdString()) == Entities.end()) {
        return;
    }

    QString weaponsProfileId;
    bool foundWeaponsProfile = false;

    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr->type == Constants::EntityType::Weapon) {
            weaponsProfileId = QString::fromStdString(key);
            foundWeaponsProfile = true;
            break;
        }
    }

    if (!foundWeaponsProfile) {
        ProfileCategaory* weaponsProfile = addProfileCategaory("Weapons");
        weaponsProfile->setProfileType(Constants::EntityType::Weapon);
        weaponsProfileId = QString::fromStdString(weaponsProfile->ID);
        dictionry[weaponsProfile->ID] = {weaponsProfile->ID};
    }

    Entity* entity = addEntity(weaponsProfileId, name, true);
    Weapon* weapon = dynamic_cast<Weapon*>(entity);
    if (weapon) {
        weapon->Name = name.toStdString();
        weapon->parentID = ID.toStdString();            // ✅ GOOD
        (Entities)[ID.toStdString()]->addWeapon(weapon); // ✅ GOOD
    } else {
        delete entity;
    }
}

QJsonObject Hierarchy::toJson()
{
    QJsonObject obj;
    obj["software_version"] = ScenarioConfig::software_version;
    obj["file_Version"] = ScenarioConfig::software_version;

    // Serialize Profile Categories
    QJsonObject profileCategoriesObj;
    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr) {
            profileCategoriesObj[QString::fromStdString(key)] = profilePtr->toJson();
        }
    }
    for (const QString& key : tempData.keys()) {
        profileCategoriesObj[key] = tempData[key].toObject();
    }
    obj["profileCategories"] = profileCategoriesObj;

    ////Console::log("Hierarchy::toJson output: " + QString(QJsonDocument(obj).toJson()).toStdString());
    return obj;
}

void Hierarchy::onParameterChanged(const QString &entityID, const QString &componentName, const QString &key, const QString &parameterType, bool add)
{
    if (Entities.find(entityID.toStdString()) != Entities.end()) {
        Entity* entity = (Entities)[entityID.toStdString()];
        QJsonObject currentData = entity->getComponent(componentName.toStdString());
        if (add) {
            // Parameter addition is handled by UpdateComponent via valueChanged signal
            //Console::log("Hierarchy::onParameterChanged added parameter " + key.toStdString() + " for " + componentName.toStdString());
        } else {
            // Remove parameter
            if (currentData.contains(key)) {
                QJsonObject mergedData = currentData;
                mergedData.remove(key);
                entity->updateComponent(componentName, mergedData);
                //Console::log("Hierarchy::onParameterChanged removed parameter " + key.toStdString() + " from " + componentName.toStdString());
            } else {
                //Console::log("Parameter not found: " + key.toStdString());
            }
        }
        emit entityUpdate(entityID);
        //getCurrentJsonData(); // Emit updated JSON
        //Console::log("Hierarchy::onParameterChanged emitted getJsonData for " + entityID.toStdString());
    } else {
        //Console::log("Entity not found: " + entityID.toStdString());
    }
}

void Hierarchy::fromJson(const QJsonObject& obj)
{

    std::vector<std::string> keys;
    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr) {
            keys.push_back(key);
        }
    }
    for (const auto& key : keys) {
        removeProfileCategaory(QString::fromStdString(key));
    }
    for (const QString& key : tempData.keys()) {
        tempData.remove(key);
    }
    emit Init();
    emit status("load");
    if (obj.contains("file_Version"))
        ScenarioConfig::file_Version = obj["file_Version"].toString();

    if (obj.contains("profileCategories") && obj["profileCategories"].isObject()) {
        QJsonObject profileCategoriesObj = obj["profileCategories"].toObject();
        for (const QString& key : profileCategoriesObj.keys()) {
            QJsonObject catObj = profileCategoriesObj[key].toObject();
            QString name = catObj["name"].toString();
            if(isDatabase){
                ProfileCategaory* profile = new ProfileCategaory(this);
                profile->Name = catObj["name"].toString().toStdString();
                profile->ID = catObj["id"].toString().toStdString();
                if (profile) {
                    addProfileCategaoryWithObject(profile);
                    profile->fromJson(catObj);
                }
            }else{
                if((name.contains("Platform")||
                     name.contains("Radio")||
                     name.contains("Sensor")||
                     name.contains("SpecialZone")||
                     name.contains("Formation")||
                     name.contains("IFF")||
                     name.contains("Supply")||
                     name.contains("Weapon")||
                     name.contains("FixedPoints")) || !fixedProfiles)
                {
                    ProfileCategaory* profile = new ProfileCategaory(this);
                    profile->Name = catObj["name"].toString().toStdString();
                    profile->ID = catObj["id"].toString().toStdString();
                    if (profile) {
                        addProfileCategaoryWithObject(profile);
                        profile->fromJson(catObj);
                    }
                }else{
                    tempData[key]=catObj;
                    profileCategoriesObj.remove(key);
                }
            }

        }
    }

}

void Hierarchy::getCurrentJsonData()
{
    emit getJsonData(toJson());
}

QJsonArray Hierarchy::searchProfile()
{
    QJsonArray profilesArray;
    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr) {
            QJsonObject profileObj;
            profileObj["id"] = QString::fromStdString(key);
            profileObj["name"] = QString::fromStdString(profilePtr->Name);
            QJsonArray indexKeys;
            auto dictIt = dictionry.find(key);
            if (dictIt != dictionry.end()) {
                for (const auto& indexKey : dictIt->second) {
                    indexKeys.append(QString::fromStdString(indexKey));
                }
            } else {
                //Console::log("Hierarchy::searchProfile: No indexKeys found for profile " + key);
            }
            profileObj["indexKey"] = indexKeys;
            profilesArray.append(profileObj);
        }
    }
    //Console::log("Hierarchy::searchProfile found " + std::to_string(profilesArray.size()) + " profiles");
    return profilesArray;
}

void Hierarchy::Anlaysis(){


    float blueCount = 0.01f;
    float redCount = 0.01f;
    float blueTeamHealth = 0.01f;
    float redTeamHealth = 0.01f;
    float blueloss = 0.01f;
    float redloss = 0.01f;
    float bluehit = 0.01f;
    float redhit = 0.01f;
    float blueEngage = 0.01f;
    float redEngage = 0.01f;
    float bluedetection = 0.01f;
    float reddetection = 0.01f;

    for (const auto& [key, entity] : Platforms) {
        if(!entity)continue;
        if (entity->team == Entity::BlueTeam) {
            if(!entity->Active)
            {
                blueloss += 1;
                continue;
            };
            blueCount++;
            blueTeamHealth += entity->Health;
            blueloss += entity->Health<=0?1:0;
            bluedetection += entity->detectionCount>0?1:0;

        }else
        if (entity->team == Entity::RedTeam) {
            if(!entity->Active)
            {
                redloss += 1;
                continue;
            };
            redCount++;
            redTeamHealth += entity->Health;
            redEngage += entity->engaged?1:0;
            reddetection += entity->detectionCount>0?1:0;
        }
    }

    blueTeamHealth = blueTeamHealth/blueCount;
    float bluelossPercent = (blueCount-blueloss)/blueCount;
    redTeamHealth = redTeamHealth/redCount;
    float redlossPercent = (redCount-redloss)/blueCount;
    bluelossPercent = bluelossPercent==NULL?0:bluelossPercent;

    if(bluelastdamages!=bluelossPercent){
        bluedamages[Simulation::simulationTime] = bluelossPercent;
        bluelastdamages = bluelossPercent;
    }

    if(redlastdamages!=redlossPercent){
        reddamages[Simulation::simulationTime] = redlossPercent;
        redlastdamages = redlossPercent;
    }

    if(bluelastengagments != blueEngage){
        blueengagements[Simulation::simulationTime] = blueEngage;
        bluelastengagments = blueEngage;
    }

    if(bluelastdetections != bluedetection){
        bluedetections[Simulation::simulationTime] = bluedetection;
        bluelastdetections = bluedetection;
    }

    if(redlastengagments != redEngage){
        redengagements[Simulation::simulationTime] = redEngage;
        redlastengagments = redEngage;
    }

    if(redlastdetections != reddetection){
        reddetections[Simulation::simulationTime] = reddetection;
        redlastdetections = reddetection;
    }
}

QJsonObject Hierarchy::loadAnalysisJson()
{
    float blueCount = 0.01f;
    float redCount = 0.01f;
    float blueTeamHealth = 0.01f;
    float redTeamHealth = 0.01f;
    float blueloss = 0.01f;
    float redloss = 0.01f;
    float redweaponcount = 0.01f;
    float blueweaponcount = 0.01f;
    float bluehit = 0.01f;
    float redhit = 0.01f;
    float bluedetection = 0.01f;
    float reddetection = 0.01f;

    for (const auto& [key, entity] : Platforms) {
        if(!entity)continue;
        if (entity->team == Entity::BlueTeam) {
            if(!entity->Active)
            {
                blueloss += 1;
                continue;
            };
            blueCount++;
            blueweaponcount += entity->weaponcount;
            bluehit += entity->hitcount;
            blueTeamHealth += entity->Health;
            bluedetection += entity->detectionCount>0?1:0;

        }else
        if (entity->team == Entity::RedTeam) {
            if(!entity->Active)
            {
                redloss += 1;
                continue;
            };
            redCount++;
            redweaponcount += entity->weaponcount;
            redhit += entity->hitcount;
            redTeamHealth += entity->Health;
            reddetection += entity->detectionCount>0?1:0;
        }
    }
    blueTeamHealth = blueTeamHealth/blueCount;
    float bluelossPercent = blueloss/blueCount;
    redTeamHealth = redTeamHealth/redCount;
    float redlossPercent = redloss/redCount;

    float blueDetectionEff = blueCount>0.5f&& redCount >0.5f?((bluedetection/redCount)*100):0;
    float redDetectionEff = blueCount>0.5f&& redCount >0.5f?((reddetection/blueCount)*100):0;
    blueDetectionEff = blueDetectionEff>100?100:blueDetectionEff;
    redDetectionEff = redDetectionEff>100?100:redDetectionEff;
    float bluewin = blueCount/redCount;
    bluewin = bluewin>1?1:bluewin;
    float redwin = redCount/blueCount;
    redwin = redwin>1?1:redwin;

    QJsonObject obj;
    obj["missionName"] = "test";
    obj["missionDate"] = "2025-03-18";
    QJsonObject teams;
    {
        QJsonObject blueteam;
        blueteam["color"] = "#2277dd";

        // --- Metrics Section ---
        QJsonObject metrics;
        metrics["successProbability"] = bluewin*100.0f;//blueTeamHealth * bluelossPercent * (bluehit/(bluehit+blueweaponcount)); // Note: QJsonValue stores as double
        metrics["friendlyLosses"] = blueloss;
        metrics["enemyLosses"] = redloss;
        metrics["detectionEfficiency"] = blueDetectionEff;
        metrics["weaponEffectiveness"] = (bluehit/(bluehit+blueweaponcount))*100;
        blueteam["metrics"] = metrics;

        // --- Engagement Timeline (Using QJsonArray) ---
        QJsonObject engagementTimeline;


        QJsonObject detectionObj;
        for (auto const& [time, value] : bluedetections) {
            // float ko string mein convert karna padega kyunki JSON key string hoti hai
            QString timeKey = QString::number(time, 'f', 2);
            detectionObj[timeKey] = value;
        }
        engagementTimeline["detection"]  = detectionObj;

        QJsonObject engagementObj;
        for (auto const& [time, value] : blueengagements) {
            // float ko string mein convert karna padega kyunki JSON key string hoti hai
            QString timeKey = QString::number(time, 'f', 2);
            engagementObj[timeKey] = value;
        }
        engagementTimeline["engagement"] = engagementObj;
        QJsonObject damageObj;
        for (auto const& [time, value] : bluedamages) {
            // float ko string mein convert karna padega kyunki JSON key string hoti hai
            QString timeKey = QString::number(time, 'f', 2);
            damageObj[timeKey] = value;
        }
        engagementTimeline["damage"] = damageObj;
        blueteam["engagementTimeline"] = engagementTimeline;

        // --- Losses vs Engagement ---
        QJsonObject lossesVsEngagement;
        lossesVsEngagement["categories"]     = QJsonArray({"Low", "Moderate", "High"});
        lossesVsEngagement["friendlyLosses"] = QJsonArray({1, 2, 3});
        lossesVsEngagement["enemyLosses"]    = QJsonArray({2, 4, 5});
        blueteam["lossesVsEngagement"] = lossesVsEngagement;
        teams["BlueTeam"] = blueteam;
    }

    {
        QJsonObject redteam;
        redteam["color"] = "#dd2222";

        // --- Metrics Section ---
        QJsonObject metrics;
        metrics["successProbability"] = redwin*100.0f;//redTeamHealth * redlossPercent * (redhit/(redhit+redweaponcount));// Note: QJsonValue stores as double
        metrics["friendlyLosses"] = redloss;
        metrics["enemyLosses"] = blueloss;
        metrics["detectionEfficiency"] = redDetectionEff;
        metrics["weaponEffectiveness"] = (redhit/(redhit+redweaponcount))*100;
        redteam["metrics"] = metrics;

        // --- Engagement Timeline (Using QJsonArray) ---
        QJsonObject engagementTimeline;
        ;
        QJsonObject detectionObj;
        for (auto const& [time, value] : reddetections) {
            // float ko string mein convert karna padega kyunki JSON key string hoti hai
            QString timeKey = QString::number(time, 'f', 2);
            detectionObj[timeKey] = value;
        }
        engagementTimeline["detection"]  = detectionObj;

        QJsonObject engagementObj;
        for (auto const& [time, value] : redengagements) {
            // float ko string mein convert karna padega kyunki JSON key string hoti hai
            QString timeKey = QString::number(time, 'f', 2);
            engagementObj[timeKey] = value;
        }
        engagementTimeline["engagement"] = engagementObj;
        QJsonObject damageObj;
        for (auto const& [time, value] : reddamages) {
            // float ko string mein convert karna padega kyunki JSON key string hoti hai
            QString timeKey = QString::number(time, 'f', 2);
            damageObj[timeKey] = value;
        }
        engagementTimeline["damage"] = damageObj;
        redteam["engagementTimeline"] = engagementTimeline;

        // --- Losses vs Engagement ---
        QJsonObject lossesVsEngagement;
        lossesVsEngagement["categories"]     = QJsonArray({"Low", "Moderate", "High"});
        lossesVsEngagement["friendlyLosses"] = QJsonArray({1, 2, 3});
        lossesVsEngagement["enemyLosses"]    = QJsonArray({2, 4, 5});
        redteam["lossesVsEngagement"] = lossesVsEngagement;
        teams["Redteam"] = redteam;
    }
    // --- Final Assembly ---
    // Ensure 'teams' is declared

    obj["teams"] = teams;
    qDebug() << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    Console::log(QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString());
    return obj;

}
