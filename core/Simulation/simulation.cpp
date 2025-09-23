
#include "simulation.h"
#include <algorithm>
#include <core/Debug/console.h>
#include <QJsonObject>
#include <QtMath>

Simulation::Simulation() {
    updateTimer = new QTimer(this);
    elapsedTimer = new QElapsedTimer();
    elapsedTimer->start();
    lastTime = elapsedTimer->elapsed();
    SimulationFrameRate = 260;
    PhysicsUpdateFrameRate = 60;
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
            frame();
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
}

void Simulation::frame() {
    qint64 currentTime = elapsedTimer->elapsed();
    deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
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
            {"x", comp.transform->toEulerAngles().x()},
            {"y", comp.transform->toEulerAngles().y()},
            {"z", comp.transform->toEulerAngles().z()}
        };

        entitiesArray.append(entityFrame);
    }

    frameData["entities"] = entitiesArray;
    frameData["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    if (recorder) {
        recorder->recordFrame(frameData);
    }

    QTimer::singleShot(0, this, [=]() {
        emit Update();
        emit Render(deltaTime * speed);
    });
    isPlay = true;
}

void Simulation::start() {
    lastTime = elapsedTimer->elapsed();
    updateTimer->start(1000 / SimulationFrameRate);
    isPlay = true;
}

void Simulation::pause() {
    updateTimer->stop();
    isPlay = false;
}

void Simulation::stop() {
    updateTimer->stop();
    isPlay = false;
    complete = true;
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
    frame();
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
        const QVector<QJsonObject>& frames = recorder->getRecordedFrames();
        replay(frames);
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
    Platform* platform = dynamic_cast<Platform*>(entity);
    if (!platform) {
        Console::error("Entity is not a Platform");
        return;
    }

    PhysicsComponent component;
    component.name = platform->Name;
    component.transform = platform->transform;
    component.rigidbody = platform->rigidbody;
    component.collider = platform->collider;
    component.dynamicModel = platform->dynamicModel;

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
            shape = new btSphereShape(platform->collider->Radius * platform->transform->matrix->scale3D().length());
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

    emit HierarchyUpdate();
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
        physicsComponent.erase(physIt);
    }

    emit HierarchyUpdate();
}

void Simulation::calculatePhysics() {
    const float dt = deltaTime * speed;
    const float maxDt = 1.0f / PhysicsUpdateFrameRate;
    const float clampedDt = std::min(dt, maxDt);

    dynamicsWorld->stepSimulation(clampedDt, 10);
    emit Physics();

    for (auto& [id, comp] : physicsComponent) {
        if (!comp.transform || !comp.rigidbody) continue;
        comp.rigidbody->deltaTime = deltaTime;
        if (comp.dynamicModel) comp.dynamicModel->Update(dt);

        auto it = bulletBodies.find(id);
        if (it != bulletBodies.end()) {
            btRigidBody* body = it->second;
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);

            btVector3 pos = trans.getOrigin();
            btQuaternion rot = trans.getRotation();
            // comp.transform->position->x = pos.getX();
            // comp.transform->position->y = pos.getY();
            // comp.transform->position->z = pos.getZ();

            btScalar x, y, z;
            rot.getEulerZYX(z, y, x);
            // comp.transform->rotation->x = qRadiansToDegrees(x);
            // comp.transform->rotation->y = qRadiansToDegrees(y);
            // comp.transform->rotation->z = qRadiansToDegrees(z);

            if (comp.collider) {
                body->getCollisionShape()->setLocalScaling(btVector3(
                    comp.transform->matrix->scale3D().x() * comp.collider->Width * 0.5f,
                    comp.transform->matrix->scale3D().y() * comp.collider->Length * 0.5f,
                    comp.transform->matrix->scale3D().z() * comp.collider->Height * 0.5f));
            }

            if (comp.rigidbody->Gravity) {
                body->setGravity(btVector3(Gravity->x, Gravity->y, Gravity->z));
            } else {
                body->setGravity(btVector3(0, 0, 0));
            }

            comp.rigidbody->velocity->x = body->getLinearVelocity().x();
            comp.rigidbody->velocity->y = body->getLinearVelocity().y();
            comp.rigidbody->velocity->z = body->getLinearVelocity().z();

            comp.rigidbody->angularVelocity->x = body->getAngularVelocity().x();
            comp.rigidbody->angularVelocity->y = body->getAngularVelocity().y();
            comp.rigidbody->angularVelocity->z = body->getAngularVelocity().z();

            btVector3 linearFactor(1, 1, 1);
            if (comp.rigidbody->freezePositionX) linearFactor.setX(0);
            if (comp.rigidbody->freezePositionY) linearFactor.setY(0);
            if (comp.rigidbody->freezePositionZ) linearFactor.setZ(0);
            body->setLinearFactor(linearFactor);

            btVector3 angularFactor(1, 1, 1);
            if (comp.rigidbody->freezeRotationX) angularFactor.setX(0);
            if (comp.rigidbody->freezeRotationY) angularFactor.setY(0);
            if (comp.rigidbody->freezeRotationZ) angularFactor.setZ(0);
            body->setAngularFactor(angularFactor);

            if (comp.rigidbody->Kinematics) {
                body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                body->setActivationState(DISABLE_DEACTIVATION);
            } else {
                body->setCollisionFlags(body->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
                body->setActivationState(ACTIVE_TAG);
            }
        }
    }
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
