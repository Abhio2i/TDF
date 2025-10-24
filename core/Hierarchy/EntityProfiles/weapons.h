

#ifndef WEAPONS_H
#define WEAPONS_H

#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <string>
#include <vector>

class Missile : public Entity
{
    Q_OBJECT
public:
    explicit Missile(Hierarchy* h);

    // Missile attributes
    enum class GuidingSensor { ActiveRadar, Infrared, PassiveRadar, GPS, Optical };
    enum class LaunchPlatformType { Air, Ground, Sea };
    enum class ControlSystem { Canard, ThrustVectoring, Fins };

    struct LocationOffset {
        float x = 0.0f; // meters
        float y = 0.0f; // meters
        float z = 0.0f; // meters
    };

    LocationOffset locationOffset;
    float thrust = 0.0f; // kN
    float burnTime = 0.0f; // seconds
    float maxSpeed = 0.0f; // Mach
    float acceleration = 0.0f; // m/s²
    float payload = 0.0f; // kg
    float proximityRange = 0.0f; // meters
    float range = 0.0f; // km
    GuidingSensor guidingSensor = GuidingSensor::ActiveRadar;
    float seekerLockOnTime = 0.0f; // seconds
    bool eccmCapability = false;
    LaunchPlatformType launchPlatformType = LaunchPlatformType::Air;
    ControlSystem controlSystem = ControlSystem::Fins;

    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

private:
    QString guidingSensorToString(GuidingSensor gs) const;
    GuidingSensor stringToGuidingSensor(const QString& str) const;
    QString launchPlatformTypeToString(LaunchPlatformType lpt) const;
    LaunchPlatformType stringToLaunchPlatformType(const QString& str) const;
    QString controlSystemToString(ControlSystem cs) const;
    ControlSystem stringToControlSystem(const QString& str) const;
};

class Gun : public Entity
{
    Q_OBJECT
public:
    explicit Gun(Hierarchy* h);

    // Gun attributes
    float muzzleSpeed = 0.0f; // m/s
    float maximumRate = 0.0f; // rounds per minute
    float barrelLength = 0.0f; // meters
    float caliber = 0.0f; // mm
    int magazineCapacity = 0;
    float effectiveRange = 0.0f; // meters
    bool burstMode = false;
    float reloadTime = 0.0f; // seconds
    float recoilForce = 0.0f; // kN

    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
};

class Bomb : public Entity
{
    Q_OBJECT
public:
    explicit Bomb(Hierarchy* h);

    // Bomb attributes
    enum class GuidingSensor { ActiveRadar, Infrared };
    enum class FuseType { Contact, Airburst, Proximity };

    struct LocationOffset {
        float x = 0.0f; // meters
        float y = 0.0f; // meters
        float z = 0.0f; // meters
    };

    LocationOffset locationOffset;
    float payload = 0.0f; // kg
    float proximityRange = 0.0f; // meters
    GuidingSensor guidingSensor = GuidingSensor::Infrared;
    float fallTime = 0.0f; // seconds
    float armingDelay = 0.0f; // seconds
    FuseType fuseType = FuseType::Contact;
    bool glideCapability = false;

    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

private:
    QString guidingSensorToString(GuidingSensor gs) const;
    GuidingSensor stringToGuidingSensor(const QString& str) const;
    QString fuseTypeToString(FuseType ft) const;
    FuseType stringToFuseType(const QString& str) const;
};

#endif // WEAPONS_H
