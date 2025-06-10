#ifndef SCENE3DWIDGET_H
#define SCENE3DWIDGET_H

#include "qdirectionallight.h"
#include "qgeometryrenderer.h"
#include "qphongmaterial.h"
#include "qtexture.h"
#include <QWidget>
#include <QSet>
#include <QKeyEvent>
#include <core/Struct/vector.h>
#include <core/Components/mesh.h>
#include <core/Components/collider.h>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QMaterial>



struct Mesh3dEntry {
    QString name;
    Vector* position;
    Vector* rotation;
    Vector* velocity;
    Vector* size;
    Mesh* mesh;
    Collider* collider;
    Qt3DRender::QMaterial* originalMaterial = nullptr;
    Qt3DCore::QEntity *entity;
};

enum class RenderMode {
    Full,
    Wireframe,
    WireframeWithDraw,
    WireframeWithThickness
};

class Scene3DWidget : public QWidget
{
    Q_OBJECT
public:

    explicit Scene3DWidget(QWidget *parent = nullptr);
    Qt3DCore::QEntity* createGridLines(Qt3DCore::QEntity *parent, int size, int spacing);
    RenderMode currentRenderMode = RenderMode::Full;

    void applyRenderMode(Qt3DRender::QGeometryRenderer* renderer);
    void setRenderModeForAll(RenderMode mode);
    void clearBuffer();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

public slots:
    void focusEntity(QString ID);
    void updateEntities();
    void addEntity(QString ID, Mesh3dEntry);
    void removeEntity(QString ID);
    void MoveEntity(QString ID);
    void selectedEntity(QString ID);
    void updateCamera();
    void updateCameraRotation();

signals:
    void selectEntityByCursor(QString ID);

private:
    std::unordered_map<std::string, Mesh3dEntry> entities3D;
    QString selectedEntityId;
    Qt3DExtras::Qt3DWindow *view;
    Qt3DCore::QEntity *rootEntity;
    Qt3DCore::QEntity *createScene();
    QSet<int> pressedKeys;
    QPoint lastMousePos;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool rightMousePressed = false;
    bool followSelectedEntity = false;
    bool middleMousePressed = false;
    QPoint lastMiddlePos;


    Qt3DRender::QDirectionalLight *mainLight = nullptr;
    Qt3DCore::QEntity *groundEntity = nullptr;
    Qt3DExtras::QPhongMaterial *sharedHighlightMaterial = nullptr;
    std::unordered_map<QString, Qt3DRender::QTexture2D*> textureCache;

};

#endif // SCENE3DWIDGET_H
