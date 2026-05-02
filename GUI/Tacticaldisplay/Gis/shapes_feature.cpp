
//============================================================================
// ShapesFeature - ENHANCED WITH LAYER INTEGRATION
// Written by: Waris
// Modified: Added MS Paint-style drag-to-draw functionality for circles and rectangles
//           Added automatic layer assignment for all shapes
//
// Purpose:
// - Handles drawing of geometric shapes (circle, rectangle, line, polygon, point)
// - Manages temporary shape creation on the canvas
// - Provides undo/history management with preview support
// - Integrates scripted shape creation and movement
// - NEW: Drag-to-draw functionality for circles and rectangles
// - NEW: Automatic layer assignment when shapes are created
//============================================================================
#include "shapes_feature.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include "GUI/Tacticaldisplay/Gis/layerpanel.h"  // NEW: Include layer panel
#include "core/Hierarchy/Struct/vector.h"
#include <core/Debug/console.h>
#include <QPainter>
#include <cmath>

// ============================================================================
// ShapeState Implementation
// ============================================================================

ShapeState::ShapeState(const ShapeState& other)
    : position(other.position),
    rotation(other.rotation),
    size(other.size),
    layerName(other.layerName) {  // NEW: Copy layer name
    // Deep copy vertices
    for (Vector* v : other.vertices) {
        vertices.push_back(new Vector(v->x, v->y, v->z));
    }
}

ShapeState& ShapeState::operator=(const ShapeState& other) {
    if (this != &other) {
        clearVertices();
        position = other.position;
        rotation = other.rotation;
        size = other.size;
        layerName = other.layerName;  // NEW: Copy layer name

        // Deep copy vertices
        for (Vector* v : other.vertices) {
            vertices.push_back(new Vector(v->x, v->y, v->z));
        }
    }
    return *this;
}

ShapeState::~ShapeState() {
    clearVertices();
}

void ShapeState::clearVertices() {
    for (Vector* v : vertices) {
        delete v;
    }
    vertices.clear();
}

ShapesFeature::ShapesFeature(CanvasWidget* canvas)
    : m_canvas(canvas), m_gislib(canvas->gislib) {
}

void ShapesFeature::handleShapeDrawing(const QString& shapeType, const QPointF& geoPos, bool finalize) {
    if (m_canvas->currentMode != DrawShape || shapeType.isEmpty()) return;

    m_canvas->selectedShape = shapeType;

    // Circle and Rectangle are handled exclusively via drag-to-draw
    // They are not processed here
    if (shapeType == "Line") {
        drawLine(geoPos, finalize);
    } else if (shapeType == "Polygon") {
        drawPolygon(geoPos, finalize);
    } else if (shapeType == "Points") {
        drawPoints(geoPos);
    }
}


// ============================================================================
// NEW: Layer Integration Helper
// ============================================================================
void ShapesFeature::addShapeToActiveLayer(const QString& shapeId, const QString& shapeType) {
    // DEBUGGING: Print detailed info
    Console::log("========== addShapeToActiveLayer DEBUG ==========");
    Console::log("Shape ID: " + shapeId.toStdString());
    Console::log("Shape Type: " + shapeType.toStdString());
    // Console::log("LayerPanel pointer: " + (m_layerPanel ? "Valid" : "NULL"));

    if (m_layerPanel) {
        QString activeLayer = m_layerPanel->getActiveLayer();
        Console::log("Active Layer: " + (activeLayer.isEmpty() ? "EMPTY" : activeLayer.toStdString()));

        if (!activeLayer.isEmpty()) {
            m_layerPanel->addShapeToLayer(shapeId, shapeType, activeLayer);
            Console::log("Added " + shapeType.toStdString() + " (" + shapeId.toStdString() +
                         ") to layer: " + activeLayer.toStdString());
        } else {
            Console::warning("No active layer - shape " + shapeId.toStdString() +
                             " not added to any layer");
        }
    } else {
        Console::warning("Layer panel not connected - shape " + shapeId.toStdString() +
                         " not added to any layer");
    }
    Console::log("================================================");
}

// ============================================================================
// NEW: Drag-to-Draw Implementation
// ============================================================================

