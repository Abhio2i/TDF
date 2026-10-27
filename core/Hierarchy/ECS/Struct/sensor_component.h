#ifndef SENSOR_COMPONENT_H
#define SENSOR_COMPONENT_H
#include <cstdint>
namespace ECS {
#pragma pack(push, 1)
struct sensorDataComponent{
    float minAzimuth = -30;
    float maxAzimuth = 30;
    float range = 1;
};

struct sensorComponent{
    sensorDataComponent sensor[10];
    uint16_t activeCount;
};
#pragma pack(pop)



}
#endif // SENSOR_COMPONENT_H
