#include "GlobalRegistry.h"

// Yeh line zaroori hai linker ke liye
std::unordered_map<ProfileCategaory*, Hierarchy*> GlobalRegistry::profileToHierarchyMap;
std::unordered_map<Folder*, Hierarchy*> GlobalRegistry::folderToHierarchyMap;
std::unordered_map<Entity*, Hierarchy*> GlobalRegistry::entityToHierarchyMap;
