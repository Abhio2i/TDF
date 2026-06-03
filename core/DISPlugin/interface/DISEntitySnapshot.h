// =============================================================================
// FILE:        DISEntitySnapshot.h
// MODULE:      DIS Network Plugin — Interface
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Plain data struct representing one entity's complete state
//              for DIS EntityState PDU transmission.
//              Engine fills this from its internal data.
//              PDUSerializer converts this to standard DIS bytes.
//              No Qt. No engine types. No DIS library types.
//
// MAPS TO:     DIS7::EntityStatePdu (IEEE 1278.1-2012)
// =============================================================================

#ifndef DISENTITYSNAPSHOT_H
#define DISENTITYSNAPSHOT_H

#include <string>
#include <cstdint>

// =============================================================================
// DIS Domain constants — matches IEEE 1278.1 enumerations
// =============================================================================
namespace DISDomain {
    constexpr uint8_t Other       = 0;
    constexpr uint8_t Land        = 1;
    constexpr uint8_t Air         = 2;
    constexpr uint8_t Surface     = 3;  // naval surface
    constexpr uint8_t Subsurface  = 4;  // submarine
    constexpr uint8_t Space       = 5;
}

// =============================================================================
// DIS Kind constants
// =============================================================================
namespace DISKind {
    constexpr uint8_t Other       = 0;
    constexpr uint8_t Platform    = 1;  // vehicles, aircraft, ships
    constexpr uint8_t Munition    = 2;  // missiles, bombs, torpedoes
    constexpr uint8_t LifeForm    = 3;  // infantry
    constexpr uint8_t Environmental= 4;
    constexpr uint8_t Sensor      = 5;
    constexpr uint8_t Expendable  = 8;  // chaff, flare
}

// =============================================================================
// DIS ForceID constants — matches STAGE team mapping
// =============================================================================
namespace DISForceID {
    constexpr uint8_t Other     = 0;
    constexpr uint8_t Friendly  = 1;  // BlueTeam
    constexpr uint8_t Opposing  = 2;  // RedTeam
    constexpr uint8_t Neutral   = 3;  // GreyTeam
    constexpr uint8_t Friendly2 = 4;
    constexpr uint8_t Opposing2 = 5;
    constexpr uint8_t Neutral2  = 6;
}

// =============================================================================
// DIS DeadReckoning algorithm codes
// =============================================================================
namespace DISDeadReckoning {
    constexpr uint8_t Static         = 1; // no movement
    constexpr uint8_t DRM_FPW        = 2; // fixed velocity world
    constexpr uint8_t DRM_RPW        = 3; // fixed velocity + rotation world
    constexpr uint8_t DRM_RVW        = 4; // velocity + acceleration + rotation
    constexpr uint8_t DRM_FVW        = 5; // velocity + acceleration
}

// =============================================================================
// DISEntityTypeData
// Maps your Entity::Category to DIS type enumeration
// Configurable per entity via dis_config.json
// Mirrors STAGE Database Editor DIS Type fields exactly
// =============================================================================
struct DISEntityTypeData {
    uint8_t  kind        = DISKind::Platform;
    uint8_t  domain      = DISDomain::Land;
    uint16_t country     = 71;   // 71 = India (SISO-REF-010)
    uint8_t  category    = 1;
    uint8_t  subcategory = 0;
    uint8_t  specific    = 0;
    uint8_t  extra       = 0;
};

// =============================================================================
// DISEntitySnapshot
// Complete state of one entity for EntityState PDU transmission
//
// Bridge fills this from:
//   entityID     ← entity->ID
//   forceID      ← teamToForceID(entity->team)
//   entityType   ← from dis_config entity type mapping
//   latitude     ← transform->geocord->latitude
//   longitude    ← transform->geocord->longitude
//   altitude     ← transform->geocord->altitude (feet)
//   heading      ← transform->geocord->Heading
//   pitch/roll   ← dynamicModel->pitch / dynamicModel->roll
//   northVel     ← dynamicModel->NorthVelocity
//   eastVel      ← dynamicModel->EastVelocity
//   verticalVel  ← dynamicModel->VerticalVelocity
//   health       ← entity->Health → appearance bits
//   marking      ← entity->Name (first 11 chars, DIS standard)
// =============================================================================
struct DISEntitySnapshot {

    // ── Identity ──────────────────────────────────────────────────────────────
    std::string entityID;       // your MongoDB short UUID
    std::string parentID;       // parent entity ID (for weapons)
    bool        active = true;  // false = send destroyed appearance then remove

    // ── DIS Entity Type ───────────────────────────────────────────────────────
    DISEntityTypeData entityType;
    uint8_t           forceID = DISForceID::Friendly;

    // ── Position (geocoordinate) ──────────────────────────────────────────────
    // CoordConverter converts these to ECEF for DIS wire format
    double latitude  = 0.0;  // degrees, from transform->geocord->latitude
    double longitude = 0.0;  // degrees, from transform->geocord->longitude
    double altitude  = 0.0;  // FEET,    from transform->geocord->altitude

    // ── Orientation ───────────────────────────────────────────────────────────
    float heading = 0.0f;    // degrees yaw,   from geocord->Heading
    float pitch   = 0.0f;    // degrees pitch, from dynamicModel->pitch
    float roll    = 0.0f;    // degrees roll,  from dynamicModel->roll

    // ── Velocity ──────────────────────────────────────────────────────────────
    // From dynamicModel — already in m/s
    float northVel    = 0.0f;  // dynamicModel->NorthVelocity
    float eastVel     = 0.0f;  // dynamicModel->EastVelocity
    float verticalVel = 0.0f;  // dynamicModel->VerticalVelocity

    // ── Dead Reckoning ────────────────────────────────────────────────────────
    uint8_t deadReckoningAlgorithm = DISDeadReckoning::DRM_FPW;

    // ── State ─────────────────────────────────────────────────────────────────
    float    health     = 100.0f;   // entity->Health, converted to appearance bits
    uint32_t appearance = 0;        // DIS appearance record bits

    // ── Marking ───────────────────────────────────────────────────────────────
    std::string marking;            // entity->Name truncated to 11 chars
                                    // shown in STAGE scenario tree

    // ── Munition specific ─────────────────────────────────────────────────────
    bool isMunition = false;        // true for Weapon entities
};

#endif // DISENTITYSNAPSHOT_H