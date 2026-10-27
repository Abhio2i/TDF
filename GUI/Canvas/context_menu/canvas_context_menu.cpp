#include "canvas_context_menu.h"
#include "GUI/Canvas/overlay_canvas.h"
#include "map_widget.h"
#include <QMenu>

CanvasContextMenu::CanvasContextMenu(OverlayCanvas* canvas, MapWidget* map, QObject* parent)
    : QObject(parent)
    , m_canvas(canvas)
    , m_map(map)
{
}

void CanvasContextMenu::show(const QPoint& globalPos, const QPoint& canvasPos)
{
    QMenu menu;
    menu.setStyleSheet(R"(
        QMenu {
            background-color: #1A3652;
            color: white;
            border: 1px solid #27446d;
            border-radius: 3px;
            padding: 4px 0px;
            font-size: 13px;
        }
        QMenu::item {
            padding: 8px 28px 8px 20px;
            margin: 2px 4px;
            border-radius: 2px;
        }
        QMenu::item:selected {
            background-color: #0078D4;
        }
        QMenu::separator {
            height: 1px;
            background-color: #27446d;
            margin: 4px 10px;
        }
    )");

    CanvasActionContext ctx;
    ctx.canvas    = m_canvas;
    ctx.map       = m_map;
    ctx.parent    = m_canvas;
    ctx.canvasPos = canvasPos;
    if (m_map)
        ctx.geoPos = m_map->canvasToGeo(canvasPos);
    const auto actions = CanvasActionApi::instance()->actions();
    for (const auto& d : actions) {
        bool enabled = true;
        if (d.isEnabled)
            enabled = d.isEnabled(ctx);
        QAction* a = menu.addAction(d.label);
        a->setEnabled(enabled);
        a->setData(d.id);
    }
    QAction* selected = menu.exec(globalPos);
    if (!selected)
        return;
    const QString id = selected->data().toString();
    CanvasActionApi::instance()->trigger(id, ctx);
}
