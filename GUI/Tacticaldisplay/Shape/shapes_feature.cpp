// shapes_feature.cpp
#include "shapes_feature.h"
#include "../canvaswidget.h"     // still needed for MeshEntry definition (struct lives here)
#include "../Gis/gislib.h"
#include "../Layer/layerpanel.h"
#include "../../../core/Hierarchy/Struct/vector.h"
#include "../../../core/Hierarchy/Components/mesh.h"
#include "../../../core/Debug/console.h"
#include <QPainter>
#include <cmath>
#include <QRegularExpression>

// ============================================================================
// ShapeState Implementation
// ============================================================================

ShapeState::ShapeState(const ShapeState& other)
    : position(other.position)
    , rotation(other.rotation)
    , size(other.size)
    , layerName(other.layerName)
{
    for (Vector* v : other.vertices) {
        vertices.push_back(new Vector(v->x, v->y, v->z));
    }
}

ShapeState& ShapeState::operator=(const ShapeState& other)
{
    if (this != &other) {
        clearVertices();
        position = other.position;
        rotation = other.rotation;
        size = other.size;
        layerName = other.layerName;
        for (Vector* v : other.vertices) {
            vertices.push_back(new Vector(v->x, v->y, v->z));
        }
    }
    return *this;
}

ShapeState::~ShapeState()
{
    clearVertices();
}

void ShapeState::clearVertices()
{
    for (Vector* v : vertices) {
        delete v;
    }
    vertices.clear();
}

// ============================================================================
// ShapesFeature Implementation
// ============================================================================

ShapesFeature::ShapesFeature(IShapeHost* host)
    : m_host(host)
    , m_layerPanel(nullptr)
{
    if (m_host) {
        m_layerPanel = m_host->getLayerPanel();
    }
}

ShapesFeature::~ShapesFeature()
{
    // Clean up temporary vertices
    for (Vector* v : m_tempLineVertices) delete v;
    m_tempLineVertices.clear();

    for (Vector* v : m_tempPolygonVertices) delete v;
    m_tempPolygonVertices.clear();

    // Clean up history states
    clearAllHistory();
}

// ============================================================================
// Layer Integration Helper
// ============================================================================

