#include "ishape_type.h"
#include "frontend/widgets/map_widget.h"

#include <QVector2D>
#include <QJsonArray>
#include <QPolygonF>
#include <cmath>

void IShapeType::drawSelection(QPainter& painter,
                               const QJsonObject& props,
                               const MapWidget* map) const
{
    if (!map)
        return;
    const QString type = props.value(QStringLiteral("type")).toString();
    const double lon   = props.value(QStringLiteral("longitude")).toDouble();
    const double lat   = props.value(QStringLiteral("latitude")).toDouble();
    const double rot   = props.value(QStringLiteral("rotation")).toDouble(0.0);
    QPointF center     = map->geoToCanvas(lat, lon);
    painter.save();
    painter.setPen(QPen(QColor(255, 220, 0), 2.5, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    if (std::fabs(rot) > 0.01) {
        painter.translate(center);
        painter.rotate(rot);
        painter.translate(-center);
    }
    // ---------- CIRCLE ----------
    if (type == QLatin1String("circle") || props.contains(QStringLiteral("radius"))) {
        double radius = props.value(QStringLiteral("radius")).toDouble();
        if (radius > 0.0) {
            QPointF edge = map->geoToCanvas(lat, lon + radius);
            double rPx = QVector2D(edge - center).length() + 6;
            painter.drawEllipse(center, rPx, rPx);
            painter.restore();
            return;
        }
    }
    // ---------- RECTANGLE ----------
    if (type == QLatin1String("rectangle")) {
        double w = props.value(QStringLiteral("width")).toDouble();
        double h = props.value(QStringLiteral("height")).toDouble();
        if (w > 0.0 && h > 0.0) {
            QPointF tl = map->geoToCanvas(lat + h / 2.0, lon - w / 2.0);
            QPointF br = map->geoToCanvas(lat - h / 2.0, lon + w / 2.0);
            QRectF r = QRectF(tl, br).normalized().adjusted(-6, -6, 6, 6);
            painter.drawRect(r);
            painter.restore();
            return;
        }
    }
    // ---------- BITMAP ----------
    if (type == QLatin1String("bitmap")) {
        int s = props.value(QStringLiteral("width")).toInt(28);
        if (s < 8) s = 8;
        QRectF r(center.x() - s / 2.0 - 4, center.y() - s / 2.0 - 4, s + 8, s + 8);
        painter.drawRect(r);
        painter.restore();
        return;
    }
    if (type == QLatin1String("text")) {
        const QString text = props.value(QStringLiteral("text")).toString();
        if (text.isEmpty()) {
            painter.drawEllipse(center, 14, 14);
            painter.restore();
            return;
        }
        int fontSize = props.value(QStringLiteral("fontSize")).toInt(14);
        if (fontSize < 8) fontSize = 14;

        QFont font(QStringLiteral("Arial"), fontSize, QFont::Bold);
        QFontMetrics fm(font);
        QRect br = fm.boundingRect(text);
        br.moveTo(static_cast<int>(center.x()),
                  static_cast<int>(center.y()) - fm.ascent());
        br.adjust(-6, -4, 6, 4);

        painter.drawRect(br);
        painter.restore();
        return;
    }
    // ---------- POLYGON / LINE ----------
    if (props.contains(QStringLiteral("points")) && props[QStringLiteral("points")].isArray()) {
        QJsonArray pts = props[QStringLiteral("points")].toArray();
        if (pts.size() >= 2) {
            QPolygonF poly;
            for (const QJsonValue& v : pts) {
                QJsonArray p = v.toArray();
                if (p.size() >= 2)
                    poly << map->geoToCanvas(p[1].toDouble(), p[0].toDouble());
            }
            if (!poly.isEmpty()) {
                if (type == QLatin1String("line"))
                    painter.drawPolyline(poly);
                else
                    painter.drawPolygon(poly);
                painter.restore();
                return;
            }
        }
    }

    // ---------- FALLBACK ----------
    painter.drawEllipse(center, 14, 14);
    painter.restore();
}
