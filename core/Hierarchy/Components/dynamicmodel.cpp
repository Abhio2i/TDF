

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

auto normalizeAngle = [](float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
};

/**
 * @brief World Velocity को Local (Body) Velocity में बदलने का फंक्शन
 * @param vEast     - पूर्व दिशा में वेग (m/s या Knots)
 * @param vNorth    - उत्तर दिशा में वेग
 * @param vVertical - ऊपर की ओर वेग (Climb Rate)
 * @param rotation  - एयरक्राफ्ट का मौजूदा रोटेशन (Quaternion)
 * @return QVector3D - Local Velocity (X=Side, Y=Up, Z=Forward)
 */
QVector3D worldToLocalVelocity(float vEast, float vNorth, float vVertical, QQuaternion rotation)
{
    // 1. वर्ल्ड वेलोसिटी वेक्टर तैयार करें
    // Qt/OpenGL स्टैंडर्ड के अनुसार:
    // X = East, Y = Up (Vertical), Z = -North (सामने की दिशा -Z होती है)
    QVector3D globalVelocity(vEast, vVertical, -vNorth);

    // 2. रोटेशन को नॉर्मलाइज़ करें (गणितीय सटीकता के लिए ज़रूरी)
    rotation.normalize();

    // 3. Global से Local में ट्रांसफ़ॉर्म करें
    // inverted() का उपयोग करने से हम वर्ल्ड फ्रेम से बॉडी फ्रेम में आ जाते हैं
    QVector3D localVel = rotation.inverted().rotatedVector(globalVelocity);

    return localVel;
}


DynamicModel::DynamicModel():Component(nullptr) {
    control = true;
    follow = true;
    moveSpeed = 800;
    customParameters = QJsonObject(); // Initialize customParameters
}

