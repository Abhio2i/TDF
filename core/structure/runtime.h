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

class Runtime : public QObject  // QObject se inherit kiya
{
    Q_OBJECT  // Meta-object system ke liye zaroori hai

public:
    Runtime();
    ScenarioConfig *scenarioconfig;
    Hierarchy *hierarchy;
    Hierarchy *Library;
    SessionManager *sessionManager;
    Simulation *simulation;
    SceneRenderer *scenerenderer;
    NetworkManager *networkManager;
    Console *console;
signals:

public slots:

};


#endif // RUNTIME_H
