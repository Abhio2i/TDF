/* =============================================================================
 * FILE:         designtoolbar.cpp
 * MODULE:       Design Toolbar
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the DesignToolBar class which provides a toolbar for
 *               design and map interaction. Includes actions for view, move,
 *               rotate, scale, zoom, grid control, snapping, measure distance,
 *               map layer management (preset, custom, GeoJSON), coordinate
 *               system selection, tooltip options, and trajectory editing.
 *               Also defines StayOpenMenu, a custom QMenu that stays open
 *               when an action is clicked.
 *
 *
 * REQUIREMENTS: Implements REQ-DESIGNTOOLBAR-010 through REQ-DESIGNTOOLBAR-018
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-DESIGNTOOLBAR-001
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "GUI/Toolbars/designtoolbar.h"
#include "designtoolbar-styles.h"
#include "GUI/Tacticaldisplay/Gis/custommapdialog.h"
#include "GUI/Tacticaldisplay/Layer/layerinformationdialog.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "GUI/Editors/recentprojectsmanager.h"
#include "GUI/Tacticaldisplay/tooltiphelper.h"
#include "core/Debug/console.h"
#include <QLineEdit>
#include <QIcon>
#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include <QMenu>
#include <QToolButton>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QCheckBox>
#include <QActionGroup>
#include <QDialog>
#include <QFileDialog>
#include <QPushButton>
#include <QHBoxLayout>
#include <qgsvectorlayer.h>
#include <qgsproject.h>
const QSize ICON_SIZE(20, 20);

// Utility function
QPixmap DesignToolBar::withWhiteBg(const QString &iconPath) {
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) return QPixmap();
    QPixmap newPixmap(pixmap.size());
    newPixmap.fill(Qt::gray);
    QPainter painter(&newPixmap);
    painter.drawPixmap(0, 0, pixmap);
    painter.end();
    return newPixmap;
}

DesignToolBar::DesignToolBar(QWidget *parent, ScenarioConfig* config)
    : QToolBar(parent), scenarioConfig(config) {
    setWindowTitle("Design ToolBar");
    setStyleSheet(DesignToolbarStyles::Toolbar);
    setIconSize(ICON_SIZE);
    createActions();
    setupToolBar();
    onModeChanged(Translate);
}

// Creates all toolbar actions, menus, and signal connections
void DesignToolBar::createActions() {
    mapLayers = {
        { "OpenStreetMap", "osm", 0, 9, "", false, 1.0, "N/A", "Raster" },
        { "Satellite Map", "satellite", 0, 9, "", true, 1.0, "N/A", "Raster" },
        { "Terrain Map", "terrain", 0, 9, "", true, 1.0, "N/A", "Raster" }
    };
    selectedBaseLayers.clear();
    selectedBaseLayers.append("osm");
    currentLayerOrder.clear();
    currentLayerOrder.append("osm");
    mapSelectLayerAction = new QAction(QIcon(withWhiteBg(":/icons/images/map.png")), tr("Map Layer"), this);
    mapSelectLayerAction->setCheckable(true);
    StayOpenMenu* mapLayerMenu = new StayOpenMenu(this);
    mapLayerMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);

    // ========== SECTION 1: Available Base Map Layers ==========
    QLabel* baseLayersLabel = new QLabel("  Available Base Map Layers");
    baseLayersLabel->setStyleSheet(DesignToolbarStyles::MenuLabel);
    QWidgetAction* baseLabelAction = new QWidgetAction(this);
    baseLabelAction->setDefaultWidget(baseLayersLabel);
    mapLayerMenu->addAction(baseLabelAction);
    addLayerActions.clear();
    for (const auto& layer : mapLayers) {
        QWidget* layerWidget = new QWidget();
        layerWidget->setStyleSheet("background-color: #1A3652;");
        QHBoxLayout* layout = new QHBoxLayout(layerWidget);
        layout->setContentsMargins(10, 2, 10, 2);
        layout->setSpacing(10);
        // Layer name label
        QLabel* nameLabel = new QLabel(layer.name);
        nameLabel->setMinimumWidth(150);
        nameLabel->setStyleSheet("color: white; background-color: transparent;");
        layout->addWidget(nameLabel);
        layout->addStretch();
        // "+" button to add layer
        QPushButton* addButton = new QPushButton("+");
        addButton->setFixedSize(25, 25);
        addButton->setStyleSheet(DesignToolbarStyles::PushButton);
        // Disable button if layer is already selected (OSM starts disabled)
        if (selectedBaseLayers.contains(layer.id)) {
            addButton->setEnabled(false);
        }
        // Capture layer.id by value
        QString capturedLayerId = layer.id;
        connect(addButton, &QPushButton::clicked, this, [this, capturedLayerId]() {
            onAddBaseLayerToSelected(capturedLayerId);
        });
        layout->addWidget(addButton);
        layerWidget->setLayout(layout);
        // Add widget to menu
        QWidgetAction* widgetAction = new QWidgetAction(this);
        widgetAction->setDefaultWidget(layerWidget);
        mapLayerMenu->addAction(widgetAction);
        // Store the add button for later enable/disable control
        addLayerActions[layer.id] = widgetAction;
        // Initialize custom maps
        if (layer.isCustom) {
            emit customMapAdded(layer.name, layer.zoomMin, layer.zoomMax,
                                layer.tileUrl, layer.opacity);
        }
    }

    mapLayerMenu->addSeparator();
    // ========== SECTION 2: Selected Layers ==========
    QLabel* selectedLayersLabel = new QLabel("  Selected Layers");
    selectedLayersLabel->setStyleSheet(DesignToolbarStyles::MenuLabel);
    QWidgetAction* selectedLabelAction = new QWidgetAction(this);
    selectedLabelAction->setDefaultWidget(selectedLayersLabel);
    mapLayerMenu->addAction(selectedLabelAction);

    // Initialize with OpenStreetMap as default
    layerActions.clear();
    QActionGroup* layerGroup = new QActionGroup(this);
    layerGroup->setExclusive(false);

    // Add OpenStreetMap as the default selected layer
    for (const auto& layer : mapLayers) {
        if (layer.id == "osm") {
            QAction* action = new QAction(layer.name + " (by default)", this);
            action->setCheckable(true);
            action->setChecked(true);
            action->setData(layer.id);
            mapLayerMenu->addAction(action);
            layerGroup->addAction(action);
            layerActions[layer.id] = action;

            // Connection for visibility toggle
            connect(action, &QAction::triggered, this, [=](bool checked) {
                QString layerId = action->data().toString();
                if (checked) {
                    // ACTIVATING: Move this layer to the front (top of rendering)
                    currentLayerOrder.removeAll(layerId);
                    currentLayerOrder.prepend(layerId);
                } else {
                    // DEACTIVATING: Just remove from order list
                    currentLayerOrder.removeAll(layerId);
                }

                // Build the active layers list based on the current order
                QStringList activeLayers;
                for (const QString& orderedLayer : currentLayerOrder) {
                    // Only include layers that are actually checked
                    if (layerActions.contains(orderedLayer) &&
                        layerActions[orderedLayer]->isChecked()) {
                        activeLayers.append(orderedLayer);
                    }
                }

                // Emit the properly ordered active layers
                emit mapLayerChanged(activeLayers.join(","));
            });

            break;
        }
    }
    mapSelectLayerAction->setMenu(mapLayerMenu);
    // Main view mode action - for panning and navigation
    viewAction = new QAction(QIcon(withWhiteBg(":/icons/images/view.jpg")), tr("View"), this);
    viewAction->setCheckable(true);
    viewAction->setShortcut(QKeySequence(Qt::Key_1));
    connect(viewAction, &QAction::triggered, this, [=]() {
        highlightAction(viewAction);
        emit modeChanged(Panning);
        emit viewTriggered();
    });

    // Main move action - for translating objects
    moveAction = new QAction(QIcon(withWhiteBg(":/icons/images/move.png")), tr("Move"), this);
    moveAction->setCheckable(true);
    moveAction->setShortcut(QKeySequence(Qt::Key_2));
    connect(moveAction, &QAction::triggered, this, [=]() {
        highlightAction(moveAction);
        emit modeChanged(Translate);
    });

    // Main rotate action - for rotating objects
    rotateAction = new QAction(QIcon(withWhiteBg(":/icons/images/rotate.png")), tr("Rotate"), this);
    rotateAction->setCheckable(true);
    rotateAction->setShortcut(QKeySequence(Qt::Key_3));
    connect(rotateAction, &QAction::triggered, this, [=]() {
        highlightAction(rotateAction);
        emit modeChanged(Rotate);
    });
    // Main zoom in action
    zoomInAction = new QAction(QIcon(withWhiteBg(":/icons/images/zoom-in.png")), tr("Zoom In"), this);
    zoomInAction->setCheckable(false);
    connect(zoomInAction, &QAction::triggered, this, [=]() {
        highlightAction(zoomInAction);
        emit zoomInTriggered();
    });

    // Main zoom out action
    zoomOutAction = new QAction(QIcon(withWhiteBg(":/icons/images/zoom-out.png")), tr("Zoom Out"), this);
    zoomOutAction->setCheckable(false);
    connect(zoomOutAction, &QAction::triggered, this, [=]() {
        highlightAction(zoomOutAction);
        emit zoomOutTriggered();
    });

    // Main center selection action for map centering
    selectCenterAction = new QAction(QIcon(withWhiteBg(":/icons/images/centremap.png")), tr("Select Center"), this);
    selectCenterAction->setCheckable(true);
    connect(selectCenterAction, &QAction::triggered, this, [=]() {
        highlightAction(selectCenterAction);
        emit selectCenterTriggered();
    });

    // Main search action with input field menu
    searchPlaceAction = new QAction(QIcon(withWhiteBg(":/icons/images/search.png")), tr("Search Place"), this);
    searchPlaceAction->setCheckable(true);

    // Main layer selection action with visualization options
    layerSelectAction = new QAction(QIcon(withWhiteBg(":/icons/images/layers.png")), tr("Select Layer"), this);
    layerSelectAction->setCheckable(true);

    StayOpenMenu* layerMenu = new StayOpenMenu(this);
    layerMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);

    QAction* tooltipAction = new QAction("ToolTip", this);
    // Create tooltip action with submenu
    tooltipAction = new QAction("ToolTip", this);
    tooltipAction->setCheckable(true);
    tooltipAction->setChecked(true);

    // Create submenu for tooltip options
    StayOpenMenu* tooltipOptionsMenu = new StayOpenMenu(this);
    tooltipOptionsMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);

    // Get available fields from TooltipHelper
    QStringList availableFields = TooltipHelper::getAvailableFields();
    tooltipFieldActions.clear();
    // LOAD SAVED SETTINGS FIRST
    QSet<QString> savedFields;
    if (scenarioConfig) {
        savedFields = scenarioConfig->loadTooltipFields();
    }

    for (const QString& field : availableFields) {
        QAction* fieldAction = new QAction(field, this);
        fieldAction->setCheckable(true);

        // Set checked state from saved settings or defaults
        if (scenarioConfig && !savedFields.isEmpty()) {
            fieldAction->setChecked(savedFields.contains(field));
        } else {
            // Use default checked state (first time or no config)
            if (field == "Name" || field == "Speed" || field == "Altitude" ||
                field == "Latitude" || field == "Longitude") {
                fieldAction->setChecked(true);
            }
        }
        tooltipOptionsMenu->addAction(fieldAction);
        tooltipFieldActions[field] = fieldAction;

        // Connect to update tooltip options
        connect(fieldAction, &QAction::triggered, this, [=]() {
            updateTooltipOptions();
        });
        tooltipAction->setMenu(tooltipOptionsMenu);
    }

    // Emit initial state
    updateTooltipOptions();
    QAction* colliderAction = new QAction("Collider", this);
    // QAction* meshAction = new QAction("Mesh", this);
    // QAction* outlineAction = new QAction("Outline", this);
    // QAction* informationAction = new QAction("Information", this);
    // QAction* fpsAction = new QAction("FPS", this);
    QAction* imageAction = new QAction("Entities", this);
    QAction* sensorsAction = new QAction("Sensors", this);
    QAction* radioAction = new QAction("Radio", this);
    QAction* trajectoriesAction = new QAction("Trajectories", this);
    tooltipAction->setCheckable(true);
    tooltipAction->setChecked(false);
    colliderAction->setCheckable(true);
    colliderAction->setChecked(true);
    // meshAction->setCheckable(true);
    // meshAction->setChecked(true);
    // outlineAction->setCheckable(true);
    // outlineAction->setChecked(true);
    // informationAction->setCheckable(true);
    // informationAction->setChecked(false);
    // fpsAction->setCheckable(true);
    // fpsAction->setChecked(true);
    imageAction->setCheckable(true);
    imageAction->setChecked(true);
    sensorsAction->setCheckable(true);
    sensorsAction->setChecked(true);
    radioAction->setCheckable(true);
    radioAction->setChecked(true);
    trajectoriesAction->setCheckable(true);
    trajectoriesAction->setChecked(true);
    layerMenu->addAction(tooltipAction);
    layerMenu->addAction(colliderAction);
    // layerMenu->addAction(meshAction);
    // layerMenu->addAction(outlineAction);
    // layerMenu->addAction(informationAction);
    // layerMenu->addAction(fpsAction);
    layerMenu->addAction(imageAction);
    layerMenu->addSeparator();
    layerMenu->addAction(sensorsAction);
    layerMenu->addAction(radioAction);
    layerMenu->addAction(trajectoriesAction);
    layerSelectAction->setMenu(layerMenu);
    connect(tooltipAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("ToolTip", checked);
        if (checked) {
            updateTooltipOptions();
        }
    });
    connect(colliderAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Collider", checked);
    });
    // connect(meshAction, &QAction::triggered, this, [=](bool checked) {
    //     emit layerOptionToggled("Mesh", checked);
    // });
    // connect(outlineAction, &QAction::triggered, this, [=](bool checked) {
    //     emit layerOptionToggled("Outline", checked);
    // });
    // // connect(informationAction, &QAction::triggered, this, [=](bool checked) {
    // //     emit layerOptionToggled("Information", checked);
    // // });
    // connect(fpsAction, &QAction::triggered, this, [=](bool checked) {
    //     emit layerOptionToggled("FPS", checked);
    // });
    connect(imageAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Image", checked);
    });
    connect(sensorsAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Sensors", checked);
    });
    connect(radioAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Radio", checked);
    });
    connect(trajectoriesAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Trajectories", checked);
    });

    // Main measurement tool action
    measureDistanceAction = new QAction(QIcon(withWhiteBg(":/icons/images/measurement.png")), tr("Measure Distance"), this);
    measureDistanceAction->setCheckable(true);
    connect(measureDistanceAction, &QAction::triggered, this, [=]() {
        highlightAction(measureDistanceAction);
        emit measureDistanceTriggered();
    });

    // Main GeoJSON import action
    importLayerAction = new QAction(QIcon(withWhiteBg(":/icons/images/qgislayer.png")), tr("Import Layer"), this);
    importLayerAction->setCheckable(false);
    connect(importLayerAction, &QAction::triggered, this, &DesignToolBar::importLayer);
    // Main GeoJSON layers management action
    // geoJsonLayersAction = new QAction(QIcon(withWhiteBg(":/icons/images/geojson-layers.png")), tr("GeoJSON Layers"), this);
    // geoJsonLayersAction->setCheckable(true);
    // StayOpenMenu* geoJsonMenu = new StayOpenMenu(this);
    // geoJsonMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);
    // geoJsonLayersAction->setMenu(geoJsonMenu);

    // Main preset layers action
    presetLayersAction = new QAction(QIcon(withWhiteBg(":/icons/images/preset.png")), tr("Preset Layers"), this);
    presetLayersAction->setCheckable(true);
    StayOpenMenu* presetLayersMenu = new StayOpenMenu(this);
    presetLayersMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);
    QAction* airbaseAction = new QAction("Airbase", this);
    airbaseAction->setCheckable(true);
    presetLayersMenu->addAction(airbaseAction);
    presetLayersAction->setMenu(presetLayersMenu);
    connect(airbaseAction, &QAction::triggered, this, [=](bool checked) {
        highlightAction(presetLayersAction);
        emit presetLayerSelected("Airbase");
    });

    editTrajectoryAction = new QAction(QIcon(withWhiteBg(":/icons/images/edit-trajectory.png")), tr("Edit Trajectory"), this);
    editTrajectoryAction->setCheckable(true);
    connect(editTrajectoryAction, &QAction::triggered, this, [=]() {
        highlightAction(editTrajectoryAction);
        emit modeChanged(DrawTrajectory);
        emit editTrajectoryTriggered();
    });

    // Main trajectory editing actions
    addTrajectoryAction = new QAction(QIcon(withWhiteBg(":/icons/images/trajectory.png")), tr("Add Trajectory"), this);
    addTrajectoryAction->setCheckable(true);
    addTrajectoryAction->setEnabled(false);
    connect(addTrajectoryAction, &QAction::triggered, this, [=]() {
        highlightAction(addTrajectoryAction);
        emit modeChanged(DrawTrajectory);
        emit addTrajectoryTriggered();
    });

    // Main layer information action
    layerInfoAction = new QAction(QIcon(withWhiteBg(":/icons/images/info.png")), tr("Layer Information"), this);
    layerInfoAction->setCheckable(false);
    connect(layerInfoAction, &QAction::triggered, this, [=]() {
        LayerInformationDialog dialog(mapLayers, this);
        dialog.setStyleSheet(DesignToolbarStyles::Dialog);
        connect(&dialog, &LayerInformationDialog::layerEdited, this, [=](int index, const LayerInformationDialog::MapLayerInfo& updatedLayer) {
            if (index >= 0 && index < mapLayers.size()) {
                QString oldName = mapLayers[index].name;
                mapLayers[index] = updatedLayer;
                for (QAction* action : layerActions) {
                    if (action->data().toString() == oldName) {
                        action->setText(updatedLayer.name);
                        action->setData(updatedLayer.id);
                        break;
                    }
                }
                QStringList activeLayers;
                QStringList activeLayerNames;
                for (const auto& act : std::as_const(layerActions)) {
                    if (act->isChecked()) {
                        activeLayers.append(act->data().toString());
                        activeLayerNames.append(act->text());
                    }
                }
                emit customMapAdded(updatedLayer.name, updatedLayer.zoomMin, updatedLayer.zoomMax, updatedLayer.tileUrl, updatedLayer.opacity);
                emit mapLayerChanged(activeLayers.join(","));
            }
        });
        dialog.exec();
    });

    // Main shape drawing action with shape type menu
    shapeAction = new QAction(QIcon(withWhiteBg(":/icons/images/shapes.png")), tr("Shape"), this);
    shapeAction->setCheckable(true);
    StayOpenMenu* shapeMenu = new StayOpenMenu(this);
    shapeMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);
    QAction* drawLineAction = new QAction("Line", this);
    QAction* drawCircleAction = new QAction("Circle", this);
    QAction* drawRectangleAction = new QAction("Rectangle", this);
    QAction* drawPolygonAction = new QAction("Polygon", this);
    QAction* drawPointsAction = new QAction("Points", this);
    drawLineAction->setCheckable(true);
    drawCircleAction->setCheckable(true);
    drawRectangleAction->setCheckable(true);
    drawPolygonAction->setCheckable(true);
    drawPointsAction->setCheckable(true);
    shapeMenu->addAction(drawLineAction);
    shapeMenu->addAction(drawCircleAction);
    shapeMenu->addAction(drawRectangleAction);
    shapeMenu->addAction(drawPolygonAction);
    shapeMenu->addAction(drawPointsAction);
    shapeAction->setMenu(shapeMenu);
    QActionGroup* shapeGroup = new QActionGroup(this);
    shapeGroup->setExclusive(true);
    shapeGroup->addAction(drawLineAction);
    shapeGroup->addAction(drawCircleAction);
    shapeGroup->addAction(drawRectangleAction);
    shapeGroup->addAction(drawPolygonAction);
    shapeGroup->addAction(drawPointsAction);
    connect(drawLineAction, &QAction::triggered, this, [=]() {
        highlightAction(shapeAction);
        emit shapeSelected("Line");
    });
    connect(drawCircleAction, &QAction::triggered, this, [=]() {
        highlightAction(shapeAction);
        emit shapeSelected("Circle");
    });
    connect(drawRectangleAction, &QAction::triggered, this, [=]() {
        highlightAction(shapeAction);
        emit shapeSelected("Rectangle");
    });
    connect(drawPolygonAction, &QAction::triggered, this, [=]() {
        highlightAction(shapeAction);
        emit shapeSelected("Polygon");
    });
    connect(drawPointsAction, &QAction::triggered, this, [=]() {
        highlightAction(shapeAction);
        emit shapeSelected("Points");
    });

    // Main bitmap symbols action
    bitmapAction = new QAction(QIcon(withWhiteBg(":/icons/images/photo.png")), tr("Bitmaps"), this);
    bitmapAction->setCheckable(true);
    StayOpenMenu* bitmapMenu = new StayOpenMenu(this);
    bitmapMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);
    QAction* hospitalAction = new QAction(QIcon(withWhiteBg(":/icons/images/hospital.png")), "Hospital", this);
    QAction* schoolAction = new QAction(QIcon(withWhiteBg(":/icons/images/school.png")), "School", this);
    QAction* forestAreaAction = new QAction(QIcon(withWhiteBg(":/icons/images/forest-area.png")), "Forest Area", this);
    hospitalAction->setCheckable(true);
    schoolAction->setCheckable(true);
    forestAreaAction->setCheckable(true);
    bitmapMenu->addAction(hospitalAction);
    bitmapMenu->addAction(schoolAction);
    bitmapMenu->addAction(forestAreaAction);
    bitmapAction->setMenu(bitmapMenu);
    QActionGroup* bitmapGroup = new QActionGroup(this);
    bitmapGroup->setExclusive(true);
    bitmapGroup->addAction(hospitalAction);
    bitmapGroup->addAction(schoolAction);
    bitmapGroup->addAction(forestAreaAction);
    connect(hospitalAction, &QAction::triggered, this, [=]() {
        highlightAction(bitmapAction);
        emit bitmapSelected("Hospital");
    });
    connect(schoolAction, &QAction::triggered, this, [=]() {
        highlightAction(bitmapAction);
        emit bitmapSelected("School");
    });
    connect(forestAreaAction, &QAction::triggered, this, [=]() {
        highlightAction(bitmapAction);
        emit bitmapSelected("Forest Area");
    });

    // Main coordinate system selection action
    coordinateSystemAction = new QAction(QIcon(withWhiteBg(":/icons/images/coordinate-system.png")), tr("Coordinate System"), this);
    coordinateSystemAction->setCheckable(true);
    StayOpenMenu* coordMenu = new StayOpenMenu(this);
    coordMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);
    QActionGroup* coordGroup = new QActionGroup(this);
    coordGroup->setExclusive(true);
    QAction* latLonAction = new QAction("Geo-detic", this);
    QAction* utmAction = new QAction("UTM", this);
    QAction* mgrsAction = new QAction("MGRS", this);
    latLonAction->setCheckable(true);
    utmAction->setCheckable(true);
    mgrsAction->setCheckable(true);
    latLonAction->setChecked(true);
    latLonAction->setData("EPSG:4326");
    utmAction->setData("UTM_AUTO");
    mgrsAction->setData("MGRS");
    coordGroup->addAction(latLonAction);
    coordGroup->addAction(utmAction);
    coordGroup->addAction(mgrsAction);
    coordMenu->addAction(latLonAction);
    coordMenu->addAction(utmAction);
    coordMenu->addAction(mgrsAction);
    coordinateSystemAction->setMenu(coordMenu);

    // Connect coordinate system changes
    connect(latLonAction, &QAction::triggered, this, [=]() {
        highlightAction(coordinateSystemAction);
        emit coordinateSystemChanged("EPSG:4326");
    });

    connect(utmAction, &QAction::triggered, this, [=]() {
        highlightAction(coordinateSystemAction);
        emit coordinateSystemChanged("UTM_AUTO");
    });

    // Connect MGRS action
    connect(mgrsAction, &QAction::triggered, this, [=]() {
        highlightAction(coordinateSystemAction);
        emit coordinateSystemChanged("MGRS");
    });

    // Main bitmap image selection action
    selectBitmapAction = new QAction(QIcon(withWhiteBg(":/icons/images/picture.png")), tr("Select Bitmap Image"), this);
    selectBitmapAction->setCheckable(false);
    connect(selectBitmapAction, &QAction::triggered, this, [=]() {
        QFileDialog fileDialog(this, tr("Select Bitmap Image"));
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        fileDialog.setNameFilter(tr("Images (*.png *.jpg *.jpeg *.bmp)"));
        fileDialog.setStyleSheet(DesignToolbarStyles::FileDialog);
        if (fileDialog.exec() == QDialog::Accepted) {
            QString selectedFile = fileDialog.selectedFiles().first();
            if (!selectedFile.isEmpty()) {
                emit bitmapImageSelected(selectedFile);
                highlightAction(selectBitmapAction);
            }
        }
    });


    // ========== Selection (With GIS / Without GIS) ==========
    selectionAction = new QAction(QIcon(withWhiteBg(":/icons/images/selection.png")), tr("Selection"), this);
    selectionAction->setCheckable(true);

    QMenu* selectionMenu = new QMenu(this);
    selectionMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);

    selectionWithGisAction = new QAction(tr("Selection with GIS"), this);
    selectionWithGisAction->setCheckable(true);
    selectionWithoutGisAction = new QAction(tr("Selection without GIS"), this);
    selectionWithoutGisAction->setCheckable(true);

    // Exclusive OFF → user same option dubara click karke uncheck kar sake
    QActionGroup* selectionGroup = new QActionGroup(this);
    selectionGroup->setExclusive(false);
    selectionGroup->addAction(selectionWithGisAction);
    selectionGroup->addAction(selectionWithoutGisAction);

    selectionMenu->addAction(selectionWithGisAction);
    selectionMenu->addAction(selectionWithoutGisAction);
    selectionAction->setMenu(selectionMenu);

    connect(selectionWithGisAction, &QAction::triggered, this, [=](bool checked) {
        if (checked) {
            selectionWithoutGisAction->setChecked(false);
            highlightAction(selectionAction);
            selectionAction->setChecked(true);
            emit selectionModeChanged(true);
            emit modeChanged(Translate);
        } else {
            if (!selectionWithoutGisAction->isChecked()) {
                clearSelectionModeUI();
                emit modeChanged(Panning);
            }
        }
    });

    connect(selectionWithoutGisAction, &QAction::triggered, this, [=](bool checked) {
        if (checked) {
            selectionWithGisAction->setChecked(false);
            highlightAction(selectionAction);
            selectionAction->setChecked(true);
            emit selectionModeChanged(false);
            emit modeChanged(Translate);
        } else {
            if (!selectionWithGisAction->isChecked()) {
                clearSelectionModeUI();
                emit modeChanged(Panning);
            }
        }
    });


    dataSyncAction = new QAction(QIcon(withWhiteBg(":/icons/images/data-sync.png")),
                                 tr("Data Sync"), this);
    dataSyncAction->setCheckable(false);
    connect(dataSyncAction, &QAction::triggered, this, [=]() {
        highlightAction(dataSyncAction);
        emit dataSyncTriggered();
    });


    // Main search menu setup with coordinate parsing
    QMenu* searchMenu = new QMenu(this);
    searchMenu->setStyleSheet(DesignToolbarStyles::Menu);

    QWidgetAction* searchAction = new QWidgetAction(this);
    QLineEdit* searchInput = new QLineEdit();
    searchInput->setPlaceholderText("Enter coordinates (lat,lon)...");
    searchInput->setMinimumWidth(200);
    searchInput->setStyleSheet(DesignToolbarStyles::LineEdit);
    connect(searchInput, &QLineEdit::returnPressed, [this, searchInput]() {
        QString input = searchInput->text().trimmed();
        QRegExp coordRegex("^[-]?\\d*\\.?\\d+,[-]?\\d*\\.?\\d+$");
        if (coordRegex.exactMatch(input)) {
            QStringList coords = input.split(",");
            bool latOk, lonOk;
            double lat = coords[0].toDouble(&latOk);
            double lon = coords[1].toDouble(&lonOk);
            if (latOk && lonOk && lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180) {
                emit searchCoordinatesTriggered(lat, lon);
            } else {
                emit searchPlaceTriggered(input);
            }
        } else {
            emit searchPlaceTriggered(input);
        }
        searchInput->clear();
    });
    searchAction->setDefaultWidget(searchInput);
    searchMenu->addAction(searchAction);
    searchPlaceAction->setMenu(searchMenu);
}

// Main highlighting method: Visual feedback for active toolbar actions
void DesignToolBar::highlightAction(QAction *activeAction) {
    QList<QAction*> actions = {
        viewAction, moveAction, rotateAction, /*scaleAction,*/
        zoomInAction, zoomOutAction,
        layerSelectAction,
        editTrajectoryAction,
        coordinateSystemAction,
        addTrajectoryAction,
        mapSelectLayerAction, searchPlaceAction,
        selectCenterAction, layerInfoAction,
        shapeAction, bitmapAction, selectBitmapAction,
        measureDistanceAction, presetLayersAction,
        selectionAction
        // geoJsonLayersAction
    };

    for (QAction *action : actions) {
        QWidget *btn = widgetForAction(action);
        if (!btn) continue;
        if (action == activeAction) {
            btn->setStyleSheet(DesignToolbarStyles::ToolbarButtonHighlighted);
        } else {
            btn->setStyleSheet(DesignToolbarStyles::ToolbarButton);
            if (action) {
                action->setChecked(false);
            }
        }
    }
}

