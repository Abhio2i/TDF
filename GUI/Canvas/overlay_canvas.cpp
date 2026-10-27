#include "overlay_canvas.h"
#include "core/Hierarchy/ECS/Struct/mesh_component.h"
#include "core/Hierarchy/ECS/Struct/trajectory_component.h"
#include "core/Hierarchy/ECS/Struct/transform_component.h"
#include "map_widget.h"
#include "GUI/Canvas/context_menu/canvas_context_menu.h"
#include "qpainterpath.h"

OverlayCanvas::OverlayCanvas(MapWidget* map, QWidget* parent)
    : QWidget(parent)
    , m_map(map)
{
    setAttribute(Qt::WA_TranslucentBackground,true);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_contextMenu = new CanvasContextMenu(this, m_map, this);
    if (m_map) {
        connect(m_map, &MapWidget::mapChanged,
                this, QOverload<>::of(&QWidget::update));
    }
}

void OverlayCanvas::setLayerName(const QString& name)
{
    m_layerName = name;
}

QString OverlayCanvas::layerName() const
{
    return m_layerName;
}

void OverlayCanvas::Render(){
    update();
}

bool OverlayCanvas::handleMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton/* && m_isDrawingPoint*/) {
        setCursor(Qt::ArrowCursor);
        return true;
    }
    return false;
}

bool OverlayCanvas::handleMouseMove(QMouseEvent* event)
{
    Q_UNUSED(event)
    return false;
}

bool OverlayCanvas::handleMouseRelease(QMouseEvent* event)
{
    Q_UNUSED(event)
    return false;
}

void OverlayCanvas::paintEvent(QPaintEvent*)
{
    if (!m_map)
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setClipping(false);
    drawContent(p);
}

void OverlayCanvas::mousePressEvent(QMouseEvent* e)
{
    if (m_map && (m_map->isGisInteractionEnabled() || m_map->isMeasuring())) {
        e->ignore();
        return;
    }
    if (handleMousePress(e))
        e->accept();
    else
        e->ignore();
}

void OverlayCanvas::mouseMoveEvent(QMouseEvent* e)
{
    if (m_map && (m_map->isGisInteractionEnabled() || m_map->isMeasuring())) {
        e->ignore();
        return;
    }
    if (handleMouseMove(e))
        e->accept();
    else
        e->ignore();
}

void OverlayCanvas::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_map && (m_map->isGisInteractionEnabled() || m_map->isMeasuring())) {
        e->ignore();
        return;
    }
    if (handleMouseRelease(e))
        e->accept();
    else
        e->ignore();
}

void OverlayCanvas::wheelEvent(QWheelEvent* e)
{
    e->ignore();
}

// void OverlayCanvas::startDrawPointMode()
// {
//     m_isDrawingPoint = true;
//     setCursor(Qt::CrossCursor);
// }

// void OverlayCanvas::clearPoints()
// {
//     m_points.clear();
//     m_isDrawingPoint = false;
//     setCursor(Qt::ArrowCursor);
//     update();
// }

// void OverlayCanvas::addPointAt(const QPoint& canvasPos)
// {
//     if (!m_map) return;
//     QPointF geo = m_map->canvasToGeo(canvasPos);
//     double lon = geo.x();
//     double lat = geo.y();
//     m_points.append({lat, lon});
//     update();
// }

void OverlayCanvas::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_map && (m_map->isGisInteractionEnabled() || m_map->isMeasuring())) {
        event->ignore();
        return;
    }
    if (m_contextMenu) {
        m_contextMenu->show(event->globalPos(), event->pos());
    }
    event->accept();
}
