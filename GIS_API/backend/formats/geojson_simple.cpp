#include "geojson_simple.h"
#include "backend/api/shape_api.h"
#include "backend/managers/shape_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <cmath>

// ============================================================
// IMPORT
// ============================================================

QList<ShapeData> GeoJsonSimple::importShapes(const QString& filePath)
{
    QList<ShapeData> result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "GeoJsonSimple: cannot open" << filePath;
        return result;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "GeoJsonSimple: parse error" << err.errorString();
        return result;
    }
    QJsonObject root = doc.object();
    if (!root.contains("features") || !root["features"].isArray()) {
        return result;
    }
    QJsonArray features = root["features"].toArray();
    for (const QJsonValue& val : features) {
        if (!val.isObject()) continue;
        QJsonObject feature = val.toObject();
        if (!feature.contains("geometry") || !feature["geometry"].isObject())
            continue;
        QJsonObject geometry = feature["geometry"].toObject();
        QString geomType = geometry["type"].toString();
        QJsonValue coordsVal = geometry["coordinates"];
        QJsonObject props;
        if (feature.contains("properties") && feature["properties"].isObject())
            props = feature["properties"].toObject();
        QString name = props.value("name").toString();
        if (name.isEmpty()) name = props.value("Name").toString();
        QString typeHint = props.value("type").toString().toLower();
        double radius = props.value("radius").toDouble(0.0);
        QColor color = QColor(props.value("color").toString("#ff0000"));
        if (!color.isValid()) color = Qt::red;
        ShapeData shape;
        shape.color = color;
        shape.text = props.value("text").toString();
        // ---------- POINT ----------
        if (geomType.compare("Point", Qt::CaseInsensitive) == 0) {
            if (!coordsVal.isArray()) continue;
            QJsonArray c = coordsVal.toArray();
            if (c.size() < 2) continue;
            shape.longitude = c[0].toDouble();
            shape.latitude  = c[1].toDouble();
            if (typeHint == "text") {
                shape.type = "text";
            } else {
                shape.type = "point";
            }
            result.append(shape);
        }
        // ---------- LINESTRING ----------
        else if (geomType.compare("LineString", Qt::CaseInsensitive) == 0) {
            if (!coordsVal.isArray()) continue;
            QJsonArray coords = coordsVal.toArray();
            if (coords.size() < 2) continue;
            QVector<QPointF> pts;
            double sumLon = 0, sumLat = 0;
            for (const QJsonValue& pv : coords) {
                if (!pv.isArray()) continue;
                QJsonArray p = pv.toArray();
                if (p.size() < 2) continue;
                double lon = p[0].toDouble();
                double lat = p[1].toDouble();
                pts.append(QPointF(lon, lat));
                sumLon += lon;
                sumLat += lat;
            }
            if (pts.size() < 2) continue;
            shape.type = "line";
            shape.points = pts;
            shape.longitude = sumLon / pts.size();
            shape.latitude  = sumLat / pts.size();
            result.append(shape);
        }
        // ---------- POLYGON ----------
        else if (geomType.compare("Polygon", Qt::CaseInsensitive) == 0) {
            if (!coordsVal.isArray()) continue;
            QJsonArray rings = coordsVal.toArray();
            if (rings.isEmpty() || !rings[0].isArray()) continue;
            QJsonArray ring = rings[0].toArray();
            int n = ring.size();
            if (n > 1) {
                QJsonArray first = ring.first().toArray();
                QJsonArray last  = ring.last().toArray();
                if (first.size() >= 2 && last.size() >= 2) {
                    if (qAbs(first[0].toDouble() - last[0].toDouble()) < 1e-12 &&
                        qAbs(first[1].toDouble() - last[1].toDouble()) < 1e-12) {
                        n--;
                    }
                }
            }
            if (n < 3) continue;
            // Circle?
            if (typeHint == "circle" || typeHint == "tempcircle") {
                double minLon = 1e9, maxLon = -1e9, minLat = 1e9, maxLat = -1e9;
                for (int i = 0; i < n; ++i) {
                    if (!ring[i].isArray()) continue;
                    QJsonArray p = ring[i].toArray();
                    if (p.size() < 2) continue;
                    double lon = p[0].toDouble();
                    double lat = p[1].toDouble();
                    minLon = qMin(minLon, lon); maxLon = qMax(maxLon, lon);
                    minLat = qMin(minLat, lat); maxLat = qMax(maxLat, lat);
                }
                shape.type = "circle";
                shape.longitude = (minLon + maxLon) / 2.0;
                shape.latitude  = (minLat + maxLat) / 2.0;
                shape.radius = (radius > 0) ? radius : ((maxLon - minLon) / 2.0);
                result.append(shape);
            } else {
                QVector<QPointF> pts;
                double sumLon = 0, sumLat = 0;
                for (int i = 0; i < n; ++i) {
                    if (!ring[i].isArray()) continue;
                    QJsonArray p = ring[i].toArray();
                    if (p.size() < 2) continue;
                    double lon = p[0].toDouble();
                    double lat = p[1].toDouble();
                    pts.append(QPointF(lon, lat));
                    sumLon += lon;
                    sumLat += lat;
                }
                if (pts.size() < 3) continue;
                shape.type = (typeHint == "rectangle") ? "rectangle" : "polygon";
                shape.points = pts;
                shape.longitude = sumLon / pts.size();
                shape.latitude  = sumLat / pts.size();
                if (shape.type == "rectangle" && pts.size() >= 4) {
                    shape.width  = qAbs(pts[0].x() - pts[2].x());
                    shape.height = qAbs(pts[0].y() - pts[2].y());
                }
                result.append(shape);
            }
        }
    }
    return result;
}

