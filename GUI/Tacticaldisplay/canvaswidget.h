#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include "GUI/Tacticaldisplay/Measurement/measurement.h"
#include "GUI/Tacticaldisplay/Shape/shapes_feature.h"
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
#include "GUI/Tacticaldisplay/Layer/layerpanel.h"
#include "GUI/Tacticaldisplay/Gis/transformmode.h"
#include "GUI/Tacticaldisplay/Shape/IShapeHost.h"
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"
#include "GUI/Tacticaldisplay/Layer/ILayerHost.h"
#include <core/Debug/console.h>
#include "GUI/Tacticaldisplay/contextmenu/contextmenu_handler.h"

// Forward declarations
class EntityContextMenu;
class EntityInfoDialog;
class QDialog;
class QSpinBox;
class QPushButton;
class ShapesFeature;
class ShapeEventHandler;
class CanvasWidget : public QWidget, public IShapeHost, public ILayerHost {
    Q_OBJECT
    friend class ShapesFeature;
    friend class ShapeEventHandler;

public:
    Measurement* m_measurementModule = nullptr;
    Measurement* getMeasurementModule() const { return m_measurementModule; }

    // Constructor / Destructor
    QPixmap* cacheimg;
    QPixmap scaledimg;
    QPen pointPen;
    const int pointRadius = 3;
    int ImageScale = 50;
    double getMetersPerPixel() const;
    CanvasWidget(QWidget *parent = nullptr);
    ~CanvasWidget();

    GISlib* gislib;

    // Public members
    std::unordered_map<std::string, MeshEntry> Meshes;
    std::string selectedEntityId;
    std::vector<std::string> selectedEntityIds;
    TransformMode currentMode = Translate;

    // Rendering and mode control
    void Render(float deltatime);
    void Refresh();
    void setTransformMode(TransformMode mode);
    void setTrajectoryDrawingMode(bool enabled);
    void saveTrajectory();
    void setImageScale(int value);
    int m_importCounter = 0;

    // Simulation and editor control
    void simulation();
    void editor();

    // Grid visibility and settings
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

    // Shape API - All delegated to ShapeEventHandler
    QString selectedShape;
    void setShapeDrawingMode(bool enabled, const QString& shapeType = "") override;
    void onBitmapImageSelected(const QString& filePath);
    QString getBitmapImagePath(const QString& bitmapType);
    void onBitmapSelected(const QString& bitmapType);
    void onBitmapImageSelectedAtGeo(const QString& path, const QPointF& geo);
    void onBitmapSelectedAtGeo(const QString& bitmapType, const QPointF& geo);
    void addTextAtGeo(const QString& text, const QPointF& geo);
    void copySelectedShape();
    void pasteShape();
    void deleteText(const QString& textId);
    void updateShapeProperties(const QString& shapeId, const QColor& color, int borderThickness);
    void showShapePropertiesDialog(const QString& shapeId);
    bool moveShapeByName(const std::string& shapeName, const QPointF& geoPos);
    bool deleteObjectById(const QString& id);
    void centerOnShape(const QString& shapeId);
    void placePastedShape(const QPointF& geoPos);
    bool isShape(const QString& shapeId) const;

    const QString& getHighlightedShapeId() const;
    void setHighlightedShapeId(const QString& id);
    bool isDraggingShape() const;
    const QString& getDraggingShapeId() const;
    bool isResizingShape() const;
    int getSelectedHandleIndex() const;
    const std::vector<QPointF>& getResizeHandles() const;
    const QString& getEditingShapeId() const;
    std::vector<QString>& getSelectedShapeIds();
    bool isMultiSelecting() const;
    bool isRotatingBitmap() const;
    const QString& getRotatingBitmapId() const;
    void setActiveRotateId(const QString& id);
    const QString& getActiveRotateId() const;
    void clearActiveRotateId();
    void clearSelectedShapeIds();
    bool isPlacingBitmap() const;
    const QString& getSelectedBitmapType() const;

