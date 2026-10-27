#include "layer_api.h"
#include "backend/managers/layer_manager.h"
#include "backend/api/shape_api.h"
#include "backend/managers/shape_manager.h"
#include "backend/formats/geojson_simple.h"
#include "backend/formats/layer_format_registry.h"
#include "backend/formats/builtin_layer_formats.h"
#include <QFileInfo>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

LayerApi* LayerApi::m_instance = nullptr;

LayerApi* LayerApi::instance() {
    if (!m_instance) m_instance = new LayerApi();
    return m_instance;
}

LayerApi::LayerApi(QObject* parent) : QObject(parent) {
    registerBuiltInLayerFormats();
    LayerManager* lm = LayerManager::instance();
    connect(lm, &LayerManager::layerAdded, this, &LayerApi::layerAdded);
    connect(lm, &LayerManager::layerDeleted, this, &LayerApi::layerDeleted);
    connect(lm, &LayerManager::layerRenamed, this, &LayerApi::layerRenamed);
    connect(lm, &LayerManager::layerVisibilityChanged, this, &LayerApi::layerVisibilityChanged);
    connect(lm, &LayerManager::layerEditableChanged, this, &LayerApi::layerEditableChanged);
    connect(lm, &LayerManager::activeLayerChanged, this, &LayerApi::activeLayerChanged);
    connect(lm, &LayerManager::shapeAssignedToLayer, this, &LayerApi::shapeAssigned);
}

LayerApi::~LayerApi() {}

bool LayerApi::setEditable(const QString& id, bool editable) {
    return LayerManager::instance()->setLayerEditable(id, editable);
}

bool LayerApi::isEditable(const QString& id) const {
    return LayerManager::instance()->isLayerEditable(id);
}

QString LayerApi::addLayer(const QString& name) {
    return LayerManager::instance()->addLayer(name);
}

bool LayerApi::deleteLayer(const QString& id) {
    QStringList shapes = LayerManager::instance()->getShapesInLayer(id);
    for (const QString& sid : shapes)
        ShapeApi::instance()->deleteShape(sid);
    return LayerManager::instance()->deleteLayer(id);
}

bool LayerApi::renameLayer(const QString& id, const QString& newName) {
    return LayerManager::instance()->renameLayer(id, newName);
}

QStringList LayerApi::getLayers() const {
    return LayerManager::instance()->getLayerIds();
}

QStringList LayerApi::getLayerNames() const {
    return LayerManager::instance()->getLayerNames();
}

bool LayerApi::layerExists(const QString& id) const {
    return LayerManager::instance()->layerExists(id);
}

QString LayerApi::getLayerName(const QString& id) const {
    return LayerManager::instance()->getLayer(id).name;
}

bool LayerApi::setVisibility(const QString& id, bool visible) {
    return LayerManager::instance()->setLayerVisibility(id, visible);
}

bool LayerApi::isVisible(const QString& id) const {
    return LayerManager::instance()->isLayerVisible(id);
}

void LayerApi::setActiveLayer(const QString& id) {
    LayerManager::instance()->setActiveLayer(id);
}

QString LayerApi::getActiveLayer() const {
    LayerManager* lm = LayerManager::instance();
    QString activeId = lm->getActiveLayer();

    if (activeId.isEmpty() || !lm->layerExists(activeId)) {
        for (const QString& lid : lm->getLayerIds()) {
            if (lm->getLayer(lid).name.compare(QStringLiteral("Default Layer"),
                                               Qt::CaseInsensitive) == 0) {
                activeId = lid;
                lm->setActiveLayer(activeId);
                return activeId;
            }
        }
        activeId = const_cast<LayerApi*>(this)->addLayer(QStringLiteral("Default Layer"));
        lm->setActiveLayer(activeId);
    }
    return activeId;
}
bool LayerApi::assignShapeToLayer(const QString& shapeId, const QString& layerId) {
    if (!LayerManager::instance()->isLayerEditable(layerId))
        return false;
    return LayerManager::instance()->addShapeToLayer(shapeId, layerId);
}

bool LayerApi::removeShapeFromLayer(const QString& shapeId) {
    return LayerManager::instance()->removeShapeFromLayer(shapeId);
}

QString LayerApi::getLayerOfShape(const QString& shapeId) const {
    return LayerManager::instance()->getLayerForShape(shapeId);
}

QStringList LayerApi::getShapesOfLayer(const QString& layerId) const {
    return LayerManager::instance()->getShapesInLayer(layerId);
}

