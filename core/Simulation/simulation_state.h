#ifndef SIMULATION_STATE_H
#define SIMULATION_STATE_H

#include <QObject>

#define SHM_PATH "/tmp/tdf.bin"
#define SHM_PAYLOAD 15728640
#define MAX_ENTITY 5000

// Semaphore
#define SHM_NAME "/qt_shared_mem"
#define SEM_NAME "/qt_shm_sem"
#define SHM_SIZE 15728640


namespace SimulationStateNS {

Q_NAMESPACE

enum State {
    INITIALIZE,
    REINITIALIZE,
    DEINITIALIZE,
    START,
    UPDATE,
    PAUSE,
    STOP
};
Q_ENUM_NS(State)

}
Q_DECLARE_METATYPE(SimulationStateNS::State)


namespace SimTypeOfUpdates {

Q_NAMESPACE
//Definig essential enums
enum TypeOfUpdate{
    dynamicDynamic,
    dynamicStatic,
    trajectory,
    sensor,
    entity,
};
Q_ENUM_NS(TypeOfUpdate)

}
Q_DECLARE_METATYPE(SimTypeOfUpdates::TypeOfUpdate)


namespace SimUpdateTypes {

Q_NAMESPACE
enum UpdateTypes{
    CREATE,
    UPDATE,
    DELETE
};
Q_ENUM_NS(UpdateTypes)

}
Q_DECLARE_METATYPE(SimUpdateTypes::UpdateTypes)

#endif
