// #ifndef BOMB_H
// #define BOMB_H

// #include "core/Hierarchy/EntityProfiles/weapon.h"

// // 300 ft in metres
// static constexpr float   DROP_ALTITUDE_M   = 304.8f;   // 1000 ft
// static constexpr int     BLAST_DISPLAY_MS  = 1500;
// static const QString BOMB_SPRITE_PATH  = ":/texture/images/Texture/bomb.png";
// static const QString BLAST_SPRITE_PATH = ":/texture/images/Texture/blast.png";

// class Bomb : public Weapon
// {
//     Q_OBJECT
// public:
//     explicit Bomb(Hierarchy* h);

//     WeaponType weaponType = WeaponType::Bomb;
//     QString weaponTypeName() const override { return "Bomb"; }

//     GuidanceType guidanceType    = GuidanceType::Unguided;
//     bool         hasPrecisionKit = false;
//     float        cep             = 30.0f;

//     enum class ReleaseMode { CCIP, CCRP, Manual, Lofting };
//     ReleaseMode releaseMode    = ReleaseMode::CCIP;
//     float dragCoefficient      = 0.02f;
//     float terminalVelocity     = 350.0f;
//     float releaseAltitude      = 0.0f;
//     bool  hasDelayFuze         = false;
//     float fuzeDelaySeconds     = 0.0f;

//     // ── All public functions named bomb* ──────────────────────────────────────
//     void launch()          override;   // entry: called by Platform::launchBombs()
//     void bombStart();                  // starts 100ms flight-monitor timer
//     void bombUpdate(float dt);         // physics every 100ms
//     void bombEnd();                    // impact: blast sprite → detonated signal
//     void checkDetonation() override;   // fuse check at end of bombUpdate
//     void flyToTarget()     override;   // no-op unguided

//     // Only ONE Weapon virtual override — redirects timer tick to bombUpdate.
//     // missileStart / missileUpdate / missileEnd are NOT overridden on Bomb.
//     void checkFlightState() override;

//     QJsonObject toJson()                     const override;
//     void        fromJson(const QJsonObject&)       override;

//     QString releaseModeToString()              const;
//     QString guidanceTypeToString()             const;
//     void    setReleaseModeFromString (const QString&);
//     void    setGuidanceTypeFromString(const QString&);

// private:
//     float   m_verticalVelocity   = 0.0f;
//     float   m_horizontalVelocity = 0.0f;
//     float   m_fuzeTimer          = 0.0f;
//     float   m_airborneTime       = 0.0f;
//     QTimer* m_blastTimer         = nullptr;
// };

// #endif // BOMB_H
#ifndef BOMB_H
#define BOMB_H

#include "core/Hierarchy/EntityProfiles/weapon.h"

// Altitude threshold at which a bomb is auto-released.
// geocord->altitude is stored in FEET, so this constant must also be in FEET.
static constexpr float   DROP_ALTITUDE_FT  = 1000.0f;  // 1000 ft ≈ 304.8 m
static constexpr int     BLAST_DISPLAY_MS  = 1500;
static const QString BOMB_SPRITE_PATH  = ":/texture/images/Texture/bomb.png";
static const QString BLAST_SPRITE_PATH = ":/texture/images/Texture/blast.png";

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
    float dragCoefficient      = 0.02f;
    float terminalVelocity     = 350.0f;
    float releaseAltitude      = 0.0f;
    bool  hasDelayFuze         = false;
    float fuzeDelaySeconds     = 0.0f;

    // ── All public functions named bomb* ──────────────────────────────────────
    void launch()          override;   // entry: called by Platform::launchBombs()
    void bombStart();                  // starts 100ms flight-monitor timer
    void bombUpdate(float dt);         // physics every 100ms
    void bombEnd();                    // impact: blast sprite → detonated signal
    void checkDetonation() override;   // fuse check at end of bombUpdate
    void flyToTarget()     override;   // no-op unguided
    void Update() override;
    // Only ONE Weapon virtual override — redirects timer tick to bombUpdate.
    // missileStart / missileUpdate / missileEnd are NOT overridden on Bomb.
    void checkFlightState() override;

    QJsonObject toJson()                     const override;
    void        fromJson(const QJsonObject&)       override;

    QString releaseModeToString()              const;
    QString guidanceTypeToString()             const;
    void    setReleaseModeFromString (const QString&);
    void    setGuidanceTypeFromString(const QString&);

private:
    float   m_verticalVelocity   = 0.0f;
    float   m_horizontalVelocity = 0.0f;
    float   m_fuzeTimer          = 0.0f;
    float   m_airborneTime       = 0.0f;
    QTimer* m_blastTimer         = nullptr;
};

#endif // BOMB_H
