
#ifndef COLLIDER_H
#define COLLIDER_H

#include "./component.h"
#include <QObject>
#include <QJsonObject>
#include <core/Hierarchy/Struct/vector.h>
#include <core/Hierarchy/Struct/constants.h>
class Hierarchy;
class Collider : public QObject, public Component
{
    Q_OBJECT
public:
    Collider(Hierarchy* h);
    ComponentType Typo() const override { return ComponentType::Collider; }
    bool Active;
    float CollideRadius;
    float WarningRadius;
    float Width;
    float Length;
    float Height;

    Vector *vector;
    Constants::EntityType type;
    Constants::ColliderType collider;

    // Add a map to store custom parameters
    QJsonObject customParameters; // Store custom parameters as key-value pairs
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QString data3 = "") override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
signals:

public slots:
    void Update(float deltaTime = 0.01f);
};

#endif // COLLIDER_H
