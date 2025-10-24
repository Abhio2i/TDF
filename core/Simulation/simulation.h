
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

struct PhysicsComponent {
    std::string name;
    Platform *entity;
    Transform *transform;
    DynamicModel *dynamicModel;
    Rigidbody *rigidbody;
    Collider *collider;
};

class Simulation : public QObject {
    Q_OBJECT
public:
    Simulation();
    ~Simulation();

    Recorder* recorder = nullptr;

    std::unordered_map<std::string, PhysicsComponent> physicsComponent;

    int SimulationFrameRate;
    int PhysicsUpdateFrameRate;
    int UIUpdateFrameRate;
    Vector* Gravity;

    bool isPlay;
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

    void replay(); // newly added overload
    void replay(const QVector<QJsonObject>& recordedFrames);

private:
    void frame();
    int rate = 1;

public slots:
    void entityAdded(QString parentID, Entity* entity);
    void entityRemoved(QString ID);
    void entityUpdate(QString ID);
    void handleReplayFrame(const QJsonObject& frame);

signals:
    void Awake();
    void Begin();
    void Update();
    void Physics();
    void HierarchyUpdate();
    void Render(float deltaTime);
    void speedUpdated(float speed);

private:
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;
    std::unordered_map<std::string, btRigidBody*> bulletBodies;

    QTimer *updateTimer;
    float deltaTime;
    float speed;
    QElapsedTimer* elapsedTimer;
    qint64 lastTime;

    bool isReplaying = false;
    int replayIndex = 0;
    QVector<QJsonObject> replayFrames;
};

#endif // SIMULATION_H
