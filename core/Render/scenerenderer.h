// #ifndef SCENERENDERER_H
// #define SCENERENDERER_H
// // #include "qtimer.h"
// #include <QObject>
// #include <QElapsedTimer>
// #include <core/Hierarchy/Components/mesh.h>
// #include <core/Hierarchy/Components/transform.h>
// #include <core/Hierarchy/entity.h>
// #include <core/Hierarchy/Components/collider.h>


// struct MeshData {
//     QString name;
//     Transform *transform;
//     Collider *collider;
//     std::vector<Mesh*> Meshes;
// };

// class SceneRenderer: public QObject
// {
//     Q_OBJECT
// public:
//     SceneRenderer();
//     std::unordered_map<std::string, Mesh> *meshes;

//     // void setUpdateInterval(int intervalMs);
//     void cleanBuffer();
//     void addMeshBuffer();
//     void removeBuffer();
//     void getSnapshot();


// public slots:
//     void entityAdded(QString parentID,Entity* entity);
//     void entityRemoved(QString ID);
// signals:
//     void Render(float deltaTime);
//     void DeltaTimeUpdated(float deltaTime);
//     void addMesh(QString ID,MeshData meshData);
//     void removeMesh(QString ID);
// private:
//     // QTimer *updateTimer;
//     // float deltaTime;
//     // QElapsedTimer* elapsedTimer;
//     // qint64 lastTime;
// };

// #endif // SCENERENDERER_H
#ifndef SCENERENDERER_H
#define SCENERENDERER_H

#include <QObject>
#include <QElapsedTimer>
#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Components/transform.h>
#include <core/Hierarchy/entity.h>
#include <core/Hierarchy/EntityProfiles/platform.h>
#include <core/Hierarchy/Components/collider.h>

struct MeshData {
    QString name;
    Transform *transform;
    Collider *collider;
    std::vector<Mesh*> Meshes;
};

class SceneRenderer : public QObject {
    Q_OBJECT
public:
    SceneRenderer();
    std::unordered_map<std::string, Mesh> *meshes;

    void cleanBuffer();
    void addMeshBuffer();
    void removeBuffer();
    void getSnapshot();

public slots:
    void entityAdded(QString parentID, Entity* entity);
    void entityRemoved(QString ID);

signals:
    void Render(float deltaTime);
    void DeltaTimeUpdated(float deltaTime);
    void addMesh(QString ID, MeshData meshData);
    void removeMesh(QString ID);
};

#endif // SCENERENDERER_H