bool LayerApi::exportLayer(const QString& layerId, const QString& format, const QString& filePath) {
    if (!LayerManager::instance()->layerExists(layerId)) return false;
    QStringList shapeIds = LayerManager::instance()->getShapesInLayer(layerId);
    if (shapeIds.isEmpty()) return false;
    QString fmt = format.toLower().trimmed();
    if (fmt.isEmpty()) {
        if (ILayerFormat* byExt = LayerFormatRegistry::instance()->formatForPath(filePath))
            fmt = byExt->id();
    }
    ILayerFormat* handler = LayerFormatRegistry::instance()->format(fmt);
    if (!handler) {
        if (fmt == "geojson" || fmt == "json")
            return GeoJsonSimple::exportShapes(shapeIds, filePath);
        qWarning() << "Unsupported export format:" << fmt;
        return false;
    }
    QList<ShapeData> shapes;
    for (const QString& sid : shapeIds) {
        ShapeData s = ShapeManager::instance()->getShape(sid);
        if (!s.id.isEmpty()) shapes.append(s);
    }
    if (shapes.isEmpty()) return false;
    return handler->exportShapes(shapes, filePath);
}

bool LayerApi::importLayer(const QString& filePath,
                           const QString& format,
                           const QString& layerName)
{
    registerBuiltInLayerFormats();
    QString fmt = format.toLower().trimmed();
    if (fmt.isEmpty()) {
        if (ILayerFormat* byExt = LayerFormatRegistry::instance()->formatForPath(filePath))
            fmt = byExt->id();
        else {
            const QString ext = QFileInfo(filePath).suffix().toLower();
            if (ext == QLatin1String("geojson") || ext == QLatin1String("json"))
                fmt = QStringLiteral("geojson");
        }
    }

    ILayerFormat* handler = LayerFormatRegistry::instance()->format(fmt);
    QList<ShapeData> shapes;
    if (handler) {
        shapes = handler->importShapes(filePath);
    } else if (fmt == QLatin1String("geojson") || fmt == QLatin1String("json")) {
        shapes = GeoJsonSimple::importShapes(filePath);
    } else {
        qWarning() << "Unsupported import format:" << fmt
                   << "(handler null — formats registered?"
                   << LayerFormatRegistry::instance()->formatIds() << ")";
        return false;
    }

    if (shapes.isEmpty()) {
        qWarning() << "importLayer: 0 shapes from" << filePath << "fmt=" << fmt;
        return false;
    }

    static QSet<QString> s_importedPaths;
    const QString absPath = QFileInfo(filePath).absoluteFilePath();
    if (s_importedPaths.contains(absPath)) {
        qWarning() << "Layer already exists (same file):" << absPath;
        return false;
    }

    QString name = layerName.isEmpty()
                       ? QFileInfo(filePath).completeBaseName()
                       : layerName;

    if (!LayerManager::instance()->findLayerIdByName(name).isEmpty()) {
        QString candidate = name + QLatin1Char('_')
                            + (fmt.isEmpty() ? QStringLiteral("import") : fmt);
        if (LayerManager::instance()->findLayerIdByName(candidate).isEmpty()) {
            name = candidate;
        } else {
            const QString base = name;
            int n = 2;
            do {
                name = base + QStringLiteral(" (%1)").arg(n++);
            } while (!LayerManager::instance()->findLayerIdByName(name).isEmpty());
        }
    }

    const QString newLayerId = LayerManager::instance()->addLayer(name);
    LayerManager::instance()->setActiveLayer(newLayerId);

    int added = 0;
    for (const ShapeData& s : shapes) {
        if (s.type.isEmpty())
            continue;
        QJsonObject json;
        json[QStringLiteral("type")]      = s.type;
        json[QStringLiteral("longitude")] = s.longitude;
        json[QStringLiteral("latitude")]  = s.latitude;
        json[QStringLiteral("radius")]    = s.radius;
        json[QStringLiteral("width")]     = s.width;
        json[QStringLiteral("height")]    = s.height;
        json[QStringLiteral("color")]     = s.color.isValid()
                                            ? s.color.name()
                                            : QStringLiteral("#ff4444");
        json[QStringLiteral("text")]      = s.text;
        json[QStringLiteral("lineWidth")] = s.lineWidth > 0 ? s.lineWidth : 2;
        json[QStringLiteral("rotation")]  = s.rotation;
        if (!s.imagePath.isEmpty())
            json[QStringLiteral("imagePath")] = s.imagePath;
        if (s.fontSize > 0)
            json[QStringLiteral("fontSize")] = s.fontSize;
        if (!s.points.isEmpty()) {
            QJsonArray pts;
            for (const QPointF& p : s.points) {
                QJsonArray pt;
                pt.append(p.x());
                pt.append(p.y());
                pts.append(pt);
            }
            json[QStringLiteral("points")] = pts;
        }

        const QString sid = ShapeApi::instance()->addShape(json);
        if (!sid.isEmpty())
            assignShapeToLayer(sid, newLayerId);
        ++added;
    }

    if (added == 0) {
        qWarning() << "importLayer: parsed shapes but none added";
        LayerManager::instance()->deleteLayer(newLayerId);
        return false;
    }

    LayerManager::instance()->setLayerEditable(newLayerId, false);
    s_importedPaths.insert(absPath);
    return true;
}

QJsonObject LayerApi::toJson() const
{
    return LayerManager::instance()->toJson();
}

void LayerApi::fromJson(const QJsonObject& obj)
{
    LayerManager::instance()->fromJson(obj);
    emit layersReset();
}
