

#ifndef DESIGNTOOLBAR_H
#define DESIGNTOOLBAR_H

#include "GUI/Tacticaldisplay/canvaswidget.h"
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <core/Hierarchy/Components/transform.h>
#include <GUI/Tacticaldisplay/Gis/layerinformationdialog.h>

class StayOpenMenu : public QMenu {
public:
    explicit StayOpenMenu(QWidget* parent = nullptr) : QMenu(parent) {}
protected:
    void mouseReleaseEvent(QMouseEvent* e) override {
        QAction* action = activeAction();
        if (action && action->isEnabled()) {
            action->trigger();
        } else {
            QMenu::mouseReleaseEvent(e);
        }
    }
};

class DesignToolBar : public QToolBar {
    Q_OBJECT
public:
    explicit DesignToolBar(QWidget *parent = nullptr);
    QAction* getLoadAction() { return loadJsonAction; }
    QAction* getSaveAction() { return saveJsonAction; }
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *mapSelectLayerAction;
    QAction *selectCenterAction;
    QAction *searchPlaceAction;
    QAction *layerInfoAction;
    QAction *shapeAction;
    QAction *bitmapAction;
    QAction *selectBitmapAction;
    QAction *presetLayersAction;
    QAction* getMeasureDistanceAction() const { return measureDistanceAction; }
    // geojson function
    QAction *importGeoJsonAction;
    QAction *geoJsonLayersAction;  // NEW: Action for GeoJSON layers menu

    struct MapLayer {
        QString name;
        QString id;
        int zoomMin;
        int zoomMax;
        QString tileUrl;
        bool isCustom;
        qreal opacity;
        QString resolution;
        QString type;
    };
signals:
    void viewTriggered();
    void moveTriggered();
    void rotateTriggered();
    void scaleTriggered();
    void zoomInTriggered();
    void zoomOutTriggered();
    void gridVisibilityToggled(bool);
    void gridSnappingToggled(bool);
    void layerSelectTriggered();
    void measureDistanceTriggered();
    // void databaseTriggered();
    void gridPlaneXToggled(bool);
    void gridPlaneYToggled(bool);
    void gridPlaneZToggled(bool);
    void gridOpacityChanged(int);
    void snapGridSizeXToggled(bool);
    void snapGridSizeYToggled(bool);
    void snapGridSizeZToggled(bool);
    void snapRotateToggled(bool);
    void snapScaleToggled(bool);
    void modeChanged(TransformMode mode);
    void transformModeChanged(const Transform& mode);
    void gridPlaneXOpacityChanged(int value);
    void gridPlaneYOpacityChanged(int value);
    void gridPlaneZOpacityChanged(int value);
    void layerOptionToggled(const QString& option, bool checked);
    void searchPlaceTriggered(const QString& location);
    void searchCoordinateTriggered(double lat, double lon);
    void mapLayerChanged(const QString& layer);
    void selectCenterTriggered();
    void customMapAdded(const QString &layerName, int zoomMin, int zoomMax, const QString &tileUrl, qreal opacity);
    void shapeSelected(const QString &shape);
    void bitmapSelected(const QString &bitmap);
    void bitmapImageSelected(const QString& filePath);
    void editTrajectoryTriggered();
    void presetLayerSelected(const QString &preset);
    // geojson function
    void importGeoJsonTriggered(const QString &filePath);
    void geoJsonLayerToggled(const QString &layerName, bool visible);  // NEW: Signal for toggling GeoJSON layer
       void searchCoordinatesTriggered(double latitude, double longitude); // New signal
public slots:
    void onModeChanged(TransformMode mode);
    void onMeasureDistanceTriggered();
    void onGeoJsonLayerAdded(const QString &layerName);  // NEW: Slot to add layer to menu


    // for geojson function
private slots:
    void importGeoJson();

private:
    QAction *viewAction;
    QAction *moveAction;
    QAction *rotateAction;
    QAction *scaleAction;
    QAction *gridToggleAction;
    QAction *snappingToggleAction;
    QAction *layerSelectAction;

    QAction *measureDistanceAction;

    // QAction *databaseAction;
    QAction *gridPlaneXAction;
    QAction *gridPlaneYAction;
    QAction *gridPlaneZAction;
    QAction *snapGridSizeXAction;
    QAction *snapGridSizeYAction;
    QAction *snapGridSizeZAction;
    QAction *snapRotateAction;
    QAction *snapScaleAction;
    QAction *addCustomMapAction;
    QMap<QString, QAction*> layerActions;
    QList<LayerInformationDialog::MapLayerInfo> mapLayers;
    void createActions();
    void setupToolBar();
    void highlightAction(QAction *action);
    QPixmap withWhiteBg(const QString &iconPath);
    QAction* loadJsonAction;
    QAction* saveJsonAction;
    void onSelectBitmapImage();
    QAction *editTrajectoryAction;
    QMap<QString, QAction*> geoJsonLayerActions;  // NEW: Map for GeoJSON layer actions
};

#endif
