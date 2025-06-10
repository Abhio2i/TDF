#include "runtime.h"

Runtime::Runtime() {
    hierarchy = new Hierarchy();
    Library = new Hierarchy();
    scenerenderer = new SceneRenderer();
    simulation = new Simulation();
    networkManager = new NetworkManager();
    console  = Console::internalInstance();

    connect(hierarchy,&Hierarchy::entityMeshAdded,scenerenderer,&SceneRenderer::entityAdded);
    connect(hierarchy,&Hierarchy::entityMeshRemoved,scenerenderer,&SceneRenderer::entityRemoved);
    connect(simulation,&Simulation::Render,scenerenderer,&SceneRenderer::Render);

    connect(hierarchy,&Hierarchy::entityPhysicsAdded,simulation,&Simulation::entityAdded);
    connect(hierarchy,&Hierarchy::entityPhysicsRemoved,simulation,&Simulation::entityRemoved);
    connect(hierarchy,&Hierarchy::entityUpdate,simulation,&Simulation::entityUpdate);
}
