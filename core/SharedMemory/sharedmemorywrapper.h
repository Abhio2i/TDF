#ifndef SHAREDMEMORYWRAPPER_H
#define SHAREDMEMORYWRAPPER_H

#include "core/Hierarchy/EntityProfiles/specialzone.h"
#include "core/Hierarchy/entity.h"
#include "qobject.h"
#include "sharedMemory.h"


struct SharedComponent {
    std::string name;
    Entity *base = nullptr;
    Platform *platform = nullptr;
    Specialzone *zone = nullptr;
    Transform *transform = nullptr;
    DynamicModel *dynamicModel = nullptr;
    Rigidbody *rigidbody = nullptr;
    Collider *collider = nullptr;
    Trajectory* trajectory;
};
class SharedMemoryWrapper: public QObject
{
    Q_OBJECT

public:
    SharedMemoryWrapper();
    ~SharedMemoryWrapper();


public slots:
    void entityAdded(QString parentID, Entity* entity);
    void entityRemoved(QString ID);
    void SimulationUpdate(float deltaTime);

private:
    void updateTransformData(SharedComponent comp,SharedStructs::Entity* entity);
    void updateDynamicData(SharedComponent comp,SharedStructs::Entity* entity);
    void updateSensorData(SharedComponent comp,SharedStructs::Entity* entity);
    void updateTrajectoryData(SharedComponent comp,SharedStructs::Entity* entity);

private:
    EntityHandler* handler;
    SharedMemory shm;
    std::unordered_map<std::string, SharedComponent> sharedComponent;

};

#endif // SHAREDMEMORYWRAPPER_H
