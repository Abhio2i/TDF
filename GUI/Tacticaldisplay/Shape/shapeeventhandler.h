#ifndef SHAPEEVENTHANDLER_H
#define SHAPEEVENTHANDLER_H

#include <QPointF>
#include <QPolygonF>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QColor>
#include <QFont>
#include <QVector3D>
#include <QQuaternion>
#include <QCursor>
#include <vector>
#include <map>
#include <QDialog>
#include <QString>
#include <QMenu>

#include "IShapeHost.h"
#include "mesh_entry.h"
#include "../Gis/transformmode.h"

class Vector;
class Mesh;
class ShapesFeature;
class LayerPanel;
class GISlib;

/**
 * @brief Handles all shape-related mouse/keyboard events and rendering
 *        COMPLETELY DECOUPLED from CanvasWidget - uses IShapeHost only
 */
class ShapeEventHandler
{
public:
    // FIXED: Constructor takes IShapeHost* (not CanvasWidget*)
    explicit ShapeEventHandler(IShapeHost* host);
    ~ShapeEventHandler();
void clearShapeSelectionState();
    // ========================================================================
    // Event Handlers
    // ========================================================================
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPainter& painter);


    // ========================================================================
    // Shape Selection & Manipulation
    // ========================================================================

    bool handleShapeSelection(QMouseEvent *event);
    void handleShapeRightClick(QMouseEvent *event);
    void handleShapeDragging(QMouseEvent *event);
    void stopShapeDragging();
    // Shape event operation handling
    void cancelActiveShapeOperation();
    void startShapeEdit(const QString& shapeId);
    void startShapeRotate(const QString& shapeId);

    // ========================================================================
    // Shape Operations
    // ========================================================================

    void copySelectedShape();
    void pasteShape();
    void deleteShape(const QString& shapeId);
    bool moveShapeByName(const std::string& shapeName, const QPointF& geoPos);
    bool deleteObjectById(const QString& id);
    void centerOnShape(const QString& shapeId);
    void placePastedShape(const QPointF& geoPos);
    void updateShapeProperties(const QString& shapeId, const QColor& color, int borderThickness);
    void showShapePropertiesDialog(const QString& shapeId);

    // ========================================================================
    // Shape Drawing
    // ========================================================================

    void setShapeDrawingMode(bool enabled, const QString& shapeType);
    void finalizePolygon();
    void finalizeLine();
    void cancelShapeDrawing();

    // ========================================================================
    // Shape Queries - GETTERS
    // ========================================================================

    std::vector<MeshEntry>& getTempMeshes() { return m_tempMeshes; }
    const std::vector<MeshEntry>& getTempMeshes() const { return m_tempMeshes; }

    std::vector<Vector*>& getTempPolygonVertices() { return m_tempPolygonVertices; }
    const std::vector<Vector*>& getTempPolygonVertices() const { return m_tempPolygonVertices; }

    std::vector<Vector*>& getTempLineVertices() { return m_tempLineVertices; }
    const std::vector<Vector*>& getTempLineVertices() const { return m_tempLineVertices; }

    std::vector<QPointF>& getTempPolygonCanvasPoints() { return m_tempPolygonCanvasPoints; }
    const std::vector<QPointF>& getTempPolygonCanvasPoints() const { return m_tempPolygonCanvasPoints; }

    std::vector<QPointF>& getTempLineCanvasPoints() { return m_tempLineCanvasPoints; }
    const std::vector<QPointF>& getTempLineCanvasPoints() const { return m_tempLineCanvasPoints; }

    const QString& getHighlightedShapeId() const { return m_highlightedShapeId; }
    void setHighlightedShapeId(const QString& id) { m_highlightedShapeId = id; }

    bool isDraggingShape() const { return m_isDraggingShape; }
    const QString& getDraggingShapeId() const { return m_draggingShapeId; }

    bool isResizingShape() const { return m_isResizingShape; }
    int getSelectedHandleIndex() const { return m_selectedHandleIndex; }
    const std::vector<QPointF>& getResizeHandles() const { return m_resizeHandles; }
    const QString& getEditingShapeId() const { return m_editingShapeId; }

    std::vector<QString>& getSelectedShapeIds() { return m_selectedShapeIds; }
    const std::vector<QString>& getSelectedShapeIds() const { return m_selectedShapeIds; }
    void clearSelectedShapeIds() { m_selectedShapeIds.clear(); }

    bool isRotatingBitmap() const { return m_isRotatingBitmap; }
    const QString& getRotatingBitmapId() const { return m_rotatingBitmapId; }

    void setActiveRotateId(const QString& id) { m_activeRotateId = id; }
    const QString& getActiveRotateId() const { return m_activeRotateId; }
    void clearActiveRotateId() { m_activeRotateId.clear(); }

    bool isPlacingBitmap() const { return m_isPlacingBitmap; }
    const QString& getSelectedBitmapType() const { return m_selectedBitmapType; }

    // ========================================================================
    // Box Selection Queries
    // ========================================================================
    bool isBoxSelectionMode() const {
        return m_isBoxSelectionMode;
    }

    void toggleBoxSelectionMode() {
        setBoxSelectionMode(!m_isBoxSelectionMode);
    }

    void setBoxSelectionMode(bool mode) {

        m_isBoxSelectionMode = mode;

        if (mode) {
            // =========================================================
            // FIX: Box Selection ON -> cancel Edit / Rotate / Drag
            // =========================================================

            cancelActiveShapeOperation();

            // Stop resize/edit
            m_isResizingShape = false;
            m_selectedHandleIndex = -1;
            m_editingShapeId.clear();

            // Stop rotate
            m_activeRotateId.clear();
            m_isRotatingBitmap = false;
            m_rotatingBitmapId.clear();

            // Stop single shape drag
            m_isDraggingShape = false;
            m_draggingShapeId.clear();

            // Stop text drag
            m_isEditingText = false;
            m_editingTextId.clear();

            // Stop bitmap drag
            m_isDraggingBitmap = false;
            m_draggingBitmapId.clear();

            // Stop user image drag
            m_isDraggingUserImage = false;
            m_draggingUserImageId.clear();

            // Reset previous box drag
            m_isBoxSelecting = false;

            if (m_host) {
                // Exit Edit / Rotate / Scale
                m_host->setCurrentMode(TransformMode::Translate);

                // Box selection cursor
                m_host->setCursor(Qt::CrossCursor);

                m_host->refreshView();
            }
        }
        else {
            m_isBoxSelecting = false;

            if (m_host) {
                m_host->setCursor(Qt::ArrowCursor);
                m_host->refreshView();
            }
        }
    }

    // ========================================================================
    // Other Queries
    // ========================================================================

    bool isShape(const QString& shapeId) const;
    bool shouldDrawShape(const QString& shapeId) const;
    MeshEntry* findMeshEntry(const QString& shapeId);
    const MeshEntry* findMeshEntry(const QString& shapeId) const;
    QPolygonF getRotatedShapePolygon(const MeshEntry& entry) const;
    bool hasCopiedShape() const { return m_copiedShape != nullptr; }
    void setLayerPanel(LayerPanel* panel);
    void clearStates();

    // ========================================================================
    // Counter Management
    // ========================================================================

    void syncCountersFromLoadedShapes(const std::vector<MeshEntry>& meshes);
    void resetCounters();

    // ========================================================================
    // Text Handling
    // ========================================================================

    void addTextAtGeo(const QString& text, const QPointF& geoPos);
    void showTextPropertiesDialog(const QString& textId);
    void deleteText(const QString& textId);
    bool handleTextSelection(QMouseEvent *event);
    bool handleTextMousePress(QMouseEvent *event);
    void handleTextMouseMove(QMouseEvent *event);
    void handleTextMouseRelease(QMouseEvent *event);
    void handleTextDragging(QMouseEvent *event);
    bool handleTextRightClick(QMouseEvent *event);
    void stopTextDragging();

    // ========================================================================
    // Bitmap Handling
    // ========================================================================

    void onBitmapImageSelected(const QString& filePath);
    void onBitmapSelected(const QString& bitmapType);
    void onBitmapImageSelectedAtGeo(const QString& path, const QPointF& geo);
    void onBitmapSelectedAtGeo(const QString& bitmapType, const QPointF& geo);
    QString getBitmapImagePath(const QString& bitmapType) const;
    void placeBitmapAtGeo(const QPointF& geoPos);

    bool handleBitmapSelection(QMouseEvent *event);
    void handleBitmapDragging(QMouseEvent *event);
    void stopBitmapDragging();

    bool handleUserImageSelection(QMouseEvent *event);
    void handleUserImageDragging(QMouseEvent *event);
    void stopUserImageDragging();

    bool handleBitmapsMousePress(QMouseEvent *event);
    void handleBitmapsMouseMove(QMouseEvent *event);
    void handleBitmapsMouseRelease(QMouseEvent *event);
    void handleBitmapsKeyPress(QKeyEvent *event);
    void handleBitmapsPaint(QPainter& painter);

    // ========================================================================
    // Resize Handles
    // ========================================================================

    void updateResizeHandlesForRotation(const QString& shapeId);
   // void handleResizeHandleSelection(QMouseEvent *event);
    bool handleResizeHandleSelection(QMouseEvent *event);

    // ========================================================================
    // Shape-specific Drawing
    // ========================================================================

    void drawTempShapesPreview(QPainter& painter);
    void drawPolygonPreview(QPainter& painter);
    void drawLinePreview(QPainter& painter);
    void drawTempMeshes(QPainter& painter);
    void drawText(QPainter& painter, const MeshEntry& entry);
    void drawTextSelectionHandles(QPainter& painter, const MeshEntry& entry);
    void drawBitmap(QPainter& painter, const MeshEntry& entry);
    void drawPoint(QPainter& painter, const MeshEntry& entry);
    void drawCircle(QPainter& painter, const MeshEntry& entry);
    void drawPolygonShape(QPainter& painter, const MeshEntry& entry);
    void drawSelectionOutline(QPainter& painter);
    void drawResizeHandles(QPainter& painter);
    void drawRotationHandle(QPainter& painter, const MeshEntry& entry);

    // ========================================================================
    // Helpers
    // ========================================================================

    bool isPointInPolygon(const QPointF& point, const std::vector<Vector*>& vertices,
                          const QPointF& centroidGeo, GISlib* gislib) const;

    // ========================================================================
    // Resize Handlers
    // ========================================================================

    void handlePolygonVertexEditing(QMouseEvent *event, MeshEntry& entry);
    void handleRectangleResizing(QMouseEvent *event, MeshEntry& entry);
    void handleCircleResizing(QMouseEvent *event, MeshEntry& entry);
    void handleBitmapResizing(QMouseEvent *event, MeshEntry& entry);
    void addImportedText(
        const QString& layerName,
        const QString& featureName,
        const QVector3D& position,
        const QString& text);

    void addImportedBitmap(
        const QString& layerName,
        const QString& featureName,
        const QVector3D& position,
        const QString& bitmapPath,
        const QVector3D& size,
        float rotation);


