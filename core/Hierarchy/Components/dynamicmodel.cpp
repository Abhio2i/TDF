
#include "dynamicmodel.h"
#include <core/InputSystem/inputmanager.h>
#include <QDebug>
#include <cmath>

DynamicModel::DynamicModel() {
    controle = true;
    follow = true;
    moveSpeed = 1;
    customParameters = QJsonObject(); // Initialize customParameters
}

void DynamicModel::Update(float deltaTime) {
    if (!controle || !transform || !rigidbody || !trajectory) return;

    // Direction vectors
    QVector3D forwardDir = transform->forward(); // x-axis (forward)
    QVector3D upDir = transform->up();           // z-axis (up)
    QVector3D rightDir = transform->right();     // y-axis (right)
    time += deltaTime;
    //qDebug() << time;
    if(start<time)
    {
        FollowTrajectory();
    }
}

// void DynamicModel::Update(float deltaTime) {
//     if (!controle || !transform || !trajectory) return;

//     // Get input from InputManager (or use autopilot values if `follow` is true)
//     float throttleInput = 1.0f;
//     float pitchInput = 0.0f;
//     float rollInput = 0.0f;
//     float yawInput = 0.0f;
//     bool brakes = false;

// /*    // Get input from InputManager (assuming it exists and provides values)
//     float throttleInput = InputManager::getThrottleInput(); // Assuming a value between 0 and 1
//     float pitchInput = InputManager::getPitchInput();     // Assuming a value between -1 and 1
//     float rollInput = InputManager::getRollInput();       // Assuming a value between -1 and 1
//     float yawInput = InputManager::getYawInput();         // Assuming a value between -1 and 1
//     bool brakes = InputManager::getAirBrakes();     */      // Assuming a boolean value


//     // --- Autopilot Logic to follow trajectory ---
//     if (follow && trajectory->Trajectories.size() > 0) {
//         // Get the current and target positions
//         QVector3D currentPosition = transform->translation();
//         Vector targetVector = *trajectory->Trajectories[trajectory->current]->position;
//         QVector3D targetPosition(targetVector.x, targetVector.y, targetVector.z);

//         // Check if we have reached the current waypoint
//         if (currentPosition.distanceToPoint(targetPosition) < 5.0f) { // Use a small radius
//             trajectory->current++;
//             if (trajectory->current >= trajectory->Trajectories.size()) {
//                 trajectory->current = 0; // Loop the trajectory
//             }
//         }

//         // Calculate the direction to the next waypoint
//         QVector3D directionToTarget = (targetPosition - currentPosition).normalized();
//         QVector3D localDirection = transform->inverseTransformDirection(directionToTarget);

//         // Calculate pitch and yaw inputs from the local direction
//         pitchInput = -localDirection.y() * 2.0f; // Scale factor for responsiveness
//         yawInput = localDirection.x() * 2.0f;   // Scale factor for responsiveness

//         // Auto-leveling for roll
//         rollInput = -transform->toEulerAngles().z() * autoRollLevel;
//     }

//     // Direction vectors based on the aircraft's current orientation
//     QVector3D forwardDir = transform->forward();
//     QVector3D upDir = transform->up();
//     QVector3D rightDir = transform->right();

//     // 1. Calculate and apply forces
//     QVector3D totalForce(0.0f, 0.0f, 0.0f);

//     // Thrust
//     enginePower = throttleInput * maxEnginePower;
//     totalForce += forwardDir * enginePower;

//     // Drag
//     forwardSpeed = velocity.length();
//     float drag = forwardSpeed * forwardSpeed * dragIncreaseFactor;
//     if (brakes) {
//         drag += forwardSpeed * airBrakesEffect;
//     }
//     if (forwardSpeed > 0) {
//         totalForce -= velocity.normalized() * drag;
//     }

//     // Lift
//     float lift = forwardSpeed * forwardSpeed * Lift;
//     if (forwardSpeed < zeroLiftSpeed) {
//         lift = 0;
//     }
//     totalForce += upDir * lift;

//     // Gravity (assuming a constant downward force)
//     //totalForce += QVector3D(0.0f, -9.8f, 0.0f); // Adjust gravity as needed

//     // 2. Update velocity and position
//     QVector3D acceleration = totalForce / mass;
//     velocity += acceleration * deltaTime;
//     transform->addTranslation(velocity * deltaTime);

