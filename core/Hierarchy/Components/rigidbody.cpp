
#include "rigidbody.h"
#include "qjsondocument.h"
#include <QDebug>

Rigidbody::Rigidbody():Component(nullptr) {
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

void Rigidbody::addSubComponent(std::string name, QString data1, QString data2, QString data3){

}

void Rigidbody::removeSubComponent(std::string ID){

}

void Rigidbody::updateSubComponent(std::string ID, const QJsonObject& obj){

}

QJsonObject Rigidbody::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

QJsonObject Rigidbody::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    obj["gravity"] = Gravity;
    obj["kinematics"] = Kinematics;
    obj["mass"] = Mass;
    obj["drag"] = Drag;
    obj["angulardrag"] = angularDrag;
    obj["deltaTime"] = deltaTime;
    obj["type"] = "component";

    QJsonObject freezeObj;
    freezeObj["type"] = "Section";
    freezeObj["freezePositionX"] = freezePositionX;
    freezeObj["freezePositionY"] = freezePositionY;
    freezeObj["freezePositionZ"] = freezePositionZ;
    freezeObj["freezeRotationX"] = freezeRotationX;
    freezeObj["freezeRotationY"] = freezeRotationY;
    freezeObj["freezeRotationZ"] = freezeRotationZ;
    obj["freeze"] = freezeObj;

    if (velocity)
        obj["velocity"] = velocity->toJson();
    if (angularVelocity)
        obj["angularVelocity"] = angularVelocity->toJson();

    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    //qDebug() << "Rigidbody::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}

void Rigidbody::fromJson(const QJsonObject& obj) {
    //qDebug() << "Rigidbody::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // Standard fields
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("gravity"))
        Gravity = obj["gravity"].toBool();
    if (obj.contains("kinematics"))
        Kinematics = obj["kinematics"].toBool();

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

    if (obj.contains("freeze") && obj["freeze"].isObject()) {
        QJsonObject freezeObj = obj["freeze"].toObject();

        if (freezeObj.contains("freezePositionX"))
            freezePositionX = freezeObj["freezePositionX"].toBool();
        if (freezeObj.contains("freezePositionY"))
            freezePositionY = freezeObj["freezePositionY"].toBool();
        if (freezeObj.contains("freezePositionZ"))
            freezePositionZ = freezeObj["freezePositionZ"].toBool();
        if (freezeObj.contains("freezeRotationX"))
            freezeRotationX = freezeObj["freezeRotationX"].toBool();
        if (freezeObj.contains("freezeRotationY"))
            freezeRotationY = freezeObj["freezeRotationY"].toBool();
        if (freezeObj.contains("freezeRotationZ"))
            freezeRotationZ = freezeObj["freezeRotationZ"].toBool();
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

    //qDebug() << "Rigidbody::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
}
