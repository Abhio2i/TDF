
#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H
#include <QWidget>
#include <QPainter>
#include <core/Hierarchy/Struct/vector.h>
#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Components/collider.h>
#include <QMouseEvent>
/* MeshEntry structure section */
struct MeshEntry {
    QString name;
    Vector* position;
    Vector* rotation;
    Vector* velocity;
    Vector* size;
    Mesh* mesh;
    Collider* collider;
};
/* TransformMode enumeration section */
enum TransformMode {
    Panning,
    Translate,
    Rotate,
    Scale
};
/* Class declaration section */
class CanvasWidget : public QWidget {
    Q_OBJECT
public:
        /* Constructor section */
    CanvasWidget(QWidget *parent = nullptr);
        /* Public members section */
    std::unordered_map<std::string, MeshEntry> Meshes;
    std::string selectedEntityId;
    TransformMode currentMode = Translate;
      /* Rendering and mode control section */
    void Render(float deltatime);
    void setTransformMode(TransformMode mode);
  /* Simulation and editor control section */
    void simulation();
    void editor();
   /* Grid visibility and settings section */
    void setXGridVisible(bool visible) { showXGrid = visible; update(); }
    void setYGridVisible(bool visible) { showYGrid = visible; update(); }
    void setGridOpacity(int opacity) { gridOpacity = opacity; update(); }

    void toggleColliders(bool show) { showColliders = show; update(); }
    void toggleMesh(bool show) { showMesh = show; update(); }
    void toggleOutline(bool show) { showOutline = show; update(); }
    void toggleInformation(bool show) { showInformation = show; update(); }
    void toggleFPS(bool show) { showFPS = show; update(); }
    void toggleLayerVisibility(const QString& layer, bool visible);
public slots:
    // Slots to handle GISlib signals
    void onGISKeyPressed(QKeyEvent *event) { keyPressEvent(event); }
    void onGISMousePressed(QMouseEvent *event) { mousePressEvent(event); }
    void onGISMouseMoved(QMouseEvent *event) { mouseMoveEvent(event); }
    void onGISMouseReleased(QMouseEvent *event) { mouseReleaseEvent(event); }
    void onGISPainted(QPaintEvent *event) { paintEvent(event); }
private:
    void drawGridLines(QPainter& painter);
    void drawEntityInformation(QPainter& painter);
    void drawSceneInformation(QPainter& painter);
    void drawTransformGizmo(QPainter& painter);
    void drawCollider(QPainter& painter);
    void drawSelectionOutline(QPainter& painter);
    void drawMesh(QPainter& painter);
    void drawImage(QPainter& painter);

    void applyGravityAndBounce(float deltaTime);

    // New functions for event handling
    void handleMousePress(QMouseEvent *event);
    void handleMouseMove(QMouseEvent *event);
    void handleMouseRelease(QMouseEvent *event);
    void handleKeyPress(QKeyEvent *event);
    void handlePaint(QPaintEvent *event);

    //=========grid=======
    bool showXGrid = true;
    bool showYGrid = true;
    int gridOpacity = 50; // 0-100 range

    //===================
    bool showColliders = true;
    bool showMesh = true;
    bool showOutline = true;
    bool showInformation = true;
    bool showFPS = true;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void selectEntitybyCursor(QString ID);
    void MoveEntity(QString ID);

private:
    bool selectEntity;
    QTimer *updateTimer;
    QElapsedTimer fpsTimer;
    int frameCount = 0;
    float fps = 0.0f;

    float angle = 0.0; // for rotating line or sweep
    QPointF dragStartPos;
    QString activeDragAxis; // "x" or "y" or ""
    bool simulate = true;
    QPointF canvasOffset = QPointF(0, 0); // origin shift
    QPoint lastMousePos; // last mouse position for panning
    bool isPanning = false; // mouse dragging flag
};

#endif // CANVASWIDGET_H
