/**
 * @file component.cpp
 * @brief Implementation of the base Component class for simulation components.
 */

#include "component.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/GlobalRegistry.h"

/**
 * @brief Constructs a Component and registers it with the global hierarchy.
 * @param h Pointer to the parent Hierarchy object.
 */
Component::Component(Hierarchy* h) {
    // Optional: Initialization code
    Hierarchy* hierarchy = h;
    if (hierarchy) {
        GlobalRegistry::registerComponent(this, hierarchy);
        hierarchy->dictionry[ID] = {};
    }
}

/**
 * @brief Destructor – performs optional cleanup.
 */
Component::~Component() {
    // Optional: Cleanup
}
