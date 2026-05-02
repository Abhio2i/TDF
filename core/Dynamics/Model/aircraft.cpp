/**
 * @file aircraft.cpp
 * @brief Implementation of the Aircraft class for fixed-wing flight dynamics.
 */

#include "aircraft.h"
#include "qmath.h"
#include <cmath>
#include <algorithm>

/**
 * @brief Constructs an Aircraft object.
 */
Aircraft::Aircraft() {}

/**
 * @brief Destructor.
 */
Aircraft::~Aircraft() {}

/**
 * @brief Updates aircraft physics and navigation over a fixed timestep.
 * @param delta Time step in seconds.
 *
 * This method performs:
 * - State synchronization (heading, altitude)
 * - Climb and pitch calculation with predictive level‑off
 * - Pitch smoothing with dynamic rate
 * - Navigation (yaw) using shortest‑path turn logic
 * - Engine thrust based on altitude efficiency
 * - Speed control with acceleration/deceleration
 * - Roll (bank) from turn rate
 * - Position update with wind displacement
 * - Waypoint reach detection
 */
void Aircraft::FixedUpdate(float delta){

    // 1. STATE SYNCHRONIZATION
    _currentHeading = EularAngles.y;
    Altitude = Position.y;
    float targetAltitude = TargetPosition.y;
    float altitudeDifference = std::abs(Altitude - targetAltitude);

    // Calculate Altitude Ratio (efficiency drops as we approach Ceiling)
    float altRatio = 1.0f - (Altitude / CeilingHeight);
    altRatio = std::clamp(altRatio, 0.0f, 1.0f);

    // 2. CLIMB AND PITCH CALCULATION
    // Calculate required vertical rate based on altitude gap and ceiling efficiency
    float targetClimbRes = ClimbRate * std::pow(altRatio, 0.1f);
    float verticalRatio = (targetAltitude > Altitude ? targetClimbRes : DiveRate) / std::max(speed, 1.0f);

    // Convert vertical requirement to pitch angle (theta = asin(climb/speed))
    float angleInRadians = std::asin(std::clamp(verticalRatio, -1.0f, 1.0f));
    RequirePitch = angleInRadians * Rad2Deg;

    // Predictive Level-Off: Prevent overshooting the target altitude
    float currentVerticalSpeed = speed * std::sin(std::abs(Pitch) * Deg2Rad);
    float timeToLevelOff = std::abs(Pitch) / PitchRate;
    float stoppingDistance = currentVerticalSpeed * timeToLevelOff * 0.5f;

    currentClimbRate = currentVerticalSpeed;

    if (altitudeDifference < stoppingDistance * 2.0f)
    {
        RequirePitch = 0; // Level off nose
    }

    // Assign pitch direction (negative X tilts nose UP in many coordinate systems)
    RequirePitch = (targetAltitude > Altitude) ? -std::abs(RequirePitch) : std::abs(RequirePitch);

    // 3. PITCH SMOOTHING
    float pitchDiff = std::abs(Pitch - RequirePitch);
    float dynamicPitchRate = PitchRate;

    if (pitchDiff < PitchRate) dynamicPitchRate = 0.5f * pitchDiff;

    _smoothPitchRate = Lerp(_smoothPitchRate, dynamicPitchRate, delta * 2.0f);

    if (Pitch < RequirePitch)
        Pitch = std::min(Pitch + _smoothPitchRate * delta, MaxPitch);
    else
        Pitch = std::max(Pitch - _smoothPitchRate * delta, -MaxPitch);

    currentPitchRate = _smoothPitchRate;

    // 4. NAVIGATION & YAW (Heading Logic)
    vec3 targetDir = (TargetPosition - Position);
    targetDir.y = 0;
    targetDir.Normalize();

    float targetHeading = std::atan2(targetDir.x, targetDir.z) * Rad2Deg;
    float headingDelta = std::abs(NormalizeAngle(targetHeading - _currentHeading));

    // Smooth turn rate as we align with target heading
    float dynamicTurnRate = turnRate;
    if (headingDelta <= dynamicTurnRate) dynamicTurnRate = 0.5f * headingDelta;

    _smoothTurnRate = Lerp(_smoothTurnRate, dynamicTurnRate, delta * 2.0f);

    // Calculate shortest rotation path using 0-360 conversion
    float target360 = (float)convertToClockwise360(targetHeading);
    float current360 = (float)convertToClockwise360(_currentHeading);
    targetHeading = NormalizeAngle(targetHeading);
    _currentHeading = NormalizeAngle(_currentHeading);

    if (std::abs(targetHeading - _currentHeading) > std::abs(target360 - current360)) {
        _currentHeading = current360;
        targetHeading = target360;
    }

    // Apply rotation
    if (_currentHeading > targetHeading) {
        _currentHeading -= _smoothTurnRate * delta;
        yawRate = -_smoothTurnRate;
    } else {
        _currentHeading += _smoothTurnRate * delta;
        yawRate = _smoothTurnRate;
    }

    // 5. ENGINE & THRUST LOGIC
    float currentAccel = MaxAcceleration * std::pow(altRatio, 0.1f);
    Thrust = Mass * currentAccel; // Standard F = ma

    float speedGap = std::abs(speed - TargetSpeed);
    if (speed < TargetSpeed) {
        float step = (speedGap < currentAccel) ? 0.5f * speedGap : currentAccel;
        speed += step * delta;
    } else {
        float step = (speedGap < MaxDecceleration) ? 0.5f * speedGap : MaxDecceleration;
        speed -= step * delta;
    }
    speed = std::max(speed, 0.0f);

    // 6. FINAL TRANSFORM APPLICATION
    // Apply Heading
    EularAngles = vec3(0, _currentHeading, 0);

    // Apply Pitch and Bank (Roll)
    // Roll is visually calculated as a factor of the current turn rate
    vec3 rot = LocalEularAngles;
    rot.x = Pitch;
    rot.z = -yawRate * 1.5f;
    LocalEularAngles = rot;

    // 7. TRANSLATION (Movement)
    // Calculate forward movement
    _moveVelocity = Forward * speed * delta;

    // Add Wind Displacement
    vec3 windVector = vec3(
                          std::sin(WindDirection * Deg2Rad),
                          0,
                          std::cos(WindDirection * Deg2Rad)
                          ) * WindIntensity * delta;

    _moveVelocity += windVector;

    // Update Physics Position
    Position += _moveVelocity;
    //calculate driftangle
    float windAngleRad = qDegreesToRadians(qRadiansToDegrees(qAtan2(windVector.x, -windVector.z)) - TrueHeading);

    float sinDrift = (WindIntensity / TrueAirSpeed) * qSin(windAngleRad);

    if (sinDrift > 1.0f) sinDrift = 1.0f;
    if (sinDrift < -1.0f) sinDrift = -1.0f;

    // Update Telemetry
    DriftAngle = qRadiansToDegrees(qAsin(sinDrift));
    Velocity = _moveVelocity / delta;
    TrueAirSpeed = (_moveVelocity / delta).Magnitude();
    GroundVelocity = WindIntensity+TrueAirSpeed;
    NorthVelocity = Velocity.x;
    EastVelocity = Velocity.z;
    VerticalVelocity = Velocity.y;
    Roll = Yaw*0.7f;
    TrueHeading = _currentHeading;
    Yaw = _currentHeading;
    // 1. Pehle Global Velocity (Ground Velocity) calculate karein
    vec3 globalVelocity = _moveVelocity / delta;

    // 2. Relative Velocity calculate karne ke liye
    // Hamein velocity ko -Heading aur -Pitch se rotate karna hoga
    float h = _currentHeading * Deg2Rad;
    float p = Pitch * Deg2Rad;

    // Simple Trigonometry to bring Global Velocity into Body Frame
    // Note: Isme Roll (Z) ko aksar skip kiya jata hai unless precision zaruri ho
    float cosH = std::cos(-h);
    float sinH = std::sin(-h);
    float cosP = std::cos(-p);
    float sinP = std::sin(-p);

    // Heading rotation (Y-axis)
    float tempX = globalVelocity.x * cosH - globalVelocity.z * sinH;
    float tempZ = globalVelocity.x * sinH + globalVelocity.z * cosH;

    // Pitch rotation (X-axis)
    float relativeForward = tempZ * cosP - globalVelocity.y * sinP;
    float relativeVertical = tempZ * sinP + globalVelocity.y * cosP;
    float relativeSide = tempX;

    localVelocity = vec3(relativeSide, relativeVertical, relativeForward);


    if (vec3::Distance(Position, TargetPosition) < speed)
    {
        waypointReach = true;
    }
    else
    {
        waypointReach = false;
    }
}

/**
 * @brief Normalizes an angle to the range [-180, 180] degrees.
 * @param angle Input angle in degrees.
 * @return Normalized angle.
 */
float Aircraft::NormalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

/**
 * @brief Converts a longitude from -180..180 to 0..360 clockwise.
 * @param lon180 Longitude in degrees (-180 to 180).
 * @return Longitude in degrees (0 to 360).
 */
double Aircraft::convertToClockwise360(double lon180) {
    double lon360 = (lon180 < 0) ? (lon180 + 360.0) : lon180;
    double clockwise = lon360;
    if (clockwise >= 360.0) clockwise = 0.0;
    return clockwise;
}

/**
 * @brief Linear interpolation between two floats.
 * @param a Start value.
 * @param b End value.
 * @param t Interpolation factor (0..1).
 * @return Interpolated value.
 */
float Aircraft::Lerp(float a, float b, float t) {
    return a + t * (b - a);
}
