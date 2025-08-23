
#include "scenerenderer.h"
#include "core/Hierarchy/EntityProfiles/fixedpoints.h"
#include "core/Hierarchy/EntityProfiles/specialzone.h"
#include <core/Debug/console.h>

SceneRenderer::SceneRenderer() {
    meshes = new std::unordered_map<std::string, Mesh>();
}

void SceneRenderer::entityAdded(QString /*parentID*/, Entity* entity) {
    Platform* platform = dynamic_cast<Platform*>(entity);
    if (platform) {
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
    Specialzone* zone = dynamic_cast<Specialzone*>(entity);
    if (zone) {
        if (!zone->transform || !zone->collider || !zone->meshRenderer2d) {
            Console::error("Required components missing for entity: " + zone->Name);
            return;
        }
        MeshData meshData;
        meshData.name = QString::fromStdString(zone->Name);
        meshData.transform = zone->transform;
        meshData.collider = zone->collider;
        meshData.trajectory = nullptr;
        meshData.Meshes = zone->meshRenderer2d->Meshes;
        emit addMesh(QString::fromStdString(zone->ID), meshData);
    }
    FixedPoints* point = dynamic_cast<FixedPoints*>(entity);
    if (point) {
        if (!point->transform || !point->collider || !point->meshRenderer2d) {
            Console::error("Required components missing for entity: " + point->Name);
            return;
        }
        MeshData meshData;
        meshData.name = QString::fromStdString(point->Name);
        meshData.transform = point->transform;
        meshData.collider = point->collider;
        meshData.trajectory = nullptr;
        meshData.Meshes = point->meshRenderer2d->Meshes;
        emit addMesh(QString::fromStdString(point->ID), meshData);
    }




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
