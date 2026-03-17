
// #endif // SHAPES_FEATURE_H
#ifndef SHAPES_FEATURE_H
#define SHAPES_FEATURE_H

#include <QPainter>
#include <QPointF>
#include <QString>
#include <vector>
#include <QVector3D>
#include <QQuaternion>
#include <QStack>
#include <QMap>
#include "GUI/Tacticaldisplay/Gis/gislib.h"

// Forward declarations
class CanvasWidget;
class LayerPanel;
class Vector;
struct MeshEntry;
struct Mesh;

// Structure to store shape state for history
struct ShapeState {
    QVector3D position;
    QQuaternion rotation;
    QVector3D size;
    std::vector<Vector*> vertices;
    QString layerName;  // NEW: Track which layer shape belongs to

    ShapeState() {}
    ShapeState(const ShapeState& other);
    ShapeState& operator=(const ShapeState& other);
    ~ShapeState();
    void clearVertices();
};

class ShapesFeature {
public:
    ShapesFeature(CanvasWidget* canvas);

    // NEW: Set layer panel reference
    void setLayerPanel(LayerPanel* panel) { m_layerPanel = panel; }
    LayerPanel* getLayerPanel() const { return m_layerPanel; }

    // Shape drawing functions
    void drawCircle(const QPointF& geoPos);
    void drawRectangle(const QPointF& geoPos);
    void drawLine(const QPointF& geoPos, bool finalize);
    void drawPolygon(const QPointF& geoPos, bool finalize);
    void drawPoints(const QPointF& geoPos);
    void handleShapeDrawing(const QString& shapeType, const QPointF& geoPos, bool finalize);

    // Drag-to-draw functions
    void startDragShape(const QString& shapeType, const QPointF& startGeoPos);
    void updateDragShape(const QPointF& currentGeoPos);
    void finalizeDragShape();
    void cancelDragShape();
    bool isDraggingShape() const { return m_isDraggingNewShape; }
    QString getDraggingShapeType() const { return m_dragShapeType; }

    // Script-related functions
    void scriptStartLine();
    void scriptAddLinePoint(const QPointF& geoPos);
    void scriptFinishLine();

    // Script based circle by amjad
    void drawCircle(const QPointF& geoPos, float radiusDeg);

    // Script update for radius by amjad
    void updateCircleRadius(MeshEntry* entry, float radiusDeg);

    // Script rectangle by amjad
    void drawRectangle(const QPointF& geoPos, float widthDeg, float heightDeg);

    // Helper functions
    static bool isPointNearLineSegment(const QPointF& p, const QPointF& v1, const QPointF& v2, qreal tolerance);
    int getPolygonCounter() { return m_polygonCounter++; }
    int getPolylineCounter() { return m_polylineCounter++; }

    // History management
    void saveShapeState(const QString& shapeId, MeshEntry* entry);
    bool restorePreviousState(const QString& shapeId, MeshEntry* entry);
    bool hasHistory(const QString& shapeId) const;
    void clearHistory(const QString& shapeId);
    void clearAllHistory();

    // Preview functions
    void showHistoryPreview(const QString& shapeId);
    void hideHistoryPreview();
    void drawHistoryPreview(QPainter& painter, GISlib* gislib);
    bool isShowingPreview() const { return m_showingPreview; }
    QString getPreviewShapeId() const { return m_previewShapeId; }
    void drawDragPreview(QPainter& painter, GISlib* gislib);

    // Move shape using script
    void moveShapeByScript(MeshEntry* entry, const QPointF& newGeoPos);

    // NEW: Layer-aware shape creation methods
    QString createShapeInActiveLayer(const QString& shapeType, const QPointF& geoPos);
    void addShapeToActiveLayer(const QString& shapeId, const QString& shapeType);

private:
    CanvasWidget* m_canvas;
    GISlib* m_gislib;
    LayerPanel* m_layerPanel = nullptr;  // NEW: Reference to layer panel

    // Temporary storage for multi-point shapes
    std::vector<Vector*> m_tempLineVertices;
    std::vector<QPointF> m_tempLineCanvasPoints;
    std::vector<Vector*> m_tempPolygonVertices;
    std::vector<QPointF> m_tempPolygonCanvasPoints;

    // Counters for unique naming
    int m_circleCounter = 0;
    int m_rectCounter = 0;
    int m_polylineCounter = 0;
    int m_polygonCounter = 0;
    int m_pointCounter = 0;
    int m_vertexCounter = 0;

    // History storage
    QMap<QString, QStack<ShapeState>> m_shapeHistory;
    static const int MAX_HISTORY_DEPTH = 10;

    // Preview state
    bool m_showingPreview = false;
    QString m_previewShapeId;
    ShapeState m_previewState;

    // Drag-to-draw state
    bool m_isDraggingNewShape = false;
    QString m_dragShapeType;
    QPointF m_dragStartGeoPos;
    QPointF m_dragCurrentGeoPos;
    int m_dragShapeCounter = 0;

    // Helper functions
    ShapeState captureState(MeshEntry* entry);
    void applyState(const ShapeState& state, MeshEntry* entry);
    void finalizeCircleDrag();
    void finalizeRectangleDrag();
    QVector3D calculateCircleProperties(const QPointF& start, const QPointF& current);
    QRectF calculateRectangleProperties(const QPointF& start, const QPointF& current);
};

#endif // SHAPES_FEATURE_H
