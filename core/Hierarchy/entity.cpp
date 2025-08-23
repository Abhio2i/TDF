#include "entity.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Utility/uuid.h>
#include <core/Debug/console.h>

Entity::Entity(Hierarchy* h) {
    ID = Uuid::generateShortUniqueID();
    Hierarchy* hierarchy = h;
    if (hierarchy) {
        GlobalRegistry::registerEntity(this, hierarchy);
        hierarchy->dictionry[ID] = {};
    }
}

Entity::~Entity() {
    Console::log("Delete" + Name);
}

void Entity::spawn() {
    // Minimal or empty implementation, to be overridden by derived classes
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

std::vector<std::string> Entity::getSupportedComponents() {
    // Minimal or empty implementation, to be overridden by derived classes
    return std::vector<std::string>();
}

// ===== RADIO Functions =====
void Entity::addRadio(Radio* radio) {
    if (radio) radioList.push_back(radio);
}

void Entity::removeRadio(Radio* radio) {
    auto it = std::find(radioList.begin(), radioList.end(), radio);
    if (it != radioList.end()) {
        delete *it;
        radioList.erase(it);
    }
}

void Entity::clearRadios() {
    for (Radio* r : radioList) delete r;
    radioList.clear();
}

// ===== SENSOR Functions =====
void Entity::addSensor(Sensor* sensor) {
    if (sensor) sensorList.push_back(sensor);
}

void Entity::removeSensor(Sensor* sensor) {
    auto it = std::find(sensorList.begin(), sensorList.end(), sensor);
    if (it != sensorList.end()) {
        delete *it;
        sensorList.erase(it);
    }
}

void Entity::clearSensors() {
    for (Sensor* s : sensorList) delete s;
    sensorList.clear();
}

// ===== IFF Functions =====
void Entity::addIFF(IFF* iff) {
    if (iff) iffList.push_back(iff);
}

void Entity::removeIFF(IFF* iff) {
    auto it = std::find(iffList.begin(), iffList.end(), iff);
    if (it != iffList.end()) {
        delete *it;
        iffList.erase(it);
    }
}

void Entity::clearIFFs() {
    for (IFF* i : iffList) delete i;
    iffList.clear();
}

