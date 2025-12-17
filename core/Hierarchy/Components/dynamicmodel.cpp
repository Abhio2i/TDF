

#include "dynamicmodel.h"
#include <core/InputSystem/inputmanager.h>
#include "core/Hierarchy/Utils/entityutils.h"
#include "qjsonarray.h"
#include "qmetaobject.h"
#include <QDebug>

auto normalizeAngle = [](float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
};




DynamicModel::DynamicModel() {
    controle = true;
    follow = true;
    moveSpeed = 800;
    customParameters = QJsonObject(); // Initialize customParameters
}

void DynamicModel::init(){
    controle = true;

    follow = true;

    moveSpeed = 800;

    customParameters = QJsonObject(); // Initialize customParameters
    angdeg = transform->toEulerAngles().y();
}

void DynamicModel::start(){
    angdeg = transform->toEulerAngles().y();
}

void DynamicModel::Update(float deltaTime) {
    if (!controle || !transform || !rigidbody || !trajectory) return;

    // Direction vectors
    // QVector3D forwardDir = transform->forward(); // x-axis (forward)
    // QVector3D upDir = transform->up();           // z-axis (up)
    // QVector3D rightDir = transform->right();     // y-axis (right)
    time += deltaTime;
    delta = deltaTime;
    //qDebug() << time;
    if(startTime<time)
    {
        FollowTrajectory();
    }
}


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
    float metredis =  distanceBetween(target.x,target.z,current.x(),current.z());

    //if((trajectory->Trajectories.size()-1) == trajectory->current && distance <0.05f ) return;
    if (metredis > 100) {
        QVector3D last(current.x(), current.y(), current.z());

        current += transform->forward() * speeed * 0.0001f;
        float deltaDis = distanceBetween(last.x(),last.z(),current.x(),current.z());
        float time = 1/delta;
        currentSpeed = deltaDis*time;

        speeed += (currentSpeed<(moveSpeed/3.6f))?0.02f:-0.02f;
        //qDebug()<<currentSpeed;
        QVector3D targetAsQVector3D(target.x, target.y, target.z);
        QVector3D direction = targetAsQVector3D - current;

        direction = direction.normalized();

        float angleRad = atan2(direction.x(), direction.z());
        float angleDeg = angleRad * (180.0f / M_PI);

        float deltaang = normalizeAngle(angleDeg - angdeg);
        float turnrad = turnRadius>metredis?((metredis/turnRadius)* turnRadius * 0.5f):turnRadius;
        turnrad = turnrad<50?50:turnrad;
        double tangent_argument = pow(speeed, 2) / (turnrad * G_ACCELERATION);

        // 2. arctan का उपयोग करके बैंक कोण (रेडियन में) की गणना
        float bank_angle_radians = atan(tangent_argument);

        // 3. रेडियन को डिग्री में बदलना
        // 1 रेडियन = 180 / PI डिग्री
        float bank_angle_degrees = bank_angle_radians * (180.0 / M_PI);
        float f_ang = deltaang * bank_angle_degrees;
        angdeg += ((f_ang*50)>(60/50))?(60/50):f_ang;//1 * 0.04f;turnRadius
        //qDebug()<<deltaang * bank_angle_degrees;
        angdeg = normalizeAngle(angdeg);

        //angdeg = lerp(angdeg,angleDeg,moveSpeed * 0.004f
        transform->setFromEulerAngles(QVector3D(0,angdeg,0));
        // Smoothly interpolate between the current rotation and the target rotation.
        //*transform->rotation = QQuaternion::slerp(*transform->rotation, targetRotation, moveSpeed * 0.05f);
    }

    transform->setTranslation(current);

    if(distanceBetween(lastLat,lastLon,current.x(),current.z())>1){
        transform->trailData.push_back(current);
    }
    lastLat = current.x();
    lastLon = current.z();

    // ////////////////////////////////////////////////////
    // if( transform->trailData.capacity()>4000){
    //     transform->trailData.erase(transform->trailData.begin());
    // }
    ////////////////////////////////////////////////////
    //*transform->position = Vector::Lerp(*tran7sform->position, *trajectory->Trajectories[trajectory->current]->position, moveSpeed * 0.1);
    metredis = distanceBetween(trajectory->Trajectories[trajectory->current]->position->x,
                               trajectory->Trajectories[trajectory->current]->position->z,
                               current.x(),
                               current.z());
    if (trajectory->Trajectories.size() > trajectory->current && /*(transform->matrix->translation()).distanceToPoint(QVector3D(
                                                                     trajectory->Trajectories[trajectory->current]->position->x,
                                                                     trajectory->Trajectories[trajectory->current]->position->y,
                                                                     trajectory->Trajectories[trajectory->current]->position->z
                                                                     )*/ metredis
                                                                     < 1000) {
        trajectory->current += 1;
        //trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? (trajectory->Trajectories.size()-1) : trajectory->current;
        trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? 0: trajectory->current;
    }
}

