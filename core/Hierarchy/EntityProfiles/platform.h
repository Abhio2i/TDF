
#ifndef PLATFORM_H
#define PLATFORM_H

// #include "core/Hierarchy/EntityProfiles/iff.h"
// #include "core/Hierarchy/EntityProfiles/radio.h"
// #include "core/Hierarchy/EntityProfiles/sensor.h"
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
    float width;
    float height;

    ///Components
    Transform *transform = nullptr;
    Trajectory *trajectory = nullptr;
    Rigidbody *rigidbody = nullptr;
    DynamicModel *dynamicModel = nullptr;
    Collider *collider = nullptr;
    NetworkObject *networkObject = nullptr;
    MeshRenderer2D *meshRenderer2d = nullptr;
    Mission *mission = nullptr;
    // Pointers to multiple Radios, Sensors, IFFs

    std::unordered_map<std::string, std::function<void()>> componentMap;
    std::unordered_map<std::string, Component> *components = nullptr;


    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // PLATFORM_H
