
// // #include "dynamicmodel.h"
// // #include <core/InputSystem/inputmanager.h>
// // #include <QDebug>
// // #include <cmath>

// // auto normalizeAngle = [](float angle) {
// //     while (angle > 180.0f) angle -= 360.0f;
// //     while (angle < -180.0f) angle += 360.0f;
// //     return angle;
// // };
// // #define EARTH_RADIUS 6371000.0 // in meters
// // const double G_ACCELERATION = 9.8;
// // double toRadians(double degree) {
// //     return degree * M_PI / 180.0;
// // }
// // float delta = 0;
// // float speeed = 0;
// // double lastLat = 0;
// // double lastLon = 0;
// // float lastdist = 0;
// // double distanceBetween(double lat1, double lon1, double lat2, double lon2) {
// //     double dLat = toRadians(lat2 - lat1);
// //     double dLon = toRadians(lon2 - lon1);

// //     lat1 = toRadians(lat1);
// //     lat2 = toRadians(lat2);

// //     double a = pow(sin(dLat / 2), 2) +
// //                pow(sin(dLon / 2), 2) * cos(lat1) * cos(lat2);
// //     double c = 2 * atan2(sqrt(a), sqrt(1 - a));

// //     return EARTH_RADIUS * c; // Distance in meters
// // }

// // DynamicModel::DynamicModel() {
// //     controle = true;
// //     follow = true;
// //     moveSpeed = 1;
// //     customParameters = QJsonObject(); // Initialize customParameters
// //     //angdeg = transform->toEulerAngles().y();
// // }

// // void DynamicModel::Update(float deltaTime) {
// //     if (!controle || !transform || !rigidbody || !trajectory) return;

// //     // Direction vectors
// //     // QVector3D forwardDir = transform->forward(); // x-axis (forward)
// //     // QVector3D upDir = transform->up();           // z-axis (up)
// //     // QVector3D rightDir = transform->right();     // y-axis (right)
// //     time += deltaTime;
// //     delta = deltaTime;
// //     //qDebug() << time;
// //     if(start<time)
// //     {
// //         FollowTrajectory();
// //     }
// // }

// // // void DynamicModel::Update(float deltaTime) {
// // //     if (!controle || !transform || !trajectory) return;

// // //     // Get input from InputManager (or use autopilot values if `follow` is true)
// // //     float throttleInput = 1.0f;
// // //     float pitchInput = 0.0f;
// // //     float rollInput = 0.0f;
// // //     float yawInput = 0.0f;
// // //     bool brakes = false;

// // // /*    // Get input from InputManager (assuming it exists and provides values)
// // //     float throttleInput = InputManager::getThrottleInput(); // Assuming a value between 0 and 1
// // //     float pitchInput = InputManager::getPitchInput();     // Assuming a value between -1 and 1
// // //     float rollInput = InputManager::getRollInput();       // Assuming a value between -1 and 1
// // //     float yawInput = InputManager::getYawInput();         // Assuming a value between -1 and 1
// // //     bool brakes = InputManager::getAirBrakes();     */      // Assuming a boolean value


// // //     // --- Autopilot Logic to follow trajectory ---
// // //     if (follow && trajectory->Trajectories.size() > 0) {
// // //         // Get the current and target positions
// // //         QVector3D currentPosition = transform->translation();
// // //         Vector targetVector = *trajectory->Trajectories[trajectory->current]->position;
// // //         QVector3D targetPosition(targetVector.x, targetVector.y, targetVector.z);

// // //         // Check if we have reached the current waypoint
// // //         if (currentPosition.distanceToPoint(targetPosition) < 5.0f) { // Use a small radius
// // //             trajectory->current++;
// // //             if (trajectory->current >= trajectory->Trajectories.size()) {
// // //                 trajectory->current = 0; // Loop the trajectory
// // //             }
// // //         }

// // //         // Calculate the direction to the next waypoint
// // //         QVector3D directionToTarget = (targetPosition - currentPosition).normalized();
// // //         QVector3D localDirection = transform->inverseTransformDirection(directionToTarget);

// // //         // Calculate pitch and yaw inputs from the local direction
// // //         pitchInput = -localDirection.y() * 2.0f; // Scale factor for responsiveness
// // //         yawInput = localDirection.x() * 2.0f;   // Scale factor for responsiveness

// // //         // Auto-leveling for roll
// // //         rollInput = -transform->toEulerAngles().z() * autoRollLevel;
// // //     }

// // //     // Direction vectors based on the aircraft's current orientation
// // //     QVector3D forwardDir = transform->forward();
// // //     QVector3D upDir = transform->up();
// // //     QVector3D rightDir = transform->right();

// // //     // 1. Calculate and apply forces
// // //     QVector3D totalForce(0.0f, 0.0f, 0.0f);

// // //     // Thrust
// // //     enginePower = throttleInput * maxEnginePower;
// // //     totalForce += forwardDir * enginePower;

