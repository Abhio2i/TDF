#include "simulation.h"
#include <algorithm>
#include <core/Debug/console.h>
#include "core/Debug/profiler.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include <QJsonObject>
#include <QtMath>
#include <GUI/mainwindow.h>
Simulation::Simulation() {
    qRegisterMetaType<SimulationStateNS::State>("SimulationStateNS::State");
    qRegisterMetaType<SimTypeOfUpdates::TypeOfUpdate>("SimTypeOfUpdates::TypeOfUpdate");
    qRegisterMetaType<SimUpdateTypes::UpdateTypes>("SimUpdateTypes::UpdateTypes");
}

void Simulation::init(){
    physicsComponent.clear();
    simulationTime = 0;
    updateTimer = new QTimer(this);
    elapsedTimer = new QElapsedTimer();
    elapsedTimer->start();
    lastTime = elapsedTimer->elapsed();
    SimulationFrameRate = MainWindow::scenarioconfig->getSavedFPS();
    PhysicsUpdateFrameRate = MainWindow::scenarioconfig->getSavedFPS();
    UIUpdateFrameRate = 30;
    Gravity = new Vector(0, 0, -3.81f);
    speed = 1;
    isPlay = false;
    complete = false;
    isReplaying = false;
    replayIndex = 0;

    connect(updateTimer, &QTimer::timeout, this, [=]() {
        if (isReplaying) {
            if (replayIndex < replayFrames.size()) {
                handleReplayFrame(replayFrames[replayIndex]);
                replayIndex++;
            } else {
                isReplaying = false;
                replayIndex = 0;
                updateTimer->stop();
                Console::log("Replay finished");
            }
        } else {
            QElapsedTimer timer;
            timer.start();  // Start measuring
            frame();
            qint64 elapsedMs = timer.elapsed();
            Profiler::currentFrame->physicsTime = elapsedMs;
            Profiler::currentFrame->analyze();
            // qDebug() << "Physics execution time:" << elapsedMs << "ms";
        }
    });

    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(Gravity->x, Gravity->y, Gravity->z));

    emit Awake();
    emit Begin();
}

void Simulation::Reset(){
    physicsComponent.clear();
    simulationTime = 0;
}

void Simulation::ReInit(){
    SimulationFrameRate = MainWindow::scenarioconfig->getSavedFPS();
    PhysicsUpdateFrameRate = MainWindow::scenarioconfig->getSavedFPS();
    UIUpdateFrameRate = 30;
    /* Shared Memory By Himanshu */
    Status = SimulationStateNS::REINITIALIZE;
    emit sendMode(Status);
    /* Shared Memory By Himanshu */
}

Simulation::~Simulation() {
    for (auto& [id, body] : bulletBodies) {
        dynamicsWorld->removeRigidBody(body);
        delete body->getMotionState();
        delete body->getCollisionShape();
        delete body;
    }
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
    delete Gravity;
    //added by Aman to avoid crash during stop
    if (updateTimer) updateTimer->stop();
}

void Simulation::setFps(int value){
    SimulationFrameRate = value;
    PhysicsUpdateFrameRate = value;
    UIUpdateFrameRate = 30;
    updateTimer->setInterval(1000/value);
}

void Simulation::frame() {
    qint64 currentTime = elapsedTimer->elapsed();
    deltaTime = (currentTime - lastTime) / 1000.0f;
    if(!isPlay){
        deltaTime = 0.02f;
    }
    lastTime = currentTime;
    applyPendingNetworkUpdates();//by Aman to apply recived update via network
    calculatePhysics();

    QJsonObject frameData;
    QJsonArray entitiesArray;
    for (const auto& [id, comp] : physicsComponent) {
        if (!comp.transform) continue;

        QJsonObject entityFrame;
        entityFrame["id"] = QString::fromStdString(id);
        entityFrame["position"] = QJsonObject{
            {"x", comp.transform->matrix->translation().x()},
            {"y", comp.transform->matrix->translation().y()},
            {"z", comp.transform->matrix->translation().z()}
        };
        entityFrame["rotation"] = QJsonObject{
            {"x", comp.transform->matrix->rotation().toEulerAngles().x()},
            {"y", comp.transform->matrix->rotation().toEulerAngles().y()},
            {"z", comp.transform->matrix->rotation().toEulerAngles().z()}
        };

        entitiesArray.append(entityFrame);
    }

    frameData["entities"] = entitiesArray;
    frameData["timestamp"] = QDateTime::currentMSecsSinceEpoch();


    QTimer::singleShot(0, this, [=]() {
        emit Update();
        emit Render(deltaTime * speed);
        /* Shared Memory By Himanshu */
        Status = SimulationStateNS::UPDATE;
        emit sendMode(Status);
        /* Shared Memory By Himanshu */
    });
    //isPlay = true;
}

