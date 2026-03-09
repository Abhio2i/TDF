#include "aircraft.h"
#include <cmath>
#include <algorithm>
Aircraft::Aircraft() {}

Aircraft::~Aircraft() {}

void Aircraft::FixedUpdate(float delta){
    // 1. STATE SYNCHRONIZATION
    _currentHeading = EularAngles.y;
    Altitude = Position.y;
    float targetAltitude = TargetPosition.y;
    float altitudeDifference = std::abs(Altitude - targetAltitude);

    // Calculate Altitude Ratio (efficiency drops as we approach Ceiling)
    float altRatio = 1.0f - (Altitude / CeilingHeight);
    altRatio = std::clamp(altRatio,0.0f,1.0f);

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

    // Assign pitch direction (In Unity, negative X usually tilts the nose UP)
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

    // Calculate shortest rotation path
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

    // Update Telemetry
    Velocity = _moveVelocity / delta;
    // localVelocity = transform.InverseTransformDirection(Velocity);

    if(vec3::Distance(Position,TargetPosition)< speed)
    {
        waypointReach = true;
    }
    else
    {
        waypointReach = false;
    }


}

float Aircraft::NormalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

double Aircraft::convertToClockwise360(double lon180) {
    double lon360 = (lon180 < 0) ? (lon180 + 360.0) : lon180;
    double clockwise =lon360;
    if (clockwise >= 360.0) clockwise = 0.0;
    return clockwise;
}

float Aircraft::Lerp(float a, float b, float t) {
    return a + t * (b - a);
}
