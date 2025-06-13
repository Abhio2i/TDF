
#ifndef PLATFORM_H
#define PLATFORM_H

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
    AttachedEnitities *radios = nullptr;
    AttachedEnitities *weopons = nullptr;
    AttachedEnitities *radar = nullptr;
    std::unordered_map<std::string, std::function<void()>> componentMap;
    std::unordered_map<std::string, Component> *components = nullptr;
    std::unordered_map<std::string, AttachedEnitities> *attachedEntities = nullptr;

    void spawn() override;
    std::vector<Component*> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // PLATFORM_H
