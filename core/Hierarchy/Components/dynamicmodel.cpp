/**
 * @file dynamicmodel.cpp
 * @brief Implementation of the DynamicModel component for entity motion and formation flying.
 */

#include "dynamicmodel.h"
#include <core/InputSystem/inputmanager.h>
#include <core/Simulation/simulation.h>
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/platform.h"  // Include here
#include "core/Hierarchy/Struct/formationposition.h" // Include here
#include <QtGlobal>
#include <cmath>
#include <QtMath>
#include "qjsonarray.h"
#include "qmetaobject.h"
#include <QDebug>

/**
 * @brief Normalizes an angle to the range [-180, 180] degrees.
 */
auto normalizeAngle = [](float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
};

/**
 * @brief Converts world velocity to local (body) velocity.
 * @param vEast     Velocity in east direction (m/s or knots)
 * @param vNorth    Velocity in north direction
 * @param vVertical Vertical velocity (climb rate)
 * @param rotation  Current rotation of the aircraft (quaternion)
 * @return QVector3D Local velocity (X = side, Y = up, Z = forward)
 */
QVector3D worldToLocalVelocity(float vEast, float vNorth, float vVertical, QQuaternion rotation)
{
    // 1. Prepare world velocity vector
    // According to Qt/OpenGL standard:
    // X = East, Y = Up (Vertical), Z = -North (forward direction is -Z)
    QVector3D globalVelocity(vEast, vVertical, -vNorth);

    // 2. Normalize rotation (required for mathematical precision)
    rotation.normalize();

    // 3. Transform from global to local
    // Using inverted() converts from world frame to body frame
    QVector3D localVel = rotation.inverted().rotatedVector(globalVelocity);

    return localVel;
}


/**
 * @brief Constructs a DynamicModel component.
 */
DynamicModel::DynamicModel():Component(nullptr) {
    control = true;
    follow = true;
    moveSpeed = 800;
    AdditionalParameters = QJsonObject(); // Initialize

}

DynamicModel::~DynamicModel(){
    if(formation){
        formation->clear();
    }

    if(formationPosition){
        formationPosition->entity = nullptr;
    }
}
/**
 * @brief Initializes the dynamic model, setting start time and default values.
 */
void DynamicModel::init(){
    if(startTime<Simulation::simulationTime){
        startTime = Simulation::simulationTime;
    }
    control = true;

    follow = true;

    moveSpeed = 800;

    AdditionalParameters = QJsonObject(); // Initialize
    angdeg = transform->toEulerAngles().y();
}

/**
 * @brief Called when the component starts; captures initial heading.
 */
void DynamicModel::start(){
    angdeg = transform->toEulerAngles().y();
    if(moveSpeed <minSpeed){
        moveSpeed = minSpeed;
    }
    if(moveSpeed > maxSpeed){
        moveSpeed = maxSpeed;
    }

}

/**
 * @brief Updates the dynamic model each frame, handling trajectory following.
 * @param deltaTime Time step in seconds.
 */
void DynamicModel::Update(float deltaTime) {
    if (!control || !transform || !rigidbody || !trajectory) return;

    time += deltaTime;
    delta = deltaTime;
    ////qDebug() << time;
    if(startTime<time)
    {
        FollowTrajectory();
    }
}


/**
 * @brief Implements trajectory following, formation keeping, and turn coordination.
 *        Handles both autonomous waypoint following and wingman formation flight.
 */
