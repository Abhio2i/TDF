// =============================================================================
// FILE:        SimulationPlugin.h
// MODULE:      DIS Network Plugin — Plugin Base
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Abstract base class for all simulation plugins.
//              Every plugin (DIS network, radar, sonar, recorder etc)
//              implements this interface.
//              Simulation::frame() calls applyPendingUpdates() on all
//              registered plugins every frame.
//              Each plugin manages its own internal thread.
//
// USAGE:
//   class DISNetworkPlugin : public SimulationPlugin { ... };
//   class RadarPlugin       : public SimulationPlugin { ... }; // future
//   class SonarPlugin       : public SimulationPlugin { ... }; // future
//
// THREAD MODEL:
//   Plugin is created on main thread.
//   Plugin starts its own internal worker thread in start().
//   applyPendingUpdates() is called on main thread every frame.
//   Internal thread pushes data to queues.
//   applyPendingUpdates() drains queues on main thread.
//   No shared mutable state between threads.
// =============================================================================

#ifndef SIMULATIONPLUGIN_H
#define SIMULATIONPLUGIN_H

#include <QObject>
#include <QString>

// Forward declarations — plugins may or may not need these
class Simulation;
class Hierarchy;

// =============================================================================
// SimulationPlugin
// Pure virtual base class.
// All plugins must implement every method.
// =============================================================================
class SimulationPlugin : public QObject {
    Q_OBJECT

public:
    explicit SimulationPlugin(QObject* parent = nullptr)
        : QObject(parent) {}

    virtual ~SimulationPlugin() = default;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    // Attach the engine hierarchy so plugin can read entity state
    // Called once before start()
    virtual void attachHierarchy(Hierarchy* hierarchy) = 0;

    // Attach the simulation so plugin can connect to sim signals
    // Called once before start()
    virtual void attachSimulation(Simulation* simulation) = 0;

    // Start the plugin — begins internal thread, opens network, etc
    // configPath = path to plugin config file (e.g. "dis_config.json")
    virtual void start(const QString& configPath) = 0;

    // Stop the plugin cleanly — shuts down thread, closes sockets
    virtual void stop() = 0;

    // ── Per-frame update ──────────────────────────────────────────────────────

    // Called by Simulation::frame() on MAIN THREAD every frame.
    // Plugin drains its incoming queues and applies results to engine.
    // Must be fast — this runs on the simulation tick.
    // Do NOT do network I/O here. Only queue drain.
    virtual void applyPendingUpdates() = 0;

    // ── Identity ──────────────────────────────────────────────────────────────

    // Human readable name for logging and UI
    virtual QString pluginName() const = 0;

    // Whether plugin is currently running
    virtual bool isRunning() const = 0;
};

#endif // SIMULATIONPLUGIN_H