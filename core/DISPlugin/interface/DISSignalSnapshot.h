#ifndef DISSIGNALSNAPSHOT_H
#define DISSIGNALSNAPSHOT_H
#include <string>
#include <cstdint>
#include <vector>

// Radio signal data — Section 7.7.3 (PDU type 26). Only meaningful
// alongside an active Transmitter PDU with the same radioID.
struct DISSignalSnapshot {
    std::string transmittingEntityID;
    uint16_t radioID = 1;               // must match an active Transmitter's radioID
    uint16_t encodingScheme = 0;        // 0 = placeholder; no real codec wired yet
    uint16_t tdlType = 0;               // 0 = Other / not a TDL message
    uint32_t sampleRate = 0;
    uint16_t samples = 0;
    std::vector<uint8_t> data;
};
#endif
