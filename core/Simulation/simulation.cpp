#include "simulation.h"
#include <algorithm>
Simulation::Simulation() {
    updateTimer = new QTimer(this);
    elapsedTimer = new QElapsedTimer();
    elapsedTimer->start();
    lastTime = elapsedTimer->elapsed();
    SimulationFrameRate = 260;
    speed = 1;
    connect(updateTimer, &QTimer::timeout, this, [=]() {
        frame();
    });
    //start();
    // updateTimer->start(1); // ~100 FPS, adjust as needed
    //🆕 Bullet World Initialization
    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, 0, -3.81f));


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
}

void  Simulation::frame(){
    qint64 currentTime = elapsedTimer->elapsed();
    this->deltaTime  = (currentTime - lastTime) / 1000.0f; // in seconds
    lastTime = currentTime;
    // Optional: emit deltaTime if needed somewhere else
    //emit DeltaTimeUpdated(deltaTime);
    calculatePhysics();
    QTimer::singleShot(0, this, [=]() {
        emit this->Render(deltaTime*speed);
    });
    isPlay = true;
}

void Simulation::start(){
    lastTime = elapsedTimer->elapsed();
    updateTimer->start(1000/SimulationFrameRate);
}

void Simulation::stop(){
    updateTimer->stop();
    isPlay = false;
}

void Simulation::setSpeed(float value){
    speed = value;
}

void Simulation::nextStep(){
    frame();
}

void Simulation::entityAdded(QString parentID, Entity* entity) {
    PhysicsComponent component;
    component.name = entity->Name;
    component.transform = entity->transform;
    component.rigidbody = entity->rigidbody;
    component.collider = entity->collider;
    component.dynamicModel = entity->dynamicModel;

    physicsComponent[entity->ID] = component;

    // Bullet body creation
    if (entity->transform && entity->rigidbody) {
        btCollisionShape* shape = nullptr;

        if (entity->collider && entity->collider->collider == Constants::ColliderType::Box) {
            //Vector* size = entity->transform->size;
            shape = new btBoxShape(btVector3(1,1,1));
            shape->setLocalScaling(btVector3(component.transform->size->x * component.collider->Width * 0.5f,
                                                                 component.transform->size->y * component.collider->Length * 0.5f,
                                                                 component.transform->size->z * component.collider->Height * 0.5f));
        } else if (entity->collider && entity->collider->collider == Constants::ColliderType::Sphere) {
            shape = new btSphereShape(entity->collider->Radius * entity->transform->size->magnitude());
        }


        if (shape) {
            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(btVector3(entity->transform->position->x, entity->transform->position->y, entity->transform->position->z));
            btQuaternion quat;
            quat.setEulerZYX(
                qDegreesToRadians(component.transform->rotation->z),  // yaw
                qDegreesToRadians(component.transform->rotation->y),  // pitch
                qDegreesToRadians(component.transform->rotation->x)   // roll
                );
            transform.setRotation(quat);

            float mass = entity->rigidbody->Mass;
            btVector3 inertia(0, 0, 0);
            if (mass > 0)
                shape->calculateLocalInertia(mass, inertia);

            btDefaultMotionState* motionState = new btDefaultMotionState(transform);
            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, inertia);
            btRigidBody* body = new btRigidBody(rbInfo);

            // Gravity Control
            if (!entity->rigidbody->Gravity) {
                body->setGravity(btVector3(0, 0, 0)); // Gravity OFF
            }else{

            }

            // Kinematic Control
            if (entity->rigidbody->Kinematics) {
                body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                body->setActivationState(DISABLE_DEACTIVATION);
            }

            body->setLinearVelocity(btVector3(entity->rigidbody->velocity->x,
                                              entity->rigidbody->velocity->y,
                                              entity->rigidbody->velocity->z));

            body->setAngularVelocity(btVector3(entity->rigidbody->angularVelocity->x,
                                              entity->rigidbody->angularVelocity->y,
                                              entity->rigidbody->angularVelocity->z));

            connect(entity->rigidbody,&Rigidbody::setLinearVel,this,[body](const Vector& velocity){
                body->setLinearVelocity(btVector3(velocity.x,velocity.y,velocity.z));
            });

            connect(entity->rigidbody, &Rigidbody::setAngularVel, this, [body](const Vector& velocity){
                body->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
            });


            // Freeze Axes
            btVector3 linearFactor(1, 1, 1);
            if (entity->rigidbody->freezePositionX) linearFactor.setX(0);
            if (entity->rigidbody->freezePositionY) linearFactor.setY(0);
            if (entity->rigidbody->freezePositionZ) linearFactor.setZ(0);
            body->setLinearFactor(linearFactor);

            btVector3 angularFactor(1, 1, 1);
            if (entity->rigidbody->freezeRotationX) angularFactor.setX(0);
            if (entity->rigidbody->freezeRotationY) angularFactor.setY(0);
            if (entity->rigidbody->freezeRotationZ) angularFactor.setZ(0);
            body->setAngularFactor(angularFactor);

            dynamicsWorld->addRigidBody(body);
            bulletBodies[entity->ID] = body;
        }
    }
}

void Simulation::entityRemoved(QString ID) {
    std::string key = ID.toStdString();

    // First check if bullet rigid body exists
    auto bulletIt = bulletBodies.find(key);
    if (bulletIt != bulletBodies.end()) {
        btRigidBody* body = bulletIt->second;

        // Remove from dynamics world
        if (body) {
            dynamicsWorld->removeRigidBody(body);

            // Clean up motion state
            if (body->getMotionState()) {
                delete body->getMotionState();
            }

            // Clean up collision shape
            if (body->getCollisionShape()) {
                delete body->getCollisionShape();
            }

            // Clean up the body itself
            delete body;
        }

        // Remove from bulletBodies map
        bulletBodies.erase(bulletIt);
    }

    // Now remove from physicsComponent
    auto physIt = physicsComponent.find(key);
    if (physIt != physicsComponent.end()) {
        physicsComponent.erase(physIt);
    }
}

