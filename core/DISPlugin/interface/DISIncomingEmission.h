#ifndef DISINCOMINGEMISSION_H
#define DISINCOMINGEMISSION_H
#include <string>
#include <cstdint>
#include <vector>

struct DISIncomingEmissionBeam {
    uint8_t beamID       = 0;
    float   frequency    = 0.0f;
    float   freqRange    = 0.0f;
    float   erpPower     = 0.0f;
    float   prf          = 0.0f;
    float   pulseWidth   = 0.0f;
    float   beamAzCenter = 0.0f;  // degrees
    float   beamAzSweep  = 0.0f;
    float   beamElCenter = 0.0f;
    float   beamElSweep  = 0.0f;
    uint8_t beamFunction = 0;
    bool    active       = true;
};

struct DISIncomingEmission {
    std::string emittingEntityID;
    uint8_t emitterFunction = 0;
    uint8_t systemID        = 1;
    std::vector<DISIncomingEmissionBeam> beams;
};
#endif
