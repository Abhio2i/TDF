#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <cmath>

/*
 * Aircraft
 * --------
 * Pure command-based aircraft dynamics solver.
 *
 * IMPORTANT UNIT CONTRACT
 * -----------------------
 * Aircraft internally uses SI units only:
 *
 *   Position / Altitude      : metres
 *   Speed / Velocity         : m/s
 *   Acceleration             : m/s^2
 *   Climb / Dive / Sink rate : m/s
 *   Angles                   : degrees
 *   Angular rates            : deg/s
 *   Wind velocity            : m/s
 *
 * The project Transform uses kilometres. Conversion between Transform and
 * Aircraft MUST be performed by Simulation::updateDynamics().
 *
 * Aircraft intentionally has NO dependency on:
 *   - Qt
 *   - Transform
 *   - DynamicModel
 *   - Trajectory
 *   - Waypoint
 *   - Entity / Platform
 *
 * Trajectory/Waypoint logic must extract target speed/heading/altitude and
 * feed Aircraft::commands. Aircraft only follows those commands.
 */
class Aircraft
{
public:
    struct vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        constexpr vec3() = default;
        constexpr vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

        constexpr vec3 operator+(const vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
        constexpr vec3 operator-(const vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
        constexpr vec3 operator-() const { return {-x, -y, -z}; }
        constexpr vec3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }

        vec3 operator/(float scalar) const
        {
            if (std::abs(scalar) <= 0.000001f)
                return {};
            const float inv = 1.0f / scalar;
            return {x * inv, y * inv, z * inv};
        }

        vec3& operator+=(const vec3& v)
        {
            x += v.x; y += v.y; z += v.z;
            return *this;
        }

        vec3& operator-=(const vec3& v)
        {
            x -= v.x; y -= v.y; z -= v.z;
            return *this;
        }

        vec3& operator*=(float scalar)
        {
            x *= scalar; y *= scalar; z *= scalar;
            return *this;
        }

        float SqrMagnitude() const { return x * x + y * y + z * z; }
        float Magnitude() const { return std::sqrt(SqrMagnitude()); }

        vec3 Normalized() const
        {
            const float m = Magnitude();
            return (m > 0.000001f) ? (*this / m) : vec3{};
        }

        static constexpr float Dot(const vec3& a, const vec3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        static vec3 Cross(const vec3& a, const vec3& b)
        {
            return {
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            };
        }

        static float Distance(const vec3& a, const vec3& b)
        {
            return (a - b).Magnitude();
        }
    };

    friend constexpr vec3 operator*(float scalar, const vec3& v)
    {
        return v * scalar;
    }

    enum class AltitudeReference
    {
        WorldY = 0,
        GroundRelative = 1
    };

    struct MaximumsData
    {
        float minSpeed = 0.0f;                  // m/s
        float maxSpeed = 300.0f / 3.6f;         // m/s
        float maxAcceleration = 5.0f;           // m/s^2
        float maxDeceleration = 5.0f;           // m/s^2
        float turnRate = 20.0f;                 // deg/s
        float minimumTurnSpeed = 5.0f;          // m/s
        float fullTurnAuthoritySpeed = 25.0f;   // m/s
        float maxPitchAngle = 30.0f;            // deg
        float maxBankAngle = 45.0f;             // deg
        float maxAltitude = 10000.0f;           // m
        float climbRate = 10.0f;                // m/s
        float diveRate = 15.0f;                 // m/s
    };

    struct FlightPhysicsData
    {
        float minimumTakeoffSpeed = 25.0f;      // m/s
        float stallSpeed = 20.0f;               // m/s
        float criticalAngleOfAttack = 15.0f;    // deg
        float gravity = 9.80665f;               // m/s^2
        float maxSinkSpeed = 80.0f;             // m/s
        float stallRecoveryRate = 8.0f;         // m/s^2
        float stalledLiftFactor = 0.25f;        // 0..1
        float takeoffPitchThreshold = 2.0f;     // deg
        float groundLevel = 0.0f;               // m
    };

    struct StallRecoveryData
    {
        bool autoStallRecovery = true;
        float stallRecoveryPitch = -10.0f;      // deg
    };

    struct ResponseData
    {
        float deltaSpdCommandMaxAcc = 10.0f;    // m/s
        float timeToReachMaxAcc = 2.0f;         // s
        float deltaSpdCommandMaxDecel = 10.0f;  // m/s
        float timeToReachMaxDecel = 2.0f;       // s
        float deltaHdgCommandMaxRot = 45.0f;    // deg
        float timeToReachMaxRot = 2.0f;         // s
        float maximumPitchRate = 10.0f;         // deg/s
        float maximumRollRate = 30.0f;          // deg/s
        float deltaToReachMaxROC = 100.0f;      // m
        float deltaToReachMaxROD = 100.0f;      // m
    };

    struct CommandData
    {
        float commandedSpeed = 0.0f;            // m/s
        float commandedHeading = 0.0f;          // deg, 0..360
        float commandedAltitude = 0.0f;         // m
        AltitudeReference altitudeReference = AltitudeReference::WorldY;
    };

    struct AerodynamicDragData
    {
        float zeroThrottleDrag = 2.0f;          // m/s^2
        float highSpeedDrag = 5.0f;             // m/s^2
        float inducedDrag = 1.5f;               // m/s^2
        float referenceSpeed = 50.0f;           // m/s
    };

    struct WindData
    {
        vec3 windVelocity;                       // world-space m/s
    };

    struct TransformState
    {
        vec3 position;                           // m
        float pitch = 0.0f;                     // deg
        float heading = 0.0f;                   // deg
        float roll = 0.0f;                      // deg
    };

    struct AircraftStateData
    {
        float currentSpeed = 0.0f;              // m/s
        float currentAcceleration = 0.0f;       // m/s^2
        float currentHeading = 0.0f;            // deg
        float currentTurnRate = 0.0f;           // deg/s
        float currentAltitude = 0.0f;           // m
        float currentPitch = 0.0f;              // deg
        float currentRoll = 0.0f;               // deg
        float currentYaw = 0.0f;                // deg
        float currentPitchRate = 0.0f;          // deg/s
        float currentRollRate = 0.0f;           // deg/s
        float currentYawRate = 0.0f;            // deg/s
        float currentHeightAboveGround = 0.0f;  // m
        float horizonPitchRate = 0.0f;          // deg/s
        float horizonRollRate = 0.0f;           // deg/s
        bool horizonRatesValid = true;
    };

    struct FlightStateData
    {
        bool isGrounded = false;
        bool isStalling = false;
        float liftFactor = 1.0f;                 // 0..1
        float verticalSpeed = 0.0f;             // m/s
        float angleOfAttack = 0.0f;             // deg
        float sideslipAngle = 0.0f;             // deg
        float flightPathAngle = 0.0f;            // deg
    };

    struct VelocityStateData
    {
        vec3 airVelocity;                        // world m/s
        vec3 bodyAirVelocity;                    // body m/s
        vec3 groundVelocity;                     // world m/s
        vec3 bodyGroundVelocity;                 // body m/s
    };

    struct SpeedStateData
    {
        float trueAirSpeed = 0.0f;               // m/s
        float groundSpeed = 0.0f;                // m/s
    };

    struct AccelerationStateData
    {
        vec3 acceleration;                       // world m/s^2
        vec3 bodyAcceleration;                   // body m/s^2
    };

    MaximumsData maximums;
    FlightPhysicsData flightPhysics;
    StallRecoveryData stallRecovery;
    ResponseData responses;
    CommandData commands;
    AerodynamicDragData aerodynamicDrag;
    WindData wind;

    TransformState transform;

    AircraftStateData aircraftState;
    FlightStateData flightState;
    VelocityStateData velocityState;
    SpeedStateData speedState;
    AccelerationStateData accelerationState;

    Aircraft();
    ~Aircraft() = default;

    void Initialize();
    void FixedUpdate(float dt);
    void ResetRuntimeState();

    void SetNavigationCommand(float speedMS,
                              float headingDeg,
                              float altitudeM,
                              AltitudeReference reference = AltitudeReference::WorldY);

    float CommandedWorldAltitude() const;

private:
    static constexpr float SpeedEpsilon = 0.001f;
    static constexpr float GroundEpsilon = 0.001f;
    static constexpr float Deg2Rad = 0.017453292519943295769f;
    static constexpr float Rad2Deg = 57.295779513082320876f;

    bool initialized_ = false;
    float sinkSpeed_ = 0.0f;
    float engineCommand_ = 0.0f;
    float commandYawRate_ = 0.0f;

    vec3 previousGroundVelocity_;

    float previousPitch_ = 0.0f;
    float previousRoll_ = 0.0f;
    float previousHeading_ = 0.0f;

    void SynchronizeStateFromTransform();
    void UpdatePropulsionAndSpeed(float dt);
    void UpdateCommandControl(float dt);
    float NavigationPitchTarget() const;
    void RebuildVelocity();
    void UpdateVelocityFrames();
    void RefreshAerodynamicState();
    void UpdateFlightPhysics(float dt);
    void ApplyCeilingToVelocity(float dt);
    void ResolveGroundContact();
    void PublishMotionTelemetry(float dt);
    void PublishAngularTelemetry(float oldPitch,
                                 float oldRoll,
                                 float oldHeading,
                                 float dt);

    float PitchRollAuthority() const;
    float AirYawAuthority() const;
    float ForwardAirSpeed() const;
    float NaturalDragAcceleration() const;

    vec3 Forward() const;
    vec3 Right() const;
    vec3 Up() const;
    vec3 HorizontalForward() const;
    vec3 WorldToBody(const vec3& worldVector) const;

    static bool IsFinite(float value);
    static float Finite(float value, float fallback = 0.0f);
    static float NonNegative(float value);
    static float Clamp(float value, float minimum, float maximum);
    static float Clamp01(float value);
    static float Lerp(float a, float b, float t);
    static float MoveTowards(float current, float target, float maxDelta);
    static float MoveTowardsAngle(float current, float target, float maxDelta);
    static float Normalize360(float angle);
    static float Normalize180(float angle);
    static float DeltaAngle(float current, float target);
};

#endif // AIRCRAFT_H
