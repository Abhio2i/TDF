#ifndef EOVISION_H
#define EOVISION_H
#include "payload.h"
class EOVision
{
public:
    EOVision();
    Vec3 geoToECEF(float lat, float lon, float alt);
    Vec3 ecefToENU(Vec3 target, Vec3 sensor, float lat_s, float lon_s);
    Vec3 rotateToSensorFrame(Vec3 v, float headingDeg, float pitchDeg);
    bool isInsideFOV(Vec3 v, float hfovDeg, float vfovDeg);
    Vec2 project(Vec3 v, float focalLength);
};

#endif // EOVISION_H
