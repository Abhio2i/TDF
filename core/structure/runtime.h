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
#include "core/Recorder/recorder.h"

class Runtime : public QObject  // QObject se inherit kiya
{
    Q_OBJECT  // Meta-object system ke liye zaroori hai

public:
    Runtime();
    ~Runtime();

    ScenarioConfig *scenarioconfig;
    Hierarchy *hierarchy;
    Hierarchy *Library;
    SessionManager *sessionManager;
    Simulation *simulation;
    SceneRenderer *scenerenderer;
    NetworkManager *networkManager;
    Console *console;
    Recorder *recorder;  // Using external Recorder

signals:

public slots:
    void handleStart();
    void handleStop();
    void handleReplay();

};


#endif // RUNTIME_H