// ============================================================
// EXPORT
// ============================================================

bool GeoJsonSimple::exportShapes(const QStringList& shapeIds, const QString& filePath)
{
    QJsonObject root;
    root["type"] = "FeatureCollection";
    QJsonObject crsProps;
    crsProps["name"] = "EPSG:4326";
    QJsonObject crs;
    crs["type"] = "name";
    crs["properties"] = crsProps;
    root["crs"] = crs;
    QJsonArray features;
    for (const QString& id : shapeIds) {
        ShapeData s = ShapeManager::instance()->getShape(id);
        if (s.id.isEmpty()) continue;
        QJsonObject feature;
        feature["type"] = "Feature";
        QJsonObject props;
        props["id"] = s.id;
        props["type"] = s.type;
        props["color"] = s.color.name();
        if (!s.text.isEmpty()) props["text"] = s.text;
        if (s.type == "circle") props["radius"] = s.radius;
        feature["properties"] = props;
        QJsonObject geometry;
        if (s.type == "point" || s.type == "text") {
            geometry["type"] = "Point";
            QJsonArray c;
            c.append(s.longitude);
            c.append(s.latitude);
            geometry["coordinates"] = c;
        }
        else if (s.type == "line") {
            geometry["type"] = "LineString";
            QJsonArray coords;
            for (const QPointF& p : s.points) {
                QJsonArray pt;
                pt.append(p.x());
                pt.append(p.y());
                coords.append(pt);
            }
            geometry["coordinates"] = coords;
        }
        else if (s.type == "polygon" || s.type == "rectangle") {
            geometry["type"] = "Polygon";
            QJsonArray ring;
            if (!s.points.isEmpty()) {
                for (const QPointF& p : s.points) {
                    QJsonArray pt;
                    pt.append(p.x());
                    pt.append(p.y());
                    ring.append(pt);
                }
            } else if (s.type == "rectangle" && s.width > 0 && s.height > 0) {
                double halfW = s.width / 2.0;
                double halfH = s.height / 2.0;
                double lon = s.longitude;
                double lat = s.latitude;
                auto add = [&](double x, double y) {
                    QJsonArray pt;
                    pt.append(x);
                    pt.append(y);
                    ring.append(pt);
                };
                add(lon - halfW, lat + halfH);
                add(lon + halfW, lat + halfH);
                add(lon + halfW, lat - halfH);
                add(lon - halfW, lat - halfH);
            }
            if (!ring.isEmpty()) {
                QJsonArray first = ring.first().toArray();
                QJsonArray last  = ring.last().toArray();
                if (first != last)
                    ring.append(first);
            }
            if (ring.size() < 4) {
                continue;
            }
            QJsonArray coords;
            coords.append(ring);
            geometry["coordinates"] = coords;
        }

        else if (s.type == "circle") {
            geometry["type"] = "Polygon";
            QJsonArray ring;
            const int N = 36;
            for (int i = 0; i <= N; ++i) {
                double angle = (i * 360.0 / N) * M_PI / 180.0;
                double lon = s.longitude + s.radius * std::cos(angle);
                double lat = s.latitude  + s.radius * std::sin(angle);
                QJsonArray pt;
                pt.append(lon);
                pt.append(lat);
                ring.append(pt);
            }
            QJsonArray coords;
            coords.append(ring);
            geometry["coordinates"] = coords;
        }
        else {
            continue;
        }
        feature["geometry"] = geometry;
        features.append(feature);
    }
    root["features"] = features;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "GeoJsonSimple: cannot write" << filePath;
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