void Simulation::start() {
    QMetaObject::invokeMethod(this, "startf", Qt::QueuedConnection);
}

void Simulation::startf() {
    lastTime = elapsedTimer->elapsed();
    updateTimer->start(1000 / SimulationFrameRate);
    isPlay = true;
    qDebug()<<"i am working";

    /* Shared Memory By Himanshu */
    Status = SimulationStateNS::START;
    emit sendMode(Status);
    /* Shared Memory By Himanshu */

    for (auto& [id, comp] : physicsComponent) {
        if (comp.platform) comp.platform->Start();
        if (comp.dynamicModel) comp.dynamicModel->start();

        // For weapon entities: (re)start the flight monitor so missiles
        // launch as soon as their entity becomes Active.
        // if (comp.base && comp.base->type == Constants::EntityType::Weapon) {
        //     Weapon* weapon = dynamic_cast<Weapon*>(comp.base);
        //     if (weapon) {
        //         // Ensure canvas is always set before launch
        //         if (m_canvas) weapon->setCanvas(m_canvas);
        //         weapon->startFlightMonitor();
        //     }
        // }
    }

}

void Simulation::pause() {
    QMetaObject::invokeMethod(this, "pausef", Qt::QueuedConnection);
}

void Simulation::pausef() {
    updateTimer->stop();
    isPlay = false;

    /* Shared Memory By Himanshu */
    Status = SimulationStateNS::PAUSE;
    emit sendMode(Status);
    /* Shared Memory By Himanshu */

}

void Simulation::stop() {
    updateTimer->stop();
    isPlay = false;
    complete = true;

    // Stop all weapon flight monitors cleanly
    for (auto& [id, comp] : physicsComponent) {
        if (comp.base && comp.base->type == Constants::EntityType::Weapon) {
            Weapon* weapon = dynamic_cast<Weapon*>(comp.base);
            if (weapon) weapon->stopFlightMonitor();
        }
    }

    /* Shared Memory By Himanshu */
    Status = SimulationStateNS::STOP;
    emit sendMode(Status);
    /* Shared Memory By Himanshu */
}

void Simulation::setSpeed(float value) {
    speed = value;
    for (auto& [id, comp] : physicsComponent) {
        if (comp.dynamicModel) {
            comp.dynamicModel->setMoveSpeed(value);
        }
    }
    emit speedUpdated(value);
}
void Simulation::nextStep() {
    if(!isPlay){
        frame();
    }
}

void Simulation::timeJump(int newTime){
    int sec = newTime-simulationTime;
    sec = sec<0?0:sec;
    qDebug()<<sec;
    float fps = 10.0;
    sec = sec*fps;
    float n = 0;
    float dt= 1.0f/fps;
    for(int i=0;i<sec;i++){
        // simulationTime +=dt;
        deltaTime = dt;
        calculatePhysics();
        // emit Render(dt);
    }
    qDebug()<<n;
    emit Render(dt);
}


int Simulation::getRate() const {
    return this->rate;
}

void Simulation::toJson() {
    QJsonObject obj;
    obj["SimulationFrameRate"] = SimulationFrameRate;
    obj["PhysicsUpdateFrameRate"] = PhysicsUpdateFrameRate;
    obj["UIUpdateFrameRate"] = UIUpdateFrameRate;
    obj["Gravity"] = QJsonObject{
        {"x", Gravity->x},
        {"y", Gravity->y},
        {"z", Gravity->z}
    };
    obj["isPlay"] = isPlay;
    obj["complete"] = complete;
    Console::log("Simulation state serialized to JSON");
}

void Simulation::fromJson() {
    Console::log("Simulation state deserialized from JSON");
}

// Start Replay
void Simulation::replay() {
    if (recorder) {
        //const QVector<QJsonObject>& frames = recorder->getRecordedFrames();
        //replay(frames);
    } else {
        Console::error("No recorder set. Cannot replay.");
    }
}

