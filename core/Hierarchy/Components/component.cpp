#include "component.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/GlobalRegistry.h"
Component::Component(Hierarchy* h) {
    // Optional: Initialization code
    Hierarchy* hierarchy = h;
    if (hierarchy) {
        GlobalRegistry::registerComponent(this, hierarchy);
        hierarchy->dictionry[ID] = {};
    }
}

Component::~Component() {
    // Optional: Cleanup
}
