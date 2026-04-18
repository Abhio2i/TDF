#include "georelativeangle.h"
#include <cmath>

static constexpr double PI = 3.14159265358979323846;

double GeoRelativeAngle::degToRad(double deg)
{
    return deg * PI / 180.0;
}

double GeoRelativeAngle::radToDeg(double rad)
{
    return rad * 180.0 / PI;
}

// Normalize angle to [-180, 180]
double GeoRelativeAngle::normalizeAngle(double angleDeg)
{
    while (angleDeg > 180.0) angleDeg -= 360.0;
    while (angleDeg < -180.0) angleDeg += 360.0;
    return angleDeg;
}

double GeoRelativeAngle::computeRelativeAngle(double sensorLatDeg,
                                              double sensorLonDeg,
                                              double sensorHeadingDeg,
                                              double targetLatDeg,
                                              double targetLonDeg)
{
    // Convert to radians
    double lat1 = degToRad(sensorLatDeg);
    double lon1 = degToRad(sensorLonDeg);
    double lat2 = degToRad(targetLatDeg);
    double lon2 = degToRad(targetLonDeg);

    double dLon = lon2 - lon1;

    // Compute bearing from sensor to target
    double y = sin(dLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) -
               sin(lat1) * cos(lat2) * cos(dLon);

    double bearingRad = atan2(y, x);
    double bearingDeg = radToDeg(bearingRad);

    // Convert bearing from [-180,180] to [0,360]
    if (bearingDeg < 0)
        bearingDeg += 360.0;

    // Compute relative angle between heading and bearing
    double relativeAngle = bearingDeg - sensorHeadingDeg;

    return normalizeAngle(relativeAngle);
}
