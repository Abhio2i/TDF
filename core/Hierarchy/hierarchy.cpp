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

/**
 * @brief Constructor – sets the current context to this instance.
 */
Hierarchy::Hierarchy()
{
    setCurrentContext(this);
}

/**
 * @brief Destructor – clears internal containers.
 */
Hierarchy::~Hierarchy()
{
    Entities.clear();
    Folders.clear();
    ProfileCategories.clear();
}

/**
 * @brief Creates and adds a new profile category.
 * @param profileName Display name of the profile.
 * @return Pointer to the created ProfileCategaory, or nullptr if name empty.
 */
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

/**
 * @brief Adds an existing profile category object to the registry.
 * @param profile Pointer to the profile (ownership transferred).
 */
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

/**
 * @brief Removes a profile category by its ID.
 * @param ID Unique identifier of the profile.
 */
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

/**
 * @brief Renames an existing profile category.
 * @param Id Profile ID.
 * @param name New display name.
 */
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

/**
 * @brief Retrieves a profile category by its unique ID.
 * @param ID Profile ID.
 * @return Pointer to the profile, or nullptr if not found.
 */
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

/**
 * @brief Retrieves a profile category by its display name (partial match).
 * @param name Display name (substring search).
 * @return Pointer to the first matching profile, or nullptr if none.
 */
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

/**
 * @brief Creates a new folder under a given parent (profile or folder).
 * @param parentId Parent ID (profile or folder).
 * @param FolderName Display name of the new folder.
 * @param Profile If true, parent is a profile; else a folder.
 * @return Pointer to the new Folder, or nullptr on failure.
 */
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

/**
 * @brief Creates a folder from network data (preset ID).
 * @param parentId Parent ID.
 * @param ID Pre‑assigned folder ID.
 * @param FolderName Folder name.
 * @param Profile Parent is profile flag.
 */
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

/**
 * @brief Removes a folder from its parent.
 * @param parentId Parent ID.
 * @param ID Folder ID to remove.
 */
void Hierarchy::removeFolder(QString parentId, QString ID)
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

/**
 * @brief Removes a folder by its ID (network command, parent deduced from map).
 * @param ID Folder ID.
 */
