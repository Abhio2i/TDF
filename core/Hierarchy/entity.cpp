/**
 * @file entity.cpp
 * @brief Implementation of the base Entity class for simulation objects.
 */

#include "entity.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Utility/uuid.h>
#include <core/Debug/console.h>

/**
 * @brief Constructs an Entity and registers it with the global hierarchy.
 * @param h Pointer to the parent Hierarchy object.
 */
Entity::Entity(Hierarchy* h) {
    ID = Uuid::generateShortUniqueID();
    Hierarchy* hierarchy = h;
    if (hierarchy) {
        GlobalRegistry::registerEntity(this, hierarchy);
        hierarchy->dictionry[ID] = {};
        root = hierarchy;
    }
}

/**
 * @brief Destructor – logs deletion of the entity.
 */
Entity::~Entity() {
    Console::log("Delete" + Name);
}

/**
 * @brief Called after the entity is fully constructed and added to the hierarchy.
 *        Emits the entityAdded signal to notify UI/network.
 */
void Entity::spawn() {
    // Minimal or empty implementation, to be overridden by derived classes
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

/**
 * @brief Returns a list of component names supported by this entity type.
 * @return Vector of component names (empty by default, overridden by subclasses).
 */
std::vector<std::string> Entity::getSupportedComponents() {
    // Minimal or empty implementation, to be overridden by derived classes
    return std::vector<std::string>();
}

// ===== RADIO Functions =====

/**
 * @brief Adds a Radio component to this entity.
 * @param radio Pointer to the Radio object to add (ownership transferred).
 */
void Entity::addRadio(Radio* radio) {
    if (radio) radioList.push_back(radio);
}

/**
 * @brief Removes and deletes a specific Radio component from this entity.
 * @param radio Pointer to the Radio to remove.
 */
void Entity::removeRadio(Radio* radio) {
    auto it = std::find(radioList.begin(), radioList.end(), radio);
    if (it != radioList.end()) {
        delete *it;
        radioList.erase(it);
    }
}

/**
 * @brief Removes and deletes all Radio components from this entity.
 */
void Entity::clearRadios() {
    for (Radio* r : radioList) delete r;
    radioList.clear();
}

// ===== SENSOR Functions =====

/**
 * @brief Adds a Sensor component to this entity.
 * @param sensor Pointer to the Sensor object to add (ownership transferred).
 */
void Entity::addSensor(Sensor* sensor) {
    if (sensor) sensorList.push_back(sensor);
}

/**
 * @brief Removes and deletes a specific Sensor component.
 * @param sensor Pointer to the Sensor to remove.
 */
void Entity::removeSensor(Sensor* sensor) {
    auto it = std::find(sensorList.begin(), sensorList.end(), sensor);
    if (it != sensorList.end()) {
        delete *it;
        sensorList.erase(it);
    }
}

/**
 * @brief Removes and deletes all Sensor components from this entity.
 */
void Entity::clearSensors() {
    for (Sensor* s : sensorList) delete s;
    sensorList.clear();
}

// ===== IFF Functions =====

/**
 * @brief Adds an IFF component to this entity.
 * @param iff Pointer to the IFF object to add (ownership transferred).
 */
void Entity::addIFF(IFF* iff) {
    if (iff) iffList.push_back(iff);
}

/**
 * @brief Removes and deletes a specific IFF component.
 * @param iff Pointer to the IFF to remove.
 */
void Entity::removeIFF(IFF* iff) {
    auto it = std::find(iffList.begin(), iffList.end(), iff);
    if (it != iffList.end()) {
        delete *it;
        iffList.erase(it);
    }
}

/**
 * @brief Removes and deletes all IFF components from this entity.
 */
void Entity::clearIFFs() {
    for (IFF* i : iffList) delete i;
    iffList.clear();
}

// ===== WEAPON Functions =====

/**
 * @brief Adds a Weapon component to this entity.
 * @param weapon Pointer to the Weapon object to add (ownership transferred).
 */
void Entity::addWeapon(Weapon* weapon) {
    if (weapon) weaponList.push_back(weapon);
}

/**
 * @brief Removes and deletes a specific Weapon component.
 * @param weapon Pointer to the Weapon to remove.
 */
void Entity::removeWeapon(Weapon* weapon) {
    auto it = std::find(weaponList.begin(), weaponList.end(), weapon);
    if (it != weaponList.end()) {
        delete *it;
        weaponList.erase(it);
    }
}

/**
 * @brief Removes and deletes all Weapon components from this entity.
 */
void Entity::clearWeapons() {
    for (Weapon* w : weaponList) delete w;
    weaponList.clear();
}
