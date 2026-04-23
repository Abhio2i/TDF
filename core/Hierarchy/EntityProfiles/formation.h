// =============================================================================
// FILE:        formation.h
// MODULE:      Tactical Simulation Entity Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the Formation class, a specialized Entity that manages
//               group behavior and spatial positioning for multiple simulation
//               objects. It handles coordinate generation for wingmen relative
//               to a lead (mothership).
//
// AUTHOR:       Waris
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Mar 2026  Integrated FormationPosition tracking and generation.
//   Rev 3  Apr 2026  Added DO-178C compliant documentation and type conversion.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef FORMATION_H
#define FORMATION_H

#include "core/Hierarchy/Struct/formationposition.h"
#include "core/Hierarchy/entity.h"

// =============================================================================
// CLASS: Formation
//
// DESCRIPTION: Concrete implementation of an Entity representing a tactical
//              formation. Manages a lead position and a map of followers with
//              dynamic position generation capabilities.
// =============================================================================
class Formation: public Entity
{
    Q_OBJECT
public:
    Formation(Hierarchy* h);

    // =========================================================================
    // SECTION: Identity & State
    // DESCRIPTION: Properties defining the formation's structure and members.
    // =========================================================================
    Constants::FormationType formationType;
    int count;
    FormationPosition *mothership;
    std::unordered_map<std::string,FormationPosition*> *formationPositions;

    // =========================================================================
    // SECTION: Formation Logic
    // DESCRIPTION: Core methods for creating and updating formation geometry.
    // =========================================================================
    void formationCreate();
    void generatePositions(int targetCount);

    // =========================================================================
    // SECTION: Virtual Interface Overrides
    // DESCRIPTION: Mandatory methods for entity lifecycle and dynamic systems.
    // =========================================================================
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    // =========================================================================
    // SECTION: Serialization & Utilities
    // DESCRIPTION: Logic for state persistence and type-string conversions.
    // =========================================================================
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    QString formationTypeToString(Constants::FormationType type) const;
    Constants::FormationType stringToFormationType(QString str) const;
    QStringList formationTypeOptions() const;

private:
    // =========================================================================
    // SECTION: Private Helpers
    // DESCRIPTION: Internal logic for resolving entity IDs during loading.
    // =========================================================================
    void resolveEntityReference(FormationPosition* position, const QJsonObject& obj);

};

#endif // FORMATION_H
