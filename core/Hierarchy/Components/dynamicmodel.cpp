/**
 * @file dynamicmodel.cpp
 * @brief Implementation of the DynamicModel component for entity motion and formation flying.
 */

#include "dynamicmodel.h"
#include <core/InputSystem/inputmanager.h>
#include <core/Simulation/simulation.h>
#include "core/Hierarchy/ECS/Struct/dynamic_component.h"
#include "core/Hierarchy/EntityProfiles/weapons/sonobuoy.h"
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

float getGlobalHeading(
    const QVector3D& pointA,
    const QVector3D& pointB)
{
    float dx = pointB.x() - pointA.x();
    float dz = pointB.z() - pointA.z();

    if (std::abs(dx) < 0.000001f &&
        std::abs(dz) < 0.000001f)
    {
        return 0.0f;
    }

    float heading =
        std::atan2(dx, dz) *
        180.0f / static_cast<float>(M_PI);

    if (heading < 0.0f)
        heading += 360.0f;

    return heading;
}

struct ProjectionResult
{
    QVector3D point;          // Original perpendicular point C
    QVector3D offsetPoint;    // C + manual offset toward B
    QVector3D segmentPoint;   // Closest point clamped inside A-B

    float t;                  // Original C progress on AB
    float offsetT;            // Offset point progress on AB
    float distance;           // Aircraft -> AB segment distance

    bool insideSegment;       // Original C A-B ke andar?
    bool offsetInsideSegment; // Offset point A-B ke andar?
};

