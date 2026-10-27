// =============================================================================
// FILE:        dynamicmodel.h
// MODULE:      Dynamic Motion Simulation
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the DynamicModel class, which provides motion dynamics
//              for simulation entities including speed, altitude, turn rates,
//              and terrain interaction. Supports 6-DoF calculations without
//              rigidbody, trajectory following, and formation flying.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added 6-DoF simulation members and formation turn logic.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef DYNAMICMODEL_H
#define DYNAMICMODEL_H

#include <QObject>
#include <QJsonObject>
#include "./transform.h"
#include "./rigidbody.h"
#include "./trajectory.h"
#include <cmath>

// Forward declarations
class Platform;
class Formation;
class FormationPosition;

// =============================================================================
// CLASS: DynamicModel
//
// DESCRIPTION: Implements kinematic and dynamic behavior for movable entities.
//              Handles speed, altitude, turning, terrain passability, and
//              trajectory/formation following. Provides physics-based updates
//              for 6-Degree-of-Freedom motion.
// =============================================================================
class DynamicModel: public QObject, public Component
{
    Q_OBJECT
public:

    // -------------------------------------------------------------------------
    // ENUM: TerrainSurface
    // DESCRIPTION: Surface types affecting movement passability and speed.
    // -------------------------------------------------------------------------
    enum TerrainSurface {
        Generic,   // Default surface, no special modifiers
        Ground,    // Solid ground
        Sand,      // Soft sand – reduces speed
        Rock       // Rocky terrain – may impede movement
    };
    Q_ENUM(TerrainSurface)

    // Altitude command reference used by the trajectory -> DynamicModel -> Aircraft bridge.
    enum AltitudeReference {
        WorldY = 0,
        GroundRelative = 1
    };
    Q_ENUM(AltitudeReference)


    DynamicModel();
    ~DynamicModel();
    ComponentType Typo() const override { return ComponentType::DynamicModel; }
    QString dynamicType = "flight_MODEL";
    // void init();
    void start();

    // =========================================================================
    // SECTION: Flight & Movement Parameters
    // DESCRIPTION: Core motion characteristics – speeds, accelerations, rates.
    // =========================================================================
    bool control;                //!< Manual control flag
    bool follow;                //!< Follow mode enabled
    bool followTarget = false;  //!< Whether following a specific target
    float turnRadius = 10;      //!< Turn radius in metres

    // Maximum performance limits
    // External/project-facing units are intentionally preserved here.
    float minSpeed = 100.0f;        //!< Minimum commanded airborne speed (km/h)
    float moveSpeed = 800.0f;       //!< Legacy/default desired speed (km/h)
    float maxSpeed = 1800.0f;       //!< Maximum speed (km/h)
    float Acceleration = 100.000f;  //!< Maximum acceleration (m/s²)
    float Decceleration = 100.000f; //!< Maximum deceleration (m/s²)
    float turnRate = 10.620f;       //!< Maximum heading/yaw rate (deg/s)
    float Roll = 90.000f;           //!< Legacy roll field (deg)

    float minimumTurnSpeed = 18.0f;       //!< No airborne yaw authority below this speed (km/h)
    float fullTurnAuthoritySpeed = 90.0f; //!< Full airborne yaw authority at/above this speed (km/h)

    float maxPitchAngle = 30.0f;    //!< Maximum pitch angle (deg)
    float maxBankAngle = 45.0f;     //!< Maximum bank angle (deg)

    float maxAltitude = 60000.0f;   //!< Maximum operational altitude (ft)
    float Altitude = 10000.0f;      //!< Legacy/current commanded altitude (ft)
    float climbRate = 1000.0f;      //!< Maximum navigation climb rate (ft/min)
    float diveRate = 1000.0f;       //!< Maximum navigation dive rate (ft/min)

    // -------------------------------------------------------------------------
    // Navigation command state.
    // Simulation extracts these from the active Trajectory/Waypoint for each
    // entity. Aircraft receives SI-converted copies of these values.
    // -------------------------------------------------------------------------
    float commandedSpeed = 800.0f;       //!< Current trajectory speed command (km/h)
    float commandedHeading = 0.0f;       //!< Current trajectory heading command (deg, 0..360)
    float commandedAltitude = 10000.0f;  //!< Current trajectory altitude command (ft)
    AltitudeReference altitudeReference = AltitudeReference::WorldY;

