// GlobalRegistry.h
#ifndef GLOBALREGISTRY_H
#define GLOBALREGISTRY_H

#include <unordered_map>

class Hierarchy;
class ProfileCategaory;
class Folder;
class Entity;

class GlobalRegistry {
public:
    static std::unordered_map<ProfileCategaory*, Hierarchy*> profileToHierarchyMap;
    static std::unordered_map<Folder*, Hierarchy*> folderToHierarchyMap;
    static std::unordered_map<Entity*, Hierarchy*> entityToHierarchyMap;

    static void registerProfile(ProfileCategaory* profile, Hierarchy* hierarchy) {
        profileToHierarchyMap[profile] = hierarchy;
    }

    static Hierarchy* getParentHierarchy(ProfileCategaory* profile) {
        if (profileToHierarchyMap.count(profile)) {
            return profileToHierarchyMap[profile];
        }
        return nullptr;
    }

    ///////////////////////
    static void registerFolder(Folder* folder, Hierarchy* hierarchy) {
        folderToHierarchyMap[folder] = hierarchy;
    }

    static Hierarchy* getParentHierarchy(Folder* folder) {
        if (folderToHierarchyMap.count(folder)) {
            return folderToHierarchyMap[folder];
        }
        return nullptr;
    }

    ///////////////////////
    static void registerEntity(Entity* entity, Hierarchy* hierarchy) {
        entityToHierarchyMap[entity] = hierarchy;
    }

    static Hierarchy* getParentHierarchy(Entity* entity) {
        if (entityToHierarchyMap.count(entity)) {
            return entityToHierarchyMap[entity];
        }
        return nullptr;
    }
};

#endif // GLOBALREGISTRY_H
