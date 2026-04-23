// =============================================================================
// FILE:        database.h
// MODULE:      Database Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Database class, which aggregates pointers to core
//              simulation components: Hierarchy (main and mission) and Console.
//              Serves as a central access point for database-related operations
//              (persistence, loading, etc.). Inherits QObject for Qt meta-object
//              system support.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <core/Config/scenarioconfig.h>
#include <core/Hierarchy/hierarchy.h>
#include <core/Config/sessionmanager.h>
#include <core/Simulation/simulation.h>
#include <core/Render/scenerenderer.h>
#include <core/Network/networkmanager.h>
#include <core/Debug/console.h>

// =============================================================================
// CLASS: Database
//
// DESCRIPTION: Aggregates references to the main simulation hierarchy and
//              console for database operations (saving/loading scenarios,
//              entity data, etc.). Provides Qt signal/slot capabilities.
// =============================================================================
class Database: public QObject      // Inherits QObject for Qt meta-object system
{
    Q_OBJECT                        // Required for signals/slots and RTTI

public:
    Database();

    // =========================================================================
    // SECTION: Core Component References
    // DESCRIPTION: Pointers to simulation subsystems.
    // =========================================================================
    Hierarchy *hierarchy;           //!< Main entity hierarchy
    Hierarchy *Mission;             //!< Mission-specific hierarchy (separate instance)
    Console *console;               //!< Logging console

signals:
    // No signals defined currently

public slots:
    // No slots defined currently
};

#endif // DATABASE_H
