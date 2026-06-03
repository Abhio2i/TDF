// =============================================================================
// FILE:        DISFireSnapshot.h
// MODULE:      DIS Network Plugin — Interface
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Plain data struct for DIS Fire PDU transmission.
//              Sent immediately when a weapon is fired.
//              Bridge fills this from Weapon::missileStart() or launch().
//              No Qt. No engine types. No DIS library types.
//
// MAPS TO:     DIS7::FirePdu (IEEE 1278.1-2012, PDU type 2)
// NOTE:        STAGE receives Fire PDUs (Send=Yes, Receive=No in their config)
//              meaning STAGE sends Fire PDUs to us but does not listen for ours.
//              We still send them for other DIS participants on the network.
// =============================================================================

#ifndef DISFIRESNAPSHOT_H
#define DISFIRESNAPSHOT_H

#include <string>
#include <cstdint>
#include "DISEntitySnapshot.h"  // for DISEntityTypeData

// =============================================================================
// DISFireSnapshot
// State at the moment a weapon is fired
//
// Bridge fills this from:
//   firingEntityID ← Platform ID that fired the weapon
//   targetEntityID ← target Platform ID (from weapon->m_targetplatform->ID)
//   munitionID     ← Weapon entity ID
//   latitude       ← firing platform transform->geocord->latitude
//   longitude      ← firing platform transform->geocord->latitude
//   altitude       ← firing platform transform->geocord->altitude
//   velocity       ← initial missile velocity from dynamicModel
//   munitionType   ← weapon DIS type (Missile, Torpedo etc)
//   warheadType    ← from weapon->warheadType
//   fuseType       ← from weapon->detonationType
//   quantity       ← always 1 for now
//   firingRate     ← rounds per minute, 0 for guided munitions
//   range          ← weapon->maxRange
// =============================================================================
struct DISFireSnapshot {

    // ── Entity IDs ────────────────────────────────────────────────────────────
    std::string firingEntityID;  // platform that fired (owns the fire event)
    std::string targetEntityID;  // intended target (can be empty if no target)
    std::string munitionID;      // weapon entity ID (missile/torpedo/etc)

    // ── Location of shot ──────────────────────────────────────────────────────
    // Position where the weapon left the firing entity
    double latitude  = 0.0;  // degrees
    double longitude = 0.0;  // degrees
    double altitude  = 0.0;  // feet

    // ── Initial velocity of munition ──────────────────────────────────────────
    float velNorth    = 0.0f;  // m/s north component
    float velEast     = 0.0f;  // m/s east component
    float velVertical = 0.0f;  // m/s vertical component

    // ── Munition descriptor ───────────────────────────────────────────────────
    DISEntityTypeData munitionType;  // DIS type of the munition fired
    uint16_t warhead  = 1000;        // DIS warhead type enum (1000=HE)
    uint16_t fuse     = 0200;        // DIS fuse type enum   (0200=contact)
    uint16_t quantity = 1;           // number of rounds
    uint16_t rate     = 0;           // rounds per minute (0 = single shot)

    // ── Range ────────────────────────────────────────────────────────────────
    float range = 0.0f;  // metres, weapon->maxRange

    // ── Event identifier ─────────────────────────────────────────────────────
    // Used to correlate Fire PDU with subsequent Detonation PDU
    // Set automatically by PDUSerializer using a counter
    uint16_t eventID = 0;
};

#endif // DISFIRESNAPSHOT_H