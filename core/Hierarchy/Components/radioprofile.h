// =============================================================================
// FILE:        radioprofile.h
// MODULE:      Radio Profile Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the RadioProfile class, which acts as a container for
//              radio communication settings and configurations. Manages a
//              collection of Radio objects and provides JSON serialization
//              for persistence and network transfer.
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

#ifndef RADIOPROFILE_H
#define RADIOPROFILE_H

#include "./component.h"
#include <QObject>
#include <QJsonObject>

class Hierarchy;
class Radio;

// =============================================================================
// CLASS: RadioProfile
//
// DESCRIPTION: Represents a radio configuration profile that can be attached
//              to an entity. Holds a collection of Radio objects (channels/
//              frequencies) and supports activation/deactivation. Provides
//              JSON serialization for saving/loading profiles.
// =============================================================================
class RadioProfile: public QObject, public Component
{
    Q_OBJECT
public:
    RadioProfile(Hierarchy* h);
    ~RadioProfile();
    ComponentType Typo() const override { return ComponentType::RadioProfile; }

    // =========================================================================
    // SECTION: Radio Profile Properties
    // DESCRIPTION: Core state and reference to global radio registry.
    // =========================================================================
    bool Active;                                      //!< Whether this radio profile is active
    std::unordered_map<std::string, Radio*> *radios; //!< Pointer to map of Radio entries

    // =========================================================================
    // SECTION: Custom Parameters
    // DESCRIPTION: Extensible key-value store for user-defined radio parameters.
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

#endif // RADIOPROFILE_H
