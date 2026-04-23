// =============================================================================
// FILE:        profilecategaory.h
// MODULE:      Tactical Simulation Profile Management
// PROJECT:      Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:     RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the ProfileCategaory class, which manages specialized
//               collections of folders and entities within a specific profile.
//               Supports serialization to JSON and entity type categorization.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Apr 2026  Standardized documentation and layout.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef PROFILECATEGAORY_H
#define PROFILECATEGAORY_H

#include <QObject>
#include <unordered_map>
#include <string>
#include <QJsonObject>
#include "./folder.h"

// Forward declarations
class Hierarchy;

// =============================================================================
// CLASS: ProfileCategaory
//
// DESCRIPTION: Represents a logical grouping of entities (Platforms, Sensors, etc.)
//              and their folder structure. Acts as a sub-container for the
//              Hierarchy management system.
// =============================================================================
class ProfileCategaory : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructs a ProfileCategaory associated with a given Hierarchy.
     * @param h Pointer to the parent Hierarchy object.
     */
    explicit ProfileCategaory(Hierarchy* h);

    /**
     * @brief Destructor – cleans up contained folders and entities.
     */
    ~ProfileCategaory();

    // =========================================================================
    // SECTION: Public Properties
    // =========================================================================
    std::string Name;                           ///< Display name of the profile category.
    bool Active;                                ///< Whether the profile is active.
    std::string ID;                             ///< Unique identifier.
    Constants::EntityType type = Constants::EntityType::Platform; ///< Type of entities this profile holds.

    // =========================================================================
    // SECTION: Data Containers
    // DESCRIPTION: Local registries for folders and entities specific to
    //              this profile category.
    // =========================================================================
    std::unordered_map<std::string, Folder*> Folders;   ///< Sub‑folders keyed by ID.
    std::unordered_map<std::string, Entity*> Entities;  ///< Entities keyed by ID.

    // =========================================================================
    // SECTION: Folder Management
    // =========================================================================
    /**
     * @brief Creates and adds a new folder under this profile category.
     * @param name Display name of the folder.
     * @param iD Optional unique identifier; auto‑generated if empty.
     * @return Pointer to the newly created Folder.
     */
    Folder* addFolder(std::string name, std::string iD = "");

    /**
     * @brief Adds an existing Folder object to this profile category.
     * @param folder Pointer to the Folder to add (ownership transferred).
     */
    void addFolderWithObject(Folder *folder);

    /**
     * @brief Removes a folder from this profile category by its name.
     * @param name Name of the folder to remove.
     */
    void removeFolder(std::string name);

    // =========================================================================
    // SECTION: Entity Management
    // =========================================================================
    /**
     * @brief Creates and adds a new entity to this profile category.
     * @param name Display name of the entity.
     * @param iD Optional unique identifier; auto‑generated if empty.
     * @param data1 Optional extra data (e.g., sensor subtype).
     * @return Pointer to the newly created Entity.
     */
    Entity* addEntity(std::string name, std::string iD = "", QString data1 = "");

    /**
     * @brief Adds an existing Entity object to this profile category.
     * @param entity Pointer to the Entity to add (ownership transferred).
     */
    void addEntityWithObject(Entity *entity);

    /**
     * @brief Removes an entity from this profile category by its name.
     * @param name Name of the entity to remove.
     */
    void removeEntity(std::string name);

    // =========================================================================
    // SECTION: Configuration & Serialization
    // =========================================================================
    /**
     * @brief Sets the profile type for this category (affects entity creation).
     * @param Type The entity type to associate.
     */
    void setProfileType(Constants::EntityType Type);

    /**
     * @brief Serializes the profile category (including children) to a JSON object.
     * @return QJsonObject representing the profile's state.
     */
    QJsonObject toJson();

    /**
     * @brief Deserializes the profile category from a JSON object.
     * @param obj JSON object containing profile data.
     */
    void fromJson(const QJsonObject& obj);
};

#endif // PROFILECATEGAORY_H
