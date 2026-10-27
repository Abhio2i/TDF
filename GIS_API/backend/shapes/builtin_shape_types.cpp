#include "builtin_shape_types.h"
#include "shape_type_registry.h"
#include "frontend/widgets/map_widget.h"
#include "backend/api/shape_api.h"
#include "backend/api/map_api.h"


#include <QVector2D>
#include <QFontMetrics>
#include <QInputDialog>
#include <cmath>

// ============================================================
// Common helpers
// ============================================================

static QPointF geoFromEvent(MapWidget* map, QMouseEvent* e)
{
    return map->canvasToGeo(e->pos());
}

static QColor colorFromProps(const QJsonObject& props, const QColor& def = Qt::red)
{
    QColor c(props.value(QStringLiteral("color")).toString(def.name()));
    return c.isValid() ? c : def;
}

static int lineWidthFromProps(const QJsonObject& props, int def = 2)
{
    return props.value(QStringLiteral("lineWidth")).toInt(def);
}

static double rotationFromProps(const QJsonObject& props)
{
    return props.value(QStringLiteral("rotation")).toDouble(0.0);
}


static QPointF unrotateGeoPoint(const QPointF& geoPos, double centerLon, double centerLat, double rotationDeg)
{
    if (std::fabs(rotationDeg) < 0.01)
        return geoPos;
    const double theta = rotationDeg * M_PI / 180.0;
    const double dLon = geoPos.x() - centerLon;
    const double dLat = geoPos.y() - centerLat;
    const double c = std::cos(theta);
    const double s = std::sin(theta);
    const double rawDLon =  dLon * c + dLat * s;
    const double rawDLat = -dLon * s + dLat * c;
    return QPointF(centerLon + rawDLon, centerLat + rawDLat);
}

static QPointF rotateCanvasPoint(const QPointF& pt, const QPointF& center, double rotationDeg)
{
    if (std::fabs(rotationDeg) < 0.01)
        return pt;
    const double theta = rotationDeg * M_PI / 180.0;
    const double c = std::cos(theta);
    const double s = std::sin(theta);
    const double dx = pt.x() - center.x();
    const double dy = pt.y() - center.y();
    return QPointF(center.x() + dx * c - dy * s,
                   center.y() + dx * s + dy * c);
}
// ============================================================
// CIRCLE
// ============================================================

bool CircleShapeType::onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points)
{
    if (e->button() != Qt::LeftButton)
        return false;
    points.clear();
    points.append(geoFromEvent(map, e));
    return false;
}

bool CircleShapeType::onMouseMove(MapWidget* /*map*/, QMouseEvent* /*e*/, const QVector<QPointF>& /*points*/)
{
    return true;
}

bool CircleShapeType::onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points)
{
    if (e->button() != Qt::LeftButton || points.isEmpty())
        return false;
    points.append(geoFromEvent(map, e));
    return true;
}

QJsonObject CircleShapeType::finishDrawing(const QVector<QPointF>& points)
{
    if (points.size() < 2)
        return {};

    const QPointF center = points[0];
    const QPointF edge   = points[1];
    const double radius = std::sqrt(std::pow(edge.x() - center.x(), 2) +
                                    std::pow(edge.y() - center.y(), 2));
    if (radius <= 0.0)
        return {};

    QJsonObject obj;
    obj[QStringLiteral("type")]      = typeId();
    obj[QStringLiteral("longitude")] = center.x();
    obj[QStringLiteral("latitude")]  = center.y();
    obj[QStringLiteral("radius")]    = radius;
    obj[QStringLiteral("color")]     = QStringLiteral("#ff4444");
    obj[QStringLiteral("lineWidth")] = 2;
    obj[QStringLiteral("rotation")]  = 0.0;
    return obj;
}

void CircleShapeType::draw(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const double lon    = props[QStringLiteral("longitude")].toDouble();
    const double lat    = props[QStringLiteral("latitude")].toDouble();
    const double radius = props[QStringLiteral("radius")].toDouble();
    const QColor color  = colorFromProps(props);
    const int lw        = lineWidthFromProps(props);
    const double rot    = rotationFromProps(props);

    const QPointF center = map->geoToCanvas(lat, lon);
    const QPointF edge   = map->geoToCanvas(lat, lon + radius);
    const double rPx     = QVector2D(edge - center).length();

    painter.save();
    if (std::fabs(rot) > 0.01) {
        painter.translate(center);
        painter.rotate(rot);
        painter.translate(-center);
    }
    painter.setPen(QPen(color, lw));
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 40));
    painter.drawEllipse(center, rPx, rPx);
    painter.restore();
}