float DynamicModel::lerp(float a, float b, float t){
    return a + (b - a) * t;
}


QString surfaceTypeToString(DynamicModel::TerrainSurface type) {
    switch (type) {
    case DynamicModel::TerrainSurface::Generic: return "Generic";
    case DynamicModel::TerrainSurface::Ground: return "Ground";
    case DynamicModel::TerrainSurface::Sand: return "Sand"; // Note: Typo, consider fixing to Cylinder
    case DynamicModel::TerrainSurface::Rock: return "Rock";
    default: return "Generic";
    }
}

DynamicModel::TerrainSurface stringTosurfaceType(const QString& str) {
    if (str == "Generic") return DynamicModel::TerrainSurface::Generic;
    if (str == "Ground") return DynamicModel::TerrainSurface::Ground;
    if (str == "Sand") return DynamicModel::TerrainSurface::Sand;
    if (str == "Rock") return DynamicModel::TerrainSurface::Rock;
    return DynamicModel::TerrainSurface::Generic; // Default
}

QStringList surfaceTypeOptions() {
    QStringList list;
    int index = DynamicModel::staticMetaObject.indexOfEnumerator("TerrainSurface");
    QMetaEnum metaEnum = DynamicModel::staticMetaObject.enumerator(index);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        list << QString::fromLatin1(metaEnum.key(i));
    }
    return list;
}


QJsonObject toParm(float value,QString unit){
    QJsonObject parm;
    parm["type"] = "unitParam";
    parm["value"] = value;
    parm["unit"] = unit;
    return parm;
}