// // //     // Drag
// // //     forwardSpeed = velocity.length();
// // //     float drag = forwardSpeed * forwardSpeed * dragIncreaseFactor;
// // //     if (brakes) {
// // //         drag += forwardSpeed * airBrakesEffect;
// // //     }
// // //     if (forwardSpeed > 0) {
// // //         totalForce -= velocity.normalized() * drag;
// // //     }

// // //     // Lift
// // //     float lift = forwardSpeed * forwardSpeed * Lift;
// // //     if (forwardSpeed < zeroLiftSpeed) {
// // //         lift = 0;
// // //     }
// // //     totalForce += upDir * lift;

// // //     // Gravity (assuming a constant downward force)
// // //     //totalForce += QVector3D(0.0f, -9.8f, 0.0f); // Adjust gravity as needed

// // //     // 2. Update velocity and position
// // //     QVector3D acceleration = totalForce / mass;
// // //     velocity += acceleration * deltaTime;
// // //     transform->addTranslation(velocity * deltaTime);

// // //     // 3. Calculate and apply torques
// // //     QVector3D totalTorque(0.0f, 0.0f, 0.0f);
// // //     QVector3D localVelocity = transform->inverseTransformDirection(velocity);

// // //     // Pitch control (up/down)
// // //     float pitchTorque = pitchInput * pitchEffect*500;
// // //     pitchTorque += -localVelocity.y() * autoPitchLevel; // Auto-leveling
// // //     totalTorque += rightDir * pitchTorque * aerodynamicEffect;

// // //     qDebug()<<pitchInput <<","<<totalTorque;
// // //     // Roll control (left/right)
// // //     float rollTorque = rollInput * rollEffect*500;
// // //     rollTorque += -localVelocity.x() * autoRollLevel; // Auto-leveling
// // //     totalTorque += forwardDir * rollTorque * aerodynamicEffect;

// // //     // Yaw control (left/right turning)
// // //     float yawTorque = yawInput * yawEffect*500;
// // //     yawTorque += -rollInput * bankedTurnEffect;
// // //     totalTorque += upDir * yawTorque * aerodynamicEffect;

// // //     // 4. Update angular velocity and rotation
// // //     angularVelocity += totalTorque * deltaTime * rotationSpeed;
// // //     transform->setRotation(transform->rotation() * QQuaternion::fromEulerAngles(angularVelocity * deltaTime));

// // // }

// // void DynamicModel::FollowTrajectory() {

// //     if (follow) {
// //         // QVector3D current = *transform->position;

// //         // // 🎯 Step 1: Base target is followEntity's position
// //         // QVector3D target = *followEntity->transform->position;

// //         // // 🎯 Step 2: Apply Offset if formationPosition is available
// //         // if (formationPosition && formationPosition->Offset) {
// //         //     //target += *formationPosition->Offset;
// //         //     target += QVector3D(formationPosition->Offset->x, formationPosition->Offset->y, formationPosition->Offset->z);
// //         // }

// //         // // 🔁 Step 3: Move towards target
// //         // QVector3D diff = target - current;
// //         // float distance = diff.length();

// //         // if (distance > 0.001f) {
// //         //     QVector3D dir = diff.normalized();
// //         //     current += dir * moveSpeed * 0.01f;
// //         // }

// //         // *transform->position = current;

// //         // // 🔄 Step 4: Update rotation
// //         // QVector3D direction = target - current;
// //         // if (direction.length() > 0.001f) {
// //         //     direction = direction.normalized();
// //         //     float angleRad = atan2(direction.y(), direction.x());
// //         //     float angleDeg = angleRad * (180.0f / M_PI);
// //         //     *transform->rotation = QQuaternion::fromAxisAndAngle(angleDeg, 0, 0, 1);
// //         // }

// //         // return; // Skip trajectory logic
// //     }
// //     if(trajectory->Trajectories.size()<2) return;
// //     QVector3D current = transform->matrix->translation();
// //     Vector target = *trajectory->Trajectories[trajectory->current]->position;
// //     QVector3D target_qvec(target.x, target.y, target.z);
// //     QVector3D diff = target_qvec - current;
// //     float distance = diff.length();
// //     float metredis = distanceBetween(target.x,target.z,current.x(),current.z());

// //     //if((trajectory->Trajectories.size()-1) == trajectory->current && distance <0.05f ) return;
// //     if (metredis > 100) {
// //         QVector3D dir = diff.normalized();
// //         QVector3D last(current.x(), current.y(), current.z());

// //         current += transform->forward() * speeed * 0.0001f;
// //         float deltaDis = distanceBetween(last.x(),last.z(),current.x(),current.z());
// //         float time = 1/delta;
// //         currentSpeed = deltaDis*time;

// //         speeed += (currentSpeed<(moveSpeed/3.6f))?0.1f:-0.1f;
// //         //qDebug()<<currentSpeed;
// //         QVector3D targetAsQVector3D(target.x, target.y, target.z);
// //         QVector3D direction = targetAsQVector3D - current;

// //         direction = direction.normalized();

// //         float angleRad = atan2(direction.x(), direction.z());
// //         float angleDeg = angleRad * (180.0f / M_PI);

