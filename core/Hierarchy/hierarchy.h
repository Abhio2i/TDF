#ifndef HIERARCHY_H
#define HIERARCHY_H

#include <QObject>
#include <unordered_map>
#include "./profilecategaory.h"
#include "core/Hierarchy/EntityProfiles/fixedpoints.h"
#include "core/Hierarchy/EntityProfiles/formation.h"
#include "core/Hierarchy/EntityProfiles/specialzone.h"

// Forward declarations to reduce compile time
class Weapon;
class Folder;
class Entity;
class Platform;
class Radio;
class Sensor;
class IFF;
class Component;
class Mesh;
class Mission;
class MeshRenderer2D;
class Trajectory;
enum class ComponentType;
class Hierarchy : public QObject
{
    Q_OBJECT
public:
    Hierarchy();
    ~Hierarchy();

//////////Map For Fast Lookup ///////////
public:
    std::unordered_map<std::string, ProfileCategaory*> ProfileCategories;
    std::unordered_map<std::string, std::list<std::string>> dictionry;
    std::unordered_map<std::string, Folder*> Folders;
    std::unordered_map<std::string, Entity*> Entities;
    std::unordered_map<std::string, Platform*> Platforms;
    std::unordered_map<std::string, Radio*> Radios;
    std::unordered_map<std::string, Sensor*> Sensors;
    std::unordered_map<std::string, FixedPoints*> FixedPointes;
    std::unordered_map<std::string, Formation*> Formations;
    std::unordered_map<std::string, Specialzone*> Specialzones;
    std::unordered_map<std::string, IFF*> Iffs;
    std::unordered_map<std::string, Weapon*> Weapons;

    std::unordered_map<std::string, Component*> Components;
    std::unordered_map<std::string, Mesh*> Meshes;
    std::unordered_map<std::string, Mission*> missionList;
    std::unordered_map<std::string, std::string*> EntityPaths;
    std::unordered_map<std::string, std::string*> FolderPaths;

public:
    std::unordered_map<float, float> redengagements;
    float redlastengagments = 0;
    std::unordered_map<float, float> reddetections;
    float redlastdetections = 0;
    std::unordered_map<float, float> reddamages;
    float redlastdamages = 0;