float valueFromParm(const QJsonObject& parm) {
    if (parm.contains("value") ) {
        return parm["value"].toVariant().toDouble();
    }
    return 0.0f; // Default value if key is missing or not a double
}
QJsonObject DynamicModel::toJson() const {
    QJsonObject obj;
    obj["controle"] = controle;
    obj["moveSpeed"] = moveSpeed;
    obj["turnRadius"] = turnRadius;
    obj["start"] = startTime;
    obj["type"] = "component";

    QJsonObject maximumObj;
    maximumObj["type"] = "Section";
    maximumObj["minSpeed"] = toParm(minSpeed,"m/s");
    maximumObj["maxSpeed"] = toParm(maxSpeed,"m/s");
    maximumObj["Acceleration"] = toParm(Acceleration,"m/s^2");
    maximumObj["Decceleration"] = toParm(Decceleration,"m/s^2");
    maximumObj["turnRate"] = toParm(turnRate,"deg/s");
    maximumObj["Roll"] = toParm(Roll,"deg");
    maximumObj["Altitude"] = toParm(Altitude,"m");
    maximumObj["climbRate"] = toParm(climbRate,"m/s");
    maximumObj["diveRate"] = toParm(diveRate,"m/s");
    obj["maximums"] = maximumObj;

    QJsonObject responsesObj;
    responsesObj["type"] = "Section";
    responsesObj["deltaSpdCommandMaxAcc"] = toParm(deltaSpdCommandMaxAcc,"m/s");
    responsesObj["timeToReachMaxAcc"] = toParm(timeToReachMaxAcc,"s");
    responsesObj["deltaSpdCommandMaxDecel"] = toParm(deltaSpdCommandMaxDecel,"m/s");
    responsesObj["timeToReachMaxDecel"] = toParm(timeToReachMaxDecel,"s");
    responsesObj["deltaHdgCommandMaxRot"] = toParm(deltaHdgCommandMaxRot,"deg");
    responsesObj["timeToReachMaxRot"] = toParm(timeToReachMaxRot,"s");
    responsesObj["maximumPitchRate"] = toParm(maximumPitchRate,"deg/s");
    responsesObj["maximumRollRate"] = toParm(maximumRollRate,"deg/s");
    responsesObj["deltaToReachMaxROC"] = toParm(deltaToReachMaxROC,"m");
    responsesObj["deltaToReachMaxROD"] = toParm(deltaToReachMaxROD,"m");
    obj["responses"] = responsesObj;

    QJsonObject passabillityObj;
    passabillityObj["type"] = "Section";
        QJsonObject terrainSurfaceObj;
        terrainSurfaceObj["type"] = "option";
        QJsonArray optionsArray;
        for (const QString& opt : surfaceTypeOptions())optionsArray.append(opt);
        terrainSurfaceObj["options"] = optionsArray;
        terrainSurfaceObj["value"] = surfaceTypeToString(terrainSurface);
        passabillityObj["terrainSurface"] = terrainSurfaceObj;
    passabillityObj["maximumSpeed"] = toParm(maximumSpeed,"%");
    passabillityObj["terrainIsPassable"] = terrainIs;
    obj["passabillity"] = passabillityObj;


    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        //obj[it.key()] = it.value();
    }

    //qDebug() << "DynamicModel::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}

