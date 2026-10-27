#ifndef DISDESIGNATORSNAPSHOT_H
#define DISDESIGNATORSNAPSHOT_H
#include <string>
#include <cstdint>

// Designator/laser designation state — Section 5.3.7.2 (PDU type 24).
struct DISDesignatorSnapshot {
    std::string designatingEntityID;
    std::string designatedEntityID;

    uint16_t codeName       = 0;
    uint16_t designatorCode = 1113;

    float designatorPower      = 0.0f;
    float designatorWavelength = 1.064f;

    double latitude  = 0.0;
    double longitude = 0.0;
    double altitude  = 0.0;

    uint8_t deadReckoningAlgorithm = 1;
    bool    active = true;
};
#endif
