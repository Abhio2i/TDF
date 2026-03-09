#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <cmath>
class Aircraft
{
public:
    Aircraft();
    ~Aircraft();

    struct vec3 {
        float x, y, z;

        // 1. Constructor
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

        // Multiplication: vector * scalar (जैसे velocity = direction * speed)
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
                *this *= (1.0f / m); // अपने ही *= ऑपरेटर का इस्तेमाल किया
            }
        }

        static float Distance(const vec3& a, const vec3& b) {
            vec3 diff = a - b;
            return diff.Magnitude();
        }
    };
    //[Header("Flight Settings")]
    //[Tooltip("Mass of the aircraft in kilograms.")]
    float Mass = 1000;
    //[Tooltip("Rate of speed increase (m/s^2).")]
    float MaxAcceleration = 10;
    //[Tooltip("Rate of speed decrease (m/s^2).")]
    float MaxDecceleration = 10;
    //[Tooltip("Maximum altitude the aircraft can reach.")]
    float CeilingHeight = 1000;
    //[Tooltip("Desired forward airspeed.")]
    float TargetSpeed = 100;
    //[Tooltip("Maximum degrees per second the aircraft can turn (Yaw).")]
    float turnRate = 30;
    //[Tooltip("Target vertical speed when climbing.")]
    float ClimbRate = 100;
    //[Tooltip("Target vertical speed when descending.")]
    float DiveRate = 100;
    //[Tooltip("Maximum allowed nose tilt (X-axis).")]
    float MaxPitch = 45;
    //[Tooltip("Speed of pitch rotation adjustments.")]
    float PitchRate = 10.0;
    //[Tooltip("The 3D coordinate the aircraft is navigating towards.")]
    vec3 TargetPosition;

    //[Header("Environment")]
    float WindDirection = 0;
    float WindIntensity = 0;

    //[Header("Output/Telemetry")]
    float RequirePitch = 0;    // Calculated target pitch for navigation
    float Pitch = 0;           // Applied pitch rotation
    float Roll = 0;            // Visual bank angle based on turn
    float Altitude = 0;        // Current Y-coordinate
    float currentClimbRate = 0; // Vertical velocity component
    float currentPitchRate = 0;
    float yawRate = 0;
    float speed = 0;
    float Thrust = 0;          // Calculated force based on acceleration

    vec3 Velocity;          // World space velocity
    vec3 localVelocity;     // Velocity relative to aircraft orientation

    bool waypointReach = false;

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

    ///Current
    vec3 Position;
    vec3 LastPosition;
    vec3 EularAngles;
    vec3 LocalEularAngles;
//     float Speed = 0.0f;//m/s
//     float Altitude = 0;//m
//     float Heading = 0;//deg

    vec3 Forward;
//     // Transform* transform;
//     // Trajectory* trajectory;
    void FixedUpdate(float delta);

private:
    vec3 _moveVelocity = vec3{0,0,0};
    float _currentHeading = 0;
    float _smoothTurnRate = 0;
    float _smoothPitchRate = 0;
    float _lastFrameSpeed = 0;


    const double Rad2Deg = 180.0 / M_PI;
    const double Deg2Rad = M_PI / 180.0;
    float NormalizeAngle(float angle);
    double convertToClockwise360(double lon180);
    float Lerp(float a, float b, float t);
};

#endif // AIRCRAFT_H