// //         float delta = normalizeAngle(angleDeg - angdeg);
// //         double tangent_argument = pow(speeed, 2) / (turnRadius * G_ACCELERATION);

// //         // 2. arctan का उपयोग करके बैंक कोण (रेडियन में) की गणना
// //         float bank_angle_radians = atan(tangent_argument);

// //         // 3. रेडियन को डिग्री में बदलना
// //         // 1 रेडियन = 180 / PI डिग्री
// //         float bank_angle_degrees = bank_angle_radians * (180.0 / M_PI);
// //         angdeg += delta * bank_angle_degrees;//1 * 0.04f;
// //         angdeg = normalizeAngle(angdeg);

// //         //angdeg = lerp(angdeg,angleDeg,moveSpeed * 0.004f);


// //         // current.setZ(current.z()-(sin(angdeg)*moveSpeed*0.01f));
// //         // current.setX(current.x()+(cos(angdeg)*moveSpeed*0.01f));
// //         //*transform->rotation = QQuaternion::fromAxisAndAngle(angdeg, 0.0f, 1.0f, 0.0f);
// //         //QQuaternion targetRotation = QQuaternion::fromEulerAngles(QVector3D(0,-angdeg,0));
// //         transform->setFromEulerAngles(QVector3D(0,angdeg,0));
// //         // Smoothly interpolate between the current rotation and the target rotation.
// //         //*transform->rotation = QQuaternion::slerp(*transform->rotation, targetRotation, moveSpeed * 0.05f);
// //     }

// //     transform->setTranslation(current);

// //     if(distanceBetween(lastLat,lastLon,current.x(),current.z())>1){
// //         transform->trailData.push_back(current);
// //     }
// //     lastLat = current.x();
// //     lastLon = current.z();

// //     ////////////////////////////////////////////////////
// //     if( transform->trailData.capacity()>4000){
// //         transform->trailData.erase(transform->trailData.begin());
// //     }
// //     ////////////////////////////////////////////////////
// //     //*transform->position = Vector::Lerp(*transform->position, *trajectory->Trajectories[trajectory->current]->position, moveSpeed * 0.1);
// //     metredis = distanceBetween(trajectory->Trajectories[trajectory->current]->position->x,
// //                                trajectory->Trajectories[trajectory->current]->position->z,
// //                                current.x(),
// //                                current.z());
// //     if (trajectory->Trajectories.size() > trajectory->current && /*(transform->matrix->translation()).distanceToPoint(QVector3D(
// //                                                                      trajectory->Trajectories[trajectory->current]->position->x,
// //                                                                      trajectory->Trajectories[trajectory->current]->position->y,
// //                                                                      trajectory->Trajectories[trajectory->current]->position->z
// //                                                                      )*/ metredis
// //                                                                      < 500) {
// //         trajectory->current += 1;
// //         //trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? (trajectory->Trajectories.size()-1) : trajectory->current;
// //         trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? 0: trajectory->current;
// //     }
// // }

// // float DynamicModel::lerp(float a, float b, float t){
// //     return a + (b - a) * t;
// // }



// // QJsonObject DynamicModel::toJson() const {
// //     QJsonObject obj;
// //     obj["controle"] = controle;
// //     obj["maxEnginePower"] = maxEnginePower;
// //     obj["Lift"] = Lift;
// //     obj["zeroLiftSpeed"] = zeroLiftSpeed;
// //     obj["moveSpeed"] = moveSpeed;
// //     obj["turnRadius"] = turnRadius;
// //     obj["start"] = start;
// //     obj["rotationSpeed"] = rotationSpeed;
// //     obj["dragIncreaseFactor"] = dragIncreaseFactor;
// //     obj["aerodynamicEffect"] = aerodynamicEffect;
// //     obj["airBrakesEffect"] = airBrakesEffect;
// //     obj["rollEffect"] = rollEffect;
// //     obj["pitchEffect"] = pitchEffect;
// //     obj["yawEffect"] = yawEffect;
// //     obj["bankedTurnEffect"] = bankedTurnEffect;
// //     obj["autoRollLevel"] = autoRollLevel;
// //     obj["autoPitchLevel"] = autoPitchLevel;
// //     obj["type"] = "component";

// //     // Add custom parameters
// //     for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
// //         obj[it.key()] = it.value();
// //     }

// //     //qDebug() << "DynamicModel::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
// //     return obj;
// // }

// // void DynamicModel::fromJson(const QJsonObject& obj) {
// //     //qDebug() << "DynamicModel::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

