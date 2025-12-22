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

    // std::string entityReferenceId;  // Store the ID for saving/loading

    void formationCreate();
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    // void resolveEntityReferences();

    // PASTE THESE 3 LINES HERE:
    QString formationTypeToString(Constants::FormationType type) const;
    Constants::FormationType stringToFormationType(QString str) const;
    QStringList formationTypeOptions() const;

private:  // Add this private section
    void resolveEntityReference(FormationPosition* position, const QJsonObject& obj);

};

#endif // FORMATION_H
