#include "folder.h"
#include <core/Debug/console.h>
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/adsbsensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aissensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/csm.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/eosensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/esm.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Hierarchy/EntityProfiles/fixedpoints.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/formation.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/specialzone.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qjsonarray.h"
#include <core/Hierarchy/hierarchy.h> // Required for type
#include <core/Utility/uuid.h>

/**
 * @brief Constructs a Folder and registers it with the global hierarchy.
 * @param h Pointer to the parent Hierarchy.
 */
Folder::Folder(Hierarchy* h) {
    ID = Uuid::generateShortUniqueID();
    Hierarchy* hierarchy = h;
    if (hierarchy) {
        GlobalRegistry::registerFolder(this, hierarchy);
        hierarchy->dictionry[ID] = {};
    }
}

/**
 * @brief Destructor – removes all child folders and entities.
 */
Folder::~Folder(){
    // Clean up dynamically allocated folder & entity categories
    while (!Folders.empty()) {
        removeFolder(Folders.begin()->second->ID);
    }

    while (!Entities.empty()) {
        removeEntity(Entities.begin()->second->ID);
    }
    Console::log("Delete"+Name);
}

/**
 * @brief Sets the profile type for this folder (affects entity creation).
 * @param Type The entity type to associate.
 */
void Folder::setProfileType(Constants::EntityType Type){
    type = Type;
}

/**
 * @brief Creates and adds a new sub‑folder.
 * @param folderName Display name of the new folder.
 * @param iD Optional unique identifier; auto‑generated if empty.
 * @return Pointer to the newly created Folder, or nullptr on failure.
 */
Folder* Folder::addFolder(std::string folderName, std::string iD) {
    if(folderName.empty()){
        return nullptr;
    }
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parent ) {
        Console::error("RunTimeError::" + std::string(__FILE__) + ":" + std::to_string(__LINE__) +
                       " - Hierarchy parent or parent->Folders is null!");
        return nullptr;
    }

    emit parent->status("add");
    Folder* folder = new Folder(parent);
    folder->Name = folderName;
    folder->parentID = this->ID;
    folder->setProfileType(this->type);

    if (!iD.empty()) {
        folder->ID = iD;
    }

    this->Folders.insert({folder->ID, folder});
    parent->Folders.insert({folder->ID, folder});

    // Ensure parentID entry exists in dictionary
    parent->dictionry[folder->parentID].push_back(folder->ID);

    QString qParentID = QString::fromStdString(folder->parentID);
    QString qFolderID = QString::fromStdString(folder->ID);
    QString qFolderName = QString::fromStdString(folderName);

    emit parent->folderAddedPointer(qParentID, folder);
    emit parent->folderAdded(qParentID, qFolderID, qFolderName);

    return folder;
}

/**
 * @brief Adds an existing Folder object as a child.
 * @param folder Pointer to the Folder to add (ownership transferred).
 */
void Folder::addFolderWithObject(Folder *folder){
    if(folder == nullptr){
        return;
    }
    folder->parentID = ID;
    Folders.insert({folder->ID, folder});

    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->status("add");
    if (parent ) {
        parent->Folders.insert({folder->ID, folder});
        emit parent->folderAddedPointer(QString::fromStdString(folder->parentID),folder);
        emit parent->folderAdded(QString::fromStdString(folder->parentID), QString::fromStdString(folder->ID),QString::fromStdString(folder->Name));
        parent->dictionry[folder->parentID].push_back(folder->ID);

    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Folders is null!");
    }
}

/**
 * @brief Removes a sub‑folder by its ID.
 * @param folderID ID of the folder to remove.
 */
void Folder::removeFolder(std::string folderID){
    if(folderID.empty()){
        return;
    }
    delete Folders[folderID];
    Folders[folderID] = nullptr;
    Folders.erase(folderID);
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->status("remove");
    if (parent ) {
        parent->Folders.erase(folderID);
        emit parent->folderRemoved(QString::fromStdString(folderID));
    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Folders is null!");
    }
}

/**
 * @brief Creates and adds a new entity to this folder.
 * @param entityName Display name of the entity.
 * @param iD Optional unique identifier; auto‑generated if empty.
 * @param data1 Optional extra data (e.g., sensor subtype).
 * @return Pointer to the newly created Entity.
 */