void CircleShapeType::drawPreview(QPainter& painter, const QVector<QPointF>& points, const MapWidget* map) const
{
    if (points.isEmpty())
        return;

    const QPointF center = map->geoToCanvas(points[0].y(), points[0].x());
    QPointF edge = center;
    if (points.size() >= 2)
        edge = map->geoToCanvas(points[1].y(), points[1].x());

    const double rPx = QVector2D(edge - center).length();
    painter.setPen(QPen(QColor(255, 200, 0), 2, Qt::DashLine));
    painter.setBrush(QColor(255, 200, 0, 40));
    painter.drawEllipse(center, rPx, rPx);
}

void CircleShapeType::drawEditHandles(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const double lon    = props[QStringLiteral("longitude")].toDouble();
    const double lat    = props[QStringLiteral("latitude")].toDouble();
    const double radius = props[QStringLiteral("radius")].toDouble();
    const QPointF handle = map->geoToCanvas(lat, lon + radius);

    painter.setPen(QPen(QColor(100, 200, 255, 80), 3));
    painter.setBrush(QColor(100, 200, 255, 30));
    painter.drawEllipse(handle, 14, 14);
    painter.setPen(QPen(Qt::blue, 2));
    painter.setBrush(QColor(100, 200, 255, 200));
    painter.drawEllipse(handle, 8, 8);
    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(Qt::white);
    painter.drawEllipse(handle, 3, 3);
}

bool CircleShapeType::hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const
{
    const double lon    = props[QStringLiteral("longitude")].toDouble();
    const double lat    = props[QStringLiteral("latitude")].toDouble();
    const double radius = props[QStringLiteral("radius")].toDouble();
    const double dx = geoPos.x() - lon;
    const double dy = geoPos.y() - lat;
    return std::sqrt(dx * dx + dy * dy) <= (radius + tolerance);
}

int CircleShapeType::findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const
{
    const double lon    = props[QStringLiteral("longitude")].toDouble();
    const double lat    = props[QStringLiteral("latitude")].toDouble();
    const double radius = props[QStringLiteral("radius")].toDouble();
    const QPointF handle = map->geoToCanvas(lat, lon + radius);
    const double dx = canvasPos.x() - handle.x();
    const double dy = canvasPos.y() - handle.y();
    if (std::sqrt(dx * dx + dy * dy) <= 24.0)
        return 0;
    return -1;
}

void CircleShapeType::updateEditPoint(QJsonObject& props, int /*index*/, const QPointF& newGeoPos)
{
    const double lon = props[QStringLiteral("longitude")].toDouble();
    const double lat = props[QStringLiteral("latitude")].toDouble();
    const double dx = newGeoPos.x() - lon;
    const double dy = newGeoPos.y() - lat;
    props[QStringLiteral("radius")] = std::sqrt(dx * dx + dy * dy);
}

// ============================================================
// RECTANGLE
// ============================================================

bool RectangleShapeType::onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points)
{
    if (e->button() != Qt::LeftButton)
        return false;
    points.clear();
    points.append(geoFromEvent(map, e));
    return false;
}

bool RectangleShapeType::onMouseMove(MapWidget* /*map*/, QMouseEvent* /*e*/, const QVector<QPointF>& /*points*/)
{
    return true;
}

bool RectangleShapeType::onMouseRelease(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points)
{
    if (e->button() != Qt::LeftButton || points.isEmpty())
        return false;
    points.append(geoFromEvent(map, e));
    return true;
}

QJsonObject RectangleShapeType::finishDrawing(const QVector<QPointF>& points)
{
    if (points.size() < 2)
        return {};

    const double lon1 = points[0].x();
    const double lat1 = points[0].y();
    const double lon2 = points[1].x();
    const double lat2 = points[1].y();
    const double w = std::abs(lon2 - lon1);
    const double h = std::abs(lat2 - lat1);
    if (w <= 0.0 || h <= 0.0)
        return {};

    QJsonObject obj;
    obj[QStringLiteral("type")]      = typeId();
    obj[QStringLiteral("longitude")] = (lon1 + lon2) / 2.0;
    obj[QStringLiteral("latitude")]  = (lat1 + lat2) / 2.0;
    obj[QStringLiteral("width")]     = w;
    obj[QStringLiteral("height")]    = h;
    obj[QStringLiteral("color")]     = QStringLiteral("#ff4444");
    obj[QStringLiteral("lineWidth")] = 2;
    obj[QStringLiteral("rotation")]  = 0.0;
    return obj;
}

