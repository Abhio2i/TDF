#ifndef EOVISION_H
#define EOVISION_H
#include "payload.h"
class EOVision
{
public:
    EOVision();
    Vec3 geoToECEF(double lat, double lon, double alt);
    Vec3 ecefToENU(Vec3 target, Vec3 sensor, double lat_s, double lon_s);
    Vec3 rotateToSensorFrame(Vec3 v, double headingDeg, double pitchDeg);
    bool isInsideFOV(Vec3 v, double hfovDeg, double vfovDeg);
    Vec2 project(Vec3 v, double focalLength);
};

#endif // EOVISION_H