void DynamicModel::fromJson(const QJsonObject& obj) {
    //qDebug() << "DynamicModel::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // Standard fields
    if (obj.contains("controle"))
        controle = obj["controle"].toBool();
    if (obj.contains("moveSpeed"))
        moveSpeed = obj["moveSpeed"].toVariant().toDouble();
    if (obj.contains("turnRadius"))
        turnRadius = obj["turnRadius"].toVariant().toDouble();
    if (obj.contains("start"))
        startTime = obj["start"].toVariant().toDouble();

    // --- maximums Section ---
    if (obj.contains("maximums") && obj["maximums"].isObject()) {
        QJsonObject maximumObj = obj["maximums"].toObject();

        if (maximumObj.contains("minSpeed") && maximumObj["minSpeed"].isObject())
            minSpeed = valueFromParm(maximumObj["minSpeed"].toObject());
        if (maximumObj.contains("maxSpeed") && maximumObj["maxSpeed"].isObject())
            maxSpeed = valueFromParm(maximumObj["maxSpeed"].toObject());
        if (maximumObj.contains("Acceleration") && maximumObj["Acceleration"].isObject())
            Acceleration = valueFromParm(maximumObj["Acceleration"].toObject());
        if (maximumObj.contains("Decceleration") && maximumObj["Decceleration"].isObject())
            Decceleration = valueFromParm(maximumObj["Decceleration"].toObject());
        if (maximumObj.contains("turnRate") && maximumObj["turnRate"].isObject())
            turnRate = valueFromParm(maximumObj["turnRate"].toObject());
        if (maximumObj.contains("Roll") && maximumObj["Roll"].isObject())
            Roll = valueFromParm(maximumObj["Roll"].toObject());
        if (maximumObj.contains("Altitude") && maximumObj["Altitude"].isObject())
            Altitude = valueFromParm(maximumObj["Altitude"].toObject());
        if (maximumObj.contains("climbRate") && maximumObj["climbRate"].isObject())
            climbRate = valueFromParm(maximumObj["climbRate"].toObject());
        if (maximumObj.contains("diveRate") && maximumObj["diveRate"].isObject())
            diveRate = valueFromParm(maximumObj["diveRate"].toObject());
    }

    // --- responses Section ---
    if (obj.contains("responses") && obj["responses"].isObject()) {
        QJsonObject responsesObj = obj["responses"].toObject();

        if (responsesObj.contains("deltaSpdCommandMaxAcc") && responsesObj["deltaSpdCommandMaxAcc"].isObject())
            deltaSpdCommandMaxAcc = valueFromParm(responsesObj["deltaSpdCommandMaxAcc"].toObject());
        if (responsesObj.contains("timeToReachMaxAcc") && responsesObj["timeToReachMaxAcc"].isObject())
            timeToReachMaxAcc = valueFromParm(responsesObj["timeToReachMaxAcc"].toObject());
        if (responsesObj.contains("deltaSpdCommandMaxDecel") && responsesObj["deltaSpdCommandMaxDecel"].isObject())
            deltaSpdCommandMaxDecel = valueFromParm(responsesObj["deltaSpdCommandMaxDecel"].toObject());
        if (responsesObj.contains("timeToReachMaxDecel") && responsesObj["timeToReachMaxDecel"].isObject())
            timeToReachMaxDecel = valueFromParm(responsesObj["timeToReachMaxDecel"].toObject());
        if (responsesObj.contains("deltaHdgCommandMaxRot") && responsesObj["deltaHdgCommandMaxRot"].isObject())
            deltaHdgCommandMaxRot = valueFromParm(responsesObj["deltaHdgCommandMaxRot"].toObject());
        if (responsesObj.contains("timeToReachMaxRot") && responsesObj["timeToReachMaxRot"].isObject())
            timeToReachMaxRot = valueFromParm(responsesObj["timeToReachMaxRot"].toObject());
        if (responsesObj.contains("maximumPitchRate") && responsesObj["maximumPitchRate"].isObject())
            maximumPitchRate = valueFromParm(responsesObj["maximumPitchRate"].toObject());
        if (responsesObj.contains("maximumRollRate") && responsesObj["maximumRollRate"].isObject())
            maximumRollRate = valueFromParm(responsesObj["maximumRollRate"].toObject());
        if (responsesObj.contains("deltaToReachMaxROC") && responsesObj["deltaToReachMaxROC"].isObject())
            deltaToReachMaxROC = valueFromParm(responsesObj["deltaToReachMaxROC"].toObject());
        if (responsesObj.contains("deltaToReachMaxROD") && responsesObj["deltaToReachMaxROD"].isObject())
            deltaToReachMaxROD = valueFromParm(responsesObj["deltaToReachMaxROD"].toObject());
    }
    if (obj.contains("passabillity") && obj["passabillity"].isObject()) {
        QJsonObject passabillityObj = obj["passabillity"].toObject();
        if (passabillityObj.contains("terrainSurface") && passabillityObj["terrainSurface"].isObject()) {
            // QJsonObject terrainSurfaceObj = obj["terrainSurface"].toObject();
            QJsonObject terrainSurfaceObj = passabillityObj["terrainSurface"].toObject();
            if (terrainSurfaceObj.contains("value"))
                terrainSurface = stringTosurfaceType(terrainSurfaceObj["value"].toString());
        }
        if (passabillityObj.contains("maximumSpeed") && passabillityObj["maximumSpeed"].isObject())
            maximumSpeed = valueFromParm(passabillityObj["maximumSpeed"].toObject());
        if (passabillityObj.contains("terrainIsPassable") && passabillityObj["terrainIsPassable"].isBool())
            // terrainIs = passabillityObj["terrainIsPassable"].toVariant().toBool();

        terrainIs = passabillityObj["terrainIsPassable"].toBool();
    }



    // Custom parameters
    QStringList standardKeys = {
        "controle", "turnRadius", "start", "moveSpeed","responses","maximums","passabillity"
    };
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!standardKeys.contains(it.key())) {
            customParameters[it.key()] = it.value();
        }
    }

    //qDebug() << "DynamicModel::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
}

void DynamicModel::setMoveSpeed(float speed) {
    //moveSpeed = qBound(1.0f, speed, 10.0f);
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

