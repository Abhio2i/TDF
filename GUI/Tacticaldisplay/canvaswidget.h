// #ifndef CANVASWIDGET_H
// #define CANVASWIDGET_H
// #include "GUI/Tacticaldisplay/Gis/gislib.h"
// #include "GUI/measuredistance/measuredistancedialog.h"
// #include <QWidget>
// #include <QPainter>
// #include <core/Hierarchy/Struct/vector.h>
// #include <core/Hierarchy/Components/mesh.h>
// #include <core/Hierarchy/Components/collider.h>
// #include <core/Hierarchy/Components/trajectory.h>
// #include <QMouseEvent>
// #include <QJsonArray>
// #include <QElapsedTimer>
// #include <Qt3DCore/QTransform>

// /* MeshEntry structure section */
// struct MeshEntry {
//     QString name;
//     Qt3DCore::QTransform* transform;
//     QVector3D* position;
//     QQuaternion* rotation;
//     QVector3D* velocity;
//     QVector3D* size;
//     Mesh* mesh;
//     Collider* collider;
//     Trajectory* trajectory;
//     QString bitmapPath;
//     QString text;
// };

// /* TransformMode enumeration section */
// enum TransformMode {
//     Panning,
//     Translate,
//     Rotate,
//     Scale,
//     DrawTrajectory,
//     DrawShape,
//     PlaceBitmap,
//     MeasureDistance,
//     EditShape
// };

// /* Class declaration section */
// class CanvasWidget : public QWidget {
//     Q_OBJECT
// public:
//     /* Constructor section */
//     CanvasWidget(QWidget *parent = nullptr);
//     GISlib* gislib;
//     /* Public members section */
//     std::unordered_map<std::string, MeshEntry> Meshes;
//     std::string selectedEntityId;
//     TransformMode currentMode = Translate;
//     /* Rendering and mode control section */
//     void Render(float deltatime);
//     void setTransformMode(TransformMode mode);
//     void setTrajectoryDrawingMode(bool enabled);
//     void saveTrajectory();
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
//     void wheelEvent(QWheelEvent *event) override;

//     QString selectedShape;
//     void setShapeDrawingMode(bool enabled, const QString& shapeType = "");
//     void drawCircle(const QPointF& geoPos);
//     void drawRectangle(const QPointF& geoPos);
//     void drawLine(const QPointF& geoPos, bool finalize);
//     void drawPolygon(const QPointF& geoPos, bool finalize);
//     void drawPoints(const QPointF& geoPos);
//     void handleShapeDrawing(const QString& shapeType, const QPointF& geoPos, bool finalize);
//     std::vector<MeshEntry> tempMeshes;
//     void onBitmapImageSelected(const QString& filePath);
//     QString getBitmapImagePath(const QString& bitmapType);
//     void onBitmapSelected(const QString& bitmapType);
//     void selectWaypoint(int index);
//     void deselectWaypoint();
//     QJsonObject toJson() const;
//     void fromJson(const QJsonObject& json);
// public slots:
//     void onGISKeyPressed(QKeyEvent *event) { keyPressEvent(event); }
//     void onGISMousePressed(QMouseEvent *event) { mousePressEvent(event); }
//     void onGISMouseMoved(QMouseEvent *event) { mouseMoveEvent(event); }
//     void onGISMouseReleased(QMouseEvent *event) { mouseReleaseEvent(event); }
//     void onGISPainted(QPaintEvent *event) { paintEvent(event); }
//     void updateWaypointsFromInspector(QString entityId, QJsonArray waypoints);
//     void onDistanceMeasured(double distance, QPointF startPoint, QPointF endPoint);
//     void onPresetLayerSelected(const QString& preset);
//     // geojson function
//     void importGeoJsonLayer(const QString &filePath);
//     void onGeoJsonLayerToggled(const QString &layerName, bool visible);  // NEW: Slot for toggling GeoJSON layer