// Public: Replay with provided frames
void Simulation::replay(const QVector<QJsonObject>& frames) {
    isReplaying = true;
    replayFrames = frames;
    replayIndex = 0;
    updateTimer->start(1000 / SimulationFrameRate);
    Console::log("Replay started with frames");
}

// Slot to handle a replayed frame
void Simulation::handleReplayFrame(const QJsonObject& frame) {
    QJsonArray entities = frame["entities"].toArray();

    for (const QJsonValue& val : entities) {
        QJsonObject entity = val.toObject();
        QString id = entity["id"].toString();

        auto it = physicsComponent.find(id.toStdString());
        if (it != physicsComponent.end()) {
            PhysicsComponent& comp = it->second;
            if (comp.transform) {
                QJsonObject pos = entity["position"].toObject();
                QJsonObject rot = entity["rotation"].toObject();

                comp.transform->matrix->translation().setX(pos["x"].toDouble());
                comp.transform->matrix->translation().setY(pos["y"].toDouble());
                comp.transform->matrix->translation().setZ(pos["z"].toDouble());

                comp.transform->toEulerAngles().setX(rot["x"].toDouble());
                comp.transform->toEulerAngles().setY(rot["y"].toDouble());
                comp.transform->toEulerAngles().setZ(rot["z"].toDouble());
            }
        }
    }

    emit Update();
    emit Render(deltaTime * speed);
}


