
#include "hierarchy.h"
#include "core/Debug/console.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include <QJsonArray>
#include <QJsonDocument>

// Thread-local context pointer
thread_local Hierarchy* Hierarchy::currentContext = nullptr;

// Constructor
Hierarchy::Hierarchy()
{
    Folders = new std::unordered_map<std::string, Folder*>();
    Entities = new std::unordered_map<std::string, Entity*>();
    setCurrentContext(this);
}

// Destructor
Hierarchy::~Hierarchy()
{
    for (auto& [key, folder] : *Folders) delete folder;
    for (auto& [key, entity] : *Entities) delete entity;
    for (auto& [key, profile] : ProfileCategories) delete profile;
    delete Folders;
    delete Entities;
}

ProfileCategaory* Hierarchy::addProfileCategaory(QString profileName)
{
    ProfileCategaory* profile = new ProfileCategaory(this);
    profile->Name = profileName.toStdString();
    ProfileCategories.insert({profile->ID, profile});
    emit profileAddedPointer(profile);
    emit profileAdded(QString::fromStdString(profile->ID), QString::fromStdString(profile->Name));
    return profile;
}

void Hierarchy::addProfileCategaoryWithObject(ProfileCategaory *profile)
{
    ProfileCategories.insert({profile->ID, profile});
    emit profileAddedPointer(profile);
    emit profileAdded(QString::fromStdString(profile->ID), QString::fromStdString(profile->Name));
}

void Hierarchy::removeProfileCategaory(QString ID)
{
    delete ProfileCategories[ID.toStdString()];
    ProfileCategories.erase(ID.toStdString());
    emit profileRemoved(ID);
}

void Hierarchy::renameProfileCategaory(QString Id, QString name)
{
    ProfileCategories[Id.toStdString()]->Name = name.toStdString();
    emit profileRenamed(Id, name);
}

Folder* Hierarchy::addFolder(QString parentId, QString FolderName, bool Profile)
{
    if (Profile) {
        return ProfileCategories[parentId.toStdString()]->addFolder(FolderName.toStdString());
    } else {
        return (*Folders)[parentId.toStdString()]->addFolder(FolderName.toStdString());
    }
}

void Hierarchy::removeFolder(QString parentId, QString ID, bool Profile)
{
    if (Profile || ProfileCategories.count(parentId.toStdString())) {
        ProfileCategories[parentId.toStdString()]->removeFolder(ID.toStdString());
    } else {
        (*Folders)[parentId.toStdString()]->removeFolder(ID.toStdString());
    }
}

void Hierarchy::renameFolder(QString Id, QString name)
{
    (*Folders)[Id.toStdString()]->Name = name.toStdString();
    emit folderRenamed(Id, name);
}

Entity* Hierarchy::addEntity(QString parentId, QString EntityName, bool Profile)
{
    if (Profile) {
        return ProfileCategories[parentId.toStdString()]->addEntity(EntityName.toStdString());
    } else {
        return (*Folders)[parentId.toStdString()]->addEntity(EntityName.toStdString());
    }
}

void Hierarchy::addEntityViaNetwork(QString parentId, QJsonObject obj, bool Profile)
{
    addEntityFromJson(parentId, obj, Profile);
}

Entity* Hierarchy::addEntityFromJson(QString parentId, QJsonObject obj, bool Profile)
{
    QString EntityName = obj["name"].toString();
    Entity* entity;
    if (Profile) {
        entity = ProfileCategories[parentId.toStdString()]->addEntity(EntityName.toStdString());
    } else {
        entity = (*Folders)[parentId.toStdString()]->addEntity(EntityName.toStdString());
    }
    std::string id = entity->ID;
    entity->fromJson(obj);
    entity->ID = id;
    return entity;
}

void Hierarchy::removeEntity(QString parentId, QString ID, bool Profile)
{
    if (Profile || ProfileCategories.count(parentId.toStdString())) {
        ProfileCategories[parentId.toStdString()]->removeEntity(ID.toStdString());
    } else {
        (*Folders)[parentId.toStdString()]->removeEntity(ID.toStdString());
    }
}

void Hierarchy::renameEntity(QString Id, QString name)
{
    (*Entities)[Id.toStdString()]->Name = name.toStdString();
    emit entityRenamed(Id, name);
}

void Hierarchy::addComponent(QString ID, QString componentName)
{
    if (Entities->find(ID.toStdString()) != Entities->end()) {
        (*Entities)[ID.toStdString()]->addComponent(componentName.toStdString());
        emit componentAdded(ID, componentName);
        getCurrentJsonData(); // Emit updated JSON
        Console::log("Hierarchy::addComponent emitted getJsonData for " + ID.toStdString() + ", component: " + componentName.toStdString());
    }
}

QJsonObject Hierarchy::getComponentData(QString ID, QString componentName)
{
    if (Entities->find(ID.toStdString()) != Entities->end()) {
        return (*Entities)[ID.toStdString()]->getComponent(componentName.toStdString());
    }
    return QJsonObject();
}

