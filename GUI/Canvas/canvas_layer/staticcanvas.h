#ifndef STATICCANVAS_H
#define STATICCANVAS_H

#include "Canvas/overlay_canvas.h"
class StaticCanvas:public OverlayCanvas
{
public:
    explicit StaticCanvas(MapWidget* map, QWidget* parent = nullptr);
    void drawContent(QPainter& painter) override;
    bool handleMousePress(QMouseEvent* event) override;
    bool handleMouseMove(QMouseEvent* event) override;
    bool handleMouseRelease(QMouseEvent* event) override;
    void setTrajectoriesVisible(bool visible);
    bool trajectoriesVisible() const { return m_showTrajectories; }

private:
    QPainterPath batchPath;
    QVector<QPointF> points;
    bool m_showTrajectories = true;
};

#endif // STATICCANVAS_H
