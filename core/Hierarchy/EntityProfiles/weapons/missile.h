/* ========================================================================= */
/* File: WeaponTypes/missile.h                                               */
/* Purpose: Guided missile — follows esm.h / csm.h pattern exactly.         */
/* ========================================================================= */

#ifndef MISSILE_H
#define MISSILE_H

#include "core/Hierarchy/EntityProfiles/weapon.h"

class Missile : public Weapon
{
    Q_OBJECT
public:
    explicit Missile(Hierarchy* h);

    WeaponType weaponType = WeaponType::Missile;
    QString weaponTypeName() const override { return "Missile"; }

    // ── Propulsion ────────────────────────────────────────────────────────────
    PropulsionType propulsionType = PropulsionType::SolidRocket;
    float thrustMain      = 200000.0f;
    float thrustBooster   = 50000.0f;
    float burnTime        = 45.0f;
    float specificImpulse = 280.0f;

    // ── Guidance / Seeker ─────────────────────────────────────────────────────
    GuidanceType guidanceType  = GuidanceType::FullyActive;
    float seekerRange          = 100000.0f;
    float seekerFOV            = 45.0f;
    float lockOnRange          = 50000.0f;
    float seekerTrackingRate   = 30.0f;
    float seekerLockAccuracy   = 0.5f;
    bool  isLocked             = false;
    bool  isActive             = false;


    // ── Overrides (same pattern as ESM::scan()) ───────────────────────────────
    void        launch()                              override;  // flight behaviour
    void        updateGuidance()                      override;
    bool        canEngage(Platform *target)           override;
    QJsonObject toJson()                        const override;
    void        fromJson(const QJsonObject&)          override;
    void Update() override;
    // ── Flight system (5 functions, all override base empty stubs) ───────────
    void      missileStart()                 override;
    void      checkFlightState()             override;
    void      missileUpdate(float deltaTime) override;
    QVector3D calcTargetVector()       const override;
    void      missileEnd()                   override;

    // ── Enum helpers ──────────────────────────────────────────────────────────
    QString guidanceTypeToString()              const;
    QString propulsionTypeToString()            const;
    void    setGuidanceTypeFromString  (const QString&);
    void    setPropulsionTypeFromString(const QString&);
};

#endif // MISSILE_H
