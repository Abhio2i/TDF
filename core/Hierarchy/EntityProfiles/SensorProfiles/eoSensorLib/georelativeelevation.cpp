#include "georelativeelevation.h"
#include <cmath>

static constexpr float PI = 3.14159265358979323846;
static constexpr float EARTH_RADIUS = 6371000.0; // meters

float GeoRelativeElevation::degToRad(float deg)
{
    return deg * PI / 180.0;
}

float GeoRelativeElevation::radToDeg(float rad)
{
    return rad * 180.0 / PI;
}

// Great-circle horizontal distance
float GeoRelativeElevation::distanceMeters(float lat1, float lon1,
                                            float lat2, float lon2)
{
    lat1 = degToRad(lat1);
    lon1 = degToRad(lon1);
    lat2 = degToRad(lat2);
    lon2 = degToRad(lon2);

    float dLat = lat2 - lat1;
    float dLon = lon2 - lon1;

    float a = sin(dLat/2)*sin(dLat/2) +
               cos(lat1)*cos(lat2)*sin(dLon/2)*sin(dLon/2);

    float c = 2 * atan2(sqrt(a), sqrt(1-a));
    return EARTH_RADIUS * c;
}

float GeoRelativeElevation::computeRelativeElevation(
    float sensorLatDeg,
    float sensorLonDeg,
    float sensorAltMeters,
    float sensorPitchDeg,
    float targetLatDeg,
    float targetLonDeg,
    float targetAltMeters)
{
    // Horizontal ground distance
    float horizontalDist = distanceMeters(sensorLatDeg, sensorLonDeg,
                                           targetLatDeg, targetLonDeg);

    // Vertical difference
    float deltaAlt = targetAltMeters - sensorAltMeters;

    // Elevation angle from sensor to target
    float elevationRad = atan2(deltaAlt, horizontalDist);
    float elevationDeg = radToDeg(elevationRad);

    // Relative to sensor pitch
    return elevationDeg - sensorPitchDeg;
}
