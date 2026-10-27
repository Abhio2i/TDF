#ifndef SEAMODEL_H
#define SEAMODEL_H

// Independent, Qt-free marine command-following dynamics.
// Local world axes: +X East, +Y Up, +Z North. Heading: 0 North, 90 East.
// All positions/velocities are metres, m/s, m/s^2; angles in degrees.
// Negative world Y is underwater when seaSurfaceY == 0.
// This is controlled kinematics (not full 6-DOF naval hydrodynamics).

#include <limits>

class SeaModel final {
public:
    struct Vec3 {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        constexpr Vec3() = default;
        constexpr Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
        Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
        Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
        Vec3 operator*(float a) const { return {x * a, y * a, z * a}; }
    };

    enum class VesselType { SurfaceShip, Submarine };
    enum class AltitudeReference { WorldY, SeaSurfaceRelative, DepthBelowSurface };

    struct TransformState {
        Vec3 position;                   // metres, Y positive upwards
        float heading = 0.0f;            // degrees clockwise from North
        float pitch = 0.0f;              // degrees, positive nose-up
        float roll = 0.0f;               // degrees
    };

    struct Limits {
        float maxSpeed = 15.0f;          // speed through water, m/s
        float maxAcceleration = 0.30f;   // m/s^2
        float maxDeceleration = 0.45f;   // m/s^2
        float maxJerk = 0.30f;           // m/s^3
        float maxTurnRate = 2.0f;        // deg/s
        float maxTurnAcceleration = 0.35f; // deg/s^2
        float minSteerageSpeed = 0.30f;  // below: zero rudder authority, m/s
        float fullSteerageSpeed = 3.0f;  // full yaw authority, m/s
        bool canTurnInPlace = false;     // true for thruster-equipped vessels

        float maxOperatingDepth = 300.0f; // submarine only; metres positive
        float maxRiseRate = 1.5f;       // submarine upwards m/s
        float maxDiveRate = 1.5f;       // submarine downwards m/s
        float maxVerticalAcceleration = 0.30f; // m/s^2
        float maxPitch = 12.0f;         // degrees
        float maxRoll = 8.0f;           // degrees
        float pitchResponseTime = 2.5f; // seconds
        float rollResponseTime = 3.0f;  // seconds
    };

    struct Responses {
        float speedResponseTime = 2.0f; // speed loop; seconds
        float headingResponseTime = 3.0f; // heading loop; seconds
        float depthResponseTime = 6.0f; // depth loop; seconds
    };

    struct Environment {
        float seaSurfaceY = 0.0f;       // world m (typically 0)
        bool hasSeafloor = false;        // false: only max depth enforced
        float seafloorY = -500.0f;      // world m, if known
        float seabedClearance = 10.0f;  // minimum clearance above seabed, m
        Vec3 waterCurrent;               // world flow m/s: +X east, +Z north
    };

    struct Commands {
        float speed = 0.0f;            // desired through-water speed m/s
        float heading = 0.0f;          // degrees clockwise from North
        float altitude = 0.0f;         // world m or relative, based on reference
        AltitudeReference altitudeReference = AltitudeReference::WorldY;
    };

    struct State {
        float speedThroughWater = 0.0f; // m/s
        float acceleration = 0.0f;      // longitudinal m/s^2
        float headingRate = 0.0f;       // deg/s, signed true heading derivative
        float verticalSpeed = 0.0f;     // water-relative upward m/s
        float pitchRate = 0.0f;         // deg/s
        float rollRate = 0.0f;          // deg/s
    };

    struct Telemetry {
        float altitude = 0.0f;         // world m; can be negative
        float depth = 0.0f;            // positive below sea surface m
        float heightAboveSeabed = std::numeric_limits<float>::quiet_NaN(); // m
        bool submerged = false;
        float heading = 0.0f;
        float pitch = 0.0f;
        float roll = 0.0f;
        float speedThroughWater = 0.0f; // m/s
        float speedOverGround = 0.0f;   // horizontal m/s
        float courseOverGround = 0.0f;  // deg, meaningful if SOG > 0
        float driftAngle = 0.0f;        // course - heading deg
        float verticalSpeed = 0.0f;     // actual world upwards m/s
        Vec3 waterRelativeVelocity;     // world m/s
        Vec3 groundVelocity;            // world m/s
        Vec3 bodyWaterVelocity;         // local right/up/forward m/s
        Vec3 bodyGroundVelocity;        // local right/up/forward m/s
        Vec3 acceleration;              // world m/s^2
        Vec3 bodyAcceleration;          // local right/up/forward m/s^2
    };

    VesselType vesselType = VesselType::SurfaceShip;
    TransformState transform;
    Limits limits;
    Responses responses;
    Environment environment;
    Commands commands;
    State state;
    Telemetry telemetry;

    void Initialize();
    void ResetRuntimeState();
    void SetNavigationCommand(float speedMps, float headingDegrees,
                              float altitudeMetres,
                              AltitudeReference reference = AltitudeReference::WorldY);
    void SetDepthCommand(float speedMps, float headingDegrees, float depthMetres);
    float CommandedWorldAltitude() const;
    void FixedUpdate(float deltaSeconds);
    bool IsInitialized() const { return initialized_; }

private:
    bool initialized_ = false;
    Vec3 lastGroundVelocity_{};
    float minAllowedAltitude() const;
    void applyDepthBounds();
    void step(float dt);
    void publishTelemetry(float dt, const Vec3& previousGroundVelocity,
                          float oldPitch, float oldRoll);
};

#endif // SEAMODEL_H