void Simulation::entityAdded(QString /*parentID*/, Entity* entity) {

    // ✅ Store current state PEHLE
    bool wasPlaying = isPlay;

    // ✅ Pause if running
    if (wasPlaying) {
        pausef();
    }

    if (entity->type == Constants::EntityType::Platform) {
        Platform* platform = dynamic_cast<Platform*>(entity);

        PhysicsComponent component;
        component.name = platform->Name;
        component.base = entity;
        component.platform = platform;
        component.transform = platform->transform;
        component.rigidbody = platform->rigidbody;
        component.collider = platform->collider;
        component.dynamicModel = platform->dynamicModel;
        component.aircraft = new Aircraft();
        physicsComponent[platform->ID] = component;

        if (platform->transform && platform->rigidbody) {
            btCollisionShape* shape = nullptr;

            if (platform->collider && platform->collider->collider == Constants::ColliderType::Box) {
                shape = new btBoxShape(btVector3(1, 1, 1));
                shape->setLocalScaling(btVector3(
                    platform->transform->matrix->scale3D().x() * platform->collider->Width * 0.5f,
                    platform->transform->matrix->scale3D().y() * platform->collider->Length * 0.5f,
                    platform->transform->matrix->scale3D().z() * platform->collider->Height * 0.5f));
            } else if (platform->collider && platform->collider->collider == Constants::ColliderType::Sphere) {
                shape = new btSphereShape(platform->collider->CollideRadius * platform->transform->matrix->scale3D().length());
            }

            if (shape) {
                btTransform transform;
                transform.setIdentity();
                transform.setOrigin(btVector3(
                    platform->transform->matrix->translation().x(),
                    platform->transform->matrix->translation().y(),
                    platform->transform->matrix->translation().z()));
                btQuaternion quat;
                quat.setEulerZYX(
                    qDegreesToRadians(platform->transform->toEulerAngles().z()),
                    qDegreesToRadians(platform->transform->toEulerAngles().y()),
                    qDegreesToRadians(platform->transform->toEulerAngles().x()));
                transform.setRotation(quat);

                float mass = platform->rigidbody->Mass;
                btVector3 inertia(0, 0, 0);
                if (mass > 0)
                    shape->calculateLocalInertia(mass, inertia);

                btDefaultMotionState* motionState = new btDefaultMotionState(transform);
                btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, inertia);
                btRigidBody* body = new btRigidBody(rbInfo);

                if (!platform->rigidbody->Gravity) {
                    body->setGravity(btVector3(0, 0, 0));
                }

                if (platform->rigidbody->Kinematics) {
                    body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                    body->setActivationState(DISABLE_DEACTIVATION);
                }

                body->setLinearVelocity(btVector3(
                    platform->rigidbody->velocity->x,
                    platform->rigidbody->velocity->y,
                    platform->rigidbody->velocity->z));

                body->setAngularVelocity(btVector3(
                    platform->rigidbody->angularVelocity->x,
                    platform->rigidbody->angularVelocity->y,
                    platform->rigidbody->angularVelocity->z));

                connect(platform->rigidbody, &Rigidbody::setLinearVel, this, [body](const Vector& velocity) {
                    body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
                });

                connect(platform->rigidbody, &Rigidbody::setAngularVel, this, [body](const Vector& velocity) {
                    body->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
                });

                btVector3 linearFactor(1, 1, 1);
                if (platform->rigidbody->freezePositionX) linearFactor.setX(0);
                if (platform->rigidbody->freezePositionY) linearFactor.setY(0);
                if (platform->rigidbody->freezePositionZ) linearFactor.setZ(0);
                body->setLinearFactor(linearFactor);

                btVector3 angularFactor(1, 1, 1);
                if (platform->rigidbody->freezeRotationX) angularFactor.setX(0);
                if (platform->rigidbody->freezeRotationY) angularFactor.setY(0);
                if (platform->rigidbody->freezeRotationZ) angularFactor.setZ(0);
                body->setAngularFactor(angularFactor);

                dynamicsWorld->addRigidBody(body);
                bulletBodies[platform->ID] = body;
            }
        }
    }

    // ── Weapon entity: register in physics + start missile flight monitor ───
    if (entity->type == Constants::EntityType::Weapon) {
        Weapon* weapon = dynamic_cast<Weapon*>(entity);
        if (weapon) {
            PhysicsComponent component;
            component.name        = weapon->Name;
            component.base        = entity;
            component.platform    = nullptr;
            component.weapon      = weapon;
            component.zone        = nullptr;
            component.transform   = weapon->transform;
            component.rigidbody   = weapon->rigidbody;
            component.collider    = weapon->collider;
            component.dynamicModel= weapon->dynamicModel;
            component.aircraft    = nullptr;   // weapons use own flight model
            physicsComponent[weapon->ID] = component;

            // Set canvas so blast PNG renders on detonation
            if (m_canvas) weapon->setCanvas(m_canvas);

            // Start the missile flight monitor: it will call missileStart()
            // automatically as soon as weapon->Active becomes true.
            // weapon->startFlightMonitor();
            Console::log("[Simulation] Weapon registered + flight monitor started: "
                         + weapon->Name);
        }
    }

    if (entity->type == Constants::EntityType::SpecialZone) {
        Specialzone* zone = dynamic_cast<Specialzone*>(entity);
        PhysicsComponent component;
        component.name = zone->Name;
        component.base = entity;
        component.zone = zone;
        component.platform = nullptr;
        component.transform = zone->transform;
        component.rigidbody = nullptr;
        component.collider = zone->collider;
        component.dynamicModel = nullptr;
        physicsComponent[zone->ID] = component;
    }

    emit HierarchyUpdate();

    // ✅ LAST ME - Auto-resume if was playing
    if (wasPlaying) {
        QTimer::singleShot(50, this, [this]() {
            startf();
            Console::log("Simulation auto-resumed after entity addition");
        });
    }
}

void Simulation::entityRemoved(QString ID) {
    std::string key = ID.toStdString();

    auto bulletIt = bulletBodies.find(key);
    if (bulletIt != bulletBodies.end()) {
        btRigidBody* body = bulletIt->second;
        if (body) {
            dynamicsWorld->removeRigidBody(body);
            delete body->getMotionState();
            delete body->getCollisionShape();
            delete body;
        }
        bulletBodies.erase(bulletIt);
    }

    auto physIt = physicsComponent.find(key);
    if (physIt != physicsComponent.end()) {
        // Stop weapon flight monitor before removing
        if (physIt->second.base &&
            physIt->second.base->type == Constants::EntityType::Weapon) {
            Weapon* weapon = dynamic_cast<Weapon*>(physIt->second.base);
            if (weapon) weapon->stopFlightMonitor();
        }
        physicsComponent.erase(physIt);
    }

    emit HierarchyUpdate();
}
//fixed:to initialise the netwrok when instance is client::by Aman
void Simulation::setNetworkManager(NetworkManager* nm)
{
    networkManager = nm;
}