    // Waypoint and trajectory
    void selectWaypoint(int index);
    void deselectWaypoint();

    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

    // Entity info
    EntityInfoDialog* entityInfoDialog = NULL;
    bool m_isBeingDestroyed = false;
    ShapesFeature* getShapesFeature() const override;

    void centerOnEntity(const QString& entityId, bool adjustZoom = false);
    void centerOnEntityWithZoom(const QString& entityId, int zoomLevel);
    double calculateTrajectoryCompletionTime(const MeshEntry& entry) const;
    void fullCanvasReset();
    void highlightSonobuoy(const QString& sonobuoyId);
    void clearSonobuoyHighlight();
    QString m_highlightedSonobuoyId;

    // These methods need to be public for ShapeEventHandler access
    void handleShapeRightClick(QMouseEvent *event);
    bool handleShapeSelection(QMouseEvent *event);

    std::vector<MeshEntry>& getTempMeshes() override;
    const std::vector<MeshEntry>& getTempMeshes() const;

    std::vector<Vector*>& getTempPolygonVertices() override;
    const std::vector<Vector*>& getTempPolygonVertices() const;

    std::vector<Vector*>& getTempLineVertices() override;
    const std::vector<Vector*>& getTempLineVertices() const;

    std::vector<QPointF>& getTempPolygonCanvasPoints() override;
    const std::vector<QPointF>& getTempPolygonCanvasPoints() const;

    std::vector<QPointF>& getTempLineCanvasPoints() override;
    const std::vector<QPointF>& getTempLineCanvasPoints() const;
    MeshEntry* getEntityMeshEntry(const QString& entityId) override;   // ADDED - entity context menu fix
    // added for shapes operation handling
    void startShapeEdit(const QString& shapeId) override;
    void startShapeRotate(const QString& shapeId) override;
    void cancelActiveShapeOperation() override;

    // ========================================================================
    // NEW: IShapeHost interface implementation
    // All trivial, inline, delegate to existing members/methods below.
    // No canvaswidget.cpp changes required because of these.
    // ========================================================================

    void refreshView() override { Refresh(); }
    void forceRedraw() override { update(); }
    QPoint getMousePosition() const override { return m_lastMousePos; }

    void setCursor(const QCursor& cursor) override { QWidget::setCursor(cursor); }

    void setCurrentMode(TransformMode mode) override { currentMode = mode; }
    TransformMode getCurrentMode() const override { return currentMode; }

    void setSelectedShape(const QString& shapeType) override { selectedShape = shapeType; }
    QString getSelectedShape() const override { return selectedShape; }

    // Named differently from the existing `isDrawingTrajectory` bool member
    // to avoid a method/member name collision in C++.
    bool isTrajectoryDrawingActive() const override { return isDrawingTrajectory; }
    // CHAITANYA JAIN CHANGE:
    // Why: Overrode getTrajectoryWaypointCount to return count of current trajectory waypoints.
    int getTrajectoryWaypointCount() const override { return currentTrajectory.size(); }

    GISlib* getGISlib() const override { return gislib; }

    void addShapeToActiveLayer(const QString& shapeId, const QString& shapeType) override {
        if (m_shapesFeature) m_shapesFeature->addShapeToActiveLayer(shapeId, shapeType);
    }

    LayerPanel* getLayerPanel() const override { return m_layerPanel; }

