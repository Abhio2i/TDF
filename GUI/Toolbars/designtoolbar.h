/* ========================================================================= */
/* File: designtoolbar.h                                                    */
/* Purpose: Defines toolbar for design and map interaction                    */
/* ========================================================================= */

#ifndef DESIGNTOOLBAR_H
#define DESIGNTOOLBAR_H

#include "GUI/Tacticaldisplay/canvaswidget.h"     // For canvas widget
#include <QToolBar>                               // For toolbar base class
#include <QAction>                                // For action items
#include <QMenu>                                  // For menu widget
#include <core/Hierarchy/Components/transform.h>  // For transform component
#include <GUI/Tacticaldisplay/Gis/layerinformationdialog.h> // For layer info dialog

// %%% StayOpenMenu Class %%%
/* Menu that stays open on click */
class StayOpenMenu : public QMenu {
public:
    // Initialize menu
    explicit StayOpenMenu(QWidget* parent = nullptr) : QMenu(parent) {}

protected:
    // Handle mouse release events
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
/* Toolbar for design and map tools */
class DesignToolBar : public QToolBar {
    Q_OBJECT

public:
    // Initialize toolbar
    explicit DesignToolBar(QWidget *parent = nullptr);
    // Get load JSON action
    QAction* getLoadAction() { return loadJsonAction; }
    // Get save JSON action
    QAction* getSaveAction() { return saveJsonAction; }
    // Zoom in action
    QAction *zoomInAction;
    // Zoom out action
    QAction *zoomOutAction;
    // Map layer selection action
    QAction *mapSelectLayerAction;
    // Select center action
    QAction *selectCenterAction;
    // Search place action
    QAction *searchPlaceAction;
    // Layer info action
    QAction *layerInfoAction;
    // Shape selection action
    QAction *shapeAction;
    // Bitmap selection action
    QAction *bitmapAction;
    // Select bitmap action
    QAction *selectBitmapAction;
    // Preset layers action
    QAction *presetLayersAction;
    // Get measure distance action
    QAction* getMeasureDistanceAction() const { return measureDistanceAction; }
    // GeoJSON import action
    QAction *importGeoJsonAction;
    // GeoJSON layers action
    QAction *geoJsonLayersAction;

