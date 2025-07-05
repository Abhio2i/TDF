
// #ifndef CANVASWIDGET_H
// #define CANVASWIDGET_H
// #include <QWidget>
// #include <QPainter>
// #include <core/Hierarchy/Struct/vector.h>
// #include <core/Hierarchy/Components/mesh.h>
// #include <core/Hierarchy/Components/collider.h>
// #include <core/Hierarchy/Components/trajectory.h>
// #include <QMouseEvent>
// #include <QJsonArray> // Added for waypoint data

// /* MeshEntry structure section */
// struct MeshEntry {
//     QString name;
//     Vector* position;
//     Vector* rotation;
//     Vector* velocity;
//     Vector* size;
//     Mesh* mesh;
//     Collider* collider;
//     Trajectory* trajectory;
// };

// /* TransformMode enumeration section */
// enum TransformMode {
//     Panning,
//     Translate,
//     Rotate,
//     Scale,
//     DrawTrajectory
// };

// /* Class declaration section */
// class CanvasWidget : public QWidget {
//     Q_OBJECT
// public:
//     /* Constructor section */
//     CanvasWidget(QWidget *parent = nullptr);
//     /* Public members section */
//     std::unordered_map<std::string, MeshEntry> Meshes;
//     std::string selectedEntityId;
//     TransformMode currentMode = Translate;
//     /* Rendering and mode control section */
//     void Render(float deltatime);
//     void setTransformMode(TransformMode mode);
//     void setTrajectoryDrawingMode(bool enabled);
//     void saveTrajectory(); // Function to save waypoints
//     /* Simulation and editor control section */
//     void simulation();
//     void editor();
//     /* Grid visibility and settings section */
//     void setXGridVisible(bool visible) { showXGrid = visible; update(); }
//     void setYGridVisible(bool visible) { showYGrid = visible; update(); }
//     void setGridOpacity(int opacity) { gridOpacity = opacity; update(); }
//     void toggleColliders(bool show) { showColliders = show; update(); }
//     void toggleMesh(bool show) { showMesh = show; update(); }
//     void toggleOutline(bool show) { showOutline = show; update(); }
//     void toggleInformation(bool show) { showInformation = show; update(); }
//     void toggleFPS(bool show) { showFPS = show; update(); }
//     void toggleLayerVisibility(const QString& layer, bool visible);
// public slots:
//     // Slots to handle GISlib signals
//     void onGISKeyPressed(QKeyEvent *event) { keyPressEvent(event); }
//     void onGISMousePressed(QMouseEvent *event) { mousePressEvent(event); }
//     void onGISMouseMoved(QMouseEvent *event) { mouseMoveEvent(event); }
//     void onGISMouseReleased(QMouseEvent *event) { mouseReleaseEvent(event); }
//     void onGISPainted(QPaintEvent *event) { paintEvent(event); }
//     // void onGISZoomChanged(int zoom); // New slot for zoom changes

//     void updateWaypointsFromInspector(QString entityId, QJsonArray waypoints);

// private:
//     void drawGridLines(QPainter& painter);
//     void drawEntityInformation(QPainter& painter);
//     void drawSceneInformation(QPainter& painter);
//     void drawTransformGizmo(QPainter& painter);
//     void drawCollider(QPainter& painter);
//     void drawSelectionOutline(QPainter& painter);
//     void drawMesh(QPainter& painter);
//     void drawImage(QPainter& painter);
//     void drawTrajectory(QPainter& painter);
//     void applyGravityAndBounce(float deltaTime);
//     // New functions for event handling
//     void handleMousePress(QMouseEvent *event);
//     void handleMouseMove(QMouseEvent *event);
//     void handleMouseRelease(QMouseEvent *event);
//     void handleKeyPress(QKeyEvent *event);
//     void handlePaint(QPaintEvent *event);
//     float zoomLevel = 1.0f; // New zoom level property
//     //=========grid=======
//     bool showXGrid = true;
//     bool showYGrid = true;
//     int gridOpacity = 50;
//     //===================
//     bool showColliders = true;
//     bool showMesh = true;
//     bool showOutline = true;
//     bool showInformation = true;
//     bool showFPS = true;
//     // Trajectory drawing state
//     bool isDrawingTrajectory = false;
//     std::vector<Waypoints*> currentTrajectory;

