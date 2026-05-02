// =============================================================================
// FILE:        hierarchy.h
// MODULE:      Tactical Simulation Hierarchy Management
// PROJECT:      Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:     RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the Hierarchy class, acting as the central registry and
//               management hub for all simulation entities, folders, and
//               profiles. It facilitates fast lookup via unordered_maps and
//               coordinates network/logger synchronization through Qt signals.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Dec 2025  Added support for Platform, Radio, and Sensor mapping.
//   Rev 3  Mar 2026  Integrated IFF, Weapons, and Mission tracking.
//   Rev 4  Apr 2026  Added DO-178C compliant documentation and bomb lifecycle.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

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

// =============================================================================
// CLASS: Hierarchy
//
// DESCRIPTION: Manages the lifecycle and categorization of simulation objects.
//              Provides a signal-slot interface for UI and network updates.
//              Maintains engagement and detection statistics for Red/Blue teams.
// =============================================================================
class Hierarchy : public QObject
{
    Q_OBJECT
public:
    Hierarchy();
    ~Hierarchy();

    // =========================================================================
    // SECTION: Fast Lookup Maps
    // DESCRIPTION: Standard library containers for O(1) access to entities
    //              by their unique string identifiers.
    // =========================================================================
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


    // =========================================================================
    // SECTION: Simulation Analytics
    // DESCRIPTION: Tracks engagement, detection, and damage metrics over time.
    // =========================================================================
public:
    /**
     * @brief Creates and adds a new profile category with the given name.
     * @param profileName Display name of the profile category.
     * @return Pointer to the newly created ProfileCategaory object.
     */
    ProfileCategaory* addProfileCategaory(QString profileName);

    /**
     * @brief Adds an existing ProfileCategaory object to the registry.
     * @param profile Pointer to the ProfileCategaory to add (ownership transferred).
     */
    void addProfileCategaoryWithObject(ProfileCategaory *profile);

    /**
     * @brief Removes the profile category identified by the given ID.
     * @param ID Unique identifier of the profile category.
     */
    void removeProfileCategaory(QString ID);

    /**
     * @brief Retrieves a profile category by its unique ID.
     * @param ID Unique identifier.
     * @return Pointer to the profile, or nullptr if not found.
     */
    ProfileCategaory* getProfileById(QString ID);

    /**
     * @brief Retrieves a profile category by its display name.
     * @param name Display name.
     * @return Pointer to the profile, or nullptr if not found.
     */
    ProfileCategaory* getProfileByName(QString name);

    /**
     * @brief Renames an existing profile category.
     * @param Id Unique identifier of the profile category.
     * @param name New display name.
     */
    void renameProfileCategaory(QString Id, QString name);

signals:
    /**
     * @brief Emitted when a profile category is added (pointer version).
     * @param profile Pointer to the newly added ProfileCategaory.
     */
    void profileAddedPointer(ProfileCategaory* profile);

    /**
     * @brief Emitted when a profile category is added (ID and name version).
     * @param ID Unique identifier.
     * @param profileName Display name.
     */
    void profileAdded(QString ID, QString profileName);

    /**
     * @brief Emitted when a profile category is removed.
     * @param ID Unique identifier of the removed category.
     */
    void profileRemoved(QString ID);

    /**
     * @brief Emitted when a profile category is renamed.
     * @param Id Unique identifier.
     * @param name New display name.
     */
    void profileRenamed(QString Id, QString name);

    // =========================================================================
    // SECTION: Folder & Entity Management
    // DESCRIPTION: CRUD operations for the simulation tree hierarchy.
    // =========================================================================//
public:
    /**
     * @brief Creates a new folder under a given parent.
     * @param parentId ID of the parent folder (or root if empty).
     * @param FolderName Display name of the new folder.
     * @param Profile If true, folder belongs to a profile category.
     * @return Pointer to the newly created Folder.
     */
    Folder* addFolder(QString parentId, QString FolderName, bool Profile);

    /**
     * @brief Creates a folder from network data (e.g., from a remote client).
     * @param parentId Parent folder ID.
     * @param ID Unique identifier for the folder (provided by network).
     * @param FolderName Display name.
     * @param Profile Profile flag.
     */
    void addFolderViaNetwork(QString parentId,QString ID,QString FolderName,bool Profile);

    /**
     * @brief Renames an existing folder.
     * @param Id Folder ID.
     * @param name New name.
     */
    void renameFolder(QString Id, QString name);

    /**
     * @brief Removes a folder from its parent.
     * @param parentId Parent folder ID.
     * @param ID Folder ID to remove.
     */
    void removeFolder(QString parentId, QString ID);

