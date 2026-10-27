#ifndef DISINCOMINGSIGNAL_H
#define DISINCOMINGSIGNAL_H
#include <string>
#include <cstdint>
#include <vector>

struct DISIncomingSignal {
    std::string transmittingEntityID;
    uint16_t radioID = 0;
    uint16_t encodingScheme = 0;
    uint32_t sampleRate = 0;
    uint16_t samples = 0;
    std::vector<uint8_t> data;
};
#endif
