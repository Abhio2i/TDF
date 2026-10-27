
#include "shape_api.h"
#include "backend/managers/shape_manager.h"
#include "backend/api/layer_api.h"
#include <QDebug>

ShapeApi* ShapeApi::m_instance = nullptr;

ShapeApi* ShapeApi::instance() {
    if (!m_instance) {
        m_instance = new ShapeApi();
    }
    return m_instance;
}

ShapeApi::ShapeApi(QObject* parent) : QObject(parent) {
    connect(ShapeManager::instance(), &ShapeManager::shapeAdded,
            this, &ShapeApi::shapeAdded);
    connect(ShapeManager::instance(), &ShapeManager::shapeUpdated,
            this, &ShapeApi::shapeUpdated);
    connect(ShapeManager::instance(), &ShapeManager::shapeDeleted,
            this, &ShapeApi::shapeDeleted);
    connect(ShapeManager::instance(), &ShapeManager::allShapesDeleted,
            this, &ShapeApi::allShapesDeleted);
}

ShapeApi::~ShapeApi() {}
static bool layerIsRasterOnly(const QString& layerId)
{
    if (layerId.isEmpty())
        return false;
    LayerApi* la = LayerApi::instance();
    const QStringList shapes = la->getShapesOfLayer(layerId);
    if (shapes.isEmpty())
        return false;
    for (const QString& sid : shapes) {
        const QString t = ShapeApi::instance()->getShapeType(sid);
        if (t.compare(QLatin1String("bitmap"), Qt::CaseInsensitive) != 0)
            return false;
    }
    return true;
}

static QString resolveVectorLayer(const QString& preferred)
{
    LayerApi* la = LayerApi::instance();
    if (!preferred.isEmpty() && !layerIsRasterOnly(preferred))
        return preferred;

    for (const QString& lid : la->getLayers()) {
        if (la->getLayerName(lid).compare(QStringLiteral("Default Layer"), Qt::CaseInsensitive) == 0
            && !layerIsRasterOnly(lid))
            return lid;
    }
    for (const QString& lid : la->getLayers()) {
        if (!layerIsRasterOnly(lid))
            return lid;
    }
    return preferred;
}

void ShapeApi::assignToActiveLayer(const QString& shapeId)
{
    LayerApi* la = LayerApi::instance();
    QString activeLayerId = la->getActiveLayer();

    bool activeIsRaster = false;
    if (!activeLayerId.isEmpty()) {
        const QStringList existingShapes = la->getShapesOfLayer(activeLayerId);
        for (const QString& sid : existingShapes) {
            const QString t = getShapeType(sid);
            if (t.compare(QLatin1String("bitmap"), Qt::CaseInsensitive) == 0
                || t.compare(QLatin1String("raster_image"), Qt::CaseInsensitive) == 0) {
                activeIsRaster = true;
                break;
            }
        }
    }
    if (activeLayerId.isEmpty() || activeIsRaster) {
        QString defaultId;
        for (const QString& lid : la->getLayers()) {
            if (la->getLayerName(lid).compare(QStringLiteral("Default Layer"), Qt::CaseInsensitive) == 0) {
                defaultId = lid;
                break;
            }
        }
        if (!defaultId.isEmpty())
            la->assignShapeToLayer(shapeId, defaultId);
        return;
    }
    la->assignShapeToLayer(shapeId, activeLayerId);
}


// ============================================================
// CREATE
// ============================================================

QString ShapeApi::addShape(const QJsonObject& data) {
    ShapeData shape = shapeFromJson(data);
    QString id = ShapeManager::instance()->addShape(shape);
    assignToActiveLayer(id);
    return id;
}

QString ShapeApi::addCircle(double lon, double lat, double radius, const QColor& color) {
    ShapeData shape;
    shape.type = "circle";
    shape.longitude = lon;
    shape.latitude = lat;
    shape.radius = radius;
    shape.color = color;
    QString id = ShapeManager::instance()->addShape(shape);
    assignToActiveLayer(id);
    return id;
}

QString ShapeApi::addRectangle(double lon, double lat, double w, double h, const QColor& color) {
    ShapeData shape;
    shape.type = "rectangle";
    shape.longitude = lon;
    shape.latitude = lat;
    shape.width = w;
    shape.height = h;
    shape.color = color;
    QString id = ShapeManager::instance()->addShape(shape);
    assignToActiveLayer(id);
    return id;
}

