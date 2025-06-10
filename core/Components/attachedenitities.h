#ifndef ATTACHEDENITITIES_H
#define ATTACHEDENITITIES_H
#include "core/Components/component.h"
#include <QObject>
#include <core/Struct/constants.h>
//#include <core/structure/entity.h>

class Entity;
class AttachedEnitities : public QObject, public Component
{

    Q_OBJECT
public:
    AttachedEnitities();
    ComponentType Typo() const override { return ComponentType::AttachedEnitities; }
    Constants::EntityType entity;
    std::unordered_map<std::string, Entity> *entities;

    QJsonObject toJson()const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // ATTACHEDENITITIES_H
