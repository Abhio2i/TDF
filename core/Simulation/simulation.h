// =============================================================================
// FILE:        simulation.h
// MODULE:      Simulation Engine
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Simulation class, the core simulation engine.
//              Manages physics (Bullet), dynamics, recording, replay, network
//              updates, and entity lifecycle. Maintains simulation time,
//              frame rates, and integrates with canvas for visual effects.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef SIMULATION_H
#define SIMULATION_H

#include <btBulletDynamicsCommon.h>
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <core/Hierarchy/entity.h>
#include <core/Hierarchy/EntityProfiles/platform.h>
#include <core/Hierarchy/Struct/vector.h>
#include <core/Hierarchy/Components/transform.h>
#include <core/Hierarchy/Components/collider.h>
#include <core/Hierarchy/Components/rigidbody.h>
#include <core/Hierarchy/Components/dynamicmodel.h> // Added for DynamicModel
#include <unordered_map>
#include <core/Recorder/recorder.h>
#include <core/Network/networkmanager.h>
#include <core/Network/libs/MessageBus.h>
#include <core/Network/libs/TransformUpdate.h>
#include "core/DISPlugin//SimulationPlugin.h"

#include "core/Dynamics/Model/aircraft.h"
#include "simulation_state.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"

// Forward declare CanvasWidget to avoid heavy include in header
class CanvasWidget;
// #include <core/SharedMemory/sharedmemory.h> //Shared Memory By Himanshu

// =============================================================================
// STRUCT: PhysicsComponent
// DESCRIPTION: Aggregates all physics-related components for an entity.
// =============================================================================
struct PhysicsComponent {
    std::string name;                       //!< Entity name/ID
    Entity *base = nullptr;                 //!< Base entity
    Platform *platform = nullptr;           //!< Platform data
    Weapon *weapon = nullptr;               //!< Weapon data (if any)
    Specialzone *zone = nullptr;            //!< Special zone data
    Transform *transform = nullptr;         //!< Spatial transform
    DynamicModel *dynamicModel = nullptr;   //!< Motion dynamics
    Rigidbody *rigidbody = nullptr;         //!< Physics rigidbody
    Collider *collider = nullptr;           //!< Collision shape
    Aircraft *aircraft = nullptr;           //!< Aircraft dynamics model
};

// =============================================================================
// CLASS: Simulation
//
// DESCRIPTION: Central simulation controller. Manages physics world (Bullet),
//              dynamics updates, recording/replay, network transform sync,
//              and entity state. Provides start/pause/stop/step controls.
// =============================================================================
class Simulation : public QObject {
    Q_OBJECT
public:
    Simulation();
    ~Simulation();

    // =========================================================================
    // SECTION: Recording
    // DESCRIPTION: Recorder and current recording instance.
    // =========================================================================
    Recorder* recorder = nullptr;       //!< Simulation recorder
    Recording* recording = nullptr;     //!< Active recording session

    // =========================================================================
    // SECTION: Shared Memory Mode
    // DESCRIPTION: State flag for shared memory integration.
    // =========================================================================
    /* Shared Memory By Himanshu */
    /* For Shared Memory Mode */
    SimulationStateNS::State Status = SimulationStateNS::STOP;   //!< Current simulation state
    /* Shared Memory By Himanshu */

    // =========================================================================
    // SECTION: Physics Components Map
    // DESCRIPTION: Maps entity IDs to their aggregated physics data.
    // =========================================================================
    std::unordered_map<std::string, PhysicsComponent> physicsComponent;

    // =========================================================================
    // SECTION: Frame Rate Settings
    // DESCRIPTION: Target frame rates for simulation subsystems.
    // =========================================================================
    int SimulationFrameRate;    //!< Target simulation FPS
    int PhysicsUpdateFrameRate; //!< Target physics update FPS
    int UIUpdateFrameRate;      //!< Target UI update FPS
    Vector* Gravity;            //!< Gravity vector (m/s²)

    // =========================================================================
    // SECTION: Global State
    // DESCRIPTION: Play flag and simulation time.
    // =========================================================================
    static inline bool isPlay;              //!< Whether simulation is playing
    static inline float simulationTime;     //!< Current simulation time (seconds)
    bool complete;                          //!< Flag indicating simulation completion