void DynamicModel::FollowTrajectory() {

    //============================================================================
    // Written by: Waris
    //============================================================================
    // --- 1. Physics & Damping Constants ---
    const float rotationSmoothFactor = 5.0f;
    const float G_ACCELERATION_VAL = 9.81f;
    const float dampingFactor = 0.90f;

    if (follow && followEntity && formationPosition && formationPosition->Offset) {
        Transform* mTransform = followEntity->transform;
        Platform* mPlatform = dynamic_cast<Platform*>(followEntity);
        DynamicModel* mModel = mPlatform ? mPlatform->dynamicModel : nullptr;

        if (mTransform && mModel && mPlatform->trajectory->getTargetWaypoint()&&mPlatform->trajectory->getTargetWaypoint()->formation) {
            // Vector offset = *formationPosition->Offset;
            // QVector3D real = mTransform->transformPoint(QVector3D(offset.x*30.f,offset.y,offset.z*30.f));
            // GeoPos cord = flatXYZToGeo(real.x(),real.y(),real.z());

            // transform->lookAt(real);
            // float speed = (mModel->currentSpeed/3600.f)*0.97f;
            // QVector3D pos = transform->translation();
            // pos += transform->forward()* speed * delta;

            // transform->setTranslation(pos);

            if(mModel->currentSpeed<10)return;
            // --- 2. TURN DETECTION & COMMITMENT SYSTEM ---
            float leaderHeading = mTransform->matrix->rotation().toEulerAngles().y();
            float myHeading = transform->matrix->rotation().toEulerAngles().y();

            // Detect if leader has started a new turn
            float leaderHeadingChange = std::abs(normalizeAngle(leaderHeading - lastLeaderHeading));

            if (!inActiveTurn && leaderHeadingChange > turnDetectionThreshold) {
                // NEW TURN DETECTED - Commit to turn type NOW
                inActiveTurn = true;
                turnStartHeading = myHeading;
                turnTargetHeading = leaderHeading;

                float initialTurnDelta = std::abs(normalizeAngle(leaderHeading - myHeading));

                // Classify turn type at START and commit to it
                if (initialTurnDelta < 45.0f) {
                    currentTurnIsTactical = false;  // CHECK turn
                } else if (initialTurnDelta > 170.0f) {
                    currentTurnIsTactical = false;  // HOOK turn
                } else {
                    currentTurnIsTactical = true;   // TACTICAL turn
                }
            }

            // Check if turn is complete
            if (inActiveTurn) {
                float remainingTurn = std::abs(normalizeAngle(leaderHeading - myHeading));
                if (remainingTurn < 3.0f) {  // Within 3° = turn complete
                    inActiveTurn = false;
                }
            }

            // Update leader heading tracker
            lastLeaderHeading = leaderHeading;

            // --- 3. APPLY COMMITTED TURN BEHAVIOR ---
            bool useTacticalLogic = inActiveTurn ? currentTurnIsTactical : false;
            float currentLookahead = useTacticalLogic ? 0.2f : 0.5f;

            // --- 4. PREDICTIVE GEOMETRY ---
            QVector3D mPos = mTransform->matrix->translation();
            mPos.setY(0);
            QVector3D mVel = mModel->velocity;
            QQuaternion mRot = mTransform->matrix->rotation();

            QVector3D mAngularVel = mModel->angularVelocity;
            QQuaternion rotPredict = QQuaternion::fromEulerAngles(mAngularVel * currentLookahead);
            QQuaternion predictedRot = mRot * rotPredict;

            QVector3D localOffset(
                formationPosition->Offset->x,
                formationPosition->Offset->y,
                formationPosition->Offset->z
                );

            QVector3D worldOffset = predictedRot.rotatedVector(localOffset);

            // --- 5. GET CURRENT POSITION ---
            QVector3D currentPos = transform->matrix->translation();
            currentPos.setY(0);

            // --- 6. TURN RADIUS COMPENSATION ---
            QVector3D leaderRight = mRot.rotatedVector(QVector3D(1, 0, 0));
            QVector3D offsetVector = currentPos - mPos;
            offsetVector.setY(0);
            float lateralPosition = QVector3D::dotProduct(offsetVector.normalized(), leaderRight);

            float turnDirection = mAngularVel.y();
            float speedModifier = 1.0f;
            float altitudeModifier = 0.0f;

            if (inActiveTurn && std::abs(turnDirection) > 0.1f) {
                float formationRadius = offsetVector.length();
                float leaderSpeed = mVel.length();
                float leaderTurnRadius = (leaderSpeed / (std::abs(mAngularVel.y()) * M_PI / 180.0f));

                bool isInsideWingman = (turnDirection * lateralPosition < 0);

                if (isInsideWingman) {
                    float insideTurnRadius = std::max(1.0f, leaderTurnRadius - formationRadius);
                    speedModifier = insideTurnRadius / leaderTurnRadius;
                    speedModifier = std::max(0.7f, speedModifier);
                    altitudeModifier = -5.0f * std::abs(turnDirection);
                } else {
                    float outsideTurnRadius = leaderTurnRadius + formationRadius;
                    speedModifier = outsideTurnRadius / leaderTurnRadius;
                    speedModifier = std::min(1.3f, speedModifier);
                    altitudeModifier = 5.0f * std::abs(turnDirection);
                }
            }

            // --- 7. TARGET POSITIONING ---
            QVector3D targetPos;

            if (inActiveTurn && useTacticalLogic) {
                targetPos = mPos + (mVel * currentLookahead) + worldOffset;
            } else {
                targetPos = mPos + worldOffset;
            }

            // --- 8. ARRIVAL BEHAVIOR WITH SPEED COMPENSATION ---
            QVector3D error = targetPos - currentPos;
            float distance = error.length();

            float mothershipSpeedMS = mVel.length();
            float mothershipSpeedKmh = mothershipSpeedMS * 3.6f;
            float maxSpeedMS;

            if (distance > 2000.0f) {
                // CATCH-UP MODE: Allow 1.5x mothership speed to catch up, but cap at own max speed
                float catchUpSpeed = std::min(mothershipSpeedMS * 1.5f, maxSpeed / 3.6f);
                catchUpSpeed = std::max(catchUpSpeed, moveSpeed / 3.6f); // at least own moveSpeed
                maxSpeedMS = catchUpSpeed * speedModifier;
            } else {
                // FORMATION MODE: Allow only +300 km/h extra for smooth following
                float maxAllowedMS = (mothershipSpeedKmh + 300.0f) / 3.6f;
                maxSpeedMS = std::min((moveSpeed / 3.6f) * speedModifier, maxAllowedMS);
            }

            // SLOWING RADIUS: Slow down only within the last 800m
            float slowingRadius = 800.0f;

            QVector3D desiredVel;
            if (distance > slowingRadius) {
                // Full speed until within slowing radius
                desiredVel = error.normalized() * maxSpeedMS;
            } else {
                // Linear slowdown over last 800m for smooth landing
                float t = distance / slowingRadius;
                desiredVel = error.normalized() * maxSpeedMS * t;
            }

            // Apply altitude compensation during turns
            if (inActiveTurn && std::abs(altitudeModifier) > 0.1f) {
                QVector3D currentPosWithY = transform->matrix->translation();
                float targetAltitude = mPos.y() + altitudeModifier;
                currentPosWithY.setY(currentPosWithY.y() + (targetAltitude - currentPosWithY.y()) * delta * 0.5f);
                transform->matrix->setTranslation(currentPosWithY);
            }

            // --- 9. PHYSICS & POSITION UPDATE ---
            QVector3D steering = (desiredVel - velocity) * dampingFactor;
            velocity += (steering / mass) * delta;




            // // Hard velocity clamp to prevent overshoot
            // if (velocity.length()*1000 > maxSpeed/3.6f) {
            //     velocity = velocity.normalized() * (maxSpeed/3600.f);
            // }

            // Hard velocity clamp to prevent overshoot
            if (velocity.length() > maxSpeedMS) {
                velocity = velocity.normalized() * maxSpeedMS;
            }

            if(velocity.length()>1.f){
                velocity =  velocity.normalized() * 1.f;
            }

            currentSpeed = velocity.length()*3600.0f;
            if(currentSpeed>maxSpeed){
                currentSpeed = maxSpeed;
            }

            // Update horizontal position
            QVector3D newPos = currentPos + velocity * delta;
            newPos.setY(transform->matrix->translation().y());  // Preserve Y from altitude compensation
            transform->matrix->setTranslation(newPos);

            // --- 10. 6-DOF ROTATION & BANKING ---
            if (velocity.lengthSquared() > 0.001f) {
                float targetYaw = atan2(velocity.x(), velocity.z()) * (180.0f / M_PI);
                QVector3D shipRight = QQuaternion::fromEulerAngles(0, targetYaw, 0).rotatedVector(QVector3D(1, 0, 0));
                float lateralForce = QVector3D::dotProduct(steering / mass, shipRight);

                float targetRoll = qBound(-60.0f, float(atan2(lateralForce, G_ACCELERATION_VAL) * (180.0 / M_PI)), 60.0f);
                float targetPitch = mTransform->matrix->rotation().toEulerAngles().x();

                QQuaternion targetRotation = QQuaternion::fromEulerAngles(targetPitch, targetYaw, targetRoll);
                QQuaternion currentRotation = transform->matrix->rotation();

                QQuaternion smoothedRot = QQuaternion::slerp(currentRotation, targetRotation, delta * rotationSmoothFactor);
                transform->setRotation(smoothedRot);
            }
            return;
        }
    }

    if(followTarget){
        return;
    }

    if(trajectory->Trajectories.size()<2) return;
    QVector3D current = transform->matrix->translation();
    QVector2D last(current.x(),current.z());
    QVector3D last3d(current.x(),current.y(),current.z());
    Vector target = *trajectory->getTargetWaypoint()->position;
    float tgtSpd = trajectory->getTargetWaypoint()->speed;
    FlatXYZ targt = geoToFlatXYZ(target.x,target.z,target.y);
    QVector3D target_qvec(targt.x, targt.y, targt.z);
    QVector2D tar(target_qvec.x(),target_qvec.z());

    QVector3D diff = target_qvec-transform->translation();

    // Standard Heading (Yaw): using atan2(x, z) because forward is Z+
    float heading = std::atan2(diff.x(), diff.z()) * (180.0f / M_PI);
    heading = heading-transform->getHeading();
    heading = heading<0?(-1*heading):heading;
    // qDebug()<<heading-transform->getHeading();

    // if(target_qvec.y() != Altitude){
    //     Altitude = target_qvec.y();
    // }

    float movespd = (moveSpeed/3600.0f);//km/h to km/s


    transform->trailData.push_back(QVector3D(transform->getLatitude(),0,transform->getLongitude()));
    if( transform->trailData.capacity()>54000){
        transform->trailData.erase(transform->trailData.begin());
    }
    // ////////////////////////////////////////////////////
    current.setZ(transform->getLatitude());
    current.setX(transform->getLongitude());
    float metredis = distanceBetween(trajectory->Trajectories[trajectory->current]->position->x,
                                     trajectory->Trajectories[trajectory->current]->position->z,
                                     current.z(),
                                     current.x());
    bool pass = false;
    if(heading>40){
        pass = metredis < calculateTurnRadius(currentSpeed,turnRate)*1.5f;
    }else{
        pass = metredis < (currentSpeed/3.6f) *2.f;
    }


    // //qDebug()<<metredis;
    if (trajectory->Trajectories.size() > trajectory->current && pass){// metredis < calculateTurnRadius(currentSpeed,turnRate)*1.5f){//(currentSpeed/3.6f) *2.f) {
        if(trajectory->reverse){
            trajectory->current -= 1;
            if(trajectory->current <= 0){
                endTime = time;
                moveSpeed = 0;
                Altitude = 0;
            }
            trajectory->current = trajectory->current <= 0 ? 0: trajectory->current;
        }else{
            trajectory->current += 1;
            // //qDebug()<<"time :"<<time;
            if(trajectory->rule == Trajectory::FollowRule::Stop&& trajectory->current >= trajectory->Trajectories.size()){
                endTime = time;
                moveSpeed = 0;
                Altitude = 0;
                return;
            }
            if(trajectory->rule == Trajectory::FollowRule::Reverse && trajectory->current >= trajectory->Trajectories.size()){
                trajectory->reverse = true;
                trajectory->current = trajectory->current-1;
            }else{
                trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? 0: trajectory->current;
            }
            if(tgtSpd > 0 || parentEntity->category == Entity::Category::Marine){
                Vector target = *trajectory->getTargetWaypoint()->position;
                //float tgtSpd = trajectory->getTargetWaypoint()->speed;
                tgtSpd = tgtSpd>maxSpeed?maxSpeed:tgtSpd;
                FlatXYZ targt = geoToFlatXYZ(target.x,target.z,target.y);
                QVector3D target_qvec(targt.x, targt.y, targt.z);
                if(tgtSpd > 0){
                    moveSpeed = tgtSpd;
                }
                if(moveSpeed <minSpeed){
                    moveSpeed = minSpeed;
                }
                if(moveSpeed > maxSpeed){
                    moveSpeed = maxSpeed;
                }
                if(Altitude>maxAltitude){
                    Altitude = maxAltitude;
                }
                // if(Altitude<0){
                //     Altitude = 0;
                // }
                //movespd = (tgtSpd/3600.0f);//km/h to km/s

                if(target_qvec.y() > 0 || parentEntity->category == Entity::Category::Marine){
                    //alt = target_qvec.y() * FTtoKM;
                    Altitude = target_qvec.y();
                }
            }
        }


    }

    if(parentEntity && (parentEntity->category == Entity::Category::Marine ||
                         parentEntity->category == Entity::Category::Marine ||
                         parentEntity->category == Entity::Category::Ground ))
    {
        moveSpeed = moveSpeed > 500?500:moveSpeed;
    }else{
        moveSpeed = moveSpeed>maxSpeed?maxSpeed:moveSpeed;
    }
    if(parentEntity->category == Entity::Category::Marine){
        Altitude  = Altitude>0?0:Altitude;
    }
}