//fix: to resolve the crash issue because both network thread and simulation thread were changing the transform..now only simulation does this by Aman
void Simulation::applyPendingNetworkUpdates()
{
    while (!incomingTransforms.empty())
    {
        auto msg = incomingTransforms.pop();
        if (!msg) break;

        auto it = physicsComponent.find(msg->id);
        if (it == physicsComponent.end()) continue;

        auto& comp = it->second;
        if (!comp.transform) continue;


        comp.transform->matrix->setTranslation(msg->pos);
        comp.transform->matrix->rotation().fromEulerAngles(msg->rot);
        //rotaion issue may be there
        // comp.transform->setTranslation(msg->pos);
        // comp.transform->setFromEulerAngles(msg->rot);
    }
}
void Simulation::enqueueTransformUpdate(const TransformUpdate& msg)//by Aman
{
    incomingTransforms.push(msg);
}
/*
    Network / Simulation Authority Rules

    We distinguish 3 modes:
      1) Stand-alone (no networking)
      2) Server (master simulation authority)
      3) Client (receives updates only)

    Behavior summary:

    | Case                     | networkActive | connected | isServer() | isMaster |
    | ------------------------ | ------------- | --------- | ---------- | -------- |
    | Stand-alone              | false         | false     |   n/a      |  true    |
    | Server                   | true          | true      |   true     |  true    |
    | Client                   | true          | true      |   false    |  false   |
    | Client not connected yet | true          | false     |   false    |  true*   |

    * Clients that are not yet connected should temporarily behave as
      master for safety — once sync completes, isMaster becomes false.

    Result:
      - Stand-alone mode runs full simulation locally
      - Server runs authoritative simulation
      - Clients only receive updates (no physics)
*/

void Simulation::updateDynamics(float dt,PhysicsComponent *comp){
    if(comp->dynamicModel->trajectory->Trajectories.size()<2) return;
    comp->aircraft->MaxAcceleration = comp->dynamicModel->Acceleration;
    comp->aircraft->MaxDecceleration = comp->dynamicModel->Decceleration;
    comp->aircraft->CeilingHeight = comp->dynamicModel->maxAltitude / 3.281f;
    comp->aircraft->TargetSpeed = comp->dynamicModel->moveSpeed  /3.6f;
    comp->aircraft->turnRate = comp->dynamicModel->turnRate;
    comp->aircraft->ClimbRate = comp->dynamicModel->climbRate/196.9f;
    comp->aircraft->DiveRate = comp->dynamicModel->diveRate/196.9f;
    // comp->aircraft->MaxPitch = comp->dynamicModel->maxPitch;
    comp->aircraft->PitchRate = comp->dynamicModel->turnRate;

    QVector3D position = comp->transform->translation();
    comp->aircraft->Position = Aircraft::vec3(position.x()* 1000.f,
                                              position.y()* 1000.f,
                                              position.z()* 1000.f);

    QVector3D eularAngle = comp->transform->toEulerAngles();
    comp->aircraft->EularAngles = Aircraft::vec3(eularAngle.x(),eularAngle.y(),eularAngle.z());
    comp->aircraft->LocalEularAngles = Aircraft::vec3(eularAngle.x(),eularAngle.y(),eularAngle.z());

    comp->aircraft->Forward.x = comp->transform->forward().x();
    comp->aircraft->Forward.y = comp->transform->forward().y();
    comp->aircraft->Forward.z = comp->transform->forward().z();

    Vector target = *comp->dynamicModel->trajectory->getTargetWaypoint()->position;
    //float tgtSpd = comp.dynamicModel->trajectory->getTargetWaypoint()->speed;
    FlatXYZ targt = geoToFlatXYZ(target.x,target.z,target.y);
    if(comp->dynamicModel->followTarget){
        targt = geoToFlatXYZ(comp->dynamicModel->followEntity->transform->getLatitude(),
                             comp->dynamicModel->followEntity->transform->getLongitude(),
                             comp->dynamicModel->followEntity->transform->getAltitude());
        comp->aircraft->TargetPosition.x = targt.x * 1000.f;
        comp->aircraft->TargetPosition.y = targt.y / 3.281f;
        comp->aircraft->TargetPosition.z = targt.z * 1000.f;

    }else{
        comp->aircraft->TargetPosition.x = targt.x * 1000.f;
        comp->aircraft->TargetPosition.y = targt.y / 3.281f;
        comp->aircraft->TargetPosition.z = targt.z * 1000.f;
    }


    comp->aircraft->FixedUpdate(dt);

    Aircraft::vec3 pos = comp->aircraft->Position;
    comp->transform->setTranslation(QVector3D(pos.x/1000.f,
                                              pos.y/1000.f,
                                              pos.z/1000.f));

    Aircraft::vec3 rot = comp->aircraft->LocalEularAngles;
    // Aircraft::vec3 rot = comp->aircraft->EularAngles;
    rot.y = comp->aircraft->EularAngles.y;
    comp->transform->matrix->setRotation(QQuaternion::fromEulerAngles(QVector3D(rot.x,rot.y,rot.z)));
    comp->dynamicModel->currentSpeed = comp->aircraft->speed * 3.6f;
    comp->dynamicModel->currentAltitude = comp->aircraft->Altitude/1000.0f;

}

