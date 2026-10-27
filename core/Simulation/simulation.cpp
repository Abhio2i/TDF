#include "simulation.h"
#include <algorithm>
#include <core/Debug/console.h>
#include "core/Debug/profiler.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "qprogressdialog.h"
#include <QJsonObject>
#include <QtMath>
#include <GUI/mainwindow.h>
#include "core/Hierarchy/hierarchy.h"

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
//by aman
void Simulation::registerPlugin(SimulationPlugin* plugin) {
    if (plugin && !m_plugins.contains(plugin))
        m_plugins.append(plugin);
}

void Simulation::unregisterPlugin(SimulationPlugin* plugin) {
    m_plugins.removeAll(plugin);
}
//byaman
void Simulation::setFps(int value){
    SimulationFrameRate = value;
    PhysicsUpdateFrameRate = value;
    UIUpdateFrameRate = 30;
    updateTimer->setInterval(1000/value);
}

// void Simulation::frame() {
//     qint64 currentTime = elapsedTimer->elapsed();
//     deltaTime = (currentTime - lastTime) / 1000.0f;
//     if(!isPlay){
//         deltaTime = 0.02f;
//     }
//     if(deltaTime>0.05){
//         deltaTime = 0.05f;
//     }
//     lastTime = currentTime;
void Simulation::frame() {
    qint64 currentTime = elapsedTimer->elapsed();
    deltaTime = (currentTime - lastTime) / 1000.0f;
    if(!isPlay){
        deltaTime = 0.02f;
    }
    if(deltaTime>0.05){
        deltaTime = 0.05f;
    }
    lastTime = currentTime;
    if(deltaTime ==0 ){
        qDebug()<<"";deltaTime = (1000.0f/SimulationFrameRate)/1000.0f;
    }
    for (SimulationPlugin* plugin : m_plugins)//by aman
        plugin->applyPendingUpdates();//by Aman to apply recived update via network
    //applyPendingNetworkUpdates();//by Aman to apply recived update via network
    calculatePhysics();

    QJsonObject frameData;
    QJsonArray entitiesArray;
    for (const auto& [id, comp] : physicsComponent) {
        if (!comp.transform) continue;

        QJsonObject entityFrame;
        entityFrame["id"] = QString::fromStdString(id);
        entityFrame["position"] = QJsonObject{
            {"x", comp.transform->matrix.translation().x()},
            {"y", comp.transform->matrix.translation().y()},
            {"z", comp.transform->matrix.translation().z()}
        };
        entityFrame["rotation"] = QJsonObject{
            {"x", comp.transform->matrix.rotation().toEulerAngles().x()},
            {"y", comp.transform->matrix.rotation().toEulerAngles().y()},
            {"z", comp.transform->matrix.rotation().toEulerAngles().z()}
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
        if (comp.platform){
            if(simulationTime == 0){
                comp.platform.data()->SaveState();
            }
            comp.platform.data()->Start();
        }


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
        // if (comp.base && comp.base->type == Constants::EntityType::Weapon) {
        //     Weapon* weapon = dynamic_cast<Weapon*>(comp.base.data());
        //     // if (weapon && weapon->isLaunched && !weapon->isDead)
        //     //     // weapon->resumeFlightMonitor();
        // }
    }

}

void Simulation::pause() {
    QMetaObject::invokeMethod(this, "pausef", Qt::QueuedConnection);
}

void Simulation::pausef() {
    updateTimer->stop();
    isPlay = false;

    // for (auto& [id, comp] : physicsComponent) {
    //     if (comp.base && comp.base->type == Constants::EntityType::Weapon) {
    //         Weapon* weapon = dynamic_cast<Weapon*>(comp.base);
    //         // if (weapon && weapon->isLaunched && !weapon->isDead)
    //         // weapon->pauseFlightMonitor();
    //     }
    // }

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
            // Weapon* weapon = dynamic_cast<Weapon*>(comp.base);
            // if (weapon) weapon->stopFlightMonitor();
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
    startf();
    pausef();

    int secDiff = newTime - simulationTime;
    bool restorestate = false;
    if(secDiff <0){
        restorestate = true;
        simulationTime= 0;
        secDiff = newTime;
    };
    deltaTime = 0.02f;
    float speedvar = speed;
    fastupdate = true;
    speed = 4;
    float fps = 60.0f;
    int totalSteps = secDiff * fps;
    float dt = 1.0f / fps;
    totalSteps = (totalSteps/4.0);

    // 1. Progress Dialog setup karein
    QProgressDialog progress("Processing simulation...", nullptr, 0, physicsComponent.size(), nullptr);
    progress.setWindowModality(Qt::ApplicationModal); // Screen block karne ke liye
    progress.setCancelButton(nullptr);
    // --- Yeh line add karein ---
    progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    // 2. Progress Bar ko dhund kar hide kar dein
    QProgressBar *bar = progress.findChild<QProgressBar *>();
    if (bar) {
        // bar->hide();
    }

    progress.show();
    int count = 0;
    for (auto& [id, comp] : physicsComponent) {
        count++;
        if(!comp.base->Active) continue;
        if (!comp.transform) continue;
        if(restorestate && comp.platform)comp.platform->RestoreState();
        if(comp.rigidbody){
            comp.rigidbody->deltaTime = deltaTime;
        }
        //fix:we are keeping dynamic only to server
        bool isMaster = true;
        if (networkManager && networkManager->isActive())
            isMaster = networkManager->isServer();
        if (isMaster && comp.dynamicModel && !comp.dynamicModel->followEntity){
            comp.dynamicModel->TimeJump(secDiff);
            comp.dynamicModel->Update(dt);
            updateDynamics(dt,&comp);
            progress.setValue(count);
        }
        comp.transform->sync();
    }

    for (auto& [id, comp] : physicsComponent) {
        count++;
        if(!comp.base->Active) continue;
        if (!comp.transform) continue;
        // if(restorestate && comp.platform)comp.platform->RestoreState();
        if(comp.rigidbody){
            comp.rigidbody->deltaTime = deltaTime;
        }
        //fix:we are keeping dynamic only to server
        bool isMaster = true;
        if (networkManager && networkManager->isActive())
            isMaster = networkManager->isServer();
        if (isMaster && comp.dynamicModel && comp.dynamicModel->followEntity){
            comp.dynamicModel->TimeJump(secDiff);
            comp.dynamicModel->Update(dt);
            updateDynamics(dt,&comp);
            progress.setValue(count);
        }
        comp.transform->sync();
    }


    simulationTime = newTime;

    // for(int i = 0; i < totalSteps; i++){
    //     // 3. Check karein agar user ne 'Cancel' dabaya hai
    //     if (progress.wasCanceled()) {
    //         qDebug() << "Simulation cancelled by user.";
    //         break;
    //     }

    //     deltaTime = dt;
    //     calculatePhysics();

    //     // Internally update karein taaki cancel button responsive rahe
    //     progress.setValue(i);

    //     // GUI freezing se bachne ke liye
    //     if(i % 5000 == 0) {
    //         emit Render(dt);
    //         QCoreApplication::processEvents();
    //     }
    // }

    progress.close();
    emit Render(dt);
    speed = speedvar;
    fastupdate = false;
    // startf();
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

                comp.transform->matrix.translation().setX(pos["x"].toDouble());
                comp.transform->matrix.translation().setY(pos["y"].toDouble());
                comp.transform->matrix.translation().setZ(pos["z"].toDouble());

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

    // Remote DIS entities — lightweight registration only.
    // Never pause simulation for remote entities.
    // They have no physics components — only transform for dead reckoning.
    if (!entity->isRemoteDISEntity) {
        Hierarchy* ctx = Hierarchy::getCurrentContext();
        if (ctx && ctx->m_pendingRemoteDISEntityIDs.contains(
                QString::fromStdString(entity->ID))) {
            entity->isRemoteDISEntity = true;
        }
    }
    if (entity->isRemoteDISEntity) {
        Platform* platform = dynamic_cast<Platform*>(entity);
        if (platform /*&& platform->transform*/) {
            // Guard: don't register twice
            if (physicsComponent.find(platform->ID) != physicsComponent.end())
                return;

            PhysicsComponent component;
            component.name         = platform->Name;
            component.base         = entity;
            component.platform     = platform;
            component.transform    = &platform->transform;
            component.rigidbody    = nullptr;
            component.collider     = nullptr;
            component.dynamicModel = nullptr;
            component.aircraft     = nullptr;
            component.sea          = nullptr;
            physicsComponent[platform->ID] = component;
        }
        emit HierarchyUpdate();
        return;
    }

    // Local entities — pause simulation during setup
    bool wasPlaying = isPlay;
    if (wasPlaying) {
        pausef();
    }

    if (entity->type == Constants::EntityType::Platform) {
        Platform* platform = dynamic_cast<Platform*>(entity);

        PhysicsComponent component;
        component.name = platform->Name;
        component.base = entity;
        component.platform = platform;
        component.transform = &platform->transform;
        component.rigidbody = &platform->rigidbody;
        component.collider = &platform->collider;
        component.dynamicModel = &platform->dynamicModel;
        if (entity->category == Entity::Category::Marine) {
            component.sea = std::make_shared<SeaModel>();
            component.aircraft.reset();
        } else {
            component.aircraft = std::make_shared<Aircraft>();
            component.sea.reset();
        }
        physicsComponent[platform->ID] = component;

        // if (/*platform->transform &&*/ platform->rigidbody) {
        btCollisionShape* shape = nullptr;

        if (/*platform->collider &&*/ platform->collider.collider == Constants::ColliderType::Box) {
            shape = new btBoxShape(btVector3(1, 1, 1));
            shape->setLocalScaling(btVector3(
                platform->transform.matrix.scale3D().x() * platform->collider.getDimension().Width * 0.5f,
                platform->transform.matrix.scale3D().y() * platform->collider.getDimension().Length * 0.5f,
                platform->transform.matrix.scale3D().z() * platform->collider.getDimension().Height * 0.5f));
        } else if (/*platform->collider && */platform->collider.collider == Constants::ColliderType::Sphere) {
            shape = new btSphereShape(platform->collider.CollideRadius * platform->transform.matrix.scale3D().length());
        }

        if (shape) {
            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(btVector3(
                platform->transform.matrix.translation().x(),
                platform->transform.matrix.translation().y(),
                platform->transform.matrix.translation().z()));
            btQuaternion quat;
            quat.setEulerZYX(
                qDegreesToRadians(platform->transform.toEulerAngles().z()),
                qDegreesToRadians(platform->transform.toEulerAngles().y()),
                qDegreesToRadians(platform->transform.toEulerAngles().x()));
            transform.setRotation(quat);

            float mass = platform->rigidbody.Mass;
            btVector3 inertia(0, 0, 0);
            if (mass > 0)
                shape->calculateLocalInertia(mass, inertia);

            btDefaultMotionState* motionState = new btDefaultMotionState(transform);
            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, inertia);
            btRigidBody* body = new btRigidBody(rbInfo);

            if (!platform->rigidbody.Gravity) {
                body->setGravity(btVector3(0, 0, 0));
            }

            if (platform->rigidbody.Kinematics) {
                body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                body->setActivationState(DISABLE_DEACTIVATION);
            }

            body->setLinearVelocity(btVector3(
                platform->rigidbody.velocity->x,
                platform->rigidbody.velocity->y,
                platform->rigidbody.velocity->z));

            body->setAngularVelocity(btVector3(
                platform->rigidbody.angularVelocity->x,
                platform->rigidbody.angularVelocity->y,
                platform->rigidbody.angularVelocity->z));

            connect(&platform->rigidbody, &Rigidbody::setLinearVel, this, [body](const Vector& velocity) {
                body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
            });

            connect(&platform->rigidbody, &Rigidbody::setAngularVel, this, [body](const Vector& velocity) {
                body->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
            });

            btVector3 linearFactor(1, 1, 1);
            if (platform->rigidbody.freezePositionX) linearFactor.setX(0);
            if (platform->rigidbody.freezePositionY) linearFactor.setY(0);
            if (platform->rigidbody.freezePositionZ) linearFactor.setZ(0);
            body->setLinearFactor(linearFactor);

            btVector3 angularFactor(1, 1, 1);
            if (platform->rigidbody.freezeRotationX) angularFactor.setX(0);
            if (platform->rigidbody.freezeRotationY) angularFactor.setY(0);
            if (platform->rigidbody.freezeRotationZ) angularFactor.setZ(0);
            body->setAngularFactor(angularFactor);

            dynamicsWorld->addRigidBody(body);
            bulletBodies[platform->ID] = body;
        }
        // }
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
            component.transform   = &weapon->transform;
            component.rigidbody   = &weapon->rigidbody;
            component.collider    = &weapon->collider;
            component.dynamicModel= &weapon->dynamicModel;
            component.aircraft    = nullptr;   // weapons use own flight model
            component.sea         = nullptr;
            physicsComponent[weapon->ID] = component;

            // Set canvas so blast PNG renders on detonation
            if (m_canvas) weapon->setCanvas(m_canvas);

            // Start the missile flight monitor: it will call missileStart()
            // automatically as soon as weapon->Active becomes true.
            // weapon->startFlightMonitor();
            // Console::log("[Simulation] Weapon registered + flight monitor started: "
            //              + weapon->Name);
        }
    }

    if (entity->type == Constants::EntityType::SpecialZone) {
        Specialzone* zone = dynamic_cast<Specialzone*>(entity);
        PhysicsComponent component;
        component.name = zone->Name;
        component.base = entity;
        component.zone = zone;
        component.platform = nullptr;
        component.transform = &zone->transform;
        component.rigidbody = nullptr;
        component.collider = &zone->collider;
        component.dynamicModel = nullptr;
        physicsComponent[zone->ID] = component;
    }

    emit HierarchyUpdate();

    // // ✅ LAST ME - Auto-resume if was playing
    // if (wasPlaying) {
    //     QTimer::singleShot(50, this, [this]() {
    //         startf();
    //         Console::log("Simulation auto-resumed after entity addition");
    //     });
    // }
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
        // if (physIt->second.base &&
        //     physIt->second.base->type == Constants::EntityType::Weapon) {
        //     Weapon* weapon = dynamic_cast<Weapon*>(physIt->second.base);
        //     // if (weapon) weapon->stopFlightMonitor();
        // }
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


        comp.transform->matrix.setTranslation(msg->pos);
        comp.transform->matrix.rotation().fromEulerAngles(msg->rot);
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

