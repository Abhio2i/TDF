#ifndef SHAPES_FEATURE_H
#define SHAPES_FEATURE_H

#include "qquaternion.h"
#include "GUI/Tacticaldisplay/Shape/IShapeHost.h"
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"
#include <QString>
#include <QPointF>
#include <QStack>
#include <QMap>
#include <QPainter>
#include <vector>

// Forward declarations
class Vector;
class GISlib;
class LayerPanel;

// ============================================================================
// ShapeState definition
// ============================================================================

struct ShapeState {
    QVector3D position;
    QQuaternion rotation;
    QVector3D size;
    std::vector<Vector*> vertices;
    QString layerName;

    ShapeState() = default;
    ShapeState(const ShapeState& other);
    ShapeState& operator=(const ShapeState& other);
    ~ShapeState();
    void clearVertices();
};

// ============================================================================
// ShapesFeature Class - Completely Decoupled from Canvas
// ============================================================================

class ShapesFeature
{
public:
    explicit ShapesFeature(IShapeHost* host);   // Only depends on IShapeHost
    ~ShapesFeature();

    // ========================================================================
    // Shape Drawing
    // ========================================================================

    void handleShapeDrawing(const QString& shapeType, const QPointF& geoPos, bool finalize = false);
    void drawCircle(const QPointF& geoPos);
    void drawCircle(const QPointF& geoPos, float radiusDeg);
    void drawRectangle(const QPointF& geoPos);
    void drawRectangle(const QPointF& geoPos, float widthDeg, float heightDeg);
    void drawLine(const QPointF& geoPos, bool finalize);
    void drawPolygon(const QPointF& geoPos, bool finalize);
    void drawPoints(const QPointF& geoPos);

    // ========================================================================
    // Drag-to-Draw
    // ========================================================================

    void startDragShape(const QString& shapeType, const QPointF& startGeoPos);
    void updateDragShape(const QPointF& currentGeoPos);
    void finalizeDragShape();
    void cancelDragShape();
    void drawDragPreview(QPainter& painter, GISlib* gislib);

    bool isDraggingShape() const { return m_isDraggingNewShape; }
    QString getDragShapeType() const { return m_dragShapeType; }

    // ========================================================================
    // Script Functions
    // ========================================================================

    void scriptStartLine();
    void scriptAddLinePoint(const QPointF& geoPos);
    void scriptFinishLine();

    // ========================================================================
    // History Management
    // ========================================================================

    void saveShapeState(const QString& shapeId, MeshEntry* entry);
    bool restorePreviousState(const QString& shapeId, MeshEntry* entry);
    bool hasHistory(const QString& shapeId) const;
    void clearHistory(const QString& shapeId);
    void clearAllHistory();

    // ========================================================================
    // History Preview
    // ========================================================================

    void showHistoryPreview(const QString& shapeId);
    void hideHistoryPreview();
    void drawHistoryPreview(QPainter& painter, GISlib* gislib);

    bool isShowingPreview() const { return m_showingPreview; }
    QString getPreviewShapeId() const { return m_previewShapeId; }

    // ========================================================================
    // Utilities
    // ========================================================================

    void addShapeToActiveLayer(const QString& shapeId, const QString& shapeType);

    static bool isPointNearLineSegment(const QPointF& p, const QPointF& v1,
                                       const QPointF& v2, qreal tolerance);

    void moveShapeByScript(MeshEntry* entry, const QPointF& newGeoPos);
    void updateCircleRadius(MeshEntry* entry, float radiusDeg);
    void syncCountersFromLoadedShapes(const std::vector<MeshEntry>& loadedMeshes);
    void resetCounters();
    void setLayerPanel(LayerPanel* panel) {
        qDebug() << "[ShapesFeature] LayerPanel received:" << panel;
        m_layerPanel = panel;
    }

    // ========================================================================
    // External Vector Setters
    // ========================================================================

    void setExternalPolygonVertices(std::vector<Vector*>* vertices) {
        m_externalPolygonVertices = vertices;
    }
    void setExternalLineVertices(std::vector<Vector*>* vertices) {
        m_externalLineVertices = vertices;
    }
    void setExternalPolygonCanvasPoints(std::vector<QPointF>* points) {
        m_externalPolygonCanvasPoints = points;
    }
    void setExternalLineCanvasPoints(std::vector<QPointF>* points) {
        m_externalLineCanvasPoints = points;
    }

    // ========================================================================
    // Getters
    // ========================================================================

    int getCircleCounter() const { return m_circleCounter; }
    int getRectCounter() const { return m_rectCounter; }
    int getPolygonCounter() const { return m_polygonCounter; }
    int getPolylineCounter() const { return m_polylineCounter; }
    int getPointCounter() const { return m_pointCounter; }

    std::vector<Vector*>& getTempLineVertices() { return m_tempLineVertices; }
    std::vector<Vector*>& getTempPolygonVertices() { return m_tempPolygonVertices; }
    std::vector<QPointF>& getTempLineCanvasPoints() { return m_tempLineCanvasPoints; }
    std::vector<QPointF>& getTempPolygonCanvasPoints() { return m_tempPolygonCanvasPoints; }

private:
    // ========================================================================
    // Private Helper Methods
    // ========================================================================

    void finalizeCircleDrag();
    void finalizeRectangleDrag();
    QVector3D calculateCircleProperties(const QPointF& start, const QPointF& current);
    QRectF calculateRectangleProperties(const QPointF& start, const QPointF& current);
    ShapeState captureState(MeshEntry* entry);
    void applyState(const ShapeState& state, MeshEntry* entry);

    // ========================================================================
    // Member Variables
    // ========================================================================

    IShapeHost* m_host;              // Only interface, no canvas dependency
    LayerPanel* m_layerPanel = nullptr;

    // External vectors (pointers to ShapeEventHandler's vectors)
    std::vector<Vector*>* m_externalPolygonVertices = nullptr;
    std::vector<Vector*>* m_externalLineVertices = nullptr;
    std::vector<QPointF>* m_externalPolygonCanvasPoints = nullptr;
    std::vector<QPointF>* m_externalLineCanvasPoints = nullptr;

    // Internal vectors (used when no external vectors provided)
    std::vector<Vector*> m_tempLineVertices;
    std::vector<Vector*> m_tempPolygonVertices;
    std::vector<QPointF> m_tempLineCanvasPoints;
    std::vector<QPointF> m_tempPolygonCanvasPoints;

    int m_circleCounter = 0;
    int m_rectCounter = 0;
    int m_polygonCounter = 0;
    int m_polylineCounter = 0;
    int m_pointCounter = 0;
    int m_vertexCounter = 0;

    bool m_isDraggingNewShape = false;
    QString m_dragShapeType;
    QPointF m_dragStartGeoPos;
    QPointF m_dragCurrentGeoPos;
    int m_dragShapeCounter = 0;

    static const int MAX_HISTORY_DEPTH = 20;
    QMap<QString, QStack<ShapeState>> m_shapeHistory;

    bool m_showingPreview = false;
    QString m_previewShapeId;
    ShapeState m_previewState;
};

#endif // SHAPES_FEATURE_H