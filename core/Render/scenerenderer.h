// =============================================================================
// FILE:        scenerenderer.h
// MODULE:      Scene Rendering Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the MeshData struct and SceneRenderer class, which
//              manages rendering of simulation entities. Maintains a collection
//              of meshes, handles buffer operations, and provides signals for
//              entity addition/removal to update the render scene.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

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

// =============================================================================
// STRUCT: MeshData
// DESCRIPTION: Aggregates all renderable components for a single entity.
// =============================================================================
struct MeshData {
    QString name;                       //!< Entity name
    Transform *transform;               //!< Spatial transformation
    Collider *collider;                 //!< Collision geometry
    Trajectory *trajectory;             //!< Path following data
    DynamicModel *dynamicmodel;         //!< Motion dynamics
    std::vector<Mesh*> Meshes;          //!< Visual meshes to render
    Entity* entity;                     //!< Parent entity reference
};

// =============================================================================
// CLASS: SceneRenderer
//
// DESCRIPTION: Central render manager for the simulation. Maintains a map
//              of meshes, provides buffer operations, and emits signals for
//              rendering and entity lifecycle events.
// =============================================================================
class SceneRenderer : public QObject {
    Q_OBJECT
public:
    SceneRenderer();

    std::unordered_map<std::string, Mesh> *meshes;   //!< Map of mesh ID to Mesh object

    void cleanBuffer();     //!< Clears all mesh buffers
    void addMeshBuffer();   //!< Adds a new mesh buffer (implementation specific)
    void removeBuffer();    //!< Removes a buffer
    void getSnapshot();     //!< Captures a snapshot of current scene state

public slots:
    void entityAdded(QString parentID, Entity* entity);   //!< Called when entity is added to hierarchy
    void entityRemoved(QString ID);                       //!< Called when entity is removed

signals:
    void Render(float deltaTime);                         //!< Emitted each frame with delta time
    void DeltaTimeUpdated(float deltaTime);               //!< Emitted when delta time changes
    void addMesh(QString ID, MeshData meshData);          //!< Requests addition of a mesh to renderer
    void removeMesh(QString ID);                          //!< Requests removal of a mesh
};

#endif // SCENERENDERER_H