// Main toolbar setup method: Arranges all actions in logical groups with separators
void DesignToolBar::setupToolBar()
{
    // Apply button style to all existing buttons
    for (QAction* action : actions()) {
        QWidget* btn = widgetForAction(action);
        if (btn) {
            btn->setStyleSheet(DesignToolbarStyles::ToolbarButton);
        }
    }

    // Main transformation tools section
    addAction(viewAction);
    addAction(moveAction);
    addAction(rotateAction);
    // addSeparator();

    // Layer and trajectory section
    QToolButton *layerButton = new QToolButton(this);
    layerButton->setDefaultAction(layerSelectAction);
    layerButton->setPopupMode(QToolButton::InstantPopup);
    layerButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    addWidget(layerButton);

    // QToolButton *addTrajectoryButton = new QToolButton(this);
    // addTrajectoryButton->setDefaultAction(addTrajectoryAction);
    // addTrajectoryButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(addTrajectoryButton);
    // addSeparator();

    // // Zoom and info section
    // addAction(zoomInAction);
    // addAction(zoomOutAction);
    // // addAction(layerInfoAction);
    // addAction(selectCenterAction);

    // QToolButton *coordSystemButton = new QToolButton(this);
    // coordSystemButton->setDefaultAction(coordinateSystemAction);
    // coordSystemButton->setPopupMode(QToolButton::InstantPopup);
    // coordSystemButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(coordSystemButton);

    // QToolButton *mapLayerButton = new QToolButton(this);
    // mapLayerButton->setDefaultAction(mapSelectLayerAction);
    // mapLayerButton->setPopupMode(QToolButton::InstantPopup);
    // mapLayerButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(mapLayerButton);
    // QToolButton *importLayerButton = new QToolButton(this);
    // importLayerButton->setDefaultAction(importLayerAction);
    // importLayerButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(importLayerButton);

    // // QToolButton *geoJsonLayersButton = new QToolButton(this);
    // // geoJsonLayersButton->setDefaultAction(geoJsonLayersAction);
    // // geoJsonLayersButton->setPopupMode(QToolButton::InstantPopup);
    // // geoJsonLayersButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // // addWidget(geoJsonLayersButton);

    // QToolButton *searchPlaceButton = new QToolButton(this);
    // searchPlaceButton->setDefaultAction(searchPlaceAction);
    // searchPlaceButton->setPopupMode(QToolButton::InstantPopup);
    // searchPlaceButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(searchPlaceButton);

    // // Drawing tools section
    // QToolButton *shapeButton = new QToolButton(this);
    // shapeButton->setDefaultAction(shapeAction);
    // shapeButton->setPopupMode(QToolButton::InstantPopup);
    // shapeButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(shapeButton);

    // QToolButton *bitmapButton = new QToolButton(this);
    // bitmapButton->setDefaultAction(bitmapAction);
    // bitmapButton->setPopupMode(QToolButton::InstantPopup);
    // bitmapButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(bitmapButton);

    // QToolButton *selectBitmapButton = new QToolButton(this);
    // selectBitmapButton->setDefaultAction(selectBitmapAction);
    // selectBitmapButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(selectBitmapButton);

    // // Measurement and preset layers section
    // QToolButton *measureDistanceButton = new QToolButton(this);
    // measureDistanceButton->setDefaultAction(measureDistanceAction);
    // measureDistanceButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(measureDistanceButton);

    // QToolButton *presetLayersButton = new QToolButton(this);
    // presetLayersButton->setDefaultAction(presetLayersAction);
    // presetLayersButton->setPopupMode(QToolButton::InstantPopup);
    // presetLayersButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(presetLayersButton);

    QToolButton *selectionButton = new QToolButton(this);
    selectionButton->setDefaultAction(selectionAction);
    selectionButton->setPopupMode(QToolButton::InstantPopup);
    selectionButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    addWidget(selectionButton);
    // QToolButton *dataSyncButton = new QToolButton(this);
    // dataSyncButton->setDefaultAction(dataSyncAction);
    // dataSyncButton->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    // addWidget(dataSyncButton);

}