void DynamicModel::TimeJump(int sec){

     if(trajectory->Trajectories.size()<2) return;
    float time = 0;
    float dis = 0;
    float alt = Altitude;
    float speed = moveSpeed*(5.f/18.f);
    int waynum = trajectory->current;
    int leftsec = 0;
    for(int i = 0;i<100;i++){
        float metredis = distanceBetween(trajectory->Trajectories[trajectory->current]->position->x,
                                         trajectory->Trajectories[trajectory->current]->position->z,
                                         transform->getLatitude(),
                                         transform->getLongitude());
        int taketime = metredis/speed;
        if(taketime+time>sec){
            leftsec = sec-time;
            break;
        }
        time+=taketime;
        dis = metredis;
        Waypoints* wp= trajectory->getCurrentWaypoint();
        moveSpeed = speed*3.6f;
        Altitude = alt;
        if(moveSpeed <minSpeed){
            moveSpeed = minSpeed;
        }
        if(moveSpeed > maxSpeed){
            moveSpeed = maxSpeed;
        }
        if(Altitude>maxAltitude){
            Altitude = maxAltitude;
        }

        if(parentEntity && (parentEntity->category == Entity::Category::Marine ||
                             parentEntity->category == Entity::Category::Marine ||
                             parentEntity->category == Entity::Category::Ground ))
        {
            moveSpeed = moveSpeed > 500?500:moveSpeed;
        }else{
            moveSpeed = moveSpeed>maxSpeed?maxSpeed:moveSpeed;
        }
        if(parentEntity->category == Entity::Category::Marine){
            Altitude  = Altitude>0?0:Altitude;
        }
        if(wp && moveSpeed>0){
            speed = moveSpeed*(5.f/18.f);
        }
        if(wp && Altitude>0){
            alt = Altitude;
        }

        transform->setGeoCord(trajectory->Trajectories[trajectory->current]->position->x,trajectory->Trajectories[trajectory->current]->position->z,Altitude);
        transform->trailData.push_back(QVector3D(transform->getLatitude(),0,transform->getLongitude()));
        if( transform->trailData.capacity()>54000){
            transform->trailData.erase(transform->trailData.begin());
        }
        trajectory->current+=1;
        if(trajectory->current >= trajectory->Trajectories.size()){
            trajectory->current = 0;
        }
        waynum = trajectory->current;
    }
    moveSpeed = speed*3.6f;
    Altitude = alt;
    transform->lookAt(trajectory->Trajectories[trajectory->current]->position->x,trajectory->Trajectories[trajectory->current]->position->z);
    float leftDis = (leftsec*speed)/1000.f;
    float rad = transform->getHeading()*(M_PI /180.0f);
    float x = leftDis * std::sin(rad);
    float z = leftDis * std::cos(rad);
    transform->setTranslation(transform->translation()+QVector3D(x,0,z));

}



