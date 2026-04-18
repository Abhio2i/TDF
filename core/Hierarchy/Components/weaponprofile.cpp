/* ========================================================================= */
/* File: weaponprofile.cpp                                                   */
/* Purpose: Exact same pattern as sensorprofile.cpp.                         */
/*          data1 = type string → picks correct subclass (like data1=="CSM") */
/*          fromJson reads "weaponTypeName" key → picks correct subclass     */
/* ========================================================================= */

#include "weaponprofile.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/weapons/missile.h"
#include "core/Hierarchy/EntityProfiles/weapons/bomb.h"
#include "core/Hierarchy/EntityProfiles/weapons/torpedo.h"
#include "core/Hierarchy/EntityProfiles/weapons/artillery.h"
#include "core/Hierarchy/EntityProfiles/weapons/rocket.h"
#include "core/Hierarchy/EntityProfiles/weapons/flare.h"
#include "core/Hierarchy/EntityProfiles/weapons/chaff.h"
#include <core/Hierarchy/hierarchy.h>

// =============================================================================
// Constructor
// =============================================================================
WeaponProfile::WeaponProfile(Hierarchy* h) : Component(h)
{
    Active  = true;
    weapons = new std::unordered_map<std::string, Weapon*>();
}

// =============================================================================
// addSubComponent
// Exact same pattern as SensorProfile::addSubComponent:
// =============================================================================
void WeaponProfile::addSubComponent(std::string name,
                                    QString data1,
                                    QString data2,
                                    QJsonObject /*data3*/)
{
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);

    Weapon* weapon = nullptr;

    if (data1 == "Missile") {
        Missile* m = new Missile(parent);
        if (!data2.isEmpty()) {
            auto it = parent->Weapons.find(data2.toStdString());
            if (it != parent->Weapons.end() && it->second) {
                std::string id = m->ID;
                m->fromJson(it->second->toJson());
                m->ID       = id;
                m->parentID = parentID;
            }
        }
        weapon = m;
    } else if (data1 == "Bomb") {
        Bomb* b = new Bomb(parent);
        if (!data2.isEmpty()) {
            auto it = parent->Weapons.find(data2.toStdString());
            if (it != parent->Weapons.end() && it->second) {
                std::string id = b->ID;
                b->fromJson(it->second->toJson());
                b->ID       = id;
                b->parentID = parentID;
            }
        }
        weapon = b;
    } else if (data1 == "Torpedo") {
        Torpedo* t = new Torpedo(parent);
        if (!data2.isEmpty()) {
            auto it = parent->Weapons.find(data2.toStdString());
            if (it != parent->Weapons.end() && it->second) {
                std::string id = t->ID;
                t->fromJson(it->second->toJson());
                t->ID       = id;
                t->parentID = parentID;
            }
        }
        weapon = t;
    } else if (data1 == "Artillery") {
        Artillery* a = new Artillery(parent);
        if (!data2.isEmpty()) {
            auto it = parent->Weapons.find(data2.toStdString());
            if (it != parent->Weapons.end() && it->second) {
                std::string id = a->ID;
                a->fromJson(it->second->toJson());
                a->ID       = id;
                a->parentID = parentID;
            }
        }
        weapon = a;
    } else if (data1 == "Rocket") {
        Rocket* r = new Rocket(parent);
        if (!data2.isEmpty()) {
            auto it = parent->Weapons.find(data2.toStdString());
            if (it != parent->Weapons.end() && it->second) {
                std::string id = r->ID;
                r->fromJson(it->second->toJson());
                r->ID       = id;
                r->parentID = parentID;
            }
        }
        weapon = r;
    } else if (data1 == "Flare") {
        Flare* f = new Flare(parent);
        if (!data2.isEmpty()) {
            auto it = parent->Weapons.find(data2.toStdString());
            if (it != parent->Weapons.end() && it->second) {
                std::string id = f->ID;
                f->fromJson(it->second->toJson());
                f->ID       = id;
                f->parentID = parentID;
            }
        }
        weapon = f;
    } else if (data1 == "Chaff") {
        Chaff* c = new Chaff(parent);
        if (!data2.isEmpty()) {
            auto it = parent->Weapons.find(data2.toStdString());
            if (it != parent->Weapons.end() && it->second) {
                std::string id = c->ID;
                c->fromJson(it->second->toJson());
                c->ID       = id;
                c->parentID = parentID;
            }
        }
        weapon = c;
    } else {
        // Default fallback — same as SensorProfile defaulting to Radar
        weapon = new Missile(parent);
    }

    weapon->parentEntity = parentEntity;
    weapon->Name         = name;
    weapon->parentID     = parentID;

    weapons->insert({weapon->ID, weapon});
    parent->Weapons.insert({weapon->ID, weapon});

    emit parent->subComponentAdded(QString::fromStdString(ID),
                                   QString::fromStdString(weapon->ID),
                                   QString::fromStdString(name));

    // Register in Entities so Inspector's getComponentData() can find it
    parent->Entities.insert({weapon->ID, weapon});

    // Emit componentAdded for all 7 auto-integrated components
    emitAutoComponentSignals(parent, weapon);
}

