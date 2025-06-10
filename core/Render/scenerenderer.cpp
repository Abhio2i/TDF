 #include "scenerenderer.h"

SceneRenderer::SceneRenderer() {
    // updateTimer = new QTimer(this);
    // elapsedTimer = new QElapsedTimer();
    // elapsedTimer->start();
    // lastTime = elapsedTimer->elapsed();

    // connect(updateTimer, &QTimer::timeout, this, [=]() {
    //     qint64 currentTime = elapsedTimer->elapsed();
    //     float deltaTime = (currentTime - lastTime) / 1000.0f; // in seconds
    //     lastTime = currentTime;
    //     // Optional: emit deltaTime if needed somewhere else
    //     emit DeltaTimeUpdated(deltaTime);
    //     QTimer::singleShot(0, this, [=]() {
    //         emit this->Render(deltaTime);
    //     });

    // });

    // updateTimer->start(1); // ~100 FPS, adjust as needed
}

void SceneRenderer::entityAdded(QString parentID,Entity* entity){
    MeshData meshData;
    meshData.name = QString::fromStdString(entity->Name);
    meshData.transform = entity->transform;
    meshData.collider = entity->collider;
    meshData.Meshes = entity->meshRenderer2d->Meshes;
    emit addMesh(QString::fromStdString(entity->ID),meshData);
}

void SceneRenderer::entityRemoved(QString ID){
    emit removeMesh(ID);
}

