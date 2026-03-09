
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

#include "core/Dynamics/Model/aircraft.h"
#include "simulation_state.h"
// #include <core/SharedMemory/sharedmemory.h> //Shared Memory By Himanshu

struct PhysicsComponent {
    std::string name;
    Entity *base = nullptr;
    Platform *platform = nullptr;
    Specialzone *zone = nullptr;
    Transform *transform = nullptr;
    DynamicModel *dynamicModel = nullptr;
    Rigidbody *rigidbody = nullptr;
    Collider *collider = nullptr;
    Aircraft *aircraft = nullptr;
};

class Simulation : public QObject {
    Q_OBJECT
public:
    Simulation();
    ~Simulation();

    Recorder* recorder = nullptr;
    Recording* recording = nullptr;

    /* Shared Memory By Himanshu */
    /* For Shared Memory Mode */
    SimulationStateNS::State Status = SimulationStateNS::STOP;
    /* Shared Memory By Himanshu */

    std::unordered_map<std::string, PhysicsComponent> physicsComponent;

    int SimulationFrameRate;
    int PhysicsUpdateFrameRate;
    int UIUpdateFrameRate;
    Vector* Gravity;

    static inline bool isPlay;
    static inline float simulationTime;
    bool complete;

    void start();
    void pause();
    void stop();
    void nextStep();
    void setSpeed(float value);

    void toJson();
    void fromJson();

    int getRate() const;
    void calculatePhysics();
    void updateDynamics(float dt,PhysicsComponent *comp);
    void calculateDynamic(float dt);

    void replay(); // newly added overload
    void replay(const QVector<QJsonObject>& recordedFrames);
    void setNetworkManager(NetworkManager* nm);//by Aman
    MessageQueue<TransformUpdate> incomingTransforms;
    void applyPendingNetworkUpdates();//by Aman
    void enqueueTransformUpdate(const TransformUpdate& msg);
private:
    void frame();
    int rate = 1;

public slots:
    void init();
    void ReInit();
    void Reset();
    void startf();
    void pausef();
    void entityAdded(QString parentID, Entity* entity);
    void entityRemoved(QString ID);
    void entityUpdate(QString ID);
    void handleReplayFrame(const QJsonObject& frame);
    void setFps(int value);
    void timeJump(int newTime);


signals:
    void Awake();
    void Begin();
    void Update();
    void Physics();
    void HierarchyUpdate();
    void Render(float deltaTime);
    void speedUpdated(float speed);
    void sendMode(SimulationStateNS::State status);//Shared Memory By Himanshu
    void sendUpdate();//Recording By Himanshu



private:
    NetworkManager *networkManager = nullptr;
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;
    std::unordered_map<std::string, btRigidBody*> bulletBodies;

    QTimer *updateTimer;
    float deltaTime;
    float speed = 1.0f;
    QElapsedTimer* elapsedTimer;
    qint64 lastTime;

    bool isReplaying = false;
    int replayIndex = 0;
    QVector<QJsonObject> replayFrames;

    int simMin = 0;
    int simCount = 50;
};

#endif // SIMULATION_H
