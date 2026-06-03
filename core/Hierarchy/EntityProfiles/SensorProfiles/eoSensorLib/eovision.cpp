#include "eovision.h"

EOVision::EOVision() {}

Vec3 EOVision::geoToECEF(float lat, float lon, float alt)
{
    float R = 6378137.0; // Earth radius approx
    float latRad = lat * M_PI / 180.0;
    float lonRad = lon * M_PI / 180.0;

    float x = (R + alt) * cos(latRad) * cos(lonRad);
    float y = (R + alt) * cos(latRad) * sin(lonRad);
    float z = (R + alt) * sin(latRad);

    return {x, y, z};
}

Vec3 EOVision::ecefToENU(Vec3 target, Vec3 sensor, float lat_s, float lon_s)
{
    float latRad = lat_s * M_PI / 180.0;
    float lonRad = lon_s * M_PI / 180.0;

    Vec3 d = target - sensor;

    float east  = -sin(lonRad)*d.x + cos(lonRad)*d.y;
    float north = -sin(latRad)*cos(lonRad)*d.x
                   -sin(latRad)*sin(lonRad)*d.y
                   +cos(latRad)*d.z;
    float up    =  cos(latRad)*cos(lonRad)*d.x
                +cos(latRad)*sin(lonRad)*d.y
                +sin(latRad)*d.z;

    return {east, north, up};
}

// Vec3 EOVision::rotateToSensorFrame(Vec3 v, float headingDeg, float pitchDeg)
// {
//     float h = -headingDeg * M_PI / 180.0; // NEGATIVE
//     float p = -pitchDeg * M_PI / 180.0;   // NEGATIVE

//     // Yaw (Z axis)
//     float x1 = cos(h)*v.x - sin(h)*v.y;
//     float y1 = sin(h)*v.x + cos(h)*v.y;
//     float z1 = v.z;

//     // Pitch (X axis)
//     float x2 = x1;
//     float y2 = cos(p)*y1 - sin(p)*z1;
//     float z2 = sin(p)*y1 + cos(p)*z1;

//     return {x2, y2, z2};
// }
Vec3 EOVision::rotateToSensorFrame(Vec3 v, float headingDeg, float pitchDeg)
{
    float h = headingDeg * M_PI / 180.0;
    float p = pitchDeg * M_PI / 180.0;

    // --- Yaw (Heading) ---
    // Rotate ENU so that heading direction becomes +Y (forward)
    float x1 =  cos(h)*v.x - sin(h)*v.y;
    float y1 =  sin(h)*v.x + cos(h)*v.y;
    float z1 =  v.z;

    // --- Pitch ---
    float x2 = x1;
    float y2 =  cos(p)*y1 + sin(p)*z1;
    float z2 = -sin(p)*y1 + cos(p)*z1;

    return {x2, y2, z2};
}

bool EOVision::isInsideFOV(Vec3 v, float hfovDeg, float vfovDeg)
{
    float angleH = atan2(v.x, v.y) * 180.0 / M_PI;
    float angleV = atan2(v.z, v.y) * 180.0 / M_PI;

    return fabs(angleH) <= hfovDeg/2 &&
           fabs(angleV) <= vfovDeg/2 &&
           v.y > 0; // in front
}

Vec2 EOVision::project(Vec3 v, float focalLength)
{
    if (v.y <= 0) return {0, 0}; // or skip
    return {
        focalLength * (v.x / v.y),
        focalLength * (v.z / v.y)
    };
}