void ShapesFeature::startDragShape(const QString& shapeType, const QPointF& startGeoPos) {
    if (shapeType != "Circle" && shapeType != "Rectangle") {
        Console::warning("Drag-to-draw only supported for Circle and Rectangle");
        return;
    }

    m_isDraggingNewShape = true;
    m_dragShapeType = shapeType;
    m_dragStartGeoPos = startGeoPos;
    m_dragCurrentGeoPos = startGeoPos;

    // Reserve counter for this shape
    if (shapeType == "Circle") {
        m_dragShapeCounter = m_circleCounter;
    } else if (shapeType == "Rectangle") {
        m_dragShapeCounter = m_rectCounter;
    }

    Console::log("Started dragging " + shapeType.toStdString() + " at (lon: " +
                 std::to_string(startGeoPos.x()) + ", lat: " + std::to_string(startGeoPos.y()) + ")");
    m_canvas->Refresh();
}

void ShapesFeature::updateDragShape(const QPointF& currentGeoPos) {
    if (!m_isDraggingNewShape) return;

    m_dragCurrentGeoPos = currentGeoPos;
    m_canvas->Refresh();
}

void ShapesFeature::finalizeDragShape() {
    if (!m_isDraggingNewShape) return;

    // Create shape regardless of drag distance
    if (m_dragShapeType == "Circle") {
        finalizeCircleDrag();
    } else if (m_dragShapeType == "Rectangle") {
        finalizeRectangleDrag();
    }

    // Reset drag state
    m_isDraggingNewShape = false;
    m_dragShapeType = "";
    m_canvas->currentMode = Translate;
    m_canvas->setCursor(Qt::ArrowCursor);
    m_canvas->Refresh();
}

void ShapesFeature::cancelDragShape() {
    if (!m_isDraggingNewShape) return;

    Console::log("Cancelled shape dragging");
    m_isDraggingNewShape = false;
    m_dragShapeType = "";
    m_canvas->Refresh();
}

void ShapesFeature::drawDragPreview(QPainter& painter, GISlib* gislib) {
    if (!m_isDraggingNewShape) return;

    painter.save();

    // Set preview style - dashed line
    QPen previewPen(Qt::blue, 2, Qt::DashLine);
    painter.setPen(previewPen);
    painter.setBrush(QColor(0, 0, 255, 30)); // Semi-transparent blue

    QPointF startCanvas = gislib->geoToCanvas(m_dragStartGeoPos.y(), m_dragStartGeoPos.x());
    QPointF currentCanvas = gislib->geoToCanvas(m_dragCurrentGeoPos.y(), m_dragCurrentGeoPos.x());

    if (m_dragShapeType == "Circle") {
        // Calculate radius from drag distance
        qreal radius = QVector2D(currentCanvas - startCanvas).length();

        if (radius > 0) {
            painter.drawEllipse(startCanvas, radius, radius);

            // Draw radius line
            painter.setPen(QPen(Qt::blue, 1, Qt::DotLine));
            painter.drawLine(startCanvas, currentCanvas);

            // Draw center point
            painter.setBrush(Qt::blue);
            painter.drawEllipse(startCanvas, 3, 3);
        }
    } else if (m_dragShapeType == "Rectangle") {
        // Draw rectangle from start to current position
        QRectF rect = QRectF(startCanvas, currentCanvas).normalized();

        if (rect.width() > 0 && rect.height() > 0) {
            painter.drawRect(rect);

            // Draw corner handles
            painter.setBrush(Qt::blue);
            painter.drawRect(QRectF(rect.topLeft().x() - 3, rect.topLeft().y() - 3, 6, 6));
            painter.drawRect(QRectF(rect.topRight().x() - 3, rect.topRight().y() - 3, 6, 6));
            painter.drawRect(QRectF(rect.bottomLeft().x() - 3, rect.bottomLeft().y() - 3, 6, 6));
            painter.drawRect(QRectF(rect.bottomRight().x() - 3, rect.bottomRight().y() - 3, 6, 6));
        }
    }

    painter.restore();
}

