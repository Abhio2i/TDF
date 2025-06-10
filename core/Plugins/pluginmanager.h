#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H0
#include <qobject.h>
#include <core/Config/scenarioconfig.h>
#include <core/structure/hierarchy.h>
#include <core/Config/sessionmanager.h>
#include <core/Simulation/simulation.h>
#include <core/Render/scenerenderer.h>
#include <core/Network/networkmanager.h>
#include <core/Debug/console.h>

class PluginManager: public QObject
{
    Q_OBJECT
public:
    PluginManager();

    ScenarioConfig *scenarioconfig;
    Hierarchy *hierarchy;
    SessionManager *sessionManager;
    Simulation *simulation;
    SceneRenderer *scenerenderer;
    NetworkManager *networkManager;
    Console *console;

    void loadPlugin();
    void getPluginList();
    void addPlugin();
    void removePlugin();
};

#endif // PLUGINMANAGER_H
