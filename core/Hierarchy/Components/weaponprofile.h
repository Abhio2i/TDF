/* ========================================================================= */
/* File: weaponprofile.h                                                     */
/* Purpose: Holds a named set of Weapon sub-objects for a Platform entity.   */
/*          Exact same pattern as sensorprofile.h / SensorProfile.           */
/* ========================================================================= */

#ifndef WEAPONPROFILE_H
#define WEAPONPROFILE_H

#include "./component.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"
#include <QObject>
#include <QJsonObject>

class Hierarchy;

class WeaponProfile : public QObject, public Component
{
    Q_OBJECT
public:
    WeaponProfile(Hierarchy* h);
    ComponentType Typo() const override { return ComponentType::WeaponProfile; }

    bool Active = true;
    std::unordered_map<std::string, Weapon*> *weapons;

    Weapon* getWeapon(const std::string& id) const;

    QJsonObject customParameters;

    // data1 = weapon type: "Missile" | "Bomb" | "Torpedo" |
    //                      "Artillery" | "Rocket" | "Flare" | "Chaff"
    // data2 = source weapon ID to copy config from (optional)
    // data3 = unused (reserved)
    void addSubComponent(std::string name,
                         QString data1 = "",
                         QString data2 = "",
                         QString data3 = "") override;

    void removeSubComponent(std::string ID)                             override;
    QJsonObject getsubComponentData(std::string ID)              const  override;
    void        updateSubComponent (std::string ID,
                            const QJsonObject& obj)             override;
    void        renameSubComponent (std::string ID, QString newName)    override;
    QJsonObject toJson()                                          const  override;
    void        fromJson(const QJsonObject& obj)                        override;

private:
    static void emitAutoComponentSignals(Hierarchy* parent, Weapon* weapon);
};

#endif // WEAPONPROFILE_H
