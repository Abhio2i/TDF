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
#include "core/Recorder/recorder.h"
#include "core/ScriptEngine/scriptengine.h"

#include <QThread> // Include QThread
#include <core/SharedMemory/sharedmemorywrapper.h>  //Shared Memory By Himanshu
#include <core/Simulation/simulation_state.h>// Recorder By Himanshu

#include <core/SQLite/sqlite.h>// For SQLite By Himanshu

class Runtime : public QObject  // QObject se inherit kiya
{
    Q_OBJECT  // Meta-object system ke liye zaroori hai

public:
    Runtime();
    ~Runtime();

    // ScenarioConfig *scenarioconfig;
    Hierarchy *hierarchy;
    Hierarchy *Library;
    SessionManager *sessionManager;
    Simulation *simulation;
    SceneRenderer *scenerenderer;
    ScriptEngine *scriptengine;
    NetworkManager *networkManager;
    Profiler *profiler;
    Console *console;
    Recorder *recorder;  // Using external Recorder
    Recording *recording;       // Using external Recorder Recording
    Replay    *replay;
    SharedMemoryWrapper* sharedWrapper; //Shared Memory By Himanshu

    SQLite *sqlite; //SQLite Memory By Himanshu

signals:

public slots:
    void handleStart();
    void handleStop();
    void handleReplay();

private:
    QThread *simulationThread;
    QThread *sharedMemoryThread; // Add this line New by Himanshu
};


#endif // RUNTIME_H
