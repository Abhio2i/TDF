
#include "dynamicmodel.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/Struct/formationposition.h"
#include "qjsondocument.h"
#include <core/InputSystem/inputmanager.h>
#include <QDebug>
#include <cmath>

DynamicModel::DynamicModel() {
    controle = true;
    follow = false;
    customParameters = QJsonObject(); // Initialize customParameters
}

void DynamicModel::Update(float deltaTime) {
    if (!controle || !transform || !rigidbody || !trajectory) return;

    // Direction vectors
    Vector forwardDir = transform->forward(); // x-axis (forward)
    Vector upDir = transform->up();           // z-axis (up)
    Vector rightDir = transform->right();     // y-axis (right)
    FollowTrajectory();
}

void DynamicModel::FollowTrajectory() {

    if (follow) {
        Vector current = *transform->position;

        // 🎯 Step 1: Base target is followEntity's position
        Vector target = *followEntity->transform->position;

        // 🎯 Step 2: Apply Offset if formationPosition is available
        if (formationPosition && formationPosition->Offset) {
            target += *formationPosition->Offset;
        }

        // 🔁 Step 3: Move towards target
        Vector diff = target - current;
        float distance = diff.magnitude();

        if (distance > 0.001f) {
            Vector dir = diff.normalized();
            current += dir * moveSpeed * 0.1f;
        }

        *transform->position = current;

        // 🔄 Step 4: Update rotation
        Vector direction = target - current;
        if (direction.magnitude() > 0.001f) {
            direction = direction.normalized();
            float angleRad = atan2(direction.y, direction.x);
            float angleDeg = angleRad * (180.0f / M_PI);
            *transform->rotation = Vector(0, 0, angleDeg);
        }

        return; // Skip trajectory logic
    }
    if(trajectory->Trajectories.size()<2) return;
    Vector current = *transform->position;
    Vector target = *trajectory->Trajectories[trajectory->current]->position;

    Vector diff = target - current;
    float distance = diff.magnitude();

    if (distance > 0.001f) {
        Vector dir = diff.normalized();
        current += dir * moveSpeed * 0.1f;
        Vector direction = target - current;

        direction = direction.normalized();

        float angleRad = atan2(direction.y, direction.x);
        float angleDeg = angleRad * (180.0f / M_PI);

        *transform->rotation = Vector(0, 0, -angleDeg);
    }
    *transform->position = current;
    //*transform->position = Vector::Lerp(*transform->position, *trajectory->Trajectories[trajectory->current]->position, moveSpeed * 0.1);

    if (trajectory->Trajectories.size() > trajectory->current && Vector::Distance(*transform->position, *trajectory->Trajectories[trajectory->current]->position) < 1) {
        trajectory->current += 1;
        trajectory->current = trajectory->current >= trajectory->Trajectories.size() ? (trajectory->Trajectories.size()-1) : trajectory->current;

    }
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
     obj["type"] = "component";

    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    qDebug() << "DynamicModel::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}

void DynamicModel::fromJson(const QJsonObject& obj) {
    qDebug() << "DynamicModel::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

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

    qDebug() << "DynamicModel::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
}

void DynamicModel::setMoveSpeed(float speed) {
    moveSpeed = qBound(1.0f, speed, 10.0f);
}