// Main mode change handler: Updates action states based on current transformation mode
void DesignToolBar::onModeChanged(TransformMode mode) {
    viewAction->setChecked(false);
    moveAction->setChecked(false);
    rotateAction->setChecked(false);
    editTrajectoryAction->setChecked(false);
    measureDistanceAction->setChecked(false);
    switch(mode) {
    case Panning:
        viewAction->setChecked(true);
        highlightAction(viewAction);
        break;
    case Translate:
        moveAction->setChecked(true);
        highlightAction(moveAction);
        break;
    case Rotate:
        rotateAction->setChecked(true);
        highlightAction(rotateAction);
        break;
    case DrawTrajectory:
        editTrajectoryAction->setChecked(true);
        addTrajectoryAction->setChecked(true);
        highlightAction(editTrajectoryAction);
        break;
    case MeasureDistance:
        measureDistanceAction->setChecked(true);
        highlightAction(measureDistanceAction);
        break;
    }
}

// Main measurement trigger handler
void DesignToolBar::onMeasureDistanceTriggered() {
    if (measureDistanceAction->isChecked()) {
        emit modeChanged(MeasureDistance);
    } else {
        emit modeChanged(Translate);
    }
}

void DesignToolBar::importLayer() {
    QString filter = "All Supported Formats (*.geojson *.json *.kml *.kmz *.shp *.gml *.fgb *.csv);;"
                     "GeoJSON (*.geojson *.json);;"
                     "KML/KMZ (*.kml *.kmz);;"
                     "Shapefile (*.shp);;"
                     "GML (*.gml);;"
                     "FlatGeobuf (*.fgb);;"
                     "CSV with WKT (*.csv)";
    QString filePath = QFileDialog::getOpenFileName(this, "Import Layer", "", filter);
    if (filePath.isEmpty()) return;
    highlightAction(importLayerAction);
    emit importLayerTriggered(filePath);
}

