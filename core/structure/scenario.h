// =============================================================================
// FILE:        scenario.h
// MODULE:      Scenario Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Scenario class, which aggregates core simulation
//              components for a specific scenario: Hierarchy (runtime and
//              library), SessionManager, SceneRenderer, ScriptEngine, and
//              Console. Provides Qt signal/slot capabilities for scenario
//              lifecycle events.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef SCENARIO_H
#define SCENARIO_H

#include "core/ScriptEngine/scriptengine.h"
#include <QObject>
#include <core/Config/scenarioconfig.h>
#include <core/Hierarchy/hierarchy.h>
#include <core/Config/sessionmanager.h>
#include <core/Simulation/simulation.h>
#include <core/Render/scenerenderer.h>
#include <core/Network/networkmanager.h>
#include <core/Debug/console.h>

// =============================================================================
// CLASS: Scenario
//
// DESCRIPTION: Container for all components needed to run a simulation
//              scenario. Holds references to the runtime hierarchy, asset
//              library, session preferences, renderer, scripting engine, and
//              console. Intended to be instantiated per scenario.
// =============================================================================
class Scenario : public QObject
{
    Q_OBJECT

public:
    Scenario();

    // =========================================================================
    // SECTION: Core Component Pointers
    // DESCRIPTION: References to major subsystems for this scenario.
    // =========================================================================
    // ScenarioConfig *scenarioconfig;   //!< (Commented out) Scenario configuration
    Hierarchy *hierarchy;               //!< Runtime entity hierarchy
    Hierarchy *Library;                 //!< Library hierarchy (static assets/profiles)
    SessionManager *sessionManager;     //!< User session preferences
    SceneRenderer *scenerenderer;       //!< 3D scene renderer
    ScriptEngine *scriptengine;         //!< Scripting engine for mission logic
    Console *console;                   //!< Logging console

signals:
    //void scenarioStarted();            //!< Emitted when scenario starts (commented out)

public slots:
    //void onScenarioStarted();          //!< Slot for scenario start event (commented out)
};

#endif // SCENARIO_H
