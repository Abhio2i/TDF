#ifndef DISINCOMINGDESIGNATOR_H
#define DISINCOMINGDESIGNATOR_H
#include <string>
#include <cstdint>

struct DISIncomingDesignator {
    std::string designatingEntityID;
    std::string designatedEntityID;
    uint16_t codeName       = 0;
    uint16_t designatorCode = 0;
    float designatorPower      = 0.0f;
    float designatorWavelength = 0.0f;
    double latitude  = 0.0;
    double longitude = 0.0;
    double altitude  = 0.0;
};
#endif