void RectangleShapeType::draw(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const double lon = props[QStringLiteral("longitude")].toDouble();
    const double lat = props[QStringLiteral("latitude")].toDouble();
    const double w   = props[QStringLiteral("width")].toDouble();
    const double h   = props[QStringLiteral("height")].toDouble();
    const QColor color = colorFromProps(props);
    const int lw = lineWidthFromProps(props);
    const double rot = rotationFromProps(props);

    const QPointF tl = map->geoToCanvas(lat + h / 2.0, lon - w / 2.0);
    const QPointF br = map->geoToCanvas(lat - h / 2.0, lon + w / 2.0);
    QRectF rect(tl, br);
    rect = rect.normalized();

    painter.save();
    if (std::fabs(rot) > 0.01) {
        const QPointF center = map->geoToCanvas(lat, lon);
        painter.translate(center);
        painter.rotate(rot);
        painter.translate(-center);
    }
    painter.setPen(QPen(color, lw));
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 40));
    painter.drawRect(rect);
    painter.restore();
}

void RectangleShapeType::drawPreview(QPainter& painter, const QVector<QPointF>& points, const MapWidget* map) const
{
    if (points.size() < 2)
        return;
    const QPointF a = map->geoToCanvas(points[0].y(), points[0].x());
    const QPointF b = map->geoToCanvas(points[1].y(), points[1].x());
    painter.setPen(QPen(QColor(255, 200, 0), 2, Qt::DashLine));
    painter.setBrush(QColor(255, 200, 0, 40));
    painter.drawRect(QRectF(a, b).normalized());
}

// ============================================================
// RECTANGLE – drawEditHandles
// ============================================================

void RectangleShapeType::drawEditHandles(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const double lon = props[QStringLiteral("longitude")].toDouble();
    const double lat = props[QStringLiteral("latitude")].toDouble();
    const double w   = props[QStringLiteral("width")].toDouble();
    const double h   = props[QStringLiteral("height")].toDouble();
    const double rot = rotationFromProps(props);
    const QPointF center = map->geoToCanvas(lat, lon);
    QPointF handle = map->geoToCanvas(lat - h / 2.0, lon + w / 2.0);
    handle = rotateCanvasPoint(handle, center, rot);

    painter.setPen(QPen(QColor(100, 200, 255, 80), 3));
    painter.setBrush(QColor(100, 200, 255, 30));
    painter.drawEllipse(handle, 14, 14);
    painter.setPen(QPen(Qt::blue, 2));
    painter.setBrush(QColor(100, 200, 255, 200));
    painter.drawEllipse(handle, 8, 8);
    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(Qt::white);
    painter.drawEllipse(handle, 3, 3);
}

bool RectangleShapeType::hitTest(const QJsonObject& props, const QPointF& geoPos, double ) const
{
    const double lon = props[QStringLiteral("longitude")].toDouble();
    const double lat = props[QStringLiteral("latitude")].toDouble();
    const double halfW = props[QStringLiteral("width")].toDouble() / 2.0;
    const double halfH = props[QStringLiteral("height")].toDouble() / 2.0;
    const double rot = rotationFromProps(props);
    const QPointF testPt = unrotateGeoPoint(geoPos, lon, lat, rot);

    return (testPt.x() >= lon - halfW && testPt.x() <= lon + halfW &&
            testPt.y() >= lat - halfH && testPt.y() <= lat + halfH);
}

// ============================================================
// POLYGON
// ============================================================

bool PolygonShapeType::onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points)
{
    if (e->button() == Qt::LeftButton) {
        points.append(geoFromEvent(map, e));
        return false;
    }
    if (e->button() == Qt::RightButton && points.size() >= 3)
        return true;
    return false;
}

bool PolygonShapeType::onMouseMove(MapWidget* , QMouseEvent* , const QVector<QPointF>&)
{
    return true;
}

bool PolygonShapeType::onMouseRelease(MapWidget* , QMouseEvent* , QVector<QPointF>&)
{
    return false;
}

QJsonObject PolygonShapeType::finishDrawing(const QVector<QPointF>& points)
{
    if (points.size() < 3)
        return {};

    QJsonObject obj;
    obj[QStringLiteral("type")] = typeId();

    QJsonArray pts;
    double sumLon = 0.0, sumLat = 0.0;
    for (const QPointF& p : points) {
        QJsonArray pt;
        pt.append(p.x());
        pt.append(p.y());
        pts.append(pt);
        sumLon += p.x();
        sumLat += p.y();
    }
    obj[QStringLiteral("points")]    = pts;
    obj[QStringLiteral("longitude")] = sumLon / points.size();
    obj[QStringLiteral("latitude")]  = sumLat / points.size();
    obj[QStringLiteral("color")]     = QStringLiteral("#ff4444");
    obj[QStringLiteral("lineWidth")] = 2;
    obj[QStringLiteral("rotation")]  = 0.0;
    return obj;
}

