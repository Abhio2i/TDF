

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
    float turnRadius = 10;//metre
    ///Maximums
    float minSpeed = 100.0f;//km/h
    float moveSpeed = 800.0f;//km/h
    float maxSpeed = 1800.0f;//km/h
    float Acceleration = 100.000f;//m/s^2
    float Decceleration = 100.000f;//m/s^2
    float turnRate = 10.620f;//deg/s
    float Roll = 90.000;//deg
    float maxAltitude = 60000;//ft
    float Altitude = 10000;//ft
    float climbRate = 1000.000;//ft/s
    float diveRate = 1000.000;//ft/s

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
    float endTime = -1;
    float time = 0;
    float currentSpeed = 0;
    float currentAltitude = 0;


    // New member variables for 6-DoF simulation without Rigidbody
    QVector3D velocity;
    QVector3D windDierction;
    float windSpeed;
    QVector3D angularVelocity;
    float mass = 1.0f; // Add mass for realistic physics calculations

    ///out Parameter
    float pitch = 0;
    float roll = 0;
    float yaw = 0;
    float Rollrate = 0;
    float Pitchrate = 0;
    float Yawrate = 0;
    float DriftAngle = 0;
    float TrueHeading = 0;
    float TrueAirSpeed = 0;
    float NorthVelocity = 0;
    float EastVelocity = 0;
    float VerticalVelocity = 0;
    float GroundVelocity = 0;
    ////////////

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

    // Formation turn state tracking
    float lastLeaderHeading = 0.0f;      // Track leader's heading
    float turnStartHeading = 0.0f;        // Wingman's heading when turn detected
    float turnTargetHeading = 0.0f;       // Target heading for this turn
    bool inActiveTurn = false;            // Are we currently executing a turn?
    bool currentTurnIsTactical = false;   // Type of turn we committed to
    float turnDetectionThreshold = 5.0f;  // Degrees of change to detect new turn

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
