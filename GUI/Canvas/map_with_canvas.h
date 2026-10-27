#ifndef MAP_WITH_CANVAS_H
#define MAP_WITH_CANVAS_H

#include "Canvas/canvas_layer/dynamiccanvas.h"
#include "Canvas/canvas_layer/editorcanvas.h"
#include "Canvas/canvas_layer/staticcanvas.h"
#include "map_widget.h"
#include "overlay_canvas.h"
#include <QWidget>
#include <QList>
#include <QString>
#include <QSet>


class MapWithCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit MapWithCanvas(QWidget* parent = nullptr);
    ~MapWithCanvas() override;
    MapWidget* map() const { return m_map; }
    EditorCanvas* editorCanvas() const { return m_editorCanvas; }
    StaticCanvas* staticCanvas() const { return m_staticCanvas; }
    DynamicCanvas* dynamicCanvas() const { return m_dynamicCanvas; }
    OverlayCanvas* addCanvasLayer(const QString& name = QString());
    void removeCanvasLayer(OverlayCanvas* layer);
    QList<OverlayCanvas*> canvasLayers() const { return m_layers; }
    void toggleLayerVisibility(const QString& layer, bool visible);
    void setTooltipOptions(const QSet<QString>& options);
    void centerOnEntity(const QString& hierarchyId, bool adjustZoom = false);
    void refreshAllLayers();
protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupOverlay();
    void layoutOverlays();
    MapWidget*              m_map = nullptr;
    StaticCanvas*          m_staticCanvas  = nullptr;
    DynamicCanvas*         m_dynamicCanvas = nullptr;
    EditorCanvas*          m_editorCanvas = nullptr;
    QList<OverlayCanvas*>   m_layers;
};

#endif
