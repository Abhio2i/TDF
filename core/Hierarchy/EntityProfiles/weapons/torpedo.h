#ifndef TORPEDO_H
#define TORPEDO_H

#include "core/Hierarchy/EntityProfiles/weapon.h"

class Torpedo : public Weapon
{
    Q_OBJECT
public:
    explicit Torpedo(Hierarchy* h);

    WeaponType weaponType = WeaponType::Torpedo;
    QString weaponTypeName() const override { return "Torpedo"; }

    GuidanceType   guidanceType   = GuidanceType::SemiActive;
    PropulsionType propulsionType = PropulsionType::LiquidRocket;
    void Update() override;
    enum class HomingMode { Passive, Active, WireGuided, WakeHoming };
    HomingMode homingMode = HomingMode::Active;
    bool  isWireGuided    = false;
    float wireLength      = 20000.0f;
    float thrustMain      = 50000.0f;
    float burnTime        = 600.0f;
    float specificImpulse = 100.0f;
    float runDepth        = 50.0f;
    float maxDepth        = 900.0f;
    float searchPatternRadius = 1000.0f;

    enum class TargetMode { AntiSurface, AntiSubmarine, DualMode };
    TargetMode targetMode = TargetMode::DualMode;
    float seekerRange     = 5000.0f;
    float seekerFOV       = 60.0f;
    bool  isLocked        = false;

    void        launch()                              override;
    void        updateGuidance()                      override;
    void        scan()                                override;
    bool        canEngage(Platform *target)           override;
    QJsonObject toJson()                        const override;
    void        fromJson(const QJsonObject&)          override;

    QString homingModeToString()               const;
    QString targetModeToString()               const;
    void    setHomingModeFromString   (const QString&);
    void    setTargetModeFromString   (const QString&);
};

#endif // TORPEDO_H
