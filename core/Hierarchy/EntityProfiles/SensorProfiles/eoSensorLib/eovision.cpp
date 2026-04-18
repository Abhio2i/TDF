#include "eovision.h"

EOVision::EOVision() {}

Vec3 EOVision::geoToECEF(double lat, double lon, double alt)
{
    double R = 6378137.0; // Earth radius approx
    double latRad = lat * M_PI / 180.0;
    double lonRad = lon * M_PI / 180.0;

    double x = (R + alt) * cos(latRad) * cos(lonRad);
    double y = (R + alt) * cos(latRad) * sin(lonRad);
    double z = (R + alt) * sin(latRad);

    return {x, y, z};
}

Vec3 EOVision::ecefToENU(Vec3 target, Vec3 sensor, double lat_s, double lon_s)
{
    double latRad = lat_s * M_PI / 180.0;
    double lonRad = lon_s * M_PI / 180.0;

    Vec3 d = target - sensor;

    double east  = -sin(lonRad)*d.x + cos(lonRad)*d.y;
    double north = -sin(latRad)*cos(lonRad)*d.x
                   -sin(latRad)*sin(lonRad)*d.y
                   +cos(latRad)*d.z;
    double up    =  cos(latRad)*cos(lonRad)*d.x
                +cos(latRad)*sin(lonRad)*d.y
                +sin(latRad)*d.z;

    return {east, north, up};
}

// Vec3 EOVision::rotateToSensorFrame(Vec3 v, double headingDeg, double pitchDeg)
// {
//     double h = -headingDeg * M_PI / 180.0; // NEGATIVE
//     double p = -pitchDeg * M_PI / 180.0;   // NEGATIVE

//     // Yaw (Z axis)
//     double x1 = cos(h)*v.x - sin(h)*v.y;
//     double y1 = sin(h)*v.x + cos(h)*v.y;
//     double z1 = v.z;

//     // Pitch (X axis)
//     double x2 = x1;
//     double y2 = cos(p)*y1 - sin(p)*z1;
//     double z2 = sin(p)*y1 + cos(p)*z1;

//     return {x2, y2, z2};
// }
Vec3 EOVision::rotateToSensorFrame(Vec3 v, double headingDeg, double pitchDeg)
{
    double h = headingDeg * M_PI / 180.0;
    double p = pitchDeg * M_PI / 180.0;

    // --- Yaw (Heading) ---
    // Rotate ENU so that heading direction becomes +Y (forward)
    double x1 =  cos(h)*v.x - sin(h)*v.y;
    double y1 =  sin(h)*v.x + cos(h)*v.y;
    double z1 =  v.z;

    // --- Pitch ---
    double x2 = x1;
    double y2 =  cos(p)*y1 + sin(p)*z1;
    double z2 = -sin(p)*y1 + cos(p)*z1;

    return {x2, y2, z2};
}

bool EOVision::isInsideFOV(Vec3 v, double hfovDeg, double vfovDeg)
{
    double angleH = atan2(v.x, v.y) * 180.0 / M_PI;
    double angleV = atan2(v.z, v.y) * 180.0 / M_PI;

    return fabs(angleH) <= hfovDeg/2 &&
           fabs(angleV) <= vfovDeg/2 &&
           v.y > 0; // in front
}

Vec2 EOVision::project(Vec3 v, double focalLength)
{
    if (v.y <= 0) return {0, 0}; // or skip
    return {
        focalLength * (v.x / v.y),
        focalLength * (v.z / v.y)
    };
}
