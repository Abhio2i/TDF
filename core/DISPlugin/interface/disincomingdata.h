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
#include <QMetaType>
Q_DECLARE_METATYPE(DISIncomingTransform)
Q_DECLARE_METATYPE(DISIncomingFire)
Q_DECLARE_METATYPE(DISIncomingDetonation)
Q_DECLARE_METATYPE(DISIncomingExercise)
Q_DECLARE_METATYPE(DISIncomingIFF)

#endif // DISINCOMINGDATA_H