/**
 * @brief Linear interpolation helper.
 */
float DynamicModel::lerp(float a, float b, float t){
    return a + (b - a) * t;
}


/**
 * @brief Converts TerrainSurface enum to string.
 */
QString surfaceTypeToString(DynamicModel::TerrainSurface type) {
    switch (type) {
    case DynamicModel::TerrainSurface::Generic: return "Generic";
    case DynamicModel::TerrainSurface::Ground: return "Ground";
    case DynamicModel::TerrainSurface::Sand: return "Sand"; // Note: Typo, consider fixing to Cylinder
    case DynamicModel::TerrainSurface::Rock: return "Rock";
    default: return "Generic";
    }
}

/**
 * @brief Converts string to TerrainSurface enum.
 */
DynamicModel::TerrainSurface stringTosurfaceType(const QString& str) {
    if (str == "Generic") return DynamicModel::TerrainSurface::Generic;
    if (str == "Ground") return DynamicModel::TerrainSurface::Ground;
    if (str == "Sand") return DynamicModel::TerrainSurface::Sand;
    if (str == "Rock") return DynamicModel::TerrainSurface::Rock;
    return DynamicModel::TerrainSurface::Generic; // Default
}

/**
 * @brief Returns list of terrain surface option strings for UI.
 */
QStringList surfaceTypeOptions() {
    QStringList list;
    int index = DynamicModel::staticMetaObject.indexOfEnumerator("TerrainSurface");
    QMetaEnum metaEnum = DynamicModel::staticMetaObject.enumerator(index);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        list << QString::fromLatin1(metaEnum.key(i));
    }
    return list;
}


