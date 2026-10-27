#ifndef DISINCOMINGDATA_H
#define DISINCOMINGDATA_H

#include <string>
#include <cstdint>

struct DISIncomingTransform {
    std::string entityID;
    double      latitude  = 0.0;
    double      longitude = 0.0;
    double      altitude  = 0.0;
    float       heading   = 0.0f;
    float       pitch     = 0.0f;
    float       roll      = 0.0f;
    float       northVel  = 0.0f;
    float       eastVel   = 0.0f;
    float       vertVel   = 0.0f;
    bool        active    = true;
    uint8_t     domain    = 1;
    uint8_t     forceID   = 1;
    uint8_t     category    = 1;     // ← ADD
    uint8_t     subcategory = 0;
    std::string marking;
};

struct DISIncomingFire {
    std::string firingEntityID;
    std::string targetEntityID;
    std::string munitionID;
    double      latitude  = 0.0;
    double      longitude = 0.0;
    double      altitude  = 0.0;
};

struct DISIncomingDetonation {
    std::string firingEntityID;
    std::string targetEntityID;
    std::string munitionID;
    double      latitude         = 0.0;
    double      longitude        = 0.0;
    double      altitude         = 0.0;
    uint8_t     detonationResult = 0;
    float       blastRadius      = 0.0f;
};

struct DISIncomingExercise {
    enum Type { StartResume, StopFreeze } type;
    uint8_t stopReason = 0;
};
// ── Incoming IFF PDU data ────────────────────────────────────────────────────
struct DISIncomingIFF {
    std::string emittingEntityID;
    bool        systemOn    = true;
    uint16_t    mode1Code   = 0;
    uint16_t    mode2Code   = 0;
    uint16_t    mode3ACode  = 0;
    bool        mode4Active = false;
    uint8_t     forceID     = 1;
};
// =============================================================================
// DISIncomingAcknowledge
// Received when a remote machine acknowledges our CreateEntity,
// RemoveEntity, StartResume, or StopFreeze PDU.
// Future: matched against pending request tracking map.
// =============================================================================
struct DISIncomingAcknowledge {
    std::string originatingEntityID; // engine ID of sender (empty if unknown)
    uint16_t    acknowledgeFlag = 0; // DISAcknowledgeFlag::* — which PDU was ACKd
    uint16_t    responseFlag    = 0; // DISResponseFlag::AbleToComply etc
    uint32_t    requestID       = 0; // matches our original request ID
};

// =============================================================================
// DISIncomingTransferOwnership
// Received when another machine transfers an entity's ownership to us.
// TransferOwnershipHandler sends Acknowledge automatically before emitting this.
// DISNetworkPlugin takes ownership on the main thread in applyTransferOwnershipEvents().
// =============================================================================
struct DISIncomingTransferOwnership {
    std::string transferEntityID;       // engine ID (resolved from mapper)
    uint16_t    transferEntitySite   = 0;
    uint16_t    transferEntityApp    = 0;
    uint16_t    transferEntityEntity = 0;
    uint16_t    originSite           = 0;
    uint16_t    originApp            = 0;
    uint16_t    originEntity         = 0;
    uint8_t     transferType         = 0;
    uint32_t    requestID            = 0;
};
#include <QMetaType>
Q_DECLARE_METATYPE(DISIncomingTransform)
Q_DECLARE_METATYPE(DISIncomingFire)
Q_DECLARE_METATYPE(DISIncomingDetonation)
Q_DECLARE_METATYPE(DISIncomingExercise)
Q_DECLARE_METATYPE(DISIncomingIFF)
Q_DECLARE_METATYPE(DISIncomingAcknowledge)
Q_DECLARE_METATYPE(DISIncomingTransferOwnership)
#endif // DISINCOMINGDATA_H