// //     // Standard fields
// //     if (obj.contains("controle"))
// //         controle = obj["controle"].toBool();
// //     if (obj.contains("maxEnginePower"))
// //         maxEnginePower = obj["maxEnginePower"].toVariant().toDouble();
// //     if (obj.contains("Lift"))
// //         Lift = obj["Lift"].toVariant().toDouble();
// //     if (obj.contains("zeroLiftSpeed"))
// //         zeroLiftSpeed = obj["zeroLiftSpeed"].toVariant().toDouble();
// //     if (obj.contains("moveSpeed"))
// //         moveSpeed = obj["moveSpeed"].toVariant().toDouble();
// //     if (obj.contains("turnRadius"))
// //         turnRadius = obj["turnRadius"].toVariant().toDouble();
// //     if (obj.contains("start"))
// //         start = obj["start"].toVariant().toDouble();
// //     if (obj.contains("rotationSpeed"))
// //         rotationSpeed = obj["rotationSpeed"].toVariant().toDouble();
// //     if (obj.contains("dragIncreaseFactor"))
// //         dragIncreaseFactor = obj["dragIncreaseFactor"].toVariant().toDouble();
// //     if (obj.contains("aerodynamicEffect"))
// //         aerodynamicEffect = obj["aerodynamicEffect"].toVariant().toDouble();
// //     if (obj.contains("airBrakesEffect"))
// //         airBrakesEffect = obj["airBrakesEffect"].toVariant().toDouble();
// //     if (obj.contains("rollEffect"))
// //         rollEffect = obj["rollEffect"].toVariant().toDouble();
// //     if (obj.contains("pitchEffect"))
// //         pitchEffect = obj["pitchEffect"].toVariant().toDouble();
// //     if (obj.contains("yawEffect"))
// //         yawEffect = obj["yawEffect"].toVariant().toDouble();
// //     if (obj.contains("bankedTurnEffect"))
// //         bankedTurnEffect = obj["bankedTurnEffect"].toVariant().toDouble();
// //     if (obj.contains("autoRollLevel"))
// //         autoRollLevel = obj["autoRollLevel"].toVariant().toDouble();
// //     if (obj.contains("autoPitchLevel"))
// //         autoPitchLevel = obj["autoPitchLevel"].toVariant().toDouble();

// //     // Custom parameters
// //     QStringList standardKeys = {
// //         "controle", "maxEnginePower", "Lift", "zeroLiftSpeed", "moveSpeed",
// //         "rotationSpeed", "dragIncreaseFactor", "aerodynamicEffect", "airBrakesEffect",
// //         "rollEffect", "pitchEffect", "yawEffect", "bankedTurnEffect",
// //         "autoRollLevel", "autoPitchLevel"
// //     };
// //     for (auto it = obj.begin(); it != obj.end(); ++it) {
// //         if (!standardKeys.contains(it.key())) {
// //             customParameters[it.key()] = it.value();
// //         }
// //     }

// //     //qDebug() << "DynamicModel::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
// // }

// // void DynamicModel::setMoveSpeed(float speed) {
// //     //moveSpeed = qBound(1.0f, speed, 10.0f);
// // }




// #include "dynamicmodel.h"
// #include <core/InputSystem/inputmanager.h>
// #include "core/Hierarchy/Utils/entityutils.h"
// #include <QDebug>

// auto normalizeAngle = [](float angle) {
//     while (angle > 180.0f) angle -= 360.0f;
//     while (angle < -180.0f) angle += 360.0f;
//     return angle;
// };

// float delta = 0;
// float speeed = 0;
// double lastLat = 0;
// double lastLon = 0;
// float lastdist = 0;


// DynamicModel::DynamicModel() {
//     controle = true;
//     follow = true;
//     moveSpeed = 1;
//     customParameters = QJsonObject(); // Initialize customParameters
// }

// void DynamicModel::Update(float deltaTime) {
//     if (!controle || !transform || !rigidbody || !trajectory) return;

//     // Direction vectors
//     // QVector3D forwardDir = transform->forward(); // x-axis (forward)
//     // QVector3D upDir = transform->up();           // z-axis (up)
//     // QVector3D rightDir = transform->right();     // y-axis (right)
//     time += deltaTime;
//     delta = deltaTime;
//     //qDebug() << time;
//     if(start<time)
//     {
//         FollowTrajectory();
//     }
// }

// // void DynamicModel::Update(float deltaTime) {
// //     if (!controle || !transform || !trajectory) return;

// //     // Get input from InputManager (or use autopilot values if `follow` is true)
// //     float throttleInput = 1.0f;
// //     float pitchInput = 0.0f;
// //     float rollInput = 0.0f;
// //     float yawInput = 0.0f;
// //     bool brakes = false;

// // /*    // Get input from InputManager (assuming it exists and provides values)
// //     float throttleInput = InputManager::getThrottleInput(); // Assuming a value between 0 and 1
// //     float pitchInput = InputManager::getPitchInput();     // Assuming a value between -1 and 1
// //     float rollInput = InputManager::getRollInput();       // Assuming a value between -1 and 1
// //     float yawInput = InputManager::getYawInput();         // Assuming a value between -1 and 1
// //     bool brakes = InputManager::getAirBrakes();     */      // Assuming a boolean value


// //     // --- Autopilot Logic to follow trajectory ---
// //     if (follow && trajectory->Trajectories.size() > 0) {
// //         // Get the current and target positions
// //         QVector3D currentPosition = transform->translation();
// //         Vector targetVector = *trajectory->Trajectories[trajectory->current]->position;
// //         QVector3D targetPosition(targetVector.x, targetVector.y, targetVector.z);