    std::unordered_map<float, float> blueengagements;
    float bluelastengagments = 0;
    std::unordered_map<float, float> bluedetections;
    float bluelastdetections = 0;
    std::unordered_map<float, float> bluedamages;
    float bluelastdamages = 0;


//////////Profile///////////
public:
    ProfileCategaory* addProfileCategaory(QString profileName);
    void addProfileCategaoryWithObject(ProfileCategaory *profile);
    void removeProfileCategaory(QString ID);
    ProfileCategaory* getProfileById(QString ID);
    ProfileCategaory* getProfileByName(QString name);
    void renameProfileCategaory(QString Id, QString name);

signals:
    void profileAddedPointer(ProfileCategaory* profile);
    void profileAdded(QString ID, QString profileName);
    void profileRemoved(QString ID);
    void profileRenamed(QString Id, QString name);

//////////Folder///////////
public:
    Folder* addFolder(QString parentId, QString FolderName, bool Profile);
    void addFolderViaNetwork(QString parentId,QString ID,QString FolderName,bool Profile);
    void renameFolder(QString Id, QString name);
    void removeFolder(QString parentId, QString ID, bool Profile);
    void removeFolderViaNetwork(QString ID);

signals:
    void folderAddedPointer(QString parentID, Folder* folder);
    void folderAdded(QString parentID, QString ID, QString folderName);
    void folderRemoved(QString ID);
    void folderRenamed(QString Id, QString name);

//////////Entity///////////
public:
    Entity* addEntity(QString parentId, QString EntityName, bool Profile);
    void addEntityViaNetwork(QString parentId,QString ID,QString EntityName,bool Profile);
    void addEntityViaLogger(QString parentId,QString ID,QString EntityName,bool Profile);
    Entity* addEntityFromJson(QString parentId, QJsonObject obj, bool Profile);
    void removeEntity(QString parentId, QString ID, bool Profile);
    Entity* getEntityById(QString ID);
    void renameEntity(QString Id, QString name);

signals:
    void entityAddedPointer(QString parentID, Entity* entity);
    void entityAdded(QString parentID, QString ID, QString entityName);
    void entityRemoved(QString ID);
    void entityRemovedfull(QString parentId, QString ID, bool Profile);
    void entityRenamed(QString Id, QString name);
    void entityMeshAdded(QString ID, Entity* entity);
    void entityMeshRemoved(QString ID);
    void entityPhysicsAdded(QString ID, Entity* entity);
    void entityPhysicsRemoved(QString ID);
    void entityUpdate(QString ID);
    void entityComponentUpdate(QString ID, QString name, QJsonObject delta);
    void entitySubComponentUpdate(QString ID, QString name, QJsonObject delta);
    void meshRenderer2DisAdded(const QString &ID, MeshRenderer2D* meshRenderer2D);
    void trajectoryisAdded(const QString &ID, Trajectory* trajectory);


//////////Components///////////
public:
    void addComponent(QString Id, QString ComponentName);
    void removeComponent(QString entityId, QString componentName);
    QJsonObject getComponentData(QString ID, QString componentName);
    void UpdateComponent(QString ID, QString name, QJsonObject delta);

signals:
    void componentAdded(QString parentID,QString ID, QString componentName);
    void componentRemoved(QString parentID, QString componentName);

//////////SubComponents///////////
public:
    void addSubComponent(QString Id, ComponentType type, QString subComponentName, QString data1 = "", QString data2 = "", QString data3 = "");
    void removeSubComponent(QString ID, QString subComponentID, QString subComponentName);
    QJsonObject getSubComponentData(QString ID, QString subComponentName);
    void updateSubComponent(QString ID, QString name, QJsonObject delta);
    void renameSubComponent(QString ID, QString subComponentID, QString newName);

signals:
    void subComponentAdded(QString parentID,QString ID, QString subComponentName);
    void subComponentRenamed(QString componentId, QString subCompId, QString newName);
    void subComponentRemoved(QString parentID,QString ID, QString subComponentName);


//////////////////////////////////////////////////////////////////////////////////////////////////
public:
    QJsonObject toJson();
    void fromJson(const QJsonObject& obj);
    void getCurrentJsonData();

    void attchedIff(QString Id, QString name);
    void attachSensors(QString ID, QString name, QString sensorType);
    void attachRadios(QString ID, QString name);
    void attachWeapons(QString ID, QString name);

    void onParameterChanged(const QString &entityID, const QString &componentName, const QString &key, const QString &parameterType, bool add);
    QJsonArray searchProfile();
    static thread_local Hierarchy* currentContext;
    static void setCurrentContext(Hierarchy* h) {
        currentContext = h;
    }

    static Hierarchy* getCurrentContext() {
        return currentContext;
    }
    void clear() {
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

        // Clear temporary data
        tempData = QJsonObject();
        dictionry.clear();
        EntityPaths.clear();
        FolderPaths.clear();
    }


    QJsonObject loadAnalysisJson();
    void Anlaysis();

    //TempData
    QJsonObject tempData;
    //
    bool isDatabase = false;
    bool isScenario = false;
    bool isRuntime = false;
    bool fixedProfiles = true;

signals:
    void Init();
    void getJsonData(const QJsonObject& obj);

    void status(QString value);
    // ── Bomb lifecycle signals ────────────────────────────────────────────────
    // Emitted by Bomb::launch() when a bomb is released from its parent aircraft.
    //   bombID     : the Bomb entity's ID
    //   aircraftID : the Platform entity that released it
    //   lat/lon/alt: release position (WGS-84 geocoord)
    void bombLaunched(const QString& bombID,
                      const QString& aircraftID,
                      double lat, double lon, double alt);

    // Emitted by Bomb::missileEnd() when the bomb reaches its terminal state.
    //   bombID     : the Bomb entity's ID
    //   lat/lon/alt: detonation position
    void bombDetonated(const QString& bombID,
                       double lat, double lon, double alt);
};

#endif // HIERARCHY_H
