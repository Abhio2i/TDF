


// #ifndef DYNAMICMODEL_H
// #define DYNAMICMODEL_H
// #include <QObject>
// #include <QJsonObject>
// #include "./transform.h"
// #include "./rigidbody.h"
// #include "./trajectory.h"
// #include <cmath>


// class Platform;
// class FormationPosition;
// class DynamicModel: public QObject, public Component
// {
//     Q_OBJECT
// public:
//     DynamicModel();
//     ComponentType Typo() const override { return ComponentType::DynamicModel; }
//     bool controle;
//     bool follow;
//     float maxEnginePower = 10;
//     float Lift = 0.002f;
//     float zeroLiftSpeed = 300;
//     float throttle = 0.0f;
//     float throttleInput = 0.0f;
//     bool airBrakes = false;

//     float dragIncreaseFactor = 0.001f;
//     float aerodynamicEffect = 1.0f;
//     float airBrakesEffect = 3.0f;
//     float rollEffect = 1.0f;
//     float pitchEffect = 1.0f;
//     float yawEffect = 0.2f;
//     float bankedTurnEffect = 0.5f;
//     float autoRollLevel = 0.2f;
//     float autoPitchLevel = 0.2f;

//     float altitude = 0.0f;
//     float forwardSpeed = 0.0f;
//     float enginePower = 0.0f;
//     float aeroFactor = 0.0f;
//     float moveSpeed = 800;//km/h
//     float turnRadius = 100;//metre
//     float rotationSpeed = 1;
//     float angdeg = 1;
//     float start = 0;
//     float time = 0;
//     float currentSpeed = 0;
//     // New member variables for 6-DoF simulation without Rigidbody
//     QVector3D velocity;
//     QVector3D angularVelocity;
//     float mass = 1.0f; // Add mass for realistic physics calculations

//     Transform* transform;
//     Rigidbody* rigidbody;
//     Trajectory* trajectory;
//     QJsonObject customParameters; // Added to store custom parameters

//     Platform* followEntity;
//     FormationPosition* formationPosition;
//     float lerp(float a, float b, float t);
//     QJsonObject toJson() const override;
//     void fromJson(const QJsonObject &obj) override;

//     void FollowTrajectory();

// public slots:
//     void Update(float deltaTime);
//     void setMoveSpeed(float speed);
// };

// #endif // DYNAMICMODEL_H



#ifndef DYNAMICMODEL_H

#define DYNAMICMODEL_H

#include <QObject>

#include <QJsonObject>

#include "./transform.h"

#include "./rigidbody.h"

#include "./trajectory.h"

#include <cmath>





class Platform;

class FormationPosition;

class DynamicModel: public QObject, public Component

{

    Q_OBJECT

public:

    DynamicModel();

    ComponentType Typo() const override { return ComponentType::DynamicModel; }



    bool isStopped = false;

    float delta = 0.0f;

    float speeed = 0.0f;

    double lastLat = 0.0;

    double lastLon = 0.0;

    float lastdist = 0.0f;

    bool isInitialized = false;

    // रोटेशन और स्पीड के लिए

    float angdeg = 0.0f; // बहुत ज़रूरी

    float currentSpeed = 0.0f;

    bool controle;

    bool follow;

    float maxEnginePower = 10;

    float Lift = 0.002f;

    float zeroLiftSpeed = 300;

    float throttle = 0.0f;

    float throttleInput = 0.0f;

    bool airBrakes = false;



    float dragIncreaseFactor = 0.001f;

    float aerodynamicEffect = 1.0f;

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

    float moveSpeed = 800;//km/h

    float turnRadius = 5000;//metre

    float rotationSpeed = 1;

    // float angdeg = 1;

    float start = 0;

    float time = 0;

    // float currentSpeed = 0;

    // New member variables for 6-DoF simulation without Rigidbody

    QVector3D velocity;

    QVector3D angularVelocity;

    float mass = 1.0f; // Add mass for realistic physics calculations



    Transform* transform;

    Rigidbody* rigidbody;

    Trajectory* trajectory;

    QJsonObject customParameters; // Added to store custom parameters



    Platform* followEntity;

    FormationPosition* formationPosition;

    float lerp(float a, float b, float t);

    QJsonObject toJson() const override;

    void fromJson(const QJsonObject &obj) override;



    void FollowTrajectory();



public slots:

    void Update(float deltaTime);

    void setMoveSpeed(float speed);

};



#endif // DYNAMICMODEL_H
