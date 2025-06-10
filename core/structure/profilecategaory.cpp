#include "profilecategaory.h"
#include <core/Debug/console.h>
#include "core/GlobalRegistry.h"
#include <core/structure/hierarchy.h> // Required for type
#include <core/Utility/uuid.h>

ProfileCategaory::ProfileCategaory(Hierarchy* h) {
    ID = Uuid::generateShortUniqueID();
    // Register this ProfileCategaory automatically
    Hierarchy* hierarchy = h;//Hierarchy::getCurrentContext(); // 💡 We'll create this next
    if (hierarchy) {
        GlobalRegistry::registerProfile(this, hierarchy);
        hierarchy->dictionry[ID] ={};
    }
}

ProfileCategaory::~ProfileCategaory(){

    // Clean up dynamically allocated folder & entity categories
    while (!Folders.empty()) {
        removeFolder(Folders.begin()->second->ID);
    }

    while (!Entities.empty()) {
        removeEntity(Entities.begin()->second->ID);
    }
    Console::log("Delete"+Name);

}


Folder* ProfileCategaory::addFolder(std::string folderName){

    // if(Folders.count(folderName)){

    //     Console::error(
    //         "RunTimeError::" + std::string(__FILE__) + "," +
    //         std::to_string(__LINE__) + " Folder already exists With Same Name"
    //         );
    //     return nullptr;
    // }
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    Folder *folder = new Folder(parent);
    folder->Name = folderName;
    folder->parentID = ID;
    Folders.insert({folder->ID, folder});

    // Automatically update hierarchy's Folders
    if (parent && parent->Folders) {
        parent->Folders->insert({folder->ID, folder});
        emit parent->folderAddedPointer(QString::fromStdString(folder->parentID),folder);
        emit parent->folderAdded(QString::fromStdString(folder->parentID), QString::fromStdString(folder->ID),QString::fromStdString(folderName));
        parent->dictionry[folder->parentID].push_back(folder->ID);

    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Folders is null!");
    }

    return folder;
}

void ProfileCategaory::addFolderWithObject(Folder *folder){
    folder->parentID = ID;
    Folders.insert({folder->ID, folder});

    // Automatically update hierarchy's Folders
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (parent && parent->Folders) {
        parent->Folders->insert({folder->ID, folder});
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


void ProfileCategaory::removeFolder(std::string folderID){

    delete Folders[folderID];
    Folders.erase(folderID);
    // Automatically update hierarchy's Folders
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (parent && parent->Folders) {
        parent->Folders->erase(folderID);
        emit parent->folderRemoved(QString::fromStdString(folderID));
    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Folders is null!");
    }
}

Entity* ProfileCategaory::addEntity(std::string entityName){
    // if(Entities.count(entityName)){

    //     Console::error(
    //         "RunTimeError::" + std::string(__FILE__) + "," +
    //         std::to_string(__LINE__) + " Folder already exists With Same Name"
    //         );
    //     return nullptr;
    // }
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    Entity *entity = new Entity(parent);
    entity->Name = entityName;
    entity->parentID = ID;
    Entities.insert({entity->ID, entity});

    // Automatically update hierarchy's Folders
    if (parent && parent->Entities) {
        parent->Entities->insert({entity->ID, entity});
        entity->spawn();
        // entity->addComponent("transform");
        // entity->addComponent("trajectory");
        // entity->addComponent("rigidbody");
        // entity->addComponent("dynamicModel");
        // entity->addComponent("collider");
        // entity->addComponent("meshRenderer2d");
        // emit parent->entityAddedPointer(QString::fromStdString(entity->parentID),entity);
        // emit parent->meshAdded(QString::fromStdString(entity->ID),QString::fromStdString(entity->Name),entity->transform,entity->meshRenderer2d->Meshes);
        // emit parent->entityAdded(QString::fromStdString(entity->parentID),QString::fromStdString(entity->ID),QString::fromStdString(entityName));
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"transform");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"trajectory");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"rigidbody");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"dynamicModel");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"collider");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"meshRenderer2d");
        parent->dictionry[entity->parentID].push_back(entity->ID);
    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Entities is null!");
    }

    return entity;
}

void ProfileCategaory::addEntityWithObject(Entity *entity){
    entity->parentID = ID;
    Entities.insert({entity->ID, entity});

    // Automatically update hierarchy's Folders
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (parent && parent->Entities) {
        parent->Entities->insert({entity->ID, entity});
        entity->spawn();
        // entity->addComponent("transform");
        // entity->addComponent("trajectory");
        // entity->addComponent("rigidbody");
        // entity->addComponent("dynamicModel");
        // entity->addComponent("collider");
        // entity->addComponent("meshRenderer2d");
        // emit parent->entityAddedPointer(QString::fromStdString(entity->parentID),entity);
        // emit parent->meshAdded(QString::fromStdString(entity->ID),QString::fromStdString(entity->Name),entity->transform,entity->meshRenderer2d->Meshes);
        // emit parent->entityAdded(QString::fromStdString(entity->parentID),QString::fromStdString(entity->ID),QString::fromStdString(entity->Name));
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"transform");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"trajectory");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"rigidbody");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"dynamicModel");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"collider");
        // emit parent->componentAdded(QString::fromStdString(entity->ID),"meshRenderer2d");
        parent->dictionry[entity->parentID].push_back(entity->ID);
    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Entities is null!");
    }

}

void ProfileCategaory::removeEntity(std::string EntityID){

    delete Entities[EntityID];
    Entities.erase(EntityID);
    // Automatically update hierarchy's Folders
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (parent && parent->Entities) {
        parent->Entities->erase(EntityID);
        emit parent->entityRemoved(QString::fromStdString(EntityID));
    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Folders is null!");
    }
}


QJsonObject ProfileCategaory::toJson() {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Profile");
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;

    // Serialize Folders
    QJsonObject folderObj;
    for (const auto& [key, folderPtr] : Folders) {
        if (folderPtr) {
            folderObj[QString::fromStdString(key)] = folderPtr->toJson();
        }
    }
    obj["folders"] = folderObj;

    // Serialize Entities
    QJsonObject entityObj;
    for (const auto& [key, entityPtr] : Entities) {
        if (entityPtr) {
            entityObj[QString::fromStdString(key)] = entityPtr->toJson();
        }
    }
    obj["entities"] = entityObj;

    return obj;
}

void ProfileCategaory::fromJson(const QJsonObject& obj)
{

    // Basic fields
    Name = obj["name"].toString().toStdString();
    ID = obj["id"].toString().toStdString();
    Active = obj["active"].toBool();

    // Foldersid
    if (obj.contains("folders") && obj["folders"].isObject()) {
        QJsonObject folderObj = obj["folders"].toObject();
        for (const QString& key : folderObj.keys()) {
            QJsonObject folderJson = folderObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            Folder *folder = new Folder(parent);
            folder->Name = folderJson["name"].toString().toStdString();
            folder->ID = folderJson["id"].toString().toStdString();
            if (folder) {
                addFolderWithObject(folder);
                folder->fromJson(folderJson);
            }
        }
    }

    // Entities
    if (obj.contains("entities") && obj["entities"].isObject()) {
        QJsonObject entityObj = obj["entities"].toObject();
        for (const QString& key : entityObj.keys()) {
            QJsonObject entityJson = entityObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            Entity *entity = new Entity(parent);
            entity->Name = entityJson["name"].toString().toStdString();
            entity->ID = entityJson["id"].toString().toStdString();
            if (entity) {
                addEntityWithObject(entity);
                entity->fromJson(entityJson);
            }
        }
    }

}
