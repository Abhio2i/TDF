
#include "scenerenderer.h"
#include <core/Debug/console.h>

SceneRenderer::SceneRenderer() {
    meshes = new std::unordered_map<std::string, Mesh>();
}

void SceneRenderer::entityAdded(QString /*parentID*/, Entity* entity) {
    Platform* platform = dynamic_cast<Platform*>(entity);
    if (!platform) {
        Console::error("Entity is not a Platform");
        return;
    }

    if (!platform->transform || !platform->collider || !platform->meshRenderer2d) {
        Console::error("Required components missing for entity: " + platform->Name);
        return;
    }

    MeshData meshData;
    meshData.name = QString::fromStdString(platform->Name);
    meshData.transform = platform->transform;
    meshData.collider = platform->collider;
    meshData.trajectory = platform->trajectory;
    meshData.Meshes = platform->meshRenderer2d->Meshes;
    emit addMesh(QString::fromStdString(platform->ID), meshData);
}

void SceneRenderer::entityRemoved(QString ID) {
    emit removeMesh(ID);
    std::string key = ID.toStdString();
    auto it = meshes->begin();
    while (it != meshes->end()) {
        if (it->first.find(key) == 0) {
            it = meshes->erase(it);
        } else {
            ++it;
        }
    }
}

void SceneRenderer::cleanBuffer() {
    meshes->clear();
    Console::log("Mesh buffer cleared");
}

void SceneRenderer::addMeshBuffer() {
    Console::log("Adding mesh to buffer");
}

void SceneRenderer::removeBuffer() {
    Console::log("Removing mesh from buffer");
}

void SceneRenderer::getSnapshot() {
    Console::log("Snapshot requested");
}
