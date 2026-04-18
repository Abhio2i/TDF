#include "georelativeelevation.h"
#include <cmath>

static constexpr double PI = 3.14159265358979323846;
static constexpr double EARTH_RADIUS = 6371000.0; // meters

double GeoRelativeElevation::degToRad(double deg)
{
    return deg * PI / 180.0;
}

double GeoRelativeElevation::radToDeg(double rad)
{
    return rad * 180.0 / PI;
}

// Great-circle horizontal distance
double GeoRelativeElevation::distanceMeters(double lat1, double lon1,
                                            double lat2, double lon2)
{
    lat1 = degToRad(lat1);
    lon1 = degToRad(lon1);
    lat2 = degToRad(lat2);
    lon2 = degToRad(lon2);

    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;

    double a = sin(dLat/2)*sin(dLat/2) +
               cos(lat1)*cos(lat2)*sin(dLon/2)*sin(dLon/2);

    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return EARTH_RADIUS * c;
}

double GeoRelativeElevation::computeRelativeElevation(
    double sensorLatDeg,
    double sensorLonDeg,
    double sensorAltMeters,
    double sensorPitchDeg,
    double targetLatDeg,
    double targetLonDeg,
    double targetAltMeters)
{
    // Horizontal ground distance
    double horizontalDist = distanceMeters(sensorLatDeg, sensorLonDeg,
                                           targetLatDeg, targetLonDeg);

    // Vertical difference
    double deltaAlt = targetAltMeters - sensorAltMeters;

    // Elevation angle from sensor to target
    double elevationRad = atan2(deltaAlt, horizontalDist);
    double elevationDeg = radToDeg(elevationRad);

    // Relative to sensor pitch
    return elevationDeg - sensorPitchDeg;
}