void Hierarchy::removeFolderViaNetwork(QString ID){
    if(ID.isEmpty() || !Folders.count(ID.toStdString())){
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

/**
 * @brief Renames an existing folder.
 * @param Id Folder ID.
 * @param name New name.
 */
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

/**
 * @brief Creates a new entity under a given parent.
 * @param parentId Parent folder/profile ID.
 * @param EntityName Display name.
 * @param Profile Parent is profile flag.
 * @param data1 Optional extra data (e.g., initial configuration).
 * @return Pointer to the new Entity, or nullptr on failure.
 */
Entity* Hierarchy::addEntity(QString parentId, QString EntityName, bool Profile, QString data1)
{
    if(parentId.isEmpty() || EntityName.isEmpty()){
        return nullptr;
    }
    emit status("add");
    std::string pId = parentId.toStdString();
    std::string EName = EntityName.toStdString();
    if (Profile) {
        auto it = ProfileCategories.find(pId);
        if (it != ProfileCategories.end()) {
            return it->second->addEntity(EName,"",data1);
        }
    } else {
        auto it = Folders.find(pId);
        if (it != Folders.end()) {
            return it->second->addEntity(EName,"",data1);
        }
    }
    return nullptr;
}

/**
 * @brief Creates an entity from network data (preset ID).
 * @param parentId Parent ID.
 * @param ID Pre‑assigned entity ID.
 * @param EntityName Entity name.
 * @param Profile Parent is profile flag.
 */
void Hierarchy::addEntityViaNetwork(QString parentId, QString ID, QString EntityName, bool Profile)
{
    if(parentId.isEmpty() || ID.isEmpty() || EntityName.isEmpty()){
        return;
    }
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

/**
 * @brief Creates an entity from logger data (replay mode).
 * @param parentId Parent ID.
 * @param ID Entity ID.
 * @param EntityName Entity name.
 * @param Profile Parent is profile flag.
 */
void Hierarchy::addEntityViaLogger(QString parentId, QString ID, QString EntityName, bool Profile)
{
    if(parentId.isEmpty() || ID.isEmpty() || EntityName.isEmpty()){
        return;
    }
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

/**
 * @brief Creates an entity from a JSON object (deserialization).
 * @param parentId Parent ID.
 * @param obj JSON object defining the entity.
 * @param Profile Parent is profile flag.
 * @return Pointer to the created Entity, or nullptr on failure.
 */
Entity* Hierarchy::addEntityFromJson(QString parentId, QJsonObject obj, bool Profile)
{
    if(parentId.isEmpty() || obj.isEmpty()){
        return nullptr;
    }
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
        return entity;
    }
    return nullptr;
}

/**
 * @brief Removes an entity from its parent.
 * @param parentId Parent ID.
 * @param ID Entity ID.
 */
void Hierarchy::removeEntity(QString parentId, QString ID)
{
    if(parentId.isEmpty() || ID.isEmpty()){
        return;
    }
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

/**
 * @brief Renames an existing entity.
 * @param Id Entity ID.
 * @param name New name.
 */
void Hierarchy::renameEntity(QString Id, QString name)
{
    if(Id.isEmpty() || name.isEmpty()){
        return;
    }
    auto it = Entities.find(Id.toStdString());
    if (it != Entities.end()) {
        emit status("rename");
        it->second->Name = name.toStdString();
        emit entityRenamed(Id, name);
    }
}

/**
 * @brief Retrieves an entity by its unique ID.
 * @param ID Entity ID.
 * @return Pointer to the Entity, or nullptr if not found.
 */
Entity* Hierarchy::getEntityById(QString ID){
    if(ID.isEmpty()){
        return nullptr;
    }
    auto it = Entities.find(ID.toStdString());
    if (it != Entities.end()) {
        return it->second;
    }else{
        return nullptr;
    }
}

/**
 * @brief Adds a component to an entity.
 * @param ID Entity ID.
 * @param componentName Name of the component.
 */
void Hierarchy::addComponent(QString ID, QString componentName)
{
    if(ID.isEmpty() || componentName.isEmpty()){
        return;
    }
    auto it = Entities.find(ID.toStdString());
    if (it != Entities.end()) {
        emit status("add");
        it->second->addComponent(componentName.toStdString());
    }
}

/**
 * @brief Retrieves component data as JSON.
 * @param ID Entity ID.
 * @param componentName Component name.
 * @return QJsonObject containing the component data.
 */
QJsonObject Hierarchy::getComponentData(QString ID, QString componentName)
{
    if(ID.isEmpty() || componentName.isEmpty()){
        return QJsonObject();;
    }
    auto it = Entities.find(ID.toStdString());
    if (it != Entities.end()) {
        return it->second->getComponent(componentName.toStdString());
    }
    return QJsonObject();
}

/**
 * @brief Updates a component (or sub‑component) with a delta JSON.
 * @param ID Entity or component ID.
 * @param componentName Component name (may contain "_sub" or "_self").
 * @param delta JSON object of changes.
 */
void Hierarchy::UpdateComponent(QString ID, QString componentName, QJsonObject delta)
{
    if(ID.isEmpty() || componentName.isEmpty() || delta.isEmpty()){
        return;
    }
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
        }
}

/**
 * @brief Removes a component from an entity.
 * @param entityId Entity ID.
 * @param componentName Component name to remove.
 */

void Hierarchy::removeComponent(QString entityId, QString componentName)
{
    if(entityId.isEmpty() || componentName.isEmpty()){
        return ;
    }
    auto it = Entities.find(entityId.toStdString());
    if (it != Entities.end()) {
        emit status("remove");
        it->second->removeComponent(componentName.toStdString());
        emit componentRemoved(entityId, componentName);
        emit entityUpdate(entityId);
    }
}

/**
 * @brief Adds a sub‑component to a component.
 * @param ID Component ID.
 * @param type Sub‑component type (unused in implementation).
 * @param subComponentName Name of the sub‑component.
 * @param data1 Optional string data.
 * @param data2 Optional second string data.
 * @param data3 Optional JSON data.
 */
void Hierarchy::addSubComponent(QString ID,QString subComponentName, QString data1 , QString data2, QJsonObject data3){
    if(ID.isEmpty() || subComponentName.isEmpty()){
        return ;
    }

    auto it = Components.find(ID.toStdString());
    if (it != Components.end()) {
        emit status("add");
        it->second->addSubComponent(subComponentName.toStdString(),data1,data2,data3);
    }
}

/**
 * @brief Renames a sub‑component.
 * @param ID Parent component ID.
 * @param subComponentID Sub‑component ID.
 * @param newName New name.
 */
void Hierarchy::renameSubComponent(QString ID, QString subComponentID, QString newName)
{
    if(ID.isEmpty() || subComponentID.isEmpty() || newName.isEmpty()){
        return ;
    }
    auto it = Components.find(ID.toStdString());
    if (it != Components.end()) {
        emit status("rename");
        it->second->renameSubComponent(subComponentID.toStdString(), newName);
    }
}

/**
 * @brief Removes a sub‑component.
 * @param ID Parent component ID.
 * @param subComponentID Sub‑component ID.
 */
void Hierarchy::removeSubComponent(QString ID, QString subComponentID){
    if(ID.isEmpty() || subComponentID.isEmpty()){
        return ;
    }
    auto it = Components.find(ID.toStdString());
    if (it != Components.end()) {
        emit status("remove");
        it->second->removeSubComponent(subComponentID.toStdString());
    }
}

//------------------IFF------------------------

/**
 * @brief Attaches an IFF component to an entity.
 * @param ID Entity ID.
 * @param name IFF component name.
 */
void Hierarchy::attchedIff(QString ID, QString name)
{
    if (Entities.find(ID.toStdString()) == Entities.end()) {
        return;
    }
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

/**
 * @brief Attaches a sensor component to an entity.
 * @param ID Entity ID.
 * @param name Sensor name.
 * @param sensorType Type of sensor ("CSM", "ESM", or generic).
 */
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

    Entity* entity = addEntity(sensorsProfileId, name, true);
    Sensor* sensor = dynamic_cast<Sensor*>(entity);

    if (!sensor) {
        qWarning() << "❌ Failed to cast entity to Sensor for:" << name;
        delete entity;
        return;
    }
    sensor->Name = name.toStdString();
    if (sensorType.compare("CSM", Qt::CaseInsensitive) == 0) {
        sensor->subType = Sensor::SubType::CSM;
    }
    else if (sensorType.compare("ESM", Qt::CaseInsensitive) == 0) {
        sensor->subType = Sensor::SubType::ESM;
    }
    else {
        sensor->subType = Sensor::SubType::Generic;
    }
    (Entities)[ID.toStdString()]->addSensor(sensor);
}

/**
 * @brief Attaches a radio component to an entity.
 * @param ID Entity ID.
 * @param name Radio name.
 */
void Hierarchy::attachRadios(QString ID, QString name)
{
    if (Entities.find(ID.toStdString()) == Entities.end()) {
        return;
    }

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
        radio->parentID = ID.toStdString();
        (Entities)[ID.toStdString()]->addRadio(radio);
    } else {
        delete radio;
    }
}

/**
 * @brief Attaches a weapon component to an entity.
 * @param ID Entity ID.
 * @param name Weapon name.
 */
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
        weapon->parentID = ID.toStdString();
        (Entities)[ID.toStdString()]->addWeapon(weapon);
    } else {
        delete entity;
    }
}

