
#ifndef TACTICALDISPLAY_H
#define TACTICALDISPLAY_H

#include "core/Render/scenerenderer.h"
#include <QDockWidget>
#include <QWidget>
#include <GUI/Tacticaldisplay/canvaswidget.h>
#include <core/Hierarchy/Components/transform.h>
#include <core/Hierarchy/Components/mesh.h>
#include <QSplitter>
#include "canvaswidget.h"
#include "GUI/scene3dwidget/scene3dwidget.h"
#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include "QStackedWidget"

class TacticalDisplay : public QWidget {
    Q_OBJECT
public:
    explicit TacticalDisplay(QWidget *parent = nullptr);

public slots:
    void addMesh(QString ID, MeshData meshData);
    void removeMesh(QString ID);
    void selectedMesh(QString ID);

signals:
    void meshSelected(QString ID);

public:
    CanvasWidget *canvas;
    QDockWidget *canvasDock;
    QDockWidget *scene3dDock;
    QSplitter *splitter;

    Scene3DWidget *scene3dwidget;

    QStackedWidget* stackedWidget;
    GISlib* mapWidget;



     void setMapLayer(const QString& layerName);

    void zoomIn() {
        if (mapWidget) {
            currentZoom++;
            mapWidget->setZoom(currentZoom);
        }
    }

    void zoomOut() {
        if (mapWidget) {
            currentZoom--;
            mapWidget->setZoom(currentZoom);
        }
    }

private:
    // QSplitter *splitter;
    // Scene3DWidget *scene3dwidget;
       int currentZoom = 3;
};

#endif // TACTICALDISPLAY_H