/**
 * @brief Adds a sub‑component (unused for DynamicModel).
 */
void DynamicModel::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){

}

/**
 * @brief Removes a sub‑component (unused).
 */
void DynamicModel::removeSubComponent(std::string ID){

}

/**
 * @brief Updates a sub‑component (unused).
 */
void DynamicModel::updateSubComponent(std::string ID, const QJsonObject& obj){

}

/**
 * @brief Gets sub‑component data (unused).
 */
QJsonObject DynamicModel::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

/**
 * @brief Serializes the DynamicModel component to JSON.
 * @return QJsonObject containing all dynamic model parameters.
 */
QJsonObject DynamicModel::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["Advanced"] = true;
    obj["control"] = control;
    // obj["moveSpeed"] = moveSpeed;
    // obj["turnRadius"] = turnRadius;
    obj["takeoff"] = toParm(startTime,"s");
    obj["type"] = "component";

    QJsonObject maximumObj;
    maximumObj["type"] = "Section";
     maximumObj["visible"] = true;
    maximumObj["minSpeed"] = toParm(minSpeed,"Km/h",0,300);
    maximumObj["maxSpeed"] = toParm(maxSpeed,"Km/h",100,2000);
    maximumObj["moveSpeed"] = toParm(moveSpeed,"Km/h",0,2000);
    maximumObj["Acceleration"] = toParm(Acceleration,"m/s^2",10,500,"i am Accerlation");
    maximumObj["Decceleration"] = toParm(Decceleration,"m/s^2",50,    400);
    // maximumObj["turnRadius"] = toParm(turnRadius,"m");
    if(parentEntity->category == Entity::Category::Marine){
        maximumObj["turnRate"] = toParm(turnRate,"deg/s",5,15);
    }else{
        maximumObj["turnRate"] = toParm(turnRate,"deg/s",5,30);
    }

    if(parentEntity->category == Entity::Category::Marine){
        maximumObj["MaxDepth"] = toParm(maxAltitude,"ft",0,-2000);
    }else{
        maximumObj["MaxAltitude"] = toParm(maxAltitude,"ft",10,60000);
    }


    if(parentEntity->category == Entity::Category::Marine)
        maximumObj["Depth"] = toParm(Altitude,"ft",-2000,0);
    else
        maximumObj["Altitude"] = toParm(Altitude,"ft",10,60000);

    if(parentEntity->category == Entity::Category::Marine){
        maximumObj["climbRate"] = toParm(climbRate,"ft/min",0,      100);
        maximumObj["diveRate"] = toParm(diveRate,"ft/min",0,      100);
    }else{
        maximumObj["climbRate"] = toParm(climbRate,"ft/min",0,      10000);
        maximumObj["diveRate"] = toParm(diveRate,"ft/min",0,      10000);
    }

    obj["maximums"] = maximumObj;

    QJsonObject responsesObj;
    responsesObj["type"] = "Section";
    responsesObj["deltaSpdCommandMaxAcc"] = toParm(deltaSpdCommandMaxAcc,"m/s",0,    200);
    responsesObj["timeToReachMaxAcc"] = toParm(timeToReachMaxAcc,"s",0,    30);
    responsesObj["deltaSpdCommandMaxDecel"] = toParm(deltaSpdCommandMaxDecel,"m/s",  0,    200);
    responsesObj["timeToReachMaxDecel"] = toParm(timeToReachMaxDecel,"s",    0,    30);
    responsesObj["deltaHdgCommandMaxRot"] = toParm(deltaHdgCommandMaxRot,"deg",  0,    180);
    responsesObj["timeToReachMaxRot"] = toParm(timeToReachMaxRot,"s",    0,    30);
    responsesObj["maximumPitchRate"] = toParm(maximumPitchRate,"deg/s",0,    60);
    responsesObj["maximumRollRate"] = toParm(maximumRollRate,"deg/s",0,    120);
    responsesObj["deltaToReachMaxROC"] = toParm(deltaToReachMaxROC,"m",    0,    5000);
    responsesObj["deltaToReachMaxROD"] = toParm(deltaToReachMaxROD,"m",    0,    5000);
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
    passabillityObj["maximumSpeed"] = toParm(maximumSpeed,"%",0, 1000);
    passabillityObj["terrainIsPassable"] = terrainIs;
    obj["passabillity"] = passabillityObj;


    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    ////qDebug() << "DynamicModel::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}

