// =============================================================================
// FILE:        fixedpoints.h
// MODULE:      Tactical Simulation Entity Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the FixedPoints class, a specialized Entity type used
//               for static simulation markers and reference points. It manages
//               spatial presence through Transform, Collider, and 2D rendering.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Apr 2026  Aligned with DO-178C documentation standards.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef FIXEDPOINTS_H
#define FIXEDPOINTS_H

#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>

// =============================================================================
// CLASS: FixedPoints
//
// DESCRIPTION: Concrete implementation of an Entity representing a non-moving
//              simulation point. Overrides mandatory lifecycle methods for
//              component management and serialization.
// =============================================================================
class FixedPoints: public Entity
{
    Q_OBJECT
public:
    FixedPoints(Hierarchy* h);

    // =========================================================================
    // SECTION: Core Components
    // DESCRIPTION: Pointers to functional blocks that define physical and
    //              visual properties.
    // =========================================================================
    Transform *transform = nullptr;
    Collider *collider = nullptr;
    MeshRenderer2D *meshRenderer2d = nullptr;

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
    // SECTION: Serialization
    // DESCRIPTION: logic for state persistence to/from JSON format.
    // =========================================================================
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

};

#endif // FIXEDPOINTS_H
