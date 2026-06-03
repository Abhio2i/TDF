#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include "GUI/measuredistance/measuredistancedialog.h"
#include "GUI/Tacticaldisplay/Gis/shapes_feature.h"
#include "qnamespace.h"
#include "qtimer.h"
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
#include <core/Hierarchy/entity.h>
#include <core/Hierarchy/Components/transform.h>
#include <GUI/Tacticaldisplay/Gis/layerpanel.h>
// #include <GUI/Tacticaldisplay/entityinfodialog.h>
class EntityContextMenu;
class EntityInfoDialog;
// Forward declarations
class QDialog;
class QSpinBox;
class QPushButton;
class ShapesFeature;

/* MeshEntry structure section */

struct MeshEntry {
    QString name;                           // Name identifier for the mesh
    Qt3DCore::QTransform* transform = nullptr;        // 3D transformation
    Transform* coreTransform = nullptr;
    QVector3D* position;                    // Position in 3D space
    QQuaternion* rotation;                  // Rotation as quaternion
    QVector3D* velocity = nullptr;                    // Velocity vector
    QVector3D* size;                        // Size/scaling factors
    Mesh* mesh;                           // Mesh geometry data
    Entity* entity = nullptr;
    Platform* platform = nullptr;
    Collider* collider = nullptr;                     // Collision detection component
    Trajectory* trajectory = nullptr;               // Path/trajectory data
    DynamicModel* dynamicModel = nullptr;
    QString bitmapPath;                     // Path to bitmap image
    QString text;                           // Text content for text entities
    QPolygonF polyline;
    QVector<QPointF> pointsToDraw;
    bool detection = true;
    bool radioVisible = false;
    QColor textColor;          // Text color
    QFont textFont;           // Text font
    int textSize;             // Font size
    bool isTextSelected;      // Selection flag for text
    int individualImageSize = -1;
    QColor trajectoryColor = Qt::blue;

    float circleRadius = 0.0f;   // geo degrees (used only for circles) by amjad
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
    friend class ShapesFeature;

public:
    /* Constructor section */
    QPixmap* cacheimg;
    QPixmap scaledimg;
    QPen pointPen;
    const int pointRadius = 3;
    int ImageScale = 50;
    double getMetersPerPixel() const;
    CanvasWidget(QWidget *parent = nullptr);
    GISlib* gislib;


    /* Public members section */
    std::unordered_map<std::string, MeshEntry> Meshes;
    std::string selectedEntityId;
    std::vector<std::string> selectedEntityIds;
    TransformMode currentMode = Translate;

    /* Rendering and mode control section */
    void Render(float deltatime);  // Main rendering function
    void Refresh();
    void setTransformMode(TransformMode mode);  // Set transformation mode
    void setTrajectoryDrawingMode(bool enabled);  // Enable/disable trajectory drawing
    void saveTrajectory();  // Save current trajectory
    void setImageScale(int value);

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
    // void handleShapeDrawing(const QString& shapeType, const QPointF& geoPos, bool finalize);
    std::vector<MeshEntry> tempMeshes;  // Temporary meshes for shapes and bitmaps

    // Bitmap/image related methods
    void onBitmapImageSelected(const QString& filePath);  // Handle user image selection
    QString getBitmapImagePath(const QString& bitmapType);  // Get path for preset bitmaps
    void onBitmapSelected(const QString& bitmapType);  // Handle preset bitmap selection
    void onBitmapImageSelectedAtGeo(const QString& path, const QPointF& geo); // Script / deterministic geo based //
    void onBitmapSelectedAtGeo(const QString& bitmapType, const QPointF& geo);  // Script / deterministic geo based //

    // Waypoint and trajectory methods
    void selectWaypoint(int index);  // Select specific waypoint
    void deselectWaypoint();  // Deselect current waypoint

    // Serialization methods
    QJsonObject toJson() const;  // Convert state to JSON
    void fromJson(const QJsonObject& json);  // Load state from JSON