//     // 3. Calculate and apply torques
//     QVector3D totalTorque(0.0f, 0.0f, 0.0f);
//     QVector3D localVelocity = transform->inverseTransformDirection(velocity);

//     // Pitch control (up/down)
//     float pitchTorque = pitchInput * pitchEffect*500;
//     pitchTorque += -localVelocity.y() * autoPitchLevel; // Auto-leveling
//     totalTorque += rightDir * pitchTorque * aerodynamicEffect;

//     qDebug()<<pitchInput <<","<<totalTorque;
//     // Roll control (left/right)
//     float rollTorque = rollInput * rollEffect*500;
//     rollTorque += -localVelocity.x() * autoRollLevel; // Auto-leveling
//     totalTorque += forwardDir * rollTorque * aerodynamicEffect;

//     // Yaw control (left/right turning)
//     float yawTorque = yawInput * yawEffect*500;
//     yawTorque += -rollInput * bankedTurnEffect;
//     totalTorque += upDir * yawTorque * aerodynamicEffect;

//     // 4. Update angular velocity and rotation
//     angularVelocity += totalTorque * deltaTime * rotationSpeed;
//     transform->setRotation(transform->rotation() * QQuaternion::fromEulerAngles(angularVelocity * deltaTime));

// }

void DynamicModel::FollowTrajectory() {

    if (follow) {
        // QVector3D current = *transform->position;

        // // 🎯 Step 1: Base target is followEntity's position
        // QVector3D target = *followEntity->transform->position;

        // // 🎯 Step 2: Apply Offset if formationPosition is available
        // if (formationPosition && formationPosition->Offset) {
        //     //target += *formationPosition->Offset;
        //     target += QVector3D(formationPosition->Offset->x, formationPosition->Offset->y, formationPosition->Offset->z);
        // }

        // // 🔁 Step 3: Move towards target
        // QVector3D diff = target - current;
        // float distance = diff.length();

        // if (distance > 0.001f) {
        //     QVector3D dir = diff.normalized();
        //     current += dir * moveSpeed * 0.01f;
        // }

        // *transform->position = current;

        // // 🔄 Step 4: Update rotation
        // QVector3D direction = target - current;
        // if (direction.length() > 0.001f) {
        //     direction = direction.normalized();
        //     float angleRad = atan2(direction.y(), direction.x());
        //     float angleDeg = angleRad * (180.0f / M_PI);
        //     *transform->rotation = QQuaternion::fromAxisAndAngle(angleDeg, 0, 0, 1);
        // }

        // return; // Skip trajectory logic
    }
    if(trajectory->Trajectories.size()<2) return;
    QVector3D current = transform->matrix->translation();
    Vector target = *trajectory->Trajectories[trajectory->current]->position;
    QVector3D target_qvec(target.x, target.y, target.z);
    QVector3D diff = target_qvec - current;
    float distance = diff.length();
    if((trajectory->Trajectories.size()-1) == trajectory->current && distance <0.05f ) return;
    if (distance > 0.01f) {
        QVector3D dir = diff.normalized();
        current += transform->forward() * moveSpeed * 0.01f;
        QVector3D targetAsQVector3D(target.x, target.y, target.z);
        QVector3D direction = targetAsQVector3D - current;

        direction = direction.normalized();

        float angleRad = atan2(direction.x(), direction.z());
        float angleDeg = angleRad * (180.0f / M_PI);

        angdeg = lerp(angdeg,angleDeg,moveSpeed * 0.004f);
        // current.setZ(current.z()-(sin(angdeg)*moveSpeed*0.01f));
        // current.setX(current.x()+(cos(angdeg)*moveSpeed*0.01f));
        //*transform->rotation = QQuaternion::fromAxisAndAngle(angdeg, 0.0f, 1.0f, 0.0f);
        //QQuaternion targetRotation = QQuaternion::fromEulerAngles(QVector3D(0,-angdeg,0));
        transform->setFromEulerAngles(QVector3D(0,angdeg,0));
        // Smoothly interpolate between the current rotation and the target rotation.
        //*transform->rotation = QQuaternion::slerp(*transform->rotation, targetRotation, moveSpeed * 0.05f);
    }
    transform->setTranslation(current);
    //*transform->position = Vector::Lerp(*transform->position, *trajectory->Trajectories[trajectory->current]->position, moveSpeed * 0.1);

    if (trajectory->Trajectories.size() > trajectory->current && (transform->matrix->translation()).distanceToPoint(QVector3D(
                                                                     trajectory->Trajectories[trajectory->current]->position->x,
                                                                     trajectory->Trajectories[trajectory->current]->position->y,
                                                                     trajectory->Trajectories[trajectory->current]->position->z
                                                                     ))
    < 1) {
        trajectory->current += 1;
        //trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? (trajectory->Trajectories.size()-1) : trajectory->current;
        trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? 0: trajectory->current;
    }
}

