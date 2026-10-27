#ifndef DYNAMICCANVAS_H
#define DYNAMICCANVAS_H
#include "editorcanvas.h"

#include "Canvas/overlay_canvas.h"
class EditorCanvas;
class DynamicCanvas :public OverlayCanvas
{
public:
    explicit DynamicCanvas(MapWidget* map, QWidget* parent = nullptr);
    void drawContent(QPainter& painter) override;
    bool handleMousePress(QMouseEvent* event) override;
    bool handleMouseMove(QMouseEvent* event) override;
    bool handleMouseRelease(QMouseEvent* event) override;
    void setImageScale(int px);
    int imageScale() const { return resoulation; }

    void setEntitiesVisible(bool visible);
    bool entitiesVisible() const { return m_showEntities; }
    void setSensorsVisible(bool visible);
    bool sensorsVisible() const { return m_showSensors; }

    void setRadiosVisible(bool visible);
    bool radiosVisible() const { return m_showRadios; }

    void setColliderVisible(bool visible);
    bool colliderVisible() const { return m_showCollider; }
    void setTrajectoriesVisible(bool visible);
    bool trajectoriesVisible() const { return m_showTrajectories; }
    void resetAtlas();
        void setEditorCanvas(EditorCanvas* editor) { m_editor = editor; }


private:
    int resoulation = 50;
    std::vector<ECS::SpritePath> m_paths;
    std::vector<QPainter::PixmapFragment> m_fragments;
    QPixmap m_pixmap;
    QPainterPath batchPath;
    bool m_showTrajectories = true;
    bool m_showEntities = true;
    bool m_showSensors = true;
    bool m_showRadios = true;
    bool m_showCollider = true;
        EditorCanvas* m_editor = nullptr; // ARTI CHANGE
};

#endif // DYNAMICCANVAS_H
