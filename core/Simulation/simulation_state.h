// =============================================================================
// FILE:        simulation_state.h
// MODULE:      Simulation State Definitions
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines shared memory paths, sizes, semaphore names, and
//              enumeration namespaces for simulation state, update types,
//              and entity update categories. Used for inter-process
//              communication and state synchronisation.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef SIMULATION_STATE_H
#define SIMULATION_STATE_H

#include <QObject>

// =============================================================================
// SECTION: Shared Memory Constants
// DESCRIPTION: Path, size, and naming for shared memory and semaphores.
// =============================================================================

#define SHM_PATH "/tmp/tdf.bin"            //!< File path for shared memory mapping
#define SHM_PAYLOAD 15728640               //!< Shared memory payload size (bytes)
#define MAX_ENTITY 5000                    //!< Maximum number of entities in shared memory

// Semaphore
#define SHM_NAME "/qt_shared_mem"          //!< Shared memory object name
#define SEM_NAME "/qt_shm_sem"             //!< Semaphore name for synchronisation
#define SHM_SIZE 15728640                  //!< Total shared memory size (bytes)

// =============================================================================
// NAMESPACE: SimulationStateNS
// DESCRIPTION: Main simulation control states.
// =============================================================================
namespace SimulationStateNS {

Q_NAMESPACE

enum State {
    INITIALIZE,     //!< Initialise simulation components
    REINITIALIZE,   //!< Re-initialise (reset)
    DEINITIALIZE,   //!< Clean up / shutdown
    START,          //!< Start simulation
    UPDATE,         //!< Update simulation tick
    PAUSE,          //!< Pause simulation
    STOP            //!< Stop simulation
};
Q_ENUM_NS(State)

}
Q_DECLARE_METATYPE(SimulationStateNS::State)

// =============================================================================
// NAMESPACE: SimTypeOfUpdates
// DESCRIPTION: Categories of simulation updates for shared memory.
// =============================================================================
namespace SimTypeOfUpdates {

Q_NAMESPACE
//Definig essential enums
enum TypeOfUpdate{
    dynamicDynamic,     //!< Dynamic entity vs dynamic entity update
    dynamicStatic,      //!< Dynamic entity vs static entity update
    trajectory,         //!< Trajectory/path update
    sensor,             //!< Sensor data update
    entity,             //!< General entity update
};
Q_ENUM_NS(TypeOfUpdate)

}
Q_DECLARE_METATYPE(SimTypeOfUpdates::TypeOfUpdate)

// =============================================================================
// NAMESPACE: SimUpdateTypes
// DESCRIPTION: Operation types for entity changes in shared memory.
// =============================================================================
namespace SimUpdateTypes {

Q_NAMESPACE
enum UpdateTypes{
    CREATE,     //!< Entity created
    UPDATE,     //!< Entity updated
    DELETE      //!< Entity deleted
};
Q_ENUM_NS(UpdateTypes)

}
Q_DECLARE_METATYPE(SimUpdateTypes::UpdateTypes)

#endif // SIMULATION_STATE_H
