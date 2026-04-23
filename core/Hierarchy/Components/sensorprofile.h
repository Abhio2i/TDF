// =============================================================================
// FILE:        sensorprofile.h
// MODULE:      Sensor Profile Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the SensorProfile class, which acts as a container for
//              sensor configurations (radar, EO, IR, etc.). Manages a collection
//              of Sensor objects and provides lookup by ID. Supports JSON
//              serialization for persistence and network transfer.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added custom parameters map and subcomponent management.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef SENSORPROFILE_H
#define SENSORPROFILE_H

#include "./component.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include <QObject>
#include <QJsonObject>

class Hierarchy;

// =============================================================================
// CLASS: SensorProfile
//
// DESCRIPTION: Represents a sensor configuration profile that can be attached
//              to an entity. Holds a collection of Sensor objects (radar, EO,
//              IR, etc.) and supports activation/deactivation. Provides JSON
//              serialization for saving/loading profiles.
// =============================================================================
class SensorProfile: public QObject, public Component
{
    Q_OBJECT
public:
    SensorProfile(Hierarchy* h);
    ComponentType Typo() const override { return ComponentType::SensorProfile; }

    // =========================================================================
    // SECTION: Sensor Profile Properties
    // DESCRIPTION: Core state and reference to global sensor registry.
    // =========================================================================
    bool Active;                                      //!< Whether this sensor profile is active
    std::unordered_map<std::string, Sensor*> *sensors; //!< Pointer to map of Sensor entries

    Sensor* getSensor(const std::string& id) const;   //!< Retrieves a sensor by its ID

    // =========================================================================
    // SECTION: Custom Parameters
    // DESCRIPTION: Extensible key-value store for user-defined sensor parameters.
    // =========================================================================
    QJsonObject AdditionalParameters;                     //!< Custom key-value parameters

    // Component interface overrides
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    void renameSubComponent(std::string ID, QString newName) override;
};

#endif // SENSORPROFILE_H
