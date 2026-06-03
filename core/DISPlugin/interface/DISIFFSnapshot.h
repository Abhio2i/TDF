#ifndef DISIFFSNAPSHOT_H
#define DISIFFSNAPSHOT_H
// =============================================================================
// FILE:        DISIFFSnapshot.h
// MODULE:      DIS Network Plugin — Interface
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// DESCRIPTION: IFF state snapshot for DIS PDU transmission.
//              Built from IFF engine component, sent as DIS IFF PDU (type 28).
// =============================================================================
#pragma once
#include <string>
#include <cstdint>

struct DISIFFSnapshot {
    std::string entityID;

    // System state
    bool     systemOn     = true;   // transponder active
    uint8_t  forceID      = 1;      // 1=Friendly 2=Opposing 3=Neutral

    // Mode codes — parsed from IFF::ModeConfiguration strings
    uint16_t mode1Code    = 0;      // military mission code
    uint16_t mode2Code    = 0;      // individual ID code
    uint16_t mode3ACode   = 1200;   // ATC squawk code
    bool     mode4Active  = false;  // encrypted mode on/off
    uint16_t modeCCode    = 0;      // altitude code

    // DIS addressing
    uint16_t siteID       = 1;
    uint16_t applicationID = 1;
};
#endif // DISIFFSNAPSHOT_H
