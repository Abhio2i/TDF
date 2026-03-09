// #ifndef WEAPON_H
// #define WEAPON_H

// #include "core/Hierarchy/EntityProfiles/platform.h"
// #include <core/Hierarchy/entity.h>
// #include <QObject>
// #include <QJsonObject>
// #include <QJsonArray>
// #include <string>
// #include <vector>
// #include <unordered_set>

// class CanvasWidget;
// class Weapon : public Entity
// {
//     Q_OBJECT
// public:
//     explicit Weapon(Hierarchy* h);

//     // Weapon types
//     enum class WeaponType { Missile, Bomb, Torpedo, Artillery, Rocket, Flare, Chaff };
//     enum class GuidanceType { Unguided, SemiActive, FullyActive, PassiveInfrared, CommandGuided, InertialGuidance };
//     enum class DetonationType { Impact, Proximity, Timed, Command };
//     enum class PropulsionType { SolidRocket, LiquidRocket, Turbofan, Ramjet, Turboprop, Gravity };

//     Entity* parentEntity = nullptr;
//     WeaponType weaponType = WeaponType::Missile;

//     // Basic weapon specifications (Specifications section)
//     std::string designation;        // e.g., "AIM-120C", "AGM-65D"
//     float length = 3.7f;            // meters
//     float diameter = 0.178f;        // meters
//     float totalMass = 45.0f;        // kg
//     float payloadMass = 20.0f;      // kg
//     float fuelMass = 15.0f;         // kg

//     // Performance characteristics (Performance section)
//     float maxVelocity = 2500.0f;    // m/s
//     float minVelocity = 100.0f;     // m/s
//     float maxRange = 100000.0f;     // meters
//     float minRange = 500.0f;        // meters
//     float maxAltitude = 20000.0f;   // meters
//     float minAltitude = 0.0f;       // meters
//     float maximumG = 25.0f;         // G-force capability
//     float flightTimeMax = 600.0f;   // seconds

//     // Guidance system (Guidance section)
//     GuidanceType guidanceType = GuidanceType::FullyActive;
//     float seekerRange = 100000.0f;  // meters
//     float seekerFOV = 45.0f;        // degrees
//     float lockOnRange = 50000.0f;   // meters
//     bool isLocked = false;
//     Platform* targetEntity = nullptr;

//     // Detonation characteristics (Detonation section)
//     DetonationType detonationType = DetonationType::Proximity;
//     float proximityRange = 100.0f;  // meters
//     float timerDelay = 5.0f;        // seconds

//     // Warhead specifications (Warhead section)
//     float blastRadius = 200.0f;     // meters
//     float effectiveRadius = 500.0f; // meters
//     float peakPressure = 500.0f;    // kPa
//     std::string warheadType;        // "HE", "Fragmentation", etc.

//     // Propulsion system (Propulsion section)
//     PropulsionType propulsionType = PropulsionType::SolidRocket;
//     float thrustMain = 200000.0f;   // Newton
//     float thrustBooster = 50000.0f; // Newton
//     float burnTime = 45.0f;         // seconds
//     float specificImpulse = 280.0f; // seconds

//     // Launch constraints & Safety (Launch section)
//     float launchG = 5.0f;           // G-forces
//     float preflightCheckTime = 30.0f; // seconds
//     int rearmTime = 600;            // seconds
//     bool armed = false;
//     bool safed = true;

//     // Seeker/Tracking (Tracking section)
//     float seekerTrackingRate = 30.0f; // degrees/second
//     float seekerLockAccuracy = 0.5f;  // degrees
//     bool isActive = false;

//     // Evasion/Counter-measures
//     float flareDispensed = 0;
//     float chaffDispensed = 0;
//     bool chainsawMode = false;

//     // Target tracking structure
//     struct WeaponTarget {
//         Platform* entity;
//         std::string name;
//         float distance = 0.0f;      // meters
//         float bearing = 0.0f;       // degrees
//         float elevation = 0.0f;     // degrees
//         float closureRate = 0.0f;   // m/s
//         float aspect = 0.0f;        // degrees
//         bool inGimbalLock = false;
//     };

//     // Weapon behavior methods
//     void launch();
//     void flyToTarget();
//     void updateGuidance();
//     void checkDetonation();
//     void disarmWeapon();
//     void rearmWeapon();
//     void scan();
//     bool canEngage(Platform* target);
//     float calculateImpactTime(Platform* target);
//     float calculateLaunchPoint();

//     std::unordered_set<Platform*> detects;
//     QVector<WeaponTarget> targets;

//     // Override entity functions
//     void spawn() override;
//     std::vector<std::string> getSupportedComponents() override;
//     void addComponent(std::string name) override;
//     void removeComponent(std::string name) override;
//     QJsonObject getComponent(std::string name) override;
//     void updateComponent(QString name, const QJsonObject& obj) override;

