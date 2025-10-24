#include "runtime.h"
#include <QDebug>

Runtime::Runtime() {
    Library = new Hierarchy();
    hierarchy = new Hierarchy();

    scenerenderer = new SceneRenderer();
    simulation = new Simulation();
    scriptengine = new ScriptEngine();
    networkManager = new NetworkManager();
    console  = Console::internalInstance();
    recorder = new Recorder(hierarchy, simulation);



    // Rendering pipeline
    connect(hierarchy,&Hierarchy::entityMeshAdded,scenerenderer,&SceneRenderer::entityAdded);
    connect(hierarchy,&Hierarchy::entityMeshRemoved,scenerenderer,&SceneRenderer::entityRemoved);
    connect(simulation,&Simulation::Render,scenerenderer,&SceneRenderer::Render);

    // Physics system connections
    connect(hierarchy,&Hierarchy::entityPhysicsAdded,simulation,&Simulation::entityAdded);
    connect(hierarchy,&Hierarchy::entityPhysicsRemoved,simulation,&Simulation::entityRemoved);
    connect(hierarchy,&Hierarchy::entityUpdate,simulation,&Simulation::entityUpdate);

    // NetworkManager connections
    networkManager->setHierarchy(hierarchy);

    connect(hierarchy, &Hierarchy::folderAdded,
            networkManager, &NetworkManager::folderAdded);
    connect(hierarchy, &Hierarchy::folderRemoved,
            networkManager, &NetworkManager::folderRemoved);

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

    connect(hierarchy, &Hierarchy::entityComponentUpdate,
            networkManager, &NetworkManager::entityComponentsUpdate);

    connect(hierarchy, &Hierarchy::entityMeshAdded,
            networkManager, &NetworkManager::entityMeshAdded);
    connect(hierarchy, &Hierarchy::entityMeshRemoved,
            networkManager, &NetworkManager::entityMeshRemoved);

    connect(hierarchy, &Hierarchy::entityPhysicsAdded,
            networkManager, &NetworkManager::entityPhysicsAdded);
    connect(hierarchy, &Hierarchy::entityPhysicsRemoved,
            networkManager, &NetworkManager::entityPhysicsRemoved);

    connect(simulation, &Simulation::Update,
            networkManager, &NetworkManager::UpdateClient);

    connect(networkManager,&NetworkManager::initData,hierarchy,&Hierarchy::fromJson);
    connect(networkManager,&NetworkManager::getCurrentJsonData,hierarchy,&Hierarchy::getCurrentJsonData);
    connect(hierarchy,&Hierarchy::getJsonData,networkManager,&NetworkManager::getJsonData);
    connect(networkManager,&NetworkManager::addFolder,hierarchy,&Hierarchy::addFolderViaNetwork);
    connect(networkManager,&NetworkManager::removeFolder,hierarchy,&Hierarchy::removeFolderViaNetwork);
    connect(networkManager,&NetworkManager::addEntity,hierarchy,&Hierarchy::addEntityViaNetwork);
    connect(networkManager,&NetworkManager::addComponent,hierarchy,&Hierarchy::addComponent);
    connect(networkManager,&NetworkManager::entityComponentUpdate,hierarchy,&Hierarchy::UpdateComponent);
    connect(networkManager,&NetworkManager::removeEntity,hierarchy,&Hierarchy::removeEntity);
    connect(networkManager,&NetworkManager::updateScene,scenerenderer,&SceneRenderer::Render);
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