    void logMessage(const QString& message) override { Console::log(message.toStdString()); }
    void logWarning(const QString& message) override { Console::warning(message.toStdString()); }
    void logError(const QString& message) override { Console::error(message.toStdString()); }

public slots:
    void ReInit();
    void onGISKeyPressed(QKeyEvent *event) { keyPressEvent(event); }
    void onGISMousePressed(QMouseEvent *event) { mousePressEvent(event); }
    void onGISMouseMoved(QMouseEvent *event) { mouseMoveEvent(event); }
    void onGISMouseReleased(QMouseEvent *event) { mouseReleaseEvent(event); }
    void onGISPainted(QPaintEvent *event) { paintEvent(event); }
    void updateWaypointsFromInspector(QString entityId, QJsonArray waypoints);
    void onDistanceMeasured(double distance, QPointF startPoint, QPointF endPoint);
    void onPresetLayerSelected(const QString& preset);
    // void importGeoJsonLayer(const QString &filePath);
    void onGeoJsonLayerToggled(const QString &layerName, bool visible);
    void resetEntityInfoDialog();
    void onWaypointSelectedFromInspector(int waypointIndex);
    void showEntityInfo(const QString& entityId);
    void hideEntityInfo();
    void importLayer(const QString& filePath);

private slots:
    // Measurement slots moved to Measurement module


private:
    // Shape event handler
    ShapeEventHandler* m_shapeEventHandler;
    ShapesFeature* m_shapesFeature;
    std::vector<MeshEntry> m_dummyMeshes;
    std::vector<Vector*> m_dummyVectors;
    std::vector<QPointF> m_dummyPoints;

    // Entity context menu
    EntityContextMenu* m_entityContextMenu;
    void handleEntityDoubleClick(const QString& entityId, MeshEntry& entry);
    void showEntityContextMenu(const QPoint& globalPos, const QString& entityId, MeshEntry& entry);
    QString checkEntityHover(const QPoint& mousePos);
    void updateHoverTooltip();
    QString m_hoveredEntityId;
    QTimer m_tooltipTimer;
    LayerPanel* m_layerPanel = nullptr;
    void selectEntitiesInBox(const QRectF& boxRect);// by chaitanya jain


    // Multi Selection
    bool isMultiDrag = false;
    QPointF multiDragStartGeo;
    int m_highlightedWaypointIndex = -1;
    QString m_highlightedWaypointEntityId;

    void clearMultiSelection();
    void moveSelectedItems(const QPointF& deltaGeo);
    bool isClickOnAnySelectedItem(const QPoint& pos);

    // Drawing methods
    void drawGridLines(QPainter& painter);
    void drawEntityInformation(QPainter& painter);
    void drawSceneInformation(QPainter& painter);
    void drawTransformGizmo(QPainter& painter);
    void drawCollider(QPainter& painter, std::string id, MeshEntry entry);
    void drawSelectionOutline(QPainter& painter);
    void drawCollision(QPainter& painter, std::string id, MeshEntry entry);
    void drawImage(QPainter& painter, std::string id, MeshEntry entry);
    void drawFormation(QPainter& painter, std::string id, MeshEntry entry);
    void drawTrajectory(QPainter& painter, const std::string& id, MeshEntry& entry);
    void drawTrail(QPainter& painter, std::string id, MeshEntry entry);
    void drawMesh(QPainter& painter);
    void drawRadar(QPainter& painter, std::string id, MeshEntry entry);
    void drawRadio(QPainter& painter, std::string id, MeshEntry entry);
    void drawSonobuoyHighlight(QPainter& painter);

    // Mouse event handlers
    void handleMousePress(QMouseEvent *event);
    void handleMouseMove(QMouseEvent *event);
    void handleMouseRelease(QMouseEvent *event);
    void handleKeyPress(QKeyEvent *event);
    void handlePaint(QPaintEvent *event);

    // Delegated handlers
    void handleShapesMousePress(QMouseEvent *event);
    bool handleBitmapsMousePress(QMouseEvent *event);
    void handleTextMousePress(QMouseEvent *event);
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
    ContextMenuHandler* m_contextMenuHandler = nullptr;
    void setupContextMenuCallbacks();

    // View and display settings
    float zoomLevel = 1.0f;
    bool showXGrid = true;
    bool showYGrid = true;
    int gridOpacity = 50;
    bool showColliders = true;
    bool showMesh = true;
    bool showOutline = true;
    bool showInformation = false;
    bool showSensors = true;
    bool showRadio = true;
    bool showTooltip = false;
    bool isDrawingTrajectory = false;
    bool showImage = true;
    bool showFPS = true;
    bool showTrajectories = true;