    // Response dynamics (smoothing and lag)
    float deltaSpdCommandMaxAcc = 20.000f;   //!< Speed error for maximum acceleration (m/s)
    float timeToReachMaxAcc = 1.000f;        //!< Engine/acceleration response time (s)
    float deltaSpdCommandMaxDecel = 20.000f; //!< Speed error for maximum deceleration (m/s)
    float timeToReachMaxDecel = 1.000f;      //!< Deceleration response time (s)
    float deltaHdgCommandMaxRot = 90.000f;   //!< Heading error for maximum turn demand (deg)
    float timeToReachMaxRot = 1.000f;        //!< Turn-rate response time (s)
    float maximumPitchRate = 30.000f;        //!< Maximum pitch rate (deg/s)
    float maximumRollRate = 30.000f;         //!< Maximum roll rate (deg/s)
    float deltaToReachMaxROC = 500.00f;      //!< Altitude error for max rate of climb (m)
    float deltaToReachMaxROD = 500.00f;      //!< Altitude error for max rate of descent (m)

    // -------------------------------------------------------------------------
    // Flight physics. These parameters are persistent per entity.
    // -------------------------------------------------------------------------
    float minimumTakeoffSpeed = 90.0f;       //!< Reference takeoff speed (km/h)
    float stallSpeed = 72.0f;                //!< Reference stall speed (km/h)
    float criticalAngleOfAttack = 15.0f;     //!< Stall AoA threshold (deg)
    float gravity = 9.80665f;                //!< Gravity acceleration (m/s²)
    float maxSinkSpeed = 80.0f;              //!< Maximum gravity/stall sink rate (m/s)
    float stallRecoveryRate = 8.0f;          //!< Sink recovery rate (m/s²)
    float stalledLiftFactor = 0.25f;         //!< Remaining lift while AoA-stalled (0..1)
    float takeoffPitchThreshold = 2.0f;      //!< Pitch required for liftoff (deg)
    float groundLevel = 0.0f;                //!< Flat ground/runway world altitude (ft)

    bool autoStallRecovery = true;            //!< Enable automatic nose-down stall recovery
    float stallRecoveryPitch = -10.0f;        //!< Stall recovery pitch command (deg)

    // -------------------------------------------------------------------------
    // Aerodynamic drag approximation used by Aircraft.
    // -------------------------------------------------------------------------
    float zeroThrottleDrag = 2.0f;            //!< Reference-speed zero-throttle drag (m/s²)
    float highSpeedDrag = 5.0f;               //!< Additional high-speed drag (m/s²)
    float inducedDrag = 1.5f;                 //!< Lift/turn induced drag (m/s²)
    float dragReferenceSpeed = 50.0f;         //!< Drag reference speed (m/s)

    // Terrain passability
    TerrainSurface terrainSurface = TerrainSurface::Generic; //!< Current surface type
    float maximumSpeed = 100;           //!< Speed percentage allowed on current surface
    bool terrainIs = false;             //!< Is terrain passable?

    // =========================================================================
    // SECTION: Runtime State Variables
    // DESCRIPTION: Current dynamic state of the model.
    // =========================================================================
    float angdeg = 1;           //!< Angular step for calculations
    float startTime = 0;        //!< Simulation start time
    float endTime = -1;         //!< End time for motion (negative = infinite)
    float time = 0;             //!< Current simulation time
    float currentSpeed = 0;     //!< Current speed (km/h)
    float currentAltitude = 0;  //!< Current altitude (ft)

    // =========================================================================
    // SECTION: 6-DoF Motion Variables (without Rigidbody)
    // DESCRIPTION: Physics state for full six-degree-of-freedom simulation.
    // =========================================================================
    QVector3D velocity;         //!< Linear velocity vector
    float windAngle = 0;            //!< Wind direction angle
    QVector3D windDierction;    //!< Wind direction vector (note: typo in original)
    float windSpeed = 0;            //!< Wind speed magnitude
    QVector3D angularVelocity;  //!< Angular velocity (roll, pitch, yaw rates)
    float mass = 1.0f;          //!< Mass for physics calculations (kg)

