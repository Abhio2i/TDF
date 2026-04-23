// =============================================================================
// FILE:        mission.h
// MODULE:      Mission Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Mission class, which represents a tactical mission
//              assigned to an entity. Contains mission name, active state,
//              entity type, and placeholder for task groups. Supports JSON
//              serialization and component-based subcomponent management.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added component interface overrides.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef MISSION_H
#define MISSION_H

#include "./component.h"
#include <QObject>
#include <core/Hierarchy/Struct/constants.h>
// #include <core/Hierarchy/Struct/task.h>

// =============================================================================
// CLASS: Mission
//
// DESCRIPTION: Represents a mission component that can be attached to an
//              entity (e.g., aircraft, ground vehicle). Stores mission
//              metadata and provides hooks for task group management.
// =============================================================================
class Mission : public QObject, public Component
{
    Q_OBJECT
public:
    Mission();
    ComponentType Typo() const override { return ComponentType::Mission; }

    // =========================================================================
    // SECTION: Mission Properties
    // DESCRIPTION: Core attributes defining the mission.
    // =========================================================================
    std::string Name;                       //!< Human-readable mission name
    bool Active;                            //!< Whether this mission is currently active
    Constants::EntityType type;             //!< Type of entity this mission applies to
    // std::unordered_map<std::string, Task> *taskGroup; //!< Collection of tasks (commented out)

    // Component interface overrides
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // MISSION_H
