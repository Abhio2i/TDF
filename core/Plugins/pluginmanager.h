// =============================================================================
// FILE:        pluginmanager.h
// MODULE:      Plugin Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the PluginManager class, which manages core simulation
//              components as plugins (or aggregate references). Holds pointers
//              to ScenarioConfig, Hierarchy, SessionManager, Simulation,
//              SceneRenderer, NetworkManager, and Console. Provides methods
//              for loading, listing, adding, and removing plugins.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H   // Note: Original guard macro has trailing "0"

#include "qpluginloader.h"
#include <qobject.h>
#include <core/Config/scenarioconfig.h>
#include <core/Hierarchy/hierarchy.h>
#include <core/Config/sessionmanager.h>
#include <core/Simulation/simulation.h>
#include <core/Render/scenerenderer.h>
#include <core/Network/networkmanager.h>
#include <core/Debug/console.h>

// class ScenarioConfig;
// class Hierarchy;
// class SessionManager;
// class Simulation;
// class SceneRenderer;
// class NetworkManager;
// class Console;

// =============================================================================
// CLASS: PluginManager
//
// DESCRIPTION: Aggregates pointers to the main simulation subsystems. Acts as
//              a central access point or plugin container. Provides methods to
//              dynamically load, list, add, and remove plugins (subsystems).
// =============================================================================
class PluginManager: public QObject
{
    Q_OBJECT
public:
    PluginManager();

    // =========================================================================
    // SECTION: Core Component References
    // DESCRIPTION: Pointers to the major simulation and configuration objects.
    // =========================================================================
    ScenarioConfig *scenarioconfig;     //!< Scenario configuration manager
    Hierarchy *hierarchy;               //!< Entity hierarchy registry
    SessionManager *sessionManager;     //!< User session preferences
    Simulation *simulation;             //!< Simulation engine/controller
    SceneRenderer *scenerenderer;       //!< 3D scene renderer
    NetworkManager *networkManager;     //!< Network synchronisation manager
    Console *console;                   //!< Logging console

    // Plugin operations
    bool loadPlugin(std::string path);      //!< Load a plugin (subsystem)
    bool unloadPlugin(std::string path);      //!< Unload a plugin (subsystem)
    // void getPluginList();   //!< Retrieves list of available/loaded plugins
    // void addPlugin();       //!< Adds a new plugin
    // void removePlugin();    //!< Removes an existing plugin

    std::unordered_map<std::string, QPluginLoader*> pluginList;
    // Sahi Syntax (Extra '<' hata diya)
    std::unordered_map<std::string, std::vector<QMetaObject::Connection>> pluginconnection;
};

#endif // PLUGINMANAGER_H
