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

    DynamicModel();
    ~DynamicModel();
    ComponentType Typo() const override { return ComponentType::DynamicModel; }
    void init();
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
    float minSpeed = 100.0f;        //!< Minimum speed (km/h)
    float moveSpeed = 800.0f;       //!< Current desired speed (km/h)
    float maxSpeed = 1800.0f;       //!< Maximum speed (km/h)
    float Acceleration = 100.000f;  //!< Acceleration rate (m/s²)
    float Decceleration = 100.000f; //!< Deceleration rate (m/s²)
    float turnRate = 10.620f;       //!< Turn rate (deg/s)
    float Roll = 90.000;            //!< Roll angle (deg)
    float maxAltitude = 60000;      //!< Maximum operational altitude (ft)
    float Altitude = 10000;         //!< Current altitude (ft)
    float climbRate = 1000.000;     //!< Climb rate (ft/s)
    float diveRate = 1000.000;      //!< Dive rate (ft/s)

    // Response dynamics (smoothing and lag)
    float deltaSpdCommandMaxAcc = 20.000;   //!< Max speed change for acceleration (m/s)
    float timeToReachMaxAcc = 1.000;        //!< Time to max acceleration (s)
    float deltaSpdCommandMaxDecel = 20.000; //!< Max speed change for deceleration (m/s)
    float timeToReachMaxDecel = 1.000;      //!< Time to max deceleration (s)
    float deltaHdgCommandMaxRot = 90.000;   //!< Max heading change per command (deg)
    float timeToReachMaxRot = 1.000;        //!< Time to max rotation (s)
    float maximumPitchRate = 30.000;        //!< Max pitch rate (deg/s)
    float maximumRollRate = 30.000;         //!< Max roll rate (deg/s)
    float deltaToReachMaxROC = 500.00;      //!< Delta altitude to reach max rate of climb (m)
    float deltaToReachMaxROD = 500.00;      //!< Delta altitude to reach max rate of descent (m)

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
    float windAngle;            //!< Wind direction angle
    QVector3D windDierction;    //!< Wind direction vector (note: typo in original)
    float windSpeed;            //!< Wind speed magnitude
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
    QVector3D localVelocity;
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

    // Component interface overrides
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    void FollowTrajectory();        //!< Execute trajectory following

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
};

#endif // DYNAMICMODEL_H
