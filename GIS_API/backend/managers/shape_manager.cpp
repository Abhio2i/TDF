#include "shape_manager.h"
#include "backend/api/map_api.h"
#include "backend/shapes/shape_type_registry.h"
#include <QUuid>
#include <QDebug>
#include <cmath>
#include <QRectF>
#include <QPolygonF>
#include <QVector2D>
#include <QJsonObject>
#include <QJsonArray>

ShapeManager* ShapeManager::m_instance = nullptr;

ShapeManager* ShapeManager::instance() {
    if (!m_instance) {
        m_instance = new ShapeManager();
    }
    return m_instance;
}

ShapeManager::ShapeManager(QObject* parent)
    : QObject(parent)
    , m_counter(0) {
}

ShapeManager::~ShapeManager() {}
QString ShapeManager::generateId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString ShapeManager::addShape(const ShapeData& shape) {
    QString id = generateId();
    ShapeData newShape = shape;
    newShape.id = id;
    m_shapes[id] = newShape;
    m_shapeOrder.append(id);
    m_counter++;
    emit shapeAdded(id);
    return id;
}

bool ShapeManager::updateShape(const QString& id, const ShapeData& shape) {
    if (!m_shapes.contains(id)) return false;
    m_shapes[id] = shape;
    emit shapeUpdated(id);
    return true;
}

bool ShapeManager::deleteShape(const QString& id) {
    if (!m_shapes.contains(id)) return false;
    m_shapes.remove(id);
    m_shapeOrder.removeAll(id);
    emit shapeDeleted(id);
    return true;
}

bool ShapeManager::deleteAllShapes() {
    m_shapes.clear();
    m_shapeOrder.clear();
    emit allShapesDeleted();
    return true;
}

int ShapeManager::deleteShapesByType(const QString& type) {
    QVector<QString> toDelete;
    for (const QString& id : m_shapeOrder) {
        if (m_shapes[id].type == type) {
            toDelete.append(id);
        }
    }
    for (const QString& id : toDelete) {
        deleteShape(id);
    }
    return toDelete.size();
}

ShapeData ShapeManager::getShape(const QString& id) const {
    return m_shapes.value(id, ShapeData());
}

QVector<ShapeData> ShapeManager::getAllShapes() const {
    QVector<ShapeData> result;
    for (const QString& id : m_shapeOrder) {
        result.append(m_shapes[id]);
    }
    return result;
}

int ShapeManager::getShapeCount() const {
    return m_shapes.size();
}

QString ShapeManager::getShapeType(const QString& id) const {
    if (!m_shapes.contains(id)) return QString();
    return m_shapes[id].type;
}

QPointF ShapeManager::getShapePosition(const QString& id) const {
    if (!m_shapes.contains(id)) return QPointF();
    return QPointF(m_shapes[id].longitude, m_shapes[id].latitude);
}

// ============================================================
// MOVE SHAPE
// ============================================================

bool ShapeManager::moveShape(const QString& id, double newLon, double newLat) {
    if (!m_shapes.contains(id)) {
        return false;
    }
    ShapeData& shape = m_shapes[id];
    double deltaLon = newLon - shape.longitude;
    double deltaLat = newLat - shape.latitude;
    shape.longitude = newLon;
    shape.latitude = newLat;
    if (shape.type == "polygon" || shape.type == "line") {
        for (int i = 0; i < shape.points.size(); ++i) {
            shape.points[i].rx() += deltaLon;
            shape.points[i].ry() += deltaLat;
        }
    }
    emit shapeUpdated(id);
    return true;
}

static QJsonObject shapeDataToHitTestProps(const ShapeData& shape)
{
    QJsonObject props;
    props[QStringLiteral("type")]      = shape.type;
    props[QStringLiteral("longitude")] = shape.longitude;
    props[QStringLiteral("latitude")]  = shape.latitude;
    props[QStringLiteral("radius")]    = shape.radius;
    props[QStringLiteral("width")]     = shape.width;
    props[QStringLiteral("height")]    = shape.height;
    props[QStringLiteral("rotation")]  = shape.rotation;
    if (!shape.points.isEmpty()) {
        QJsonArray pts;
        for (const QPointF& p : shape.points) {
            QJsonArray pt;
            pt.append(p.x());
            pt.append(p.y());
            pts.append(pt);
        }
        props[QStringLiteral("points")] = pts;
    }
    return props;
}

// ============================================================
// FIND SHAPE AT POSITION
// ============================================================

QString ShapeManager::findShapeAt(const QPointF& geoPos) const {

    const double kDefaultTolerance = 0.08;
    ShapeTypeRegistry* registry = ShapeTypeRegistry::instance();

    for (int i = m_shapeOrder.size() - 1; i >= 0; --i) {
        const QString& id = m_shapeOrder[i];
        const ShapeData& shape = m_shapes[id];

        IShapeType* shapeType = registry->type(shape.type);
        if (shapeType) {
            const QJsonObject props = shapeDataToHitTestProps(shape);
            if (shapeType->hitTest(props, geoPos, kDefaultTolerance))
                return id;
            continue;
        }

        qWarning() << "ShapeManager::findShapeAt: no registered IShapeType for"
                   << shape.type << "- using generic fallback hit test";
        double dx = geoPos.x() - shape.longitude, dy = geoPos.y() - shape.latitude;
        if (std::sqrt(dx*dx + dy*dy) < kDefaultTolerance)
            return id;
    }
    return QString();
}

// ============================================================
//Point on line segment
// ============================================================

bool ShapeManager::pointOnSegment(const QPointF& p, const QPointF& p1, const QPointF& p2) const {
    double tolerance = 0.001;
    double cross = (p2.x() - p1.x()) * (p.y() - p1.y()) - (p2.y() - p1.y()) * (p.x() - p1.x());
    if (std::abs(cross) > tolerance) return false;
    double dot = (p.x() - p1.x()) * (p2.x() - p1.x()) + (p.y() - p1.y()) * (p2.y() - p1.y());
    if (dot < 0) return false;
    double lenSq = (p2.x() - p1.x()) * (p2.x() - p1.x()) + (p2.y() - p1.y()) * (p2.y() - p1.y());
    if (dot > lenSq) return false;
    return true;
}

// ============================================================
// Point near line segment
// ============================================================

bool ShapeManager::pointNearSegment(const QPointF& p, const QPointF& p1, const QPointF& p2, double tolerance) const {
    QPointF ab = p2 - p1;
    QPointF ap = p - p1;
    double lenSq = ab.x() * ab.x() + ab.y() * ab.y();
    if (lenSq < 1e-12) {
        double dx = p.x() - p1.x();
        double dy = p.y() - p1.y();
        return std::sqrt(dx*dx + dy*dy) < tolerance;
    }
    double t = (ap.x() * ab.x() + ap.y() * ab.y()) / lenSq;
    t = std::max(0.0, std::min(1.0, t));
    QPointF proj = p1 + ab * t;
    double dx = p.x() - proj.x();
    double dy = p.y() - proj.y();
    return std::sqrt(dx*dx + dy*dy) < tolerance;
}

QVector<QString> ShapeManager::findShapesInRect(const QRectF& rect) const {
    QVector<QString> result;
    for (const QString& id : m_shapeOrder) {
        const ShapeData& shape = m_shapes[id];
        QPointF pos(shape.longitude, shape.latitude);
        if (rect.contains(pos)) {
            result.append(id);
        }
    }
    return result;
}
