#ifndef COMPONENT_H
#define COMPONENT_H

#include "qjsonobject.h"
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
    AttachedEnitities

    // aur bhi component types
};
class Component
{
public:
    Component();
    ~Component();
    virtual ComponentType Typo() const { return ComponentType::Unknown; }

    // 🔧 Add these two pure virtual functions
    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject& obj) = 0;
};

#endif // COMPONENT_H