void DynamicModel::init(){
    if(startTime<Simulation::simulationTime){
        startTime = Simulation::simulationTime;
    }
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
    ////qDebug() << time;
    if(startTime<time)
    {
        FollowTrajectory();
    }
}


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
                // CATCH-UP MODE: Mothership speed ka 1.5x allow karo taaki formation ban sake
                // moveSpeed ki bhi cap lagao taaki unrealistic na ho
                // max() se ensure karo ki mothership se zyada speed mile
                // min() se ensure karo ki apni max speed se aage na jaaye
                float catchUpSpeed = std::min(mothershipSpeedMS * 1.5f, maxSpeed / 3.6f);
                catchUpSpeed = std::max(catchUpSpeed, moveSpeed / 3.6f); // at least apni moveSpeed
                maxSpeedMS = catchUpSpeed * speedModifier;
            } else {
                // FORMATION MODE: Sirf 300 km/h extra allow karo — smooth follow ke liye
                float maxAllowedMS = (mothershipSpeedKmh + 300.0f) / 3.6f;
                maxSpeedMS = std::min((moveSpeed / 3.6f) * speedModifier, maxAllowedMS);
            }

            // SLOWING RADIUS: Sirf last 800m pe slowdown hoga
            // Pehle distance*40% tha — 10km door ho toh 4km se slow hona shuru — bahut zyada tha
            // Ab fixed 800m — door se full speed, paas aake smooth brake
            float slowingRadius = 800.0f;

            QVector3D desiredVel;
            if (distance > slowingRadius) {
                // Full speed se aao jab tak 800m door ho
                desiredVel = error.normalized() * maxSpeedMS;
            } else {
                // Last 800m mein linear slowdown — smooth landing
                // t*t (quadratic) hataya kyunki bahut slow ho jaata tha
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

            // FIX 3: Hard velocity clamp
            // Pehle velocity accumulate hoti rehti thi — koi upper limit nahi tha
            // Isse physics velocity badh jaati thi aur entity target se aage nikal jaati thi (overshoot)
            if (velocity.length() > maxSpeedMS) {
                velocity = velocity.normalized() * maxSpeedMS;
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
    // if(moveSpeed <minSpeed){
    //     moveSpeed = minSpeed;
    // }
    // if(moveSpeed > maxSpeed){
    //     moveSpeed = maxSpeed;
    // }
    // if(Altitude>maxAltitude){
    //     Altitude = maxAltitude;
    // }
    // if(Altitude<0){
    //     Altitude = 0;
    // }
    float movespd = (moveSpeed/3600.0f);//km/h to km/s
    // float accel = (Acceleration/1000.0f);//m/s to km/s
    // float dccel = (Decceleration/1000.0f);//m/s to km/s
    // float alt = Altitude * FTtoKM;//ft to km
    // float clmbrate = climbRate * FTminToKMs;//ft/min to km/s
    // float divrate = diveRate * FTminToKMs;//ft/min to km/s
    // currentAltitude = current.y();



    // float diffspeed = std::abs(speed-movespd);
    // float lastspeed = speed;
    // if(currentSpeed < 300){

    //     // alt = 10*FTtoKM;
    //     // divrate = 9.8f/1000.0f;
    // }

    // if(speed<movespd){
    //     if(diffspeed<accel){
    //         accel = 0.5f*diffspeed;
    //     }
    //     speed += accel*delta;
    // }else{
    //     if(diffspeed<dccel){
    //         dccel = 0.5f*diffspeed;
    //     }
    //     speed -= dccel*delta;
    // }
    // // float deltaSpeed = std::abs(speed - lastspeed)*1000 * (1/delta);
    // float distance = last.distanceToPoint(tar)*1000;
    // float offset = movespd*1000;//active current speed
    // // //qDebug()<<distance<<","<<deltaSpeed ;
    // if((speed*1000) > offset  &&  distance < (movespd*1000*3)){
    //     speed -= offset* delta * 0.4f;
    // }
    // speed = speed<0?0:speed;

    // // //qDebug()<<delta;
    // float diffaalt = std::abs(currentAltitude-alt);
    // if(currentAltitude<alt){
    //     if(diffaalt<clmbrate){
    //         clmbrate = 0.5f*diffaalt;
    //     }
    //     currentAltitude += clmbrate*delta;
    // }else{
    //     if(diffaalt<divrate){
    //         divrate = 0.5f*diffaalt;
    //     }
    //     currentAltitude -= divrate*delta;
    // }
    // QVector3D direction = target_qvec - current;
    // direction = direction.normalized();

    // float angleRad = atan2(direction.x(), direction.z());
    // float targetDeg = angleRad * (180.0f / M_PI);
    // float deltaang = std::abs(normalizeAngle(targetDeg - angdeg));

    // float tunrate = turnRate;
    // if(deltaang <= tunrate){
    //     tunrate = 0.5f*deltaang;
    // }
    // float tr = convertToClockwise360(targetDeg);
    // float cr = convertToClockwise360(angdeg);
    // targetDeg = normalizeAngle(targetDeg);
    // angdeg = normalizeAngle(angdeg);
    // float diff1 = std::abs(tr-cr);//0-360
    // float diff2 = std::abs(targetDeg-angdeg);//-180 0 180

    // if(diff2>diff1){
    //     angdeg = cr;
    //     targetDeg = tr;
    // }

    // if(angdeg>targetDeg){
    //     angdeg -= tunrate * delta;
    // }else{
    //     angdeg += tunrate * delta;
    // }

    // // //qDebug()<<targetDeg<<","<<angdeg <<","<<deltaang;
    // transform->matrix->setRotation(QQuaternion::fromEulerAngles(QVector3D(0,angdeg,0)));

    // // transform->lookAt(target_qvec);
    // current += ((transform->forward() * speed) + (windDierction* windSpeed))  * delta;
    // current.setY(currentAltitude);

    // //calculate driftangle
    // // 1. Wind Angle निकालें (हवा और हेडिंग के बीच का अंतर)
    // float windAngleRad = qDegreesToRadians(qRadiansToDegrees(qAtan2(windDierction.x(), -windDierction.z())) - TrueHeading);

    // float sinDrift = (windSpeed / TrueAirSpeed) * qSin(windAngleRad);

    // // सुरक्षा के लिए लिमिट चेक (ताकि asin एरर न दे)
    // if (sinDrift > 1.0f) sinDrift = 1.0f;
    // if (sinDrift < -1.0f) sinDrift = -1.0f;

    // DriftAngle = qRadiansToDegrees(qAsin(sinDrift));

    // ///calculate pitch
    // float ratio = VerticalVelocity / TrueAirSpeed;

    // // 3. सुरक्षा के लिए चेक करें (ताकि asin एरर न दे)
    // if (ratio > 1.0f) ratio = 1.0f;
    // if (ratio < -1.0f) ratio = -1.0f;

    // // 4. Angle निकालें (Radians में) और फिर Degrees में बदलें
    // float pitchDegrees = qRadiansToDegrees(qAsin(ratio));
    // float g = 9.81f;

    // // 4. Bank Angle निकालें: tan(theta) = (v * omega) / g
    // float rollDegrees = qRadiansToDegrees(qAtan((TrueAirSpeed * qDegreesToRadians(turnRate)) / g));

    // Pitchrate = (pitch - pitchDegrees)*(1/delta);
    // Rollrate = (roll - rollDegrees)*(1/delta);
    // Yawrate = (yaw - transform->yaw())*(1/delta);

    // pitch = pitchDegrees;
    // roll = rollDegrees;
    // yaw = transform->yaw();
    // transform->matrix->setTranslation(current);
    // float unit = (1/delta)*3600;
    // NorthVelocity = (current.x()-last3d.x()) * unit;
    // EastVelocity = (current.z()-last3d.z()) * unit;
    // VerticalVelocity = (current.y()-last3d.y()) * unit;
    // TrueAirSpeed = QVector2D(current.x(),current.z()).distanceToPoint(last) * unit;
    // GroundVelocity = (windSpeed*3600)+TrueAirSpeed;
    // velocity = worldToLocalVelocity(EastVelocity, NorthVelocity, VerticalVelocity, transform->rotation());
    // if(windSpeed > 0){
    //     currentSpeed = QVector2D(current.x(),current.z()).distanceToPoint(last) * (1/delta) * 3600;
    // }else{
    //     currentSpeed = (speed * 3600)+(windSpeed*3600);
    // }
    // windDierction.setX(0);
    // windDierction.setY(0);
    // windDierction.setZ(0);
    // windSpeed = 0;
    // // ////////////////////////////////////////////////////



    transform->trailData.push_back(QVector3D(transform->getLatitude(),0,transform->getLongitude()));
    if( transform->trailData.capacity()>54000){
        transform->trailData.erase(transform->trailData.begin());
    }
    // ////////////////////////////////////////////////////
    // //*transform->position = Vector::Lerp(*tran7sform->position, *trajectory->Trajectories[trajectory->current]->position, moveSpeed * 0.1);
    current.setZ(transform->getLatitude());
    current.setX(transform->getLongitude());
    float metredis = distanceBetween(trajectory->Trajectories[trajectory->current]->position->x,
                                     trajectory->Trajectories[trajectory->current]->position->z,
                                     current.z(),
                                     current.x());

    // //qDebug()<<metredis;
    if (trajectory->Trajectories.size() > trajectory->current &&  metredis < (currentSpeed/3.6f) *2.f) {
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
            if(trajectory->current >= trajectory->Trajectories.size()){
                endTime = time;
                moveSpeed = 0;
                Altitude = 0;
            }
            trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? 0: trajectory->current;
            if(tgtSpd > 0){
                moveSpeed = tgtSpd;
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
                //movespd = (tgtSpd/3600.0f);//km/h to km/s
                moveSpeed = tgtSpd;
                if(target_qvec.y() > 0){
                    //alt = target_qvec.y() * FTtoKM;
                    Altitude = target_qvec.y();
                }
            }
        }


    }

    if(parentEntity && (parentEntity->category == Entity::Category::Ship ||
                         parentEntity->category == Entity::Category::Submarine ||
                         parentEntity->category == Entity::Category::Tank ))
    {
        Altitude = 0;
        moveSpeed = moveSpeed > 60?60:moveSpeed;
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



void DynamicModel::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){

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
    maximumObj["minSpeed"] = toParm(minSpeed,"Km/h",0,300);
    maximumObj["maxSpeed"] = toParm(maxSpeed,"Km/h",100,2000);
    maximumObj["moveSpeed"] = toParm(moveSpeed,"Km/h",0,2000);
    maximumObj["Acceleration"] = toParm(Acceleration,"m/s^2",100,500,"i am Accerlation");
    maximumObj["Decceleration"] = toParm(Decceleration,"m/s^2",50,    400);
    // maximumObj["turnRadius"] = toParm(turnRadius,"m");
    maximumObj["turnRate"] = toParm(turnRate,"deg/s",5,30);
    maximumObj["MaxAltitude"] = toParm(maxAltitude,"ft",10,60000);
    maximumObj["Altitude"] = toParm(Altitude,"ft",10,60000);
    maximumObj["climbRate"] = toParm(climbRate,"ft/min",0,      10000);
    maximumObj["diveRate"] = toParm(diveRate,"ft/min",0,      10000);
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


    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        //obj[it.key()] = it.value();
    }

    ////qDebug() << "DynamicModel::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}

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
        if (maximumObj.contains("MaxAltitude") && maximumObj["MaxAltitude"].isObject())
            maxAltitude = valueFromParm(maximumObj["MaxAltitude"].toObject());
        if (maximumObj.contains("Altitude") && maximumObj["Altitude"].isObject())
            Altitude = valueFromParm(maximumObj["Altitude"].toObject());
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

    // Custom parameters
    QStringList standardKeys = {
        "control", "turnRadius", "start", "moveSpeed","responses","maximums","passabillity"
    };
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!standardKeys.contains(it.key())) {
            customParameters[it.key()] = it.value();
        }
    }

    ////qDebug() << "DynamicModel::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
}

void DynamicModel::setMoveSpeed(float speed) {
    //moveSpeed = qBound(1.0f, speed, 10.0f);
}

