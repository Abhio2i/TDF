

#include "dynamicmodel.h"
#include <core/InputSystem/inputmanager.h>
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/platform.h"  // Include here
#include "core/Hierarchy/Struct/formationposition.h" // Include here
#include <QtGlobal>
#include <cmath>
#include "qjsonarray.h"
#include "qmetaobject.h"
#include <QDebug>

auto normalizeAngle = [](float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
};

DynamicModel::DynamicModel():Component(nullptr) {
    control = true;
    follow = true;
    moveSpeed = 800;
    customParameters = QJsonObject(); // Initialize customParameters
}

void DynamicModel::init(){
    control = true;

    follow = true;

    moveSpeed = 800;

    customParameters = QJsonObject(); // Initialize customParameters
    angdeg = transform->toEulerAngles().y();
}

void DynamicModel::start(){
    angdeg = transform->toEulerAngles().y();
}

void DynamicModel::Update(float deltaTime) {
    if (!control || !transform || !rigidbody || !trajectory) return;

    time += deltaTime;
    delta = deltaTime;
    //qDebug() << time;
    if(startTime<time)
    {
        FollowTrajectory();
    }
}


void DynamicModel::FollowTrajectory() {

    // // dynamic perfect but fly fully

    // // --- 1. Realistic Physics Constants ---
    // const float Kp = 2.5f;                       // Aggressiveness of position correction
    // const float rotationSmoothFactor = 8.0f;    // How fast the nose turns (Yaw)
    // const float rollSmoothFactor = 4.5f;         // How fast the wings tilt (Roll)
    // const float G_ACCELERATION = 9.81f;          // Earth Gravity
    // const float lookaheadTime = 0.2f;            // Seconds to project into the future
    // const float pitchCompensationFactor = 0.08f; // Nose-up pull to maintain lift in turns
    // const float G_ACCELERATION_VAL = 9.81f;

    // if (follow && followEntity && formationPosition && formationPosition->Offset) {
    //     Transform* mTransform = followEntity->transform;
    //     Platform* mPlatform = dynamic_cast<Platform*>(followEntity);
    //     DynamicModel* mModel = mPlatform ? mPlatform->dynamicModel : nullptr;

    //     if (mTransform && mModel) {
    //         // --- 2. PREDICTIVE GEOMETRY (The "Pro" Secret) ---
    //         // We don't follow where the leader IS. We follow where the leader WILL BE.
    //         QVector3D mPos = mTransform->matrix->translation();
    //         mPos.setY(0);
    //         QVector3D mVel = mModel->velocity;
    //         QQuaternion mRot = mTransform->rotation();

    //         // Predict the Leader's Future Heading (Angular Velocity)
    //         // This ensures the wingman starts turning *with* the leader
    //         QVector3D mAngularVel = mModel->angularVelocity;
    //         QQuaternion rotPredict = QQuaternion::fromEulerAngles(mAngularVel * lookaheadTime);
    //         QQuaternion predictedRot = mRot * rotPredict;

    //         // Calculate the 3D Slot (Meters) relative to predicted orientation
    //         QVector3D localOffset(
    //             formationPosition->Offset->x,
    //             formationPosition->Offset->y,
    //             formationPosition->Offset->z
    //             );
    //         QVector3D worldOffset = predictedRot.rotatedVector(localOffset);

    //         // Target Position = Future Leader Center + Predicted Offset
    //         QVector3D targetPos = (mPos + mVel * lookaheadTime) + worldOffset;

    //         // --- 3. SMOOTH STEERING ---
    //         QVector3D currentPos = transform->matrix->translation();
    //         currentPos.setY(0);
    //         QVector3D error = targetPos - currentPos;

    //         // PID-style Velocity calculation
    //         QVector3D desiredVel = error * Kp;
    //         float maxSpeedMS = moveSpeed / 3.6f; // Convert km/h to m/s
    //         if (desiredVel.length() > maxSpeedMS) {
    //             desiredVel = desiredVel.normalized() * maxSpeedMS;
    //         }

    //         QVector3D steering = desiredVel - velocity;
    //         QVector3D force = steering / mass;
    //         velocity += force * delta;

    //         // Final Position Update
    //         transform->setTranslation(currentPos + velocity * delta);

    //         // 7. Handle 6-DOF Rotation
    //         if (velocity.lengthSquared() > 0.001f) {
    //             QVector3D currentEuler = transform->toEulerAngles();

    //             // Calculate Yaw based on velocity vector in the flat plane
    //             float targetYaw = atan2(velocity.x(), velocity.z()) * (180.0f / M_PI);
    //             float smoothedYaw = lerp(currentEuler.y(), targetYaw, delta * rotationSmoothFactor);

    //             // Calculate Bank/Roll based on centrifugal force (centripetal acceleration)
    //             QVector3D shipRight = QQuaternion::fromEulerAngles(0, smoothedYaw, 0).rotatedVector(QVector3D(1, 0, 0));
    //             float lateralForce = QVector3D::dotProduct(steering / mass, shipRight);

    //             // Bank angle = atan(v^2 / (rg)) -> derived from lateral force
    //             float targetRoll = qBound(-65.0f, float(atan2(lateralForce, G_ACCELERATION_VAL) * (180.0 / M_PI)), 65.0f);
    //             float smoothedRoll = lerp(currentEuler.z(), targetRoll, delta * rollSmoothFactor);

    //             // Keep pitch relative to leader's pitch to look natural
    //             float targetPitch = mTransform->toEulerAngles().x();
    //             float smoothedPitch = lerp(currentEuler.x(), targetPitch, delta * rotationSmoothFactor);

    //             transform->setFromEulerAngles(QVector3D(smoothedPitch, smoothedYaw, smoothedRoll));
    //         }
    //     }
    // }

    // --- 1. Refined Physics Constants for Smoothness almost working ---
    const float Kp = 1.2f;                       // Lowered from 2.0 to stop "snapping"/oscillations
    const float rotationSmoothFactor = 5.0f;     // Slightly slower for heavy aircraft feel
    const float rollSmoothFactor = 3.5f;         // Slower roll for cinematic turns
    const float G_ACCELERATION_VAL = 9.81f;
    const float lookaheadTime = 1.5f;            // Increased lookahead to anticipate turns earlier
    const float dampingFactor = 0.95f;           // Helps smooth out jittery velocity changes

    if (follow && followEntity && formationPosition && formationPosition->Offset) {
        Transform* mTransform = followEntity->transform;
        Platform* mPlatform = dynamic_cast<Platform*>(followEntity);
        DynamicModel* mModel = mPlatform ? mPlatform->dynamicModel : nullptr;

        if (mTransform && mModel) {
            // --- 2. PREDICTIVE GEOMETRY ---
            QVector3D mPos = mTransform->matrix->translation();
            mPos.setY(0);
            QVector3D mVel = mModel->velocity;
            QQuaternion mRot = mTransform->rotation();

            // Predict where the leader will be based on their current turn rate (Angular Velocity)
            QVector3D mAngularVel = mModel->angularVelocity;
            QQuaternion rotPredict = QQuaternion::fromEulerAngles(mAngularVel * lookaheadTime);
            QQuaternion predictedRot = mRot * rotPredict;

            // Calculate the slot position in the future
            QVector3D localOffset(
                formationPosition->Offset->x,
                formationPosition->Offset->y,
                formationPosition->Offset->z
                );
            QVector3D worldOffset = predictedRot.rotatedVector(localOffset);

            // Target Position = Predicted Leader Pos + Predicted World Offset
            QVector3D targetPos = (mPos + mVel * lookaheadTime) + worldOffset;

            // --- 3. SMOOTH STEERING (Anti-Oscillation) ---
            QVector3D currentPos = transform->matrix->translation();
            currentPos.setY(0);
            QVector3D error = targetPos - currentPos;

            // Calculate desired velocity but blend it with current velocity to dampen "springing"
            QVector3D desiredVel = error * Kp;
            float maxSpeedMS = moveSpeed / 3.6f;
            if (desiredVel.length() > maxSpeedMS) {
                desiredVel = desiredVel.normalized() * maxSpeedMS;
            }

            // Apply damping to steering to prevent the left-right jitter
            QVector3D steering = (desiredVel - velocity) * dampingFactor;
            QVector3D force = steering / mass;
            velocity += force * delta;

            // Final Position Update
            transform->setTranslation(currentPos + velocity * delta);

            // --- 4. 6-DOF ROTATION (Using Slerp for Smoothness) ---
            if (velocity.lengthSquared() > 0.001f) {
                // Determine heading from movement
                float targetYaw = atan2(velocity.x(), velocity.z()) * (180.0f / M_PI);

                // Calculate roll from lateral steering force
                QVector3D shipRight = QQuaternion::fromEulerAngles(0, targetYaw, 0).rotatedVector(QVector3D(1, 0, 0));
                float lateralForce = QVector3D::dotProduct(steering / mass, shipRight);

                // Limit the bank to 60 degrees for stability
                float targetRoll = qBound(-60.0f, float(atan2(lateralForce, G_ACCELERATION_VAL) * (180.0 / M_PI)), 60.0f);

                // Match leader's pitch (usually 0 or slight climb)
                float targetPitch = mTransform->toEulerAngles().x();

                // SLERP (Spherical Linear Interpolation) handles the 0/360 wrap-around perfectly
                QQuaternion targetRotation = QQuaternion::fromEulerAngles(targetPitch, targetYaw, targetRoll);
                QQuaternion currentRotation = transform->rotation();

                QQuaternion smoothedRot = QQuaternion::slerp(currentRotation, targetRotation, delta * rotationSmoothFactor);
                transform->setRotation(smoothedRot);
            }

            return; // Stay in Follow Mode logic only
        }
    }

    if(trajectory->Trajectories.size()<2) return;
    QVector3D current = transform->matrix->translation();
    // current.setZ(transform->getLatitude());
    // current.setX(transform->getLongitude());
    Vector target = *trajectory->Trajectories[trajectory->current]->position;
    FlatXYZ targt = geoToFlatXYZ(target.x,target.z,target.y);
    QVector3D target_qvec(targt.x, targt.y, targt.z);
    float movespd = moveSpeed/3600.0f;//km/h to km/s
    float accel = Acceleration/1000.0f;//m/s to km/s
    float dccel = Decceleration/1000.0f;//m/s to km/s
    float alt = Altitude * FTtoKM;//ft to km
    float clmbrate = climbRate * FTminToKMs;//ft/min to km/s
    float divrate = diveRate * FTminToKMs;//ft/min to km/s
    currentAltitude = current.y();
    if(target_qvec.y() > 0){
        alt = target_qvec.y() * FTtoKM;
        Altitude = target_qvec.y();
    }
    if(speed<movespd){
        speed += accel*delta;
    }else{
        speed -= dccel*delta;
    }
    // qDebug()<<currentAltitude<<","<<alt;
    if(currentAltitude<alt){
        currentAltitude += clmbrate*delta;
    }else{
        currentAltitude -= divrate*delta;
    }
    QVector3D direction = target_qvec - current;
    direction = direction.normalized();

    float angleRad = atan2(direction.x(), direction.z());
    float angleDeg = angleRad * (180.0f / M_PI);
    float deltaang = normalizeAngle(angleDeg - angdeg);
    float dis = current.distanceToPoint(target_qvec)*1000;
    float turnrad = turnRadius>dis?((dis/turnRadius)* turnRadius * 0.9f):turnRadius;
    turnrad = turnrad<50?50:turnrad;
    double tangent_argument = pow(speed, 2) / (turnrad * G_ACCELERATION);
    float bank_angle_radians = atan(tangent_argument);
    float bank_angle_degrees = bank_angle_radians * (180.0 / M_PI);
    float f_ang = deltaang * bank_angle_degrees;
    angdeg += ((f_ang*50)>(60/50))?(60/50):f_ang;//1 * 0.04f;turnRadius
    angdeg = normalizeAngle(angdeg);
    transform->setFromEulerAngles(QVector3D(0,angdeg,0));

    // transform->lookAt(target_qvec);
    current += transform->forward() * speed * delta;
    current.setY(currentAltitude);
    transform->setTranslation(current);
    currentSpeed = speed*3600;

    // // ////////////////////////////////////////////////////
    // // if( transform->trailData.capacity()>4000){
    // //     transform->trailData.erase(transform->trailData.begin());
    // // }
    // ////////////////////////////////////////////////////
    // //*transform->position = Vector::Lerp(*tran7sform->position, *trajectory->Trajectories[trajectory->current]->position, moveSpeed * 0.1);
    current.setZ(transform->getLatitude());
    current.setX(transform->getLongitude());
    float metredis = distanceBetween(trajectory->Trajectories[trajectory->current]->position->x,
                               trajectory->Trajectories[trajectory->current]->position->z,
                               current.z(),
                               current.x());
    // qDebug()<<metredis;
    if (trajectory->Trajectories.size() > trajectory->current &&  metredis < 100) {
        trajectory->current += 1;
        qDebug()<<"time :"<<time;
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



void DynamicModel::addSubComponent(std::string name, QString data1, QString data2, QString data3){

}

void DynamicModel::removeSubComponent(std::string ID){

}

void DynamicModel::updateSubComponent(std::string ID, const QJsonObject& obj){

}

QJsonObject DynamicModel::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

QJsonObject DynamicModel::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["control"] = control;
    // obj["moveSpeed"] = moveSpeed;
    // obj["turnRadius"] = turnRadius;
    obj["takeoff"] = toParm(startTime,"s");
    obj["type"] = "component";

    QJsonObject maximumObj;
    maximumObj["type"] = "Section";
    // maximumObj["minSpeed"] = toParm(minSpeed,"Km/h");
    // maximumObj["maxSpeed"] = toParm(maxSpeed,"Km/h");
    maximumObj["moveSpeed"] = toParm(moveSpeed,"Km/h");
    maximumObj["Acceleration"] = toParm(Acceleration,"m/s^2");
    maximumObj["Decceleration"] = toParm(Decceleration,"m/s^2");
    maximumObj["turnRadius"] = toParm(turnRadius,"m");
    // maximumObj["Roll"] = toParm(Roll,"deg");
    maximumObj["Altitude"] = toParm(Altitude,"ft");
    maximumObj["climbRate"] = toParm(climbRate,"ft/min");
    maximumObj["diveRate"] = toParm(diveRate,"ft/min");
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
    //obj["responses"] = responsesObj;

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
    //obj["passabillity"] = passabillityObj;


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
    if (obj.contains("control"))
        control = obj["control"].toBool();
    // if (obj.contains("moveSpeed"))
    //     moveSpeed = obj["moveSpeed"].toVariant().toDouble();
    // if (obj.contains("turnRadius"))
    //     turnRadius = obj["turnRadius"].toVariant().toDouble();
    if (obj.contains("takeoff"))
        startTime = valueFromParm(obj["takeoff"].toObject());

    // --- maximums Section ---
    if (obj.contains("maximums") && obj["maximums"].isObject()) {
        QJsonObject maximumObj = obj["maximums"].toObject();

        if (maximumObj.contains("minSpeed") && maximumObj["minSpeed"].isObject())
            minSpeed = valueFromParm(maximumObj["minSpeed"].toObject());
        if (maximumObj.contains("maxSpeed") && maximumObj["maxSpeed"].isObject())
            maxSpeed = valueFromParm(maximumObj["maxSpeed"].toObject());
        if (maximumObj.contains("moveSpeed") && maximumObj["moveSpeed"].isObject())
            moveSpeed = valueFromParm(maximumObj["moveSpeed"].toObject());
        if (maximumObj.contains("Acceleration") && maximumObj["Acceleration"].isObject())
            Acceleration = valueFromParm(maximumObj["Acceleration"].toObject());
        if (maximumObj.contains("Decceleration") && maximumObj["Decceleration"].isObject())
            Decceleration = valueFromParm(maximumObj["Decceleration"].toObject());
        if (maximumObj.contains("turnRadius") && maximumObj["turnRadius"].isObject())
            turnRadius = valueFromParm(maximumObj["turnRadius"].toObject());
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
        "control", "turnRadius", "start", "moveSpeed","responses","maximums","passabillity"
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
