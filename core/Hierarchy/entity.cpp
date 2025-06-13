

#include "entity.h"
#include "core/GlobalRegistry.h"
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

std::vector<Component*> Entity::getSupportedComponents() {
    // Minimal or empty implementation, to be overridden by derived classes
    return std::vector<Component*>();
}
