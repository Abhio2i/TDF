#ifndef OVERLAY_CANVAS_H
#define OVERLAY_CANVAS_H

#include "core/Hierarchy/ECS/Struct/mesh_component.h"
#include "core/Hierarchy/ECS/World/world.h"
#include "qpainterpath.h"
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QVector>
#include <QPointF>


class MapWidget;
class CanvasContextMenu;

class OverlayCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayCanvas(MapWidget* map, QWidget* parent = nullptr);
    virtual ~OverlayCanvas() = default;
    void setLayerName(const QString& name);
    QString layerName() const;
    // void startDrawPointMode();
    // void clearPoints();
    void setWorld(ECS::World* world) { m_world = world; };
    MapWidget* getMap(){return m_map;};
    ECS::World* getWorld(){return m_world;};
    void Render();
    virtual void drawContent(QPainter& painter) = 0;
    virtual bool handleMousePress(QMouseEvent* event) = 0;
    virtual bool handleMouseMove(QMouseEvent* event) = 0;
    virtual bool handleMouseRelease(QMouseEvent* event) = 0;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    MapWidget* map() const { return m_map; }


private:
    // void addPointAt(const QPoint& canvasPos);
    MapWidget* m_map = nullptr;
    ECS::World* m_world = nullptr;
    QString    m_layerName;
    CanvasContextMenu* m_contextMenu = nullptr;
    // bool m_isDrawingPoint = false;
    // struct GeoPoint {
    //     double lat;
    //     double lon;
    // };
    // QVector<GeoPoint> m_points;


};

#endif