void Simulation::calculateDynamic(float dt){
    for (auto& [id, comp] : physicsComponent) {
        if(!comp.base->Active) continue;
        if(comp.dynamicModel)
            comp.dynamicModel->Update(dt);
    }
}
void Simulation::calculatePhysics() {
    const float dt = deltaTime * speed;
    const float maxDt = 1.0f / PhysicsUpdateFrameRate;
    const float clampedDt = std::min(dt, maxDt);
    simulationTime += dt;
    //dynamicsWorld->stepSimulation(clampedDt, 10);
    emit Physics();

    simMin +=simCount;
    if(physicsComponent.size() <= simMin){
        simMin = 0;
    }

    Profiler::currentFrame->RadarTime = 0;
    Profiler::currentFrame->EWTime = 0;
    Profiler::currentFrame->CSMTime = 0;
    Profiler::currentFrame->ESMTime = 0;
    Profiler::currentFrame->IFFTime = 0;
    Profiler::currentFrame->RadioTime = 0;
    int dynamicTime = 0;
    int sensorTime = 0;
    int num = 0;
    int simMax = simMin+simCount;
    // qDebug()<<simMin<<","<<simMax;
    for (auto& [id, comp] : physicsComponent) {
        if(!comp.base->Active) continue;
        if (!comp.transform) continue;

        if(comp.rigidbody){
            comp.rigidbody->deltaTime = deltaTime;
        }


        if(comp.collider){
            comp.collider->Update(dt);
        }
        //fix:we are keeping dynamic only to server
        bool isMaster = true;
        if (networkManager && networkManager->isActive())
            isMaster = networkManager->isServer();
        if (isMaster && comp.dynamicModel){
            QElapsedTimer timer;
            timer.start();  // Start measuring
            comp.dynamicModel->Update(dt);
            updateDynamics(dt,&comp);
            qint64 elapsedMs = timer.elapsed();
            dynamicTime +=elapsedMs;
        }
        bool opt = true;
        if(physicsComponent.size()>70){
            opt = simMin<num && num< simMax;
        }
        if(comp.base->type == Constants::EntityType::Weapon) {
            Weapon* weapon = dynamic_cast<Weapon*>(comp.base);
            if (weapon&&weapon->isLaunched&&!weapon->isDead) weapon->missileUpdate(dt);
        }
        if (comp.platform && opt){
            QElapsedTimer timer;
            timer.start();  // Start measuring
            comp.platform->update();
            qint64 elapsedMs = timer.elapsed();
            sensorTime +=elapsedMs;
        }

        if(comp.zone){
            comp.zone->Update(dt);
        }
        comp.transform->sync();
        num++;
        // auto it = bulletBodies.find(id);
        // if (it != bulletBodies.end()) {
        //     btRigidBody* body = it->second;
        //     btTransform trans;
        //     body->getMotionState()->getWorldTransform(trans);

        //     btVector3 pos = trans.getOrigin();
        //     btQuaternion rot = trans.getRotation();
        //     // comp.transform->position->x = pos.getX();
        //     // comp.transform->position->y = pos.getY();
        //     // comp.transform->position->z = pos.getZ();

        //     btScalar x, y, z;
        //     rot.getEulerZYX(z, y, x);
        //     // comp.transform->rotation->x = qRadiansToDegrees(x);
        //     // comp.transform->rotation->y = qRadiansToDegrees(y);
        //     // comp.transform->rotation->z = qRadiansToDegrees(z);

        //     if (comp.collider) {
        //         body->getCollisionShape()->setLocalScaling(btVector3(
        //             comp.transform->matrix->scale3D().x() * comp.collider->Width * 0.5f,
        //             comp.transform->matrix->scale3D().y() * comp.collider->Length * 0.5f,
        //             comp.transform->matrix->scale3D().z() * comp.collider->Height * 0.5f));
        //     }

        //     if (comp.rigidbody->Gravity) {
        //         body->setGravity(btVector3(Gravity->x, Gravity->y, Gravity->z));
        //     } else {
        //         body->setGravity(btVector3(0, 0, 0));
        //     }

        //     comp.rigidbody->velocity->x = body->getLinearVelocity().x();
        //     comp.rigidbody->velocity->y = body->getLinearVelocity().y();
        //     comp.rigidbody->velocity->z = body->getLinearVelocity().z();

        //     comp.rigidbody->angularVelocity->x = body->getAngularVelocity().x();
        //     comp.rigidbody->angularVelocity->y = body->getAngularVelocity().y();
        //     comp.rigidbody->angularVelocity->z = body->getAngularVelocity().z();

        //     btVector3 linearFactor(1, 1, 1);
        //     if (comp.rigidbody->freezePositionX) linearFactor.setX(0);
        //     if (comp.rigidbody->freezePositionY) linearFactor.setY(0);
        //     if (comp.rigidbody->freezePositionZ) linearFactor.setZ(0);
        //     body->setLinearFactor(linearFactor);

        //     btVector3 angularFactor(1, 1, 1);
        //     if (comp.rigidbody->freezeRotationX) angularFactor.setX(0);
        //     if (comp.rigidbody->freezeRotationY) angularFactor.setY(0);
        //     if (comp.rigidbody->freezeRotationZ) angularFactor.setZ(0);
        //     body->setAngularFactor(angularFactor);

        //     if (comp.rigidbody->Kinematics) {
        //         body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        //         body->setActivationState(DISABLE_DEACTIVATION);
        //     } else {
        //         body->setCollisionFlags(body->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
        //         body->setActivationState(ACTIVE_TAG);
        //     }
        // }
    }
    Profiler::currentFrame->dynamicTime = dynamicTime;
    Profiler::currentFrame->SensorTime = sensorTime;
}

