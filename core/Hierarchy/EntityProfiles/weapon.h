
// =============================================================================
// FILE:        weapon.h
// MODULE:      Tactical Simulation Ordnance Systems
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the Weapon abstract base class. This module serves as
//               the foundation for all ordnance types (Missiles, Bombs, etc.),
//               managing targeting logic, flight monitoring, and detonation
//               parameters within the tactical simulation.
//
// AUTHOR:       Waris
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Dec 2025  Integrated WeaponTarget and flight check timers.
//   Rev 3  Mar 2026  Added support for dynamic target platform tracking.
//   Rev 4  Apr 2026  Aligned with DO-178C documentation standards for O2I.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================
#ifndef WEAPON_H
#define WEAPON_H

#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QElapsedTimer>
#include <string>
#include <vector>
#include <unordered_set>

// ── Auto-integrated component headers ────────────────────────────────────────
#include <core/Hierarchy/Components/transform.h>
#include <core/Hierarchy/Components/rigidbody.h>
#include <core/Hierarchy/Components/collider.h>
#include <core/Hierarchy/Components/trajectory.h>
#include <core/Hierarchy/Components/meshrenderer2d.h>
#include <core/Hierarchy/Components/dynamicmodel.h>
#include <core/Hierarchy/Components/crosssection.h>

class Platform;
class CanvasWidget;

// ── WeaponTarget struct (same as Sensor's Target struct) ─────────────────────
struct WeaponTarget {
    Platform   *entity    = nullptr;
    std::string name;
    float distance    = 0.0f;
    float bearing     = 0.0f;
    float elevation   = 0.0f;
    float closureRate = 0.0f;
    float aspect      = 0.0f;
    bool  inGimbalLock = false;
};

class Weapon : public Entity
{
    Q_OBJECT
public:
    explicit Weapon(Hierarchy* h);
    ~Weapon() override;

    // ── WeaponType enum (same role as Sensor::SubType) ────────────────────────
    enum class WeaponType     { Missile, Bomb, Torpedo, Artillery, Rocket, Flare, Chaff };
    enum class GuidanceType   { Unguided, SemiActive, FullyActive, PassiveInfrared,
                              CommandGuided, InertialGuidance };
    enum class DetonationType { Impact, Proximity, Timed, Command };
    enum class PropulsionType { SolidRocket, LiquidRocket, Turbofan, Ramjet, Turboprop, Gravity };

    // ── Type tag (overridden by each subclass, same role as Sensor::subType) ──
    WeaponType weaponType = WeaponType::Missile;
    virtual QString weaponTypeName() const { return "Missile"; }

    // ── Ownership ─────────────────────────────────────────────────────────────
    Entity *parentEntity = nullptr;

    // =========================================================================
    // AUTO-INTEGRATED COMPONENTS  (created in constructor, same as Platform)
    // =========================================================================
    Transform      *transform      = nullptr;
    Rigidbody      *rigidbody      = nullptr;
    Collider       *collider       = nullptr;
    Trajectory     *trajectory     = nullptr;
    MeshRenderer2D *meshRenderer2d = nullptr;
    DynamicModel   *dynamicModel   = nullptr;
    CrossSection   *crossSection   = nullptr;

    // ── Shared physical fields ────────────────────────────────────────────────
    std::string designation;
    float length        = 3.7f;
    float diameter      = 0.178f;
    float totalMass     = 45.0f;
    float payloadMass   = 20.0f;
    float fuelMass      = 15.0f;

    // ── Shared performance fields ─────────────────────────────────────────────
    float maxVelocity   = 2500.0f;
    float minVelocity   = 100.0f;
    float maxRange      = 100000.0f;
    float minRange      = 500.0f;
    float maxAltitude   = 20000.0f;
    float minAltitude   = 0.0f;
    float maximumG      = 25.0f;
    float flightTimeMax = 600.0f;

    // ── Shared detonation fields ──────────────────────────────────────────────
    DetonationType detonationType = DetonationType::Proximity;
    float proximityRange = 100.0f;
    float timerDelay     = 5.0f;

    // ── Shared warhead fields ─────────────────────────────────────────────────
    float blastRadius     = 200.0f;
    float effectiveRadius = 500.0f;
    float peakPressure    = 500.0f;
    std::string warheadType;

    // ── Shared launch / safety fields ─────────────────────────────────────────
    float launchG            = 5.0f;
    float preflightCheckTime = 30.0f;
    int   rearmTime          = 600;
    bool  armed              = false;
    bool  safed              = true;