    void start();           //!< Starts or resumes simulation
    void pause();           //!< Pauses simulation
    void stop();            //!< Stops simulation and resets time
    void nextStep();        //!< Advances simulation by one step (when paused)
    void setSpeed(float value); //!< Sets simulation speed multiplier

    void toJson();          //!< Serialises simulation config to JSON
    void fromJson();        //!< Deserialises simulation config from JSON

    int getRate() const;    //!< Returns current simulation rate multiplier

    void calculatePhysics();            //!< Performs physics calculations
    void updateDynamics(float dt, PhysicsComponent *comp); //!< Updates dynamics for a component
    void calculateDynamic(float dt);    //!< Updates all dynamics

    void replay();                              //!< Starts replay from recorded data
    void replay(const QVector<QJsonObject>& recordedFrames); //!< Replays given frames
    void setNetworkManager(NetworkManager* nm); //!< Injects network manager
    MessageQueue<TransformUpdate> incomingTransforms; //!< Queue of pending transform updates
    void applyPendingNetworkUpdates();          //!< Processes queued network updates
    void enqueueTransformUpdate(const TransformUpdate& msg); //!< Adds transform update to queue
    void registerPlugin(SimulationPlugin* plugin);
    void unregisterPlugin(SimulationPlugin* plugin);
    // Set canvas so weapons can display blast effects on detonation
    void setCanvas(CanvasWidget* canvas) { m_canvas = canvas; }

private:
    void frame();           //!< Called each simulation frame
    int rate = 1;           //!< Internal rate multiplier

public slots:
    void init();            //!< Initialises simulation
    void ReInit();          //!< Re-initialises simulation
    void Reset();           //!< Resets simulation state
    void startf();          //!< Starts (slot version)
    void pausef();          //!< Pauses (slot version)
    void entityAdded(QString parentID, Entity* entity);     //!< Adds entity to physics world
    void entityRemoved(QString ID);                         //!< Removes entity from physics world
    void entityUpdate(QString ID);                          //!< Updates entity state
    void handleReplayFrame(const QJsonObject& frame);       //!< Processes a replay frame
    void setFps(int value);                                 //!< Sets target FPS
    void timeJump(int newTime);                             //!< Jumps simulation time

signals:
    void Awake();                       //!< Emitted on awake
    void Begin();                       //!< Emitted on begin/start
    void Update();                      //!< Emitted each simulation update
    void Physics();                     //!< Emitted before physics step
    void HierarchyUpdate();             //!< Emitted when hierarchy changes
    void Render(float deltaTime);       //!< Emitted for rendering with delta time
    void speedUpdated(float speed);     //!< Emitted when speed multiplier changes
    void sendMode(SimulationStateNS::State status); //!< Sends mode for shared memory
    void sendUpdate();                  //!< Emitted for recording updates

private:
    NetworkManager *networkManager = nullptr;   //!< Network manager for sync
    CanvasWidget   *m_canvas       = nullptr;   //!< Canvas for weapon blast effects

    // Bullet Physics world components
    btBroadphaseInterface* broadphase;                      //!< Broadphase collision detection
    btDefaultCollisionConfiguration* collisionConfiguration; //!< Collision configuration
    btCollisionDispatcher* dispatcher;                      //!< Collision dispatcher
    btSequentialImpulseConstraintSolver* solver;            //!< Constraint solver
    btDiscreteDynamicsWorld* dynamicsWorld;                 //!< Bullet dynamics world
    std::unordered_map<std::string, btRigidBody*> bulletBodies; //!< Map of Bullet rigid bodies

    QTimer *updateTimer;        //!< Timer for frame updates
    float deltaTime;            //!< Time since last frame
    float speed = 1.0f;         //!< Speed multiplier
    QElapsedTimer* elapsedTimer; //!< High-resolution timer
    qint64 lastTime;            //!< Last timestamp

    bool isReplaying = false;           //!< Whether replay is active
    int replayIndex = 0;                //!< Current replay frame index
    QVector<QJsonObject> replayFrames;  //!< Stored replay frames
    QList<SimulationPlugin*> m_plugins;

    int simMin = 0;         //!< Simulation minimum (for time jump)
    int simCount = 50;      //!< Simulation count (for time jump)
};

#endif // SIMULATION_H