/**
 * @brief Deserializes the DynamicModel component from JSON.
 * @param obj JSON object containing dynamic model data.
 */
void DynamicModel::fromJson(const QJsonObject& obj) {
    ////qDebug() << "DynamicModel::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

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
        if (maximumObj.contains("turnRate") && maximumObj["turnRate"].isObject())
            turnRate = valueFromParm(maximumObj["turnRate"].toObject());
        if(parentEntity->category == Entity::Category::Marine){
            if (maximumObj.contains("MaxDepth") && maximumObj["MaxAltitude"].isObject())
                maxAltitude = valueFromParm(maximumObj["MaxAltitude"].toObject());
        }else{
            if (maximumObj.contains("MaxAltitude") && maximumObj["MaxAltitude"].isObject())
                maxAltitude = valueFromParm(maximumObj["MaxAltitude"].toObject());
        }
        if(parentEntity->category == Entity::Category::Marine){
            if (maximumObj.contains("Depth") && maximumObj["Depth"].isObject())
                Altitude = valueFromParm(maximumObj["Depth"].toObject());
        }else{
            if (maximumObj.contains("Altitude") && maximumObj["Altitude"].isObject())
                Altitude = valueFromParm(maximumObj["Altitude"].toObject());
        }

        if (maximumObj.contains("climbRate") && maximumObj["climbRate"].isObject())
            climbRate = valueFromParm(maximumObj["climbRate"].toObject());
        if (maximumObj.contains("diveRate") && maximumObj["diveRate"].isObject())
            diveRate = valueFromParm(maximumObj["diveRate"].toObject());

        if(moveSpeed <minSpeed){
            moveSpeed = minSpeed;
        }
        if(moveSpeed > maxSpeed){
            moveSpeed = maxSpeed;
        }
        if(Altitude>maxAltitude){
            Altitude = maxAltitude;
        }
        if(Altitude<0){
            Altitude = 0;
        }
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

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }

    ////qDebug() << "DynamicModel::fromJson :" << QJsonDocument().toJson(QJsonDocument::Compact);
}

/**
 * @brief Sets the movement speed (clamped to valid range).
 * @param speed Desired speed.
 */
void DynamicModel::setMoveSpeed(float speed) {
    //moveSpeed = qBound(1.0f, speed, 10.0f);
}

double DynamicModel::calculateTurnRadius(double speedKmph, double turnRateDegPerSec) {
    // Safe check: Agar turn rate 0 hai ya bohot kam hai, toh radius infinite hoga (straight line)
    if (std::abs(turnRateDegPerSec) < 1e-6) {
        return 0.0; // Ya fir safely std::numeric_limits<double>::infinity() return karein
    }

    // Constant expression: (50.0 / PI) approx 15.9154943
    constexpr double conversionConstant = 50.0 / M_PI;

    return (speedKmph / turnRateDegPerSec) * conversionConstant;
}
