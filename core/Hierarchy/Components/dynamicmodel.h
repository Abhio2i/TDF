


#ifndef DYNAMICMODEL_H
#define DYNAMICMODEL_H
#include <QObject>
#include <QJsonObject>
#include "./transform.h"
#include "./rigidbody.h"
#include "./trajectory.h"

class DynamicModel: public QObject, public Component
{
    Q_OBJECT
public:
    DynamicModel();
    ComponentType Typo() const override { return ComponentType::DynamicModel; }
    bool controle;
    float maxEnginePower = 40;
    float Lift = 0.002f;
    float zeroLiftSpeed = 300;
    float throttle = 0.0f;
    float throttleInput = 0.0f;
    bool airBrakes = false;

    float dragIncreaseFactor = 0.001f;
    float aerodynamicEffect = 0.02f;
    float airBrakesEffect = 3.0f;
    float rollEffect = 1.0f;
    float pitchEffect = 1.0f;
    float yawEffect = 0.2f;
    float bankedTurnEffect = 0.5f;
    float autoRollLevel = 0.2f;
    float autoPitchLevel = 0.2f;

    float altitude = 0.0f;
    float forwardSpeed = 0.0f;
    float enginePower = 0.0f;
    float aeroFactor = 0.0f;
    float moveSpeed = 0.01;
    float rotationSpeed = 1;
    Transform* transform;
    Rigidbody* rigidbody;
    Trajectory* trajectory;
    QJsonObject customParameters; // Added to store custom parameters

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    void FollowTrajectory();

public slots:
    void Update(float deltaTime);
    void setMoveSpeed(float speed);
};

#endif // DYNAMICMODEL_H
