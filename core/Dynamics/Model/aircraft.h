// =============================================================================
// FILE:        aircraft.h
// MODULE:      Aircraft Dynamics Simulation
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Aircraft class, a lightweight 6-DoF flight dynamics
//              model with vector mathematics, navigation, and telemetry.
//              Includes a nested vec3 struct with arithmetic operators,
//              physics helpers, and flight parameters (mass, acceleration,
//              turn rate, pitch, altitude, wind, etc.). Provides FixedUpdate
//              for time‑step integration.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <cmath>

// =============================================================================
// CLASS: Aircraft
//
// DESCRIPTION: Simulates fixed‑wing aircraft motion including acceleration,
//              turn rates, pitch/roll, altitude, and waypoint navigation.
//              Uses a simple Euler integration in FixedUpdate. Includes
//              telemetry output and environmental wind effects.
// =============================================================================
class Aircraft
{
public:
    Aircraft();
    ~Aircraft();

    // =========================================================================
    // STRUCT: vec3
    // DESCRIPTION: 3D vector with arithmetic operators and physics helpers.
    // =========================================================================
    struct vec3 {
        float x, y, z;

        // Constructor
        vec3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}

        // --- Basic Vector Operations ---

        // Addition: vector + vector
        vec3 operator+(const vec3& v) const {
            return vec3(x + v.x, y + v.y, z + v.z);
        }

        // Subtraction: vector - vector
        vec3 operator-(const vec3& v) const {
            return vec3(x - v.x, y - v.y, z - v.z);
        }

        // Multiplication: vector * scalar (e.g., velocity = direction * speed)
        vec3 operator*(float scalar) const {
            return vec3(x * scalar, y * scalar, z * scalar);
        }

        // Multiplication: vector * vector (Hadamard product - component-wise)
        vec3 operator*(const vec3& v) const {
            return vec3(x * v.x, y * v.y, z * v.z);
        }

        // Division: vector / scalar
        vec3 operator/(float scalar) const {
            if (std::abs(scalar) > 0.00001f) {
                float inv = 1.0f / scalar;
                return vec3(x * inv, y * inv, z * inv);
            }
            return vec3(0, 0, 0);
        }

        // --- Shorthand Operations (+=, -=, *=) ---

        vec3& operator+=(const vec3& v) {
            x += v.x; y += v.y; z += v.z;
            return *this;
        }

        vec3& operator*=(float scalar) {
            x *= scalar; y *= scalar; z *= scalar;
            return *this;
        }

        // --- Physics Helpers ---

        float Magnitude() const {
            return std::sqrt(x * x + y * y + z * z);
        }

        void Normalize() {
            float m = Magnitude();
            if (m > 0.00001f) {
                *this *= (1.0f / m);
            }
        }