void PolygonShapeType::draw(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const QJsonArray pts = props[QStringLiteral("points")].toArray();
    if (pts.size() < 3)
        return;

    const QColor color = colorFromProps(props);
    const int lw = lineWidthFromProps(props);
    const double rot = rotationFromProps(props);
    const QPointF center = map->geoToCanvas(props[QStringLiteral("latitude")].toDouble(),
                                            props[QStringLiteral("longitude")].toDouble());

    QPolygonF poly;
    for (const QJsonValue& v : pts) {
        const QJsonArray p = v.toArray();
        if (p.size() >= 2)
            poly << map->geoToCanvas(p[1].toDouble(), p[0].toDouble());
    }

    painter.save();
    if (std::fabs(rot) > 0.01) {
        painter.translate(center);
        painter.rotate(rot);
        painter.translate(-center);
    }
    painter.setPen(QPen(color, lw));
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 40));
    painter.drawPolygon(poly);
    painter.restore();
}

void PolygonShapeType::drawPreview(QPainter& painter, const QVector<QPointF>& points, const MapWidget* map) const
{
    if (points.size() < 2)
        return;

    QPolygonF poly;
    for (const QPointF& g : points)
        poly << map->geoToCanvas(g.y(), g.x());

    painter.setPen(QPen(QColor(255, 200, 0), 2, Qt::DashLine));
    painter.setBrush(QColor(255, 200, 0, 40));
    if (points.size() >= 3)
        painter.drawPolygon(poly);
    else
        painter.drawPolyline(poly);
}

void PolygonShapeType::drawEditHandles(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const QJsonArray pts = props[QStringLiteral("points")].toArray();
    if (pts.isEmpty())
        return;

    const double cLon = props[QStringLiteral("longitude")].toDouble();
    const double cLat = props[QStringLiteral("latitude")].toDouble();
    const double rot  = rotationFromProps(props);
    const QPointF center = map->geoToCanvas(cLat, cLon);

    for (const QJsonValue& v : pts) {
        const QJsonArray p = v.toArray();
        if (p.size() < 2)
            continue;
        QPointF c = map->geoToCanvas(p[1].toDouble(), p[0].toDouble());
        c = rotateCanvasPoint(c, center, rot);

        painter.setPen(QPen(QColor(100, 200, 255, 80), 3));
        painter.setBrush(QColor(100, 200, 255, 30));
        painter.drawEllipse(c, 14, 14);
        painter.setPen(QPen(Qt::blue, 2));
        painter.setBrush(QColor(100, 200, 255, 200));
        painter.drawEllipse(c, 8, 8);
        painter.setPen(QPen(Qt::white, 1));
        painter.setBrush(Qt::white);
        painter.drawEllipse(c, 3, 3);
    }
}

bool PolygonShapeType::hitTest(const QJsonObject& props, const QPointF& geoPos, double) const
{
    const QJsonArray pts = props[QStringLiteral("points")].toArray();
    if (pts.size() < 3) return false;

    MapApi* map = MapApi::instance();
    if (!map) return false;

    const double cLon = props[QStringLiteral("longitude")].toDouble();
    const double cLat = props[QStringLiteral("latitude")].toDouble();
    const double rot  = rotationFromProps(props);

    const QPointF center = map->geoToCanvas(cLat, cLon);
    const QPointF clickC = map->geoToCanvas(geoPos.y(), geoPos.x());
    const QPointF testC  = rotateCanvasPoint(clickC, center, -rot);

    QPolygonF poly;
    for (const QJsonValue& v : pts) {
        const QJsonArray p = v.toArray();
        if (p.size() >= 2)
            poly << map->geoToCanvas(p[1].toDouble(), p[0].toDouble());
    }
    if (poly.size() < 3) return false;
    return poly.containsPoint(testC, Qt::OddEvenFill);
}
int PolygonShapeType::findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const
{
    const QJsonArray pts = props[QStringLiteral("points")].toArray();
    double bestDist = 1e9;
    int bestIdx = -1;

    const double cLon = props[QStringLiteral("longitude")].toDouble();
    const double cLat = props[QStringLiteral("latitude")].toDouble();
    const double rot  = rotationFromProps(props);
    const QPointF center = map->geoToCanvas(cLat, cLon);

    for (int i = 0; i < pts.size(); ++i) {
        const QJsonArray p = pts[i].toArray();
        if (p.size() < 2)
            continue;
        QPointF c = map->geoToCanvas(p[1].toDouble(), p[0].toDouble());
        c = rotateCanvasPoint(c, center, rot);
        const double d = QVector2D(c - canvasPos).length();
        if (d <= 24.0 && d < bestDist) {
            bestDist = d;
            bestIdx = i;
        }
    }
    return bestIdx;
}
void PolygonShapeType::updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos)
{
    QJsonArray pts = props[QStringLiteral("points")].toArray();
    if (index < 0 || index >= pts.size()) return;

    MapApi* map = MapApi::instance();
    if (!map) return;

    const double cLon = props[QStringLiteral("longitude")].toDouble();
    const double cLat = props[QStringLiteral("latitude")].toDouble();
    const double rot  = rotationFromProps(props);

    const QPointF center    = map->geoToCanvas(cLat, cLon);
    const QPointF mouseC    = map->geoToCanvas(newGeoPos.y(), newGeoPos.x());
    const QPointF rawCanvas = rotateCanvasPoint(mouseC, center, -rot);
    const QPointF rawGeo    = map->canvasToGeo(rawCanvas);

    QJsonArray np;
    np.append(rawGeo.x());
    np.append(rawGeo.y());
    pts[index] = np;
    props[QStringLiteral("points")] = pts;
}