// protected:
//     void keyPressEvent(QKeyEvent *event) override;
//     void paintEvent(QPaintEvent *event) override;
//     void mousePressEvent(QMouseEvent *event) override;
//     void mouseMoveEvent(QMouseEvent *event) override;
//     void mouseReleaseEvent(QMouseEvent *event) override;
// signals:
//     void selectEntitybyCursor(QString ID);
//     void MoveEntity(QString ID);
//     void trajectoryUpdated(QString entityId, QJsonArray waypoints); // Updated signal with waypoints

// private:
//     bool selectEntity;
//     QTimer *updateTimer;
//     QElapsedTimer fpsTimer;
//     int frameCount = 0;
//     float fps = 0.0f;
//     float angle = 0.0;
//     QPointF dragStartPos;
//     QString activeDragAxis;
//     bool simulate = true;
//     QPointF canvasOffset = QPointF(0, 0);
//     QPoint lastMousePos;
//     bool isPanning = false;
// };
// #endif // CANVASWIDGET_H




#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H
#include <QWidget>
#include <QPainter>
#include <core/Hierarchy/Struct/vector.h>
#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Components/collider.h>
#include <core/Hierarchy/Components/trajectory.h>
#include <QMouseEvent>
#include <QJsonArray>

/* MeshEntry structure section */
struct MeshEntry {
    QString name;
    Vector* position;
    Vector* rotation;
    Vector* velocity;
    Vector* size;
    Mesh* mesh;
    Collider* collider;
    Trajectory* trajectory;
};

/* TransformMode enumeration section */
enum TransformMode {
    Panning,
    Translate,
    Rotate,
    Scale,
    DrawTrajectory
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
    void setTrajectoryDrawingMode(bool enabled);
    void saveTrajectory();
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
    void onGISKeyPressed(QKeyEvent *event) { keyPressEvent(event); }
    void onGISMousePressed(QMouseEvent *event) { mousePressEvent(event); }
    void onGISMouseMoved(QMouseEvent *event) { mouseMoveEvent(event); }
    void onGISMouseReleased(QMouseEvent *event) { mouseReleaseEvent(event); }
    void onGISPainted(QPaintEvent *event) { paintEvent(event); }
    void updateWaypointsFromInspector(QString entityId, QJsonArray waypoints);
private:
    void drawGridLines(QPainter& painter);
    void drawEntityInformation(QPainter& painter);
    void drawSceneInformation(QPainter& painter);
    void drawTransformGizmo(QPainter& painter);
    void drawCollider(QPainter& painter);
    void drawSelectionOutline(QPainter& painter);
    void drawMesh(QPainter& painter);
    void drawImage(QPainter& painter);
    void drawTrajectory(QPainter& painter);
    void applyGravityAndBounce(float deltaTime);
    void handleMousePress(QMouseEvent *event);
    void handleMouseMove(QMouseEvent *event);
    void handleMouseRelease(QMouseEvent *event);
    void handleKeyPress(QKeyEvent *event);
    void handlePaint(QPaintEvent *event);
    float zoomLevel = 1.0f;
    bool showXGrid = true;
    bool showYGrid = true;
    int gridOpacity = 50;
    bool showColliders = true;
    bool showMesh = true;
    bool showOutline = true;
    bool showInformation = true;
    bool showFPS = true;
    bool isDrawingTrajectory = false;
    std::vector<Waypoints*> currentTrajectory;
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
signals:
    void selectEntitybyCursor(QString ID);
    void MoveEntity(QString ID);
    void trajectoryUpdated(QString entityId, QJsonArray waypoints);
private:
    bool selectEntity;
    QTimer *updateTimer;
    QElapsedTimer fpsTimer;
    int frameCount = 0;
    float fps = 0.0f;
    float angle = 0.0;
    QPointF dragStartPos;
    QString activeDragAxis;
    bool simulate = true;
    QPointF canvasOffset = QPointF(0, 0);
    QPoint lastMousePos;
    bool isPanning = false;
};
#endif // CANVASWIDGET_H
