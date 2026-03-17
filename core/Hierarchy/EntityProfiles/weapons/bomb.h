#ifndef BOMB_H
#define BOMB_H

#include "core/Hierarchy/EntityProfiles/weapon.h"

class Bomb : public Weapon
{
    Q_OBJECT
public:
    explicit Bomb(Hierarchy* h);

    WeaponType weaponType = WeaponType::Bomb;
    QString weaponTypeName() const override { return "Bomb"; }

    GuidanceType guidanceType    = GuidanceType::Unguided;
    bool         hasPrecisionKit = false;
    float        cep             = 30.0f;

    enum class ReleaseMode { CCIP, CCRP, Manual, Lofting };
    ReleaseMode releaseMode    = ReleaseMode::CCIP;
    float dragCoefficient      = 0.4f;
    float terminalVelocity     = 350.0f;
    float releaseAltitude      = 0.0f;
    bool  hasDelayFuze         = false;
    float fuzeDelaySeconds     = 0.0f;

    void        launch()                              override;
    void        flyToTarget()                         override;
    void        checkDetonation()                     override;
    QJsonObject toJson()                        const override;
    void        fromJson(const QJsonObject&)          override;

    QString releaseModeToString()             const;
    QString guidanceTypeToString()            const;
    void    setReleaseModeFromString  (const QString&);
    void    setGuidanceTypeFromString (const QString&);
};

#endif // BOMB_H