    /**
     * @brief Removes a folder based on network command.
     * @param ID Folder ID to remove.
     */
    void removeFolderViaNetwork(QString ID);

signals:
    /**
     * @brief Emitted when a folder is added (pointer version).
     * @param parentID Parent folder ID.
     * @param folder Pointer to the added Folder.
     */
    void folderAddedPointer(QString parentID, Folder* folder);

    /**
     * @brief Emitted when a folder is added (ID and name version).
     * @param parentID Parent folder ID.
     * @param ID Folder ID.
     * @param folderName Folder name.
     */
    void folderAdded(QString parentID, QString ID, QString folderName);

    /**
     * @brief Emitted when a folder is removed.
     * @param ID Folder ID.
     */
    void folderRemoved(QString ID);

    /**
     * @brief Emitted when a folder is renamed.
     * @param Id Folder ID.
     * @param name New name.
     */
    void folderRenamed(QString Id, QString name);

    //////////Entity///////////
public:
    /**
     * @brief Creates a new entity under a given parent.
     * @param parentId Parent folder ID.
     * @param EntityName Display name of the entity.
     * @param Profile Profile flag.
     * @param data1 Optional extra data (e.g., initial configuration).
     * @return Pointer to the newly created Entity.
     */
    Entity* addEntity(QString parentId, QString EntityName, bool Profile,QString data1 = "");

    /**
     * @brief Creates an entity from network data.
     * @param parentId Parent folder ID.
     * @param ID Entity ID (provided by network).
     * @param EntityName Display name.
     * @param Profile Profile flag.
     */
    void addEntityViaNetwork(QString parentId,QString ID,QString EntityName,bool Profile);

    /**
     * @brief Creates an entity from logger data (replay mode).
     * @param parentId Parent folder ID.
     * @param ID Entity ID.
     * @param EntityName Display name.
     * @param Profile Profile flag.
     */
    void addEntityViaLogger(QString parentId,QString ID,QString EntityName,bool Profile);

    /**
     * @brief Creates an entity from a JSON object (deserialization).
     * @param parentId Parent folder ID.
     * @param obj JSON object containing entity definition.
     * @param Profile Profile flag.
     * @return Pointer to the created Entity, or nullptr on failure.
     */
    Entity* addEntityFromJson(QString parentId, QJsonObject obj, bool Profile);

    /**
     * @brief Retrieves an entity by its unique ID.
     * @param ID Entity ID.
     * @return Pointer to the Entity, or nullptr if not found.
     */
    Entity* getEntityById(QString ID);

    /**
     * @brief Renames an existing entity.
     * @param Id Entity ID.
     * @param name New name.
     */
    void renameEntity(QString Id, QString name);

    /**
     * @brief Removes an entity from its parent.
     * @param parentId Parent folder ID.
     * @param ID Entity ID to remove.
     */
    void removeEntity(QString parentId, QString ID);


signals:
    /**
     * @brief Emitted when an entity is added (pointer version).
     * @param parentID Parent folder ID.
     * @param entity Pointer to the added Entity.
     */
    void entityAddedPointer(QString parentID, Entity* entity);

    /**
     * @brief Emitted when an entity is added (ID and name version).
     * @param parentID Parent folder ID.
     * @param ID Entity ID.
     * @param entityName Entity name.
     */
    void entityAdded(QString parentID, QString ID, QString entityName);

    /**
     * @brief Emitted when an entity is removed.
     * @param ID Entity ID.
     */
    void entityRemoved(QString ID);

    /**
     * @brief Emitted when an entity is fully removed (including profile info).
     * @param parentId Parent folder ID.
     * @param ID Entity ID.
     * @param Profile Profile flag.
     */
    void entityRemovedfull(QString parentId, QString ID, bool Profile);

    /**
     * @brief Emitted when an entity is renamed.
     * @param Id Entity ID.
     * @param name New name.
     */
    void entityRenamed(QString Id, QString name);

    /**
     * @brief Emitted when a mesh is added to an entity.
     * @param ID Entity ID.
     * @param entity Pointer to the entity.
     */
    void entityMeshAdded(QString ID, Entity* entity);

    /**
     * @brief Emitted when a mesh is removed from an entity.
     * @param ID Entity ID.
     */
    void entityMeshRemoved(QString ID);

    /**
     * @brief Emitted when physics are added to an entity.
     * @param ID Entity ID.
     * @param entity Pointer to the entity.
     */
    void entityPhysicsAdded(QString ID, Entity* entity);

