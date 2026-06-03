// =============================================================================
// FILE:        runtime.h
// MODULE:      Runtime Environment
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Runtime class, which aggregates and manages all
//              core simulation subsystems: Hierarchy, SessionManager,
//              Simulation, SceneRenderer, ScriptEngine, NetworkManager,
//              Profiler, Console, Recorder, SharedMemoryWrapper, and SQLite.
//              Runs simulation and shared memory in separate threads.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef RUNTIME_H
#define RUNTIME_H

#include <QObject>
#include <core/Config/scenarioconfig.h>
#include <core/Hierarchy/hierarchy.h>
#include <core/Config/sessionmanager.h>
#include <core/Simulation/simulation.h>
#include <core/Render/scenerenderer.h>
#include <core/Network/networkmanager.h>
#include <core/Debug/console.h>
#include "core/Debug/profiler.h"
#include "core/Plugins/pluginmanager.h"
#include "core/Recorder/recorder.h"
#include "core/ScriptEngine/scriptengine.h"
#include "core/DISPlugin/DISNetworkPlugin.h"

#include <QThread> // Include QThread
#include <core/SharedMemory/sharedmemorywrapper.h>  //Shared Memory By Himanshu
#include <core/Simulation/simulation_state.h>// Recorder By Himanshu

#include <core/SQLite/sqlite.h>// For SQLite By Himanshu

// =============================================================================
// CLASS: Runtime
//
// DESCRIPTION: Central runtime container that owns and initialises all major
//              simulation components. Provides start/stop/replay slots and
//              runs the simulation and shared memory tasks in separate threads
//              to avoid blocking the main (UI) thread.
// =============================================================================
class Runtime : public QObject
{
    Q_OBJECT

public:
    Runtime();
    ~Runtime();

    // =========================================================================
    // SECTION: Core Component Pointers
    // DESCRIPTION: References to all major subsystems (owned by Runtime).
    // =========================================================================
    // ScenarioConfig *scenarioconfig; // (commented out)
    Hierarchy *hierarchy;           //!< Main entity hierarchy (runtime)
    Hierarchy *Library;             //!< Library hierarchy (static assets/profiles)
    SessionManager *sessionManager; //!< User session preferences
    Simulation *simulation;         //!< Simulation engine
    SceneRenderer *scenerenderer;   //!< 3D scene renderer
    ScriptEngine *scriptengine;     //!< Scripting engine (e.g., for mission logic)
    NetworkManager *networkManager; //!< Network synchronisation manager
    Profiler *profiler;             //!< Performance profiler
    Console *console;               //!< Logging console
    Recorder *recorder;             //!< Simulation recorder (external)
    Recording *recording;           //!< Active recording session
    Replay    *replay;              //!< Replay controller
    SharedMemoryWrapper* sharedWrapper; //!< Shared memory interface (Himanshu)
    SQLite *sqlite;                 //!< SQLite database interface (Himanshu)
    DISNetworkPlugin* disPlugin = nullptr;
    PluginManager* pluginManager = nullptr;

signals:
    // No signals defined currently

public slots:
    void handleStart();     //!< Starts the simulation (called from UI)
    void handleStop();      //!< Stops the simulation
    void handleReplay();    //!< Starts replay mode

private:
    QThread *simulationThread;      //!< Thread where simulation runs
    QThread *sharedMemoryThread;    //!< Thread for shared memory updates (Himanshu)
};

#endif // RUNTIME_H
