#include "folder.h"
#include <core/Debug/console.h>
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <core/Hierarchy/hierarchy.h> // Required for type
#include <core/Utility/uuid.h>

Folder::Folder(Hierarchy* h) {
    ID = Uuid::generateShortUniqueID();
    // Register this Folder automatically
    Hierarchy* hierarchy = h;// Hierarchy::getCurrentContext(); // 💡 We'll create this next
    if (hierarchy) {
        GlobalRegistry::registerFolder(this, hierarchy);
        hierarchy->dictionry[ID] = {};
    }
}

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


Folder* Folder::addFolder(std::string folderName){
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


void Folder::addFolderWithObject(Folder *folder){
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


void Folder::removeFolder(std::string folderID){

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

Entity* Folder::addEntity(std::string entityName){
    // if(Entities.count(entityName)){

    //     Console::error(
    //         "RunTimeError::" + std::string(__FILE__) + "," +
    //         std::to_string(__LINE__) + " Folder already exists With Same Name"
    //         );
    //     return nullptr;
    // }
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    Entity *entity = new Platform(parent);
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

        parent->dictionry[entity->parentID].push_back(entity->ID);
    } else {
        Console::error(
            "RunTimeError::" + std::string(__FILE__) + "," +
            std::to_string(__LINE__) +
            "Hierarchy parent or parent->Entities is null!");
    }

    return entity;
}

void Folder::addEntityWithObject(Entity *entity){
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

void Folder::removeEntity(std::string EntityID){

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


QJsonObject Folder::toJson() {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Folder");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
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

void Folder::fromJson(const QJsonObject& obj)
{

    // Basic values
    Name = obj["name"].toString().toStdString();
    ID = obj["id"].toString().toStdString();
    parentID = obj["parent_id"].toString().toStdString();
    Active = obj["active"].toBool();

    // 🔸 Deserialize nested folders
    if (obj.contains("folders") && obj["folders"].isObject()) {
        QJsonObject foldersObj = obj["folders"].toObject();
        for (const QString& key : foldersObj.keys()) {
            QJsonObject childFolderObj = foldersObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            Folder *folder = new Folder(parent);
            folder->Name = childFolderObj["name"].toString().toStdString();
            folder->ID = childFolderObj["id"].toString().toStdString();
            if (folder) {
                addFolderWithObject(folder);
                folder->fromJson(childFolderObj);
            }
        }
    }

    // 🔸 Deserialize entities
    if (obj.contains("entities") && obj["entities"].isObject()) {
        QJsonObject entitiesObj = obj["entities"].toObject();
        for (const QString& key : entitiesObj.keys()) {
            QJsonObject entityObj = entitiesObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            Entity *entity = new Platform(parent);
            entity->Name = entityObj["name"].toString().toStdString();
            entity->ID = entityObj["id"].toString().toStdString();
            if (entity) {
                addEntityWithObject(entity);
                entity->fromJson(entityObj);
            }
        }
    }

}