void ShapesFeature::finalizeCircleDrag() {
    // Center is at the start position
    QPointF centerGeo = m_dragStartGeoPos;

    // Calculate radius in CANVAS coordinates (pixels) first
    QPointF startCanvas = m_gislib->geoToCanvas(m_dragStartGeoPos.y(), m_dragStartGeoPos.x());
    QPointF currentCanvas = m_gislib->geoToCanvas(m_dragCurrentGeoPos.y(), m_dragCurrentGeoPos.x());
    qreal radiusPixels = QVector2D(currentCanvas - startCanvas).length();

    // Convert the radius from pixels to geographic units
    // Create a point at the edge of the circle (in canvas coordinates)
    QPointF radiusPointCanvas = startCanvas + QPointF(radiusPixels, 0);

    // Convert that edge point back to geographic coordinates
    QPointF radiusPointGeo = m_gislib->canvasToGeo(radiusPointCanvas);

    // The geographic radius is the distance between center and edge point
    qreal dx = radiusPointGeo.x() - centerGeo.x();
    qreal dy = radiusPointGeo.y() - centerGeo.y();
    float radiusGeo = std::sqrt(dx * dx + dy * dy);

    // Create shape ID
    QString shapeId = QString("TempCircle_%1").arg(m_circleCounter);

    MeshEntry entry;
    entry.name = shapeId;
    entry.position = new QVector3D(centerGeo.x(), centerGeo.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(radiusGeo, radiusGeo, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";
    entry.mesh = new Mesh();

    if (!entry.mesh) {
        Console::error("Failed to allocate Mesh for circle");
        return;
    }

    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;
    entry.mesh->polygen.push_back(new Vector(0, 0, 0));

    m_canvas->tempMeshes.push_back(entry);

    Console::log("Created dragged circle with radius " + std::to_string(radiusGeo) +
                 " (from " + std::to_string(radiusPixels) + " pixels) at (lon: " +
                 std::to_string(centerGeo.x()) + ", lat: " + std::to_string(centerGeo.y()) + ")");

    // Increment counter
    m_circleCounter++;

    // NEW: Add to active layer
    addShapeToActiveLayer(shapeId, "Circle");
}

void ShapesFeature::finalizeRectangleDrag() {
    // Calculate center point in geographic coordinates
    qreal centerLon = (m_dragStartGeoPos.x() + m_dragCurrentGeoPos.x()) / 2.0;
    qreal centerLat = (m_dragStartGeoPos.y() + m_dragCurrentGeoPos.y()) / 2.0;
    QPointF centerGeo(centerLon, centerLat);

    // Calculate half-width and half-height in geographic coordinates
    qreal halfWidth = std::abs(m_dragCurrentGeoPos.x() - m_dragStartGeoPos.x()) / 2.0;
    qreal halfHeight = std::abs(m_dragCurrentGeoPos.y() - m_dragStartGeoPos.y()) / 2.0;

    // Create shape ID
    QString shapeId = QString("TempRectangle_%1").arg(m_rectCounter);

    MeshEntry entry;
    entry.name = shapeId;
    entry.position = new QVector3D(centerGeo.x(), centerGeo.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(halfWidth * 2, halfHeight * 2, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";
    entry.mesh = new Mesh();

    if (!entry.mesh) {
        Console::error("Failed to allocate Mesh for rectangle");
        return;
    }

    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;

    entry.mesh->polygen = {
        new Vector(-halfWidth, -halfHeight, 0),
        new Vector(halfWidth, -halfHeight, 0),
        new Vector(halfWidth, halfHeight, 0),
        new Vector(-halfWidth, halfHeight, 0)
    };

    m_canvas->tempMeshes.push_back(entry);

    Console::log("Created dragged rectangle at (lon: " + std::to_string(centerGeo.x()) +
                 ", lat: " + std::to_string(centerGeo.y()) + ") with size " +
                 std::to_string(halfWidth * 2) + "x" + std::to_string(halfHeight * 2));

    // Increment counter
    m_rectCounter++;

    // NEW: Add to active layer
    addShapeToActiveLayer(shapeId, "Rectangle");
}

QVector3D ShapesFeature::calculateCircleProperties(const QPointF& start, const QPointF& current) {
    // Center is at the start point
    QPointF centerGeo = start;

    // Calculate radius in geographic coordinates
    qreal dx = current.x() - start.x();
    qreal dy = current.y() - start.y();
    float radiusGeo = std::sqrt(dx * dx + dy * dy);

    return QVector3D(centerGeo.x(), centerGeo.y(), radiusGeo);
}

QRectF ShapesFeature::calculateRectangleProperties(const QPointF& start, const QPointF& current) {
    // Create rectangle from two corner points
    qreal left = std::min(start.x(), current.x());
    qreal right = std::max(start.x(), current.x());
    qreal top = std::max(start.y(), current.y());
    qreal bottom = std::min(start.y(), current.y());

    return QRectF(QPointF(left, bottom), QPointF(right, top));
}

// ============================================================================
// DEPRECATED: Original Shape Drawing Functions
// These functions are kept for backward compatibility only (e.g., script usage)
// For interactive drawing, use drag-to-draw methods (startDragShape, etc.)
// ============================================================================

void ShapesFeature::drawCircle(const QPointF& geoPos) {
    // Create shape ID
    QString shapeId = QString("TempCircle_%1").arg(m_circleCounter);

    MeshEntry entry;
    entry.name = shapeId;
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(2.1, 2.1, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";
    entry.mesh = new Mesh();

    if (!entry.mesh) {
        Console::error("Failed to allocate Mesh for circle");
        return;
    }

    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;
    entry.mesh->polygen.push_back(new Vector(0, 0, 0));

    m_canvas->tempMeshes.push_back(entry);

    // Switch to Translate mode after creating circle
    m_canvas->currentMode = Translate;
    m_canvas->setCursor(Qt::ArrowCursor);

    Console::log("Created temporary circle at (lon: " + std::to_string(geoPos.x()) +
                 ", lat: " + std::to_string(geoPos.y()) + ")");

    // Increment counter
    m_circleCounter++;

    // NEW: Add to active layer
    addShapeToActiveLayer(shapeId, "Circle");

    m_canvas->Refresh();
}

void ShapesFeature::drawRectangle(const QPointF& geoPos) {
    // Create shape ID
    QString shapeId = QString("TempRectangle_%1").arg(m_rectCounter);

    MeshEntry entry;
    entry.name = shapeId;
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(2.1, 2.1, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";
    entry.mesh = new Mesh();

    if (!entry.mesh) {
        Console::error("Failed to allocate Mesh for rectangle");
        return;
    }

    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;

    float halfWidth = 0.01f;
    float halfHeight = 0.005f;
    entry.mesh->polygen = {
        new Vector(-halfWidth, -halfHeight, 0),
        new Vector(halfWidth, -halfHeight, 0),
        new Vector(halfWidth, halfHeight, 0),
        new Vector(-halfWidth, halfHeight, 0)
    };

    m_canvas->tempMeshes.push_back(entry);

    // Switch to Translate mode after creating rectangle
    m_canvas->currentMode = Translate;
    m_canvas->setCursor(Qt::ArrowCursor);

    Console::log("Created temporary rectangle at (lon: " + std::to_string(geoPos.x()) +
                 ", lat: " + std::to_string(geoPos.y()) + ")");

    // Increment counter
    m_rectCounter++;

    // NEW: Add to active layer
    addShapeToActiveLayer(shapeId, "Rectangle");

    m_canvas->Refresh();
}

void ShapesFeature::drawLine(const QPointF& geoPos, bool finalize) {
    if (!finalize) {
        // Add vertex to CanvasWidget's temporary line vector
        m_canvas->tempLineVertices.push_back(new Vector(geoPos.x(), geoPos.y(), 0));
        m_canvas->tempLineCanvasPoints.push_back(m_gislib->geoToCanvas(geoPos.y(), geoPos.x()));
        Console::log("Added line vertex at (lon: " + std::to_string(geoPos.x()) +
                     ", lat: " + std::to_string(geoPos.y()) + ")");
        m_canvas->Refresh();
    } else if (m_canvas->tempLineVertices.size() >= 2) {
        // Create shape ID
        QString shapeId = QString("TempPolyline_%1").arg(m_polylineCounter);

        // Finalize line using CanvasWidget's vectors
        MeshEntry entry;
        entry.name = shapeId;

        // Calculate centroid
        float avgX = 0, avgY = 0;
        for (const Vector* v : m_canvas->tempLineVertices) {
            avgX += v->x;
            avgY += v->y;
        }
        avgX /= m_canvas->tempLineVertices.size();
        avgY /= m_canvas->tempLineVertices.size();

        entry.position = new QVector3D(avgX, avgY, 0);
        entry.rotation = new QQuaternion();
        entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
        entry.velocity = new QVector3D(0, 0, 0);
        entry.trajectory = nullptr;
        entry.collider = nullptr;
        entry.bitmapPath = "";
        entry.text = "";
        entry.mesh = new Mesh();

        if (!entry.mesh) {
            Console::error("Failed to allocate Mesh for line");
            return;
        }

        entry.mesh->color = new QColor(Qt::red);
        entry.mesh->lineWidth = 2;
        entry.mesh->closePath = false;

        // Store vertices as relative to centroid
        for (Vector* v : m_canvas->tempLineVertices) {
            entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
        }

        m_canvas->tempMeshes.push_back(entry);
        addShapeToActiveLayer(shapeId, "Line");

        // Clear CanvasWidget's temporary data
        for (Vector* v : m_canvas->tempLineVertices) {
            delete v;
        }
        m_canvas->tempLineVertices.clear();
        m_canvas->tempLineCanvasPoints.clear();

        // Switch to Translate mode
        m_canvas->currentMode = Translate;
        m_canvas->setCursor(Qt::ArrowCursor);

        Console::log("Created temporary polyline with " +
                     std::to_string(entry.mesh->polygen.size()) + " vertices");

        // Increment counter
        m_polylineCounter++;

        // Add to active layer
        addShapeToActiveLayer(shapeId, "Line");

        m_canvas->Refresh();
    }
}

void ShapesFeature::drawPolygon(const QPointF& geoPos, bool finalize) {
    if (!finalize) {
        // Add vertex to CanvasWidget's temporary polygon vector
        m_canvas->tempPolygonVertices.push_back(new Vector(geoPos.x(), geoPos.y(), 0));
        m_canvas->tempPolygonCanvasPoints.push_back(m_gislib->geoToCanvas(geoPos.y(), geoPos.x()));
        Console::log("Added polygon vertex at (lon: " + std::to_string(geoPos.x()) +
                     ", lat: " + std::to_string(geoPos.y()) + ")");
        m_canvas->Refresh();
    } else if (m_canvas->tempPolygonVertices.size() >= 3) {
        // Create shape ID
        QString shapeId = QString("TempPolygon_%1").arg(m_polygonCounter);

        // Finalize polygon using CanvasWidget's vectors
        MeshEntry entry;
        entry.name = shapeId;

        // Calculate centroid
        float avgX = 0, avgY = 0;
        for (const Vector* v : m_canvas->tempPolygonVertices) {
            avgX += v->x;
            avgY += v->y;
        }
        avgX /= m_canvas->tempPolygonVertices.size();
        avgY /= m_canvas->tempPolygonVertices.size();

        entry.position = new QVector3D(avgX, avgY, 0);
        entry.rotation = new QQuaternion();
        entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
        entry.velocity = new QVector3D(0, 0, 0);
        entry.trajectory = nullptr;
        entry.collider = nullptr;
        entry.bitmapPath = "";
        entry.text = "";
        entry.mesh = new Mesh();

        if (!entry.mesh) {
            Console::error("Failed to allocate Mesh for polygon");
            return;
        }

        entry.mesh->color = new QColor(Qt::red);
        entry.mesh->lineWidth = 2;
        entry.mesh->closePath = true;

        // Store vertices as relative to centroid
        for (Vector* v : m_canvas->tempPolygonVertices) {
            entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
        }

        m_canvas->tempMeshes.push_back(entry);
        addShapeToActiveLayer(shapeId, "Polygon");

        // Clear CanvasWidget's temporary data
        for (Vector* v : m_canvas->tempPolygonVertices) {
            delete v;
        }
        m_canvas->tempPolygonVertices.clear();
        m_canvas->tempPolygonCanvasPoints.clear();

        // Switch to Translate mode
        m_canvas->currentMode = Translate;
        m_canvas->setCursor(Qt::ArrowCursor);

        Console::log("Created temporary polygon with " +
                     std::to_string(entry.mesh->polygen.size()) + " vertices");

        // Increment counter
        m_polygonCounter++;

        // Add to active layer
        addShapeToActiveLayer(shapeId, "Polygon");

        m_canvas->Refresh();
    }
}

void ShapesFeature::drawPoints(const QPointF& geoPos) {
    // Create shape ID
    QString shapeId = QString("TempPoint_%1").arg(m_pointCounter);

    MeshEntry entry;
    entry.name = shapeId;
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(0, 0, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";
    entry.mesh = new Mesh();

    if (!entry.mesh) {
        Console::error("Failed to allocate Mesh for point");
        return;
    }

    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 3;
    entry.mesh->closePath = true;
    entry.mesh->polygen.push_back(new Vector(0, 0, 0));

    m_canvas->tempMeshes.push_back(entry);

    Console::log("Created temporary point at (lon: " + std::to_string(geoPos.x()) +
                 ", lat: " + std::to_string(geoPos.y()) + ")");

    // Increment counter
    m_pointCounter++;

    // NEW: Add to active layer
    addShapeToActiveLayer(shapeId, "Point");

    m_canvas->Refresh();
}

// ============================================================================
// History Management Implementation (unchanged)
// ============================================================================

ShapeState ShapesFeature::captureState(MeshEntry* entry) {
    ShapeState state;
    if (entry->position) state.position = *entry->position;
    if (entry->rotation) state.rotation = *entry->rotation;
    if (entry->size) state.size = *entry->size;

    if (entry->mesh && !entry->mesh->polygen.empty()) {
        for (Vector* v : entry->mesh->polygen) {
            state.vertices.push_back(new Vector(v->x, v->y, v->z));
        }
    }

    // NEW: Capture layer information if layer panel is available
    if (m_layerPanel) {
        state.layerName = m_layerPanel->getLayerForShape(QString::fromStdString(entry->name.toStdString()));
    }

    return state;
}

void ShapesFeature::applyState(const ShapeState& state, MeshEntry* entry) {
    if (entry->position) *entry->position = state.position;
    if (entry->rotation) *entry->rotation = state.rotation;
    if (entry->size) *entry->size = state.size;

    if (entry->mesh && !state.vertices.empty()) {
        for (Vector* v : entry->mesh->polygen) {
            delete v;
        }
        entry->mesh->polygen.clear();

        for (Vector* v : state.vertices) {
            entry->mesh->polygen.push_back(new Vector(v->x, v->y, v->z));
        }
    }
}

void ShapesFeature::saveShapeState(const QString& shapeId, MeshEntry* entry) {
    if (!entry) return;

    ShapeState currentState = captureState(entry);

    if (!m_shapeHistory.contains(shapeId)) {
        m_shapeHistory[shapeId] = QStack<ShapeState>();
    }

    m_shapeHistory[shapeId].push(currentState);

    if (m_shapeHistory[shapeId].size() > MAX_HISTORY_DEPTH) {
        m_shapeHistory[shapeId].removeFirst();
    }

    Console::log("Saved state for shape " + shapeId.toStdString() +
                 " (history depth: " + std::to_string(m_shapeHistory[shapeId].size()) + ")");
}

bool ShapesFeature::restorePreviousState(const QString& shapeId, MeshEntry* entry) {
    if (!entry || !m_shapeHistory.contains(shapeId) || m_shapeHistory[shapeId].isEmpty()) {
        return false;
    }

    ShapeState previousState = m_shapeHistory[shapeId].pop();
    applyState(previousState, entry);

    Console::log("Restored previous state for shape " + shapeId.toStdString() +
                 " (remaining history: " + std::to_string(m_shapeHistory[shapeId].size()) + ")");

    m_canvas->Refresh();
    return true;
}

bool ShapesFeature::hasHistory(const QString& shapeId) const {
    return m_shapeHistory.contains(shapeId) && !m_shapeHistory[shapeId].isEmpty();
}

void ShapesFeature::clearHistory(const QString& shapeId) {
    if (m_shapeHistory.contains(shapeId)) {
        m_shapeHistory[shapeId].clear();
        m_shapeHistory.remove(shapeId);
        Console::log("Cleared history for shape " + shapeId.toStdString());
    }
}

void ShapesFeature::clearAllHistory() {
    m_shapeHistory.clear();
    Console::log("Cleared all shape history");
}

// ============================================================================
// History Preview Implementation (unchanged)
// ============================================================================

void ShapesFeature::showHistoryPreview(const QString& shapeId) {
    if (!m_shapeHistory.contains(shapeId) || m_shapeHistory[shapeId].isEmpty()) {
        Console::log("No history to preview for shape " + shapeId.toStdString());
        return;
    }

    m_previewState = m_shapeHistory[shapeId].top();
    m_previewShapeId = shapeId;
    m_showingPreview = true;

    Console::log("Showing history preview for shape " + shapeId.toStdString());
    m_canvas->Refresh();
}

void ShapesFeature::hideHistoryPreview() {
    if (m_showingPreview) {
        m_showingPreview = false;
        m_previewShapeId.clear();
        m_previewState.clearVertices();
        Console::log("Hidden history preview");
        m_canvas->Refresh();
    }
}

void ShapesFeature::drawHistoryPreview(QPainter& painter, GISlib* gislib) {
    if (!m_showingPreview || m_previewShapeId.isEmpty()) {
        return;
    }

    MeshEntry* currentEntry = nullptr;
    for (auto& entry : m_canvas->tempMeshes) {
        if (entry.name == m_previewShapeId) {
            currentEntry = &entry;
            break;
        }
    }

    if (!currentEntry) {
        return;
    }

    painter.save();

    QPen previewPen(Qt::green, 2, Qt::DotLine);
    painter.setPen(previewPen);
    painter.setBrush(QColor(0, 255, 0, 30));

    QPointF centerGeo(m_previewState.position.x(), m_previewState.position.y());
    QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());

    float rotationRad = m_previewState.rotation.z();
    float cosFwd = std::cos(rotationRad);
    float sinFwd = std::sin(rotationRad);

    if (currentEntry->name.startsWith("TempCircle")) {
        QPointF radiusPointGeo(m_previewState.position.x() + m_previewState.size.x(),
                               m_previewState.position.y());
        QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
        float radius = QVector2D(radiusPointCanvas - centerCanvas).length();

        painter.drawEllipse(centerCanvas, radius, radius);

        painter.setBrush(Qt::green);
        painter.drawRect(QRectF(radiusPointCanvas.x() - 4, radiusPointCanvas.y() - 4, 8, 8));
    }
    else if (currentEntry->name.startsWith("TempPoint")) {
        painter.setBrush(Qt::green);
        painter.drawEllipse(centerCanvas, 4, 4);
    }
    else if (!m_previewState.vertices.empty()) {
        QPolygonF polygon;
        QVector<QPointF> handles;

        for (Vector* v : m_previewState.vertices) {
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;

            QPointF vGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
            QPointF vCanvas = gislib->geoToCanvas(vGeo.y(), vGeo.x());
            polygon << vCanvas;
            handles.push_back(vCanvas);
        }

        if (!polygon.isEmpty()) {
            if (currentEntry->name.startsWith("TempPolyline")) {
                painter.drawPolyline(polygon);
            } else {
                painter.drawPolygon(polygon);
            }

            painter.setBrush(Qt::green);
            for (const QPointF& handle : handles) {
                painter.drawRect(QRectF(handle.x() - 4, handle.y() - 4, 8, 8));
            }
        }
    }

    painter.restore();
}

// ============================================================================
// Script Functions (unchanged)
// ============================================================================

void ShapesFeature::scriptStartLine() {
    m_canvas->currentMode = DrawShape;
    m_canvas->selectedShape = "Line";

    for (auto* v : m_tempLineVertices) delete v;
    m_tempLineVertices.clear();
    m_tempLineCanvasPoints.clear();
}

void ShapesFeature::scriptAddLinePoint(const QPointF& geoPos) {
    drawLine(geoPos, false);
}

void ShapesFeature::scriptFinishLine()
{
    if (m_tempLineVertices.size() < 2)
        return;

    QString lineName = QString("Line_%1").arg(m_vertexCounter++);

    MeshEntry entry;
    entry.name = lineName;
    entry.position = new QVector3D(0,0,0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(0,0,1);
    entry.velocity = new QVector3D(0,0,0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";

    entry.mesh = new Mesh();
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 3;
    entry.mesh->closePath = false;

    // add vertices to the line mesh
    for (Vector* v : m_tempLineVertices) {
        entry.mesh->polygen.push_back(new Vector(v->x, v->y, 0));
    }

    m_canvas->tempMeshes.push_back(entry);

    // add to layer
    addShapeToActiveLayer(lineName, "Line");

    m_canvas->selectedShape.clear();

    // force exit drawing state
    m_canvas->currentMode = Translate;
    m_canvas->setShapeDrawingMode(false, "");

    // clear internal shape tool
    m_canvas->selectedShape = "";

    m_canvas->setCursor(Qt::ArrowCursor);

    for (auto* v : m_tempLineVertices)
        delete v;

    m_tempLineVertices.clear();
    m_tempLineCanvasPoints.clear();

    m_canvas->Refresh();
}

bool ShapesFeature::isPointNearLineSegment(const QPointF& p, const QPointF& v1, const QPointF& v2, qreal tolerance) {
    QPointF vec = v2 - v1;
    QPointF vp = p - v1;
    qreal lenSquared = vec.x() * vec.x() + vec.y() * vec.y();

    if (lenSquared < 1e-6) {
        return QVector2D(p - v1).length() < tolerance;
    }

    qreal t = std::max<qreal>(0.0, std::min<qreal>(1.0, (vp.x() * vec.x() + vp.y() * vec.y()) / lenSquared));
    QPointF projection = v1 + t * vec;
    return QVector2D(p - projection).length() < tolerance;
}

///////////// moveshape using script by amjad ///////////////
void ShapesFeature::moveShapeByScript(MeshEntry* entry, const QPointF& newGeoPos)
{
    if (!entry || !entry->position) return;

    saveShapeState(entry->name, entry);

    entry->position->setX(newGeoPos.x());
    entry->position->setY(newGeoPos.y());

    m_canvas->Refresh();
}

// draw circle and adjust their radius by amjad
void ShapesFeature::drawCircle(const QPointF& geoPos, float radiusDeg)
{
    MeshEntry entry;
    entry.name = QString("TempCircle_%1").arg(m_circleCounter++);
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(radiusDeg, radiusDeg, 1);
    entry.circleRadius = radiusDeg;

    entry.velocity = new QVector3D(0, 0, 0);
    entry.mesh = new Mesh();
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;

    const int SEG = 64;
    for (int i = 0; i < SEG; ++i) {
        float a = (2 * M_PI * i) / SEG;
        entry.mesh->polygen.push_back(
            new Vector(std::cos(a) * radiusDeg,
                       std::sin(a) * radiusDeg, 0));
    }

    m_canvas->tempMeshes.push_back(entry);

    // add to layer
    // addShapeToActiveLayer(entry.name, "Circle");

    m_canvas->Refresh();
}

// draw rectange and adjust their heigth and width by amjad
void ShapesFeature::drawRectangle(
    const QPointF& geoPos,
    float widthDeg,
    float heightDeg)
{
    MeshEntry entry;
    entry.name = QString("TempRectangle_%1").arg(m_rectCounter++);
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new QQuaternion();

    entry.size = new QVector3D(widthDeg, heightDeg, 1);
    // entry.isScriptShape = true;

    entry.velocity = new QVector3D(0, 0, 0);
    entry.mesh = new Mesh();
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;

    float hw = widthDeg  / 2.0f;
    float hh = heightDeg / 2.0f;

    entry.mesh->polygen = {
        new Vector(-hw, -hh, 0),
        new Vector( hw, -hh, 0),
        new Vector( hw,  hh, 0),
        new Vector(-hw,  hh, 0)
    };

    m_canvas->tempMeshes.push_back(entry);

    addShapeToActiveLayer(entry.name, "Rectangle");

    m_canvas->Refresh();
}