// private slots:
//     void onMeasurementTypeChanged(bool isEll);
// public:
//     void dragEnterEvents(QDragEnterEvent *event) ;
//     void dragMoveEvents(QDragMoveEvent *event) ;
//     void dropEvents(QDropEvent *event) ;
// public:
//     // Distance Measurement API for scripts
//     void startDistanceMeasurement(); // Enables MeasureDistance mode
//     void addMeasurePoint(double lon, double lat); // Adds a point manually
//     double getLastSegmentDistance() const; // Distance between last two points
//     double getTotalDistance() const;       // Total distance from first to last
//     void clearMeasurementPoints();         // Clears all points
//     void setMeasurementUnit(const QString &unit); // e.g., "m", "km", "ft", "mile"
//     QString measurementUnit = "m";      // default: meters
//     double conversionFactor = 1.0;      // default factor for meters
// private:
//     void drawGridLines(QPainter& painter);
//     void drawEntityInformation(QPainter& painter);
//     void drawSceneInformation(QPainter& painter);
//     void drawTransformGizmo(QPainter& painter);
//     void drawCollider(QPainter& painter);
//     void drawSelectionOutline(QPainter& painter);
//     void drawImage(QPainter& painter);
//     void drawTrajectory(QPainter& painter);
//     void handleMousePress(QMouseEvent *event);
//     void handleMouseMove(QMouseEvent *event);
//     void handleMouseRelease(QMouseEvent *event);
//     void handleKeyPress(QKeyEvent *event);
//     void handlePaint(QPaintEvent *event);
//     float zoomLevel = 1.0f;
//     bool showXGrid = true;
//     bool showYGrid = true;
//     int gridOpacity = 50;
//     bool showColliders = true;
//     bool showMesh = true;
//     bool showOutline = true;
//     bool showInformation = true;
//     bool showFPS = true;
//     bool isDrawingTrajectory = false;
//     QString selectedBitmapType;
//     bool isPlacingBitmap = false;
//     std::vector<Waypoints*> currentTrajectory;
//     std::vector<Vector*> tempPolygonVertices;
//     std::vector<QPointF> tempPolygonCanvasPoints;
//     int selectedWaypointIndex;
//     bool isDraggingWaypoint;
//     QString editingShapeId;
//     int selectedHandleIndex;
//     bool isResizingShape;
//     std::vector<QPointF> resizeHandles;
//     bool isPointInPolygon(const QPointF& point, const std::vector<Vector*>& vertices, const QPointF& centroidGeo, GISlib* gislib); // Added declaration
//     void handleRightClick(QMouseEvent *event);
//     void handleTrajectoryRightClick(QMouseEvent *event);
//     void handleShapeRightClick(QMouseEvent *event);
//     // for presetlayes
//     std::vector<std::pair<double, double>> airbasePositions;  // lon, lat pairs
//     bool showAirbases = false;  // Toggle flag
//     QString airbaseIconPath = ":/icons/images/airbase.png";
//     QPixmap airbasePixmap;  // Cached pixmap for efficiency

//     void loadAirbaseData();  // Load static/dynamic data
//     void drawAirbases(QPainter& painter);  // New drawing method

//     MeasureDistanceDialog* measureDialog = nullptr;
//     QList<QPointF> measurePoints; // geo points (lon, lat)

// protected:
//     void keyPressEvent(QKeyEvent *event) override;
//     void paintEvent(QPaintEvent *event) override;
//     void mousePressEvent(QMouseEvent *event) override;
//     void mouseMoveEvent(QMouseEvent *event) override;
//     void mouseReleaseEvent(QMouseEvent *event) override;
//     void drawMesh(QPainter& painter);

// signals:
//     void selectEntitybyCursor(QString ID);
//     void MoveEntity(QString ID);
//     void trajectoryUpdated(QString entityId, QJsonArray waypoints);
//     void airbaseLayerToggled(bool visible);
//     // Add missing signals geojson
//     void geoJsonLayerAdded(const QString& layerName);
//     void pointsUpdated(const QList<QPointF>& points);
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
//     int findNearestWaypoint(QPointF canvasPos);
//     void updateTrajectoryData();
//     // geojson
//     QMap<QString, bool> geoJsonLayers;
//     std::vector<Vector*> tempLineVertices;
//     std::vector<QPointF> tempLineCanvasPoints;

//     // Bitmap selection and dragging methods
//     bool isDraggingBitmap = false;
//     QString draggingBitmapId;
//     QPointF bitmapDragStartPos;
//     // USER UPLOADED IMAGE DRAGGING (NEW - separate)
//     bool isDraggingUserImage = false;
//     QString draggingUserImageId;
//     QPointF userImageDragStartPos;
//     // Shape dragging functionality
//     bool isDraggingShape = false;
//     QString draggingShapeId;
//     QPointF shapeDragStartPos;
//     // shape selection and dragging methods
//     bool handleShapeSelection(QMouseEvent *event);
//     void handleShapeDragging(QMouseEvent *event);
//     void stopShapeDragging();
//     // Bitmap selection and dragging methods
//     bool handleBitmapSelection(QMouseEvent *event);
//     void handleBitmapDragging(QMouseEvent *event);
//     void stopBitmapDragging();
//     // Separate methods for user images
//     bool handleUserImageSelection(QMouseEvent *event);
//     void handleUserImageDragging(QMouseEvent *event);
//     void stopUserImageDragging();
// };
// #endif // CANVASWIDGET_H


