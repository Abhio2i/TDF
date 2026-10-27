#include "shape_history_api.h"
#include "backend/api/shape_api.h"

ShapeHistoryApi* ShapeHistoryApi::m_instance = nullptr;

ShapeHistoryApi* ShapeHistoryApi::instance()
{
    if (!m_instance)
        m_instance = new ShapeHistoryApi();
    return m_instance;
}

ShapeHistoryApi::ShapeHistoryApi(QObject* parent)
    : QObject(parent)
{
}

QJsonObject ShapeHistoryApi::stripId(const QJsonObject& obj)
{
    QJsonObject o = obj;
    o.remove(QStringLiteral("id"));
    return o;
}

bool ShapeHistoryApi::roughlyEqual(const QJsonObject& a, const QJsonObject& b)
{
    return stripId(a) == stripId(b);
}

void ShapeHistoryApi::appendSnapshot(const QString& shapeId, const QJsonObject& snapIn)
{
    if (shapeId.isEmpty() || snapIn.isEmpty())
        return;

    QJsonObject snap = stripId(snapIn);
    QVector<QJsonObject>& list = m_history[shapeId];
    if (!list.isEmpty() && roughlyEqual(list.last(), snap))
        return;
    list.append(snap);
    while (list.size() > kMaxSnapshots)
        list.removeFirst();
    emit historyChanged(shapeId);
}

void ShapeHistoryApi::pushSnapshot(const QString& shapeId)
{
    if (shapeId.isEmpty())
        return;
    QJsonObject props = ShapeApi::instance()->getShapeProperties(shapeId);
    if (props.isEmpty())
        return;
    appendSnapshot(shapeId, props);
}

void ShapeHistoryApi::pushSnapshot(const QString& shapeId, const QJsonObject& props)
{
    appendSnapshot(shapeId, props);
}

bool ShapeHistoryApi::hasHistory(const QString& shapeId) const
{
    return m_history.value(shapeId).size() > 0;
}

int ShapeHistoryApi::historyCount(const QString& shapeId) const
{
    return m_history.value(shapeId).size();
}

QJsonObject ShapeHistoryApi::lastSnapshot(const QString& shapeId) const
{
    const QVector<QJsonObject> list = m_history.value(shapeId);
    if (list.isEmpty())
        return {};
    return list.last();
}

void ShapeHistoryApi::clear(const QString& shapeId)
{
    if (m_previewId == shapeId)
        hidePreview();
    if (m_history.remove(shapeId) > 0)
        emit historyChanged(shapeId);
}

void ShapeHistoryApi::clearAll()
{
    hidePreview();
    if (m_history.isEmpty())
        return;
    m_history.clear();
    emit historyChanged(QString());
}

bool ShapeHistoryApi::isPreviewActive() const
{
    return !m_previewId.isEmpty() && !m_previewGeom.isEmpty();
}

QString ShapeHistoryApi::previewShapeId() const
{
    return m_previewId;
}

QJsonObject ShapeHistoryApi::previewGeometry() const
{
    return m_previewGeom;
}

bool ShapeHistoryApi::startPreview(const QString& shapeId)
{
    if (shapeId.isEmpty() || !hasHistory(shapeId))
        return false;

    m_previewId = shapeId;
    m_previewGeom = lastSnapshot(shapeId);
    emit previewChanged(true, shapeId);
    return true;
}

void ShapeHistoryApi::hidePreview()
{
    if (m_previewId.isEmpty() && m_previewGeom.isEmpty())
        return;
    const QString id = m_previewId;
    m_previewId.clear();
    m_previewGeom = QJsonObject();
    emit previewChanged(false, id);
}

bool ShapeHistoryApi::restoreHistory(const QString& shapeId)
{
    if (shapeId.isEmpty() || !hasHistory(shapeId))
        return false;

    if (m_previewId == shapeId || !m_previewId.isEmpty()) {
        const QString pid = m_previewId;
        m_previewId.clear();
        m_previewGeom = QJsonObject();
        emit previewChanged(false, pid);
    }

    QVector<QJsonObject>& list = m_history[shapeId];
    if (list.isEmpty())
        return false;

    QJsonObject snap = list.takeLast();
    ShapeApi::instance()->updateShape(shapeId, snap);

    if (list.isEmpty())
        m_history.remove(shapeId);

    emit historyChanged(shapeId);
    return true;
}