// Handle adding a base layer to selected layers
void DesignToolBar::onAddBaseLayerToSelected(const QString& layerId) {
    if (selectedBaseLayers.contains(layerId)) {
        return;
    }
    int layerIndex = -1;
    for (int i = 0; i < mapLayers.size(); ++i) {
        if (mapLayers[i].id == layerId) {
            layerIndex = i;
            break;
        }
    }
    if (layerIndex < 0) {
        return;
    }

    selectedBaseLayers.append(layerId);
    updateMapLayersMenu();
    currentLayerOrder.removeAll(layerId);
    currentLayerOrder.prepend(layerId);
    QStringList activeLayers;
    for (const QString& orderedLayer : currentLayerOrder) {
        if (layerActions.contains(orderedLayer) &&
            layerActions[orderedLayer]->isChecked()) {
            activeLayers.append(orderedLayer);
        }
    }
    emit mapLayerChanged(activeLayers.join(","));
}

// Update the map layers menu to reflect current state
void DesignToolBar::updateMapLayersMenu() {
    // Get the menu
    StayOpenMenu* mapLayerMenu = qobject_cast<StayOpenMenu*>(mapSelectLayerAction->menu());
    if (!mapLayerMenu) return;
    // Clear existing menu
    mapLayerMenu->clear();
    // Recreate menu with updated state
    mapLayerMenu->setStyleSheet(DesignToolbarStyles::StayOpenMenu);
    // ========== SECTION 1: Available Base Map Layers ==========
    QLabel* baseLayersLabel = new QLabel("  Available Base Map Layers");
    baseLayersLabel->setStyleSheet(DesignToolbarStyles::MenuLabel);
    QWidgetAction* baseLabelAction = new QWidgetAction(this);
    baseLabelAction->setDefaultWidget(baseLayersLabel);
    mapLayerMenu->addAction(baseLabelAction);

    // Add base layer items with "+" buttons
    for (int i = 0; i < mapLayers.size(); ++i) {
        const auto& layer = mapLayers[i];
        QWidget* layerWidget = new QWidget();
        layerWidget->setStyleSheet("background-color: #1A3652;");

        QHBoxLayout* layout = new QHBoxLayout(layerWidget);
        layout->setContentsMargins(10, 2, 10, 2);
        layout->setSpacing(10);

        QLabel* nameLabel = new QLabel(layer.name);
        nameLabel->setMinimumWidth(150);
        nameLabel->setStyleSheet("color: white; background-color: transparent;");
        layout->addWidget(nameLabel);
        layout->addStretch();

        QPushButton* addButton = new QPushButton("+");
        addButton->setFixedSize(25, 25);
        addButton->setStyleSheet(DesignToolbarStyles::PushButton);

        // Disable button if layer is already selected
        if (selectedBaseLayers.contains(layer.id)) {
            addButton->setEnabled(false);
        }
        // Capture layer.id by value to avoid dangling references
        QString capturedLayerId = layer.id;
        connect(addButton, &QPushButton::clicked, this, [this, capturedLayerId]() {
            onAddBaseLayerToSelected(capturedLayerId);
        });

        layout->addWidget(addButton);
        layerWidget->setLayout(layout);
        QWidgetAction* widgetAction = new QWidgetAction(this);
        widgetAction->setDefaultWidget(layerWidget);
        mapLayerMenu->addAction(widgetAction);
        addLayerActions[layer.id] = widgetAction;
    }
    // Add separator between sections
    mapLayerMenu->addSeparator();

    // ========== SECTION 2: Selected Layers ==========
    QLabel* selectedLayersLabel = new QLabel("  Selected Layers");
    selectedLayersLabel->setStyleSheet(DesignToolbarStyles::MenuLabel);
    QWidgetAction* selectedLabelAction = new QWidgetAction(this);
    selectedLabelAction->setDefaultWidget(selectedLayersLabel);
    mapLayerMenu->addAction(selectedLabelAction);

    QActionGroup* layerGroup = new QActionGroup(this);
    layerGroup->setExclusive(false);
    for (const QString& layerId : selectedBaseLayers) {
        int layerIndex = -1;
        for (int i = 0; i < mapLayers.size(); ++i) {
            if (mapLayers[i].id == layerId) {
                layerIndex = i;
                break;
            }
        }
        if (layerIndex < 0) continue;
        const auto& layer = mapLayers[layerIndex];
        QString displayName = layer.name;
        if (layerId == "osm") {
            displayName += " (by default)";
        }
        QAction* action = nullptr;
        if (layerActions.contains(layerId)) {
            action = layerActions[layerId];
            action->setText(displayName);
        } else {
            action = new QAction(displayName, this);
            action->setCheckable(true);
            action->setChecked(true);
            action->setData(layerId);
            connect(action, &QAction::triggered, this, [this, layerId](bool checked) {
                if (checked) {
                    currentLayerOrder.removeAll(layerId);
                    currentLayerOrder.prepend(layerId);
                } else {
                    currentLayerOrder.removeAll(layerId);
                }
                QStringList activeLayers;
                for (const QString& orderedLayer : currentLayerOrder) {
                    if (layerActions.contains(orderedLayer) &&
                        layerActions[orderedLayer]->isChecked()) {
                        activeLayers.append(orderedLayer);
                    }
                }
                emit mapLayerChanged(activeLayers.join(","));
            });
            layerActions[layerId] = action;
        }
        mapLayerMenu->addAction(action);
        layerGroup->addAction(action);
    }
}

