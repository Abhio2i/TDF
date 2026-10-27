#ifndef DESIGNTOOLBAR_H
#define DESIGNTOOLBAR_H

#include "GUI/Tacticaldisplay/canvaswidget.h"
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QToolButton>
#include <QMap>
#include <QList>
#include <core/Hierarchy/Components/transform.h>
#include <GUI/Tacticaldisplay/Layer/layerinformationdialog.h>
#include <core/Config/scenarioconfig.h>

// %%% StayOpenMenu Class %%%
class StayOpenMenu : public QMenu {
    Q_OBJECT
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

// %%% DesignToolBar Class %%%
class DesignToolBar : public QToolBar {
    Q_OBJECT

public:
    explicit DesignToolBar(QWidget *parent = nullptr, ScenarioConfig* config = nullptr);

    QAction* getZoomInAction()           const { return zoomInAction; }
    QAction* getZoomOutAction()          const { return zoomOutAction; }
    QAction* getMapSelectLayerAction()   const { return mapSelectLayerAction; }
    QAction* getSelectCenterAction()     const { return selectCenterAction; }
    QAction* getSearchPlaceAction()      const { return searchPlaceAction; }
    QAction* getLayerInfoAction()        const { return layerInfoAction; }
    QAction* getShapeAction()            const { return shapeAction; }
    QAction* getBitmapAction()           const { return bitmapAction; }
    QAction* getSelectBitmapAction()     const { return selectBitmapAction; }
    QAction* getPresetLayersAction()     const { return presetLayersAction; }
    QAction* getCoordinateSystemAction() const { return coordinateSystemAction; }
    QAction* getLayerSelectAction()      const { return layerSelectAction; }
    QAction* getMeasureDistanceAction()  const { return measureDistanceAction; }
    QAction* getAddTrajectoryAction()    const { return addTrajectoryAction; }
    QAction* getLoadAction()             const { return loadJsonAction; }
    QAction* getSaveAction()             const { return saveJsonAction; }

    QAction* zoomInAction           = nullptr;
    QAction* zoomOutAction          = nullptr;
    QAction* selectCenterAction     = nullptr;

    // %%% Data Structures %%%
    struct MapLayer {
        QString name;
        QString id;
        int     zoomMin;
        int     zoomMax;
        QString tileUrl;
        bool    isCustom;
        qreal   opacity;
        QString resolution;
        QString type;
    };

    QAction* registerPluginAction(const QString& pluginId,
                                  QAction* action,
                                  int position = -1);
    QToolButton* registerPluginToolButton(const QString& pluginId,
                                          QAction* action,
                                          QToolButton::ToolButtonPopupMode mode = QToolButton::InstantPopup,
                                          int position = -1);
    QAction* registerPluginMenu(const QString& pluginId,
                                const QString& title,
                                QMenu* menu,
                                int position = -1);

    void unregisterPlugin(const QString& pluginId);
    QAction* getSelectionAction() const { return selectionAction; }

signals:
    void viewTriggered();
    void zoomInTriggered();
    void zoomOutTriggered();
    void layerSelectTriggered();
    void measureDistanceTriggered();
    void modeChanged(TransformMode mode);
    void layerOptionToggled(const QString& option, bool checked);
    void searchPlaceTriggered(const QString& location);
    void searchCoordinatesTriggered(double latitude, double longitude);
    void mapLayerChanged(const QString& layer);
    void selectCenterTriggered();
    void customMapAdded(const QString& layerName, int zoomMin, int zoomMax,
                        const QString& tileUrl, qreal opacity);
    void shapeSelected(const QString& shape);
    void bitmapSelected(const QString& bitmap);
    void bitmapImageSelected(const QString& filePath);
    void editTrajectoryTriggered();
    void presetLayerSelected(const QString& preset);
    void importLayerTriggered(const QString& filePath);
    void geoJsonLayerToggled(const QString& layerName, bool visible);
    void addTrajectoryTriggered();
    void coordinateSystemChanged(const QString& crsId);
    void tooltipOptionsChanged(const QSet<QString>& activeOptions);
    void selectionModeChanged(bool withGis);
    void dataSyncTriggered();

public slots:
    void onModeChanged(TransformMode mode);
    void onMeasureDistanceTriggered();
    void setTrajectoryActionEnabled(bool enabled);
    void clearSelectionModeUI();

private slots:
    void onAddBaseLayerToSelected(const QString& layerId);
    void updateMapLayersMenu();
    void importLayer();

private:
    // %%% Actions %%%
    QAction* mapSelectLayerAction   = nullptr;
    QAction* searchPlaceAction      = nullptr;
    QAction* layerInfoAction        = nullptr;
    QAction* shapeAction            = nullptr;
    QAction* bitmapAction           = nullptr;
    QAction* selectBitmapAction     = nullptr;
    QAction* presetLayersAction     = nullptr;
    QAction* coordinateSystemAction = nullptr;
    QAction* layerSelectAction      = nullptr;
    QAction* addTrajectoryAction    = nullptr;
    QAction* viewAction             = nullptr;
    QAction* moveAction             = nullptr;
    QAction* rotateAction           = nullptr;
    QAction* measureDistanceAction  = nullptr;
    QAction* editTrajectoryAction   = nullptr;
    QAction* importLayerAction      = nullptr;
    QAction* loadJsonAction         = nullptr;
    QAction* saveJsonAction         = nullptr;

    // %%% Maps & Lists %%%
    QMap<QString, QAction*>  layerActions;
    QMap<QString, QAction*>  addLayerActions;
    QMap<QString, QAction*>  geoJsonLayerActions;
    QMap<QString, QAction*>  tooltipFieldActions;
    QList<LayerInformationDialog::MapLayerInfo> mapLayers;
    QStringList              currentLayerOrder;
    QStringList              selectedBaseLayers;
    ScenarioConfig* scenarioConfig = nullptr;

    // Plugin tracking
    QMap<QString, QList<QObject*>> m_pluginOwnedObjects;

    // %%% Utility Methods %%%
    void     createActions();
    void     setupToolBar();
    void     highlightAction(QAction* action);
    QPixmap  withWhiteBg(const QString& iconPath);
    void     updateTooltipOptions();
    QAction* selectionAction = nullptr;
    QAction* selectionWithGisAction = nullptr;
    QAction* selectionWithoutGisAction = nullptr;
    QAction* dataSyncAction = nullptr;
};


#endif // DESIGNTOOLBAR_H
