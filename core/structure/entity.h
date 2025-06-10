#ifndef ENTITY_H
#define ENTITY_H
#include "core/Components/attachedenitities.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <core/Components/transform.h>
#include <core/Components/trajectory.h>
#include <core/Components/rigidbody.h>
#include <core/Components/collider.h>
#include <core/Components/networkobject.h>
#include <core/Components/meshrenderer2d.h>
#include <core/Components/component.h>
#include <core/Components/dynamicmodel.h>
#include <core/Components/mission.h>
//#include <core/Components/attachedenitities.h>

class Hierarchy;
class AttachedEnitities;
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


    ///Components
    Transform *transform = nullptr;
    Trajectory *trajectory = nullptr;
    Rigidbody *rigidbody = nullptr;
    DynamicModel *dynamicModel = nullptr;
    Collider *collider = nullptr;
    NetworkObject *networkObject = nullptr;
    MeshRenderer2D *meshRenderer2d = nullptr;
    Mission *mission;
    AttachedEnitities *radios = nullptr;
    AttachedEnitities *weopons = nullptr;
    AttachedEnitities *radar = nullptr;
    std::unordered_map<std::string, std::function<void()>> componentMap;
    std::unordered_map<std::string, Component> *components;
    std::unordered_map<std::string, AttachedEnitities> *attachedEntities;

    void spawn();
    std::vector<Component*> getSupportedComponents();
    void addComponent(std::string name);
    void removeComponent(std::string name);
    QJsonObject getComponent(std::string);
    void updateComponent(QString name,const QJsonObject& obj);

    QJsonObject toJson();
    void fromJson(const QJsonObject& obj);

};

#endif // ENTITY_H