// //         // Check if we have reached the current waypoint
// //         if (currentPosition.distanceToPoint(targetPosition) < 5.0f) { // Use a small radius
// //             trajectory->current++;
// //             if (trajectory->current >= trajectory->Trajectories.size()) {
// //                 trajectory->current = 0; // Loop the trajectory
// //             }
// //         }

// //         // Calculate the direction to the next waypoint
// //         QVector3D directionToTarget = (targetPosition - currentPosition).normalized();
// //         QVector3D localDirection = transform->inverseTransformDirection(directionToTarget);

// //         // Calculate pitch and yaw inputs from the local direction
// //         pitchInput = -localDirection.y() * 2.0f; // Scale factor for responsiveness
// //         yawInput = localDirection.x() * 2.0f;   // Scale factor for responsiveness

// //         // Auto-leveling for roll
// //         rollInput = -transform->toEulerAngles().z() * autoRollLevel;
// //     }

// //     // Direction vectors based on the aircraft's current orientation
// //     QVector3D forwardDir = transform->forward();
// //     QVector3D upDir = transform->up();
// //     QVector3D rightDir = transform->right();

// //     // 1. Calculate and apply forces
// //     QVector3D totalForce(0.0f, 0.0f, 0.0f);

// //     // Thrust
// //     enginePower = throttleInput * maxEnginePower;
// //     totalForce += forwardDir * enginePower;

// //     // Drag
// //     forwardSpeed = velocity.length();
// //     float drag = forwardSpeed * forwardSpeed * dragIncreaseFactor;
// //     if (brakes) {
// //         drag += forwardSpeed * airBrakesEffect;
// //     }
// //     if (forwardSpeed > 0) {
// //         totalForce -= velocity.normalized() * drag;
// //     }

// //     // Lift
// //     float lift = forwardSpeed * forwardSpeed * Lift;
// //     if (forwardSpeed < zeroLiftSpeed) {
// //         lift = 0;
// //     }
// //     totalForce += upDir * lift;

// //     // Gravity (assuming a constant downward force)
// //     //totalForce += QVector3D(0.0f, -9.8f, 0.0f); // Adjust gravity as needed

// //     // 2. Update velocity and position
// //     QVector3D acceleration = totalForce / mass;
// //     velocity += acceleration * deltaTime;
// //     transform->addTranslation(velocity * deltaTime);

// //     // 3. Calculate and apply torques
// //     QVector3D totalTorque(0.0f, 0.0f, 0.0f);
// //     QVector3D localVelocity = transform->inverseTransformDirection(velocity);

// //     // Pitch control (up/down)
// //     float pitchTorque = pitchInput * pitchEffect*500;
// //     pitchTorque += -localVelocity.y() * autoPitchLevel; // Auto-leveling
// //     totalTorque += rightDir * pitchTorque * aerodynamicEffect;

// //     qDebug()<<pitchInput <<","<<totalTorque;
// //     // Roll control (left/right)
// //     float rollTorque = rollInput * rollEffect*500;
// //     rollTorque += -localVelocity.x() * autoRollLevel; // Auto-leveling
// //     totalTorque += forwardDir * rollTorque * aerodynamicEffect;

// //     // Yaw control (left/right turning)
// //     float yawTorque = yawInput * yawEffect*500;
// //     yawTorque += -rollInput * bankedTurnEffect;
// //     totalTorque += upDir * yawTorque * aerodynamicEffect;

// //     // 4. Update angular velocity and rotation
// //     angularVelocity += totalTorque * deltaTime * rotationSpeed;
// //     transform->setRotation(transform->rotation() * QQuaternion::fromEulerAngles(angularVelocity * deltaTime));

// // }

// void DynamicModel::FollowTrajectory() {

//     if (follow) {
//         // QVector3D current = *transform->position;

//         // // 🎯 Step 1: Base target is followEntity's position
//         // QVector3D target = *followEntity->transform->position;

//         // // 🎯 Step 2: Apply Offset if formationPosition is available
//         // if (formationPosition && formationPosition->Offset) {
//         //     //target += *formationPosition->Offset;
//         //     target += QVector3D(formationPosition->Offset->x, formationPosition->Offset->y, formationPosition->Offset->z);
//         // }

//         // // 🔁 Step 3: Move towards target
//         // QVector3D diff = target - current;
//         // float distance = diff.length();

//         // if (distance > 0.001f) {
//         //     QVector3D dir = diff.normalized();
//         //     current += dir * moveSpeed * 0.01f;
//         // }

//         // *transform->position = current;

//         // // 🔄 Step 4: Update rotation
//         // QVector3D direction = target - current;
//         // if (direction.length() > 0.001f) {
//         //     direction = direction.normalized();
//         //     float angleRad = atan2(direction.y(), direction.x());
//         //     float angleDeg = angleRad * (180.0f / M_PI);
//         //     *transform->rotation = QQuaternion::fromAxisAndAngle(angleDeg, 0, 0, 1);
//         // }