    // %%% Data Structures %%%
    /* Structure for map layer data */
    struct MapLayer {
        QString name;                             // Layer name
        QString id;                               // Layer ID
        int zoomMin;                              // Minimum zoom level
        int zoomMax;                              // Maximum zoom level
        QString tileUrl;                          // Tile URL
        bool isCustom;                            // Custom layer flag
        qreal opacity;                            // Layer opacity
        QString resolution;                       // Layer resolution
        QString type;                             // Layer type
    };

signals:
    // Signal view action triggered
    void viewTriggered();
    // Signal move action triggered
    void moveTriggered();
    // Signal rotate action triggered
    void rotateTriggered();
    // Signal scale action triggered
    void scaleTriggered();
    // Signal zoom in triggered
    void zoomInTriggered();
    // Signal zoom out triggered
    void zoomOutTriggered();
    // Signal grid visibility toggle
    void gridVisibilityToggled(bool);
    // Signal grid snapping toggle
    void gridSnappingToggled(bool);
    // Signal layer selection triggered
    void layerSelectTriggered();
    // Signal measure distance triggered
    void measureDistanceTriggered();
    // Signal database triggered (commented)
    // void databaseTriggered();
    // Signal grid plane X toggle
    void gridPlaneXToggled(bool);
    // Signal grid plane Y toggle
    void gridPlaneYToggled(bool);
    // Signal grid plane Z toggle
    void gridPlaneZToggled(bool);
    // Signal grid opacity change
    void gridOpacityChanged(int);
    // Signal snap grid size X toggle
    void snapGridSizeXToggled(bool);
    // Signal snap grid size Y toggle
    void snapGridSizeYToggled(bool);
    // Signal snap grid size Z toggle
    void snapGridSizeZToggled(bool);
    // Signal snap rotate toggle
    void snapRotateToggled(bool);
    // Signal snap scale toggle
    void snapScaleToggled(bool);
    // Signal mode change
    void modeChanged(TransformMode mode);
    // Signal transform mode change
    void transformModeChanged(const Transform& mode);
    // Signal grid plane X opacity change
    void gridPlaneXOpacityChanged(int value);
    // Signal grid plane Y opacity change
    void gridPlaneYOpacityChanged(int value);
    // Signal grid plane Z opacity change
    void gridPlaneZOpacityChanged(int value);
    // Signal layer option toggle
    void layerOptionToggled(const QString& option, bool checked);
    // Signal search place triggered
    void searchPlaceTriggered(const QString& location);
    // Signal search coordinate triggered
    void searchCoordinateTriggered(double lat, double lon);
    // Signal map layer change
    void mapLayerChanged(const QString& layer);
    // Signal select center triggered
    void selectCenterTriggered();
    // Signal custom map added
    void customMapAdded(const QString &layerName, int zoomMin, int zoomMax, const QString &tileUrl, qreal opacity);
    // Signal shape selection
    void shapeSelected(const QString &shape);
    // Signal bitmap selection
    void bitmapSelected(const QString &bitmap);
    // Signal bitmap image selection
    void bitmapImageSelected(const QString& filePath);
    // Signal edit trajectory triggered
    void editTrajectoryTriggered();
    // Signal preset layer selection
    void presetLayerSelected(const QString &preset);
    // Signal GeoJSON import triggered
    void importGeoJsonTriggered(const QString &filePath);
    // Signal GeoJSON layer toggle
    void geoJsonLayerToggled(const QString &layerName, bool visible);
    // Signal search coordinates triggered
    void searchCoordinatesTriggered(double latitude, double longitude);

public slots:
    // Handle mode change
    void onModeChanged(TransformMode mode);
    // Handle measure distance triggered
    void onMeasureDistanceTriggered();
    // Handle GeoJSON layer addition
    void onGeoJsonLayerAdded(const QString &layerName);

private slots:
    // Handle GeoJSON import
    void importGeoJson();

private:
    // %%% UI Components %%%
    // View action
    QAction *viewAction;
    // Move action
    QAction *moveAction;
    // Rotate action
    QAction *rotateAction;
    // Scale action
    QAction *scaleAction;
    // Grid toggle action
    QAction *gridToggleAction;
    // Snapping toggle action
    QAction *snappingToggleAction;
    // Layer selection action
    QAction *layerSelectAction;
    // Measure distance action
    QAction *measureDistanceAction;
    // Database action (commented)
    // QAction *databaseAction;
    // Grid plane X action
    QAction *gridPlaneXAction;
    // Grid plane Y action
    QAction *gridPlaneYAction;
    // Grid plane Z action
    QAction *gridPlaneZAction;
    // Snap grid size X action
    QAction *snapGridSizeXAction;
    // Snap grid size Y action
    QAction *snapGridSizeYAction;
    // Snap grid size Z action
    QAction *snapGridSizeZAction;
    // Snap rotate action
    QAction *snapRotateAction;
    // Snap scale action
    QAction *snapScaleAction;
    // Add custom map action
    QAction *addCustomMapAction;
    // Map of layer actions
    QMap<QString, QAction*> layerActions;
    // List of map layer info
    QList<LayerInformationDialog::MapLayerInfo> mapLayers;
    // Load JSON action
    QAction* loadJsonAction;
    // Save JSON action
    QAction* saveJsonAction;
    // Edit trajectory action
    QAction *editTrajectoryAction;
    // Map of GeoJSON layer actions
    QMap<QString, QAction*> geoJsonLayerActions;

    // %%% Utility Methods %%%
    // Create toolbar actions
    void createActions();
    // Setup toolbar
    void setupToolBar();
    // Highlight action
    void highlightAction(QAction *action);
    // Create pixmap with white background
    QPixmap withWhiteBg(const QString &iconPath);
    // Handle bitmap image selection
    void onSelectBitmapImage();
};

#endif