ProjectionResult projectAircraftOnPath(
    const QVector3D& aircraft,
    const QVector3D& A,
    const QVector3D& B,
    float offsetTowardB = 0.0f)
{
    ProjectionResult result{};


    QVector3D AB(
        B.x() - A.x(),
        0.0f,
        B.z() - A.z()
        );

    QVector3D AP(
        aircraft.x() - A.x(),
        0.0f,
        aircraft.z() - A.z()
        );

    const float lengthSq =
        AB.x() * AB.x() +
        AB.z() * AB.z();

    const float segmentLength =
        std::sqrt(lengthSq);



    if (segmentLength < 0.000001f)
    {
        result.point = A;
        result.offsetPoint = A;
        result.segmentPoint = A;

        result.t = 0.0f;
        result.offsetT = 0.0f;

        QVector3D error(
            aircraft.x() - A.x(),
            0.0f,
            aircraft.z() - A.z()
            );

        result.distance = error.length();

        result.insideSegment = false;
        result.offsetInsideSegment = false;

        return result;
    }



    const float t =
        (AP.x() * AB.x() +
         AP.z() * AB.z()) /
        lengthSq;

    result.t = t;

    result.point =
        A + AB * t;

    // Y ko path plane par rakho
    result.point.setY(A.y());

    result.insideSegment =
        (t >= 0.0f && t <= 1.0f);


    const float clampedT =
        std::clamp(
            t,
            0.0f,
            1.0f);

    result.segmentPoint =
        A + AB * clampedT;

    result.segmentPoint.setY(A.y());


    QVector3D error(
        aircraft.x() -
            result.segmentPoint.x(),

        0.0f,

        aircraft.z() -
            result.segmentPoint.z()
        );

    result.distance =
        error.length();


    const float offsetT =
        offsetTowardB /
        segmentLength;

    result.offsetT =
        t + offsetT;


    // Is offset point actual segment ke andar hai?
    result.offsetInsideSegment =
        (result.offsetT >= 0.0f &&
         result.offsetT <= 1.0f);


    const float clampedOffsetT =
        std::clamp(
            result.offsetT,
            0.0f,
            1.0f);

    result.offsetPoint =
        A + AB * clampedOffsetT;

    result.offsetPoint.setY(A.y());


    return result;
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
DynamicModel::DynamicModel():Component() {
    control = true;
    follow = true;
    moveSpeed = 800;
    AdditionalParameters = QJsonObject(); // Initialize
    if(!parentEntity)return;
    parentEntity->root->world.AddComponent<ECS::DynamicSpeedConstants>(parentEntity->ecsID);
    parentEntity->root->world.AddComponent<ECS::DynamicAltitudeConstants>(parentEntity->ecsID);
    parentEntity->root->world.AddComponent<ECS::DynamicClimbRateConstants>(parentEntity->ecsID);
    parentEntity->root->world.AddComponent<ECS::DynamicMaxAcceDeccConstants>(parentEntity->ecsID);
    parentEntity->root->world.AddComponent<ECS::DynamicVariables>(parentEntity->ecsID);
    parentEntity->root->world.AddComponent<ECS::DynamicVelocity>(parentEntity->ecsID);
    parentEntity->root->world.AddComponent<ECS::DynamicOrientation>(parentEntity->ecsID);
    // parentEntity->root->world.AddComponent<ECS::DynamicTarget>(parentEntity->ecsID);
}

DynamicModel::~DynamicModel(){
    if(formation){
        formation->clear();
    }

    if(formationPosition){
        formationPosition->entity = nullptr;
    }
}

void DynamicModel::Awake(){}
void DynamicModel::Start(){}
/**
 * @brief Initializes the dynamic model, setting start time and default values.
 */
void DynamicModel::init(Entity *Entity){
    parentEntity = Entity;
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
    if (parentEntity && parentEntity->isRemoteDISEntity) return;
    if ( !transform || !rigidbody || !trajectory)
    {
        moveSpeed = moveSpeed>maxSpeed?maxSpeed:(moveSpeed<minSpeed?minSpeed:moveSpeed);
        Altitude = Altitude >maxAltitude?maxAltitude:Altitude;
        if(parentEntity->category == Entity::Category::Marine){
            Altitude  = Altitude>0?0:Altitude;
        }else{
            Altitude  = Altitude<0?0:Altitude;
        }
        return;
    }

    time += deltaTime;
    delta = deltaTime;
    ////qDebug() << time;
    if(startTime<time)
    {
        FollowTrajectory();
    }
}


void DynamicModel::FollowFormation(){
    const float rotationSmoothFactor = 5.0f;
    const float G_ACCELERATION_VAL = 9.81f;
    const float dampingFactor = 0.90f;

    if (follow && followEntity && formationPosition && formationPosition->Offset) {
        Transform& mTransform = followEntity->transform;
        Platform* mPlatform = dynamic_cast<Platform*>(followEntity);
        DynamicModel& mModel = /*mPlatform ?*/ mPlatform->dynamicModel; /*: nullptr;*/

        if (/*mTransform &&*/ /*mModel &&*/ mPlatform->trajectory.getTargetWaypoint()&&mPlatform->trajectory.getTargetWaypoint()->formation) {


            if(mModel.currentSpeed<10)return;
            // --- 2. TURN DETECTION & COMMITMENT SYSTEM ---
            float leaderHeading = mTransform.matrix.rotation().toEulerAngles().y();
            float myHeading = transform->matrix.rotation().toEulerAngles().y();

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
            QVector3D mPos = mTransform.matrix.translation();
            mPos.setY(0);
            QVector3D mVel = mModel.velocity;
            QQuaternion mRot = mTransform.matrix.rotation();

            QVector3D mAngularVel = mModel.angularVelocity;
            QQuaternion rotPredict = QQuaternion::fromEulerAngles(mAngularVel * currentLookahead);
            QQuaternion predictedRot = mRot * rotPredict;

            QVector3D localOffset(
                formationPosition->Offset->x,
                formationPosition->Offset->y,
                formationPosition->Offset->z
                );

            // QVector3D worldOffset = predictedRot.rotatedVector(localOffset);
            QVector3D worldOffset = mRot.rotatedVector(localOffset);

            // --- 5. GET CURRENT POSITION ---
            QVector3D currentPos = transform->matrix.translation();
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
                // targetPos = mPos + (mVel * currentLookahead) + worldOffset;
                targetPos = mPos + worldOffset;
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
                // FORMATION MODE:
                // Agar paas aa gaye hain, toh mothership ki speed se thodi kam speed rakhein (jaise 95% ya -15 km/h)
                float targetFormationSpeedKmh = mothershipSpeedKmh - 15.0f; // Mothership se 15 km/h kam
                // Aap chahe toh percentage bhi use kar sakte hain: mothershipSpeedKmh * 0.95f

                targetFormationSpeedKmh = std::max(targetFormationSpeedKmh, moveSpeed / 3.6f * 3.6f); // Minimum moveSpeed se niche na jaye
                // FORMATION MODE: Allow only +300 km/h extra for smooth following
                float maxAllowedMS = (/*mothershipSpeedKmh*/targetFormationSpeedKmh + 300.0f) / 3.6f;
                maxSpeedMS = std::min((moveSpeed / 3.6f) * speedModifier, maxAllowedMS);
            }

            // SLOWING RADIUS: Slow down only within the last 800m
            float slowingRadius = 800.0f;

            QVector3D desiredVel;
            if (distance > slowingRadius) {
                // Full speed until within slowing radius
                desiredVel = error.normalized() * maxSpeedMS;
            } else {
                // // Linear slowdown over last 800m for smooth landing
                // float t = distance / slowingRadius;
                // desiredVel = error.normalized() * maxSpeedMS * t;
                // Smooth arrival: jaise-jaise pass aaye, speed mothership-minus wali speed par settle ho jaye
                float t = distance / slowingRadius;
                // Yahan hum chahein toh minimum speed bhi maintain kar sakte hain taaki ekdum ruk na jaye
                float minSpeedMS = (mothershipSpeedKmh - 30.0f) / 3.6f;
                minSpeedMS = std::max(0.0f, minSpeedMS);

                float blendedSpeed = minSpeedMS + (maxSpeedMS - minSpeedMS) * t;
                desiredVel = error.normalized() * blendedSpeed;
            }

            // Apply altitude compensation during turns
            if (inActiveTurn && std::abs(altitudeModifier) > 0.1f) {
                QVector3D currentPosWithY = transform->matrix.translation();
                float targetAltitude = mPos.y() + altitudeModifier;
                currentPosWithY.setY(currentPosWithY.y() + (targetAltitude - currentPosWithY.y()) * delta * 0.5f);
                transform->matrix.setTranslation(currentPosWithY);
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
            newPos.setY(transform->matrix.translation().y());  // Preserve Y from altitude compensation
            transform->matrix.setTranslation(newPos);

            // --- 10. 6-DOF ROTATION & BANKING ---
            if (velocity.lengthSquared() > 0.001f) {
                float targetYaw = atan2(velocity.x(), velocity.z()) * (180.0f / M_PI);
                QVector3D shipRight = QQuaternion::fromEulerAngles(0, targetYaw, 0).rotatedVector(QVector3D(1, 0, 0));
                float lateralForce = QVector3D::dotProduct(steering / mass, shipRight);

                float targetRoll = qBound(-60.0f, float(atan2(lateralForce, G_ACCELERATION_VAL) * (180.0 / M_PI)), 60.0f);
                float targetPitch = mTransform.matrix.rotation().toEulerAngles().x();

                QQuaternion targetRotation = QQuaternion::fromEulerAngles(targetPitch, targetYaw, targetRoll);
                QQuaternion currentRotation = transform->matrix.rotation();

                QQuaternion smoothedRot = QQuaternion::slerp(currentRotation, targetRotation, delta * rotationSmoothFactor);
                transform->setRotation(smoothedRot);
            }else{
                QQuaternion currentRotation = transform->matrix.rotation();
                QQuaternion smoothedRot = QQuaternion::slerp(currentRotation, mModel.transform->rotation(), delta);
                transform->setRotation(smoothedRot);
            }
            return;
        }
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

    FollowFormation();

    if(followTarget){
        return;
    }

    if(trajectory->Trajectories.size()<2) return;
    QVector3D current = transform->matrix.translation();
    QVector2D last(current.x(),current.z());
    QVector3D last3d(current.x(),current.y(),current.z());
    Vector lastT;
    if(trajectory->getCurrentWaypoint() != nullptr){
        lastT = *trajectory->getCurrentWaypoint()->position;
    }
    Vector target = *trajectory->getTargetWaypoint()->position;
    float tgtSpd = trajectory->getTargetWaypoint()->speed;
    FlatXYZ lasttargt = geoToFlatXYZ(lastT.x,lastT.z,lastT.y);
    FlatXYZ targt = geoToFlatXYZ(target.x,target.z,target.y);
    QVector3D target_qvec(targt.x, targt.y, targt.z);
    QVector2D tar(target_qvec.x(),target_qvec.z());
    QVector3D diff = target_qvec-transform->translation();

    // Standard Heading (Yaw): using atan2(x, z) because forward is Z+
    float heading = std::atan2(diff.x(), diff.z()) * (180.0f / M_PI);
    heading = heading-transform->getHeading();
    heading = heading<0?(-1*heading):heading;

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
            trajectory->getTrajectoryPath().currentWaypoint = trajectory->current;
            if(trajectory->current <= 0){
                endTime = time;
                moveSpeed = 0;
                Altitude = 0;
            }
            trajectory->current = trajectory->current <= 0 ? 0: trajectory->current;
            trajectory->getTrajectoryPath().currentWaypoint = trajectory->current;
        }else{
            trajectory->current += 1;
            trajectory->getTrajectoryPath().currentWaypoint = trajectory->current;
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
                trajectory->getTrajectoryPath().currentWaypoint = trajectory->current;
            }else{
                trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? 0: trajectory->current;
                trajectory->getTrajectoryPath().currentWaypoint = trajectory->current;
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
                if(target_qvec.y() > 0 || (parentEntity->category == Entity::Category::Marine && target_qvec.y()<0)){
                    //alt = target_qvec.y() * FTtoKM;
                    Altitude = target_qvec.y();
                }
            }
        }
    }
    if(parentEntity && (parentEntity->category == Entity::Category::Marine ||
                         parentEntity->category == Entity::Category::Ground ))
    {
        // moveSpeed = moveSpeed > 500?500:moveSpeed;
    }else{
        moveSpeed = moveSpeed>maxSpeed?maxSpeed:moveSpeed;
    }
    if(parentEntity->category == Entity::Category::Marine){
        Altitude  = Altitude>0?0:Altitude;
        if(parentEntity->marineCategory !=Entity::SubMarineCategory::Submarine){
            Altitude = 0;
        }
    }else
    if(parentEntity->category== Entity::Ground){
        Altitude = 0;
    }else
    if(parentEntity->category== Entity::Air){
        Altitude  = Altitude<0?0:Altitude;
    }

    // auto* tp = parentEntity->root->world.GetComponent<ECS::DynamicTarget>(parentEntity->ecsID);
    // if(!tp){
    //     tp = &parentEntity->root->world.AddComponent<ECS::DynamicTarget>(parentEntity->ecsID);
    // }
    // tp->pos = last3d;
    ProjectionResult r =
        projectAircraftOnPath(
            last3d,
            QVector3D(lasttargt.x,lasttargt.y,lasttargt.z),
            QVector3D(targt.x,targt.y,targt.z),
            2
            );
    // tp->pos = r.offsetPoint;
    heading = getGlobalHeading(last3d,r.offsetPoint);


    commandedAltitude = Altitude;
    commandedSpeed = moveSpeed;
    if(control){
        commandedHeading = heading;
    }

}

