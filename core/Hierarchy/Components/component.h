// =============================================================================
// FILE:        component.h
// MODULE:      Tactical Simulation Component System
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the abstract Component base class. This is the core
//               interface for all functional building blocks (Transform,
//               Physics, Profiles) that can be attached to simulation Entities.
//               It provides a unified interface for sub-component management
//               and serialization.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Mar 2026  Added pure virtual interfaces for sub-component CRUD.
//   Rev 3  Apr 2026  Aligned with DO-178C documentation standards for O2I.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef COMPONENT_H
#define COMPONENT_H

#include "qjsonobject.h"
#include <core/Utility/uuid.h>

// =============================================================================
// SECTION: Enumerations
// DESCRIPTION: Unique identifiers for different functional component types.
// =============================================================================
enum class ComponentType {
    Unknown,
    Transform,
    Rigidbody,
    NetworkObject,
    Mission,
    MeshRenderer2D,
    DynamicModel,
    Collider,
    Trajectory,
    AttachedEnitities,
    CrossSection,
    SensorProfile,
    IFFProfile,
    RadioProfile,
    WeaponProfile
};

class Hierarchy;
class Entity;

// =============================================================================
// CLASS: Component (Abstract)
//
// DESCRIPTION: Abstract base class that defines the lifecycle and interface
//              for all modular systems within the simulation.
// =============================================================================
class Component
{
public:
    Component(Hierarchy* h);
    virtual ~Component();

    // =========================================================================
    // SECTION: Identity & Hierarchy
    // DESCRIPTION: Unique identification and parent-child relationship markers.
    // =========================================================================
    std::string ID = Uuid::generateShortUniqueID();
    std::string parentID;
    Entity* parentEntity = nullptr;

    // =========================================================================
    // SECTION: Virtual Interface (DO-178C Compliance)
    // DESCRIPTION: Pure virtual methods for runtime type identification,
    //              dynamic sub-component management, and state persistence.
    // =========================================================================
    virtual ComponentType Typo() const { return ComponentType::Unknown; }

    // --- Sub-Component Management ---
    virtual void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) = 0;
    virtual void removeSubComponent(std::string ID) = 0;
    virtual QJsonObject getsubComponentData(std::string ID) const = 0;
    virtual void updateSubComponent(std::string ID, const QJsonObject& obj) = 0;
    virtual void renameSubComponent(std::string ID, QString newName) {}

    // --- Serialization ---
    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject& obj) = 0;
};

#endif // COMPONENT_H
