// =============================================================================
// FILE:        collider.h
// MODULE:      Tactical Simulation Physics System
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the Collider class, a component responsible for spatial
//               volume representation and proximity detection. It manages
//               bounding volumes (Radius, Width, Length, Height) and handles
//               collision/warning triggers within the simulation.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Feb 2026  Added WarningRadius and dynamic Update slot.
//   Rev 3  Apr 2026  Aligned with DO-178C documentation standards for O2I.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef COLLIDER_H
#define COLLIDER_H

#include "./component.h"
#include <QObject>
#include <QJsonObject>
#include <core/Hierarchy/Struct/vector.h>
#include <core/Hierarchy/Struct/constants.h>

class Hierarchy;

// =============================================================================
// CLASS: Collider
//
// DESCRIPTION: Component-based collision handler. Manages geometric bounds and
//              provides an interface for proximity-based logic and sub-component
//              parameter storage.
// =============================================================================
class Collider : public QObject, public Component
{
    Q_OBJECT
public:
    Collider(Hierarchy* h);

    // =========================================================================
    // SECTION: Component Identity
    // DESCRIPTION: Runtime type identification for the physics system.
    // =========================================================================
    ComponentType Typo() const override { return ComponentType::Collider; }

    // =========================================================================
    // SECTION: Physical Dimensions & State
    // DESCRIPTION: Core attributes defining the collision volume and operational
    //              status of the collider.
    // =========================================================================
    bool Active;
    float CollideRadius;
    float WarningRadius;
    float Width;
    float Length;
    float Height;

    // =========================================================================
    // SECTION: Type Definitions
    // DESCRIPTION: Pointers to vector data and classification constants.
    // =========================================================================
    Vector *vector;
    Constants::EntityType type;
    Constants::ColliderType collider;

    // =========================================================================
    // SECTION: Parameter Management
    // DESCRIPTION: Storage for custom key-value pairs associated with collision.
    // =========================================================================
    QJsonObject AdditionalParameters;

    // =========================================================================
    // SECTION: Virtual Interface Overrides
    // DESCRIPTION: Mandatory methods for sub-component CRUD and serialization.
    // =========================================================================
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // --- Serialization ---
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

signals:
    // Signal definitions can be added here if required for future compliance.

public slots:
    // =========================================================================
    // SECTION: Simulation Control
    // DESCRIPTION: Slot for real-time physics updates based on delta time.
    // =========================================================================
    void Update(float deltaTime = 0.01f);
};

#endif // COLLIDER_H