//         // return; // Skip trajectory logic
//     }
//     if(trajectory->Trajectories.size()<2) return;
//     QVector3D current = transform->matrix->translation();
//     Vector target = *trajectory->Trajectories[trajectory->current]->position;
//     QVector3D target_qvec(target.x, target.y, target.z);
//     QVector3D diff = target_qvec - current;
//     float distance = diff.length();
//     float metredis =  distanceBetween(target.x,target.z,current.x(),current.z());

//     //if((trajectory->Trajectories.size()-1) == trajectory->current && distance <0.05f ) return;
//     if (metredis > 100) {
//         QVector3D dir = diff.normalized();
//         QVector3D last(current.x(), current.y(), current.z());

//         current += transform->forward() * speeed * 0.0001f;
//         float deltaDis = distanceBetween(last.x(),last.z(),current.x(),current.z());
//         float time = 1/delta;
//         currentSpeed = deltaDis*time;

//         speeed += (currentSpeed<(moveSpeed/3.6f))?0.1f:-0.1f;
//         //qDebug()<<currentSpeed;
//         QVector3D targetAsQVector3D(target.x, target.y, target.z);
//         QVector3D direction = targetAsQVector3D - current;

//         direction = direction.normalized();

//         float angleRad = atan2(direction.x(), direction.z());
//         float angleDeg = angleRad * (180.0f / M_PI);

//         float delta = normalizeAngle(angleDeg - angdeg);
//         double tangent_argument = pow(speeed, 2) / (turnRadius * G_ACCELERATION);

//         // 2. arctan का उपयोग करके बैंक कोण (रेडियन में) की गणना
//         float bank_angle_radians = atan(tangent_argument);

//         // 3. रेडियन को डिग्री में बदलना
//         // 1 रेडियन = 180 / PI डिग्री
//         float bank_angle_degrees = bank_angle_radians * (180.0 / M_PI);
//         angdeg += delta * bank_angle_degrees;//1 * 0.04f;
//         angdeg = normalizeAngle(angdeg);

//         //angdeg = lerp(angdeg,angleDeg,moveSpeed * 0.004f);


//         // current.setZ(current.z()-(sin(angdeg)*moveSpeed*0.01f));
//         // current.setX(current.x()+(cos(angdeg)*moveSpeed*0.01f));
//         //*transform->rotation = QQuaternion::fromAxisAndAngle(angdeg, 0.0f, 1.0f, 0.0f);
//         //QQuaternion targetRotation = QQuaternion::fromEulerAngles(QVector3D(0,-angdeg,0));
//         transform->setFromEulerAngles(QVector3D(0,angdeg,0));
//         // Smoothly interpolate between the current rotation and the target rotation.
//         //*transform->rotation = QQuaternion::slerp(*transform->rotation, targetRotation, moveSpeed * 0.05f);
//     }

//     transform->setTranslation(current);

//     if(distanceBetween(lastLat,lastLon,current.x(),current.z())>1){
//         transform->trailData.push_back(current);
//     }
//     lastLat = current.x();
//     lastLon = current.z();

//     ////////////////////////////////////////////////////
//     if( transform->trailData.capacity()>4000){
//         transform->trailData.erase(transform->trailData.begin());
//     }
//     ////////////////////////////////////////////////////
//     //*transform->position = Vector::Lerp(*transform->position, *trajectory->Trajectories[trajectory->current]->position, moveSpeed * 0.1);
//     metredis = distanceBetween(trajectory->Trajectories[trajectory->current]->position->x,
//                                trajectory->Trajectories[trajectory->current]->position->z,
//                                current.x(),
//                                current.z());
//     if (trajectory->Trajectories.size() > trajectory->current && /*(transform->matrix->translation()).distanceToPoint(QVector3D(
//                                                                      trajectory->Trajectories[trajectory->current]->position->x,
//                                                                      trajectory->Trajectories[trajectory->current]->position->y,
//                                                                      trajectory->Trajectories[trajectory->current]->position->z
//                                                                      )*/ metredis
//                                                                      < 500) {
//         trajectory->current += 1;
//         //trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? (trajectory->Trajectories.size()-1) : trajectory->current;
//         trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? 0: trajectory->current;
//     }
// }

// float DynamicModel::lerp(float a, float b, float t){
//     return a + (b - a) * t;
// }

// QJsonObject DynamicModel::toJson() const {
//     QJsonObject obj;
//     obj["controle"] = controle;
//     obj["maxEnginePower"] = maxEnginePower;
//     obj["Lift"] = Lift;
//     obj["zeroLiftSpeed"] = zeroLiftSpeed;
//     obj["moveSpeed"] = moveSpeed;
//     obj["turnRadius"] = turnRadius;
//     obj["start"] = start;
//     obj["rotationSpeed"] = rotationSpeed;
//     obj["dragIncreaseFactor"] = dragIncreaseFactor;
//     obj["aerodynamicEffect"] = aerodynamicEffect;
//     obj["airBrakesEffect"] = airBrakesEffect;
//     obj["rollEffect"] = rollEffect;
//     obj["pitchEffect"] = pitchEffect;
//     obj["yawEffect"] = yawEffect;
//     obj["bankedTurnEffect"] = bankedTurnEffect;
//     obj["autoRollLevel"] = autoRollLevel;
//     obj["autoPitchLevel"] = autoPitchLevel;
//     obj["type"] = "component";

