// =============================================================================
// FILE:        rigidbody.h
// MODULE:      Physics Simulation
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Rigidbody class, which provides physics simulation
//              capabilities for entities. Handles mass, drag, gravity, kinematics,
//              velocity, angular velocity, and force/torque application. Supports
//              freezing position/rotation axes and JSON serialization.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added custom parameters and force/torque methods.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include "./component.h"
#include <QObject>
#include <core/Hierarchy/Struct/vector.h>
#include <QJsonObject>

// =============================================================================
// CLASS: Rigidbody
//
// DESCRIPTION: Component that adds Newtonian physics to an entity. Manages
//              linear and angular motion under forces, gravity, and drag.
//              Provides axis locking for constrained simulation.
// =============================================================================
class Rigidbody: public QObject, public Component
{
    Q_OBJECT
public:
    Rigidbody();
    ComponentType Typo() const override { return ComponentType::Rigidbody; }

    // =========================================================================
    // SECTION: Physics State Flags
    // DESCRIPTION: Enables/disables various physics behaviours.
    // =========================================================================
    bool Active;            //!< Whether physics simulation is active
    bool Gravity;           //!< Apply gravity to this rigidbody
    bool Kinematics;        //!< If true, rigidbody is not driven by physics

    // =========================================================================
    // SECTION: Position & Rotation Constraints
    // DESCRIPTION: Freeze specific axes of motion or rotation.
    // =========================================================================
    bool freezePositionX;   //!< Lock movement along X axis
    bool freezePositionY;   //!< Lock movement along Y axis
    bool freezePositionZ;   //!< Lock movement along Z axis

    bool freezeRotationX;   //!< Lock rotation around X axis
    bool freezeRotationY;   //!< Lock rotation around Y axis
    bool freezeRotationZ;   //!< Lock rotation around Z axis

    // =========================================================================
    // SECTION: Physical Properties
    // DESCRIPTION: Mass, drag coefficients, and time step.
    // =========================================================================
    float Mass;             //!< Mass of the object (kg)
    float Drag;             //!< Linear drag coefficient
    float angularDrag;      //!< Angular drag coefficient
    float deltaTime;        //!< Simulation time step

    // =========================================================================
    // SECTION: Motion Vectors
    // DESCRIPTION: Linear and angular velocity of the rigidbody.
    // =========================================================================
    Vector *velocity;           //!< Linear velocity vector
    Vector *angularVelocity;    //!< Angular velocity vector

    // =========================================================================
    // SECTION: Custom Parameters
    // DESCRIPTION: Extensible key-value store for user-defined physics data.
    // =========================================================================
    QJsonObject AdditionalParameters;

    // Physics methods
    void addForce(const Vector& force);         //!< Applies a linear force
    void addTorque(const Vector& torque);       //!< Applies a torque (rotational force)
    void addVelocity(const Vector& v);          //!< Adds directly to linear velocity

    Vector* setLinearVelocity(const Vector& v); //!< Sets linear velocity
    Vector* setAngularVelocity(const Vector& v);//!< Sets angular velocity

    // Component interface overrides
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

signals:
    void setLinearVel(const Vector& velocity);  //!< Emitted when linear velocity changes
    void setAngularVel(const Vector& velocity); //!< Emitted when angular velocity changes
};

#endif // RIGIDBODY_H
