
#include "rigidbody.h"
#include <QDebug>

Rigidbody::Rigidbody() {
    Active = true;
    Gravity = true;
    Kinematics = false;
    freezePositionX = false;
    freezePositionY = false;
    freezePositionZ = true;
    freezeRotationX = true;
    freezeRotationY = true;
    freezeRotationZ = false;
    Mass = 1;
    Drag = 0.1;
    angularDrag = 0.1;
    deltaTime = 0.0f;
    velocity = new Vector();
    angularVelocity = new Vector();
    customParameters = QJsonObject(); // Initialize customParameters
}

void Rigidbody::addForce(const Vector& force) {
    if (!velocity || Mass <= 0.0f) return;
    Vector acceleration = force / Mass;
    *velocity += acceleration * deltaTime;
    *velocity *= (1.0f - Drag * deltaTime);
    emit setLinearVel(*velocity);
}

void Rigidbody::addTorque(const Vector& torque) {
    if (!angularVelocity || Mass <= 0.0f) return;
    Vector angularAcceleration = torque / Mass;
    *angularVelocity += angularAcceleration * deltaTime;
    *angularVelocity *= (1.0f - angularDrag * deltaTime);
    qDebug() << "Rigidbody::addTorque angularVelocity: x=" << angularVelocity->x << "y=" << angularVelocity->y << "z=" << angularVelocity->z;
    emit setAngularVel(*angularVelocity);
}

void Rigidbody::addVelocity(const Vector& v) {
    if (!velocity) return;
    *velocity += v;
    emit setLinearVel(*velocity);
}

Vector* Rigidbody::setLinearVelocity(const Vector& v) {
    *velocity = v;
    emit setLinearVel(*velocity);
    return velocity;
}

Vector* Rigidbody::setAngularVelocity(const Vector& v) {
    *angularVelocity = v;
    emit setAngularVel(*angularVelocity);
    return angularVelocity;
}

QJsonObject Rigidbody::toJson() const {
    QJsonObject obj;
    obj["active"] = Active;
    obj["gravity"] = Gravity;
    obj["kinematics"] = Kinematics;
    obj["freezePositionX"] = freezePositionX;
    obj["freezePositionY"] = freezePositionY;
    obj["freezePositionZ"] = freezePositionZ;
    obj["freezeRotationX"] = freezeRotationX;
    obj["freezeRotationY"] = freezeRotationY;
    obj["freezeRotationZ"] = freezeRotationZ;
    obj["mass"] = Mass;
    obj["drag"] = Drag;
    obj["angulardrag"] = angularDrag;
    obj["deltaTime"] = deltaTime;

    if (velocity)
        obj["velocity"] = velocity->toJson();
    if (angularVelocity)
        obj["angularVelocity"] = angularVelocity->toJson();

    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    qDebug() << "Rigidbody::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}

void Rigidbody::fromJson(const QJsonObject& obj) {
    qDebug() << "Rigidbody::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // Standard fields
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("gravity"))
        Gravity = obj["gravity"].toBool();
    if (obj.contains("kinematics"))
        Kinematics = obj["kinematics"].toBool();
    if (obj.contains("freezePositionX"))
        freezePositionX = obj["freezePositionX"].toBool();
    if (obj.contains("freezePositionY"))
        freezePositionY = obj["freezePositionY"].toBool();
    if (obj.contains("freezePositionZ"))
        freezePositionZ = obj["freezePositionZ"].toBool();
    if (obj.contains("freezeRotationX"))
        freezeRotationX = obj["freezeRotationX"].toBool();
    if (obj.contains("freezeRotationY"))
        freezeRotationY = obj["freezeRotationY"].toBool();
    if (obj.contains("freezeRotationZ"))
        freezeRotationZ = obj["freezeRotationZ"].toBool();
    if (obj.contains("mass"))
        Mass = obj["mass"].toVariant().toDouble();
    if (obj.contains("drag"))
        Drag = obj["drag"].toVariant().toDouble();
    if (obj.contains("angulardrag"))
        angularDrag = obj["angulardrag"].toVariant().toDouble();
    if (obj.contains("deltaTime"))
        deltaTime = obj["deltaTime"].toDouble();
    if (obj.contains("velocity") && obj["velocity"].isObject()) {
        if (!velocity) velocity = new Vector();
        velocity->fromJson(obj["velocity"].toObject());
    }
    if (obj.contains("angularVelocity") && obj["angularVelocity"].isObject()) {
        if (!angularVelocity) angularVelocity = new Vector();
        angularVelocity->fromJson(obj["angularVelocity"].toObject());
    }

    // Custom parameters
    QStringList standardKeys = {
        "active", "gravity", "kinematics",
        "freezePositionX", "freezePositionY", "freezePositionZ",
        "freezeRotationX", "freezeRotationY", "freezeRotationZ",
        "mass", "drag", "angulardrag", "deltaTime",
        "velocity", "angularVelocity"
    };
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!standardKeys.contains(it.key())) {
            customParameters[it.key()] = it.value();
        }
    }

    qDebug() << "Rigidbody::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
}
