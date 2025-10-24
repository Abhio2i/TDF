/* ========================================================================= */
/* File: scene3dwidget.h                                                    */
/* Purpose: Defines widget for 3D scene visualization                        */
/* ========================================================================= */

#ifndef SCENE3DWIDGET_H
#define SCENE3DWIDGET_H

#include "qdirectionallight.h"                    // For directional light
#include "qgeometryrenderer.h"                    // For geometry rendering
#include "qphongmaterial.h"                       // For Phong material
#include "qtexture.h"                             // For texture handling
#include <QWidget>                                // For widget base class
#include <QSet>                                   // For set container
#include <QKeyEvent>                              // For key event handling
#include <core/Hierarchy/Struct/vector.h>         // For vector structure
#include <core/Hierarchy/Components/mesh.h>       // For mesh component
#include <core/Hierarchy/Components/collider.h>   // For collider component
#include <Qt3DExtras/Qt3DWindow>                  // For 3D window
#include <Qt3DCore/QEntity>                       // For 3D entity
#include <Qt3DCore/QTransform>                    // For 3D transform
#include <Qt3DRender/QMaterial>                   // For material rendering

// %%% Mesh3dEntry Structure %%%
/* Structure for 3D entity data */
struct Mesh3dEntry {
    QString name;                                 // Entity name
    Qt3DCore::QTransform* transform;              // Transform component
    QVector3D* position;                          // Position vector
    QQuaternion* rotation;                        // Rotation quaternion
    QVector3D* velocity;                          // Velocity vector
    QVector3D* size;                              // Size vector
    Mesh* mesh;                                   // Mesh component
    Collider* collider;                           // Collider component
    Qt3DRender::QMaterial* originalMaterial;      // Original material
    Qt3DCore::QEntity *entity;                    // 3D entity
};

// %%% RenderMode Enum %%%
/* Enum for rendering modes */
enum class RenderMode {
    Full,                                         // Full rendering
    Wireframe,                                    // Wireframe rendering
    WireframeWithDraw,                            // Wireframe with draw
    WireframeWithThickness                        // Wireframe with thickness
};

// %%% Class Definition %%%
/* Widget for 3D scene visualization */
class Scene3DWidget : public QWidget
{
    Q_OBJECT

public:
    // Initialize 3D widget
    explicit Scene3DWidget(QWidget *parent = nullptr);
    // Create grid lines
    Qt3DCore::QEntity* createGridLines(Qt3DCore::QEntity *parent, int size, int spacing);
    // Current rendering mode
    RenderMode currentRenderMode = RenderMode::Full;
    // Apply render mode
    void applyRenderMode(Qt3DRender::QGeometryRenderer* renderer);
    // Set render mode for all
    void setRenderModeForAll(RenderMode mode);
    // Clear render buffer
    void clearBuffer();

protected:
    // %%% Event Handlers %%%
    // Handle key press events
    void keyPressEvent(QKeyEvent *event) override;
    // Handle key release events
    void keyReleaseEvent(QKeyEvent *event) override;
    // Handle event filtering
    bool eventFilter(QObject *watched, QEvent *event) override;
    // Handle mouse press events
    void mousePressEvent(QMouseEvent *event) override;
    // Handle mouse release events
    void mouseReleaseEvent(QMouseEvent *event) override;
    // Handle mouse move events
    void mouseMoveEvent(QMouseEvent *event) override;

public slots:
    // Focus on entity by ID
    void focusEntity(QString ID);
    // Update all entities
    void updateEntities();
    // Add entity with Mesh3dEntry
    void addEntity(QString ID, Mesh3dEntry);
    // Remove entity by ID
    void removeEntity(QString ID);
    // Move entity by ID
    void MoveEntity(QString ID);
    // Select entity by ID
    void selectedEntity(QString ID);
    // Update camera position
    void updateCamera();
    // Update camera rotation
    void updateCameraRotation();

signals:
    // Signal entity selection by cursor
    void selectEntityByCursor(QString ID);

private:
    // %%% Data Members %%%
    // Map of 3D entities
    std::unordered_map<std::string, Mesh3dEntry> entities3D;
    // Selected entity ID
    QString selectedEntityId;
    // 3D window view
    Qt3DExtras::Qt3DWindow *view;
    // Root entity for scene
    Qt3DCore::QEntity *rootEntity;
    // Create scene root
    Qt3DCore::QEntity *createScene();
    // Set of pressed keys
    QSet<int> pressedKeys;
    // Last mouse position
    QPoint lastMousePos;
    // Camera yaw angle
    float yaw = 0.0f;
    // Camera pitch angle
    float pitch = 0.0f;
    // Right mouse button state
    bool rightMousePressed = false;
    // Follow selected entity flag
    bool followSelectedEntity = false;
    // Middle mouse button state
    bool middleMousePressed = false;
    // Last middle mouse position
    QPoint lastMiddlePos;
    // Main directional light
    Qt3DRender::QDirectionalLight *mainLight = nullptr;
    // Ground entity
    Qt3DCore::QEntity *groundEntity = nullptr;
    // Shared highlight material
    Qt3DExtras::QPhongMaterial *sharedHighlightMaterial = nullptr;
    // Texture cache
    std::unordered_map<QString, Qt3DRender::QTexture2D*> textureCache;
};

#endif // SCENE3DWIDGET_H