//     // Add custom parameters
//     for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
//         obj[it.key()] = it.value();
//     }

//     //qDebug() << "DynamicModel::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
//     return obj;
// }

// void DynamicModel::fromJson(const QJsonObject& obj) {
//     //qDebug() << "DynamicModel::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

//     // Standard fields
//     if (obj.contains("controle"))
//         controle = obj["controle"].toBool();
//     if (obj.contains("maxEnginePower"))
//         maxEnginePower = obj["maxEnginePower"].toVariant().toDouble();
//     if (obj.contains("Lift"))
//         Lift = obj["Lift"].toVariant().toDouble();
//     if (obj.contains("zeroLiftSpeed"))
//         zeroLiftSpeed = obj["zeroLiftSpeed"].toVariant().toDouble();
//     if (obj.contains("moveSpeed"))
//         moveSpeed = obj["moveSpeed"].toVariant().toDouble();
//     if (obj.contains("turnRadius"))
//         turnRadius = obj["turnRadius"].toVariant().toDouble();
//     if (obj.contains("start"))
//         start = obj["start"].toVariant().toDouble();
//     if (obj.contains("rotationSpeed"))
//         rotationSpeed = obj["rotationSpeed"].toVariant().toDouble();
//     if (obj.contains("dragIncreaseFactor"))
//         dragIncreaseFactor = obj["dragIncreaseFactor"].toVariant().toDouble();
//     if (obj.contains("aerodynamicEffect"))
//         aerodynamicEffect = obj["aerodynamicEffect"].toVariant().toDouble();
//     if (obj.contains("airBrakesEffect"))
//         airBrakesEffect = obj["airBrakesEffect"].toVariant().toDouble();
//     if (obj.contains("rollEffect"))
//         rollEffect = obj["rollEffect"].toVariant().toDouble();
//     if (obj.contains("pitchEffect"))
//         pitchEffect = obj["pitchEffect"].toVariant().toDouble();
//     if (obj.contains("yawEffect"))
//         yawEffect = obj["yawEffect"].toVariant().toDouble();
//     if (obj.contains("bankedTurnEffect"))
//         bankedTurnEffect = obj["bankedTurnEffect"].toVariant().toDouble();
//     if (obj.contains("autoRollLevel"))
//         autoRollLevel = obj["autoRollLevel"].toVariant().toDouble();
//     if (obj.contains("autoPitchLevel"))
//         autoPitchLevel = obj["autoPitchLevel"].toVariant().toDouble();

//     // Custom parameters
//     QStringList standardKeys = {
//         "controle", "maxEnginePower", "Lift", "zeroLiftSpeed", "moveSpeed",
//         "rotationSpeed", "dragIncreaseFactor", "aerodynamicEffect", "airBrakesEffect",
//         "rollEffect", "pitchEffect", "yawEffect", "bankedTurnEffect",
//         "autoRollLevel", "autoPitchLevel"
//     };
//     for (auto it = obj.begin(); it != obj.end(); ++it) {
//         if (!standardKeys.contains(it.key())) {
//             customParameters[it.key()] = it.value();
//         }
//     }

//     //qDebug() << "DynamicModel::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
// }

// void DynamicModel::setMoveSpeed(float speed) {
//     //moveSpeed = qBound(1.0f, speed, 10.0f);
// }



#include "dynamicmodel.h"

#include <core/InputSystem/inputmanager.h>

#include "core/Hierarchy/Utils/entityutils.h"

#include <QDebug>



auto normalizeAngle = [](float angle) {

    while (angle > 180.0f) angle -= 360.0f;

    while (angle < -180.0f) angle += 360.0f;

    return angle;

};



// float delta = 0;

// float speeed = 0;

// double lastLat = 0;

// double lastLon = 0;

// float lastdist = 0;





DynamicModel::DynamicModel() {

    controle = true;

    follow = true;

    moveSpeed = 1;

    customParameters = QJsonObject(); // Initialize customParameters

}




void DynamicModel::Update(float deltaTime) {

    if (!controle || !transform || !rigidbody || !trajectory || isStopped) return; // 💡 isStopped चेक जोड़ा गया



    time += deltaTime;

    this->delta = deltaTime;



    if (start < time)

    {

        if (!isInitialized) {

            trajectory->current = 0;

            speeed = 0.0f;

            isInitialized = true;

            isStopped = false; // 💡 सिमुलेशन शुरू होने पर रीसेट करें

        }



        FollowTrajectory();

    }

}

