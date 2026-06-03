#include "georelativeangle.h"
#include <cmath>

static constexpr float PI = 3.14159265358979323846;

float GeoRelativeAngle::degToRad(float deg)
{
    return deg * PI / 180.0;
}

float GeoRelativeAngle::radToDeg(float rad)
{
    return rad * 180.0 / PI;
}

// Normalize angle to [-180, 180]
float GeoRelativeAngle::normalizeAngle(float angleDeg)
{
    while (angleDeg > 180.0) angleDeg -= 360.0;
    while (angleDeg < -180.0) angleDeg += 360.0;
    return angleDeg;
}

float GeoRelativeAngle::computeRelativeAngle(
    float sensorLatDeg,
    float sensorLonDeg,
    float sensorHeadingDeg,
    float targetLatDeg,
    float targetLonDeg)
{
    // Convert to radians
    float lat1 = degToRad(sensorLatDeg);
    float lon1 = degToRad(sensorLonDeg);
    float lat2 = degToRad(targetLatDeg);
    float lon2 = degToRad(targetLonDeg);

    float dLon = lon2 - lon1;

    // Compute bearing from sensor to target
    float y = sin(dLon) * cos(lat2);
    float x = cos(lat1) * sin(lat2) -
               sin(lat1) * cos(lat2) * cos(dLon);

    float bearingRad = atan2(y, x);
    float bearingDeg = radToDeg(bearingRad);

    // Convert bearing from [-180,180] to [0,360]
    if (bearingDeg < 0)
        bearingDeg += 360.0;

    // Compute relative angle between heading and bearing
    float relativeAngle = bearingDeg - sensorHeadingDeg;

    return normalizeAngle(relativeAngle);
}