void DynamicModel::TimeJump(int sec){
    Platform* platform = nullptr;
    if(parentEntity){
        platform = dynamic_cast<Platform*>(parentEntity);
    }
    if(trajectory->Trajectories.size()<2) return;
    if(time<startTime){
        float secdiff = startTime-time;
        time +=sec;
        sec -= secdiff;
        sec = sec<0?0:sec;
    }else{
        time +=sec;
    }
    if(sec == 0){
        return;
    }
    float Time = 0;
    float dis = 0;
    float alt = Altitude;
    float speed = moveSpeed*(5.f/18.f);
    int waynum = trajectory->current;
    int lastwaynum = trajectory->current;
    int leftsec = 0;
    transform->trailData.clear();
    if(follow && followEntity && formationPosition && formationPosition->Offset) {
        Transform& mTransform = followEntity->transform;
        Platform* mPlatform = dynamic_cast<Platform*>(followEntity);
        DynamicModel& mModel = /*mPlatform ?*/ mPlatform->dynamicModel; /*: nullptr;*/

        if (/*mTransform &&*//* mModel &&*/ mPlatform->trajectory.getTargetWaypoint()&&mPlatform->trajectory.getTargetWaypoint()->formation) {

            transform->setTranslation(mTransform.translation()-mTransform.TransformDirection(QVector3D(formationPosition->Offset->x,formationPosition->Offset->y,formationPosition->Offset->z)));
            transform->setHeading(mTransform.getHeading());
            return;
        }

    }

    for(int i = 0;i<100;i++){
        Waypoints* wp= trajectory->getCurrentWaypoint();
        if(waynum == 0 && lastwaynum == trajectory->Trajectories.size()-1){
            wp = trajectory->getWaypointByIndex(lastwaynum+1);
        }
        if(wp&&wp->speed>0){
            moveSpeed = wp->speed;//*3.6f;
        }
        if(wp&&wp->position->y>0){
            Altitude = wp->position->y;
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

        if(parentEntity && (parentEntity->category == Entity::Category::Marine ||
                             parentEntity->category == Entity::Category::Marine ||
                             parentEntity->category == Entity::Category::Ground ))
        {
            // moveSpeed = moveSpeed > 500?500:moveSpeed;
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
        float metredis = distanceBetween(trajectory->Trajectories[trajectory->current]->position->x,
                                         trajectory->Trajectories[trajectory->current]->position->z,
                                         transform->getLatitude(),
                                         transform->getLongitude());
        int taketime = metredis/speed;
        dis = metredis;
        float lastdis = 0;
        if(wp){
            lastdis = distanceBetween(wp->position->x,
                                      wp->position->z,
                                      transform->getLatitude(),
                                      transform->getLongitude());
        }
        if(platform && wp && wp->dropWeapon&&lastdis<1000){
            int dropcount = 0;
            bool autoDrop = false;
            float dropdistance = 0;
            float totaldistance = 0;
            Transform trans;
            int curwp = trajectory->current;
            int curspeed = speed;
            trans.setTranslation(transform->translation());
            float initTime = Time;
            QVector3D inital = trans.translation();
            float initlat = trans.getLatitude();
            float initlon = trans.getLongitude();
            trans.setGeoCord(trajectory->Trajectories[curwp]->position->x,trajectory->Trajectories[curwp]->position->z,Altitude);
            QVector3D target_qvec =trans.translation();
            QVector3D diff = target_qvec-inital;
            // Standard Heading (Yaw): using atan2(x, z) because forward is Z+
            float heading = std::atan2(diff.x(), diff.z()) * (180.0f / M_PI);
            dropcount = wp->dropcount;
            autoDrop = wp->dropWeapon;
            platform->dropcount += dropcount;
            platform->autoDrop = autoDrop;

            for (auto const& pair :*platform->weapons.weapons) {
                Weapon* s = pair.second;
                if(!s)continue;
                Sonobuoy* sono = dynamic_cast<Sonobuoy*>(s);
                if(sono && !sono->drop && dropcount>0){
                    autoDrop = true;
                    //dropTime = time+dropInterval;
                    sono->dropped();
                    dropcount--;
                    platform->dropcount--;
                    // if(sono->transform){
                    float leftDis = dropdistance/1000.f;
                    float rad = heading * (M_PI /180.0f);
                    float x = leftDis * std::sin(rad);
                    float z = leftDis * std::cos(rad);
                    sono->transform.setTranslation(inital+QVector3D(x,0,z));
                    sono->transform.setAltitude(-sono->Depth*3.281f);
                    Waypoints* wp= trajectory->getWaypointByIndex(curwp);
                    if(wp&&wp->speed>0){
                        curspeed = wp->speed*(5.f/18.f);
                    }
                    dropdistance += curspeed * platform->dropInterval;
                    float metredis = distanceBetween(trajectory->Trajectories[curwp]->position->x,
                                                     trajectory->Trajectories[curwp]->position->z,
                                                     initlat,
                                                     initlon);

                    platform->dropTime = initTime + platform->dropInterval;
                    initTime +=platform->dropInterval;
                    int taketime = totaldistance/curspeed;
                    totaldistance +=dropdistance;

                    if(platform->dropTime>sec){
                        break;
                    }
                    if(dropdistance>metredis){
                        int curntwp = curwp;
                        float lastlat = trajectory->Trajectories[curwp]->position->x;
                        float lastlon = trajectory->Trajectories[curwp]->position->z;
                        float dist = metredis;
                        for(int t = 0;t<100;t++){
                            int tempwp =curntwp+1;
                            if(tempwp>=trajectory->Trajectories.size()){
                                tempwp = 0;
                            }
                            float mdist = distanceBetween(trajectory->Trajectories[tempwp]->position->x,
                                                          trajectory->Trajectories[tempwp]->position->z,
                                                          lastlat,
                                                          lastlon);
                            if(dropdistance>dist+mdist){
                                curntwp = tempwp;
                                dist +=mdist;
                                Waypoints* wp= trajectory->getWaypointByIndex(tempwp);
                                if(wp&&wp->speed>0){
                                    curspeed = wp->speed*(5.f/18.f);
                                }
                                lastlat = trajectory->Trajectories[curntwp]->position->x;
                                lastlon = trajectory->Trajectories[curntwp]->position->z;
                            }else{
                                metredis = dist;
                                curwp = curntwp;
                                break;
                            }
                        }
                        dropdistance = dropdistance-metredis;
                        totaldistance +=dropdistance;
                        initlat = trajectory->Trajectories[curwp]->position->x;
                        initlon = trajectory->Trajectories[curwp]->position->z;
                        trans.setGeoCord(initlat,initlon);
                        inital = trans.translation();
                        curwp++;
                        if(curwp>=trajectory->Trajectories.size()){
                            curwp = 0;
                        }
                        trans.setGeoCord(trajectory->Trajectories[curwp]->position->x,trajectory->Trajectories[curwp]->position->z,Altitude);
                        target_qvec =trans.translation();
                        diff = target_qvec-inital;
                        // Standard Heading (Yaw): using atan2(x, z) because forward is Z+
                        heading = std::atan2(diff.x(), diff.z()) * (180.0f / M_PI);
                    }
                    // }

                }
            }
        }

        if(taketime+Time>sec){
            leftsec = sec-Time;
            break;
        }
        Time+=taketime;
        transform->setGeoCord(trajectory->Trajectories[trajectory->current]->position->x,trajectory->Trajectories[trajectory->current]->position->z,Altitude);
        transform->trailData.push_back(QVector3D(transform->getLatitude(),0,transform->getLongitude()));
        ////
        transform->lookAt(trajectory->Trajectories[trajectory->current]->position->x,trajectory->Trajectories[trajectory->current]->position->z);
        if( transform->trailData.capacity()>54000){
            transform->trailData.erase(transform->trailData.begin());
        }
        trajectory->current+=1;
        trajectory->getTrajectoryPath().currentWaypoint = trajectory->current;
        if(trajectory->current >= trajectory->Trajectories.size()){
            trajectory->current = 0;
            trajectory->getTrajectoryPath().currentWaypoint = trajectory->current;
        }
        lastwaynum = waynum;
        waynum = trajectory->current;
    }
    currentSpeed = moveSpeed;
    transform->lookAt(trajectory->Trajectories[trajectory->current]->position->x,trajectory->Trajectories[trajectory->current]->position->z);
    float leftDis = (leftsec*speed)/1000.f;
    float rad = transform->getHeading()*(M_PI /180.0f);
    float x = leftDis * std::sin(rad);
    float z = leftDis * std::cos(rad);
    transform->setTranslation(transform->translation()+QVector3D(x,0,z));
    transform->setAltitude(Altitude);
    if(platform){
        platform->_lastcurrent=trajectory->current;
    }
}

void DynamicModel::SyncByJson(const QJsonObject& obj){
    // Keep SyncByJson lightweight: only persistent/configurable inputs are
    // accepted here. Runtime Aircraft telemetry remains simulation-owned.

    if (obj.contains("maximums") && obj["maximums"].isObject()) {
        QJsonObject m = obj["maximums"].toObject();

        if (m.contains("minSpeed") && m["minSpeed"].isObject()) minSpeed = valueFromParm(m["minSpeed"].toObject());
        if (m.contains("maxSpeed") && m["maxSpeed"].isObject()) maxSpeed = valueFromParm(m["maxSpeed"].toObject());
        if (m.contains("moveSpeed") && m["moveSpeed"].isObject()) moveSpeed = valueFromParm(m["moveSpeed"].toObject());
        if (m.contains("Acceleration") && m["Acceleration"].isObject()) Acceleration = valueFromParm(m["Acceleration"].toObject());
        if (m.contains("Decceleration") && m["Decceleration"].isObject()) Decceleration = valueFromParm(m["Decceleration"].toObject());
        if (m.contains("turnRate") && m["turnRate"].isObject()) turnRate = valueFromParm(m["turnRate"].toObject());
        if (m.contains("minimumTurnSpeed") && m["minimumTurnSpeed"].isObject()) minimumTurnSpeed = valueFromParm(m["minimumTurnSpeed"].toObject());
        if (m.contains("fullTurnAuthoritySpeed") && m["fullTurnAuthoritySpeed"].isObject()) fullTurnAuthoritySpeed = valueFromParm(m["fullTurnAuthoritySpeed"].toObject());
        if (m.contains("maxPitchAngle") && m["maxPitchAngle"].isObject()) maxPitchAngle = valueFromParm(m["maxPitchAngle"].toObject());
        if (m.contains("maxBankAngle") && m["maxBankAngle"].isObject()) maxBankAngle = valueFromParm(m["maxBankAngle"].toObject());

        if (parentEntity && parentEntity->category == Entity::Category::Marine) {
            if (m.contains("MaxDepth") && m["MaxDepth"].isObject()) maxAltitude = valueFromParm(m["MaxDepth"].toObject());
            if (m.contains("Depth") && m["Depth"].isObject()) Altitude = valueFromParm(m["Depth"].toObject());
        } else {
            if (m.contains("MaxAltitude") && m["MaxAltitude"].isObject()) maxAltitude = valueFromParm(m["MaxAltitude"].toObject());
            if (m.contains("Altitude") && m["Altitude"].isObject()) Altitude = valueFromParm(m["Altitude"].toObject());
        }

        if (m.contains("climbRate") && m["climbRate"].isObject()) climbRate = valueFromParm(m["climbRate"].toObject());
        if (m.contains("diveRate") && m["diveRate"].isObject()) diveRate = valueFromParm(m["diveRate"].toObject());
    }

    if (obj.contains("responses") && obj["responses"].isObject()) {
        QJsonObject r = obj["responses"].toObject();
        if (r.contains("deltaSpdCommandMaxAcc") && r["deltaSpdCommandMaxAcc"].isObject()) deltaSpdCommandMaxAcc = valueFromParm(r["deltaSpdCommandMaxAcc"].toObject());
        if (r.contains("timeToReachMaxAcc") && r["timeToReachMaxAcc"].isObject()) timeToReachMaxAcc = valueFromParm(r["timeToReachMaxAcc"].toObject());
        if (r.contains("deltaSpdCommandMaxDecel") && r["deltaSpdCommandMaxDecel"].isObject()) deltaSpdCommandMaxDecel = valueFromParm(r["deltaSpdCommandMaxDecel"].toObject());
        if (r.contains("timeToReachMaxDecel") && r["timeToReachMaxDecel"].isObject()) timeToReachMaxDecel = valueFromParm(r["timeToReachMaxDecel"].toObject());
        if (r.contains("deltaHdgCommandMaxRot") && r["deltaHdgCommandMaxRot"].isObject()) deltaHdgCommandMaxRot = valueFromParm(r["deltaHdgCommandMaxRot"].toObject());
        if (r.contains("timeToReachMaxRot") && r["timeToReachMaxRot"].isObject()) timeToReachMaxRot = valueFromParm(r["timeToReachMaxRot"].toObject());
        if (r.contains("maximumPitchRate") && r["maximumPitchRate"].isObject()) maximumPitchRate = valueFromParm(r["maximumPitchRate"].toObject());
        if (r.contains("maximumRollRate") && r["maximumRollRate"].isObject()) maximumRollRate = valueFromParm(r["maximumRollRate"].toObject());
        if (r.contains("deltaToReachMaxROC") && r["deltaToReachMaxROC"].isObject()) deltaToReachMaxROC = valueFromParm(r["deltaToReachMaxROC"].toObject());
        if (r.contains("deltaToReachMaxROD") && r["deltaToReachMaxROD"].isObject()) deltaToReachMaxROD = valueFromParm(r["deltaToReachMaxROD"].toObject());
    }

    if (obj.contains("flightPhysics") && obj["flightPhysics"].isObject()) {
        QJsonObject p = obj["flightPhysics"].toObject();
        if (p.contains("minimumTakeoffSpeed") && p["minimumTakeoffSpeed"].isObject()) minimumTakeoffSpeed = valueFromParm(p["minimumTakeoffSpeed"].toObject());
        if (p.contains("stallSpeed") && p["stallSpeed"].isObject()) stallSpeed = valueFromParm(p["stallSpeed"].toObject());
        if (p.contains("criticalAngleOfAttack") && p["criticalAngleOfAttack"].isObject()) criticalAngleOfAttack = valueFromParm(p["criticalAngleOfAttack"].toObject());
        if (p.contains("gravity") && p["gravity"].isObject()) gravity = valueFromParm(p["gravity"].toObject());
        if (p.contains("maxSinkSpeed") && p["maxSinkSpeed"].isObject()) maxSinkSpeed = valueFromParm(p["maxSinkSpeed"].toObject());
        if (p.contains("stallRecoveryRate") && p["stallRecoveryRate"].isObject()) stallRecoveryRate = valueFromParm(p["stallRecoveryRate"].toObject());
        if (p.contains("stalledLiftFactor") && p["stalledLiftFactor"].isObject()) stalledLiftFactor = valueFromParm(p["stalledLiftFactor"].toObject());
        if (p.contains("takeoffPitchThreshold") && p["takeoffPitchThreshold"].isObject()) takeoffPitchThreshold = valueFromParm(p["takeoffPitchThreshold"].toObject());
        if (p.contains("groundLevel") && p["groundLevel"].isObject()) groundLevel = valueFromParm(p["groundLevel"].toObject());
    }

    if (obj.contains("stallRecovery") && obj["stallRecovery"].isObject()) {
        QJsonObject s = obj["stallRecovery"].toObject();
        if (s.contains("autoStallRecovery")) autoStallRecovery = s["autoStallRecovery"].toBool();
        if (s.contains("stallRecoveryPitch") && s["stallRecoveryPitch"].isObject()) stallRecoveryPitch = valueFromParm(s["stallRecoveryPitch"].toObject());
    }

    if (obj.contains("aerodynamicDrag") && obj["aerodynamicDrag"].isObject()) {
        QJsonObject d = obj["aerodynamicDrag"].toObject();
        if (d.contains("zeroThrottleDrag") && d["zeroThrottleDrag"].isObject()) zeroThrottleDrag = valueFromParm(d["zeroThrottleDrag"].toObject());
        if (d.contains("highSpeedDrag") && d["highSpeedDrag"].isObject()) highSpeedDrag = valueFromParm(d["highSpeedDrag"].toObject());
        if (d.contains("inducedDrag") && d["inducedDrag"].isObject()) inducedDrag = valueFromParm(d["inducedDrag"].toObject());
        if (d.contains("referenceSpeed") && d["referenceSpeed"].isObject()) dragReferenceSpeed = valueFromParm(d["referenceSpeed"].toObject());
    }

    if (obj.contains("commands") && obj["commands"].isObject()) {
        QJsonObject c = obj["commands"].toObject();
        if (c.contains("commandedSpeed") && c["commandedSpeed"].isObject()) commandedSpeed = valueFromParm(c["commandedSpeed"].toObject());
        if (c.contains("commandedHeading") && c["commandedHeading"].isObject()) commandedHeading = valueFromParm(c["commandedHeading"].toObject());
        if (c.contains("commandedAltitude") && c["commandedAltitude"].isObject()) commandedAltitude = valueFromParm(c["commandedAltitude"].toObject());
        if (c.contains("altitudeReference") && c["altitudeReference"].isObject()) {
            QString value = c["altitudeReference"].toObject()["value"].toString();
            altitudeReference = (value == "GroundRelative") ? AltitudeReference::GroundRelative : AltitudeReference::WorldY;
        }
    }

    moveSpeed = qBound(minSpeed, moveSpeed, maxSpeed);
    commandedSpeed = qBound(0.0f, commandedSpeed, maxSpeed);
    stalledLiftFactor = qBound(0.0f, stalledLiftFactor, 1.0f);
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
    obj["type"] = "component";

    QJsonObject timeObj;
    timeObj["type"] = "Time";
    timeObj["value"] = toParm(startTime,"s");
    obj["takeoff"] = timeObj;

    if(parentEntity && parentEntity->root){
        QJsonObject dynamicTypeObj;
        dynamicTypeObj["type"] = "option";
        QJsonArray optionsArray;
        for (const QString& opt : parentEntity->root->getAvailableDynamicTypes())
            optionsArray.append(opt);
        dynamicTypeObj["options"] = optionsArray;
        dynamicTypeObj["value"] = dynamicType;
        obj["dynamicType"] = dynamicTypeObj;
    }

    // ---------------------------------------------------------------------
    // MAXIMUMS / BASIC MOTION
    // ---------------------------------------------------------------------
    QJsonObject maximumObj;
    maximumObj["type"] = "Section";
    maximumObj["visible"] = true;
    maximumObj["minSpeed"] = toParm(minSpeed,"Km/h",0,3000);
    maximumObj["maxSpeed"] = toParm(maxSpeed,"Km/h",0,5000);
    maximumObj["moveSpeed"] = toParm(moveSpeed,"Km/h",0,5000);
    maximumObj["Acceleration"] = toParm(Acceleration,"m/s^2",0,500);
    maximumObj["Decceleration"] = toParm(Decceleration,"m/s^2",0,500);
    maximumObj["turnRate"] = toParm(turnRate,"deg/s",0,60);
    maximumObj["minimumTurnSpeed"] = toParm(minimumTurnSpeed,"Km/h",0,2000);
    maximumObj["fullTurnAuthoritySpeed"] = toParm(fullTurnAuthoritySpeed,"Km/h",0,3000);
    maximumObj["maxPitchAngle"] = toParm(maxPitchAngle,"deg",0,89);
    maximumObj["maxBankAngle"] = toParm(maxBankAngle,"deg",0,180);

    if(parentEntity && parentEntity->category == Entity::Category::Marine){
        maximumObj["MaxDepth"] = toParm(maxAltitude,"ft",-2000,0);
        maximumObj["Depth"] = toParm(Altitude,"ft",-2000,0);
        maximumObj["climbRate"] = toParm(climbRate,"ft/min",0,100);
        maximumObj["diveRate"] = toParm(diveRate,"ft/min",0,100);
    }else{
        maximumObj["MaxAltitude"] = toParm(maxAltitude,"ft",0,100000);
        maximumObj["Altitude"] = toParm(Altitude,"ft",0,100000);
        maximumObj["climbRate"] = toParm(climbRate,"ft/min",0,10000);
        maximumObj["diveRate"] = toParm(diveRate,"ft/min",0,10000);
    }
    obj["maximums"] = maximumObj;

    // ---------------------------------------------------------------------
    // TRAJECTORY/NAVIGATION COMMAND
    // ---------------------------------------------------------------------
    QJsonObject commandsObj;
    commandsObj["type"] = "Section";
    commandsObj["commandedSpeed"] = toParm(commandedSpeed,"Km/h",0,5000);
    commandsObj["commandedHeading"] = toParm(commandedHeading,"deg",0,360);
    commandsObj["commandedAltitude"] = toParm(commandedAltitude,"ft",-2000,100000);

    QJsonObject altitudeReferenceObj;
    altitudeReferenceObj["type"] = "option";
    QJsonArray altitudeOptions;
    altitudeOptions.append("WorldY");
    altitudeOptions.append("GroundRelative");
    altitudeReferenceObj["options"] = altitudeOptions;
    altitudeReferenceObj["value"] =
        altitudeReference == AltitudeReference::GroundRelative ? "GroundRelative" : "WorldY";
    commandsObj["altitudeReference"] = altitudeReferenceObj;
    obj["commands"] = commandsObj;

    // ---------------------------------------------------------------------
    // RESPONSE
    // ---------------------------------------------------------------------
    QJsonObject responsesObj;
    responsesObj["type"] = "Section";
    responsesObj["deltaSpdCommandMaxAcc"] = toParm(deltaSpdCommandMaxAcc,"m/s",0,200);
    responsesObj["timeToReachMaxAcc"] = toParm(timeToReachMaxAcc,"s",0,30);
    responsesObj["deltaSpdCommandMaxDecel"] = toParm(deltaSpdCommandMaxDecel,"m/s",0,200);
    responsesObj["timeToReachMaxDecel"] = toParm(timeToReachMaxDecel,"s",0,30);
    responsesObj["deltaHdgCommandMaxRot"] = toParm(deltaHdgCommandMaxRot,"deg",0,180);
    responsesObj["timeToReachMaxRot"] = toParm(timeToReachMaxRot,"s",0,30);
    responsesObj["maximumPitchRate"] = toParm(maximumPitchRate,"deg/s",0,120);
    responsesObj["maximumRollRate"] = toParm(maximumRollRate,"deg/s",0,240);
    responsesObj["deltaToReachMaxROC"] = toParm(deltaToReachMaxROC,"m",0,5000);
    responsesObj["deltaToReachMaxROD"] = toParm(deltaToReachMaxROD,"m",0,5000);
    obj["responses"] = responsesObj;

    // ---------------------------------------------------------------------
    // FLIGHT PHYSICS
    // ---------------------------------------------------------------------
    QJsonObject physicsObj;
    physicsObj["type"] = "Section";
    physicsObj["visible"] = false;
    physicsObj["minimumTakeoffSpeed"] = toParm(minimumTakeoffSpeed,"Km/h",0,1000);
    physicsObj["stallSpeed"] = toParm(stallSpeed,"Km/h",0,1000);
    physicsObj["criticalAngleOfAttack"] = toParm(criticalAngleOfAttack,"deg",0,90);
    physicsObj["gravity"] = toParm(gravity,"m/s^2",0,30);
    physicsObj["maxSinkSpeed"] = toParm(maxSinkSpeed,"m/s",0,300);
    physicsObj["stallRecoveryRate"] = toParm(stallRecoveryRate,"m/s^2",0,100);
    physicsObj["stalledLiftFactor"] = toParm(stalledLiftFactor,"ratio",0,1);
    physicsObj["takeoffPitchThreshold"] = toParm(takeoffPitchThreshold,"deg",0,30);
    physicsObj["groundLevel"] = toParm(groundLevel,"ft",-2000,100000);
    obj["flightPhysics"] = physicsObj;

    // ---------------------------------------------------------------------
    // STALL RECOVERY
    // ---------------------------------------------------------------------
    QJsonObject stallObj;
    stallObj["type"] = "Section";
    stallObj["visible"] = false;
    stallObj["autoStallRecovery"] = autoStallRecovery;
    stallObj["stallRecoveryPitch"] = toParm(stallRecoveryPitch,"deg",-45,10);
    obj["stallRecovery"] = stallObj;

    // ---------------------------------------------------------------------
    // AERODYNAMIC DRAG
    // ---------------------------------------------------------------------
    QJsonObject dragObj;
    dragObj["type"] = "Section";
    dragObj["visible"] = false;
    dragObj["zeroThrottleDrag"] = toParm(zeroThrottleDrag,"m/s^2",0,100);
    dragObj["highSpeedDrag"] = toParm(highSpeedDrag,"m/s^2",0,100);
    dragObj["inducedDrag"] = toParm(inducedDrag,"m/s^2",0,100);
    dragObj["referenceSpeed"] = toParm(dragReferenceSpeed,"m/s",1,1000);
    obj["aerodynamicDrag"] = dragObj;

    // ---------------------------------------------------------------------
    // ENVIRONMENT
    // ---------------------------------------------------------------------
    QJsonObject environmentObj;
    environmentObj["type"] = "Section";
    environmentObj["visible"] = false;
    environmentObj["windAngle"] = toParm(windAngle,"deg",0,360);
    environmentObj["windSpeed"] = toParm(windSpeed,"m/s",0,300);
    obj["environment"] = environmentObj;

    // ---------------------------------------------------------------------
    // RUNTIME TELEMETRY (published, not loaded as configuration)
    // ---------------------------------------------------------------------
    QJsonObject stateObj;
    stateObj["type"] = "Section";
    stateObj["visible"] = false;
    stateObj["currentSpeed"] = toParm(currentSpeed,"Km/h",0,10000);
    stateObj["currentAltitude"] = toParm(currentAltitude* 3281,"ft",-10000,200000);
    stateObj["currentAcceleration"] = toParm(currentAcceralation,"m/s^2",-500,500);
    stateObj["pitch"] = toParm(pitch,"deg",-180,180);
    stateObj["roll"] = toParm(roll,"deg",-180,180);
    stateObj["yaw"] = toParm(yaw,"deg",0,360);
    stateObj["Pitchrate"] = toParm(Pitchrate,"deg/s",-500,500);
    stateObj["Rollrate"] = toParm(Rollrate,"deg/s",-500,500);
    stateObj["Yawrate"] = toParm(Yawrate,"deg/s",-500,500);
    stateObj["currentTurnRate"] = toParm(currentTurnRate,"deg/s",-500,500);
    stateObj["TrueAirSpeed"] = toParm(TrueAirSpeed,"Km/h",0,10000);
    stateObj["GroundVelocity"] = toParm(GroundVelocity,"Km/h",0,10000);
    stateObj["VerticalVelocity"] = toParm(VerticalVelocity,"m/s",-1000,1000);
    stateObj["angleOfAttack"] = toParm(angleOfAttack,"deg",-180,180);
    stateObj["sideslipAngle"] = toParm(sideslipAngle,"deg",-180,180);
    stateObj["flightPathAngle"] = toParm(flightPathAngle,"deg",-90,90);
    stateObj["currentHeightAboveGround"] = toParm(currentHeightAboveGround*3281,"ft",-10000,200000);
    stateObj["liftFactor"] = toParm(liftFactor,"ratio",0,10);
    stateObj["isGrounded"] = isGrounded;
    stateObj["isStalling"] = isStalling;
    stateObj["horizonPitchRate"] = toParm(horizonPitchRate,"deg/s",-500,500);
    stateObj["horizonRollRate"] = toParm(horizonRollRate,"deg/s",-500,500);
    stateObj["horizonRatesValid"] = horizonRatesValid;
    obj["aircraftState"] = stateObj;

    QJsonObject velocityObj;
    velocityObj["type"] = "Section";
    velocityObj["visible"] = false;
    velocityObj["airVelocityX"] = toParm(airVelocity.x(),"m/s",-5000,5000);
    velocityObj["airVelocityY"] = toParm(airVelocity.y(),"m/s",-5000,5000);
    velocityObj["airVelocityZ"] = toParm(airVelocity.z(),"m/s",-5000,5000);
    velocityObj["bodyAirVelocityX"] = toParm(bodyAirVelocity.x(),"m/s",-5000,5000);
    velocityObj["bodyAirVelocityY"] = toParm(bodyAirVelocity.y(),"m/s",-5000,5000);
    velocityObj["bodyAirVelocityZ"] = toParm(bodyAirVelocity.z(),"m/s",-5000,5000);
    velocityObj["groundVelocityX"] = toParm(groundVelocity.x(),"m/s",-5000,5000);
    velocityObj["groundVelocityY"] = toParm(groundVelocity.y(),"m/s",-5000,5000);
    velocityObj["groundVelocityZ"] = toParm(groundVelocity.z(),"m/s",-5000,5000);
    velocityObj["bodyGroundVelocityX"] = toParm(bodyGroundVelocity.x(),"m/s",-5000,5000);
    velocityObj["bodyGroundVelocityY"] = toParm(bodyGroundVelocity.y(),"m/s",-5000,5000);
    velocityObj["bodyGroundVelocityZ"] = toParm(bodyGroundVelocity.z(),"m/s",-5000,5000);
    obj["velocityState"] = velocityObj;

    // Existing terrain/passability data remains unchanged.
    QJsonObject passabillityObj;
    passabillityObj["type"] = "Section";
    QJsonObject terrainSurfaceObj;
    terrainSurfaceObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : surfaceTypeOptions()) optionsArray.append(opt);
    terrainSurfaceObj["options"] = optionsArray;
    terrainSurfaceObj["value"] = surfaceTypeToString(terrainSurface);
    passabillityObj["terrainSurface"] = terrainSurfaceObj;
    passabillityObj["maximumSpeed"] = toParm(maximumSpeed,"%",0,1000);
    passabillityObj["terrainIsPassable"] = terrainIs;
    obj["passabillity"] = passabillityObj;

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    return obj;
}


/**
 * @brief Deserializes the DynamicModel component from JSON.
 * @param obj JSON object containing dynamic model data.
 */
void DynamicModel::fromJson(const QJsonObject& obj) {
    if (!parentEntity || !parentEntity->root)
        return;

    if (obj.contains("control"))
        control = obj["control"].toBool();

    if (obj.contains("dynamicType") && obj["dynamicType"].isObject()) {
        QJsonObject dynamicTypeObj = obj["dynamicType"].toObject();
        if (dynamicTypeObj.contains("value"))
            dynamicType = dynamicTypeObj["value"].toString();
    }

    if (obj.contains("takeoff") && obj["takeoff"].isObject()) {
        QJsonObject timeObj = obj["takeoff"].toObject();
        if (timeObj.contains("value") && timeObj["value"].isObject())
            startTime = valueFromParm(timeObj["value"].toObject());
    }

    // Use the same persistent-config parser used by editor/network sync.
    SyncByJson(obj);

    if (obj.contains("environment") && obj["environment"].isObject()) {
        QJsonObject e = obj["environment"].toObject();
        if (e.contains("windAngle") && e["windAngle"].isObject()) windAngle = valueFromParm(e["windAngle"].toObject());
        if (e.contains("windSpeed") && e["windSpeed"].isObject()) windSpeed = valueFromParm(e["windSpeed"].toObject());
    }

    if (obj.contains("passabillity") && obj["passabillity"].isObject()) {
        QJsonObject passabillityObj = obj["passabillity"].toObject();
        if (passabillityObj.contains("terrainSurface") && passabillityObj["terrainSurface"].isObject()) {
            QJsonObject terrainSurfaceObj = passabillityObj["terrainSurface"].toObject();
            if (terrainSurfaceObj.contains("value"))
                terrainSurface = stringTosurfaceType(terrainSurfaceObj["value"].toString());
        }
        if (passabillityObj.contains("maximumSpeed") && passabillityObj["maximumSpeed"].isObject())
            maximumSpeed = valueFromParm(passabillityObj["maximumSpeed"].toObject());
        if (passabillityObj.contains("terrainIsPassable") && passabillityObj["terrainIsPassable"].isBool())
            terrainIs = passabillityObj["terrainIsPassable"].toBool();
    }

    if (obj.contains("AdditionalParameters") && obj["AdditionalParameters"].isObject())
        AdditionalParameters = obj["AdditionalParameters"].toObject();

    // Preserve legacy safety/clamping behaviour.
    moveSpeed = qBound(minSpeed, moveSpeed, maxSpeed);
    commandedSpeed = qBound(0.0f, commandedSpeed, maxSpeed);
    if (Altitude > maxAltitude)
        Altitude = maxAltitude;
    if (commandedAltitude > maxAltitude && altitudeReference == AltitudeReference::WorldY)
        commandedAltitude = maxAltitude;

    if (parentEntity->category == Entity::Category::Marine &&
        (parentEntity->marineCategory == Entity::SubMarineCategory::Ship ||
         parentEntity->marineCategory == Entity::SubMarineCategory::Frigate)) {
        Altitude = 0.0f;
        commandedAltitude = 0.0f;
    }

    // Runtime aircraftState/velocityState sections are intentionally NOT read
    // here. They are outputs owned by Simulation/Aircraft and are republished
    // every update.

    if(!statesaved){
        statesaved = true;
        QJsonObject timeObj;
        timeObj["type"] = "Time";
        timeObj["value"] = toParm(startTime,"s");
        InitalState["takeoff"] = timeObj;

        // Store all persistent model configuration so RestoreState() can retain
        // the new Aircraft-related parameters as well.
        QJsonObject snapshot = toJson();
        InitalState["maximums"] = snapshot["maximums"];
        InitalState["commands"] = snapshot["commands"];
        InitalState["responses"] = snapshot["responses"];
        InitalState["flightPhysics"] = snapshot["flightPhysics"];
        InitalState["stallRecovery"] = snapshot["stallRecovery"];
        InitalState["aerodynamicDrag"] = snapshot["aerodynamicDrag"];
        InitalState["environment"] = snapshot["environment"];
    }
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

void DynamicModel::SaveState(){
    if(!parentEntity) return;

    statesaved = true;

    // Save only persistent configuration. Runtime aircraft telemetry is
    // recalculated by Aircraft after restore.
    QJsonObject snapshot = toJson();

    InitalState = QJsonObject();
    InitalState["takeoff"] = snapshot["takeoff"];
    InitalState["maximums"] = snapshot["maximums"];
    InitalState["commands"] = snapshot["commands"];
    InitalState["responses"] = snapshot["responses"];
    InitalState["flightPhysics"] = snapshot["flightPhysics"];
    InitalState["stallRecovery"] = snapshot["stallRecovery"];
    InitalState["aerodynamicDrag"] = snapshot["aerodynamicDrag"];
    InitalState["environment"] = snapshot["environment"];
    InitalState["passabillity"] = snapshot["passabillity"];
    InitalState["AdditionalParameters"] = snapshot["AdditionalParameters"];

    if(snapshot.contains("dynamicType"))
        InitalState["dynamicType"] = snapshot["dynamicType"];

    InitalState["control"] = control;
}


void DynamicModel::RestoreState(){
    fromJson(InitalState);
    control =true;
    time = 0;
}
/**
 * @brief Linear interpolation helper.
 */
float DynamicModel::lerp(float a, float b, float t){
    return a + (b - a) * t;
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