void Hierarchy::UpdateComponent(QString ID, QString componentName, QJsonObject delta)
{
    if (Entities->find(ID.toStdString()) != Entities->end()) {
        Entity* entity = (*Entities)[ID.toStdString()];
        QJsonObject currentData = entity->getComponent(componentName.toStdString());
        // Merge delta into current data, preserving existing keys
        QJsonObject mergedData = currentData;
        for (auto it = delta.begin(); it != delta.end(); ++it) {
            mergedData[it.key()] = it.value();
        }
        entity->updateComponent(componentName, mergedData);
        emit entityUpdate(ID);
        getCurrentJsonData(); // Emit updated JSON
        Console::log("Hierarchy::UpdateComponent merged data for " + componentName.toStdString() + ": " + QString(QJsonDocument(mergedData).toJson()).toStdString());
        Console::log("Hierarchy::UpdateComponent emitted getJsonData for " + ID.toStdString());
    } else {
        Console::log("Entity not found: " + ID.toStdString());
    }
}

void Hierarchy::onParameterChanged(const QString &entityID, const QString &componentName, const QString &key, const QString &parameterType, bool add)
{
    if (Entities->find(entityID.toStdString()) != Entities->end()) {
        Entity* entity = (*Entities)[entityID.toStdString()];
        QJsonObject currentData = entity->getComponent(componentName.toStdString());
        if (add) {
            // Parameter addition is handled by UpdateComponent via valueChanged signal
            Console::log("Hierarchy::onParameterChanged added parameter " + key.toStdString() + " for " + componentName.toStdString());
        } else {
            // Remove parameter
            if (currentData.contains(key)) {
                QJsonObject mergedData = currentData;
                mergedData.remove(key);
                entity->updateComponent(componentName, mergedData);
                Console::log("Hierarchy::onParameterChanged removed parameter " + key.toStdString() + " from " + componentName.toStdString());
            } else {
                Console::log("Parameter not found: " + key.toStdString());
            }
        }
        emit entityUpdate(entityID);
        getCurrentJsonData(); // Emit updated JSON
        Console::log("Hierarchy::onParameterChanged emitted getJsonData for " + entityID.toStdString());
    } else {
        Console::log("Entity not found: " + entityID.toStdString());
    }
}

void Hierarchy::removeComponent(QString entityId, QString componentName)
{
    if (Entities->find(entityId.toStdString()) != Entities->end()) {
        (*Entities)[entityId.toStdString()]->removeComponent(componentName.toStdString());
        emit componentRemoved(entityId, componentName);
        emit entityUpdate(entityId);
        getCurrentJsonData(); // Emit updated JSON
        Console::log("Hierarchy::removeComponent emitted getJsonData for " + entityId.toStdString() + ", component: " + componentName.toStdString());
    }
}

QJsonObject Hierarchy::toJson()
{
    QJsonObject obj;

    // Serialize Profile Categories
    QJsonObject profileCategoriesObj;
    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr) {
            profileCategoriesObj[QString::fromStdString(key)] = profilePtr->toJson();
        }
    }
    obj["profileCategories"] = profileCategoriesObj;

    // Serialize Folders
    if (Folders) {
        QJsonObject foldersObj;
        for (const auto& [key, folderPtr] : *Folders) {
            if (folderPtr) {
                foldersObj[QString::fromStdString(key)] = QString::fromStdString(folderPtr->Name);
            }
        }
        obj["folders"] = foldersObj;
    }

    // Serialize Entities
    if (Entities) {
        QJsonObject entitiesObj;
        static const QStringList componentNames = {
            "transform",  // Added transform component
            "collider",
            "trajectory",
            "dynamicModel",
            "meshRenderer2d"
        };
        for (const auto& [key, entityPtr] : *Entities) {
            if (entityPtr) {
                QJsonObject entityObj;
                entityObj["name"] = QString::fromStdString(entityPtr->Name);
                for (const QString& compName : componentNames) {
                    QJsonObject compData = entityPtr->getComponent(compName.toStdString());
                    if (!compData.isEmpty()) {
                        entityObj[compName] = compData;
                    }
                }
                entitiesObj[QString::fromStdString(key)] = entityObj;
            }
        }
        obj["entities"] = entitiesObj;
    }

    Console::log("Hierarchy::toJson output: " + QString(QJsonDocument(obj).toJson()).toStdString());
    return obj;
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

    if (obj.contains("profileCategories") && obj["profileCategories"].isObject()) {
        QJsonObject profileCategoriesObj = obj["profileCategories"].toObject();
        for (const QString& key : profileCategoriesObj.keys()) {
            QJsonObject catObj = profileCategoriesObj[key].toObject();
            ProfileCategaory* profile = new ProfileCategaory(this);
            profile->Name = catObj["name"].toString().toStdString();
            profile->ID = catObj["id"].toString().toStdString();
            if (profile) {
                addProfileCategaoryWithObject(profile);
                profile->fromJson(catObj);
            }
        }
    }
}

void Hierarchy::getCurrentJsonData()
{
    emit getJsonData(toJson());
}
