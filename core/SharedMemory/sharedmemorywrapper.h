// =============================================================================
// FILE:        sharedmemorywrapper.h
// MODULE:      Shared Memory Integration
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the SharedComponent struct and SharedMemoryWrapper class,
//              which bridge simulation entities to a shared memory interface
//              (likely for inter-process communication). Manages entity
//              lifecycle, updates transform, dynamic, sensor, and trajectory
//              data to shared memory structures.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef SHAREDMEMORYWRAPPER_H
#define SHAREDMEMORYWRAPPER_H

#include "core/Hierarchy/EntityProfiles/specialzone.h"
#include "core/Hierarchy/entity.h"
#include "qobject.h"
#include "sharedMemory.h"

// =============================================================================
// STRUCT: SharedComponent
// DESCRIPTION: Aggregates pointers to various entity components for transfer
//              to shared memory. Each component type is optional (nullptr if
//              not applicable).
// =============================================================================
struct SharedComponent {
    std::string name;                       //!< Entity name/identifier
    Entity *base = nullptr;                 //!< Base entity pointer
    Platform *platform = nullptr;           //!< Platform-specific data
    Specialzone *zone = nullptr;            //!< Special zone data (if applicable)
    Transform *transform = nullptr;         //!< Spatial transform
    DynamicModel *dynamicModel = nullptr;   //!< Motion dynamics
    Rigidbody *rigidbody = nullptr;         //!< Physics rigidbody
    Collider *collider = nullptr;           //!< Collision shape
    Trajectory* trajectory;                 //!< Path following data
};

// =============================================================================
// CLASS: SharedMemoryWrapper
//
// DESCRIPTION: Wraps the shared memory interface, listening to entity
//              addition/removal signals and simulation updates. Copies
//              relevant component data into shared memory structures for
//              external processes (e.g., visualisation, logging).
// =============================================================================
class SharedMemoryWrapper: public QObject
{
    Q_OBJECT

public:
    SharedMemoryWrapper();
    ~SharedMemoryWrapper();

public slots:
    void entityAdded(QString parentID, Entity* entity);   //!< Adds entity to shared memory
    void entityRemoved(QString ID);                       //!< Removes entity from shared memory
    void SimulationUpdate(float deltaTime);               //!< Updates shared memory with current simulation state

private:
    // Data copy helpers
    void updateTransformData(SharedComponent comp, SharedStructs::Entity* entity);
    void updateDynamicData(SharedComponent comp, SharedStructs::Entity* entity);
    void updateSensorData(SharedComponent comp, SharedStructs::Entity* entity);
    void updateTrajectoryData(SharedComponent comp, SharedStructs::Entity* entity);

private:
    EntityHandler* handler;                                     //!< Entity handler (unclear, likely from sharedMemory.h)
    SharedMemory shm;                                           //!< Shared memory interface object
    std::unordered_map<std::string, SharedComponent> sharedComponent; //!< Map of entity ID to its shared component data
};

#endif // SHAREDMEMORYWRAPPER_H
