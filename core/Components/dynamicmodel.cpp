#include "dynamicmodel.h"
#include <core/InputSystem/inputmanager.h>

DynamicModel::DynamicModel() {
    controle = false;
}

void DynamicModel::Update(float deltaTime) {
    if (!controle || !transform || !rigidbody) return;

    // Direction vectors
    Vector forwardDir = transform->forward(); // x-axis (forward)
    Vector upDir = transform->up();           // z-axis (up)
    Vector rightDir = transform->right();     // y-axis (right)

    // Input mapping
    float rollInput = 0, pitchInput = 0, yawInput = 0, throttleInput = 0;
    bool airBrakes = INPUT->isKeyPressed(Qt::Key_Space);

    if (INPUT->isKeyPressed(Qt::Key_W)) pitchInput = 1;  // Nose down
    if (INPUT->isKeyPressed(Qt::Key_S)) pitchInput = -1;   // Nose up
    if (INPUT->isKeyPressed(Qt::Key_A)) rollInput = 1;    // Roll left
    if (INPUT->isKeyPressed(Qt::Key_D)) rollInput = -1;   // Roll right
    if (INPUT->isKeyPressed(Qt::Key_Q)) yawInput = -1;    // Yaw left
    if (INPUT->isKeyPressed(Qt::Key_E)) yawInput = 1;     // Yaw right
    if (INPUT->isKeyPressed(Qt::Key_Shift)) throttleInput = 1;
    if (INPUT->isKeyPressed(Qt::Key_Control)) throttleInput = -1;

    // Clamp inputs
    rollInput = qBound(-1.0f, rollInput, 1.0f);
    pitchInput = qBound(-1.0f, pitchInput, 1.0f);
    yawInput = qBound(-1.0f, yawInput, 1.0f);
    throttleInput = qBound(-1.0f, throttleInput, 1.0f);

    // Throttle and velocity
    throttle += throttleInput * deltaTime * 0.5f;
    throttle = qBound(0.0f, throttle, 1.0f);
    float speed = throttle * maxEnginePower; // maxSpeed replaces maxEnginePower

    // Set velocity along forward direction
    *rigidbody->velocity = forwardDir * speed;
    if (airBrakes) {
        *rigidbody->velocity *= (1.0f - deltaTime * airBrakesEffect);
    }
    rigidbody->setLinearVelocity(*rigidbody->velocity);

    // Define angular velocity in local axes (roll on x, pitch on y, yaw on z)
    Vector localAngularVelocity(
        rollInput * rotationSpeed,   // Roll (local x-axis, forward)
        pitchInput * rotationSpeed,  // Pitch (local y-axis, right)
        yawInput * rotationSpeed     // Yaw (local z-axis, up)
        );
    // Transform to global axes
    Vector globalAngularVelocity =
        forwardDir * localAngularVelocity.x +  // Roll contribution
        rightDir * localAngularVelocity.y +    // Pitch contribution
        upDir * localAngularVelocity.z;        // Yaw contribution
    rigidbody->setAngularVelocity(globalAngularVelocity);
    // Altitude
    altitude = transform->position->z;
}

QJsonObject DynamicModel::toJson() const {
    QJsonObject obj;
    obj["controle"] = controle;
    obj["maxEnginePower"] = maxEnginePower;
    obj["Lift"] = Lift;
    obj["zeroLiftSpeed"] = zeroLiftSpeed;
    obj["moveSpeed"] = moveSpeed;
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
    return obj;
}

void DynamicModel::fromJson(const QJsonObject& obj) {
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
}