// ============================================================
// LINE
// ============================================================

bool LineShapeType::onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points)
{
    if (e->button() == Qt::LeftButton) {
        points.append(geoFromEvent(map, e));
        return false;
    }
    if (e->button() == Qt::RightButton && points.size() >= 2)
        return true;
    return false;
}

bool LineShapeType::onMouseMove(MapWidget*, QMouseEvent*, const QVector<QPointF>&)
{
    return true;
}

bool LineShapeType::onMouseRelease(MapWidget*, QMouseEvent*, QVector<QPointF>&)
{
    return false;
}

QJsonObject LineShapeType::finishDrawing(const QVector<QPointF>& points)
{
    if (points.size() < 2)
        return {};

    QJsonObject obj;
    obj[QStringLiteral("type")] = typeId();

    QJsonArray pts;
    double sumLon = 0.0, sumLat = 0.0;
    for (const QPointF& p : points) {
        QJsonArray pt;
        pt.append(p.x());
        pt.append(p.y());
        pts.append(pt);
        sumLon += p.x();
        sumLat += p.y();
    }
    obj[QStringLiteral("points")]    = pts;
    obj[QStringLiteral("longitude")] = sumLon / points.size();
    obj[QStringLiteral("latitude")]  = sumLat / points.size();
    obj[QStringLiteral("color")]     = QStringLiteral("#ff4444");
    obj[QStringLiteral("lineWidth")] = 2;
    obj[QStringLiteral("rotation")]  = 0.0;
    return obj;
}

void LineShapeType::draw(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const QJsonArray pts = props[QStringLiteral("points")].toArray();
    if (pts.size() < 2)
        return;

    const QColor color = colorFromProps(props);
    const int lw = lineWidthFromProps(props);
    const double rot = rotationFromProps(props);
    const QPointF center = map->geoToCanvas(props[QStringLiteral("latitude")].toDouble(),
                                            props[QStringLiteral("longitude")].toDouble());

    QPolygonF poly;
    for (const QJsonValue& v : pts) {
        const QJsonArray p = v.toArray();
        if (p.size() >= 2)
            poly << map->geoToCanvas(p[1].toDouble(), p[0].toDouble());
    }

    painter.save();
    if (std::fabs(rot) > 0.01) {
        painter.translate(center);
        painter.rotate(rot);
        painter.translate(-center);
    }
    painter.setPen(QPen(color, lw));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(poly);
    painter.restore();
}

void LineShapeType::drawPreview(QPainter& painter, const QVector<QPointF>& points, const MapWidget* map) const
{
    if (points.size() < 2)
        return;

    QPolygonF poly;
    for (const QPointF& g : points)
        poly << map->geoToCanvas(g.y(), g.x());

    painter.setPen(QPen(QColor(255, 200, 0), 2, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(poly);
}

void LineShapeType::drawEditHandles(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const QJsonArray pts = props[QStringLiteral("points")].toArray();
    if (pts.isEmpty())
        return;

    const double cLon = props[QStringLiteral("longitude")].toDouble();
    const double cLat = props[QStringLiteral("latitude")].toDouble();
    const double rot  = rotationFromProps(props);
    const QPointF center = map->geoToCanvas(cLat, cLon);

    for (const QJsonValue& v : pts) {
        const QJsonArray p = v.toArray();
        if (p.size() < 2)
            continue;
        QPointF c = map->geoToCanvas(p[1].toDouble(), p[0].toDouble());
        c = rotateCanvasPoint(c, center, rot);

        painter.setPen(QPen(QColor(100, 200, 255, 80), 3));
        painter.setBrush(QColor(100, 200, 255, 30));
        painter.drawEllipse(c, 14, 14);
        painter.setPen(QPen(Qt::blue, 2));
        painter.setBrush(QColor(100, 200, 255, 200));
        painter.drawEllipse(c, 8, 8);
        painter.setPen(QPen(Qt::white, 1));
        painter.setBrush(Qt::white);
        painter.drawEllipse(c, 3, 3);
    }
}

bool LineShapeType::hitTest(const QJsonObject& props, const QPointF& geoPos, double /*tolerance*/) const
{
    const QJsonArray pts = props[QStringLiteral("points")].toArray();
    if (pts.size() < 2) return false;

    MapApi* map = MapApi::instance();
    if (!map) return false;

    const double cLon = props[QStringLiteral("longitude")].toDouble();
    const double cLat = props[QStringLiteral("latitude")].toDouble();
    const double rot  = rotationFromProps(props);

    const QPointF center = map->geoToCanvas(cLat, cLon);
    const QPointF clickC = map->geoToCanvas(geoPos.y(), geoPos.x());
    const QPointF testC  = rotateCanvasPoint(clickC, center, -rot);
    const double tolPx = 12.0;

    for (int i = 0; i < pts.size() - 1; ++i) {
        const QJsonArray a = pts[i].toArray();
        const QJsonArray b = pts[i + 1].toArray();
        if (a.size() < 2 || b.size() < 2) continue;

        const QPointF p1 = map->geoToCanvas(a[1].toDouble(), a[0].toDouble());
        const QPointF p2 = map->geoToCanvas(b[1].toDouble(), b[0].toDouble());
        const QPointF ab = p2 - p1;
        const QPointF ap = testC - p1;
        const double lenSq = ab.x()*ab.x() + ab.y()*ab.y();
        if (lenSq < 1e-6) continue;

        double t = (ap.x()*ab.x() + ap.y()*ab.y()) / lenSq;
        t = qBound(0.0, 1.0, t);
        const QPointF proj = p1 + ab * t;
        if (QVector2D(testC - proj).length() < tolPx)
            return true;
    }
    return false;
}
int LineShapeType::findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const
{
    return PolygonShapeType().findNearestEditPoint(props, canvasPos, map);
}

void LineShapeType::updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos)
{
    PolygonShapeType().updateEditPoint(props, index, newGeoPos);
}


bool PointShapeType::onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points)
{
    if (e->button() != Qt::LeftButton)
        return false;
    points.clear();
    points.append(geoFromEvent(map, e));
    return true;
}