void DynamicModel::FollowTrajectory() {



    if(trajectory->Trajectories.size() < 2) return;



    QVector3D current = transform->matrix->translation();



    // 1. Target and Distance Calculation

    Vector target = *trajectory->Trajectories[trajectory->current]->position;

    QVector3D target_qvec(target.x, target.y, target.z);



    // Calculate 2D distance to the current target waypoint

    float metredis = distanceBetween(target.x, target.z, current.x(), current.z());



    bool isFinalWaypoint = (trajectory->current == trajectory->Trajectories.size() - 1);



    // 2. Waypoint Progression Logic (Switch to next waypoint if within 50m)

    if (trajectory->Trajectories.size() > trajectory->current && metredis < 50) {

        trajectory->current += 1;



        // 🛑 LOOP STOPPING: Locks the index at the last waypoint

        if (trajectory->current >= trajectory->Trajectories.size()) {

            trajectory->current = trajectory->Trajectories.size() - 1;

            // 💡 तुरंत चेक करें: यदि यह अंतिम वेपॉइंट था, तो गति शून्य कर दें।

            speeed = 0.0f;

            // यदि अंतिम वेपॉइंट पर पहुँचे, तो यहीं से बाहर निकलें ताकि कोई अनावश्यक मूवमेंट न हो

            if (metredis < 5) {

                transform->setTranslation(target_qvec); // Snap to the target

                return;

            }

        }

    }



    // 3. Movement and Steering Logic Initialization

    QVector3D last(current.x(), current.y(), current.z());



    // --- Speed Control ---

    float distance_to_move_this_frame = speeed * 0.0001f;



    if (isFinalWaypoint && metredis < 5) {

        // 🚀 अंतिम वेपॉइंट पर कठोर क्लैम्पिंग और ब्रेकिंग



        // 1. गति को तुरंत 0 करें

        speeed = lerp(speeed, 0.0f, 0.1f);

        if (speeed < 0.01f) {

            speeed = 0.0f;

        }



        // 2. कठोर स्टॉप: यदि Entity 1 मीटर के भीतर है और गति 0 है, तो फ़ंक्शन से बाहर निकलें।

        if (speeed == 0.0f && metredis < 1.0f) {

            transform->setTranslation(target_qvec); // Snap to exact position



            isStopped = true;

            return; // 🎯 फ़्लिकरिंग को रोकने के लिए तुरंत बाहर निकलें



        }



        // 3. दूरी को क्लैम्प करें (Overshooting रोकने के लिए)

        distance_to_move_this_frame = qMin(distance_to_move_this_frame, metredis);



    } else if (metredis > 1.0f) { // Normal acceleration/deceleration

        speeed += (currentSpeed < (moveSpeed / 3.6f)) ? 0.1f : -0.1f;

        distance_to_move_this_frame = speeed * 0.0001f;

    } else {

        // Stop movement if too close (even to mid-waypoint)

        speeed = 0.0f;

        distance_to_move_this_frame = 0.0f;

    }





    // --- Apply Translation ---

    if (distance_to_move_this_frame > 0.0f) {

        current += transform->forward() * distance_to_move_this_frame;

    }



    // Speed Recalculation (for feedback loop)

    float deltaDis = distanceBetween(last.x(), last.z(), current.x(), current.z());

    float time = 1/delta;

    currentSpeed = deltaDis * time;



    // --- Rotation/Steering Control ---



    // ⭐ सुधार: केवल तभी घूमें जब Entity चल रही हो (speeed > 0)

    if (speeed > 0.0f) {

        // Target direction calculation

        QVector3D direction = target_qvec - current;

        direction = direction.normalized();



        float angleRad = atan2(direction.x(), direction.z());

        float targetAngle = angleRad * (180.0f / M_PI); // Target Angle



        // Turn Stop Check (5m के भीतर मुड़ना बंद)

        if (isFinalWaypoint && metredis < 5.0f) {

            // Keep angle constant

        } else {

            // Dynamic Smooth Factor

            float dynamicSmoothFactor = moveSpeed * 0.00001f;

            float clampedSmoothFactor = qMin(dynamicSmoothFactor, 0.1f);



            angdeg = lerp(angdeg, targetAngle, clampedSmoothFactor);

        }



        angdeg = normalizeAngle(angdeg);

        transform->setFromEulerAngles(QVector3D(0, angdeg, 0));

    }



    // Apply new position

    transform->setTranslation(current);



    // --- Trail/Track logging logic ---

    if(distanceBetween(lastLat,lastLon,current.x(),current.z()) > 1){

        transform->trailData.push_back(current);

    }

    lastLat = current.x();

    lastLon = current.z();



    if( transform->trailData.capacity() > 4000){

        transform->trailData.erase(transform->trailData.begin());

    }



    // Recalculate metredis (for the next frame)

    metredis = distanceBetween(trajectory->Trajectories[trajectory->current]->position->x,

                               trajectory->Trajectories[trajectory->current]->position->z,

                               current.x(),

                               current.z());

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

    obj["turnRadius"] = turnRadius;

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

    if (obj.contains("turnRadius"))

    turnRadius = obj["turnRadius"].toVariant().toDouble();

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

    //moveSpeed = qBound(1.0f, speed, 10.0f);

}