void DesignToolBar::updateTooltipOptions()
{
    QSet<QString> activeOptions;
    for (auto it = tooltipFieldActions.constBegin(); it != tooltipFieldActions.constEnd(); ++it) {
        if (it.value()->isChecked()) {
            activeOptions.insert(it.key());
        }
    }
    if (scenarioConfig) {
        scenarioConfig->saveTooltipFields(activeOptions);
    }
    emit tooltipOptionsChanged(activeOptions);
}

void DesignToolBar::setTrajectoryActionEnabled(bool enabled) {
    if (addTrajectoryAction) {
        addTrajectoryAction->setEnabled(enabled);
        QWidget* btn = widgetForAction(addTrajectoryAction);
        if (btn) {
            btn->setStyleSheet(enabled
                                   ? DesignToolbarStyles::ToolbarButton
                                   : DesignToolbarStyles::ToolbarButton + "opacity: 0.4;");
        }
    }
}

QAction* DesignToolBar::registerPluginAction(const QString& pluginId,
                                             QAction* action,
                                             int position)
{
    if (!action || pluginId.isEmpty())
        return nullptr;
    if (position < 0 || position >= this->actions().size()) {
        addAction(action);
    } else {
        insertAction(this->actions().at(position), action);
    }
    QWidget* btn = widgetForAction(action);
    if (btn) {
        btn->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    }
    m_pluginOwnedObjects[pluginId].append(action);
    return action;
}

