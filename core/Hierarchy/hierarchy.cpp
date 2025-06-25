#include "hierarchy.h"
#include "core/Debug/console.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include <QJsonArray>

// 🔸 Thread-local context pointer (for auto-registration)
thread_local Hierarchy* Hierarchy::currentContext = nullptr;

// 🔹 Constructor
Hierarchy::Hierarchy()
{
    // Initialize maps for folders and entities
    Folders = new std::unordered_map<std::string, Folder*>();
    Entities = new std::unordered_map<std::string, Entity*>();

    // Set this instance as the current context for registration
    setCurrentContext(this);

    Entity* e = new Platform(this);


    // Check if it's Platform
    if (Platform* p = dynamic_cast<Platform*>(e)) {
        qDebug() << "It's a Platform!";
    }
    // Check if it's Radio
    else if (Radio* r = dynamic_cast<Radio*>(e)) {
        qDebug() << "It's a Radio!";
    }
    // Check if it's IFF
    else if (IFF* i = dynamic_cast<IFF*>(e)) {
        qDebug() << "It's an IFF!";
    }
    else {
        qDebug() << "Unknown Entity Type!";
    }
    delete e;
}



// 🔹 Add a new ProfileCategory to the hierarchy
ProfileCategaory* Hierarchy::addProfileCategaory(QString profileName)
{
    // Create and register new profile
    ProfileCategaory* profile = new ProfileCategaory(this);
    profile->Name = profileName.toStdString();
    ProfileCategories.insert({profile->ID, profile});

    emit profileAddedPointer(profile); // Signal for UI or observers
    emit profileAdded(QString::fromStdString(profile->ID),QString::fromStdString(profile->Name));
    return profile;
}


void Hierarchy::addProfileCategaoryWithObject(ProfileCategaory *profile)
{
    ProfileCategories.insert({profile->ID, profile});
    emit profileAddedPointer(profile); // Signal for UI or observers
    emit profileAdded(QString::fromStdString(profile->ID),QString::fromStdString(profile->Name));
}

void Hierarchy::removeProfileCategaory(QString ID){
    delete ProfileCategories[ID.toStdString()];
    ProfileCategories.erase(ID.toStdString());
    emit profileRemoved(ID);
}

void Hierarchy::renameProfileCategaory(QString Id, QString name){
    ProfileCategories[Id.toStdString()]->Name = name.toStdString();
    emit profileRenamed(Id,name);
}

Folder* Hierarchy::addFolder(QString parentId,QString FolderName,bool Profile){
    if(Profile){
        return ProfileCategories[parentId.toStdString()]->addFolder(FolderName.toStdString());
    }else{
        //::log((*Folders)[parentId.toStdString()]);
        return (*Folders)[parentId.toStdString()]->addFolder(FolderName.toStdString());
    }
}
void Hierarchy::removeFolder(QString parentId,QString ID,bool Profile){
    if(Profile || ProfileCategories.count(parentId.toStdString())){
        return ProfileCategories[parentId.toStdString()]->removeFolder(ID.toStdString());
    }else{
        return (*Folders)[parentId.toStdString()]->removeFolder(ID.toStdString());
    }
}

void Hierarchy::renameFolder(QString Id, QString name){
    (*Folders)[Id.toStdString()]->Name = name.toStdString();
    emit folderRenamed(Id,name);
}

Entity* Hierarchy::addEntity(QString parentId,QString EntityName,bool Profile){
    if(Profile){
        return ProfileCategories[parentId.toStdString()]->addEntity(EntityName.toStdString());
    }else{
        return (*Folders)[parentId.toStdString()]->addEntity(EntityName.toStdString());
    }
}


//=====addentityViaNetwork======
void Hierarchy::addEntityViaNetwork(QString parentId,QJsonObject obj,bool Profile){
    addEntityFromJson(parentId,obj,Profile);
}

Entity* Hierarchy::addEntityFromJson(QString parentId,QJsonObject obj,bool Profile){
    QString EntityName = obj["name"].toString();
    Entity* entity;
    if(Profile){
        entity = ProfileCategories[parentId.toStdString()]->addEntity(EntityName.toStdString());
    }else{
        entity = (*Folders)[parentId.toStdString()]->addEntity(EntityName.toStdString());
    }
    std::string id = entity->ID;
    entity->fromJson(obj);
    entity->ID = id;
    return entity;
}

void Hierarchy::removeEntity(QString parentId,QString ID,bool Profile){
    if(Profile || ProfileCategories.count(parentId.toStdString())){
        return ProfileCategories[parentId.toStdString()]->removeEntity(ID.toStdString());
    }else{
        return (*Folders)[parentId.toStdString()]->removeEntity(ID.toStdString());
    }
}

void Hierarchy::renameEntity(QString Id, QString name){
    (*Entities)[Id.toStdString()]->Name = name.toStdString();
    emit entityRenamed(Id,name);
}
//==============
void Hierarchy::addComponent(QString ID,QString componentName){
    if (Entities->find(ID.toStdString()) != Entities->end()) {
        (*Entities)[ID.toStdString()]->addComponent(componentName.toStdString());
    }
}

QJsonObject Hierarchy::getComponentData(QString ID,QString componentName){
    if (Entities->find(ID.toStdString()) != Entities->end()) {
        return (*Entities)[ID.toStdString()]->getComponent(componentName.toStdString());
    }
}

void Hierarchy::UpdateComponent(QString ID,QString componentName,QJsonObject delta){
    if (Entities->find(ID.toStdString()) != Entities->end()){
        (*Entities)[ID.toStdString()]->updateComponent(componentName,delta);
        emit entityUpdate(ID);
        Console::log("🎯 Entity found: " + componentName.toStdString() + ", applying changes...");
        Console::log(delta);
    }
}

// 🔹 Convert the entire Hierarchy to JSON
QJsonObject Hierarchy::toJson()
{
    QJsonObject obj;

    // 🔸 Serialize Profile Categories
    QJsonObject profileCategoriesObj;
    for (const auto& [key, profilePtr] : ProfileCategories) {
        if (profilePtr) {
            profileCategoriesObj[QString::fromStdString(key)] = profilePtr->toJson();
        }
    }
    obj["profileCategories"] = profileCategoriesObj;

    // 🔸 Serialize Folders
    if (Folders) {
        QJsonObject foldersObj;
        for (const auto& [key, folderPtr] : *Folders) {
            if (folderPtr) {
                foldersObj[QString::fromStdString(key)] = QString::fromStdString(folderPtr->Name);
            }
        }
        obj["folders"] = foldersObj;
    }

    // 🔸 Serialize Entities
    if (Entities) {
        QJsonObject entitiesObj;
        for (const auto& [key, entityPtr] : *Entities) {
            if (entityPtr) {
                entitiesObj[QString::fromStdString(key)] = QString::fromStdString(entityPtr->Name);
            }
        }
        obj["entities"] = entitiesObj;
    }



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
    // 🔸 Deserialize Profile Categories
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
void Hierarchy::removeComponent(QString entityId, QString componentName)
{
    if (Entities->find(entityId.toStdString()) != Entities->end()) {
        (*Entities)[entityId.toStdString()]->removeComponent(componentName.toStdString());
        emit componentRemoved(entityId, componentName);
        emit entityUpdate(entityId); // Notify that the entity has changed
    }
}

// getcurrentjsonData

void Hierarchy::getCurrentJsonData(){
    emit getJsonData(toJson());
}

