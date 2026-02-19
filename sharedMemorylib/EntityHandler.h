
#ifndef EntityHandler_H
#define EntityHandler_H

#include "struct.h"
#include <string>
#include <unordered_map>
#include <mutex> 
class EntityHandler {
public:
    EntityHandler();        
    ~EntityHandler();
    
    void setSharedData(SharedStructs::SharedData* data);
    
    // Read operations
    SharedStructs::Entity* getEntityByIndex(int index);
    SharedStructs::Entity* getEntityByID(const std::string& id);

    // Write operations (Protected by Mutex)
    void addEntity(const SharedStructs::Entity& entity);
    void removeEntityByID(const std::string& id);
    
    // Maintenance
    void updateEntityIDList();
    int getEntityCount() const;

private:
    SharedStructs::SharedData* shareddata;
    std::unordered_map<std::string, int> idToIndexMap; 
    std::mutex handlerMutex; // Lock for thread safety
};

#endif