QToolButton* DesignToolBar::registerPluginToolButton(const QString& pluginId,
                                                     QAction* action,
                                                     QToolButton::ToolButtonPopupMode mode,
                                                     int position)
{
    if (!action || pluginId.isEmpty())
        return nullptr;
    QToolButton* btn = new QToolButton(this);
    btn->setDefaultAction(action);
    btn->setPopupMode(mode);
    btn->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    btn->setIconSize(ICON_SIZE);
    QAction* wrapperAction = nullptr;
    if (position < 0) {
        wrapperAction = addWidget(btn);
    } else if (position < this->actions().size()) {
        wrapperAction = insertWidget(this->actions().at(position), btn);
    } else {
        wrapperAction = addWidget(btn);
    }
    m_pluginOwnedObjects[pluginId].append(btn);
    m_pluginOwnedObjects[pluginId].append(action);
    if (wrapperAction) {
        m_pluginOwnedObjects[pluginId].append(wrapperAction);
    }
    return btn;
}

QAction* DesignToolBar::registerPluginMenu(const QString& pluginId,
                                           const QString& title,
                                           QMenu* menu,
                                           int position)
{
    if (!menu || pluginId.isEmpty())
        return nullptr;
    QAction* act = new QAction(title, this);
    act->setMenu(menu);
    registerPluginToolButton(pluginId, act, QToolButton::InstantPopup, position);
    m_pluginOwnedObjects[pluginId].append(menu);
    return act;
}

void DesignToolBar::unregisterPlugin(const QString& pluginId)
{
    if (!m_pluginOwnedObjects.contains(pluginId))
        return;
    const QList<QObject*> objects = m_pluginOwnedObjects.value(pluginId);
    for (QObject* obj : objects) {
        if (QAction* act = qobject_cast<QAction*>(obj)) {
            removeAction(act);
            act->deleteLater();
        }
        else if (QToolButton* btn = qobject_cast<QToolButton*>(obj)) {
            btn->deleteLater();
        }
        else if (QMenu* menu = qobject_cast<QMenu*>(obj)) {
            menu->deleteLater();
        }
    }
    m_pluginOwnedObjects.remove(pluginId);
}
void DesignToolBar::clearSelectionModeUI()
{
    if (selectionWithGisAction)
        selectionWithGisAction->setChecked(false);
    if (selectionWithoutGisAction)
        selectionWithoutGisAction->setChecked(false);
    if (selectionAction) {
        selectionAction->setChecked(false);
        QWidget* btn = widgetForAction(selectionAction);
        if (btn)
            btn->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    }
}
