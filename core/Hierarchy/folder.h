// =============================================================================
// FILE:        folder.h
// MODULE:      Tactical Simulation Hierarchy Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Folder class, which represents a logical node in
//              the simulation tree. It manages nested Folders and Entities,
//              supporting recursive hierarchy builds and JSON serialization.
//
// AUTHOR:      Pankaj Chauhan
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Apr 2026  Aligned with DO-178C documentation standards.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef FOLDER_H
#define FOLDER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <core/Hierarchy/EntityProfiles/platform.h>

// Forward declarations
class Hierarchy;

// =============================================================================
// CLASS: Folder
//
// DESCRIPTION: Acts as a container for simulation objects and sub-folders.
//              Handles lifecycle management of its children and provides
//              serialization hooks for scenario saving/loading.
// =============================================================================
class Folder: public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a Folder associated with a given Hierarchy context.
     * @param h Pointer to the parent Hierarchy object.
     */
    Folder(Hierarchy* h);

    /**
     * @brief Destructor – cleans up child folders and entities.
     */
    ~Folder();

    // =========================================================================
    // SECTION: Identity & Properties
    // DESCRIPTION: Core attributes defining the folder's state and type.
    // =========================================================================
    std::string Name;       ///< Display name of the folder.
    bool Active;            ///< Whether the folder is active in simulation.
    std::string ID;         ///< Unique identifier.
    std::string parentID;   ///< ID of the parent folder (or profile).
    Constants::EntityType type = Constants::EntityType::Platform; ///< Type of entities this folder may contain.

    // =========================================================================
    // SECTION: Child Management (Containers)
    // DESCRIPTION: Maps for O(1) access to nested hierarchy elements.
    // =========================================================================
    std::unordered_map<std::string, Folder*> Folders;   ///< Sub‑folders keyed by ID.
    std::unordered_map<std::string, Entity*> Entities;  ///< Entities keyed by ID.
    Mission *mission;           ///< Optional mission associated with this folder.
    //Formation *formation;

    // =========================================================================
    // SECTION: Hierarchy Operations (CRUD)
    // DESCRIPTION: Methods to add or remove children within the simulation tree.
    // =========================================================================

    /**
     * @brief Creates and adds a new sub‑folder.
     * @param name Display name of the new folder.
     * @param iD Optional unique identifier; auto‑generated if empty.
     * @return Pointer to the newly created Folder.
     */
    Folder* addFolder(std::string name, std::string iD = "");

    /**
     * @brief Adds an existing Folder object as a child.
     * @param folder Pointer to the Folder to add (ownership transferred).
     */
    void addFolderWithObject(Folder *folder);

    /**
     * @brief Removes a sub‑folder by its name.
     * @param name Name of the folder to remove.
     */
    void removeFolder(std::string name);

    /**
     * @brief Creates and adds a new entity to this folder.
     * @param name Display name of the entity.
     * @param iD Optional unique identifier; auto‑generated if empty.
     * @param data1 Optional extra data (e.g., initial configuration).
     * @return Pointer to the newly created Entity.
     */
    Entity* addEntity(std::string name, std::string iD = "", QString data1="");

    /**
     * @brief Adds an existing Entity object to this folder.
     * @param entity Pointer to the Entity to add (ownership transferred).
     */
    void addEntityWithObject(Entity *entity);

    /**
     * @brief Removes an entity from this folder by its name.
     * @param name Name of the entity to remove.
     */
    void removeEntity(std::string name);

    /**
     * @brief Sets the profile type for this folder (affects entity filtering).
     * @param Type The entity type to associate.
     */
    void setProfileType(Constants::EntityType Type);

    // =========================================================================
    // SECTION: Serialization
    // DESCRIPTION: Logic for converting folder state to/from JSON format.
    // =========================================================================
    /**
     * @brief Serializes the folder (including children) to a JSON object.
     * @return QJsonObject representing the folder's state.
     */
    QJsonObject toJson();

    /**
     * @brief Deserializes the folder from a JSON object.
     * @param obj JSON object containing folder data.
     */
    void fromJson(const QJsonObject& obj);
};

#endif // FOLDER_H
