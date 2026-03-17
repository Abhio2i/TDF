#ifndef ROCKET_H
#define ROCKET_H

#include "core/Hierarchy/EntityProfiles/weapon.h"

class Rocket : public Weapon
{
    Q_OBJECT
public:
    explicit Rocket(Hierarchy* h);

    WeaponType weaponType = WeaponType::Rocket;
    QString weaponTypeName() const override { return "Rocket"; }

    PropulsionType propulsionType        = PropulsionType::SolidRocket;
    float thrustMain                     = 12000.0f;
    float burnTime                       = 1.1f;
    float specificImpulse                = 220.0f;
    float calibre                        = 70.0f;
    bool  finStabilised                  = true;
    float dragCoefficient                = 0.35f;
    float spinRateRPM                    = 0.0f;
    GuidanceType guidanceType            = GuidanceType::Unguided;
    bool         hasAdvancedPrecisionKit = false;
    float        cep                     = 5.0f;

    enum class WarheadType { HE, HEAT, Flechette, Illumination, Smoke, WP };
    WarheadType warheadVariant = WarheadType::HE;

    int   salvoCount    = 1;
    float salvoInterval = 0.05f;
    int   podCapacity   = 19;

    void        launch()                              override;
    void        flyToTarget()                         override;
    QJsonObject toJson()                        const override;
    void        fromJson(const QJsonObject&)          override;

    QString warheadTypeToString()            const;
    void    setWarheadTypeFromString (const QString&);
};

#endif // ROCKET_H