#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

// Include necessary libraries and headers
#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include "GUI/measuredistance/measuredistancedialog.h"
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
// Structure to hold mesh entity data including transform, mesh, collider, trajectory, etc.
struct MeshEntry {
    QString name;                           // Name identifier for the mesh
    Qt3DCore::QTransform* transform;        // 3D transformation
    QVector3D* position;                    // Position in 3D space
    QQuaternion* rotation;                  // Rotation as quaternion
    QVector3D* velocity;                    // Velocity vector
    QVector3D* size;                        // Size/scaling factors
    Mesh* mesh;                             // Mesh geometry data
    Collider* collider;                     // Collision detection component
    Trajectory* trajectory;                 // Path/trajectory data
    QString bitmapPath;                     // Path to bitmap image
    QString text;                           // Text content for text entities
};

/* TransformMode enumeration section */
// Enumeration for different transformation and interaction modes
enum TransformMode {
    Panning,           // Camera panning mode
    Translate,         // Object translation mode
    Rotate,            // Object rotation mode
    Scale,             // Object scaling mode
    DrawTrajectory,    // Trajectory drawing mode
    DrawShape,         // Shape drawing mode
    PlaceBitmap,       // Bitmap placement mode
    MeasureDistance,   // Distance measurement mode
    EditShape          // Shape editing mode
};

/* Class declaration section */
// Main canvas widget class for rendering and interaction
class CanvasWidget : public QWidget {
    Q_OBJECT  // Qt macro for signals/slots

public:
    /* Constructor section */
    CanvasWidget(QWidget *parent = nullptr);  // Constructor
    GISlib* gislib;  // GIS library instance for geographic operations

    /* Public members section */
    std::unordered_map<std::string, MeshEntry> Meshes;  // Storage for all mesh entities
    std::string selectedEntityId;  // Currently selected entity ID
    TransformMode currentMode = Translate;  // Current transformation mode

    /* Rendering and mode control section */
    void Render(float deltatime);  // Main rendering function
    void setTransformMode(TransformMode mode);  // Set transformation mode
    void setTrajectoryDrawingMode(bool enabled);  // Enable/disable trajectory drawing
    void saveTrajectory();  // Save current trajectory

    /* Simulation and editor control section */
    void simulation();  // Enter simulation mode
    void editor();      // Enter editor mode

    /* Grid visibility and settings section */
    void setXGridVisible(bool visible) { showXGrid = visible; update(); }  // Toggle X grid
    void setYGridVisible(bool visible) { showYGrid = visible; update(); }  // Toggle Y grid
    void setGridOpacity(int opacity) { gridOpacity = opacity; update(); }  // Set grid opacity
    void toggleColliders(bool show) { showColliders = show; update(); }  // Toggle collider visibility
    void toggleMesh(bool show) { showMesh = show; update(); }  // Toggle mesh visibility
    void toggleOutline(bool show) { showOutline = show; update(); }  // Toggle selection outline
    void toggleInformation(bool show) { showInformation = show; update(); }  // Toggle info display
    void toggleFPS(bool show) { showFPS = show; update(); }  // Toggle FPS display
    void toggleLayerVisibility(const QString& layer, bool visible);  // Toggle specific layer
    void wheelEvent(QWheelEvent *event) override;  // Handle mouse wheel events

    // Shape drawing related members and methods
    QString selectedShape;  // Currently selected shape type
    void setShapeDrawingMode(bool enabled, const QString& shapeType = "");  // Set shape drawing mode
    void drawCircle(const QPointF& geoPos);  // Draw circle at geographic position
    void drawRectangle(const QPointF& geoPos);  // Draw rectangle at geographic position
    void drawLine(const QPointF& geoPos, bool finalize);  // Draw line (multi-point)
    void drawPolygon(const QPointF& geoPos, bool finalize);  // Draw polygon (multi-point)
    void drawPoints(const QPointF& geoPos);  // Draw point marker
    void handleShapeDrawing(const QString& shapeType, const QPointF& geoPos, bool finalize);  // Main shape drawing handler
    std::vector<MeshEntry> tempMeshes;  // Temporary meshes for shapes and bitmaps

    // Bitmap/image related methods
    void onBitmapImageSelected(const QString& filePath);  // Handle user image selection
    QString getBitmapImagePath(const QString& bitmapType);  // Get path for preset bitmaps
    void onBitmapSelected(const QString& bitmapType);  // Handle preset bitmap selection