/**
 * @brief Serializes the entire hierarchy to a JSON object.
 * @return QJsonObject representing the current state.
 */
QJsonObject Hierarchy::toJson()
{
    QJsonObject obj;
    obj["software_version"] = ScenarioConfig::software_version;
    obj["file_Version"] = ScenarioConfig::software_version;

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
    return obj;
}

/**
 * @brief Handles parameter changes for a component (add/remove).
 * @param entityID Entity ID.
 * @param componentName Component name.
 * @param key Parameter key.
 * @param parameterType Type of parameter (unused).
 * @param add True to add, false to remove.
 */
void Hierarchy::onParameterChanged(const QString &entityID, const QString &componentName, const QString &key, const QString &parameterType, bool add)
{
    if (Entities.find(entityID.toStdString()) != Entities.end()) {
        Entity* entity = (Entities)[entityID.toStdString()];
        QJsonObject currentData = entity->getComponent(componentName.toStdString());
        if (add) {
            // Parameter addition is handled by UpdateComponent via valueChanged signal
        } else {
            if (currentData.contains(key)) {
                QJsonObject mergedData = currentData;
                mergedData.remove(key);
                entity->updateComponent(componentName, mergedData);
            }
        }
        emit entityUpdate(entityID);
    }
}

/**
 * @brief Deserializes the hierarchy from a JSON object.
 * @param obj JSON object to load.
 */
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
    clear();
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

/**
 * @brief Emits the current JSON data via the getJsonData signal.
 */
void Hierarchy::getCurrentJsonData()
{
    emit getJsonData(toJson());
}

/**
 * @brief Searches all profiles and returns a JSON array of results.
 * @return QJsonArray containing profile IDs, names, and index keys.
 */
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
            }
            profileObj["indexKey"] = indexKeys;
            profilesArray.append(profileObj);
        }
    }
    return profilesArray;
}

/**
 * @brief Performs analytics on engagements, detections, and damages.
 * Updates the internal engagement/damage maps over simulation time.
 */
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