QString ShapeApi::addPolygon(const QVector<QPointF>& points, const QColor& color) {
    ShapeData shape;
    shape.type = "polygon";
    shape.points = points;
    shape.color = color;
    double cx = 0, cy = 0;
    for (const auto& p : points) { cx += p.x(); cy += p.y(); }
    if (!points.isEmpty()) {
        shape.longitude = cx / points.size();
        shape.latitude = cy / points.size();
    }
    QString id = ShapeManager::instance()->addShape(shape);
    assignToActiveLayer(id);
    return id;
}

QString ShapeApi::addLine(const QVector<QPointF>& points, const QColor& color) {
    ShapeData shape;
    shape.type = "line";
    shape.points = points;
    shape.color = color;
    double cx = 0, cy = 0;
    for (const auto& p : points) { cx += p.x(); cy += p.y(); }
    if (!points.isEmpty()) {
        shape.longitude = cx / points.size();
        shape.latitude = cy / points.size();
    }
    QString id = ShapeManager::instance()->addShape(shape);
    assignToActiveLayer(id);
    return id;
}

QString ShapeApi::addPoint(double lon, double lat, const QColor& color) {
    ShapeData shape;
    shape.type = "point";
    shape.longitude = lon;
    shape.latitude = lat;
    shape.color = color;
    QString id = ShapeManager::instance()->addShape(shape);
    assignToActiveLayer(id);
    return id;
}

QString ShapeApi::addText(double lon, double lat, const QString& text, const QColor& color) {
    ShapeData shape;
    shape.type = "text";
    shape.longitude = lon;
    shape.latitude = lat;
    shape.text = text;
    shape.color = color;
    shape.fontSize = 14;
    QString id = ShapeManager::instance()->addShape(shape);
    assignToActiveLayer(id);
    return id;
}

// ============================================================
// DELETE
// ============================================================

bool ShapeApi::deleteShape(const QString& id) {
    if (id.isEmpty())
        return false;

    LayerApi* la = LayerApi::instance();
    const QString layerId = la->getLayerOfShape(id);
    const QString type = getShapeType(id);

    la->removeShapeFromLayer(id);
    const bool ok = ShapeManager::instance()->deleteShape(id);

    if (ok && !layerId.isEmpty()
        && type.compare(QLatin1String("raster_image"), Qt::CaseInsensitive) == 0) {
        if (la->getShapesOfLayer(layerId).isEmpty()) {
            const QString layerName = la->getLayerName(layerId);
            if (layerName.compare(QStringLiteral("Default Layer"), Qt::CaseInsensitive) != 0)
                la->deleteLayer(layerId);
        }
    }
    return ok;
}
bool ShapeApi::deleteAllShapes() {
    const QVector<ShapeData> existing = ShapeManager::instance()->getAllShapes();
    for (const ShapeData& s : existing) {
        if (!s.id.isEmpty())
            LayerApi::instance()->removeShapeFromLayer(s.id);
    }
    return ShapeManager::instance()->deleteAllShapes();
}

int ShapeApi::deleteShapesByType(const QString& type) {
    return ShapeManager::instance()->deleteShapesByType(type);
}

// ============================================================
// EDIT
// ============================================================

bool ShapeApi::updateShape(const QString& id, const QJsonObject& data) {
    ShapeData existing = ShapeManager::instance()->getShape(id);
    if (existing.id.isEmpty()) return false;
    ShapeData shape = shapeFromJson(data, existing);
    return ShapeManager::instance()->updateShape(id, shape);
}

bool ShapeApi::updateShapeColor(const QString& id, const QColor& color) {
    ShapeData shape = ShapeManager::instance()->getShape(id);
    if (shape.id.isEmpty()) return false;
    shape.color = color;
    return ShapeManager::instance()->updateShape(id, shape);
}

bool ShapeApi::updateShapeWidth(const QString& id, int width) {
    ShapeData shape = ShapeManager::instance()->getShape(id);
    if (shape.id.isEmpty()) return false;
    shape.lineWidth = width;
    return ShapeManager::instance()->updateShape(id, shape);
}

bool ShapeApi::moveShape(const QString& id, double newLon, double newLat) {
    return ShapeManager::instance()->moveShape(id, newLon, newLat);
}

bool ShapeApi::moveShapeBy(const QString& id, double deltaLon, double deltaLat) {
    ShapeData shape = ShapeManager::instance()->getShape(id);
    if (shape.id.isEmpty()) return false;
    return ShapeManager::instance()->moveShape(id, shape.longitude + deltaLon, shape.latitude + deltaLat);
}

