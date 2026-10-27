#ifndef DYNAMIC_COMPONENT_H
#define DYNAMIC_COMPONENT_H
#include "qvector3d.h"
namespace ECS {

struct DynamicSpeedConstants{
    float maxSpeed;
    float minSpeed;
};

struct DynamicAltitudeConstants{
    float maxAlttiude;
    float minAltitude;
};

struct DynamicClimbRateConstants{
    float maxClimbRate;
    float maxDiveRate;
};

struct DynamicMaxAcceDeccConstants{
    float MaxAcceleration;
    float MaxDecceleration;
};

struct DynamicVariables{
    float speed;
    float turnrate;
};

struct DynamicVelocity{
    QVector3D velocity;
};

struct DynamicControls{
    bool contole;
    bool follow;
    bool followTarget;
};

struct DynamicOrientation{
    float pitch;
    float roll;
    float yaw;
};

struct DynamicOrientationRate{
    float pitchRate;
    float rollRate;
    float yawRate;
};

struct DynamicGeoData{
    float DriftAngle;
    float TrueHeading;
    float TrueAirSpeed;
    float NorthVelocity;
    float EastVelocity;
    float VerticalVelocity;
    float GroundVelocity;
    float localVelocity;
};

}
#endif // DYNAMIC_COMPONENT_H
