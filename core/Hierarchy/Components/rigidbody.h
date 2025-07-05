
#ifndef RIGIDBODY_H
#define RIGIDBODY_H
#include "./component.h"
#include <QObject>
#include <core/Hierarchy/Struct/vector.h>
#include <QJsonObject>

class Rigidbody: public QObject, public Component
{
    Q_OBJECT
public:
    Rigidbody();
    ComponentType Typo() const override { return ComponentType::Rigidbody; }
    bool Active;
    bool Gravity;
    bool Kinematics;
    bool freezePositionX;
    bool freezePositionY;
    bool freezePositionZ;

    bool freezeRotationX;
    bool freezeRotationY;
    bool freezeRotationZ;

    float Mass;
    float Drag;
    float angularDrag;
    float deltaTime;

    Vector *velocity;
    Vector *angularVelocity;
    QJsonObject customParameters;

    void addForce(const Vector& force);
    void addTorque(const Vector& torque);
    void addVelocity(const Vector& v);

    Vector* setLinearVelocity(const Vector& v);
    Vector* setAngularVelocity(const Vector& v);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

signals:
    void setLinearVel(const Vector& velocity);
    void setAngularVel(const Vector& velocity);
};

#endif // RIGIDBODY_H