/**
 * @brief Loads analysis data and returns a JSON object with mission metrics.
 * @return QJsonObject containing analysis results.
 */
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

        QJsonObject metrics;
        metrics["successProbability"] = bluewin*100.0f;
        metrics["friendlyLosses"] = blueloss;
        metrics["enemyLosses"] = redloss;
        metrics["detectionEfficiency"] = blueDetectionEff;
        metrics["weaponEffectiveness"] = (bluehit/(bluehit+blueweaponcount))*100;
        blueteam["metrics"] = metrics;

        QJsonObject engagementTimeline;

        QJsonObject detectionObj;
        for (auto const& [time, value] : bluedetections) {
            QString timeKey = QString::number(time, 'f', 2);
            detectionObj[timeKey] = value;
        }
        engagementTimeline["detection"]  = detectionObj;

        QJsonObject engagementObj;
        for (auto const& [time, value] : blueengagements) {
            QString timeKey = QString::number(time, 'f', 2);
            engagementObj[timeKey] = value;
        }
        engagementTimeline["engagement"] = engagementObj;
        QJsonObject damageObj;
        for (auto const& [time, value] : bluedamages) {
            QString timeKey = QString::number(time, 'f', 2);
            damageObj[timeKey] = value;
        }
        engagementTimeline["damage"] = damageObj;
        blueteam["engagementTimeline"] = engagementTimeline;

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

        QJsonObject metrics;
        metrics["successProbability"] = redwin*100.0f;
        metrics["friendlyLosses"] = redloss;
        metrics["enemyLosses"] = blueloss;
        metrics["detectionEfficiency"] = redDetectionEff;
        metrics["weaponEffectiveness"] = (redhit/(redhit+redweaponcount))*100;
        redteam["metrics"] = metrics;

        QJsonObject engagementTimeline;
        QJsonObject detectionObj;
        for (auto const& [time, value] : reddetections) {
            QString timeKey = QString::number(time, 'f', 2);
            detectionObj[timeKey] = value;
        }
        engagementTimeline["detection"]  = detectionObj;

        QJsonObject engagementObj;
        for (auto const& [time, value] : redengagements) {
            QString timeKey = QString::number(time, 'f', 2);
            engagementObj[timeKey] = value;
        }
        engagementTimeline["engagement"] = engagementObj;
        QJsonObject damageObj;
        for (auto const& [time, value] : reddamages) {
            QString timeKey = QString::number(time, 'f', 2);
            damageObj[timeKey] = value;
        }
        engagementTimeline["damage"] = damageObj;
        redteam["engagementTimeline"] = engagementTimeline;

        QJsonObject lossesVsEngagement;
        lossesVsEngagement["categories"]     = QJsonArray({"Low", "Moderate", "High"});
        lossesVsEngagement["friendlyLosses"] = QJsonArray({1, 2, 3});
        lossesVsEngagement["enemyLosses"]    = QJsonArray({2, 4, 5});
        redteam["lossesVsEngagement"] = lossesVsEngagement;
        teams["Redteam"] = redteam;
    }

    obj["teams"] = teams;
    qDebug() << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    Console::log(QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString());
    return obj;
}

/**
 * @brief Clears all hierarchy data (profiles, folders, entities, maps).
 */
void Hierarchy::clear() {
    std::vector<std::string> keysToRemove;
    for (const auto& [key, profilePtr] : ProfileCategories) {
        keysToRemove.push_back(key);
    }

    for (const auto& key : keysToRemove) {
        removeProfileCategaory(QString::fromStdString(key));
    }

    Folders.clear();
    Entities.clear();
    Platforms.clear();
    Radios.clear();
    Sensors.clear();
    FixedPointes.clear();
    Formations.clear();
    Specialzones.clear();
    Iffs.clear();
    Components.clear();
    Meshes.clear();
    missionList.clear();

    tempData = QJsonObject();
    dictionry.clear();
    EntityPaths.clear();
    FolderPaths.clear();
}
QStringList Hierarchy::getAvailableSensorTypes() const
{
    return *sensorlist ;//QStringList({"Generic", "CSM", "ESM", "EO", "IR", "Sonar", "AIS", "ADSB", "AESA"});
}
bool Hierarchy::registerNewSensor(std::string name){
    if(sensorlist->contains(QString::fromStdString(name))){
        return false;
    }else{
        sensorlist->append(QString::fromStdString(name));
        return true;
    }
}

bool Hierarchy::unregisterNewSensor(std::string name){
    if(sensorlist->contains(QString::fromStdString(name))){
        sensorlist->removeAt(sensorlist->indexOf(QString::fromStdString(name)));
        return true;
    }else{
        return false;
    }
}