bool PointShapeType::onMouseMove(MapWidget*, QMouseEvent*, const QVector<QPointF>&)
{
    return false;
}

bool PointShapeType::onMouseRelease(MapWidget*, QMouseEvent*, QVector<QPointF>&)
{
    return false;
}

QJsonObject PointShapeType::finishDrawing(const QVector<QPointF>& points)
{
    if (points.isEmpty())
        return {};

    QJsonObject obj;
    obj[QStringLiteral("type")]      = typeId();
    obj[QStringLiteral("longitude")] = points[0].x();
    obj[QStringLiteral("latitude")]  = points[0].y();
    obj[QStringLiteral("color")]     = QStringLiteral("#ff4444");
    obj[QStringLiteral("lineWidth")] = 2;
    return obj;
}

void PointShapeType::draw(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const QPointF pos = map->geoToCanvas(props[QStringLiteral("latitude")].toDouble(),
                                         props[QStringLiteral("longitude")].toDouble());
    const QColor color = colorFromProps(props);
    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(color);
    painter.drawEllipse(pos, 6, 6);
}

void PointShapeType::drawPreview(QPainter& painter, const QVector<QPointF>& points, const MapWidget* map) const
{
    if (points.isEmpty())
        return;
    const QPointF pos = map->geoToCanvas(points[0].y(), points[0].x());
    painter.setPen(QPen(Qt::yellow, 1));
    painter.setBrush(QColor(255, 200, 0));
    painter.drawEllipse(pos, 6, 6);
}

bool PointShapeType::hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const
{
    const double dx = geoPos.x() - props[QStringLiteral("longitude")].toDouble();
    const double dy = geoPos.y() - props[QStringLiteral("latitude")].toDouble();
    return std::sqrt(dx * dx + dy * dy) < tolerance;
}

bool TextShapeType::onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points)
{
    if (e->button() != Qt::LeftButton)
        return false;
    points.clear();
    points.append(geoFromEvent(map, e));
    return true;
}

bool TextShapeType::onMouseMove(MapWidget*, QMouseEvent*, const QVector<QPointF>&)
{
    return false;
}

bool TextShapeType::onMouseRelease(MapWidget*, QMouseEvent*, QVector<QPointF>&)
{
    return false;
}

QJsonObject TextShapeType::finishDrawing(const QVector<QPointF>& points)
{
    if (points.isEmpty())
        return {};
    QJsonObject obj;
    obj[QStringLiteral("type")]      = typeId();
    obj[QStringLiteral("longitude")] = points[0].x();
    obj[QStringLiteral("latitude")]  = points[0].y();
    obj[QStringLiteral("text")]      = QStringLiteral("Text");
    obj[QStringLiteral("fontSize")]  = 14;
    obj[QStringLiteral("color")]     = QStringLiteral("#000000");
    obj[QStringLiteral("rotation")]  = 0.0;
    return obj;
}