private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    IShapeHost* m_host = nullptr;
    ShapesFeature* m_shapesFeature = nullptr;
    LayerPanel* m_layerPanel = nullptr;

    // Temporary mesh storage
    std::vector<MeshEntry> m_tempMeshes;

    // Temporary vertices for polygon/line drawing
    std::vector<Vector*> m_tempPolygonVertices;
    std::vector<Vector*> m_tempLineVertices;
    std::vector<QPointF> m_tempPolygonCanvasPoints;
    std::vector<QPointF> m_tempLineCanvasPoints;

    // Counters
    int m_polygonCounter = 0;
    int m_polylineCounter = 0;
    int m_bitmapCounter = 0;
    int m_textCounter = 0;
    int m_userImageCounter = 0;

    // Shape dragging state
    bool m_isDraggingShape = false;
    QString m_draggingShapeId;
    QPointF m_shapeDragStartPos;
    std::vector<QString> m_selectedShapeIds;
    QString m_highlightedShapeId;

    // Edit mode state
    QString m_editingShapeId;
    bool m_isResizingShape = false;
    int m_selectedHandleIndex = -1;
    std::vector<QPointF> m_resizeHandles;

    // Rotate state
    QString m_activeRotateId;
    bool m_isRotatingBitmap = false;
    QString m_rotatingBitmapId;
    QPointF m_rotateHandleCenter;
    QPointF m_rotateHandleStartPos;
    qreal m_initialBitmapAngle = 0.0f;

    // Copy/Paste state
    MeshEntry* m_copiedShape = nullptr;
    // bool m_isPastePending = false;
    // Text editing state
    bool m_isEditingText = false;
    QString m_editingTextId;

    // Paste positioning state
    bool m_clickedSinceCopy = false;
    QPointF m_lastLeftClickGeoPos;

    // Bitmap state
    bool m_isPlacingBitmap = false;
    bool m_isDraggingBitmap = false;
    QString m_draggingBitmapId;
    QPointF m_bitmapDragStartPos;
    QString m_selectedBitmapType;

    // User image state
    bool m_isDraggingUserImage = false;
    QString m_draggingUserImageId;
    QPointF m_userImageDragStartPos;

    // Box selection state
    bool m_isBoxSelectionMode = false;
    bool m_isBoxSelecting = false;
    QPoint m_boxStartPos;
    QPoint m_boxCurrentPos;

    // Dialogs
    QDialog* m_shapePropertiesDialog = nullptr;
    QDialog* m_textPropertiesDialog = nullptr;
};

#endif // SHAPEEVENTHANDLER_H
