
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

struct PhysicsComponent {
    std::string name;
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

    void calculatePhysics();

private:
    void frame();

public slots:
    void entityAdded(QString parentID, Entity* entity);
    void entityRemoved(QString ID);
    void entityUpdate(QString ID);

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
};

#endif // SIMULATION_H
