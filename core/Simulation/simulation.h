#ifndef SIMULATION_H
#define SIMULATION_H

// #define BT_NO_SIMD
// #define BT_NO_SIMD_OPERATOR_OVERLOADS
#include <btBulletDynamicsCommon.h> // 🆕 Bullet include

#include <qobject.h>
#include "core/structure/entity.h"
#include "qtimer.h"
#include <QElapsedTimer>
#include <core/Struct/vector.h>
#include <core/Components/transform.h>
#include <core/Components/collider.h>
#include <core/Components/rigidbody.h>


struct PhysicsComponent {
    std::string name;
    Transform *transform;
    DynamicModel *dynamicModel;
    Rigidbody *rigidbody;
    Collider *collider;
};

class Simulation: public QObject
{
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
    void entityAdded(QString parentID,Entity* entity);
    void entityRemoved(QString ID);
    void entityUpdate(QString ID);

signals:
    void Awake();
    void Begin();
    void Update();
    void Physics();
    void HierarchyUpdate();
    void Render(float deltaTime);

private:
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;
    std::unordered_map<std::string, btRigidBody*> bulletBodies; // 🆕 Entity ID -> Bullet Body map

private:
    QTimer *updateTimer;
    float deltaTime;
    float speed;
    QElapsedTimer* elapsedTimer;
    qint64 lastTime;
};

#endif // SIMULATION_H
