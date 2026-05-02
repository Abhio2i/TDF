// =============================================================================
// FILE:        iffprofile.h
// MODULE:      IFF Profile Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the IFFProfile class, which acts as a container for
//              IFF (Identification Friend or Foe) settings and configurations.
//              It manages a collection of IFF objects and provides serialization
//              to/from JSON for persistence and network transfer.
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

#ifndef IFFPROFILE_H
#define IFFPROFILE_H

#include "./component.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include <QObject>
#include <QJsonObject>

class Hierarchy;

// =============================================================================
// CLASS: IFFProfile
//
// DESCRIPTION: Represents an IFF configuration profile that can be attached to
//              an entity. Holds a collection of IFF objects (modes/codes) and
//              supports activation/deactivation. Provides JSON serialization
//              for saving/loading profiles.
// =============================================================================
class IFFProfile: public QObject, public Component
{
    Q_OBJECT
public:
    IFFProfile(Hierarchy* h);
    ~IFFProfile();
    ComponentType Typo() const override { return ComponentType::IFFProfile; }

    // =========================================================================
    // SECTION: IFF Profile Properties
    // DESCRIPTION: Core state and reference to global IFF registry.
    // =========================================================================
    bool Active;                                      //!< Whether this IFF profile is active
    std::unordered_map<std::string, IFF*> *iffs;     //!< Pointer to map of IFF entries (modes/codes)

    // =========================================================================
    // SECTION: Custom Parameters
    // DESCRIPTION: Extensible key-value store for user-defined IFF parameters.
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

#endif // IFFPROFILE_H