// ============================================================================
// Simulation::updateDynamics
// Trajectory/Waypoint -> DynamicModel command -> Aircraft SI solver
// -> DynamicModel telemetry + Qt Transform
// ============================================================================

// ============================================================================
// Simulation::updateDynamics
// Trajectory/Waypoint -> DynamicModel command -> Aircraft SI solver
// -> DynamicModel telemetry + Qt Transform
// ============================================================================

void Simulation::updateDynamics(float dt, PhysicsComponent *comp)
{
    if (!comp ||!comp->dynamicModel  ||!comp->transform ||!comp->dynamicModel->trajectory)
    {
        return;
    }

    DynamicModel *dm = comp->dynamicModel;
    Transform *transform = comp->transform;
    Trajectory *trajectory = dm->trajectory;

    if (dt <= 0.0f || !std::isfinite(dt))
        return;

    if (dm->startTime > dm->time)
        return;

    // Formation/follow uses the existing specialized path.
    if (dm->follow && dm->followEntity)
        return;

    if (!trajectory->Active ||!trajectory->FollowPath ||trajectory->Trajectories.size() < 2)
    {
        return;
    }

    // ========================================================================
    // UNIT CONTRACT
    // ========================================================================
    constexpr float KM_TO_M = 1000.0f;
    constexpr float M_TO_KM = 0.001f;
    constexpr float KMH_TO_MS = 1.0f / 3.6f;
    constexpr float MS_TO_KMH = 3.6f;
    constexpr float FT_TO_M = 0.3048f;
    constexpr float M_TO_FT = 3.280839895f;
    constexpr float FPM_TO_MS = 0.00508f;

    auto normalize360 = [](float angle) -> float
    {
        angle = std::fmod(angle, 360.0f);
        if (angle < 0.0f)
            angle += 360.0f;
        return angle;
    };

    auto normalize180 = [](float angle) -> float
    {
        angle = std::fmod(angle + 180.0f, 360.0f);
        if (angle < 0.0f)
            angle += 360.0f;
        return angle - 180.0f;
    };

    if (comp->base && comp->base->category == Entity::Category::Marine)
    {
        SeaModel *sea = comp->sea.get();
        if (!sea) return;
        const bool submarine = comp->base->marineCategory == Entity::SubMarineCategory::Submarine;
        sea->vesselType = submarine ? SeaModel::VesselType::Submarine : SeaModel::VesselType::SurfaceShip;
        sea->environment.seaSurfaceY = 0;
        sea->environment.hasSeafloor = 0;
        sea->environment.seafloorY = -500;
        sea->environment.seabedClearance = 10.0f;
        sea->environment.waterCurrent = SeaModel::Vec3(0,0,0);

        // Marine-specific defaults intentionally replace aircraft's 1800 km/h,
        // 100 m/s^2 and 10.62 deg/s defaults. Override using the JSON block.
        sea->limits.maxSpeed = dm->maxSpeed * KMH_TO_MS;
        sea->limits.maxAcceleration = dm->Acceleration;
        sea->limits.maxDeceleration = dm->Decceleration;
        sea->limits.maxJerk = 0.30f;
        sea->limits.maxTurnRate = dm->turnRate;
        sea->limits.maxTurnAcceleration = 0.35f;
        sea->limits.minSteerageSpeed = 0.30f;
        sea->limits.fullSteerageSpeed = 3.0f;
        sea->limits.canTurnInPlace = false;
        const float defaultDepth = dm->maxAltitude < 0.0f
                                       ? -dm->maxAltitude * FT_TO_M : 300.0f;
        sea->limits.maxOperatingDepth = defaultDepth;
        sea->limits.maxRiseRate = dm->climbRate * FPM_TO_MS;
        sea->limits.maxDiveRate = dm->diveRate * FPM_TO_MS;
        sea->limits.maxVerticalAcceleration = 0.30f;
        sea->limits.maxPitch = 12;
        sea->limits.maxRoll = 8;
        sea->responses.speedResponseTime = 2;
        sea->responses.headingResponseTime = 3;
        sea->responses.depthResponseTime = 6;

        const QVector3D posKm = transform->translation();
        const QVector3D qtEuler = transform->toEulerAngles();
        sea->transform.position = SeaModel::Vec3(posKm.x() * KM_TO_M,
                                                 posKm.y() * KM_TO_M,
                                                 posKm.z() * KM_TO_M);
        sea->transform.heading = normalize360(qtEuler.y());
        sea->transform.pitch = -qtEuler.x();
        sea->transform.roll = qtEuler.z();
        if (!sea->IsInitialized())
            sea->state.speedThroughWater = std::max(0.0f, dm->currentSpeed) * KMH_TO_MS;
        // Marine accepts negative altitude. 0 is sea level, -100 m is 100 m below.
        // "GroundRelative" is not meaningful underwater: for Marine only, it
        // is interpreted as an offset relative to the sea surface.
        const auto ref = dm->altitudeReference == DynamicModel::AltitudeReference::GroundRelative
                             ? SeaModel::AltitudeReference::SeaSurfaceRelative
                             : SeaModel::AltitudeReference::WorldY;
        sea->SetNavigationCommand(dm->commandedSpeed * KMH_TO_MS,
                                  dm->commandedHeading,
                                  dm->commandedAltitude * FT_TO_M,
                                  ref);
        if (comp->platform && comp->platform->fuel <= 0.5f)
            sea->commands.speed = 0.0f;
        sea->FixedUpdate(dt);

        const SeaModel::TransformState &st = sea->transform;
        const SeaModel::Telemetry &t = sea->telemetry;
        const SeaModel::State &rs = sea->state;
        transform->setTranslation(QVector3D(st.position.x * M_TO_KM,
                                            st.position.y * M_TO_KM,
                                            st.position.z * M_TO_KM));
        transform->matrix.setRotation(QQuaternion::fromEulerAngles(
            -st.pitch, st.heading, st.roll));

        dm->currentSpeed = t.speedThroughWater * MS_TO_KMH;
        dm->currentAcceralation = rs.acceleration;
        dm->currentAltitude = t.altitude * M_TO_KM; // existing runtime contract: km
        //dm->currentDepth = t.depth;                 // new field: positive metres
        dm->currentHeightAboveGround = 0.0f;  // aircraft-only telemetry
        //dm->currentHeightAboveSeabed = std::isfinite(t.heightAboveSeabed)
                                           // ? t.heightAboveSeabed : -1.0f;  // metres, -1 when seafloor unknown
        dm->pitch = t.pitch;
        dm->roll = t.roll;
        dm->yaw = t.heading;
        dm->TrueHeading = t.heading;
        dm->Pitchrate = rs.pitchRate;
        dm->Rollrate = rs.rollRate;
        dm->Yawrate = rs.headingRate;
        dm->currentTurnRate = rs.headingRate;
        dm->horizonPitchRate = rs.pitchRate;
        dm->horizonRollRate = rs.rollRate;
        dm->horizonRatesValid = true;
        dm->TrueAirSpeed = 0.0f;              // not applicable to marine
        dm->GroundVelocity = t.speedOverGround * MS_TO_KMH;
        dm->NorthVelocity = t.groundVelocity.z;
        dm->EastVelocity = t.groundVelocity.x;
        dm->VerticalVelocity = t.verticalSpeed;
        dm->groundVelocity = QVector3D(t.groundVelocity.x,
                                       t.groundVelocity.y,
                                       t.groundVelocity.z);
        dm->bodyGroundVelocity = QVector3D(t.bodyGroundVelocity.x,
                                           t.bodyGroundVelocity.y,
                                           t.bodyGroundVelocity.z);
        dm->airVelocity = QVector3D();        // water != air
        dm->bodyAirVelocity = QVector3D();
        dm->localVelocity = dm->bodyGroundVelocity;
        dm->acceleration = QVector3D(t.acceleration.x, t.acceleration.y, t.acceleration.z);
        dm->bodyAcceleration = QVector3D(t.bodyAcceleration.x, t.bodyAcceleration.y,
                                         t.bodyAcceleration.z);
        dm->velocity = dm->groundVelocity * M_TO_KM; // legacy formation km/s
        dm->angularVelocity = QVector3D(rs.pitchRate, rs.headingRate, rs.rollRate);
        dm->DriftAngle = t.driftAngle;
        dm->isGrounded = false;
        dm->isStalling = false;
        dm->liftFactor = 0.0f;
        dm->angleOfAttack = 0.0f;
        dm->sideslipAngle = 0.0f;
        dm->flightPathAngle = std::atan2(t.verticalSpeed,
                                         std::max(0.001f, t.speedOverGround)) *
                              (180.0f / static_cast<float>(M_PI));
        transform->geocord.altitude = static_cast<double>(t.altitude) * M_TO_FT;
        return;

    }

    // ========================================================================
    // 2. DYNAMIC MODEL CONFIG -> AIRCRAFT
    // ========================================================================

    if (!comp->aircraft)
    {
        return;
    }
    Aircraft *aircraft = comp->aircraft.get();
    aircraft->maximums.minSpeed = std::max(0.0f, dm->minSpeed) * KMH_TO_MS;
    aircraft->maximums.maxSpeed = std::max(0.0f, dm->maxSpeed) * KMH_TO_MS;
    aircraft->maximums.maxAcceleration = std::max(0.0f, dm->Acceleration);
    aircraft->maximums.maxDeceleration = std::max(0.0f, dm->Decceleration);
    aircraft->maximums.turnRate = std::max(0.0f, dm->turnRate);
    aircraft->maximums.minimumTurnSpeed = std::max(0.0f, dm->minimumTurnSpeed) * KMH_TO_MS;
    aircraft->maximums.fullTurnAuthoritySpeed = std::max(0.0f, dm->fullTurnAuthoritySpeed) * KMH_TO_MS;
    aircraft->maximums.maxPitchAngle = std::clamp(dm->maxPitchAngle, 0.0f, 89.0f);
    aircraft->maximums.maxBankAngle = std::clamp(dm->maxBankAngle, 0.0f, 180.0f);
    aircraft->maximums.maxAltitude = dm->maxAltitude * FT_TO_M;
    aircraft->maximums.climbRate = std::max(0.0f, dm->climbRate) * FPM_TO_MS;
    aircraft->maximums.diveRate = std::max(0.0f, dm->diveRate) * FPM_TO_MS;
    // Responses
    aircraft->responses.deltaSpdCommandMaxAcc = std::max( 0.001f, dm->deltaSpdCommandMaxAcc);
    aircraft->responses.timeToReachMaxAcc = std::max( 0.0f, dm->timeToReachMaxAcc);
    aircraft->responses.deltaSpdCommandMaxDecel = std::max( 0.001f, dm->deltaSpdCommandMaxDecel);
    aircraft->responses.timeToReachMaxDecel = std::max( 0.0f, dm->timeToReachMaxDecel);
    aircraft->responses.deltaHdgCommandMaxRot = std::max( 0.001f, dm->deltaHdgCommandMaxRot);
    aircraft->responses.timeToReachMaxRot = std::max( 0.0f, dm->timeToReachMaxRot);
    aircraft->responses.maximumPitchRate = std::max( 0.0f, dm->maximumPitchRate);
    aircraft->responses.maximumRollRate = std::max( 0.0f, dm->maximumRollRate);
    aircraft->responses.deltaToReachMaxROC = std::max( 0.001f, dm->deltaToReachMaxROC);
    aircraft->responses.deltaToReachMaxROD = std::max( 0.001f, dm->deltaToReachMaxROD);
    // Flight physics
    aircraft->flightPhysics.minimumTakeoffSpeed = std::max( 0.0f, dm->minimumTakeoffSpeed) * KMH_TO_MS;
    aircraft->flightPhysics.stallSpeed = std::max( 0.0f, dm->stallSpeed) * KMH_TO_MS;
    aircraft->flightPhysics.criticalAngleOfAttack = std::max( 0.0f, dm->criticalAngleOfAttack);
    aircraft->flightPhysics.gravity = std::max( 0.0f, dm->gravity);
    aircraft->flightPhysics.maxSinkSpeed = std::max( 0.0f, dm->maxSinkSpeed);
    aircraft->flightPhysics.stallRecoveryRate = std::max( 0.0f, dm->stallRecoveryRate);
    aircraft->flightPhysics.stalledLiftFactor = std::clamp( dm->stalledLiftFactor, 0.0f, 1.0f);
    aircraft->flightPhysics.takeoffPitchThreshold = std::max( 0.0f, dm->takeoffPitchThreshold);
    aircraft->flightPhysics.groundLevel = dm->groundLevel * FT_TO_M;
    aircraft->stallRecovery.autoStallRecovery = dm->autoStallRecovery;
    aircraft->stallRecovery.stallRecoveryPitch = dm->stallRecoveryPitch;
    aircraft->aerodynamicDrag.zeroThrottleDrag = std::max( 0.0f, dm->zeroThrottleDrag);
    aircraft->aerodynamicDrag.highSpeedDrag = std::max( 0.0f, dm->highSpeedDrag);
    aircraft->aerodynamicDrag.inducedDrag = std::max( 0.0f, dm->inducedDrag);
    aircraft->aerodynamicDrag.referenceSpeed = std::max( 1.0f, dm->dragReferenceSpeed);
    // Wind: 0 deg -> +Z, 90 deg -> +X.
    const float windRad =dm->windAngle * static_cast<float>( M_PI / 180.0);
    aircraft->wind.windVelocity = Aircraft::vec3( dm->windSpeed * std::sin(windRad), 0.0f, dm->windSpeed * std::cos(windRad));
    // ========================================================================
    // 3. QT TRANSFORM (km) -> AIRCRAFT TRANSFORM (m)
    // ========================================================================
    const QVector3D qtPositionKm = transform->translation();
    aircraft->transform.position = Aircraft::vec3( qtPositionKm.x() * KM_TO_M, qtPositionKm.y() * KM_TO_M, qtPositionKm.z() * KM_TO_M);
    const QVector3D qtEuler = transform->toEulerAngles();

    // Aircraft positive pitch = nose up.
    aircraft->transform.pitch = -qtEuler.x();
    aircraft->transform.heading = normalize360( qtEuler.y());
    aircraft->transform.roll = qtEuler.z();
    aircraft->aircraftState.currentSpeed = std::max( 0.0f, dm->currentSpeed) * KMH_TO_MS;
    // ========================================================================
    // 4. DYNAMICMODEL COMMAND -> AIRCRAFT COMMAND
    // ========================================================================
    const Aircraft::AltitudeReference altitudeReference =dm->altitudeReference ==DynamicModel::AltitudeReference::GroundRelative? Aircraft::AltitudeReference::GroundRelative: Aircraft::AltitudeReference::WorldY;
    aircraft->SetNavigationCommand(
        dm->commandedSpeed *
            KMH_TO_MS,
        dm->commandedHeading,
        dm->commandedAltitude *
            FT_TO_M,
        altitudeReference);
    // Fuel exhausted: engine command is zero, but Aircraft continues under
    // drag/gravity instead of freezing.
    if (comp->platform && comp->platform->fuel <= 0.5f)
    {
        aircraft->commands.commandedSpeed = 0.0f;
    }
    // 5. AIRCRAFT SOLVER
    aircraft->FixedUpdate(dt);
    // 6. AIRCRAFT TRANSFORM (m) -> QT TRANSFORM (km)
    const Aircraft::vec3 &p = aircraft->transform.position;
    transform->setTranslation( QVector3D( p.x * M_TO_KM, p.y * M_TO_KM, p.z * M_TO_KM));

    // if (dm->control)
    // {
        transform->matrix.setRotation( QQuaternion::fromEulerAngles(
                -aircraft->aircraftState.currentPitch,
                aircraft->aircraftState.currentHeading,
                aircraft->aircraftState.currentRoll));
    // }

    // ========================================================================
    // 7. AIRCRAFT -> DYNAMICMODEL TELEMETRY
    // ========================================================================

    dm->currentSpeed = aircraft->aircraftState.currentSpeed * MS_TO_KMH;
    dm->currentAcceralation = aircraft->aircraftState.currentAcceleration;
    // Existing project contract: currentAltitude is kilometres.
    dm->currentAltitude = aircraft->aircraftState.currentAltitude * M_TO_KM;
    dm->currentHeightAboveGround = aircraft->aircraftState.currentHeightAboveGround * M_TO_KM;
    dm->pitch = aircraft->aircraftState.currentPitch;
    dm->roll = aircraft->aircraftState.currentRoll;
    dm->yaw = aircraft->aircraftState.currentYaw;
    dm->TrueHeading = aircraft->aircraftState.currentHeading;
    dm->Pitchrate = aircraft->aircraftState.currentPitchRate;
    dm->Rollrate = aircraft->aircraftState.currentRollRate;
    dm->Yawrate = aircraft->aircraftState.currentYawRate;
    dm->currentTurnRate = aircraft->aircraftState.currentTurnRate;
    dm->horizonPitchRate = aircraft->aircraftState.horizonPitchRate;
    dm->horizonRollRate = aircraft->aircraftState.horizonRollRate;
    dm->horizonRatesValid = aircraft->aircraftState.horizonRatesValid;
    dm->isGrounded = aircraft->flightState.isGrounded;
    dm->isStalling = aircraft->flightState.isStalling;
    dm->liftFactor = aircraft->flightState.liftFactor;
    dm->angleOfAttack = aircraft->flightState.angleOfAttack;
    dm->sideslipAngle = aircraft->flightState.sideslipAngle;
    dm->flightPathAngle = aircraft->flightState.flightPathAngle;
    dm->TrueAirSpeed = aircraft->speedState.trueAirSpeed * MS_TO_KMH;
    dm->GroundVelocity = aircraft->speedState.groundSpeed * MS_TO_KMH;
    const Aircraft::vec3 &gv = aircraft->velocityState.groundVelocity;
    dm->EastVelocity = gv.x;
    dm->VerticalVelocity = gv.y;
    dm->NorthVelocity = gv.z;
    dm->groundVelocity = QVector3D( gv.x, gv.y, gv.z);
    const Aircraft::vec3 &av = aircraft->velocityState.airVelocity;
    dm->airVelocity = QVector3D( av.x, av.y, av.z);
    const Aircraft::vec3 &bav = aircraft->velocityState.bodyAirVelocity;
    dm->bodyAirVelocity = QVector3D( bav.x, bav.y, bav.z);
    const Aircraft::vec3 &bgv = aircraft->velocityState.bodyGroundVelocity;
    dm->bodyGroundVelocity = QVector3D( bgv.x, bgv.y, bgv.z);
    // Legacy localVelocity: body ground-relative m/s.
    dm->localVelocity = dm->bodyGroundVelocity;
    const Aircraft::vec3 &acc = aircraft->accelerationState.acceleration;
    dm->acceleration = QVector3D( acc.x, acc.y, acc.z);
    const Aircraft::vec3 &bodyAcc = aircraft->accelerationState.bodyAcceleration;
    dm->bodyAcceleration = QVector3D( bodyAcc.x, bodyAcc.y, bodyAcc.z);
    // Existing formation logic expects DynamicModel velocity in km/s.
    dm->velocity = QVector3D( gv.x * M_TO_KM, gv.y * M_TO_KM, gv.z * M_TO_KM);
    dm->angularVelocity = QVector3D( dm->Pitchrate, dm->Yawrate, dm->Rollrate);
    // Drift = horizontal ground-track minus aircraft heading.
    if ((gv.x * gv.x + gv.z * gv.z) > 0.000001f)
    {
        const float groundTrack = normalize360( std::atan2( gv.x, gv.z) * (180.0f / static_cast<float>(M_PI)));
        dm->DriftAngle = normalize180( groundTrack - aircraft->aircraftState.currentHeading);
    }
    else
    {
        dm->DriftAngle = 0.0f;
    }
    // Existing geocord altitude contract remains feet.
    transform->geocord.altitude = static_cast<double>( aircraft->aircraftState.currentAltitude) * M_TO_FT;
}

