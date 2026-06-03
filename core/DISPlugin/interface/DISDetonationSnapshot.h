// =============================================================================
// FILE:        DISDetonationSnapshot.h
// MODULE:      DIS Network Plugin — Interface
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Plain data struct for DIS Detonation PDU transmission.
//              Sent immediately when a weapon detonates (hit or miss).
//              Bridge fills this from Weapon::missileDetonated signal.
//              No Qt. No engine types. No DIS library types.
//
// MAPS TO:     DIS7::DetonationPdu (IEEE 1278.1-2012, PDU type 3)
// =============================================================================

#ifndef DISDETONATIONSNAPSHOT_H
#define DISDETONATIONSNAPSHOT_H

#include <string>
#include <cstdint>
#include "DISEntitySnapshot.h"

// =============================================================================
// DIS Detonation Result codes — IEEE 1278.1 Table B.7
// =============================================================================
namespace DISDetonationResult {
    constexpr uint8_t Other                    = 0;
    constexpr uint8_t EntityImpact             = 1;  // direct hit
    constexpr uint8_t EntityProximateDetonation= 2;  // proximity hit
    constexpr uint8_t GroundImpact             = 3;  // hit ground
    constexpr uint8_t GroundProximateDetonation= 4;  // near ground
    constexpr uint8_t Detonation               = 5;  // mid-air
    constexpr uint8_t None                     = 6;  // dud
    constexpr uint8_t HEHit                    = 7;
    constexpr uint8_t NonPenetratingHit        = 8;
    constexpr uint8_t Penetrating              = 12;
}

// =============================================================================
// DISDetonationSnapshot
// State at the moment a weapon detonates
//
// Bridge fills this from Weapon::missileDetonated signal:
//   firingEntityID ← weapon->parentEntity->ID (who fired it)
//   targetEntityID ← weapon->m_targetplatform->ID (what was hit)
//   munitionID     ← weapon->ID (the missile that detonated)
//   latitude       ← lat parameter from missileDetonated signal
//   longitude      ← lon parameter from missileDetonated signal
//   altitude       ← alt parameter from missileDetonated signal
//   result         ← determined by proximity check
//   blastRadius    ← weapon->blastRadius
//   eventID        ← must match the Fire PDU eventID for this shot
// =============================================================================
struct DISDetonationSnapshot {

    // ── Entity IDs ────────────────────────────────────────────────────────────
    std::string firingEntityID;  // platform that fired (empty if unknown)
    std::string targetEntityID;  // entity that was hit (empty if miss)
    std::string munitionID;      // weapon entity that detonated

    // ── Location of detonation ────────────────────────────────────────────────
    double latitude  = 0.0;  // degrees — from missileDetonated signal
    double longitude = 0.0;  // degrees — from missileDetonated signal
    double altitude  = 0.0;  // feet    — from missileDetonated signal

    // ── Velocity at detonation ────────────────────────────────────────────────
    float velNorth    = 0.0f;
    float velEast     = 0.0f;
    float velVertical = 0.0f;

    // ── Munition descriptor ───────────────────────────────────────────────────
    DISEntityTypeData munitionType;
    uint16_t warhead = 1000;  // HE
    uint16_t fuse    = 0200;  // contact

    // ── Result ────────────────────────────────────────────────────────────────
    // Set by bridge based on whether target was hit
    uint8_t detonationResult = DISDetonationResult::None;

    // ── Blast data ────────────────────────────────────────────────────────────
    // Used by receiving simulation to apply damage
    float blastRadius     = 0.0f;  // weapon->blastRadius (metres)
    float effectiveRadius = 0.0f;  // weapon->effectiveRadius (metres)
    float damage          = 0.0f;  // calculated damage 0.0-100.0

    // ── Event identifier ──────────────────────────────────────────────────────
    // Must match the eventID from the corresponding Fire PDU
    uint16_t eventID = 0;
};

#endif // DISDETONATIONSNAPSHOT_H