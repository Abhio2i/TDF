


#ifndef SCENERENDERER_H
#define SCENERENDERER_H

#include <QObject>
#include <QElapsedTimer>
#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Components/transform.h>
#include <core/Hierarchy/entity.h>
#include <core/Hierarchy/EntityProfiles/platform.h>
#include <core/Hierarchy/Components/collider.h>
#include <core/Hierarchy/Components/trajectory.h>

struct MeshData {
    QString name;
    Transform *transform;
    Collider *collider;
    Trajectory *trajectory;
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
