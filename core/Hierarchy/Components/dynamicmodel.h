


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
    enum TerrainSurface {
        Generic,
        Ground,
        Sand,
        Rock
    };
    Q_ENUM(TerrainSurface)

    DynamicModel();
    ComponentType Typo() const override { return ComponentType::DynamicModel; }
    void init();
    void start();
    bool control;
    bool follow;
    float turnRadius = 500;//metre
    ///Maximums
    float minSpeed = 1800.0f;//km/h
    float moveSpeed = 800.0f;//km/h
    float maxSpeed = 100.0f;//km/h
    float Acceleration = 100.000f;//m/s^2
    float Decceleration = 100.000f;//m/s^2
    float turnRate = 71.620f;//deg/s
    float Roll = 90.000;//deg
    float Altitude = 10000;//ft
    float climbRate = 100.000;//ft/s
    float diveRate = 100.000;//ft/s

    ///Resposes
    float deltaSpdCommandMaxAcc = 20.000;//m/s
    float timeToReachMaxAcc = 1.000;//s
    float deltaSpdCommandMaxDecel = 20.000;//m/s
    float timeToReachMaxDecel = 1.000;//s
    float deltaHdgCommandMaxRot = 90.000;//deg
    float timeToReachMaxRot = 1.000;//s
    float maximumPitchRate = 30.000;//deg/s
    float maximumRollRate = 30.000;//deg/s
    float deltaToReachMaxROC = 500.00;//m
    float deltaToReachMaxROD = 500.00;//m

    ///Passabillity
    TerrainSurface terrainSurface = TerrainSurface::Generic;
    float maximumSpeed = 100;//%
    bool terrainIs = false;//Passable

    float angdeg = 1;
    float startTime = 0;
    float time = 0;
    float currentSpeed = 0;
    float currentAltitude = 0;


    // New member variables for 6-DoF simulation without Rigidbody
    QVector3D velocity;
    QVector3D angularVelocity;
    float mass = 1.0f; // Add mass for realistic physics calculations

    Transform* transform;
    Rigidbody* rigidbody;
    Trajectory* trajectory;
    QJsonObject customParameters; // Added to store custom parameters

    Platform *followEntity = nullptr;          // ADD '= nullptr'
    FormationPosition *formationPosition = nullptr; // ADD '= nullptr'
    float lerp(float a, float b, float t);

    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QString data3 = "") override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    void FollowTrajectory();

public slots:
    void Update(float deltaTime);
    void setMoveSpeed(float speed);
private:
    float speed = 0;//km/s
    float delta = 0;
    double lastLat = 0;
    double lastLon = 0;
    float lastdist = 0;
};

#endif // DYNAMICMODEL_H
