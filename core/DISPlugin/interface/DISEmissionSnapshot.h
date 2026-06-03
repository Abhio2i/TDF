// =============================================================================
// FILE:        DISEmissionSnapshot.h
// MODULE:      DIS Network Plugin — Interface
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Plain data struct for DIS Electromagnetic Emission PDU.
//              Sent when a radar or sensor is actively emitting.
//              Bridge fills this from Platform sensor list.
//              No Qt. No engine types. No DIS library types.
//
// MAPS TO:     DIS7::ElectromagneticEmissionsPdu (PDU type 23)
//              Also covers UaPdu (underwater acoustic, PDU type 29)
//              for submarine/naval sonar
// =============================================================================

#ifndef DISEMISSIONSNAPSHOT_H
#define DISEMISSIONSNAPSHOT_H

#include <string>
#include <cstdint>
#include <vector>

// =============================================================================
// Emission function codes — IEEE 1278.1 enumerations
// =============================================================================
namespace DISEmissionFunction {
    constexpr uint8_t Other             = 0;
    constexpr uint8_t MultiFunction     = 1;
    constexpr uint8_t EarlyWarning      = 2;
    constexpr uint8_t HeightFinding     = 3;
    constexpr uint8_t FireControl       = 4;
    constexpr uint8_t AcquisitionDet    = 5;
    constexpr uint8_t Tracking          = 6;
    constexpr uint8_t MissileGuidance   = 7;
    constexpr uint8_t Illumination      = 8;
    constexpr uint8_t RangeOnly         = 9;
    constexpr uint8_t RadarAltimeter    = 10;
    constexpr uint8_t Imaging           = 11;
    constexpr uint8_t MotionDetection   = 12;
    constexpr uint8_t Navigation        = 13;
    constexpr uint8_t WeaponGuidance    = 14;
    constexpr uint8_t ActiveRadarGuid   = 15;
    constexpr uint8_t Sonar             = 16; // naval sonar
}

// =============================================================================
// DISEmissionBeam
// One radar beam from one emitter system
// =============================================================================
struct DISEmissionBeam {
    uint8_t  beamID          = 1;
    float    frequency       = 0.0f;   // Hz
    float    freqRange       = 0.0f;   // Hz bandwidth
    float    erpPower        = 0.0f;   // dBm effective radiated power
    float    prf             = 0.0f;   // Hz pulse repetition frequency
    float    pulseWidth      = 0.0f;   // microseconds
    float    beamAzCenter    = 0.0f;   // degrees azimuth centre
    float    beamAzSweep     = 0.0f;   // degrees azimuth sweep
    float    beamElCenter    = 0.0f;   // degrees elevation centre
    float    beamElSweep     = 0.0f;   // degrees elevation sweep
    uint8_t  beamFunction    = DISEmissionFunction::Tracking;

    // Tracks (targets being illuminated)
    std::vector<std::string> trackedEntityIDs;
};

// =============================================================================
// DISEmissionSnapshot
// State of one emitter system on one entity
//
// Bridge fills this from Platform sensor list:
//   emittingEntityID ← platform->ID
//   emitterName      ← sensor->Name
//   emitterFunction  ← mapped from sensor type (Radar/Sonar/ESM)
//   location         ← platform transform position
//   beams            ← one per radar beam
// =============================================================================
struct DISEmissionSnapshot {

    // ── Identity ──────────────────────────────────────────────────────────────
    std::string emittingEntityID;
    std::string emitterName;        // sensor name, first 12 chars used in PDU

    // ── Location ──────────────────────────────────────────────────────────────
    double latitude  = 0.0;
    double longitude = 0.0;
    double altitude  = 0.0;

    // ── System ───────────────────────────────────────────────────────────────
    uint8_t emitterFunction = DISEmissionFunction::MultiFunction;
    uint8_t systemID        = 1;  // system number on this entity

    // ── Beams ────────────────────────────────────────────────────────────────
    std::vector<DISEmissionBeam> beams;

    // ── State ────────────────────────────────────────────────────────────────
    bool active = true;  // false = sensor turned off
};

#endif // DISEMISSIONSNAPSHOT_H