        static float Distance(const vec3& a, const vec3& b) {
            vec3 diff = a - b;
            return diff.Magnitude();
        }
    };

    // =========================================================================
    // SECTION: Flight Settings
    // DESCRIPTION: Physical and control parameters for the aircraft.
    // =========================================================================

    //[Header("Flight Settings")]
    //[Tooltip("Mass of the aircraft in kilograms.")]
    float Mass = 1000;                      //!< Mass (kg)

    //[Tooltip("Rate of speed increase (m/s^2).")]
    float MaxAcceleration = 10;             //!< Maximum acceleration (m/s²)

    //[Tooltip("Rate of speed decrease (m/s^2).")]
    float MaxDecceleration = 10;            //!< Maximum deceleration (m/s²)

    //[Tooltip("Maximum altitude the aircraft can reach.")]
    float CeilingHeight = 1000;             //!< Service ceiling (m)

    //[Tooltip("Desired forward airspeed.")]
    float TargetSpeed = 100;                //!< Target speed (m/s)

    //[Tooltip("Maximum degrees per second the aircraft can turn (Yaw).")]
    float turnRate = 30;                    //!< Turn rate (deg/s)

    //[Tooltip("Target vertical speed when climbing.")]
    float ClimbRate = 100;                  //!< Climb rate (m/s)

    //[Tooltip("Target vertical speed when descending.")]
    float DiveRate = 100;                   //!< Dive rate (m/s)

    //[Tooltip("Maximum allowed nose tilt (X-axis).")]
    float MaxPitch = 45;                    //!< Maximum pitch angle (deg)

    //[Tooltip("Speed of pitch rotation adjustments.")]
    float PitchRate = 10.0;                 //!< Pitch rate (deg/s)

    //[Tooltip("The 3D coordinate the aircraft is navigating towards.")]
    vec3 TargetPosition;                    //!< Destination waypoint

    // =========================================================================
    // SECTION: Environment
    // DESCRIPTION: Wind influence on flight.
    // =========================================================================
    //[Header("Environment")]
    float WindDirection = 0;                //!< Wind direction (degrees)
    float WindIntensity = 0;                //!< Wind speed (m/s)

    // =========================================================================
    // SECTION: Output / Telemetry
    // DESCRIPTION: Current state variables and control outputs.
    // =========================================================================
    //[Header("Output/Telemetry")]
    float RequirePitch = 0;     //!< Calculated target pitch for navigation
    float Pitch = 0;            //!< Applied pitch rotation (deg)
    float Roll = 0;             //!< Visual bank angle based on turn (deg)
    float Yaw = 0;
    float Altitude = 0;         //!< Current altitude (m, Y-coordinate)
    float currentClimbRate = 0; //!< Vertical velocity component (m/s)
    float currentPitchRate = 0; //!< Current pitch rate (deg/s)
    float Rollrate = 0;         //!< Roll rate (deg/s)
    float yawRate = 0;          //!< Current yaw rate (deg/s)
    float speed = 0;            //!< Current speed (m/s)
    float Thrust = 0;           //!< Calculated force based on acceleration (N)

    float DriftAngle = 0;       //!< Sideslip/drift angle (deg)
    float TrueHeading = 0;      //!< True heading (deg)
    float TrueAirSpeed = 0;     //!< True airspeed (m/s)
    float NorthVelocity = 0;    //!< North component of velocity (m/s)
    float EastVelocity = 0;     //!< East component of velocity (m/s)
    float VerticalVelocity = 0; //!< Vertical velocity (m/s)
    float GroundVelocity = 0;   //!< Ground speed (m/s)


    vec3 Velocity;              //!< World space velocity (m/s)
    vec3 localVelocity;         //!< Velocity relative to aircraft orientation

    bool waypointReach = false; //!< Flag indicating waypoint reached

    // The following block is legacy/commented parameters
    //     ///Maximums
    //     float minSpeed = 100.0f;//m/s
    //     float maxSpeed = 1800.0f;//m/s
    //     float Acceleration = 100.000f;//m/s^2
    //     float Decceleration = 100.000f;//m/s^2
    //     float turnRate = 10.620f;//deg/s
    //     float Roll = 90.000;//deg
    //     float maxAltitude = 60000;//m
    //     float climbRate = 1000.000;//m/s
    //     float diveRate = 1000.000;//m/s

    //     ///Target
    //     vec3 TargetPosition;
    //     float TargetSpeed = 800.0f;//m/s
    //     float TargetAltitude = 300.0f;//m/s

    // =========================================================================
    // SECTION: Current State
    // DESCRIPTION: Real‑time position and orientation.
    // =========================================================================
    ///Current
    vec3 Position;              //!< World position (m)
    vec3 LastPosition;          //!< Previous frame position (m)
    vec3 EularAngles;           //!< Euler angles (deg)
    vec3 LocalEularAngles;      //!< Local Euler angles (deg)
    //     float Speed = 0.0f;//m/s
    //     float Altitude = 0;//m
    //     float Heading = 0;//deg

    vec3 Forward;               //!< Forward direction vector (unit)

    //     // Transform* transform;
    //     // Trajectory* trajectory;

    void FixedUpdate(float delta);  //!< Updates physics and navigation each fixed timestep

private:
    vec3 _moveVelocity = vec3{0,0,0};   //!< Internal velocity accumulator
    float _currentHeading = 0;          //!< Current heading (deg)
    float _smoothTurnRate = 0;          //!< Smoothed turn rate for gradual response
    float _smoothPitchRate = 0;         //!< Smoothed pitch rate
    float _lastFrameSpeed = 0;          //!< Speed from previous frame

    const double Rad2Deg = 180.0 / M_PI; //!< Radians to degrees
    const double Deg2Rad = M_PI / 180.0; //!< Degrees to radians

    float NormalizeAngle(float angle);          //!< Normalises angle to [-180,180]
    double convertToClockwise360(double lon180);//!< Converts -180..180 to 0..360
    float Lerp(float a, float b, float t);      //!< Linear interpolation
};

#endif // AIRCRAFT_H
