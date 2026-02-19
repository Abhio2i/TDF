
#include "EntityHandler.h"
#include <iostream>

// Constructor: Initialize shared data pointer to nullptr
EntityHandler::EntityHandler() : shareddata(nullptr) {
    std::cout << "EntityHandler object created." << std::endl;
}

// Destructor: Clean up resources
EntityHandler::~EntityHandler() {
    shareddata = nullptr;
    std::cout << "EntityHandler object destroyed." << std::endl;
}

// Set shared data and rebuild the ID-to-index map for consistency
void EntityHandler::setSharedData(SharedStructs::SharedData* data) {
    std::lock_guard<std::mutex> lock(handlerMutex); // Protect shared data access
    shareddata = data;
    
    // Rebuild the map when new data is set
    if (shareddata != nullptr) {
        idToIndexMap.clear();
        for (int i = 0; i < shareddata->entityCount; ++i) {
            char idBuffer[30];
            shareddata->entities[i].getID(idBuffer);
            idToIndexMap[std::string(idBuffer)] = i;
        }
    }
}

int EntityHandler::getEntityCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(handlerMutex)); // Protect shared data access
    if (shareddata != nullptr) {
        return shareddata->entityCount;
    }
    return 0;
}

// Retrieve entity by array index
SharedStructs::Entity* EntityHandler::getEntityByIndex(int index) {
    if (shareddata != nullptr && index >= 0 && index < shareddata->entityCount) {
        return &(shareddata->entities[index]);
    }
    return nullptr;
}

// Retrieve entity by ID string
SharedStructs::Entity* EntityHandler::getEntityByID(const std::string& id) {
    std::lock_guard<std::mutex> lock(handlerMutex); // Protect map access
    
    if (shareddata != nullptr) {
        auto it = idToIndexMap.find(id);
        if (it != idToIndexMap.end()) {
            int index = it->second;
            if (index >= 0 && index < shareddata->entityCount) {
                return &(shareddata->entities[index]);  
            }
        }
    }
    return nullptr;
}

// Refresh the ID-to-index mapping from current entities
void EntityHandler::updateEntityIDList() {
    std::lock_guard<std::mutex> lock(handlerMutex); // Synchronize map update
    idToIndexMap.clear();
    
    if (shareddata != nullptr) {
        for (int i = 0; i < shareddata->entityCount; ++i) {
            char idBuffer[30];
            shareddata->entities[i].getID(idBuffer);
            idToIndexMap[std::string(idBuffer)] = i;
        }
    }
}

// Add new entity to shared memory and update ID map
void EntityHandler::addEntity(const SharedStructs::Entity& entity) {
    std::lock_guard<std::mutex> lock(handlerMutex); // Critical section protection

    if (shareddata != nullptr && shareddata->entityCount < 1000) {
        // Copy entity to shared memory at current count position
        shareddata->entities[shareddata->entityCount] = entity;
        
        // Extract entity ID and update map
        char idBuffer[30];
        entity.getID(idBuffer);
        std::string idStr(idBuffer);
        idToIndexMap[idStr] = shareddata->entityCount;
        
        // Increment entity count
        shareddata->entityCount++;
    }
}

// Remove entity by ID using swap-and-pop strategy
void EntityHandler::removeEntityByID(const std::string& id) {
    std::lock_guard<std::mutex> lock(handlerMutex); // Critical section protection

    if (shareddata != nullptr) {
        auto it = idToIndexMap.find(id);
        if (it != idToIndexMap.end()) {
            int indexToRemove = it->second;
            int lastIndex = shareddata->entityCount - 1;

            // If removing entity is not the last one, swap with last entity
            if (indexToRemove != lastIndex) {
                shareddata->entities[indexToRemove] = shareddata->entities[lastIndex];
                
                // Update map for the moved entity
                char movedEntityID[30];
                shareddata->entities[indexToRemove].getID(movedEntityID);
                std::string movedIDStr(movedEntityID);
                idToIndexMap[movedIDStr] = indexToRemove;
            }

            // Remove deleted entity from map
            idToIndexMap.erase(it);

            // Decrement entity count
            shareddata->entityCount--;
        }
    }
}
