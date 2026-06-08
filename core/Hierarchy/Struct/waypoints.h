// =============================================================================
// FILE:        waypoints.h
// MODULE:      Waypoint Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Waypoints class, which represents a single waypoint
//              along a trajectory. Stores geographic coordinates (Geocords) and
//              local position (Vector), along with desired speed and flags for
//              sensor activation and formation change at the waypoint. Provides
//              JSON serialisation for persistence.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef WAYPOINTS_H
#define WAYPOINTS_H

#include <QObject>
#include <QJsonObject>
#include "./vector.h"
#include "./geocords.h"

// =============================================================================
// CLASS: Waypoints
//
// DESCRIPTION: Represents a single navigation waypoint. Contains both geographic
//              and local coordinate representations, a speed target, and flags
//              indicating whether to activate sensors or change formation upon
//              reaching this waypoint.
// =============================================================================
class Waypoints: public QObject
{
    Q_OBJECT
public:
    Waypoints();

    // =========================================================================
    // SECTION: Waypoint Data
    // DESCRIPTION: Position, speed, and behaviour flags.
    // =========================================================================
    Geocords *geocord = nullptr;   //!< Geographic coordinates (lat/lon/alt/heading)
    Vector *position = nullptr;    //!< Local Cartesian position (x, y, z)
    double speed = 0;              //!< Desired speed when reaching this waypoint
    bool sensor = true;            //!< Whether to activate sensors at this waypoint
    bool formation = true;         //!< Whether to trigger formation change at this waypoint
    bool dropWeapon = false;
    float dropcount = 0;

    // Serialization
    QJsonObject toJson() const;    //!< Serialises waypoint to JSON
    void fromJson(const QJsonObject& obj); //!< Deserialises waypoint from JSON
};

#endif // WAYPOINTS_H
