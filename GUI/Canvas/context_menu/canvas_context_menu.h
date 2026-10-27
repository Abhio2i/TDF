
#ifndef CANVAS_CONTEXT_MENU_H
#define CANVAS_CONTEXT_MENU_H

#include <QObject>
#include <QPoint>
#include "GUI/Canvas/canvas_action_api.h"

class OverlayCanvas;
class MapWidget;

class CanvasContextMenu : public QObject
{
    Q_OBJECT
public:
    explicit CanvasContextMenu(OverlayCanvas* canvas, MapWidget* map, QObject* parent = nullptr);
    void show(const QPoint& globalPos, const QPoint& canvasPos);

private:
    OverlayCanvas* m_canvas = nullptr;
    MapWidget*     m_map    = nullptr;
};

#endif
