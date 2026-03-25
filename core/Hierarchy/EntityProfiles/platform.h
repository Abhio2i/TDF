#ifndef PLATFORM_H
#define PLATFORM_H

// #include "core/Hierarchy/EntityProfiles/iff.h"
// #include "core/Hierarchy/EntityProfiles/radio.h"
// #include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/Components/crosssection.h"
#include "core/Hierarchy/Components/iffprofile.h"
#include "core/Hierarchy/Components/radioprofile.h"
#include "core/Hierarchy/Components/sensorprofile.h"
#include "core/Hierarchy/Components/weaponprofile.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"
#include "mission/taskgroup.h"
#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <core/Hierarchy/Components/transform.h>
#include <core/Hierarchy/Components/trajectory.h>
#include <core/Hierarchy/Components/rigidbody.h>
#include <core/Hierarchy/Components/collider.h>
#include <core/Hierarchy/Components/networkobject.h>
#include <core/Hierarchy/Components/meshrenderer2d.h>
#include <core/Hierarchy/Components/component.h>
#include <core/Hierarchy/Components/dynamicmodel.h>
#include <core/Hierarchy/Components/mission.h>
#include <core/Hierarchy/Components/attachedenitities.h>
#include <vector>


class Platform: public Entity
{
    Q_OBJECT
public:
    Platform(Hierarchy* h);
    ~Platform();
    Hierarchy* m_hierarchy = nullptr;
    float width;
    float height;
    enum MissionType{
        PATROL,
        SURVEILLANCE,
        INTERCEPTION,
        STRIKE,
        ESCORT,
        AREA_DENIAL,
        SEARCH_AND_RESCUE,
        BLOCKADE,
        RECONNAISSANCE,
        DEFENSIVE_HOLD
    };
    const std::string MissionTypeNames[10] = { "PATROL",
                                              "SURVEILLANCE",
                                              "INTERCEPTION",
                                              "STRIKE",
                                              "ESCORT",
                                              "AREA_DENIAL",
                                              "SEARCH_AND_RESCUE",
                                              "BLOCKADE",
                                              "RECONNAISSANCE",
                                              "DEFENSIVE_HOLD"};
    enum ROI{
        HOLD_FIRE,
        RETURN_FIRE_ONLY,
        DEFENSIVE_ONLY,
        FIRE_ON_DETECTION,
        FIRE_ON_IDENTIFICATION,
        FREE_FIRE,
        COMMAND_AUTHORIZATION_REQUIRED,
    };
    const std::string ROINames[7] = { "HOLD_FIRE",
                                     "RETURN_FIRE_ONLY",
                                     "DEFENSIVE_ONLY",
                                     "FIRE_ON_DETECTION",
                                     "FIRE_ON_IDENTIFICATION",
                                     "FREE_FIRE",
                                     "COMMAND_AUTHORIZATION_REQUIRED"};
    enum Engagement{
        NEAREST_TARGET,
        HIGHEST_THREAT,
        LOWEST_HEALTH_TARGET,
        ASSIGNED_TARGET_ONLY,
        HIGH_VALUE_TARGET,
        GROUP_ENGAGEMENT,
        SEQUENTIAL_ENGAGEMENT
    };
    const std::string EngagementNames[7] = { "NEAREST_TARGET",
                                            "HIGHEST_THREAT",
                                            "LOWEST_HEALTH_TARGET",
                                            "ASSIGNED_TARGET_ONLY",
                                            "HIGH_VALUE_TARGET",
                                            "GROUP_ENGAGEMENT",
                                            "SEQUENTIAL_ENGAGEMENT"};
    enum Retreat{
        NEVER_RETREAT,
        RETREAT_IF_OUTNUMBERED,
        RETREAT_IF_DAMAGE_EXCEEDS_THRESHOLD,
        RETREAT_IF_FUEL_LOW,
        RETREAT_IF_AMMO_DEPLETED,
        RETREAT_IF_COMMAND_ORDERED,
        TACTICAL_WITHDRAWAL
    };
    const std::string RetreatNames[7] = { "NEVER_RETREAT",
                                         "RETREAT_IF_OUTNUMBERED",
                                         "RETREAT_IF_DAMAGE_EXCEEDS_THRESHOLD",
                                         "RETREAT_IF_FUEL_LOW",
                                         "RETREAT_IF_AMMO_DEPLETED",
                                         "RETREAT_IF_COMMAND_ORDERED",
                                         "TACTICAL_WITHDRAWAL"};
    enum Detection{
        PASSIVE_SENSORS_ONLY,
        ACTIVE_RADAR_ALLOWED,
        FULL_SENSOR_USAGE,
        STEALTH_MODE,
        EMCON_PASSIVE,
        INTERMITTENT_RADAR
    };
    const std::string DetectionNames[6] = { "PASSIVE_SENSORS_ONLY",
                                           "ACTIVE_RADAR_ALLOWED",
                                           "FULL_SENSOR_USAGE",
                                           "STEALTH_MODE",
                                           "EMCON_PASSIVE",
                                           "INTERMITTENT_RADAR"};
    enum WeaponRelease{
        AUTOMATIC,
        SEMI_AUTOMATIC,
        COMMAND_APPROVAL_REQUIRED,
        WEAPON_FREE,
        WEAPON_TIGHT,
        WEAPON_HOLD
    };
    const std::string WeaponReleaseNames[6] = { "AUTOMATIC",
                                               "SEMI_AUTOMATIC",
                                               "COMMAND_APPROVAL_REQUIRED",
                                               "WEAPON_FREE",
                                               "WEAPON_TIGHT",
                                               "WEAPON_HOLD"};
    MissionType mtype = MissionType::PATROL;
    ROI roi = ROI::DEFENSIVE_ONLY;
    Engagement engagement = Engagement::ASSIGNED_TARGET_ONLY;
    Retreat retreat = Retreat::NEVER_RETREAT;
    Detection detection = Detection::FULL_SENSOR_USAGE;
    WeaponRelease weaponrelease = WeaponRelease::WEAPON_HOLD;
    float healthThreshold = 20;
    float fuelthreshold = 20;
    float engagementRange = 30;
    ///Components
    Transform *transform = nullptr;
    Trajectory *trajectory = nullptr;
    Rigidbody *rigidbody = nullptr;
    DynamicModel *dynamicModel = nullptr;
    Collider *collider = nullptr;
    NetworkObject *networkObject = nullptr;
    MeshRenderer2D *meshRenderer2d = nullptr;
    Mission *mission = nullptr;
    TaskGroup* taskgroup;
    CrossSection *crossSection = nullptr;
    SensorProfile *sensors = nullptr;
    RadioProfile *radios = nullptr;
    WeaponProfile *weapons = nullptr;
    IFFProfile *iffs = nullptr;
    // Pointers to multiple Radios, Sensors, IFFs
    Sensor* getSensorByName(const std::string& name) const;
    Radio* getRadioByName(const std::string& name) const;
    IFF* getIFFByName(const std::string& name) const;
    std::unordered_map<std::string, std::function<void()>> componentMap;
    std::unordered_map<std::string, Component> *components = nullptr;

    void addParam(std::string key,std::string value);
    void editParam(std::string key,std::string value);
    std::string getParam(std::string key);
    void removeParam(std::string key);

    void Start();
    void update();
    void Decision();
    void spawn() override;

    // ── Bomb altitude trigger ─────────────────────────────────────────────────
    // Called from update() when aircraft first crosses DROP_ALTITUDE_M (300 ft).
    // Iterates WeaponProfile, calls Bomb::launch() on every unlaunched Bomb.
    void launchBombs();
    void fireMissile();

    bool m_bombsReleased = false;  // guard: launchBombs() fires only once per flight
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;
    QJsonObject customParameters; // Added to store custom parameters
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    // Helper methods
    Weapon* getWeaponByName(const std::string& name) const;

public slots:
    void start();
    void reset();
    void pause();
};

#endif // PLATFORM_H