Entity* Folder::addEntity(std::string entityName, std::string iD,QString data1){
    if(entityName.empty()){
        return nullptr;
    }
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->status("add");
    Entity *entity;
    if(type == Constants::EntityType::Radio){
        entity = new Radio(parent);
    }else
        if(type == Constants::EntityType::Sensor){
            if(data1 == "Generic"){
                entity = new Radar(parent);
            }else
                if(data1 == "CSM"){
                    entity = new CSM(parent);
                }else
                    if(data1 == "ESM"){
                        entity = new ESM(parent);
                    }else
                        if(data1 == "EO"){
                            entity = new EOSensor(parent);
                        }else
                            if(data1 == "Sonar"){
                                entity = new Sonar(parent);
                            }else
                                if(data1 == "AIS"){
                                    entity = new AISSensor(parent);
                                }else
                                    if(data1 == "ADSB"){
                                        entity = new ADSBSensor(parent);
                                    }else
                                        if(data1 == "AESA"){
                                            entity = new AESARadar(parent);
                                        }else{
                                            entity = new Sensor(parent);
                                        }
        }else
            if(type == Constants::EntityType::FixedPoint){
                entity = new FixedPoints(parent);
            }else
                if(type == Constants::EntityType::Formation){
                    entity = new Formation(parent);
                }else
                    if(type == Constants::EntityType::SpecialZone){
                        entity = new Specialzone(parent);
                    }else
                        if(type == Constants::EntityType::IFF){
                            entity = new IFF(parent);
                        }else{
                            entity = new Platform(parent);
                        }
    entity->Name = entityName;
    entity->parentID = ID;
    entity->type = type;
    if(!iD.empty()){
        entity->ID = iD;
    }
    Entities.insert({entity->ID, entity});

    if (parent) {
        parent->Entities.insert({entity->ID, entity});
        if(type == Constants::EntityType::Radio){
            parent->Radios.insert({entity->ID, dynamic_cast<Radio*>(entity)});
        }else
            if(type == Constants::EntityType::Sensor){
                parent->Sensors.insert({entity->ID, dynamic_cast<Sensor*>(entity)});
            }else
                if(type == Constants::EntityType::FixedPoint){
                    parent->FixedPointes.insert({entity->ID, dynamic_cast<FixedPoints*>(entity)});
                }else
                    if(type == Constants::EntityType::Formation){
                        parent->Formations.insert({entity->ID, dynamic_cast<Formation*>(entity)});
                    }else
                        if(type == Constants::EntityType::SpecialZone){
                            parent->Specialzones.insert({entity->ID, dynamic_cast<Specialzone*>(entity)});
                        }else
                            if(type == Constants::EntityType::IFF){
                                parent->Iffs.insert({entity->ID, dynamic_cast<IFF*>(entity)});
                            }else{
                                parent->Platforms.insert({entity->ID, dynamic_cast<Platform*>(entity)});
                            }
        entity->spawn();
        parent->dictionry[entity->parentID].push_back(entity->ID);
    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Entities is null!");
    }

    return entity;
}

/**
 * @brief Adds an existing Entity object to this folder.
 * @param entity Pointer to the Entity to add (ownership transferred).
 */
void Folder::addEntityWithObject(Entity *entity){
    if(entity == nullptr){
        return;
    }
    entity->parentID = ID;
    Entities.insert({entity->ID, entity});

    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->status("add");
    if (parent ) {
        parent->Entities.insert({entity->ID, entity});
        if(type == Constants::EntityType::Radio){
            parent->Radios.insert({entity->ID, dynamic_cast<Radio*>(entity)});
        }else
            if(type == Constants::EntityType::Sensor){
                parent->Sensors.insert({entity->ID, dynamic_cast<Sensor*>(entity)});
            }else
                if(type == Constants::EntityType::FixedPoint){
                    parent->FixedPointes.insert({entity->ID, dynamic_cast<FixedPoints*>(entity)});
                }else
                    if(type == Constants::EntityType::Formation){
                        parent->Formations.insert({entity->ID, dynamic_cast<Formation*>(entity)});
                    }else
                        if(type == Constants::EntityType::SpecialZone){
                            parent->Specialzones.insert({entity->ID, dynamic_cast<Specialzone*>(entity)});
                        }else
                            if(type == Constants::EntityType::IFF){
                                parent->Iffs.insert({entity->ID, dynamic_cast<IFF*>(entity)});
                            }else{
                                parent->Platforms.insert({entity->ID, dynamic_cast<Platform*>(entity)});
                            }
        entity->spawn();

        parent->dictionry[entity->parentID].push_back(entity->ID);
    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Entities is null!");
    }
}

/**
 * @brief Removes an entity from this folder by its ID.
 * @param EntityID ID of the entity to remove.
 */
