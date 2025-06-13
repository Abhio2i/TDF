#ifndef SCENARIO_H
#define SCENARIO_H

#include <QObject>
#include <core/Config/scenarioconfig.h>
#include <core/Hierarchy/hierarchy.h>
#include <core/Config/sessionmanager.h>
#include <core/Simulation/simulation.h>
#include <core/Render/scenerenderer.h>
#include <core/Network/networkmanager.h>
#include <core/Debug/console.h>

class Scenario : public QObject  // QObject se inherit kiya
{
    Q_OBJECT  // Meta-object system ke liye zaroori hai

public:
    Scenario();
    ScenarioConfig *scenarioconfig;
    Hierarchy *hierarchy;
    Hierarchy *Library;
    SessionManager *sessionManager;
    SceneRenderer *scenerenderer;
    Console *console;
signals:
    //void scenarioStarted();  // Signal define kiya

public slots:
    //void onScenarioStarted();
};

#endif // SCENARIO_H
