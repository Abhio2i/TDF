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
class QComboBox;
class QLabel;

class TacticalDisplay : public QWidget {
    Q_OBJECT
public:
    explicit TacticalDisplay(QWidget *parent = nullptr);
    void show3DView();
    void hide3DView();

public slots:
    void addMesh(QString ID, MeshData meshData);
    void removeMesh(QString ID);
    void selectedMesh(QString ID);
    void setMapLayers(const QStringList& layerNames);
    void addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl, qreal opacity = 1.0);

signals:
    void meshSelected(QString ID);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
public:
    CanvasWidget *canvas;
    QDockWidget *canvasDock;
    QDockWidget *scene3dDock;
    QSplitter *splitter;
    Scene3DWidget *scene3dwidget;
    QStackedWidget* stackedWidget;
    GISlib* mapWidget;
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
    int currentZoom = 3;
    bool is3DViewVisible = false;
    QLabel *overlayLabel;
    QComboBox *crsComboBox;
};

#endif // TACTICALDISPLAY_H