void Simulation::calculatePhysics() {
    const float dt = deltaTime * speed;
    const float maxDt = 1.0f / 60.0f;
    const float clampedDt = std::min(dt, maxDt);

    // Step the bullet physics world
    dynamicsWorld->stepSimulation(clampedDt, 10);

    // Sync Bullet world transforms back to our Entity transforms
    for (auto& [id, comp] : physicsComponent) {
        if (!comp.transform) continue;
        comp.rigidbody->deltaTime = deltaTime;
        comp.dynamicModel->Update(deltaTime);

        auto it = bulletBodies.find(id);
        if (it != bulletBodies.end()) {
            btRigidBody* body = it->second;
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);

            btVector3 pos = trans.getOrigin();
            btQuaternion rot = trans.getRotation();
            comp.transform->position->x = pos.getX();
            comp.transform->position->y = pos.getY();
            comp.transform->position->z = pos.getZ();

            // Correct quaternion -> Euler conversion
            btScalar x, y, z;
            rot.getEulerZYX(z, y, x);

            comp.transform->rotation->x = qRadiansToDegrees(x); // Pitch (X)
            comp.transform->rotation->y = qRadiansToDegrees(y);   // Yaw (Y)
            comp.transform->rotation->z = qRadiansToDegrees(z);  // Roll (Z)

            body->getCollisionShape()->setLocalScaling(btVector3(comp.transform->size->x * comp.collider->Width * 0.5f,
                                                                 comp.transform->size->y * comp.collider->Length * 0.5f,
                                                                 comp.transform->size->z * comp.collider->Height) * 0.5f);


            // 1. Gravity Update
            if (comp.rigidbody->Gravity) {
                body->setGravity(dynamicsWorld->getGravity());
            } else {
                body->setGravity(btVector3(0, 0, 0)); // Gravity OFF
            }

            // Convert btVector3 to Vector
            comp.rigidbody->velocity->x = body->getLinearVelocity().x();
            comp.rigidbody->velocity->y = body->getLinearVelocity().y();
            comp.rigidbody->velocity->z = body->getLinearVelocity().z();

            // Similarly for angular velocity
            comp.rigidbody->angularVelocity->x = body->getAngularVelocity().x();
            comp.rigidbody->angularVelocity->y = body->getAngularVelocity().y();
            comp.rigidbody->angularVelocity->z = body->getAngularVelocity().z();

            // 2. Freeze Axis Update
            btVector3 linearFactor(1, 1, 1);
            if (comp.rigidbody->freezePositionX) linearFactor.setX(0);
            if (comp.rigidbody->freezePositionY) linearFactor.setY(0);
            if (comp.rigidbody->freezePositionZ) linearFactor.setZ(0);
            body->setLinearFactor(linearFactor);

            // btVector3 angularFactor(1, 1, 1);
            // if (comp.rigidbody->freezeRotationX) angularFactor.setX(0);
            // if (comp.rigidbody->freezeRotationY) angularFactor.setY(0);
            // if (comp.rigidbody->freezeRotationZ) angularFactor.setZ(0);
            // body->setAngularFactor(angularFactor);

            // 3. Kinematics Update
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
    if (physIt != physicsComponent.end()) {

        auto bulletIt = bulletBodies.find(key);
        if (bulletIt != bulletBodies.end()) {
            btRigidBody* body = bulletIt->second;
            PhysicsComponent& comp = physIt->second;

            if (body && comp.transform) {
                btTransform trans;
                body->getMotionState()->getWorldTransform(trans);

                // Update position and rotation
                trans.setOrigin(btVector3(
                    comp.transform->position->x,
                    comp.transform->position->y,
                    comp.transform->position->z
                    ));

                btQuaternion quat;
                quat.setEulerZYX(
                    qDegreesToRadians(comp.transform->rotation->z),  // yaw
                    qDegreesToRadians(comp.transform->rotation->y),  // pitch
                    qDegreesToRadians(comp.transform->rotation->x)   // roll
                    );
                trans.setRotation(quat);

                // Apply updated transform
                body->setWorldTransform(trans);

                body->getCollisionShape()->setLocalScaling(btVector3(comp.transform->size->x * comp.collider->Width * 0.5f,
                                                                     comp.transform->size->y * comp.collider->Length * 0.5f,
                                                                     comp.transform->size->z * comp.collider->Height) * 0.5f);

                btQuaternion rot = trans.getRotation();
                btScalar roll, pitch, yaw;
                rot.getEulerZYX(yaw, pitch, roll);
                // qDebug()<<"x:"<<qRadiansToDegrees(roll)<<
                //          " y:"<<qRadiansToDegrees(pitch)<<
                //          " z:"<<qRadiansToDegrees(yaw);

                // Convert btVector3 to Vector
                comp.rigidbody->velocity->x = body->getLinearVelocity().x();
                comp.rigidbody->velocity->y = body->getLinearVelocity().y();
                comp.rigidbody->velocity->z = body->getLinearVelocity().z();

                // Similarly for angular velocity
                comp.rigidbody->angularVelocity->x = body->getAngularVelocity().x();
                comp.rigidbody->angularVelocity->y = body->getAngularVelocity().y();
                comp.rigidbody->angularVelocity->z = body->getAngularVelocity().z();


                if (body->getMotionState()) {
                    body->getMotionState()->setWorldTransform(trans);
                }
            }
        }
    }
}