// ============================================================
// GET
// ============================================================

QJsonObject ShapeApi::getShapeProperties(const QString& id) const {
    return shapeToJson(ShapeManager::instance()->getShape(id));
}

QJsonArray ShapeApi::getAllShapes() const {
    QJsonArray array;
    for (const ShapeData& shape : ShapeManager::instance()->getAllShapes()) {
        array.append(shapeToJson(shape));
    }
    return array;
}

int ShapeApi::getShapeCount() const {
    return ShapeManager::instance()->getShapeCount();
}

QString ShapeApi::getShapeType(const QString& id) const {
    return ShapeManager::instance()->getShapeType(id);
}

QString ShapeApi::findShapeAt(const QPointF& geoPos) const {
    return ShapeManager::instance()->findShapeAt(geoPos);
}

// ============================================================
// Single-shape JSON
// ============================================================

ShapeData ShapeApi::shapeFromJson(const QJsonObject& json) const {
    return shapeFromJson(json, ShapeData());
}

ShapeData ShapeApi::shapeFromJson(const QJsonObject& json, const ShapeData& base) const {
    ShapeData shape = base;
    if (json.contains("type"))
        shape.type = json["type"].toString();
    if (json.contains("longitude"))
        shape.longitude = json["longitude"].toDouble();
    if (json.contains("latitude"))
        shape.latitude = json["latitude"].toDouble();
    if (json.contains("radius"))
        shape.radius = json["radius"].toDouble();
    if (json.contains("width"))
        shape.width = json["width"].toDouble();
    if (json.contains("height"))
        shape.height = json["height"].toDouble();
    if (json.contains("lineWidth"))
        shape.lineWidth = json["lineWidth"].toInt(shape.lineWidth);
    if (json.contains("color"))
        shape.color = QColor(json["color"].toString(shape.color.name()));
    if (json.contains("text"))
        shape.text = json["text"].toString();
    if (json.contains("fontSize"))
        shape.fontSize = json["fontSize"].toInt(shape.fontSize > 0 ? shape.fontSize : 14);
    if (json.contains("rotation"))
        shape.rotation = json["rotation"].toDouble();
    if (json.contains("imagePath"))
        shape.imagePath = json["imagePath"].toString();
    if (json.contains("points") && json["points"].isArray()) {
        QVector<QPointF> pts;
        QJsonArray pointsArray = json["points"].toArray();
        for (const QJsonValue& val : pointsArray) {
            QJsonArray pt = val.toArray();
            if (pt.size() >= 2) {
                pts.append(QPointF(pt[0].toDouble(), pt[1].toDouble()));
            }
        }
        shape.points = pts;
    }
    return shape;
}

QJsonObject ShapeApi::shapeToJson(const ShapeData& shape) const {
    QJsonObject json;
    json["id"] = shape.id;
    json["type"] = shape.type;
    json["longitude"] = shape.longitude;
    json["latitude"] = shape.latitude;
    json["radius"] = shape.radius;
    json["width"] = shape.width;
    json["height"] = shape.height;
    json["lineWidth"] = shape.lineWidth;
    json["color"] = shape.color.name();
    json["text"] = shape.text;
    json["fontSize"] = shape.fontSize > 0 ? shape.fontSize : 14;
    json["rotation"] = shape.rotation;
    if (!shape.imagePath.isEmpty())
        json["imagePath"] = shape.imagePath;
    if (!shape.points.isEmpty()) {
        QJsonArray pointsArray;
        for (const QPointF& p : shape.points) {
            QJsonArray pt;
            pt.append(p.x());
            pt.append(p.y());
            pointsArray.append(pt);
        }
        json["points"] = pointsArray;
    }
    if (!shape.id.isEmpty()) {
        const QString layerId = LayerApi::instance()->getLayerOfShape(shape.id);
        if (!layerId.isEmpty()) {
            json[QStringLiteral("layerId")] = layerId;
            json[QStringLiteral("layerName")] = LayerApi::instance()->getLayerName(layerId);
        }
    }
    return json;
}
// ============================================================
// Document JSON (public) — scenario / runtime file
// ============================================================

QJsonObject ShapeApi::toJson() const
{
    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("shapes")] = getAllShapes();
    root[QStringLiteral("layers")] = LayerApi::instance()->toJson();
    return root;
}

