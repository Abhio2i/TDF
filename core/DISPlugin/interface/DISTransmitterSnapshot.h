#ifndef DISTRANSMITTERSNAPSHOT_H
#define DISTRANSMITTERSNAPSHOT_H
#include <string>
#include <cstdint>

// Radio transmitter state — Section 7.7.2 (PDU type 25).
struct DISTransmitterSnapshot {
    std::string transmittingEntityID;
    uint16_t radioID = 1;

    // RadioType — defaults to "Other" until a real radio-type mapping exists
    uint8_t  radioKind = 0;
    uint8_t  radioDomain = 0;
    uint16_t radioCountry = 0;
    uint8_t  radioCategory = 0;
    uint8_t  radioSubcategory = 0;
    uint8_t  radioSpecific = 0;
    uint8_t  radioExtra = 0;

    uint8_t transmitState = 2;   // 0=Off, 1=On-not-transmitting, 2=On-transmitting
    uint8_t inputSource   = 0;   // 0=Other

    double latitude  = 0.0;      // antenna location, world coords
    double longitude = 0.0;
    double altitude  = 0.0;

    uint64_t frequency = 0;                    // Hz — wire type is genuinely 64-bit
    float transmitFrequencyBandwidth = 0.0f;   // Hz
    float power = 0.0f;                        // dBm

    uint16_t modSpreadSpectrum  = 0;
    uint16_t modMajorModulation = 0;
    uint16_t modDetail          = 0;
    uint16_t modRadioSystem     = 0;

    uint16_t cryptoSystem = 0;
    uint16_t cryptoKeyId  = 0;

    bool active = true;
};
#endif