void Simulation::calculateDynamic(float dt){
    for (auto& [id, comp] : physicsComponent) {
        if(!comp.base->Active) continue;
        if(comp.dynamicModel)
            comp.dynamicModel->Update(dt);
    }
}

void Simulation::promoteToLocalPhysics(const std::string& entityID) {
    auto it = physicsComponent.find(entityID);
    if (it == physicsComponent.end() || !it->second.platform) return;
    Platform* platform = it->second.platform;
    it->second.dynamicModel = &platform->dynamicModel;
    it->second.rigidbody    = &platform->rigidbody;
    it->second.collider     = &platform->collider;
    if (!it->second.aircraft) {
        it->second.aircraft = std::make_shared<Aircraft>();
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


        if(comp.collider && !fastupdate){
            comp.collider->Update(dt);
        }
        //fix:we are keeping dynamic only to server
        bool isMaster = true;
        if (networkManager && networkManager->isActive())
            isMaster = networkManager->isServer();
        // if (isMaster && comp.dynamicModel){
        if (isMaster && comp.dynamicModel && !comp.base->isRemoteDISEntity){//by new aman
            QElapsedTimer timer;
            timer.start();  // Start measuring
            comp.dynamicModel->Update(dt);
            updateDynamics(dt,&comp);
            qint64 elapsedMs = timer.elapsed();
            dynamicTime +=elapsedMs;
        }
        bool opt = true;
        if(physicsComponent.size()>70){
            // opt = simMin<num && num< simMax;
        }
        if(comp.base->type == Constants::EntityType::Weapon) {
            // Weapon* weapon = dynamic_cast<Weapon*>(comp.base);
            // if (weapon&&weapon->isLaunched&&!weapon->isDead) weapon->missileUpdate(dt);
        }
        if (comp.platform && opt){
            QElapsedTimer timer;
            timer.start();  // Start measuring
            comp.platform->update();
            qint64 elapsedMs = timer.elapsed();
            sensorTime +=elapsedMs;
        }

        if(comp.zone && !fastupdate){
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

        //     if (comp.rigidbody.Gravity) {
        //         body->setGravity(btVector3(Gravity->x, Gravity->y, Gravity->z));
        //     } else {
        //         body->setGravity(btVector3(0, 0, 0));
        //     }

        //     comp.rigidbody.velocity->x = body->getLinearVelocity().x();
        //     comp.rigidbody.velocity->y = body->getLinearVelocity().y();
        //     comp.rigidbody.velocity->z = body->getLinearVelocity().z();

        //     comp.rigidbody.angularVelocity->x = body->getAngularVelocity().x();
        //     comp.rigidbody.angularVelocity->y = body->getAngularVelocity().y();
        //     comp.rigidbody.angularVelocity->z = body->getAngularVelocity().z();

        //     btVector3 linearFactor(1, 1, 1);
        //     if (comp.rigidbody.freezePositionX) linearFactor.setX(0);
        //     if (comp.rigidbody.freezePositionY) linearFactor.setY(0);
        //     if (comp.rigidbody.freezePositionZ) linearFactor.setZ(0);
        //     body->setLinearFactor(linearFactor);

        //     btVector3 angularFactor(1, 1, 1);
        //     if (comp.rigidbody.freezeRotationX) angularFactor.setX(0);
        //     if (comp.rigidbody.freezeRotationY) angularFactor.setY(0);
        //     if (comp.rigidbody.freezeRotationZ) angularFactor.setZ(0);
        //     body->setAngularFactor(angularFactor);

        //     if (comp.rigidbody.Kinematics) {
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
            comp.transform->matrix.translation().x(),
            comp.transform->matrix.translation().y(),
            comp.transform->matrix.translation().z()));

        btQuaternion quat;
        quat.setEulerZYX(
            qDegreesToRadians(comp.transform->toEulerAngles().z()),
            qDegreesToRadians(comp.transform->toEulerAngles().y()),
            qDegreesToRadians(comp.transform->toEulerAngles().x()));
        trans.setRotation(quat);

        body->setWorldTransform(trans);

        if (comp.collider) {
            body->getCollisionShape()->setLocalScaling(btVector3(
                comp.transform->matrix.scale3D().x() * comp.collider->getDimension().Width * 0.5f,
                comp.transform->matrix.scale3D().y() * comp.collider->getDimension().Length * 0.5f,
                comp.transform->matrix.scale3D().z() * comp.collider->getDimension().Height * 0.5f));
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