    // Waypoint and trajectory methods
    void selectWaypoint(int index);  // Select specific waypoint
    void deselectWaypoint();  // Deselect current waypoint

    // Serialization methods
    QJsonObject toJson() const;  // Convert state to JSON
    void fromJson(const QJsonObject& json);  // Load state from JSON

public slots:
    // GIS event forwarding slots
    void onGISKeyPressed(QKeyEvent *event) { keyPressEvent(event); }
    void onGISMousePressed(QMouseEvent *event) { mousePressEvent(event); }
    void onGISMouseMoved(QMouseEvent *event) { mouseMoveEvent(event); }
    void onGISMouseReleased(QMouseEvent *event) { mouseReleaseEvent(event); }
    void onGISPainted(QPaintEvent *event) { paintEvent(event); }

    // Data update slots
    void updateWaypointsFromInspector(QString entityId, QJsonArray waypoints);  // Update waypoints from UI
    void onDistanceMeasured(double distance, QPointF startPoint, QPointF endPoint);  // Handle distance measurement
    void onPresetLayerSelected(const QString& preset);  // Handle preset layer selection

    // GeoJSON functionality
    void importGeoJsonLayer(const QString &filePath);  // Import GeoJSON layer
    void onGeoJsonLayerToggled(const QString &layerName, bool visible);  // Toggle GeoJSON layer visibility

private slots:
    void onMeasurementTypeChanged(bool isEll);  // Handle measurement type change (ellipsoidal vs planar)

public:
    // Drag and drop event handlers
    void dragEnterEvents(QDragEnterEvent *event);
    void dragMoveEvents(QDragMoveEvent *event);
    void dropEvents(QDropEvent *event);

public:
    // Distance Measurement API for scripts
    void startDistanceMeasurement();  // Enables MeasureDistance mode
    void addMeasurePoint(double lon, double lat);  // Adds a point manually
    double getLastSegmentDistance() const;  // Distance between last two points
    double getTotalDistance() const;        // Total distance from first to last
    void clearMeasurementPoints();          // Clears all points
    void setMeasurementUnit(const QString &unit);  // Set measurement unit (e.g., "m", "km", "ft", "mile")

    QString measurementUnit = "m";      // Default: meters
    double conversionFactor = 1.0;      // Default factor for meters

private:
    // Drawing methods for different canvas elements
    void drawGridLines(QPainter& painter);  // Draw grid lines
    void drawEntityInformation(QPainter& painter);  // Draw entity info overlay
    void drawSceneInformation(QPainter& painter);  // Draw scene info (FPS, mode)
    void drawTransformGizmo(QPainter& painter);  // Draw transformation gizmo
    void drawCollider(QPainter& painter);  // Draw collision boundaries
    void drawSelectionOutline(QPainter& painter);  // Draw selection outline
    void drawImage(QPainter& painter);  // Draw entity images
    void drawTrajectory(QPainter& painter);  // Draw trajectory paths
    void drawMesh(QPainter& painter);  // Draw mesh geometries

    // Mouse event handlers
    void handleMousePress(QMouseEvent *event);
    void handleMouseMove(QMouseEvent *event);
    void handleMouseRelease(QMouseEvent *event);
    void handleKeyPress(QKeyEvent *event);
    void handlePaint(QPaintEvent *event);

    // View and display settings
    float zoomLevel = 1.0f;  // Current zoom level
    bool showXGrid = true;   // Show X grid lines
    bool showYGrid = true;   // Show Y grid lines
    int gridOpacity = 50;    // Grid opacity percentage
    bool showColliders = true;  // Show collision boundaries
    bool showMesh = true;    // Show mesh geometries
    bool showOutline = true; // Show selection outlines
    bool showInformation = true;  // Show information overlay
    bool showFPS = true;     // Show FPS counter

    // Mode and state flags
    bool isDrawingTrajectory = false;  // Currently drawing trajectory
    QString selectedBitmapType;  // Selected bitmap type for placement
    bool isPlacingBitmap = false;  // Currently placing bitmap

    // Trajectory data
    std::vector<Waypoints*> currentTrajectory;  // Current trajectory waypoints
    std::vector<Vector*> tempPolygonVertices;   // Temporary polygon vertices
    std::vector<QPointF> tempPolygonCanvasPoints;  // Temporary polygon canvas points

    // Waypoint selection and manipulation
    int selectedWaypointIndex;  // Index of selected waypoint
    bool isDraggingWaypoint;    // Currently dragging waypoint