    // Trajectory data
    std::vector<Waypoints*> currentTrajectory;
    int selectedWaypointIndex;
    bool isDraggingWaypoint;

    // Right-click handlers
    void handleRightClick(QMouseEvent *event);
    void handleTrajectoryRightClick(QMouseEvent *event);

    // Distance measurement moved to Measurement module


    // Box-zoom
    bool isBoxZooming = false;
    bool boxZoomPending = false;
    QPoint boxZoomStart;
    QPoint boxZoomCurrent;
    QElapsedTimer doubleClickTimer;
    static constexpr int DOUBLE_CLICK_TIMEOUT = 500;

    // Helpers
    bool isClickOnEmptyCanvas(const QPoint& pos);
    QSet<QString> activeTooltipOptions;
    QPoint m_lastMousePos;
    bool shouldDrawShape(const QString& shapeId) const;
    QPointF m_lastLeftClickGeo;

    // Internal state
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
    std::vector<QString> m_dummyShapeIds;
    // Trajectory management
    int findNearestWaypoint(QPointF canvasPos);
    void updateTrajectoryData();
    // GeoJSON
    QMap<QString, bool> geoJsonLayers;
    QMap<QString, QString> geoJsonLayerFilePaths;
    MeshEntry* m_copiedShape = nullptr;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

signals:
    void selectEntitybyCursor(const QString& entityId, bool isEntitySelection = true);
    void MoveEntity(QString ID);
    void trajectoryUpdated(QString entityId, QJsonArray waypoints);
    void trajectoryUpdatedforLogger(QString entityId, std::vector<Waypoints *> Trajectories);
    void airbaseLayerToggled(bool visible);
    void geoJsonLayerAdded(const QString& layerName);
    void pointsUpdated(const QList<QPointF>& points);
    void requestAddEntityAtPosition(double longitude, double latitude);
    void entitySelectedOnCanvas(QString entityId);
    void shapeSelectedFromCanvas(const QString& shapeId);

public:
    void addImportedMesh(
        const MeshEntry& entry
        ) override;
    // Drag and drop
    void dragEnterEvents(QDragEnterEvent *event);
    void dragMoveEvents(QDragMoveEvent *event);
    void dropEvents(QDropEvent *event);

    // Distance Measurement API (now managed by Measurement module)

    EntityInfoDialog* getEntityInfoDialog() const { return entityInfoDialog; }
    void selectMultipleEntities(const QList<QString>& entityIds);
    void clearSelection();
    void setTooltipOptions(const QSet<QString>& options);
    void setLayerPanel(LayerPanel* panel);
    // NOTE: getLayerPanel() already declared above as part of IShapeHost override — not repeated here.

    // Static helper for line segment distance (used in right-click)
    static bool isPointNearLineSegment(const QPointF& p, const QPointF& v1, const QPointF& v2, qreal tolerance);

    // CHAITANYA JAIN CHANGE:
    // Why: Declared overrides for the ILayerHost interface.
    bool deleteObjectFromCanvas(const QString& shapeId) override;
    void addRasterMesh(const MeshEntry& entry) override;
    std::vector<MeshEntry> getMeshEntriesForShapes(const QStringList& shapeIds) override;
    ViewportInfo getViewportInfo() const override;
    void fitMapBounds(double minLon, double minLat, double maxLon, double maxLat) override;
    void registerImportedShape(const QString& layerName,
                               const QString& shapeType,
                               const QString& shapeName,
                               const QVector3D& position,
                               const std::vector<QVector3D>& polygenOffsets,
                               const QVector3D& size,
                               bool closePath) ;
    void registerImportedText(
        const QString& layerName,
        const QString& featureName,
        const QVector3D& position,
        const QString& text) override;

    void registerImportedBitmap(
        const QString& layerName,
        const QString& featureName,
        const QVector3D& position,
        const QString& bitmapPath,
        const QVector3D& size,
        float rotation) override;


};

#endif // CANVASWIDGET_H
