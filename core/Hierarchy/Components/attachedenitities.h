// =============================================================================
// FILE:        attachedenitities.h
// MODULE:      Tactical Simulation Component System
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the AttachedEnitities class, a specialized Component
//               responsible for managing relationships between a parent entity
//               and its sub-entities (e.g., hardpoints, sub-systems).
//               It maintains a registry of attached actors for hierarchical
//               simulation updates.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Oct 2025  Initial implementation for TDF project.
//   Rev 2  Apr 2026  Aligned with DO-178C documentation standards for O2I.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef ATTACHEDENITITIES_H
#define ATTACHEDENITITIES_H

#include "./component.h"
#include <QObject>
#include <core/Hierarchy/Struct/constants.h>

class Entity;

// =============================================================================
// CLASS: AttachedEnitities
//
// DESCRIPTION: Component-based manager for entity-to-entity attachments.
//              Overrides base Component methods for type identification and
//              state persistence.
// =============================================================================
class AttachedEnitities : public QObject, public Component
{
    Q_OBJECT

public:
    AttachedEnitities();

    // =========================================================================
    // SECTION: Component Identity
    // DESCRIPTION: Runtime type identification for the attachment system.
    // =========================================================================
    ComponentType Typo() const override { return ComponentType::AttachedEnitities; }

    // =========================================================================
    // SECTION: Data Members
    // DESCRIPTION: Registry of attached entities and their associated types.
    // =========================================================================
    Constants::EntityType entity;
    std::unordered_map<std::string, Entity> *entities;

    // =========================================================================
    // SECTION: Virtual Interface Overrides
    // DESCRIPTION: Logic for data retrieval and state serialization.
    // =========================================================================
    QJsonObject getsubComponentData(std::string ID) const override;

    // --- Serialization ---
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    // --- Sub-Component Management (Stubs/Implementations) ---
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override {}
    void removeSubComponent(std::string ID) override {}
    void updateSubComponent(std::string ID, const QJsonObject& obj) override {}
};

#endif // ATTACHEDENITITIES_H
