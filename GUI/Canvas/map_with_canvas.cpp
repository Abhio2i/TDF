#include "map_with_canvas.h"
#include "map_widget.h"


MapWithCanvas::MapWithCanvas(QWidget* parent)
    : QWidget(parent)
{
    m_map = new MapWidget(this);
    m_staticCanvas = new StaticCanvas(m_map, m_map);
    m_staticCanvas->setLayerName(QStringLiteral("static"));
    m_layers.append(m_staticCanvas);

    m_editorCanvas = new EditorCanvas(m_map, m_map);
    m_layers.append(m_editorCanvas);
    m_editorCanvas->raise();

    m_dynamicCanvas = new DynamicCanvas(m_map, m_map);
    m_dynamicCanvas->setLayerName(QStringLiteral("dynamic"));
    m_layers.append(m_dynamicCanvas);

    m_staticCanvas->raise();
    m_dynamicCanvas->raise();
    m_editorCanvas->raise();

    m_staticCanvas->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_dynamicCanvas->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_dynamicCanvas->setEditorCanvas(m_editorCanvas);
    connect(m_editorCanvas, &EditorCanvas::selectionChanged,
            m_dynamicCanvas, [this]() { m_dynamicCanvas->update(); },
            Qt::QueuedConnection);
    layoutOverlays();
}

MapWithCanvas::~MapWithCanvas()
{

}

void MapWithCanvas::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutOverlays();
}

void MapWithCanvas::layoutOverlays()
{
    const QRect r = rect();
    if (m_map)
        m_map->setGeometry(r);
    for (OverlayCanvas* layer : m_layers) {
        if (!layer)
            continue;
        layer->setGeometry(0, 0, r.width(), r.height());
    }
    if (m_staticCanvas)
        m_staticCanvas->raise();
    if (m_dynamicCanvas)
        m_dynamicCanvas->raise();
    if (m_editorCanvas)
        m_editorCanvas->raise();
}

void MapWithCanvas::setupOverlay()
{
    // m_editorCanvas = addCanvasLayer("PrimaryCanvas");
}

OverlayCanvas* MapWithCanvas::addCanvasLayer(const QString& name)
{
    // OverlayCanvas* layer = new OverlayCanvas(m_map, m_map);
    // if (!name.isEmpty())
    //     layer->setLayerName(name);
    // m_layers.append(layer);
    // layer->raise();
    // layoutOverlays();
    // return layer;
    return nullptr;
}

void MapWithCanvas::removeCanvasLayer(OverlayCanvas* layer)
{
//     if (!layer || !m_layers.contains(layer))
//         return;
//     if (layer == m_staticCanvas || layer == m_editorCanvas || layer == m_dynamicCanvas)
//         return;
//     m_layers.removeOne(layer);
//     layer->deleteLater();
//     layoutOverlays();
}

void MapWithCanvas::toggleLayerVisibility(const QString& layer, bool visible)
{
    if (layer == QStringLiteral("Trajectories") ||
        layer.compare(QStringLiteral("Trajectory"), Qt::CaseInsensitive) == 0) {
        if (m_staticCanvas)
            m_staticCanvas->setTrajectoriesVisible(visible);
        if (m_editorCanvas)
            m_editorCanvas->setTrajectoriesVisible(visible);
        if (m_dynamicCanvas)
            m_dynamicCanvas->setTrajectoriesVisible(visible);
    } else if (layer == QStringLiteral("Image") ||
               layer == QStringLiteral("Entities") ||
               layer.compare(QStringLiteral("Entity"), Qt::CaseInsensitive) == 0) {
        if (m_dynamicCanvas)
            m_dynamicCanvas->setEntitiesVisible(visible);
        if (m_editorCanvas)
            m_editorCanvas->setEntitiesVisible(visible);
    } else if (layer == QStringLiteral("Sensors") ||
               layer.compare(QStringLiteral("Sensor"), Qt::CaseInsensitive) == 0) {
        if (m_dynamicCanvas)
            m_dynamicCanvas->setSensorsVisible(visible);
    } else if (layer == QStringLiteral("Radio") ||
               layer == QStringLiteral("Radios")) {
        if (m_dynamicCanvas)
            m_dynamicCanvas->setRadiosVisible(visible);
    } else if (layer == QStringLiteral("Collider") ||
               layer == QStringLiteral("Colliders")) {
        if (m_dynamicCanvas)
            m_dynamicCanvas->setColliderVisible(visible);
    } else if (layer == QStringLiteral("ToolTip") ||
               layer == QStringLiteral("Tooltip") ||
               layer == QStringLiteral("Tool tip")) {
        if (m_editorCanvas)
            m_editorCanvas->setTooltipEnabled(visible);
    }
    if (m_dynamicCanvas) m_dynamicCanvas->update();
    if (m_staticCanvas)  m_staticCanvas->update();
    if (m_editorCanvas)  m_editorCanvas->update();
    if (m_map)           m_map->update();
}

void MapWithCanvas::setTooltipOptions(const QSet<QString>& options)
{
    if (m_editorCanvas)
        m_editorCanvas->setTooltipOptions(options);
}
void MapWithCanvas::centerOnEntity(const QString& hierarchyId, bool adjustZoom)
{
    Q_UNUSED(adjustZoom);
    if (hierarchyId.isEmpty() || !m_map || !m_editorCanvas)
        return;
    double lat = 0.0;
    double lon = 0.0;
    if (!m_editorCanvas->getEntityGeo(hierarchyId, lat, lon))
        return;
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
        return;
       m_map->goToLocation(lat, lon, false);
    if (m_editorCanvas)
        m_editorCanvas->update();
    if (m_dynamicCanvas)
        m_dynamicCanvas->update();
    if (m_staticCanvas)
        m_staticCanvas->update();
}

void MapWithCanvas::refreshAllLayers()
{
    if (m_map)           m_map->update();
    if (m_staticCanvas)  m_staticCanvas->update();
    if (m_dynamicCanvas) m_dynamicCanvas->update();
    if (m_editorCanvas)  m_editorCanvas->update();
}