    void setTextPropertiesMode(bool enabled);
    void updateTextProperties(const QString& textId, const QColor& color,
                              const QFont& font, int fontSize);
    void deleteText(const QString& textId);
    //=============info
    EntityInfoDialog *entityInfoDialog = NULL;
    bool m_isBeingDestroyed = false;

    void updateShapeProperties(const QString& shapeId, const QColor& color, int borderThickness);

    ShapesFeature* getShapesFeature() const;  // getter for shape feature


    /////////// move shape by coordinate ////////
    bool moveShapeByName(const std::string& shapeName,
                         const QPointF& geoPos);

    // add text using script ///////
    void addTextAtGeo(const QString& text, const QPointF& geo);

    /////// delete shapes ////////////
    bool deleteObjectById(const QString& id);
    void centerOnEntity(const QString& entityId, bool adjustZoom = false);
    void centerOnEntityWithZoom(const QString& entityId, int zoomLevel);
    double calculateTrajectoryCompletionTime(const MeshEntry& entry) const;
    // static void runUnitTestsOnce();
    // canvaswidget.h mein public section mein add karo:
    void loadImportedLayerFeaturesToMeshes(const QString& filePath,
                                           const QString& layerName);
public slots:
    void ReInit();
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
    void centerOnShape(const QString& shapeId);   // new
    void resetEntityInfoDialog();  // ← add karo


private slots:
    void onMeasurementTypeChanged(bool isEll);  // Handle measurement type change (ellipsoidal vs planar)

private:
    // Add this member variable
    EntityContextMenu* m_entityContextMenu;
    // Add this method declaration
    void handleEntityDoubleClick(const QString& entityId, MeshEntry& entry);
    void showEntityContextMenu(const QPoint& globalPos, const QString& entityId, MeshEntry& entry);
    QString checkEntityHover(const QPoint& mousePos);
    void updateHoverTooltip();
    QString m_hoveredEntityId;
    QTimer m_tooltipTimer;
    LayerPanel* m_layerPanel = nullptr;
    QString m_highlightedShapeId;   // shape to draw selection outline for
    // private members mein add karo
    MeshEntry* m_copiedShape = nullptr;  // clipboard for shape copy
    bool m_renderEnabled = true;

    bool isBoxSelecting = false;
    QPoint boxStartPos;
    QPoint boxCurrentPos;

    // Multi Selection (Basic)
    std::vector<QString> selectedShapeIds;   // Shapes, Bitmaps, Text ke liye

    // Multi Drag
    bool isMultiDrag = false;
    QPointF multiDragStartGeo;

    bool isMultiSelecting() const {
        return !selectedEntityIds.empty() || !selectedShapeIds.empty();
    }

    // Function Declarations
    void clearMultiSelection();
    void performBoxSelection(const QPoint& p1, const QPoint& p2);
    void moveSelectedItems(const QPointF& deltaGeo);
    bool isClickOnAnySelectedItem(const QPoint& pos);
    bool isBoxSelectionMode = false;


public slots:
    void showEntityInfo(const QString& entityId);
    void hideEntityInfo();
    void importLayer(const QString& filePath);
    // void setTooltipOptions(const QSet<QString>& options);
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
    EntityInfoDialog* getEntityInfoDialog() const { return entityInfoDialog; }
    void selectMultipleEntities(const QList<QString>& entityIds);
    void clearSelection();
    void setTooltipOptions(const QSet<QString>& options);