// =============================================================================
// emitAutoComponentSignals  — private helper
// =============================================================================
void WeaponProfile::emitAutoComponentSignals(Hierarchy* parent, Weapon* weapon)
{
    QString wID = QString::fromStdString(weapon->ID);
    struct { const char* name; std::string id; } comps[] = {
                 { "transform",    weapon->transform      ? weapon->transform->ID      : "" },
                 { "rigidbody",    weapon->rigidbody      ? weapon->rigidbody->ID      : "" },
                 { "collider",     weapon->collider       ? weapon->collider->ID       : "" },
                 { "trajectory",   weapon->trajectory     ? weapon->trajectory->ID     : "" },
                 { "bitmap",       weapon->meshRenderer2d ? weapon->meshRenderer2d->ID : "" },
                 { "dynamicModel", weapon->dynamicModel   ? weapon->dynamicModel->ID   : "" },
                 { "crossSection", weapon->crossSection   ? weapon->crossSection->ID   : "" },
                 };
    for (const auto& c : comps)
        if (!c.id.empty())
            emit parent->componentAdded(wID,
                                        QString::fromStdString(c.id),
                                        QString::fromLatin1(c.name));
}

// =============================================================================
// removeSubComponent  — identical to SensorProfile::removeSubComponent
// =============================================================================
void WeaponProfile::removeSubComponent(std::string ID)
{
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    auto it = weapons->find(ID);
    if (it != weapons->end() && it->second) {
        Weapon* weapon = it->second;
        emit parent->subComponentRemoved(QString::fromStdString(this->ID),
                                         QString::fromStdString(ID),
                                         QString::fromStdString(weapon->Name));
        weapons->erase(weapon->ID);
        parent->Weapons.erase(weapon->ID);
        delete weapon;
    }
}

// =============================================================================
// updateSubComponent  — identical to SensorProfile::updateSubComponent
// =============================================================================
void WeaponProfile::updateSubComponent(std::string ID, const QJsonObject& obj)
{
    auto it = weapons->find(ID);
    if (it != weapons->end() && it->second)
        it->second->fromJson(obj);
}

// =============================================================================
// getsubComponentData  — identical to SensorProfile::getsubComponentData
// =============================================================================
QJsonObject WeaponProfile::getsubComponentData(std::string ID) const
{
    auto it = weapons->find(ID);
    if (it != weapons->end() && it->second)
        return it->second->toJson();
    return QJsonObject();
}

// =============================================================================
// toJson  — identical structure to SensorProfile::toJson
// =============================================================================
QJsonObject WeaponProfile::toJson() const
{
    QJsonObject obj;
    obj["id"]     = QString::fromStdString(ID);
    obj["active"] = Active;

    QJsonObject weaponObj;
    for (const auto& [key, weaponPtr] : *weapons)
        if (weaponPtr)
            weaponObj[QString::fromStdString(key)] = weaponPtr->toJson();
    // Each subclass toJson() writes "weaponTypeName" → used in fromJson

    obj["weapons"] = weaponObj;
    return obj;
}

// =============================================================================
// fromJson  — reads "weaponTypeName" key to pick correct subclass
// Exact same pattern as SensorProfile::fromJson reading "SensorType" key:
// =============================================================================
void WeaponProfile::fromJson(const QJsonObject& obj)
{
    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (!obj.contains("weapons") || !obj["weapons"].isObject())
        return;

    QJsonObject weaponsObj = obj["weapons"].toObject();
    Hierarchy*  parent     = GlobalRegistry::getParentHierarchy(this);

    for (const QString& key : weaponsObj.keys()) {
        QJsonObject weaponObj = weaponsObj[key].toObject();
        QString     type      = weaponObj.value("weaponTypeName").toString();
        std::string id        = weaponObj["id"].toString().toStdString();

        Weapon* weapon  = nullptr;
        bool    exists  = weapons->count(id) > 0;

        if (exists) {
            weapon = (*weapons)[id];
        } else if (type == "Missile") {
            weapon = new Missile(parent);
        } else if (type == "Bomb") {
            weapon = new Bomb(parent);
        } else if (type == "Torpedo") {
            weapon = new Torpedo(parent);
        } else if (type == "Artillery") {
            weapon = new Artillery(parent);
        } else if (type == "Rocket") {
            weapon = new Rocket(parent);
        } else if (type == "Flare") {
            weapon = new Flare(parent);
        } else if (type == "Chaff") {
            weapon = new Chaff(parent);
        } else {
            weapon = new Missile(parent);  // default fallback
        }

        weapon->parentEntity = parentEntity;
        weapon->Name         = weaponObj["name"].toString().toStdString();
        weapon->ID           = weaponObj["id"].toString().toStdString();
        weapon->fromJson(weaponObj);

        if (!exists) {
            weapon->parentID = parentID;
            weapons->insert({weapon->ID, weapon});
            parent->Weapons.insert({weapon->ID, weapon});
            emit parent->subComponentAdded(QString::fromStdString(ID),
                                           QString::fromStdString(weapon->ID),
                                           QString::fromStdString(weapon->Name));
            parent->Entities.insert({weapon->ID, weapon});
            emitAutoComponentSignals(parent, weapon);
        }
    }
}

// =============================================================================
// getWeapon  — same as SensorProfile::getSensor
// =============================================================================
Weapon* WeaponProfile::getWeapon(const std::string& id) const
{
    if (!weapons) return nullptr;
    auto it = weapons->find(id);
    if (it == weapons->end()) return nullptr;
    return it->second;
}

// =============================================================================
// renameSubComponent  — identical to SensorProfile::renameSubComponent
// =============================================================================
void WeaponProfile::renameSubComponent(std::string id, QString newName)
{
    auto it = weapons->find(id);
    if (it != weapons->end() && it->second) {
        Hierarchy* parent    = GlobalRegistry::getParentHierarchy(this);
        it->second->Name     = newName.toStdString();
        emit parent->subComponentRenamed(QString::fromStdString(ID),
                                         QString::fromStdString(id),
                                         newName);
    }
}