void ShapeApi::fromJson(const QJsonObject& obj)
{
    if (obj.isEmpty()) {
        deleteAllShapes();
        LayerApi::instance()->fromJson(QJsonObject());
        return;
    }

    if (obj.contains(QStringLiteral("layers")) && obj.value(QStringLiteral("layers")).isObject()) {
        LayerApi::instance()->fromJson(obj.value(QStringLiteral("layers")).toObject());
    } else {
        LayerApi::instance()->fromJson(QJsonObject());
    }

    if (obj.contains(QStringLiteral("shapes")) && obj.value(QStringLiteral("shapes")).isArray()) {
        loadShapes(obj.value(QStringLiteral("shapes")).toArray(), true);
    } else {
        deleteAllShapes();
    }
}

void ShapeApi::loadShapes(const QJsonArray& shapes, bool clearExisting)
{
    if (clearExisting)
        deleteAllShapes();

    LayerApi* la = LayerApi::instance();

    for (const QJsonValue& v : shapes) {
        if (!v.isObject())
            continue;
        const QJsonObject o = v.toObject();
        if (o.value(QStringLiteral("type")).toString().trimmed().isEmpty())
            continue;

        QString targetLayerName = o.value(QStringLiteral("layerName")).toString().trimmed();
        QString targetLayerId;
        if (!targetLayerName.isEmpty()) {
            for (const QString& lid : la->getLayers()) {
                if (la->getLayerName(lid) == targetLayerName) {
                    targetLayerId = lid;
                    break;
                }
            }
        }
        if (targetLayerId.isEmpty()) {
            const QString savedLayerId = o.value(QStringLiteral("layerId")).toString();
            if (!savedLayerId.isEmpty() && la->layerExists(savedLayerId))
                targetLayerId = savedLayerId;
        }

        const QString prevActive = la->getActiveLayer();
        if (!targetLayerId.isEmpty())
            la->setActiveLayer(targetLayerId);

        const QString sid = addShape(o);

        if (!sid.isEmpty() && !targetLayerId.isEmpty())
            la->assignShapeToLayer(sid, targetLayerId);

        if (!prevActive.isEmpty() && prevActive != la->getActiveLayer())
            la->setActiveLayer(prevActive);
    }
}

