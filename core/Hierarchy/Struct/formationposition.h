// =============================================================================
// FILE:        formationposition.h
// MODULE:      Formation Position Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the FormationPosition class, which stores positional
//              offsets for an entity within a formation. Contains a reference
//              to the entity, a local vector offset, and a geographic offset.
//              Provides JSON serialization for persistence.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef FORMATIONPOSITION_H
#define FORMATIONPOSITION_H

#include <QObject>
#include <core/Hierarchy/entity.h>
#include "./vector.h"
#include "./geocords.h"

// =============================================================================
// CLASS: FormationPosition
//
// DESCRIPTION: Holds the position data for an entity relative to a formation
//              leader or reference point. Provides both local Cartesian offset
//              (Vector) and geographic offset (Geocords) for flexibility in
//              different coordinate systems.
// =============================================================================
class FormationPosition: public QObject
{
    Q_OBJECT
public:
    FormationPosition();

    // =========================================================================
    // SECTION: Formation Position Data
    // DESCRIPTION: Entity reference and offset vectors.
    // =========================================================================
    Entity *entity;         //!< Pointer to the entity in this formation slot
    Vector *Offset;         //!< Local Cartesian offset from formation leader
    Geocords *geoOffset;    //!< Geographic offset (lat/lon/alt) from leader

    // Serialization
    QJsonObject toJson();               //!< Serialises formation position to JSON
    void fromJson(const QJsonObject &obj); //!< Deserialises from JSON
};

#endif // FORMATIONPOSITION_H