    // Shape editing
    QString editingShapeId;     // ID of shape being edited
    int selectedHandleIndex;    // Index of selected resize handle
    bool isResizingShape;       // Currently resizing shape
    std::vector<QPointF> resizeHandles;  // Resize handle positions

    // Utility function for point-in-polygon detection
    bool isPointInPolygon(const QPointF& point, const std::vector<Vector*>& vertices,
                          const QPointF& centroidGeo, GISlib* gislib);

    // Right-click context menu handlers
    void handleRightClick(QMouseEvent *event);
    void handleTrajectoryRightClick(QMouseEvent *event);
    void handleShapeRightClick(QMouseEvent *event);

    // Airbase layer functionality (preset layer)
    std::vector<std::pair<double, double>> airbasePositions;  // lon, lat pairs
    bool showAirbases = false;  // Toggle flag for airbase visibility
    QString airbaseIconPath = ":/icons/images/airbase.png";  // Airbase icon path
    QPixmap airbasePixmap;  // Cached pixmap for efficiency

    void loadAirbaseData();  // Load static/dynamic airbase data
    void drawAirbases(QPainter& painter);  // Draw airbase icons

    // Distance measurement
    MeasureDistanceDialog* measureDialog = nullptr;  // Measurement dialog
    QList<QPointF> measurePoints; // geo points (lon, lat) for measurement

protected:
    // Qt event overrides
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    // Communication signals with other components
    void selectEntitybyCursor(QString ID);  // Entity selection signal
    void MoveEntity(QString ID);  // Entity movement signal
    void trajectoryUpdated(QString entityId, QJsonArray waypoints);  // Trajectory update signal
    void airbaseLayerToggled(bool visible);  // Airbase layer toggle signal

    // GeoJSON signals
    void geoJsonLayerAdded(const QString& layerName);  // New GeoJSON layer added
    void pointsUpdated(const QList<QPointF>& points);  // Measurement points updated

private:
    // Internal state variables
    bool selectEntity;  // Entity selection flag
    QTimer *updateTimer;  // Timer for periodic updates
    QElapsedTimer fpsTimer;  // Timer for FPS calculation
    int frameCount = 0;  // Frame counter for FPS
    float fps = 0.0f;    // Current FPS value
    float angle = 0.0;   // Rotation angle for demo

    // Dragging and interaction
    QPointF dragStartPos;  // Starting position for drag operations
    QString activeDragAxis;  // Currently active drag axis
    bool simulate = true;  // Simulation mode flag
    QPointF canvasOffset = QPointF(0, 0);  // Canvas panning offset
    QPoint lastMousePos;  // Last mouse position for panning
    bool isPanning = false;  // Currently panning canvas

    // Trajectory management
    int findNearestWaypoint(QPointF canvasPos);  // Find nearest waypoint to canvas position
    void updateTrajectoryData();  // Update trajectory data and emit signals

    // GeoJSON layer management
    QMap<QString, bool> geoJsonLayers;  // GeoJSON layer visibility map
    std::vector<Vector*> tempLineVertices;  // Temporary line vertices
    std::vector<QPointF> tempLineCanvasPoints;  // Temporary line canvas points

    // Bitmap dragging functionality
    bool isDraggingBitmap = false;  // Currently dragging bitmap
    QString draggingBitmapId;  // ID of bitmap being dragged
    QPointF bitmapDragStartPos;  // Start position for bitmap drag

    // User uploaded image dragging (separate from preset bitmaps)
    bool isDraggingUserImage = false;  // Currently dragging user image
    QString draggingUserImageId;  // ID of user image being dragged
    QPointF userImageDragStartPos;  // Start position for user image drag

    // Shape dragging functionality
    bool isDraggingShape = false;  // Currently dragging shape
    QString draggingShapeId;  // ID of shape being dragged
    QPointF shapeDragStartPos;  // Start position for shape drag

    // Selection and dragging handler methods
    bool handleShapeSelection(QMouseEvent *event);  // Handle shape selection
    void handleShapeDragging(QMouseEvent *event);  // Handle shape dragging
    void stopShapeDragging();  // Stop shape dragging

    bool handleBitmapSelection(QMouseEvent *event);  // Handle bitmap selection
    void handleBitmapDragging(QMouseEvent *event);  // Handle bitmap dragging
    void stopBitmapDragging();  // Stop bitmap dragging

    bool handleUserImageSelection(QMouseEvent *event);  // Handle user image selection
    void handleUserImageDragging(QMouseEvent *event);  // Handle user image dragging
    void stopUserImageDragging();  // Stop user image dragging
};

#endif // CANVASWIDGET_H