void ShapesFeature::addShapeToActiveLayer(const QString& shapeId, const QString& shapeType)
{
    Console::log("========== addShapeToActiveLayer DEBUG ==========");
    Console::log("Shape ID: " + shapeId.toStdString());
    Console::log("Shape Type: " + shapeType.toStdString());

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
// Shape Drawing - Main Entry Point
// ============================================================================

void ShapesFeature::handleShapeDrawing(const QString& shapeType, const QPointF& geoPos, bool finalize)
{
    qDebug() << "[ShapesFeature] handleShapeDrawing"
             << shapeType
             << geoPos
             << finalize;

    if (!m_host) return;

    if (m_host->getCurrentMode() != DrawShape || shapeType.isEmpty()) {
        return;
    }

    m_host->setSelectedShape(shapeType);

    if (shapeType == "Line") {
        drawLine(geoPos, finalize);
    } else if (shapeType == "Polygon") {
        drawPolygon(geoPos, finalize);
    } else if (shapeType == "Points") {
        drawPoints(geoPos);
    }
}

// ============================================================================
// Shape Drawing - Individual Shapes
// ============================================================================

void ShapesFeature::drawCircle(const QPointF& geoPos)
{
    if (!m_host) return;

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

    m_host->getTempMeshes().push_back(entry);

    m_host->setCurrentMode(Translate);
    m_host->setCursor(Qt::ArrowCursor);

    m_circleCounter++;
    addShapeToActiveLayer(shapeId, "Circle");
    m_host->refreshView();
}

void ShapesFeature::drawCircle(const QPointF& geoPos, float radiusDeg)
{
    if (!m_host) return;

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

    m_host->getTempMeshes().push_back(entry);
    addShapeToActiveLayer(entry.name, "Circle");
    m_host->refreshView();
}

void ShapesFeature::drawRectangle(const QPointF& geoPos)
{
    if (!m_host) return;

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

    m_host->getTempMeshes().push_back(entry);

    m_host->setCurrentMode(Translate);
    m_host->setCursor(Qt::ArrowCursor);
    m_rectCounter++;
    addShapeToActiveLayer(shapeId, "Rectangle");
    m_host->refreshView();
}

void ShapesFeature::drawRectangle(const QPointF& geoPos, float widthDeg, float heightDeg)
{
    if (!m_host) return;

    MeshEntry entry;
    entry.name = QString("TempRectangle_%1").arg(m_rectCounter++);
    entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(widthDeg, heightDeg, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.mesh = new Mesh();
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 2;
    entry.mesh->closePath = true;

    float hw = widthDeg / 2.0f;
    float hh = heightDeg / 2.0f;

    entry.mesh->polygen = {
        new Vector(-hw, -hh, 0),
        new Vector( hw, -hh, 0),
        new Vector( hw,  hh, 0),
        new Vector(-hw,  hh, 0)
    };

    m_host->getTempMeshes().push_back(entry);
    addShapeToActiveLayer(entry.name, "Rectangle");
    m_host->refreshView();
}

void ShapesFeature::drawLine(const QPointF& geoPos, bool finalize)
{
    if (!m_host) return;

    // Use external vectors if available, otherwise use internal ones
    std::vector<Vector*>* lineVerts = m_externalLineVertices ?
                                           m_externalLineVertices : &m_tempLineVertices;
    std::vector<QPointF>* linePoints = m_externalLineCanvasPoints ?
                                           m_externalLineCanvasPoints : &m_tempLineCanvasPoints;

    if (!finalize) {
        lineVerts->push_back(new Vector(geoPos.x(), geoPos.y(), 0));
        if (m_host->getGISlib()) {
            linePoints->push_back(m_host->getGISlib()->geoToCanvas(geoPos.y(), geoPos.x()));
        }
        Console::log("Line vertex added: " + std::to_string(lineVerts->size()));
        m_host->refreshView();
        m_host->forceRedraw();
    } else if (lineVerts->size() >= 2) {
        QString shapeId = QString("TempPolyline_%1").arg(m_polylineCounter);

        MeshEntry entry;
        entry.name = shapeId;

        float avgX = 0, avgY = 0;
        for (const Vector* v : *lineVerts) {
            avgX += v->x;
            avgY += v->y;
        }
        avgX /= lineVerts->size();
        avgY /= lineVerts->size();

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

        for (Vector* v : *lineVerts) {
            entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
        }

        m_host->getTempMeshes().push_back(entry);
        addShapeToActiveLayer(shapeId, "Line");

        for (Vector* v : *lineVerts) {
            delete v;
        }
        lineVerts->clear();
        linePoints->clear();

        m_host->setCurrentMode(Translate);
        m_host->setCursor(Qt::ArrowCursor);
        m_polylineCounter++;
        m_host->refreshView();
        m_host->forceRedraw();
    }
}
void ShapesFeature::drawPolygon(const QPointF& geoPos, bool finalize)
{
    if (!m_host) return;

    // Use external vectors if available, otherwise use internal ones
    std::vector<Vector*>* polyVerts = m_externalPolygonVertices ?
                                           m_externalPolygonVertices : &m_tempPolygonVertices;
    std::vector<QPointF>* polyPoints = m_externalPolygonCanvasPoints ?
                                           m_externalPolygonCanvasPoints : &m_tempPolygonCanvasPoints;

    if (!finalize) {
        polyVerts->push_back(new Vector(geoPos.x(), geoPos.y(), 0));
        if (m_host->getGISlib()) {
            polyPoints->push_back(m_host->getGISlib()->geoToCanvas(geoPos.y(), geoPos.x()));
        }
        Console::log("Polygon vertex added: " + std::to_string(polyVerts->size()));
        m_host->refreshView();
        m_host->forceRedraw();
    } else if (polyVerts->size() >= 3) {
        QString shapeId = QString("TempPolygon_%1").arg(m_polygonCounter);

        MeshEntry entry;
        entry.name = shapeId;

        float avgX = 0, avgY = 0;
        for (const Vector* v : *polyVerts) {
            avgX += v->x;
            avgY += v->y;
        }
        avgX /= polyVerts->size();
        avgY /= polyVerts->size();

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

        for (Vector* v : *polyVerts) {
            entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
        }

        m_host->getTempMeshes().push_back(entry);
        addShapeToActiveLayer(shapeId, "Polygon");

        // Clear vertices
        for (Vector* v : *polyVerts) {
            delete v;
        }
        polyVerts->clear();
        polyPoints->clear();

        m_host->setCurrentMode(Translate);
        m_host->setCursor(Qt::ArrowCursor);
        m_polygonCounter++;
        m_host->refreshView();
        m_host->forceRedraw();
    }
}
void ShapesFeature::drawPoints(const QPointF& geoPos)
{
    if (!m_host) return;

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

    m_host->getTempMeshes().push_back(entry);

    Console::log("Created temporary point at (lon: " + std::to_string(geoPos.x()) +
                 ", lat: " + std::to_string(geoPos.y()) + ")");

    m_pointCounter++;
    addShapeToActiveLayer(shapeId, "Point");
    m_host->setCurrentMode(Translate);
    m_host->setSelectedShape("");
    m_host->setCursor(Qt::ArrowCursor);
    m_host->refreshView();
}

// ============================================================================
// Drag-to-Draw Implementation
// ============================================================================

void ShapesFeature::startDragShape(const QString& shapeType, const QPointF& startGeoPos)
{
    if (!m_host) return;

    if (shapeType != "Circle" && shapeType != "Rectangle") {
        Console::warning("Drag-to-draw only supported for Circle and Rectangle");
        return;
    }

    m_isDraggingNewShape = true;
    m_dragShapeType = shapeType;
    m_dragStartGeoPos = startGeoPos;
    m_dragCurrentGeoPos = startGeoPos;

    if (shapeType == "Circle") {
        m_dragShapeCounter = m_circleCounter;
    } else if (shapeType == "Rectangle") {
        m_dragShapeCounter = m_rectCounter;
    }

    Console::log("Started dragging " + shapeType.toStdString() + " at (lon: " +
                 std::to_string(startGeoPos.x()) + ", lat: " + std::to_string(startGeoPos.y()) + ")");
    m_host->refreshView();
}

void ShapesFeature::updateDragShape(const QPointF& currentGeoPos)
{
    if (!m_isDraggingNewShape) return;
    m_dragCurrentGeoPos = currentGeoPos;
    m_host->refreshView();
}

void ShapesFeature::finalizeDragShape()
{
    if (!m_isDraggingNewShape || !m_host) return;

    if (!m_host->getGISlib()) {
        m_isDraggingNewShape = false;
        m_dragShapeType = "";
        return;
    }

    QPointF startCanvas = m_host->getGISlib()->geoToCanvas(
        m_dragStartGeoPos.y(),
        m_dragStartGeoPos.x());
    QPointF currentCanvas = m_host->getGISlib()->geoToCanvas(
        m_dragCurrentGeoPos.y(),
        m_dragCurrentGeoPos.x());

    qreal dragDistance = QVector2D(currentCanvas - startCanvas).length();

    if (dragDistance < 1e-3) {
        m_isDraggingNewShape = false;
        m_dragShapeType = "";
        m_host->refreshView();
        return;
    }

    if (m_dragShapeType == "Circle") {
        finalizeCircleDrag();
    } else if (m_dragShapeType == "Rectangle") {
        finalizeRectangleDrag();
    }

    m_isDraggingNewShape = false;
    m_dragShapeType = "";
    m_host->setCurrentMode(Translate);
    m_host->setCursor(Qt::ArrowCursor);
    m_host->refreshView();
}

void ShapesFeature::cancelDragShape()
{
    if (!m_isDraggingNewShape || !m_host) return;
    Console::log("Cancelled shape dragging");
    m_isDraggingNewShape = false;
    m_dragShapeType = "";
    m_host->refreshView();
}

void ShapesFeature::drawDragPreview(QPainter& painter, GISlib* gislib)
{
    if (!m_isDraggingNewShape || !gislib) return;

    painter.save();

    QPen previewPen(Qt::blue, 2, Qt::DashLine);
    painter.setPen(previewPen);
    painter.setBrush(QColor(0, 0, 255, 30));

    QPointF startCanvas = gislib->geoToCanvas(m_dragStartGeoPos.y(), m_dragStartGeoPos.x());
    QPointF currentCanvas = gislib->geoToCanvas(m_dragCurrentGeoPos.y(), m_dragCurrentGeoPos.x());

    if (m_dragShapeType == "Circle") {
        qreal radius = QVector2D(currentCanvas - startCanvas).length();

        if (radius > 0) {
            painter.drawEllipse(startCanvas, radius, radius);
            painter.setPen(QPen(Qt::blue, 1, Qt::DotLine));
            painter.drawLine(startCanvas, currentCanvas);
            painter.setBrush(Qt::blue);
            painter.drawEllipse(startCanvas, 3, 3);
        }
    } else if (m_dragShapeType == "Rectangle") {
        QRectF rect = QRectF(startCanvas, currentCanvas).normalized();

        if (rect.width() > 0 && rect.height() > 0) {
            painter.drawRect(rect);
            painter.setBrush(Qt::blue);
            painter.drawRect(QRectF(rect.topLeft().x() - 3, rect.topLeft().y() - 3, 6, 6));
            painter.drawRect(QRectF(rect.topRight().x() - 3, rect.topRight().y() - 3, 6, 6));
            painter.drawRect(QRectF(rect.bottomLeft().x() - 3, rect.bottomLeft().y() - 3, 6, 6));
            painter.drawRect(QRectF(rect.bottomRight().x() - 3, rect.bottomRight().y() - 3, 6, 6));
        }
    }

    painter.restore();
}

void ShapesFeature::finalizeCircleDrag()
{
    if (!m_host || !m_host->getGISlib()) return;

    GISlib* gislib = m_host->getGISlib();
    QPointF centerGeo = m_dragStartGeoPos;
    QPointF startCanvas = gislib->geoToCanvas(m_dragStartGeoPos.y(), m_dragStartGeoPos.x());
    QPointF currentCanvas = gislib->geoToCanvas(m_dragCurrentGeoPos.y(), m_dragCurrentGeoPos.x());
    qreal radiusPixels = QVector2D(currentCanvas - startCanvas).length();

    QPointF radiusPointCanvas = startCanvas + QPointF(radiusPixels, 0);
    QPointF radiusPointGeo = gislib->canvasToGeo(radiusPointCanvas);

    qreal dx = radiusPointGeo.x() - centerGeo.x();
    qreal dy = radiusPointGeo.y() - centerGeo.y();
    float radiusGeo = std::sqrt(dx * dx + dy * dy);

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

    m_host->getTempMeshes().push_back(entry);

    Console::log("Created dragged circle with radius " + std::to_string(radiusGeo) +
                 " at (lon: " + std::to_string(centerGeo.x()) +
                 ", lat: " + std::to_string(centerGeo.y()) + ")");

    m_circleCounter++;
    addShapeToActiveLayer(shapeId, "Circle");
}

void ShapesFeature::finalizeRectangleDrag()
{
    if (!m_host || !m_host->getGISlib()) return;

    qreal centerLon = (m_dragStartGeoPos.x() + m_dragCurrentGeoPos.x()) / 2.0;
    qreal centerLat = (m_dragStartGeoPos.y() + m_dragCurrentGeoPos.y()) / 2.0;
    QPointF centerGeo(centerLon, centerLat);

    qreal halfWidth = std::abs(m_dragCurrentGeoPos.x() - m_dragStartGeoPos.x()) / 2.0;
    qreal halfHeight = std::abs(m_dragCurrentGeoPos.y() - m_dragStartGeoPos.y()) / 2.0;

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

    m_host->getTempMeshes().push_back(entry);

    Console::log("Created dragged rectangle at (lon: " + std::to_string(centerGeo.x()) +
                 ", lat: " + std::to_string(centerGeo.y()) + ") with size " +
                 std::to_string(halfWidth * 2) + "x" + std::to_string(halfHeight * 2));

    m_rectCounter++;
    addShapeToActiveLayer(shapeId, "Rectangle");
}

QVector3D ShapesFeature::calculateCircleProperties(const QPointF& start, const QPointF& current)
{
    QPointF centerGeo = start;
    qreal dx = current.x() - start.x();
    qreal dy = current.y() - start.y();
    float radiusGeo = std::sqrt(dx * dx + dy * dy);
    return QVector3D(centerGeo.x(), centerGeo.y(), radiusGeo);
}

QRectF ShapesFeature::calculateRectangleProperties(const QPointF& start, const QPointF& current)
{
    qreal left = std::min(start.x(), current.x());
    qreal right = std::max(start.x(), current.x());
    qreal top = std::max(start.y(), current.y());
    qreal bottom = std::min(start.y(), current.y());
    return QRectF(QPointF(left, bottom), QPointF(right, top));
}

// ============================================================================
// Script Functions
// ============================================================================

void ShapesFeature::scriptStartLine()
{
    if (!m_host) return;

    m_host->setCurrentMode(DrawShape);
    m_host->setSelectedShape("Line");

    for (auto* v : m_tempLineVertices) delete v;
    m_tempLineVertices.clear();
    m_tempLineCanvasPoints.clear();
}

void ShapesFeature::scriptAddLinePoint(const QPointF& geoPos)
{
    drawLine(geoPos, false);
}

void ShapesFeature::scriptFinishLine()
{
    if (!m_host) return;

    if (m_tempLineVertices.size() < 2) return;

    QString lineName = QString("Line_%1").arg(m_vertexCounter++);

    MeshEntry entry;
    entry.name = lineName;
    entry.position = new QVector3D(0, 0, 0);
    entry.rotation = new QQuaternion();
    entry.size = new QVector3D(0, 0, 1);
    entry.velocity = new QVector3D(0, 0, 0);
    entry.trajectory = nullptr;
    entry.collider = nullptr;
    entry.bitmapPath = "";
    entry.text = "";

    entry.mesh = new Mesh();
    entry.mesh->color = new QColor(Qt::red);
    entry.mesh->lineWidth = 3;
    entry.mesh->closePath = false;

    for (Vector* v : m_tempLineVertices) {
        entry.mesh->polygen.push_back(new Vector(v->x, v->y, 0));
    }

    m_host->getTempMeshes().push_back(entry);
    addShapeToActiveLayer(lineName, "Line");

    m_host->setSelectedShape("");
    m_host->setCurrentMode(Translate);
    m_host->setShapeDrawingMode(false, "");
    m_host->setCursor(Qt::ArrowCursor);

    for (auto* v : m_tempLineVertices) delete v;
    m_tempLineVertices.clear();
    m_tempLineCanvasPoints.clear();
    m_host->refreshView();
}

// ============================================================================
// History Management
// ============================================================================

ShapeState ShapesFeature::captureState(MeshEntry* entry)
{
    ShapeState state;
    if (entry->position) state.position = *entry->position;
    if (entry->rotation) state.rotation = *entry->rotation;
    if (entry->size) state.size = *entry->size;

    if (entry->mesh && !entry->mesh->polygen.empty()) {
        for (Vector* v : entry->mesh->polygen) {
            state.vertices.push_back(new Vector(v->x, v->y, v->z));
        }
    }

    if (m_layerPanel) {
        state.layerName = m_layerPanel->getLayerForShape(entry->name);
    }

    return state;
}

void ShapesFeature::applyState(const ShapeState& state, MeshEntry* entry)
{
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

void ShapesFeature::saveShapeState(const QString& shapeId, MeshEntry* entry)
{
    if (!entry) return;

    ShapeState currentState = captureState(entry);
    if (!m_shapeHistory.contains(shapeId)) {
        m_shapeHistory[shapeId] = QStack<ShapeState>();
    }
    m_shapeHistory[shapeId].push(currentState);
    if (m_shapeHistory[shapeId].size() > MAX_HISTORY_DEPTH) {
        m_shapeHistory[shapeId].removeFirst();
    }
}

bool ShapesFeature::restorePreviousState(const QString& shapeId, MeshEntry* entry)
{
    if (!entry || !m_shapeHistory.contains(shapeId) || m_shapeHistory[shapeId].isEmpty()) {
        return false;
    }
    ShapeState previousState = m_shapeHistory[shapeId].pop();
    applyState(previousState, entry);
    if (m_host) m_host->refreshView();
    return true;
}

bool ShapesFeature::hasHistory(const QString& shapeId) const
{
    return m_shapeHistory.contains(shapeId) && !m_shapeHistory[shapeId].isEmpty();
}

void ShapesFeature::clearHistory(const QString& shapeId)
{
    if (m_shapeHistory.contains(shapeId)) {
        m_shapeHistory[shapeId].clear();
        m_shapeHistory.remove(shapeId);
    }
}

void ShapesFeature::clearAllHistory()
{
    for (auto& key : m_shapeHistory.keys()) {
        m_shapeHistory[key].clear();
    }
    m_shapeHistory.clear();
}

// ============================================================================
// History Preview
// ============================================================================

void ShapesFeature::showHistoryPreview(const QString& shapeId)
{
    if (!m_shapeHistory.contains(shapeId) || m_shapeHistory[shapeId].isEmpty()) {
        return;
    }
    m_previewState = m_shapeHistory[shapeId].top();
    m_previewShapeId = shapeId;
    m_showingPreview = true;
    if (m_host) m_host->refreshView();
}

void ShapesFeature::hideHistoryPreview()
{
    if (m_showingPreview) {
        m_showingPreview = false;
        m_previewShapeId.clear();
        m_previewState.clearVertices();
        if (m_host) m_host->refreshView();
    }
}

void ShapesFeature::drawHistoryPreview(QPainter& painter, GISlib* gislib)
{
    if (!m_showingPreview || m_previewShapeId.isEmpty() || !gislib || !m_host) {
        return;
    }

    MeshEntry* currentEntry = nullptr;
    for (auto& entry : m_host->getTempMeshes()) {
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
// Utility Functions
// ============================================================================

bool ShapesFeature::isPointNearLineSegment(const QPointF& p, const QPointF& v1,
                                           const QPointF& v2, qreal tolerance)
{
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

void ShapesFeature::moveShapeByScript(MeshEntry* entry, const QPointF& newGeoPos)
{
    if (!entry || !entry->position) return;
    saveShapeState(entry->name, entry);
    entry->position->setX(newGeoPos.x());
    entry->position->setY(newGeoPos.y());
    if (m_host) m_host->refreshView();
}

void ShapesFeature::updateCircleRadius(MeshEntry* entry, float radiusDeg)
{
    if (!entry || !entry->mesh) return;
    saveShapeState(entry->name, entry);
    entry->size->setX(radiusDeg);
    entry->size->setY(radiusDeg);

    // Rebuild circle polygon
    for (Vector* v : entry->mesh->polygen) delete v;
    entry->mesh->polygen.clear();
    const int SEG = 64;
    for (int i = 0; i < SEG; ++i) {
        float a = (2 * M_PI * i) / SEG;
        entry->mesh->polygen.push_back(
            new Vector(std::cos(a) * radiusDeg,
                       std::sin(a) * radiusDeg, 0));
    }
    if (m_host) m_host->refreshView();
}

void ShapesFeature::syncCountersFromLoadedShapes(const std::vector<MeshEntry>& loadedMeshes)
{
    QRegularExpression re("_(\\d+)$");

    for (const MeshEntry& entry : loadedMeshes) {
        QRegularExpressionMatch m = re.match(entry.name);
        if (!m.hasMatch()) continue;

        int idx = m.captured(1).toInt();

        if (entry.name.startsWith("TempCircle_")) {
            m_circleCounter = qMax(m_circleCounter, idx + 1);
        } else if (entry.name.startsWith("TempRectangle_")) {
            m_rectCounter = qMax(m_rectCounter, idx + 1);
        } else if (entry.name.startsWith("TempPolygon_")) {
            m_polygonCounter = qMax(m_polygonCounter, idx + 1);
        } else if (entry.name.startsWith("TempPolyline_")) {
            m_polylineCounter = qMax(m_polylineCounter, idx + 1);
        } else if (entry.name.startsWith("TempPoint_")) {
            m_pointCounter = qMax(m_pointCounter, idx + 1);
        }
    }
}

void ShapesFeature::resetCounters()
{
    m_circleCounter = 0;
    m_rectCounter = 0;
    m_polygonCounter = 0;
    m_polylineCounter = 0;
    m_pointCounter = 0;
    Console::log("ShapesFeature counters reset to 0");
}
