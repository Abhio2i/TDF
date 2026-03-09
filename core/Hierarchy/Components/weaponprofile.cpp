#include "weaponprofile.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"
#include "core/Hierarchy/hierarchy.h"

WeaponProfile::WeaponProfile(Hierarchy* h) : Component(h) {
    Active = true;
    weapons = new std::unordered_map<std::string, Weapon*>();
}

void WeaponProfile::addSubComponent(std::string name, QString data1, QString data2, QString data3) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    Weapon* weapon = new Weapon(parent);

    // If data2 contains a weapon profile ID, copy its configuration
    if (!data2.isEmpty()) {
        auto it = parent->Weapons->find(data2.toStdString());
        if (it != parent->Weapons->end() && it->second) {
            std::string id = weapon->ID;
            QJsonObject obj = it->second->toJson();
            weapon->fromJson(obj);
            weapon->ID = id;
            weapon->parentID = parentID;
        }
    }

    weapon->parentEntity = parentEntity;
    weapon->Name = name;
    weapons->insert({weapon->ID, weapon});
    parent->Weapons->insert({weapon->ID, weapon});
    emit parent->subComponentAdded(QString::fromStdString(ID),
                                   QString::fromStdString(weapon->ID),
                                   QString::fromStdString(name));
}

void WeaponProfile::removeSubComponent(std::string ID) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    auto it = weapons->find(ID);

    if (it != weapons->end() && it->second != nullptr) {
        Weapon *weapon = it->second;
        emit parent->subComponentRemoved(QString::fromStdString(this->ID),
                                         QString::fromStdString(ID),
                                         QString::fromStdString(weapon->Name));
        weapons->erase(weapon->ID);
        parent->Weapons->erase(weapon->ID);
        delete weapon;
    }
}

void WeaponProfile::updateSubComponent(std::string ID, const QJsonObject& obj) {
    auto it = weapons->find(ID);
    if (it != weapons->end() && it->second != nullptr) {
        it->second->fromJson(obj);
    }
}

QJsonObject WeaponProfile::getsubComponentData(std::string ID) const {
    auto it = weapons->find(ID);
    if (it != weapons->end() && it->second != nullptr) {
        return it->second->toJson();
    }
    return QJsonObject(); // Return empty object if not found
}

QJsonObject WeaponProfile::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;

    QJsonObject weaponObj;
    for (const auto& [key, weaponPtr] : *weapons) {
        if (weaponPtr) {
            weaponObj[QString::fromStdString(key)] = weaponPtr->toJson();
        }
    }
    obj["weapons"] = weaponObj;
    return obj;
}

// ✅ FIX #2: fromJson() with parentID assignment
void WeaponProfile::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")) {
        // ID is typically not overwritten during deserialization
    }

    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("weapons") && obj["weapons"].isObject()) {
        QJsonObject weaponsObj = obj["weapons"].toObject();

        for (const QString& key : weaponsObj.keys()) {
            QJsonObject weaponObj = weaponsObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            std::string id = weaponObj["id"].toString().toStdString();

            Weapon *weapon;
            bool exists = weapons->count(id);

            if (exists > 0) {
                weapon = (*weapons)[id];
            } else {
                weapon = new Weapon(parent);
                weapon->parentEntity = parentEntity;
                // ✅ FIX #2: ADD THIS LINE - Restore parent ID relationship on load
                weapon->parentID = parentID;
            }

            weapon->Name = weaponObj["name"].toString().toStdString();
            weapon->ID = weaponObj["id"].toString().toStdString();
            weapon->fromJson(weaponObj);

            if (!exists) {
                weapons->insert({weapon->ID, weapon});
                parent->Weapons->insert({weapon->ID, weapon});
                emit parent->subComponentAdded(QString::fromStdString(ID),
                                               QString::fromStdString(weapon->ID),
                                               QString::fromStdString(weapon->Name));
            }
        }
    }
}

void WeaponProfile::renameSubComponent(std::string id, QString newName) {
    auto it = weapons->find(id);
    if (it != weapons->end() && it->second != nullptr) {
        Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
        it->second->Name = newName.toStdString();
        emit parent->subComponentRenamed(
            QString::fromStdString(ID),
            QString::fromStdString(id),
            newName);
    }
}
