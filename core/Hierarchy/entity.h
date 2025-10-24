
#ifndef ENTITY_H
#define ENTITY_H
#include "core/Hierarchy/Components/attachedenitities.h"
#include <QObject>
#include <QString>
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
// #include "core/Hierarchy/EntityProfiles/iff.h"
// #include "core/Hierarchy/EntityProfiles/radio.h"
// #include "core/Hierarchy/EntityProfiles/sensor.h"
class Hierarchy;
class AttachedEnitities;
class Radio;
class Sensor;
class IFF;
class Entity: public QObject
{
    Q_OBJECT

public:
    Entity(Hierarchy* h);
    ~Entity();
    std::string Name;
    bool Active;
    std::string ID;
    std::string parentID;
    Constants::EntityType type;
    std::unordered_map<std::string, std::shared_ptr<Parameter>> parameters;
    std::vector<Radio*> radioList;
    std::vector<Sensor*> sensorList;
    std::vector<IFF*> iffList;
    void addRadio(Radio* radio);
    void removeRadio(Radio* radio);
    void clearRadios();
    void addSensor(Sensor* sensor);
    void removeSensor(Sensor* sensor);
    void clearSensors();
    void addIFF(IFF* iff);
    void removeIFF(IFF* iff);
    void clearIFFs();
    virtual void spawn() = 0;
    virtual std::vector<std::string> getSupportedComponents() = 0;
    virtual void addComponent(std::string name) = 0;
    virtual void removeComponent(std::string name) = 0;
    virtual QJsonObject getComponent(std::string name) = 0;
    virtual void updateComponent(QString name, const QJsonObject& obj) = 0;

    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject& obj) = 0;

};

#endif // ENTITY_H