void Folder::removeEntity(std::string EntityID){
    if(EntityID.empty()){
        return;
    }
    delete Entities[EntityID];
    Entities.erase(EntityID);
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->status("remove");
    if (parent ) {
        parent->Entities.erase(EntityID);
        if(type == Constants::EntityType::Radio){
            parent->Radios.erase(EntityID);
        }else
            if(type == Constants::EntityType::Sensor){
                parent->Sensors.erase(EntityID);
            }else
                if(type == Constants::EntityType::FixedPoint){
                    parent->FixedPointes.erase(EntityID);
                }else
                    if(type == Constants::EntityType::Formation){
                        parent->Formations.erase(EntityID);
                    }else
                        if(type == Constants::EntityType::SpecialZone){
                            parent->Specialzones.erase(EntityID);
                        }else
                            if(type == Constants::EntityType::IFF){
                                parent->Iffs.erase(EntityID);
                            }else{
                                parent->Platforms.erase(EntityID);
                            }
        emit parent->entityRemoved(QString::fromStdString(EntityID));
        emit parent->entityRemovedfull(QString::fromStdString(ID),QString::fromStdString(EntityID),false);
    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Folders is null!");
    }
}

/**
 * @brief Serializes the folder (including children) to a JSON object.
 * @return QJsonObject representing the folder's state.
 */
QJsonObject Folder::toJson() {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Folder");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;

    QJsonObject folderObj;
    for (const auto& [key, folderPtr] : Folders) {
        if (folderPtr) {
            folderObj[QString::fromStdString(key)] = folderPtr->toJson();
        }
    }
    obj["folders"] = folderObj;

    QJsonObject entityObj;
    for (const auto& [key, entityPtr] : Entities) {
        if (entityPtr) {
            entityObj[QString::fromStdString(key)] = entityPtr->toJson();
        }
    }
    obj["entities"] = entityObj;

    QJsonObject typeObj;
    typeObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : entityTypeOptions())
        optionsArray.append(opt);

    typeObj["options"] = optionsArray;
    typeObj["value"] = entityTypeToString(type);
    obj["type"] = typeObj;
    return obj;
}

/**
 * @brief Deserializes the folder from a JSON object.
 * @param obj JSON object containing folder data.
 */
void Folder::fromJson(const QJsonObject& obj)
{
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();
    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();

    if (obj.contains("folders") && obj["folders"].isObject()) {
        QJsonObject foldersObj = obj["folders"].toObject();
        for (const QString& key : foldersObj.keys()) {
            QJsonObject childFolderObj = foldersObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            Folder *folder = new Folder(parent);
            folder->setProfileType(type);
            folder->Name = childFolderObj["name"].toString().toStdString();
            folder->ID = childFolderObj["id"].toString().toStdString();
            if (folder) {
                addFolderWithObject(folder);
                folder->fromJson(childFolderObj);
            }
        }
    }
    if (obj.contains("type") && obj["type"].isObject()) {
        QJsonObject entityObj = obj["type"].toObject();
        if (entityObj.contains("value"))
            type = stringToEntityType(entityObj["value"].toString());
    }

    if (obj.contains("entities") && obj["entities"].isObject()) {
        QJsonObject entitiesObj = obj["entities"].toObject();
        for (const QString& key : entitiesObj.keys()) {
            QJsonObject entityObj = entitiesObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            Entity *entity;
            if(type == Constants::EntityType::Radio){
                entity = new Radio(parent);
            }else
                if(type == Constants::EntityType::Sensor){
                    QString data1 = entityObj["SensorType"].toString();
                    if(data1 == "Generic"){
                        entity = new Radar(parent);
                    }else
                        if(data1 == "CSM"){
                            entity = new CSM(parent);
                        }else
                            if(data1 == "ESM"){
                                entity = new ESM(parent);
                            }else
                                if(data1 == "EO"){
                                    entity = new EOSensor(parent);
                                }else
                                    if(data1 == "Sonar"){
                                        entity = new Sonar(parent);
                                    }else
                                        if(data1 == "AIS"){
                                            entity = new AISSensor(parent);
                                        }else
                                            if(data1 == "ADSB"){
                                                entity = new ADSBSensor(parent);
                                            }else
                                                if(data1 == "AESA"){
                                                    entity = new AESARadar(parent);
                                                }else{
                                                    entity = new Sensor(parent);
                                                }
                }else
                    if(type == Constants::EntityType::FixedPoint){
                        entity = new FixedPoints(parent);
                    }else
                        if(type == Constants::EntityType::Formation){
                            entity = new Formation(parent);
                        }else
                            if(type == Constants::EntityType::SpecialZone){
                                entity = new Specialzone(parent);
                            }else
                                if(type == Constants::EntityType::IFF){
                                    entity = new IFF(parent);
                                }else{
                                    entity = new Platform(parent);
                                }
            entity->type = type;
            entity->Name = entityObj["name"].toString().toStdString();
            entity->ID = entityObj["id"].toString().toStdString();
            if (entity) {
                addEntityWithObject(entity);
                entity->fromJson(entityObj);
            }
        }
    }
}
