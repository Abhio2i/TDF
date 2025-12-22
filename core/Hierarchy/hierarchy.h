
#ifndef HIERARCHY_H
#define HIERARCHY_H

#include <QObject>
#include <unordered_map>
#include "./profilecategaory.h"
#include "core/Hierarchy/EntityProfiles/fixedpoints.h"
#include "core/Hierarchy/EntityProfiles/formation.h"
#include "core/Hierarchy/EntityProfiles/specialzone.h"

class Hierarchy : public QObject
{
    Q_OBJECT
public:
    Hierarchy();
    ~Hierarchy();

    //TempData
    QJsonObject tempData;
    //
    bool isDatabase = false;
    bool isScenario = false;
    bool isRuntime = false;
    std::unordered_map<std::string, ProfileCategaory*> ProfileCategories;
    std::unordered_map<std::string, std::list<std::string>> dictionry;
    std::unordered_map<std::string, Folder*> *Folders;
    std::unordered_map<std::string, Entity*> *Entities;
    std::unordered_map<std::string, Platform*> *Platforms;
    std::unordered_map<std::string, Radio*> *Radios;
    std::unordered_map<std::string, Sensor*> *Sensors;
    std::unordered_map<std::string, FixedPoints*> *FixedPointes;
    std::unordered_map<std::string, Formation*> *Formations;
    std::unordered_map<std::string, Specialzone*> *Specialzones;
    std::unordered_map<std::string, IFF*> *Iffs;

    std::unordered_map<std::string, Component*> *Components;
    std::unordered_map<std::string, Mesh*> *Meshes;
    std::unordered_map<std::string, Mission*> *missionList;
    std::unordered_map<std::string, std::string*> EntityPaths;
    std::unordered_map<std::string, std::string*> FolderPaths;

    ProfileCategaory* addProfileCategaory(QString profileName);
    void addProfileCategaoryWithObject(ProfileCategaory *profile);
    void removeProfileCategaory(QString ID);

    Folder* addFolder(QString parentId, QString FolderName, bool Profile);
    void addFolderViaNetwork(QString parentId,QString ID,QString FolderName,bool Profile);
    void removeFolder(QString parentId, QString ID, bool Profile);
    void removeFolderViaNetwork(QString ID);

    Entity* addEntity(QString parentId, QString EntityName, bool Profile);
    void addEntityViaNetwork(QString parentId,QString ID,QString EntityName,bool Profile);
    Entity* addEntityFromJson(QString parentId, QJsonObject obj, bool Profile);
    void removeEntity(QString parentId, QString ID, bool Profile);

    void renameProfileCategaory(QString Id, QString name);
    void renameFolder(QString Id, QString name);
    void renameEntity(QString Id, QString name);
    void removeComponent(QString entityId, QString componentName);

    QJsonObject toJson();
    void fromJson(const QJsonObject& obj);
    void getCurrentJsonData();
    void addComponent(QString Id, QString ComponentName);
    void addSubComponent(QString Id, ComponentType type, QString subComponentName, QString data1 = "", QString data2 = "", QString data3 = "");
    void attchedIff(QString Id, QString name);
    void attachSensors(QString ID, QString name, QString sensorType);
    void attachRadios(QString ID, QString name);


    QJsonObject getComponentData(QString ID, QString componentName);
    void UpdateComponent(QString ID, QString name, QJsonObject delta);

    QJsonObject getSubComponentData(QString ID, QString subComponentName);
    void updateSubComponent(QString ID, QString name, QJsonObject delta);

    void onParameterChanged(const QString &entityID, const QString &componentName, const QString &key, const QString &parameterType, bool add);
    QJsonArray searchProfile();
    static thread_local Hierarchy* currentContext;
    static void setCurrentContext(Hierarchy* h) {
        currentContext = h;
    }

    static Hierarchy* getCurrentContext() {
        return currentContext;
    }

signals:
    void profileAddedPointer(ProfileCategaory* profile);
    void folderAddedPointer(QString parentID, Folder* folder);
    void entityAddedPointer(QString parentID, Entity* entity);

    void profileAdded(QString ID, QString profileName);
    void folderAdded(QString parentID, QString ID, QString folderName);
    void entityAdded(QString parentID, QString ID, QString entityName);
    void componentAdded(QString parentID,QString ID, QString componentName);
    void subComponentAdded(QString parentID,QString ID, QString subComponentName);

    void profileRemoved(QString ID);
    void folderRemoved(QString ID);
    void entityRemoved(QString ID);
    void entityRemovedfull(QString parentId, QString ID, bool Profile);
    void componentRemoved(QString parentID, QString componentName);
    void subComponentRemoved(QString parentID, QString subComponentName);


    void profileRenamed(QString Id, QString name);
    void folderRenamed(QString Id, QString name);
    void entityRenamed(QString Id, QString name);

    void entityMeshAdded(QString ID, Entity* entity);
    void entityMeshRemoved(QString ID);

    void entityPhysicsAdded(QString ID, Entity* entity);
    void entityPhysicsRemoved(QString ID);

    void entityUpdate(QString ID);
    void entityComponentUpdate(QString ID, QString name, QJsonObject delta);
    void entitySubComponentUpdate(QString ID, QString name, QJsonObject delta);
    void getJsonData(const QJsonObject& obj);
};

#endif // HIERARCHY_H
