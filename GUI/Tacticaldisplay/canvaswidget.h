#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H
#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include <QWidget>
#include <QPainter>
#include <core/Hierarchy/Struct/vector.h>
#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Components/collider.h>
#include <core/Hierarchy/Components/trajectory.h>
#include <QMouseEvent>
#include <QJsonArray>
#include <QElapsedTimer>
#include <Qt3DCore/QTransform>

/* MeshEntry structure section */
struct MeshEntry {
    QString name;
    Qt3DCore::QTransform* transform;
    QVector3D* position;
    QQuaternion* rotation;
    QVector3D* velocity;
    QVector3D* size;
    Mesh* mesh;
    Collider* collider;
    Trajectory* trajectory;
    QString bitmapPath;
    QString text;
};

/* TransformMode enumeration section */
enum TransformMode {
    Panning,
    Translate,
    Rotate,
    Scale,
    DrawTrajectory,
    DrawShape,
    PlaceBitmap,
    MeasureDistance,
    EditShape
};

/* Class declaration section */
class CanvasWidget : public QWidget {
    Q_OBJECT
public:
    /* Constructor section */
    CanvasWidget(QWidget *parent = nullptr);
    GISlib* gislib;
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
    void wheelEvent(QWheelEvent *event) override;

    QString selectedShape;
    void setShapeDrawingMode(bool enabled, const QString& shapeType = "");

    std::vector<MeshEntry> tempMeshes;
    void onBitmapImageSelected(const QString& filePath);
    void onBitmapSelected(const QString& bitmapType);
    void selectWaypoint(int index);
    void deselectWaypoint();
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
public slots:
    void onGISKeyPressed(QKeyEvent *event) { keyPressEvent(event); }
    void onGISMousePressed(QMouseEvent *event) { mousePressEvent(event); }
    void onGISMouseMoved(QMouseEvent *event) { mouseMoveEvent(event); }
    void onGISMouseReleased(QMouseEvent *event) { mouseReleaseEvent(event); }
    void onGISPainted(QPaintEvent *event) { paintEvent(event); }
    void updateWaypointsFromInspector(QString entityId, QJsonArray waypoints);
    void onDistanceMeasured(double distance, QPointF startPoint, QPointF endPoint);
    void onPresetLayerSelected(const QString& preset);

public:
    void dragEnterEvents(QDragEnterEvent *event) ;
    void dragMoveEvents(QDragMoveEvent *event) ;
    void dropEvents(QDropEvent *event) ;

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
    QString selectedBitmapType;
    bool isPlacingBitmap = false;
    std::vector<Waypoints*> currentTrajectory;
    std::vector<Vector*> tempPolygonVertices;
    std::vector<QPointF> tempPolygonCanvasPoints;
    std::vector<Vector*> tempLineVertices;
    QString getBitmapImagePath(const QString& bitmapType);
    std::vector<QPointF> tempLineCanvasPoints;
    int selectedWaypointIndex;
    bool isDraggingWaypoint;
    QString editingShapeId;
    int selectedHandleIndex;
    bool isResizingShape;
    std::vector<QPointF> resizeHandles;
    bool isPointInPolygon(const QPointF& point, const std::vector<Vector*>& vertices, const QPointF& centroidGeo, GISlib* gislib); // Added declaration
    void handleRightClick(QMouseEvent *event);
    void handleShapeDrawing(QMouseEvent *event);
    void handleTrajectoryRightClick(QMouseEvent *event);
    void handleShapeRightClick(QMouseEvent *event);
    // for presetlayes
    std::vector<std::pair<double, double>> airbasePositions;  // lon, lat pairs
    bool showAirbases = false;  // Toggle flag
    QString airbaseIconPath = ":/icons/images/airbase.png";
    QPixmap airbasePixmap;  // Cached pixmap for efficiency

    void loadAirbaseData();  // Load static/dynamic data
    void drawAirbases(QPainter& painter);  // New drawing method

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
    void airbaseLayerToggled(bool visible);
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
    int findNearestWaypoint(QPointF canvasPos);
    void updateTrajectoryData();
};
#endif // CANVASWIDGET_H
