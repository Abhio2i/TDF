#include "scenario.h"
#include <QJsonDocument>

#include <core/Debug/console.h>
#include <core/Hierarchy/profilecategaory.h>
#include <core/Hierarchy/folder.h>
#include <core/Hierarchy/entity.h>
Scenario::Scenario() {
    hierarchy = new Hierarchy();
    Library = new Hierarchy();
    scenerenderer = new SceneRenderer();
    console  = Console::internalInstance();
    scriptengine = new ScriptEngine();
    //scriptengine->setHierarchy(hierarchy);
    connect(hierarchy,&Hierarchy::entityMeshAdded,scenerenderer,&SceneRenderer::entityAdded);
    connect(hierarchy,&Hierarchy::entityMeshRemoved,scenerenderer,&SceneRenderer::entityRemoved);

}