float DynamicModel::lerp(float a, float b, float t){
    return a + (b - a) * t;
}

QJsonObject DynamicModel::toJson() const {
    QJsonObject obj;
    obj["controle"] = controle;
    obj["maxEnginePower"] = maxEnginePower;
    obj["Lift"] = Lift;
    obj["zeroLiftSpeed"] = zeroLiftSpeed;
    obj["moveSpeed"] = moveSpeed;
    obj["start"] = start;
    obj["rotationSpeed"] = rotationSpeed;
    obj["dragIncreaseFactor"] = dragIncreaseFactor;
    obj["aerodynamicEffect"] = aerodynamicEffect;
    obj["airBrakesEffect"] = airBrakesEffect;
    obj["rollEffect"] = rollEffect;
    obj["pitchEffect"] = pitchEffect;
    obj["yawEffect"] = yawEffect;
    obj["bankedTurnEffect"] = bankedTurnEffect;
    obj["autoRollLevel"] = autoRollLevel;
    obj["autoPitchLevel"] = autoPitchLevel;
     obj["type"] = "component";

    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    //qDebug() << "DynamicModel::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}

void DynamicModel::fromJson(const QJsonObject& obj) {
    //qDebug() << "DynamicModel::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // Standard fields
    if (obj.contains("controle"))
        controle = obj["controle"].toBool();
    if (obj.contains("maxEnginePower"))
        maxEnginePower = obj["maxEnginePower"].toVariant().toDouble();
    if (obj.contains("Lift"))
        Lift = obj["Lift"].toVariant().toDouble();
    if (obj.contains("zeroLiftSpeed"))
        zeroLiftSpeed = obj["zeroLiftSpeed"].toVariant().toDouble();
    if (obj.contains("moveSpeed"))
        moveSpeed = obj["moveSpeed"].toVariant().toDouble();
    if (obj.contains("start"))
        start = obj["start"].toVariant().toDouble();
    if (obj.contains("rotationSpeed"))
        rotationSpeed = obj["rotationSpeed"].toVariant().toDouble();
    if (obj.contains("dragIncreaseFactor"))
        dragIncreaseFactor = obj["dragIncreaseFactor"].toVariant().toDouble();
    if (obj.contains("aerodynamicEffect"))
        aerodynamicEffect = obj["aerodynamicEffect"].toVariant().toDouble();
    if (obj.contains("airBrakesEffect"))
        airBrakesEffect = obj["airBrakesEffect"].toVariant().toDouble();
    if (obj.contains("rollEffect"))
        rollEffect = obj["rollEffect"].toVariant().toDouble();
    if (obj.contains("pitchEffect"))
        pitchEffect = obj["pitchEffect"].toVariant().toDouble();
    if (obj.contains("yawEffect"))
        yawEffect = obj["yawEffect"].toVariant().toDouble();
    if (obj.contains("bankedTurnEffect"))
        bankedTurnEffect = obj["bankedTurnEffect"].toVariant().toDouble();
    if (obj.contains("autoRollLevel"))
        autoRollLevel = obj["autoRollLevel"].toVariant().toDouble();
    if (obj.contains("autoPitchLevel"))
        autoPitchLevel = obj["autoPitchLevel"].toVariant().toDouble();

    // Custom parameters
    QStringList standardKeys = {
        "controle", "maxEnginePower", "Lift", "zeroLiftSpeed", "moveSpeed",
        "rotationSpeed", "dragIncreaseFactor", "aerodynamicEffect", "airBrakesEffect",
        "rollEffect", "pitchEffect", "yawEffect", "bankedTurnEffect",
        "autoRollLevel", "autoPitchLevel"
    };
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!standardKeys.contains(it.key())) {
            customParameters[it.key()] = it.value();
        }
    }

    //qDebug() << "DynamicModel::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
}

void DynamicModel::setMoveSpeed(float speed) {
    moveSpeed = qBound(1.0f, speed, 10.0f);
}
