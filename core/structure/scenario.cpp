#include "scenario.h"
#include <QJsonDocument>

#include <core/Debug/console.h>
#include <core/structure/profilecategaory.h>
#include <core/structure/folder.h>
#include <core/structure/entity.h>
Scenario::Scenario() {
    hierarchy = new Hierarchy();
    Library = new Hierarchy();
    scenerenderer = new SceneRenderer();
    console  = Console::internalInstance();
    connect(hierarchy,&Hierarchy::entityMeshAdded,scenerenderer,&SceneRenderer::entityAdded);
    connect(hierarchy,&Hierarchy::entityMeshRemoved,scenerenderer,&SceneRenderer::entityRemoved);

}
