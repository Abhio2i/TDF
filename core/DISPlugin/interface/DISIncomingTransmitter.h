#ifndef DISINCOMINGTRANSMITTER_H
#define DISINCOMINGTRANSMITTER_H
#include <string>
#include <cstdint>

struct DISIncomingTransmitter {
    std::string transmittingEntityID;
    uint16_t radioID = 0;
    uint8_t  transmitState = 0;
    double latitude = 0.0, longitude = 0.0, altitude = 0.0;
    uint64_t frequency = 0;
    float power = 0.0f;
};
#endif
