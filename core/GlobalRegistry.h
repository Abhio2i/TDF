// =============================================================================
// FILE:        GlobalRegistry.h
// MODULE:      Global Registry for Hierarchy Lookup
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the GlobalRegistry class, which provides static maps
//              to associate simulation objects (ProfileCategory, Folder,
//              Entity, Component) with their owning Hierarchy instance.
//              Enables reverse lookup to find the parent hierarchy of any
//              registered object.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef GLOBALREGISTRY_H
#define GLOBALREGISTRY_H

#include <unordered_map>

class Hierarchy;
class ProfileCategaory;
class Folder;
class Entity;
class Component;

// =============================================================================
// CLASS: GlobalRegistry
//
// DESCRIPTION: Static registry that maintains bidirectional (or forward-only)
//              mappings from simulation objects to the Hierarchy that owns them.
//              Provides registration methods and lookup functions for each
//              object type.
// =============================================================================
class GlobalRegistry {
public:
    // =========================================================================
    // SECTION: Static Maps
    // DESCRIPTION: Maps from object pointers to their parent Hierarchy.
    // =========================================================================
    static std::unordered_map<ProfileCategaory*, Hierarchy*> profileToHierarchyMap; //!< Profile to hierarchy mapping
    static std::unordered_map<Folder*, Hierarchy*> folderToHierarchyMap;           //!< Folder to hierarchy mapping
    static std::unordered_map<Entity*, Hierarchy*> entityToHierarchyMap;           //!< Entity to hierarchy mapping
    static std::unordered_map<Component*, Hierarchy*> componentToHierarchyMap;     //!< Component to hierarchy mapping

    // -------------------------------------------------------------------------
    // SECTION: Profile Registration & Lookup
    // -------------------------------------------------------------------------
    static void registerProfile(ProfileCategaory* profile, Hierarchy* hierarchy) {
        profileToHierarchyMap[profile] = hierarchy;
    }

    static Hierarchy* getParentHierarchy(ProfileCategaory* profile) {
        if (profileToHierarchyMap.count(profile)) {
            return profileToHierarchyMap[profile];
        }
        return nullptr;
    }

    // -------------------------------------------------------------------------
    // SECTION: Folder Registration & Lookup
    // -------------------------------------------------------------------------
    static void registerFolder(Folder* folder, Hierarchy* hierarchy) {
        folderToHierarchyMap[folder] = hierarchy;
    }

    static Hierarchy* getParentHierarchy(Folder* folder) {
        if (folderToHierarchyMap.count(folder)) {
            return folderToHierarchyMap[folder];
        }
        return nullptr;
    }

    // -------------------------------------------------------------------------
    // SECTION: Entity Registration & Lookup
    // -------------------------------------------------------------------------
    static void registerEntity(Entity* entity, Hierarchy* hierarchy) {
        entityToHierarchyMap[entity] = hierarchy;
    }

    static Hierarchy* getParentHierarchy(Entity* entity) {
        if (entityToHierarchyMap.count(entity)) {
            return entityToHierarchyMap[entity];
        }
        return nullptr;
    }

    // -------------------------------------------------------------------------
    // SECTION: Component Registration & Lookup
    // -------------------------------------------------------------------------
    static void registerComponent(Component* component, Hierarchy* hierarchy) {
        componentToHierarchyMap[component] = hierarchy;
    }

    static Hierarchy* getParentHierarchy(Component* component) {
        if (componentToHierarchyMap.count(component)) {
            return componentToHierarchyMap[component];
        }
        return nullptr;
    }
};

#endif // GLOBALREGISTRY_H
