#ifndef FORMATION_H
#define FORMATION_H

#include "core/Hierarchy/Struct/formationposition.h"
#include "core/Hierarchy/entity.h"

class Formation: public Entity
{
    Q_OBJECT
public:
    Formation(Hierarchy* h);

    Constants::FormationType formationType;
    int count;
    FormationPosition *mothership;
    std::unordered_map<std::string,FormationPosition*> *formationPositions;

    void formationCreate();
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // FORMATION_H