    // ── Countermeasures (Flare / Chaff use these) ─────────────────────────────
    float flareDispensed = 0;
    float chaffDispensed = 0;
    bool  chainsawMode   = false;

    // ── Target tracking ───────────────────────────────────────────────────────
    std::unordered_set<Platform*> detects;
    QVector<WeaponTarget>         targets;
    Platform *targetEntity = nullptr;

    // ── Virtual launch() — same role as Sensor::scan() ───────────────────────
    // weapon.cpp has an empty stub.
    // Each subclass (Missile, Bomb, etc.) overrides with its own behaviour.
    virtual void launch();

    // ── Other shared virtual behaviours ──────────────────────────────────────
    virtual void  flyToTarget();
    virtual void  updateGuidance();
    virtual void  checkDetonation();
    virtual void  scan();
    virtual bool  canEngage(Platform *target);
    virtual float calculateImpactTime(Platform *target);
    virtual float calculateLaunchPoint();
    void          disarmWeapon();
    void          rearmWeapon();
    void          clearTargets();

    // ── Entity interface overrides ────────────────────────────────────────────
    void spawn()                                               override;
    std::vector<std::string> getSupportedComponents()          override;
    void addComponent   (std::string name)                     override;
    void removeComponent(std::string name)                     override;
    QJsonObject getComponent(std::string name)                 override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    // ── Serialisation ─────────────────────────────────────────────────────────
    // Subclass calls Weapon::toJson() first, then appends its own fields.
    // Subclass calls Weapon::fromJson() first, then reads its own fields.
    QJsonObject toJson()              const override;
    void        fromJson(const QJsonObject&)  override;

    // ── Shared enum helpers ───────────────────────────────────────────────────
    QString detonationTypeToString()              const;
    void    setDetonationTypeFromString(const QString&);

    // ── Static helper (same pattern as Sensor::getSubTypeFromString) ─────────
    static WeaponType getWeaponTypeFromString(const QString& str) {
        if (str == "Missile")   return WeaponType::Missile;
        if (str == "Bomb")      return WeaponType::Bomb;
        if (str == "Torpedo")   return WeaponType::Torpedo;
        if (str == "Artillery") return WeaponType::Artillery;
        if (str == "Rocket")    return WeaponType::Rocket;
        if (str == "Flare")     return WeaponType::Flare;
        if (str == "Chaff")     return WeaponType::Chaff;
        return WeaponType::Missile;
    }
    QString weaponTypeToString()              const;
    QString weaponTypeNameFromEnum(WeaponType t) const;

    // ── Sync base fields into sub-components ─────────────────────────────────
    void syncComponentsFromWeaponData();

    // ── Flight monitor state (Missile uses these; other types ignore them) ────
    bool  m_flightActive      = false;
    bool  m_updateRunning     = false;
    float m_distanceTravelled = 0.0f;
    float m_flightTime        = 0.0f;
    float m_launchHeading     = 0.0f;
    bool  isLaunched          = false;
    bool  isDead              = false;

    double m_targetLat  = 28.6358;
    double m_targetLon  = 77.2244;
    Transform *m_target = nullptr;
    Platform  *m_targetplatform = nullptr;
    float      m_detonationRange = 5000.0f;
    CanvasWidget *m_canvas = nullptr;

    QTimer       *m_flightCheckTimer = nullptr;
    QTimer       *m_updateTimer      = nullptr;
    QElapsedTimer m_dtClock;

    void setCanvas(CanvasWidget* canvas) { m_canvas = canvas; }
    void setTarget(double lat, double lon, float detonationRangeMetres = 5000.0f);
    void setTarget(Transform *tr, float detonationRangeMetres);
    double getTargetLat()       const { return m_targetLat; }
    double getTargetLon()       const { return m_targetLon; }
    float  getDetonationRange() const { return m_detonationRange; }

    void startFlightMonitor();
    void stopFlightMonitor();
    void pauseFlightMonitor();   // pauses timer without resetting state
    void resumeFlightMonitor();  // restarts timer from where it left off

    // ── Virtual flight hooks (empty stubs here, Missile overrides all 5) ──────
    virtual void      missileStart();
    virtual void      checkFlightState();
    virtual void      missileUpdate(float deltaTime);
    virtual QVector3D calcTargetVector() const;
    virtual void      missileEnd();

signals:
    void missileDetonated(const QString& weaponID,
                          double lat, double lon, double alt);
};

#endif // WEAPON_H
