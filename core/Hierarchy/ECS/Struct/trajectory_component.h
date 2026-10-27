#ifndef TRAJECTORY_COMPONENT_H
#define TRAJECTORY_COMPONENT_H
#include <cstdint>
namespace ECS {

#pragma pack(push, 1)
struct waypointDataComponent{
    double lat;
    double lon;
};

struct trajectoryPathComponent{
    waypointDataComponent path[101];
    uint16_t activeCount;
    bool delta = false;
    int currentWaypoint = 0;
    bool active = true;
};
#pragma pack(pop)



struct trajectoryRuleComponent{
    enum FollowRule{
        Stop,
        Reverse,
        Short
    };
    FollowRule rule;
    bool reverse;
};

struct trajectoryDynamicComponent{
    float speed;
    float altitude;
    int currentWaypoint = 0;
};

struct trajectoryEventComponent{
    bool formation;
};



}
#endif // TRAJECTORY_COMPONENT_H
