#ifndef CHAFF_H
#define CHAFF_H

#include "core/Hierarchy/EntityProfiles/weapon.h"

class Chaff : public Weapon
{
    Q_OBJECT
public:
    explicit Chaff(Hierarchy* h);

    WeaponType weaponType = WeaponType::Chaff;
    QString weaponTypeName() const override { return "Chaff"; }

    float dipoleCount          = 50000.0f;
    float radarFrequencyHz     = 9.5e9f;
    float bloomRCS             = 10000.0f;
    float bloomRadius          = 50.0f;
    float bloomGrowthRate      = 5.0f;
    float bloomDecayTime       = 60.0f;

    enum class ChaffMode { Seduction, Saturation };
    ChaffMode chaffMode          = ChaffMode::Seduction;
    float breakLockProbability   = 0.80f;
    float saturationClutterDb    = 30.0f;

    enum class DispensingMode { Single, Burst, Salvo, Auto };
    DispensingMode dispensingMode = DispensingMode::Burst;
    int   burstCount              = 2;
    float burstInterval           = 0.1f;
    int   cartridgesRemaining     = 60;
    float ejectionVelocity        = 15.0f;
    float ejectionAngle           = 90.0f;
    float ejectionSpread          = 20.0f;

    void        launch()                              override;
    bool        canEngage(Platform *target)           override;
    QJsonObject toJson()                        const override;
    void        fromJson(const QJsonObject&)          override;
    void Update() override;
    QString chaffModeToString()              const;
    QString dispensingModeToString()         const;
    void    setChaffModeFromString     (const QString&);
    void    setDispensingModeFromString(const QString&);
};

#endif // CHAFF_H
