#ifndef RADIO_COMPONENT_H
#define RADIO_COMPONENT_H

#include <cstdint>
namespace ECS {
#pragma pack(push, 1)

struct radioComponent{
    int radios[100];
    uint16_t activeCount;
};
#pragma pack(pop)


}

#endif // RADIO_COMPONENT_H