    /**
     * @brief Emitted when physics are removed from an entity.
     * @param ID Entity ID.
     */
    void entityPhysicsRemoved(QString ID);

    /**
     * @brief Emitted when any entity property updates (e.g., position, status).
     * @param ID Entity ID.
     */
    void entityUpdate(QString ID);

    /**
     * @brief Emitted when a component of an entity changes.
     * @param ID Entity ID.
     * @param name Component name.
     * @param delta JSON delta of changes.
     */
    void entityComponentUpdate(QString ID, QString name, QJsonObject delta);

    /**
     * @brief Emitted when a sub‑component of an entity changes.
     * @param ID Entity ID.
     * @param name Sub‑component name.
     * @param delta JSON delta of changes.
     */
    void entitySubComponentUpdate(QString ID, QString name, QJsonObject delta);

    /**
     * @brief Emitted when a 2D mesh renderer is added to an entity.
     * @param ID Entity ID.
     * @param meshRenderer2D Pointer to the MeshRenderer2D.
     */
    void meshRenderer2DisAdded(const QString &ID, MeshRenderer2D* meshRenderer2D);

    /**
     * @brief Emitted when a trajectory is added to an entity.
     * @param ID Entity ID.
     * @param trajectory Pointer to the Trajectory.
     */
    void trajectoryisAdded(const QString &ID, Trajectory* trajectory);


    // =========================================================================
    // SECTION: Component & Attachment Management
    // DESCRIPTION: Dynamically attaches sensors, radios, and weapons to entities.
    // =========================================================================
public:
    /**
     * @brief Adds a component (e.g., sensor, radio) to an entity.
     * @param Id Entity ID.
     * @param ComponentName Name of the component to add.
     */
    void addComponent(QString Id, QString ComponentName);

    /**
     * @brief Retrieves component data as JSON.
     * @param ID Entity ID.
     * @param componentName Component name.
     * @return QJsonObject containing component data.
     */
    QJsonObject getComponentData(QString ID, QString componentName);

    /**
     * @brief Updates a component with delta JSON.
     * @param ID Entity ID.
     * @param name Component name.
     * @param delta JSON object of changes.
     */
    void UpdateComponent(QString ID, QString name, QJsonObject delta);

    /**
     * @brief Removes a component from an entity.
     * @param entityId Entity ID.
     * @param componentName Component name to remove.
     */
    void removeComponent(QString entityId, QString componentName);

signals:
    /**
     * @brief Emitted when a component is added.
     * @param parentID Entity ID.
     * @param ID Component ID.
     * @param componentName Component name.
     */
    void componentAdded(QString parentID,QString ID, QString componentName);

    /**
     * @brief Emitted when a component is removed.
     * @param parentID Entity ID.
     * @param componentName Component name.
     */
    void componentRemoved(QString parentID, QString componentName);

    //////////SubComponents///////////
public:
    /**
     * @brief Adds a sub‑component (e.g., antenna, engine) to a component.
     * @param Id Parent component ID.
     * @param type Type of sub‑component (enum ComponentType).
     * @param subComponentName Name of the sub‑component.
     * @param data1 Optional string data.
     * @param data2 Optional second string data.
     * @param data3 Optional JSON data.
     */
    void addSubComponent(QString Id, QString subComponentName, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject());

    /**
     * @brief Renames a sub‑component.
     * @param ID Parent component ID.
     * @param subComponentID Sub‑component ID.
     * @param newName New name.
     */
    void renameSubComponent(QString ID, QString subComponentID, QString newName);

    /**
     * @brief Removes a sub‑component.
     * @param ID Parent component ID.
     * @param subComponentID Sub‑component ID.
     */
    void removeSubComponent(QString ID, QString subComponentID);

signals:
    /**
     * @brief Emitted when a sub‑component is added.
     * @param parentID Parent component ID.
     * @param ID Sub‑component ID.
     * @param subComponentName Sub‑component name.
     */
    void subComponentAdded(QString parentID,QString ID, QString subComponentName);

    /**
     * @brief Emitted when a sub‑component is renamed.
     * @param componentId Parent component ID.
     * @param subCompId Sub‑component ID.
     * @param newName New name.
     */
    void subComponentRenamed(QString componentId, QString subCompId, QString newName);

    /**
     * @brief Emitted when a sub‑component is removed.
     * @param parentID Parent component ID.
     * @param ID Sub‑component ID.
     * @param subComponentName Sub‑component name.
     */
    void subComponentRemoved(QString parentID,QString ID, QString subComponentName);