void TextShapeType::draw(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const QString text = props[QStringLiteral("text")].toString();
    if (text.isEmpty())
        return;

    int fontSize = props[QStringLiteral("fontSize")].toInt(14);
    if (fontSize < 8)
        fontSize = 14;

    const QColor color = colorFromProps(props, Qt::black);
    const double rot = rotationFromProps(props);
    const QPointF pos = map->geoToCanvas(props[QStringLiteral("latitude")].toDouble(),
                                         props[QStringLiteral("longitude")].toDouble());

    painter.save();
    painter.setPen(color);
    painter.setFont(QFont(QStringLiteral("Arial"), fontSize, QFont::Bold));
    if (std::fabs(rot) > 0.01) {
        painter.translate(pos);
        painter.rotate(rot);
        painter.translate(-pos);
    }
    painter.drawText(pos, text);
    painter.restore();
}

void TextShapeType::drawPreview(QPainter& painter, const QVector<QPointF>& points, const MapWidget* map) const
{
    if (points.isEmpty())
        return;
    const QPointF pos = map->geoToCanvas(points[0].y(), points[0].x());
    painter.setPen(Qt::yellow);
    painter.setFont(QFont(QStringLiteral("Arial"), 14, QFont::Bold));
    painter.drawText(pos, QStringLiteral("Text"));
}

bool TextShapeType::hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const
{
    const double dx = geoPos.x() - props[QStringLiteral("longitude")].toDouble();
    const double dy = geoPos.y() - props[QStringLiteral("latitude")].toDouble();
    return std::sqrt(dx * dx + dy * dy) < (tolerance * 1.5);
}


bool BitmapShapeType::onMousePress(MapWidget* map, QMouseEvent* e, QVector<QPointF>& points)
{
    if (e->button() != Qt::LeftButton)
        return false;
    points.clear();
    points.append(geoFromEvent(map, e));
    return true;
}

bool BitmapShapeType::onMouseMove(MapWidget*, QMouseEvent*, const QVector<QPointF>&)
{
    return false;
}

bool BitmapShapeType::onMouseRelease(MapWidget*, QMouseEvent*, QVector<QPointF>&)
{
    return false;
}

QJsonObject BitmapShapeType::finishDrawing(const QVector<QPointF>& points)
{
    if (points.isEmpty())
        return {};

    QJsonObject obj;
    obj[QStringLiteral("type")]      = typeId();
    obj[QStringLiteral("longitude")] = points[0].x();
    obj[QStringLiteral("latitude")]  = points[0].y();
    obj[QStringLiteral("width")]     = 28;
    obj[QStringLiteral("height")]    = 28;
    obj[QStringLiteral("imagePath")] = QString();
    obj[QStringLiteral("rotation")]  = 0.0;
    return obj;
}

void BitmapShapeType::draw(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const QString path = props[QStringLiteral("imagePath")].toString();
    if (path.isEmpty()) return;
    QPixmap pix(path);
    if (pix.isNull()) return;

    const double lon = props[QStringLiteral("longitude")].toDouble();
    const double lat = props[QStringLiteral("latitude")].toDouble();
    const double w   = props[QStringLiteral("width")].toDouble();
    const double h   = props.contains(QStringLiteral("height"))
                         ? props[QStringLiteral("height")].toDouble()
                         : w;
    const double rot = rotationFromProps(props);

    const QPointF tl = map->geoToCanvas(lat + h / 2.0, lon - w / 2.0);
    const QPointF br = map->geoToCanvas(lat - h / 2.0, lon + w / 2.0);
    QRectF rect = QRectF(tl, br).normalized();

    painter.save();
    if (std::fabs(rot) > 0.01) {
        const QPointF center = map->geoToCanvas(lat, lon);
        painter.translate(center);
        painter.rotate(rot);
        painter.translate(-center);
    }
    painter.drawPixmap(rect, pix, pix.rect());
    painter.restore();
}

void BitmapShapeType::drawEditHandles(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    RectangleShapeType().drawEditHandles(painter, props, map);
}

bool BitmapShapeType::hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const
{
    return RectangleShapeType().hitTest(props, geoPos, tolerance);
}

int BitmapShapeType::findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const
{
    return RectangleShapeType().findNearestEditPoint(props, canvasPos, map);
}

void BitmapShapeType::updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos)
{
    RectangleShapeType().updateEditPoint(props, index, newGeoPos);
}

