#include "runtime.h"
#include <QDebug>

Runtime::Runtime() {
    hierarchy = new Hierarchy();
    Library = new Hierarchy();
    scenerenderer = new SceneRenderer();
    simulation = new Simulation();
    networkManager = new NetworkManager();
    console  = Console::internalInstance();

    recorder = new Recorder(hierarchy, simulation);  //  Using external Recorder class

    // Rendering pipeline
    connect(hierarchy,&Hierarchy::entityMeshAdded,scenerenderer,&SceneRenderer::entityAdded);
    connect(hierarchy,&Hierarchy::entityMeshRemoved,scenerenderer,&SceneRenderer::entityRemoved);
    connect(simulation,&Simulation::Render,scenerenderer,&SceneRenderer::Render);

    // Physics system connections
    connect(hierarchy,&Hierarchy::entityPhysicsAdded,simulation,&Simulation::entityAdded);
    connect(hierarchy,&Hierarchy::entityPhysicsRemoved,simulation,&Simulation::entityRemoved);
    connect(hierarchy,&Hierarchy::entityUpdate,simulation,&Simulation::entityUpdate);

    // NetworkManager connections
    connect(hierarchy, &Hierarchy::entityAddedPointer,
            networkManager, &NetworkManager::entityAddedPointer);
    connect(hierarchy, &Hierarchy::entityAdded,
            networkManager, &NetworkManager::entityAdded);
    connect(hierarchy, &Hierarchy::entityRemovedfull,
            networkManager, &NetworkManager::entityRemoved);
    connect(hierarchy, &Hierarchy::entityRenamed,
            networkManager, &NetworkManager::entityRenamed);
    connect(hierarchy, &Hierarchy::entityUpdate,
            networkManager, &NetworkManager::entityUpdate);

    connect(hierarchy, &Hierarchy::componentAdded,
            networkManager, &NetworkManager::componentAdded);

    connect(hierarchy, &Hierarchy::entityMeshAdded,
            networkManager, &NetworkManager::entityMeshAdded);
    connect(hierarchy, &Hierarchy::entityMeshRemoved,
            networkManager, &NetworkManager::entityMeshRemoved);

    connect(hierarchy, &Hierarchy::entityPhysicsAdded,
            networkManager, &NetworkManager::entityPhysicsAdded);
    connect(hierarchy, &Hierarchy::entityPhysicsRemoved,
            networkManager, &NetworkManager::entityPhysicsRemoved);

    connect(networkManager,&NetworkManager::initData,hierarchy,&Hierarchy::fromJson);
    connect(networkManager,&NetworkManager::getCurrentJsonData,hierarchy,&Hierarchy::getCurrentJsonData);
    connect(hierarchy,&Hierarchy::getJsonData,networkManager,&NetworkManager::getJsonData);
    connect(networkManager,&NetworkManager::addEntityFromJson,hierarchy,&Hierarchy::addEntityViaNetwork);
    connect(networkManager,&NetworkManager::addComponent,hierarchy,&Hierarchy::addComponent);
    connect(networkManager,&NetworkManager::removeEntity,hierarchy,&Hierarchy::removeEntity);
}

Runtime::~Runtime() {
    delete recorder;
    // delete missionExcuter;
    delete console;
    delete networkManager;
    delete scenerenderer;
    delete simulation;
    delete Library;
    delete hierarchy;
}

void Runtime::handleStart() {
    if (simulation) simulation->start();
}

void Runtime::handleStop() {
    if (simulation) simulation->stop();
}

void Runtime::handleReplay() {
    if (simulation) {
        simulation->replay();
    }
}
