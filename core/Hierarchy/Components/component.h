#ifndef COMPONENT_H
#define COMPONENT_H

#include "qjsonobject.h"
#include <core/Utility/uuid.h>
enum class ComponentType {
    Unknown,
    Transform,
    Rigidbody,
    NetworkObject,
    Mission,
    MeshRenderer2D,
    DynamicModel,
    Collider,
    Trajectory,
    AttachedEnitities,
    CrossSection,
    SensorProfile,
    IFFProfile,
    RadioProfile,
    WeaponProfile

    // aur bhi component types
};
class Hierarchy;
class Entity;
class Component
{
public:
    Component(Hierarchy* h);
    ~Component();
    std::string ID = Uuid::generateShortUniqueID();
    std::string parentID;
    Entity* parentEntity = nullptr;
    virtual ComponentType Typo() const { return ComponentType::Unknown; }

    // 🔧 Add these two pure virtual functions
    virtual void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) = 0;
    virtual void removeSubComponent(std::string ID) = 0;
    virtual QJsonObject getsubComponentData(std::string ID) const = 0;
    virtual void updateSubComponent(std::string ID, const QJsonObject& obj) = 0;
    virtual void renameSubComponent(std::string ID, QString newName) {}
    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject& obj) = 0;

};

#endif // COMPONENT_H