    // =========================================================================
    // SECTION: Serialization & State Control
    // DESCRIPTION: Conversion to/from JSON and simulation context management.
    // =========================================================================
public:
    /**
     * @brief Serializes the entire hierarchy to a JSON object.
     * @return QJsonObject representing the current state.
     */
    QJsonObject toJson();

    /**
     * @brief Deserializes the hierarchy from a JSON object.
     * @param obj JSON object to load.
     */
    void fromJson(const QJsonObject& obj);

    /**
     * @brief Emits the current JSON data via the getJsonData signal.
     */
    void getCurrentJsonData();

    /**
     * @brief Attaches an IFF component to an entity.
     * @param Id Entity ID.
     * @param name IFF component name.
     */
    void attchedIff(QString Id, QString name);

    /**
     * @brief Attaches a sensor component to an entity.
     * @param ID Entity ID.
     * @param name Sensor name.
     * @param sensorType Type of sensor (e.g., radar, ESM).
     */
    void attachSensors(QString ID, QString name, QString sensorType);

    /**
     * @brief Attaches a radio component to an entity.
     * @param ID Entity ID.
     * @param name Radio name.
     */
    void attachRadios(QString ID, QString name);

    /**
     * @brief Attaches a weapon component to an entity.
     * @param ID Entity ID.
     * @param name Weapon name.
     */
    void attachWeapons(QString ID, QString name);

    /**
     * @brief Handles parameter changes for a component (e.g., add/remove).
     * @param entityID Entity ID.
     * @param componentName Component name.
     * @param key Parameter key.
     * @param parameterType Type of parameter.
     * @param add True to add, false to remove.
     */
    void onParameterChanged(const QString &entityID, const QString &componentName, const QString &key, const QString &parameterType, bool add);

    /**
     * @brief Searches all profiles and returns a JSON array of results.
     * @return QJsonArray containing matching profiles.
     */
    QJsonArray searchProfile();

    static thread_local Hierarchy* currentContext;
    static void setCurrentContext(Hierarchy* h) {
        currentContext = h;
    }

    static Hierarchy* getCurrentContext() {
        return currentContext;
    }

    void clear();

    /**
     * @brief Loads analysis data from a JSON file.
     * @return QJsonObject containing analysis data.
     */
    QJsonObject loadAnalysisJson();

    /**
     * @brief Performs analytics on engagements, detections, and damages.
     */
    void Anlaysis();

    //TempData
    QJsonObject tempData;
    //
    bool isDatabase = false;
    bool isScenario = false;
    bool isRuntime = false;
    bool fixedProfiles = true;

    struct BroadcastMsg{
        std::string track_id;      // Unique ID
        double latitude;        // WGS84 format
        double longitude;
        float altitude;
        float frequency_mhz;    // Agar ESM data hai
        uint64_t timestamp;     // Microseconds me

    };


signals:
    /**
     * @brief Emitted to initialise the hierarchy after loading.
     */
    void Init();

    void broadCast(BroadcastMsg msg);

    /**
     * @brief Emitted with the current JSON representation of the hierarchy.
     * @param obj JSON object.
     */
    void getJsonData(const QJsonObject& obj);

    /**
     * @brief Emitted for status messages (e.g., to UI or log).
     * @param value Status string.
     */
    void status(QString value);

    // ── Bomb lifecycle signals ────────────────────────────────────────────────
    // Emitted by Bomb::launch() when a bomb is released from its parent aircraft.
    //   bombID     : the Bomb entity's ID
    //   aircraftID : the Platform entity that released it
    //   lat/lon/alt: release position (WGS-84 geocoord)
    /**
     * @brief Emitted when a bomb is launched from an aircraft.
     * @param bombID Bomb entity ID.
     * @param aircraftID Aircraft (Platform) ID.
     * @param lat Launch latitude (WGS‑84).
     * @param lon Launch longitude.
     * @param alt Launch altitude (meters).
     */
    void bombLaunched(const QString& bombID,
                      const QString& aircraftID,
                      double lat, double lon, double alt);

    // Emitted by Bomb::missileEnd() when the bomb reaches its terminal state.
    //   bombID     : the Bomb entity's ID
    //   lat/lon/alt: detonation position
    /**
     * @brief Emitted when a bomb detonates.
     * @param bombID Bomb entity ID.
     * @param lat Detonation latitude.
     * @param lon Detonation longitude.
     * @param alt Detonation altitude.
     */
    void bombDetonated(const QString& bombID,
                       double lat, double lon, double alt);
};

#endif // HIERARCHY_H