//     // Serialization
//     QJsonObject toJson() const override;
//     void fromJson(const QJsonObject& obj) override;
// };

// #endif // WEAPON_H
#ifndef WEAPON_H
#define WEAPON_H

#include "core/Hierarchy/EntityProfiles/platform.h"
#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <string>
#include <vector>
#include <unordered_set>

class CanvasWidget;
class Weapon : public Entity
{
    Q_OBJECT
public:
    explicit Weapon(Hierarchy* h);

    // Weapon types
    enum class WeaponType { Missile, Bomb, Torpedo, Artillery, Rocket, Flare, Chaff };
    enum class GuidanceType { Unguided, SemiActive, FullyActive, PassiveInfrared, CommandGuided, InertialGuidance };
    enum class DetonationType { Impact, Proximity, Timed, Command };
    enum class PropulsionType { SolidRocket, LiquidRocket, Turbofan, Ramjet, Turboprop, Gravity };

    Entity* parentEntity = nullptr;
    WeaponType weaponType = WeaponType::Missile;

    // Basic weapon specifications (Specifications section)
    std::string designation;        // e.g., "AIM-120C", "AGM-65D"
    float length = 3.7f;            // meters
    float diameter = 0.178f;        // meters
    float totalMass = 45.0f;        // kg
    float payloadMass = 20.0f;      // kg
    float fuelMass = 15.0f;         // kg

    // Performance characteristics (Performance section)
    float maxVelocity = 2500.0f;    // m/s
    float minVelocity = 100.0f;     // m/s
    float maxRange = 100000.0f;     // meters
    float minRange = 500.0f;        // meters
    float maxAltitude = 20000.0f;   // meters
    float minAltitude = 0.0f;       // meters
    float maximumG = 25.0f;         // G-force capability
    float flightTimeMax = 600.0f;   // seconds

    // Guidance system (Guidance section)
    GuidanceType guidanceType = GuidanceType::FullyActive;
    float seekerRange = 100000.0f;  // meters
    float seekerFOV = 45.0f;        // degrees
    float lockOnRange = 50000.0f;   // meters
    bool isLocked = false;
    Platform* targetEntity = nullptr;

    // Detonation characteristics (Detonation section)
    DetonationType detonationType = DetonationType::Proximity;
    float proximityRange = 100.0f;  // meters
    float timerDelay = 5.0f;        // seconds

    // Warhead specifications (Warhead section)
    float blastRadius = 200.0f;     // meters
    float effectiveRadius = 500.0f; // meters
    float peakPressure = 500.0f;    // kPa
    std::string warheadType;        // "HE", "Fragmentation", etc.

    // Propulsion system (Propulsion section)
    PropulsionType propulsionType = PropulsionType::SolidRocket;
    float thrustMain = 200000.0f;   // Newton
    float thrustBooster = 50000.0f; // Newton
    float burnTime = 45.0f;         // seconds
    float specificImpulse = 280.0f; // seconds

    // Launch constraints & Safety (Launch section)
    float launchG = 5.0f;           // G-forces
    float preflightCheckTime = 30.0f; // seconds
    int rearmTime = 600;            // seconds
    bool armed = false;
    bool safed = true;

    // Seeker/Tracking (Tracking section)
    float seekerTrackingRate = 30.0f; // degrees/second
    float seekerLockAccuracy = 0.5f;  // degrees
    bool isActive = false;

    // Evasion/Counter-measures
    float flareDispensed = 0;
    float chaffDispensed = 0;
    bool chainsawMode = false;

    // Target tracking structure
    struct WeaponTarget {
        Platform* entity;
        std::string name;
        float distance = 0.0f;      // meters
        float bearing = 0.0f;       // degrees
        float elevation = 0.0f;     // degrees
        float closureRate = 0.0f;   // m/s
        float aspect = 0.0f;        // degrees
        bool inGimbalLock = false;
    };

    // Weapon behavior methods
    void launch();
    void flyToTarget();
    void updateGuidance();
    void checkDetonation();
    void disarmWeapon();
    void rearmWeapon();
    void scan();
    bool canEngage(Platform* target);
    float calculateImpactTime(Platform* target);
    float calculateLaunchPoint();

    std::unordered_set<Platform*> detects;
    QVector<WeaponTarget> targets;

    // Override entity functions
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

    // ✅ TYPE-SPECIFIC PARAMETER HELPERS
    QString guidanceTypeToString() const;
    QString propulsionTypeToString() const;
    QString detonationTypeToString() const;

    void setGuidanceTypeFromString(const QString& str);
    void setPropulsionTypeFromString(const QString& str);
    void setDetonationTypeFromString(const QString& str);
};

#endif // WEAPON_H
