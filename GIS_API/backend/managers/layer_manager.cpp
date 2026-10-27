#include "layer_manager.h"
#include <QUuid>
#include <QJsonArray>
#include <QJsonObject>

LayerManager* LayerManager::m_instance = nullptr;

LayerManager* LayerManager::instance() {
    if (!m_instance) {
        m_instance = new LayerManager();
    }
    return m_instance;
}

LayerManager::LayerManager(QObject* parent) : QObject(parent) {
    QString defaultId = addLayer("Default Layer");
    setActiveLayer(defaultId);
}

QString LayerManager::generateId() {
    return "layer_" + QString::number(++m_counter) + "_" +
           QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QString LayerManager::addLayer(const QString& name) {
    QString id = generateId();
    LayerInfo info;
    info.id = id;
    info.name = name.isEmpty() ? ("Layer " + QString::number(m_counter)) : name;
    info.visible = true;
    info.editable = true;
    m_layers[id] = info;
    m_layerOrder.append(id);
    emit layerAdded(id);
    return id;
}
// ---- setLayerEditable ----
bool LayerManager::setLayerEditable(const QString& id, bool editable) {
    if (!m_layers.contains(id)) return false;
    if (m_layers[id].editable == editable) return true;
    m_layers[id].editable = editable;
    emit layerEditableChanged(id, editable);
    return true;
}

// ---- isLayerEditable ----
bool LayerManager::isLayerEditable(const QString& id) const {
    return m_layers.value(id).editable;
}
bool LayerManager::deleteLayer(const QString& id) {
    if (!m_layers.contains(id)) return false;
    const QStringList shapes = m_layers[id].shapeIds;
    for (const QString& sid : shapes) {
        m_shapeToLayer.remove(sid);
        emit shapeRemovedFromLayer(sid, id);
    }
    m_layers.remove(id);
    m_layerOrder.removeAll(id);
    if (m_activeLayerId == id) {
        m_activeLayerId = m_layerOrder.isEmpty() ? QString() : m_layerOrder.first();
        emit activeLayerChanged(m_activeLayerId);
    }
    emit layerDeleted(id);
    return true;
}

bool LayerManager::renameLayer(const QString& id, const QString& newName) {
    if (!m_layers.contains(id) || newName.trimmed().isEmpty()) return false;
    m_layers[id].name = newName.trimmed();
    emit layerRenamed(id, newName.trimmed());
    return true;
}

QStringList LayerManager::getLayerIds() const {
    QStringList list;
    for (const QString& id : m_layerOrder) {
        list.append(id);
    }
    return list;
}


QStringList LayerManager::getLayerNames() const {
    QStringList names;
    for (const QString& id : m_layerOrder) {
        names << m_layers[id].name;
    }
    return names;
}

LayerInfo LayerManager::getLayer(const QString& id) const {
    return m_layers.value(id, LayerInfo());
}

bool LayerManager::layerExists(const QString& id) const {
    return m_layers.contains(id);
}

QString LayerManager::findLayerIdByName(const QString& name) const {
    for (auto it = m_layers.constBegin(); it != m_layers.constEnd(); ++it) {
        if (it.value().name == name) return it.key();
    }
    return QString();
}

bool LayerManager::setLayerVisibility(const QString& id, bool visible) {
    if (!m_layers.contains(id)) return false;
    m_layers[id].visible = visible;
    emit layerVisibilityChanged(id, visible);
    return true;
}

bool LayerManager::isLayerVisible(const QString& id) const {
    return m_layers.value(id).visible;
}

bool LayerManager::addShapeToLayer(const QString& shapeId, const QString& layerId) {
    if (!m_layers.contains(layerId)) return false;
    QString oldLayer = m_shapeToLayer.value(shapeId);
    if (!oldLayer.isEmpty() && m_layers.contains(oldLayer)) {
        m_layers[oldLayer].shapeIds.removeAll(shapeId);
        emit shapeRemovedFromLayer(shapeId, oldLayer);
    }
    m_shapeToLayer[shapeId] = layerId;
    if (!m_layers[layerId].shapeIds.contains(shapeId)) {
        m_layers[layerId].shapeIds.append(shapeId);
    }
    emit shapeAssignedToLayer(shapeId, layerId);
    return true;
}

bool LayerManager::removeShapeFromLayer(const QString& shapeId) {
    QString layerId = m_shapeToLayer.value(shapeId);
    if (layerId.isEmpty()) return false;
    if (m_layers.contains(layerId)) {
        m_layers[layerId].shapeIds.removeAll(shapeId);
    }
    m_shapeToLayer.remove(shapeId);
    emit shapeRemovedFromLayer(shapeId, layerId);
    return true;
}

QString LayerManager::getLayerForShape(const QString& shapeId) const {
    return m_shapeToLayer.value(shapeId);
}

QStringList LayerManager::getShapesInLayer(const QString& layerId) const {
    return m_layers.value(layerId).shapeIds;
}

void LayerManager::clearShapesOfLayer(const QString& layerId) {
    if (!m_layers.contains(layerId)) return;
    for (const QString& sid : m_layers[layerId].shapeIds) {
        m_shapeToLayer.remove(sid);
    }
    m_layers[layerId].shapeIds.clear();
}

void LayerManager::setActiveLayer(const QString& id) {
    if (id != m_activeLayerId && (id.isEmpty() || m_layers.contains(id))) {
        m_activeLayerId = id;
        emit activeLayerChanged(id);
    }
}

QString LayerManager::getActiveLayer() const {
    return m_activeLayerId;
}
void LayerManager::clearAllLayers()
{
    const QStringList ids = getLayerIds();
    for (const QString& id : ids) {
        const QStringList shapes = m_layers.value(id).shapeIds;
        for (const QString& sid : shapes) {
            m_shapeToLayer.remove(sid);
            emit shapeRemovedFromLayer(sid, id);
        }
        m_layers.remove(id);
        m_layerOrder.removeAll(id);
        emit layerDeleted(id);
    }
    m_shapeToLayer.clear();
    m_activeLayerId.clear();
    m_counter = 0;

    const QString defaultId = addLayer(QStringLiteral("Default Layer"));
    setActiveLayer(defaultId);
}

QJsonObject LayerManager::toJson() const
{
    QJsonObject root;
    root[QStringLiteral("version")] = 1;

    QJsonArray layersArr;
    for (const QString& id : m_layerOrder) {
        if (!m_layers.contains(id))
            continue;
        const LayerInfo& info = m_layers[id];
        QJsonObject lo;
        lo[QStringLiteral("id")] = info.id;
        lo[QStringLiteral("name")] = info.name;
        lo[QStringLiteral("visible")] = info.visible;
        lo[QStringLiteral("editable")] = info.editable;
        layersArr.append(lo);
    }
    root[QStringLiteral("layers")] = layersArr;
    root[QStringLiteral("activeLayerId")] = m_activeLayerId;

    QJsonObject shapeMap;
    for (auto it = m_shapeToLayer.constBegin(); it != m_shapeToLayer.constEnd(); ++it) {
        shapeMap[it.key()] = it.value();
    }
    root[QStringLiteral("shapeToLayer")] = shapeMap;
    return root;
}

void LayerManager::fromJson(const QJsonObject& obj)
{
    if (obj.isEmpty()) {
        clearAllLayers();
        return;
    }

    // Clear existing layers completely
    {
        const QStringList ids = getLayerIds();
        for (const QString& id : ids) {
            const QStringList shapes = m_layers.value(id).shapeIds;
            for (const QString& sid : shapes) {
                m_shapeToLayer.remove(sid);
                emit shapeRemovedFromLayer(sid, id);
            }
            m_layers.remove(id);
            m_layerOrder.removeAll(id);
            emit layerDeleted(id);
        }
        m_shapeToLayer.clear();
        m_activeLayerId.clear();
    }

    QMap<QString, QString> oldIdToNewId;

    const QJsonArray layersArr = obj.value(QStringLiteral("layers")).toArray();
    if (layersArr.isEmpty()) {
        const QString defaultId = addLayer(QStringLiteral("Default Layer"));
        setActiveLayer(defaultId);
        return;
    }

    for (const QJsonValue& v : layersArr) {
        if (!v.isObject())
            continue;
        const QJsonObject lo = v.toObject();
        const QString oldId = lo.value(QStringLiteral("id")).toString();
        QString name = lo.value(QStringLiteral("name")).toString().trimmed();
        if (name.isEmpty())
            name = QStringLiteral("Layer");
        const bool visible = lo.value(QStringLiteral("visible")).toBool(true);
        const bool editable = lo.value(QStringLiteral("editable")).toBool(true);
        const QString newId = addLayer(name);
        if (!oldId.isEmpty())
            oldIdToNewId.insert(oldId, newId);
        setLayerVisibility(newId, visible);
        setLayerEditable(newId, editable);
    }

    if (m_layerOrder.isEmpty()) {
        const QString defaultId = addLayer(QStringLiteral("Default Layer"));
        setActiveLayer(defaultId);
        return;
    }

    const QString oldActive = obj.value(QStringLiteral("activeLayerId")).toString();
    QString newActive = oldIdToNewId.value(oldActive);
    if (newActive.isEmpty() || !m_layers.contains(newActive))
        newActive = m_layerOrder.first();
    setActiveLayer(newActive);
}
