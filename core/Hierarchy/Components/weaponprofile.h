#ifndef WEAPONPROFILE_H
#define WEAPONPROFILE_H

#include "./component.h"
#include <QObject>
#include <QJsonObject>

class Hierarchy;
class Weapon;

class WeaponProfile: public QObject, public Component
{
    Q_OBJECT
public:
    WeaponProfile(Hierarchy* h);
    ComponentType Typo() const override { return ComponentType::WeaponProfile; }

    bool Active;
    std::unordered_map<std::string, Weapon*> *weapons;

    // Store custom parameters as key-value pairs
    QJsonObject customParameters;

    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QString data3 = "") override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
    void renameSubComponent(std::string ID, QString newName) override;

};

#endif // WEAPONPROFILE_H
