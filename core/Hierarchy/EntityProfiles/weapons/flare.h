#ifndef FLARE_H
#define FLARE_H

#include "core/Hierarchy/EntityProfiles/weapon.h"

class Flare : public Weapon
{
    Q_OBJECT
public:
    explicit Flare(Hierarchy* h);

    WeaponType weaponType = WeaponType::Flare;
    QString weaponTypeName() const override { return "Flare"; }

    float burnTemperature      = 2000.0f;
    float burnDuration         = 5.0f;
    float peakIrradiance       = 1500.0f;
    float spectralMatch        = 0.95f;
    float ejectionVelocity     = 15.0f;
    float ejectionAngle        = 45.0f;
    float ejectionSpread       = 10.0f;
    void Update() override;
    enum class DispensingMode { Single, Burst, Salvo, Auto };
    DispensingMode dispensingMode  = DispensingMode::Burst;
    int   burstCount               = 2;
    float burstInterval            = 0.1f;
    int   cartridgesRemaining      = 60;
    float breakLockProbability     = 0.75f;

    void        launch()                              override;
    bool        canEngage(Platform *target)           override;
    QJsonObject toJson()                        const override;
    void        fromJson(const QJsonObject&)          override;

    QString dispensingModeToString()          const;
    void    setDispensingModeFromString(const QString&);
};

#endif // FLARE_H