void ShapeApi::importFromLegacyTactical(const QJsonObject& tactical)
{
    const QJsonArray meshes = tactical.value(QStringLiteral("tempMeshes")).toArray();
    const QJsonObject layersRoot = tactical.value(QStringLiteral("layers")).toObject();
    const QJsonArray layerArr = layersRoot.value(QStringLiteral("layers")).toArray();

    if (meshes.isEmpty() && layerArr.isEmpty()) {
        fromJson(QJsonObject());
        return;
    }

    deleteAllShapes();
    LayerApi::instance()->fromJson(QJsonObject());
    LayerApi* la = LayerApi::instance();
    QMap<QString, QString> nameToLayerId;
    QMap<QString, QString> meshNameToLayerId;
    for (const QJsonValue& lv : layerArr) {
        if (!lv.isObject())
            continue;
        const QJsonObject lo = lv.toObject();
        QString lname = lo.value(QStringLiteral("name")).toString().trimmed();
        if (lname.isEmpty())
            continue;

        QString lid;
        for (const QString& existing : la->getLayers()) {
            if (la->getLayerName(existing) == lname) {
                lid = existing;
                break;
            }
        }
        if (lid.isEmpty())
            lid = la->addLayer(lname);

        nameToLayerId.insert(lname, lid);
        la->setVisibility(lid, lo.value(QStringLiteral("visible")).toBool(true));

        const QJsonArray shapesMeta = lo.value(QStringLiteral("shapes")).toArray();
        for (const QJsonValue& sv : shapesMeta) {
            if (!sv.isObject())
                continue;
            const QString meshName = sv.toObject().value(QStringLiteral("id")).toString();
            if (!meshName.isEmpty())
                meshNameToLayerId.insert(meshName, lid);
        }
    }

    const QString activeName = layersRoot.value(QStringLiteral("activeLayer")).toString();
    if (!activeName.isEmpty() && nameToLayerId.contains(activeName))
        la->setActiveLayer(nameToLayerId.value(activeName));
    else if (!la->getLayers().isEmpty())
        la->setActiveLayer(la->getLayers().first());

    for (const QJsonValue& mv : meshes) {
        if (!mv.isObject())
            continue;
        const QJsonObject mesh = mv.toObject();
        const QString name = mesh.value(QStringLiteral("name")).toString();
        if (name.isEmpty())
            continue;

        const QJsonObject pos = mesh.value(QStringLiteral("position")).toObject();
        const double lon = pos.value(QStringLiteral("x")).toDouble();
        const double lat = pos.value(QStringLiteral("y")).toDouble();

        const QJsonObject m = mesh.value(QStringLiteral("mesh")).toObject();
        QColor color = Qt::red;
        if (m.contains(QStringLiteral("color"))) {
            const QJsonObject c = m.value(QStringLiteral("color")).toObject();
            color = QColor(c.value(QStringLiteral("r")).toInt(255),
                           c.value(QStringLiteral("g")).toInt(0),
                           c.value(QStringLiteral("b")).toInt(0),
                           c.value(QStringLiteral("a")).toInt(255));
        }
        const int lineWidth = m.value(QStringLiteral("lineWidth")).toInt(2);

        QJsonObject shapeJson;
        shapeJson[QStringLiteral("longitude")] = lon;
        shapeJson[QStringLiteral("latitude")]  = lat;
        shapeJson[QStringLiteral("color")]     = color.name(QColor::HexRgb);
        shapeJson[QStringLiteral("lineWidth")] = lineWidth > 0 ? lineWidth : 2;

        if (name.startsWith(QStringLiteral("TempCircle"), Qt::CaseInsensitive)) {
            shapeJson[QStringLiteral("type")] = QStringLiteral("circle");
            const QJsonObject size = mesh.value(QStringLiteral("size")).toObject();
            double radius = size.value(QStringLiteral("x")).toDouble();
            if (radius <= 0.0)
                radius = 0.05;
            shapeJson[QStringLiteral("radius")] = radius;
        } else if (name.startsWith(QStringLiteral("TempText"), Qt::CaseInsensitive)) {
            shapeJson[QStringLiteral("type")] = QStringLiteral("text");
            QString text = mesh.value(QStringLiteral("text")).toString();
            if (text.isEmpty())
                text = name;
            shapeJson[QStringLiteral("text")] = text;
            int fontSize = 14;
            const QJsonObject tf = mesh.value(QStringLiteral("textFont")).toObject();
            if (tf.contains(QStringLiteral("size")))
                fontSize = tf.value(QStringLiteral("size")).toInt(14);
            shapeJson[QStringLiteral("fontSize")] = fontSize;
            if (mesh.contains(QStringLiteral("textColor"))) {
                const QJsonObject tc = mesh.value(QStringLiteral("textColor")).toObject();
                color = QColor(tc.value(QStringLiteral("r")).toInt(0),
                               tc.value(QStringLiteral("g")).toInt(0),
                               tc.value(QStringLiteral("b")).toInt(0),
                               tc.value(QStringLiteral("a")).toInt(255));
                shapeJson[QStringLiteral("color")] = color.name(QColor::HexRgb);
            }
        } else if (name.startsWith(QStringLiteral("TempLine"), Qt::CaseInsensitive)
                   || name.startsWith(QStringLiteral("TempPolygon"), Qt::CaseInsensitive)) {
            const QJsonArray poly = m.value(QStringLiteral("polygen")).toArray();
            QJsonArray pts;
            for (const QJsonValue& pv : poly) {
                if (!pv.isObject())
                    continue;
                const QJsonObject p = pv.toObject();
                QJsonArray pt;
                pt.append(p.value(QStringLiteral("x")).toDouble());
                pt.append(p.value(QStringLiteral("y")).toDouble());
                pts.append(pt);
            }
            if (name.startsWith(QStringLiteral("TempPolygon"), Qt::CaseInsensitive))
                shapeJson[QStringLiteral("type")] = QStringLiteral("polygon");
            else
                shapeJson[QStringLiteral("type")] = QStringLiteral("line");
            if (!pts.isEmpty())
                shapeJson[QStringLiteral("points")] = pts;
            else
                shapeJson[QStringLiteral("type")] = QStringLiteral("point");
        } else {
            shapeJson[QStringLiteral("type")] = QStringLiteral("point");
        }

        QString targetLayer = meshNameToLayerId.value(name);
        if (targetLayer.isEmpty())
            targetLayer = la->getActiveLayer();

        const QString prev = la->getActiveLayer();
        if (!targetLayer.isEmpty())
            la->setActiveLayer(targetLayer);

        const QString sid = addShape(shapeJson);
        if (!sid.isEmpty() && !targetLayer.isEmpty())
            la->assignShapeToLayer(sid, targetLayer);

        if (!prev.isEmpty())
            la->setActiveLayer(prev);
    }
}
