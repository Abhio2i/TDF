
#ifndef COLLIDER_H
#define COLLIDER_H

#include "./component.h"
#include <QObject>
#include <QJsonObject>
#include <core/Hierarchy/Struct/vector.h>
#include <core/Hierarchy/Struct/constants.h>

class Collider : public QObject, public Component
{
    Q_OBJECT
public:
    Collider();
    ComponentType Typo() const override { return ComponentType::Collider; }
    bool Active;
    float Radius;
    float Width;
    float Length;
    float Height;

    Vector *vector;
    Constants::EntityType type;
    Constants::ColliderType collider;

    // Add a map to store custom parameters
    QJsonObject customParameters; // Store custom parameters as key-value pairs

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
signals:
};

#endif // COLLIDER_H