    // Output parameters (calculated from motion)
    float pitch = 0;            //!< Pitch angle (deg)
    float roll = 0;             //!< Roll angle (deg)
    float yaw = 0;              //!< Yaw angle (deg)
    float Rollrate = 0;         //!< Roll rate (deg/s)
    float Pitchrate = 0;        //!< Pitch rate (deg/s)
    float Yawrate = 0;          //!< Yaw rate (deg/s)
    float DriftAngle = 0;       //!< Sideslip/drift angle (deg)
    float TrueHeading = 0;      //!< True heading (deg)
    float TrueAirSpeed = 0;     //!< True airspeed (km/h)
    float NorthVelocity = 0;    //!< North component of velocity (m/s)
    float EastVelocity = 0;     //!< East component of velocity (m/s)
    float VerticalVelocity = 0; //!< Vertical velocity (m/s)
    float GroundVelocity = 0;   //!< Ground speed (km/h)
    float currentAcceralation = 0; //!< Actual longitudinal acceleration (m/s²)
    QVector3D localVelocity;         //!< Legacy body-relative velocity (m/s)

    // Extended Aircraft telemetry.
    float currentHeightAboveGround = 0.0f; //!< Height above configured ground level (ft)
    float currentTurnRate = 0.0f;          //!< World heading rate (deg/s)
    float horizonPitchRate = 0.0f;         //!< Horizon pitch derivative (deg/s)
    float horizonRollRate = 0.0f;          //!< Horizon roll derivative (deg/s)
    bool horizonRatesValid = true;

    bool isGrounded = false;
    bool isStalling = false;
    float liftFactor = 0.0f;
    float angleOfAttack = 0.0f;            //!< deg
    float sideslipAngle = 0.0f;            //!< deg
    float flightPathAngle = 0.0f;          //!< deg

    QVector3D airVelocity;                 //!< World air-relative velocity (m/s)
    QVector3D bodyAirVelocity;             //!< Body air-relative velocity (m/s)
    QVector3D groundVelocity;              //!< World ground-relative velocity (m/s)
    QVector3D bodyGroundVelocity;          //!< Body ground-relative velocity (m/s)

    QVector3D acceleration;                //!< World acceleration (m/s²)
    QVector3D bodyAcceleration;            //!< Body acceleration (m/s²)
    // =========================================================================
    // SECTION: External Component References
    // DESCRIPTION: Pointers to associated simulation components.
    // =========================================================================
    Transform* transform;           //!< Position/orientation component
    Rigidbody* rigidbody;           //!< Physics rigidbody (if used)
    Trajectory* trajectory;         //!< Predefined path follower
    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

    // Formation flying support
    Platform *followEntity = nullptr;               //!< Entity to follow in formation
    Formation* formation = nullptr;
    FormationPosition *formationPosition = nullptr; //!< Formation offset data

    float lerp(float a, float b, float t);          //!< Linear interpolation helper

    double calculateTurnRadius(double speedKmph, double turnRateDegPerSec);

    void init(Entity* Entity) override;
    void Awake() override;
    void Start() override;

    // Component interface overrides
    void SyncByJson(const QJsonObject& obj);
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
    void SaveState();
    void RestoreState();

    void FollowTrajectory();        //!< Execute trajectory following
    void FollowFormation();
    void TimeJump(int sec);
    // Formation turn tracking state
    float lastLeaderHeading = 0.0f;      //!< Leader's previous heading
    float turnStartHeading = 0.0f;       //!< Wingman heading at turn start
    float turnTargetHeading = 0.0f;      //!< Desired heading for current turn
    bool inActiveTurn = false;           //!< Currently executing a turn?
    bool currentTurnIsTactical = false;  //!< Turn type (tactical or standard)
    float turnDetectionThreshold = 5.0f; //!< Heading change threshold to detect turn (deg)

public slots:
    void Update(float deltaTime);   //!< Called each frame to update dynamics
    void setMoveSpeed(float speed); //!< Sets desired movement speed

private:
    float speed = 0;        //!< Internal speed (km/s)
    float delta = 0;        //!< Last delta time
    double lastLat = 0;     //!< Previous latitude for distance calc
    double lastLon = 0;     //!< Previous longitude for distance calc
    float lastdist = 0;     //!< Last computed distance
    bool statesaved = false;
    QJsonObject InitalState;
};

#endif // DYNAMICMODEL_H