void Simulation::entityUpdate(QString ID) {
    std::string key = ID.toStdString();
    auto physIt = physicsComponent.find(key);
    if (physIt == physicsComponent.end()) return;

    auto bulletIt = bulletBodies.find(key);
    if (bulletIt == bulletBodies.end()) return;

    btRigidBody* body = bulletIt->second;
    PhysicsComponent& comp = physIt->second;

    if (body && comp.transform) {
        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);

        trans.setOrigin(btVector3(
            comp.transform->matrix->translation().x(),
            comp.transform->matrix->translation().y(),
            comp.transform->matrix->translation().z()));

        btQuaternion quat;
        quat.setEulerZYX(
            qDegreesToRadians(comp.transform->toEulerAngles().z()),
            qDegreesToRadians(comp.transform->toEulerAngles().y()),
            qDegreesToRadians(comp.transform->toEulerAngles().x()));
        trans.setRotation(quat);

        body->setWorldTransform(trans);

        if (comp.collider) {
            body->getCollisionShape()->setLocalScaling(btVector3(
                comp.transform->matrix->scale3D().x() * comp.collider->Width * 0.5f,
                comp.transform->matrix->scale3D().y() * comp.collider->Length * 0.5f,
                comp.transform->matrix->scale3D().z() * comp.collider->Height * 0.5f));
        }

        comp.rigidbody->velocity->x = body->getLinearVelocity().x();
        comp.rigidbody->velocity->y = body->getLinearVelocity().y();
        comp.rigidbody->velocity->z = body->getLinearVelocity().z();

        comp.rigidbody->angularVelocity->x = body->getAngularVelocity().x();
        comp.rigidbody->angularVelocity->y = body->getAngularVelocity().y();
        comp.rigidbody->angularVelocity->z = body->getAngularVelocity().z();

        if (body->getMotionState()) {
            body->getMotionState()->setWorldTransform(trans);
        }
    }

    emit HierarchyUpdate();
}
