#ifndef ISHAPEHOST_H
#define ISHAPEHOST_H
#include <QString>
#include <QPointF>
#include <QPoint>
#include <QColor>
#include <QCursor>
#include <vector>
#include "GUI/Tacticaldisplay/Gis/transformmode.h"
#include "qpaintdevice.h"
class Vector;
struct MeshEntry;
class GISlib;
class LayerPanel;
class ShapesFeature;
/**
 * @brief Interface for any canvas/host that wants to use ShapesFeature
 */
class IShapeHost
{
public:
    virtual ~IShapeHost() = default;
    // ========================================================================
    // Data Access & Modification
    // ========================================================================
    virtual void addImportedMesh(const MeshEntry& entry) = 0;
    virtual std::vector<MeshEntry>& getTempMeshes() = 0;
    virtual std::vector<Vector*>& getTempPolygonVertices() = 0;
    virtual std::vector<Vector*>& getTempLineVertices() = 0;
    virtual std::vector<QPointF>& getTempPolygonCanvasPoints() = 0;
    virtual std::vector<QPointF>& getTempLineCanvasPoints() = 0;
    virtual MeshEntry* getEntityMeshEntry(const QString& entityId) = 0;
     virtual void selectEntitiesInBox(const QRectF& boxRect) = 0;
    // ========================================================================
    // UI Interaction
    // ========================================================================
    virtual void refreshView() = 0;
    virtual void forceRedraw() = 0;
    virtual void setCursor(const QCursor& cursor) = 0;
    virtual QPoint getMousePosition() const = 0;
    // ========================================================================
    // State & Mode Management
    // ========================================================================
    virtual void setCurrentMode(TransformMode mode) = 0;
    virtual TransformMode getCurrentMode() const = 0;
    virtual void setSelectedShape(const QString& shapeType) = 0;
    virtual QString getSelectedShape() const = 0;
    virtual bool isTrajectoryDrawingActive() const = 0;
    // by chaitanya
    virtual void startShapeEdit(const QString& shapeId) = 0;
    virtual void startShapeRotate(const QString& shapeId) = 0;
    virtual void cancelActiveShapeOperation() = 0;

    // ADDED - Chaitanya Jain
    // Reason: ContextMenuHandler needs the live waypoint count to decide whether
    // "Delete All Waypoints" should appear in the trajectory context menu.
    // Previously this value wasn't reachable through IShapeHost at all, so the
    // caller was passing a hardcoded 0.
    virtual int getTrajectoryWaypointCount() const = 0;

    // ========================================================================
    // Canvas Utilities
    // ========================================================================
    virtual GISlib* getGISlib() const = 0;
    virtual void setShapeDrawingMode(bool enabled, const QString& shapeType) = 0;
    virtual ShapesFeature* getShapesFeature() const = 0;
    // ========================================================================
    // Layer Management
    // ========================================================================
    virtual void addShapeToActiveLayer(const QString& shapeId, const QString& shapeType) = 0;
    virtual LayerPanel* getLayerPanel() const = 0;
    // ========================================================================
    // Logging
    // ========================================================================
    virtual void logMessage(const QString& message) = 0;
    virtual void logWarning(const QString& message) = 0;
    virtual void logError(const QString& message) = 0;
};
#endif // ISHAPEHOST_H
