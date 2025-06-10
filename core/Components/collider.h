#ifndef COLLIDER_H
#define COLLIDER_H

#include "core/Components/component.h"
#include <QObject>
#include <QJsonObject>
#include <core/Struct/vector.h>
#include <core/Struct/constants.h>

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

    QJsonObject toJson()const override;
    void fromJson(const QJsonObject &obj) override;
signals:
};

#endif // COLLIDER_H