void registerBuiltInShapeTypes()
{
    auto* reg = ShapeTypeRegistry::instance();
    reg->registerType(new CircleShapeType());
    reg->registerType(new RectangleShapeType());
    reg->registerType(new PolygonShapeType());
    reg->registerType(new LineShapeType());
    reg->registerType(new PointShapeType());
    reg->registerType(new TextShapeType());
    reg->registerType(new BitmapShapeType());
     reg->registerType(new RasterImageShapeType());
}
int RectangleShapeType::findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const
{
    const double lon = props[QStringLiteral("longitude")].toDouble();
    const double lat = props[QStringLiteral("latitude")].toDouble();
    const double w   = props[QStringLiteral("width")].toDouble();
    const double h   = props[QStringLiteral("height")].toDouble();
    const double rot = rotationFromProps(props);

    const QPointF center = map->geoToCanvas(lat, lon);
    QPointF handle = map->geoToCanvas(lat - h / 2.0, lon + w / 2.0);
    handle = rotateCanvasPoint(handle, center, rot);

    const double dx = canvasPos.x() - handle.x();
    const double dy = canvasPos.y() - handle.y();
    if (std::sqrt(dx * dx + dy * dy) <= 24.0)
        return 0;
    return -1;
}

void RectangleShapeType::updateEditPoint(QJsonObject& props, int, const QPointF& newGeoPos)
{
    MapApi* mapApi = MapApi::instance();
    if (!mapApi)
        return;

    const double lon = props[QStringLiteral("longitude")].toDouble();
    const double lat = props[QStringLiteral("latitude")].toDouble();
    const double rot = rotationFromProps(props);

    const QPointF center    = mapApi->geoToCanvas(lat, lon);
    const QPointF mouseC    = mapApi->geoToCanvas(newGeoPos.y(), newGeoPos.x());
    const QPointF rawCanvas = rotateCanvasPoint(mouseC, center, -rot);
    const QPointF rawGeo    = mapApi->canvasToGeo(rawCanvas);

    const double newW = 2.0 * std::abs(rawGeo.x() - lon);
    const double newH = 2.0 * std::abs(rawGeo.y() - lat);

    if (newW > 1e-8)
        props[QStringLiteral("width")] = newW;
    if (newH > 1e-8)
        props[QStringLiteral("height")] = newH;
}

bool RasterImageShapeType::onMousePress(MapWidget*, QMouseEvent*, QVector<QPointF>&) { return false; }
bool RasterImageShapeType::onMouseMove(MapWidget*, QMouseEvent*, const QVector<QPointF>&) { return false; }
bool RasterImageShapeType::onMouseRelease(MapWidget*, QMouseEvent*, QVector<QPointF>&) { return false; }
QJsonObject RasterImageShapeType::finishDrawing(const QVector<QPointF>&) { return {}; }

void RasterImageShapeType::draw(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    const QString path = props[QStringLiteral("imagePath")].toString();
    if (path.isEmpty()) return;
    QPixmap pix(path);
    if (pix.isNull()) return;

    const double lon = props[QStringLiteral("longitude")].toDouble();
    const double lat = props[QStringLiteral("latitude")].toDouble();
    const double w   = props[QStringLiteral("width")].toDouble();
    const double h   = props[QStringLiteral("height")].toDouble();
    const double rot = rotationFromProps(props);

    const QPointF tl = map->geoToCanvas(lat + h / 2.0, lon - w / 2.0);
    const QPointF br = map->geoToCanvas(lat - h / 2.0, lon + w / 2.0);
    QRectF rect = QRectF(tl, br).normalized();

    painter.save();
    if (std::fabs(rot) > 0.01) {
        const QPointF center = map->geoToCanvas(lat, lon);
        painter.translate(center);
        painter.rotate(rot);
        painter.translate(-center);
    }
    painter.drawPixmap(rect, pix, pix.rect());
    painter.restore();
}

void RasterImageShapeType::drawPreview(QPainter&, const QVector<QPointF>&, const MapWidget*) const {}

void RasterImageShapeType::drawEditHandles(QPainter& painter, const QJsonObject& props, const MapWidget* map) const
{
    RectangleShapeType().drawEditHandles(painter, props, map);
}
void BitmapShapeType::drawPreview(QPainter& painter, const QVector<QPointF>& points, const MapWidget* map) const
{
    if (points.isEmpty())
        return;
    const QPointF pos = map->geoToCanvas(points[0].y(), points[0].x());
    painter.setPen(QPen(Qt::yellow, 2));
    painter.setBrush(QColor(255, 200, 0, 80));
    painter.drawRect(QRectF(pos.x() - 14, pos.y() - 14, 28, 28));
}
bool RasterImageShapeType::hitTest(const QJsonObject& props, const QPointF& geoPos, double tolerance) const
{
    return RectangleShapeType().hitTest(props, geoPos, tolerance);
}

int RasterImageShapeType::findNearestEditPoint(const QJsonObject& props, const QPointF& canvasPos, const MapWidget* map) const
{
    return RectangleShapeType().findNearestEditPoint(props, canvasPos, map);
}

void RasterImageShapeType::updateEditPoint(QJsonObject& props, int index, const QPointF& newGeoPos)
{
    RectangleShapeType().updateEditPoint(props, index, newGeoPos);
}
