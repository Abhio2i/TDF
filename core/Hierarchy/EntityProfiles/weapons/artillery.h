#ifndef ARTILLERY_H
#define ARTILLERY_H

#include "core/Hierarchy/EntityProfiles/weapon.h"

class Artillery : public Weapon
{
    Q_OBJECT
public:
    explicit Artillery(Hierarchy* h);

    WeaponType weaponType = WeaponType::Artillery;
    QString weaponTypeName() const override { return "Artillery"; }

    float muzzleVelocity   = 827.0f;
    float calibre          = 155.0f;
    float barrelLength     = 6.0f;
    int   propellantCharge = 8;
    float launchAngle      = 45.0f;
    float dragCoefficient  = 0.295f;
    bool  spinStabilised   = true;
    void Update() override;
    enum class ShellType { HE, HEAT, HESH, Illumination, Smoke, Cluster, Excalibur, DPICM };
    ShellType shellType = ShellType::HE;

    GuidanceType guidanceType   = GuidanceType::Unguided;
    bool         hasGpsGuidance = false;
    float        cep            = 50.0f;

    bool  mrsiEnabled        = false;
    int   mrsiRounds         = 5;
    float mrsiFiringInterval = 2.5f;

    enum class FuzeType { PointDetonating, Delay, ProximityAirburst, TimedAirburst };
    FuzeType fuzeType       = FuzeType::PointDetonating;
    float    airburstHeight = 10.0f;

    void        launch()                              override;
    void        flyToTarget()                         override;
    void        checkDetonation()                     override;
    float       calculateImpactTime(Platform *target) override;
    QJsonObject toJson()                        const override;
    void        fromJson(const QJsonObject&)          override;

    QString shellTypeToString()             const;
    QString fuzeTypeToString()              const;
    void    setShellTypeFromString  (const QString&);
    void    setFuzeTypeFromString   (const QString&);
};

#endif // ARTILLERY_H