    void setLayerPanel(LayerPanel* panel);
    LayerPanel* getLayerPanel() const { return m_layerPanel; }
    bool isDrawingTrajectory = false;  // Currently drawing trajectory
    bool showImage = true;
    bool showFPS = true;     // Show FPS counter
    bool showTrajectories = true;
    QMap<QString, QString> geoJsonLayerFilePaths;
    void copySelectedShape();
    void pasteShape();
private:
    // Drawing methods for different canvas elements
    void drawGridLines(QPainter& painter);  // Draw grid lines
    void drawEntityInformation(QPainter& painter);  // Draw entity info overlay
    void drawSceneInformation(QPainter& painter);  // Draw scene info (FPS, mode)
    void drawTransformGizmo(QPainter& painter);  // Draw transformation gizmo
    void drawCollider(QPainter& painter,std::string id , MeshEntry entry);  // Draw collision boundaries
    void drawSelectionOutline(QPainter& painter);  // Draw selection outline
    void drawCollision(QPainter& painter,std::string id , MeshEntry entry);
    void drawImage(QPainter& painter,std::string id , MeshEntry entry);  // Draw entity images
    void drawFormation(QPainter& painter,std::string id , MeshEntry entry);
    void drawTrajectory(QPainter& painter,const std::string& id , MeshEntry& entry);  // Draw trajectory paths
    void drawTrail(QPainter& painter,std::string id , MeshEntry entry);  // Draw trajectory paths
    void drawMesh(QPainter& painter);  // Draw mesh geometries
    // void drawMesh(QPainter& painter,std::string id , MeshEntry entry);  // Draw mesh geometries
    void drawRadar(QPainter& painter,std::string id , MeshEntry entry);  // Draw collision boundaries
    void drawRadio(QPainter& painter,std::string id , MeshEntry entry);  // Draw collision boundaries

    // Mouse event handlers
    void handleMousePress(QMouseEvent *event);
    void handleMouseMove(QMouseEvent *event);
    void handleMouseRelease(QMouseEvent *event);
    void handleKeyPress(QKeyEvent *event);
    void handlePaint(QPaintEvent *event);

    void handleShapesMousePress(QMouseEvent *event);
    // void handleBitmapsMousePress(QMouseEvent *event);
    bool handleBitmapsMousePress(QMouseEvent *event);
    void handleTextMousePress(QMouseEvent *event);  // NEW: Text handling function

    void handleShapesMouseMove(QMouseEvent *event);
    void handleTextMouseMove(QMouseEvent *event);
    void handleBitmapsMouseMove(QMouseEvent *event);

    void handleShapesMouseRelease(QMouseEvent *event);
    void handleTextMouseRelease(QMouseEvent *event);
    void handleBitmapsMouseRelease(QMouseEvent *event);

    void handleShapesKeyPress(QKeyEvent *event);
    void handleTextKeyPress(QKeyEvent *event);
    void handleBitmapsKeyPress(QKeyEvent *event);

    void handleShapesPaint(QPainter& painter);
    void handleTextPaint(QPainter& painter);
    void handleBitmapsPaint(QPainter& painter);

    // View and display settings
    float zoomLevel = 1.0f;  // Current zoom level
    bool showXGrid = true;   // Show X grid lines
    bool showYGrid = true;   // Show Y grid lines
    int gridOpacity = 50;    // Grid opacity percentage
    bool showColliders = true;  // Show collision boundaries
    bool showMesh = true;    // Show mesh geometries
    bool showOutline = true; // Show selection outlines
    bool showInformation = false;  // Show information overlay
    // bool showFPS = true;     // Show FPS counter
    bool showSensors = true;     // Default sensors visible
    bool showRadio = true;       // Default radio visible
    // bool showImage = true;
    bool showTooltip = false;
    // bool showTrajectories = true;
    // Mode and state flags
    // bool isDrawingTrajectory = false;  // Currently drawing trajectory
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

    // Distance measurement
    MeasureDistanceDialog* measureDialog = nullptr;  // Measurement dialog
    QList<QPointF> measurePoints; // geo points (lon, lat) for measurement

    ShapesFeature* shapesFeature;
    void updateResizeHandlesForRotation(const QString& shapeId);
    QPolygonF getRotatedShapePolygon(const MeshEntry& entry) const;
    // Box-zoom functionality
    bool isBoxZooming = false;           // Currently performing box-zoom drag
    bool boxZoomPending = false;         // Double-click detected, waiting for drag
    QPoint boxZoomStart;                 // Start point of box-zoom (canvas coordinates)
    QPoint boxZoomCurrent;               // Current mouse position during box-zoom
    QElapsedTimer doubleClickTimer;      // Timer to track double-click timing
    static constexpr int DOUBLE_CLICK_TIMEOUT = 500;  // Max time between clicks (ms)

    // Helper function to check if click is on empty canvas (no entities/shapes/bitmaps)
    bool isClickOnEmptyCanvas(const QPoint& pos);
    QSet<QString> activeTooltipOptions;
    bool shouldDrawShape(const QString& shapeId) const;

protected:
    // Qt event overrides
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    // Communication signals with other components
    // void selectEntitybyCursor(QString ID);  // Entity selection signal
    void selectEntitybyCursor(const QString& entityId, bool isEntitySelection = true);

    void MoveEntity(QString ID);  // Entity movement signal
    void trajectoryUpdated(QString entityId, QJsonArray waypoints);  // Trajectory update signal
    void trajectoryUpdatedforLogger(QString entityId, std::vector<Waypoints *> Trajectories);
    void airbaseLayerToggled(bool visible);  // Airbase layer toggle signal

    // GeoJSON signals
    void geoJsonLayerAdded(const QString& layerName);  // GeoJSON layer added
    void pointsUpdated(const QList<QPointF>& points);  // Measurement points updated
    void requestAddEntityAtPosition(double longitude, double latitude);
    void entitySelectedOnCanvas(QString entityId);
    void shapeSelectedFromCanvas(const QString& shapeId);

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

    void placeBitmapAtGeo(const QPointF& geo);  // bitmap plaing using geo //

    bool handleBitmapSelection(QMouseEvent *event);  // Handle bitmap selection
    void handleBitmapDragging(QMouseEvent *event);  // Handle bitmap dragging
    void stopBitmapDragging();  // Stop bitmap dragging

    bool handleUserImageSelection(QMouseEvent *event);  // Handle user image selection
    void handleUserImageDragging(QMouseEvent *event);  // Handle user image dragging
    void stopUserImageDragging();  // Stop user image dragging

    bool   isRotating = false;          // are we rotating a temp object?
    QString rotatingId;                 // id of the object being rotated
    QPoint  rotateStartPos;             // mouse pos when rotation started
    qreal   rotateStartAngle = 0.0;     // initial rotation of the object (degrees)
    static qreal angleBetweenPoints(const QPointF &center, const QPointF &p1, const QPointF &p2);
    void drawRotationHandle(QPainter &painter, const MeshEntry &entry);
    QPointF rotateCenter;
    qreal initialMouseAngle = 0.0;
    QString rotatingBitmapId;           // For bitmap rotation
    QPointF rotateHandleCenter;         // Handle center (canvas)
    QPointF rotateHandleStartPos;       // Mouse start
    qreal initialBitmapAngle = 0.0;     // For smooth rotation
    bool isRotatingBitmap = false;
    QString activeRotateId;  // NEW: Tracks which shape is in rotate mode

    // Text editing related members
    bool isEditingText = false;
    QString editingTextId;
    QColor currentTextColor = Qt::black;
    QFont currentTextFont = QFont("Arial", 12);
    int currentTextSize = 12;

    // Text editing methods
    bool handleTextSelection(QMouseEvent *event);
    void handleTextDragging(QMouseEvent *event);
    void stopTextDragging();
    void showTextPropertiesDialog(const QString& textId);
    void drawTextResizeHandles(QPainter& painter, const MeshEntry& entry);
    bool handleTextRightClick(QMouseEvent *event);

    QDialog* textPropertiesDialog = nullptr;

    // Shape properties dialog
    QDialog* shapePropertiesDialog = nullptr;

    // Shape properties methods
    void showShapePropertiesDialog(const QString& shapeId);
    bool isShape(const QString& shapeId) const;

};

#endif // CANVASWIDGET_H
