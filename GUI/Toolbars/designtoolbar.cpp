
#include "GUI/Toolbars/designtoolbar.h"
#include "GUI/Tacticaldisplay/Gis/custommapdialog.h"
#include "GUI/Tacticaldisplay/Gis/layerinformationdialog.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
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

DesignToolBar::DesignToolBar(QWidget *parent) : QToolBar(parent) {
    setStyleSheet(
        "QToolButton::menu-indicator { image: none; }"
        "QToolButton::menu-button { border: none; width: 16px; }"
        "QToolButton::menu-button:open { background-color: transparent; }"
        );

    createActions();
    setupToolBar();

    QMenu* databaseMenu = new QMenu(this);
    loadJsonAction = new QAction("Load JSON", this);
    saveJsonAction = new QAction("Save JSON", this);
    databaseMenu->addAction(loadJsonAction);
    databaseMenu->addAction(saveJsonAction);
    databaseAction->setMenu(databaseMenu);

    QToolButton* dbButton = dynamic_cast<QToolButton*>(widgetForAction(databaseAction));
    if (dbButton) {
        dbButton->setPopupMode(QToolButton::InstantPopup);
    }
}

void DesignToolBar::createActions() {
    mapLayers = {
                 { "OpenStreetMap", "osm", 0, 19, "", false, -1.0, "N/A", "Raster" },
                 };

    viewAction = new QAction(QIcon(withWhiteBg(":/icons/images/view.jpg")), tr("View"), this);
    viewAction->setCheckable(true);
    viewAction->setShortcut(QKeySequence(Qt::Key_0));
    connect(viewAction, &QAction::triggered, this, [=]() {
        highlightAction(viewAction);
        emit modeChanged(Panning);
        emit viewTriggered();
    });

    moveAction = new QAction(QIcon(withWhiteBg(":/icons/images/move.png")), tr("Move"), this);
    moveAction->setCheckable(true);
    moveAction->setShortcut(QKeySequence(Qt::Key_1));
    connect(moveAction, &QAction::triggered, this, [=]() {
        highlightAction(moveAction);
        emit modeChanged(Translate);
        qDebug() << "Move mode activated";
    });

    rotateAction = new QAction(QIcon(withWhiteBg(":/icons/images/rotate.png")), tr("Rotate"), this);
    rotateAction->setCheckable(true);
    rotateAction->setShortcut(QKeySequence(Qt::Key_2));
    connect(rotateAction, &QAction::triggered, this, [=]() {
        highlightAction(rotateAction);
        emit modeChanged(Rotate);
        qDebug() << "Rotate mode activated";
    });

    scaleAction = new QAction(QIcon(withWhiteBg(":/icons/images/scale.png")), tr("Scale"), this);
    scaleAction->setCheckable(true);
    scaleAction->setShortcut(QKeySequence(Qt::Key_3));
    connect(scaleAction, &QAction::triggered, this, [=]() {
        highlightAction(scaleAction);
        emit modeChanged(Scale);
        qDebug() << "Scale mode activated";
    });

    zoomInAction = new QAction(QIcon(withWhiteBg(":/icons/images/zoom-in.png")), tr("Zoom In"), this);
    zoomInAction->setCheckable(false);
    connect(zoomInAction, &QAction::triggered, this, [=]() {
        highlightAction(zoomInAction);
        emit zoomInTriggered();
    });

    zoomOutAction = new QAction(QIcon(withWhiteBg(":/icons/images/zoom-out.png")), tr("Zoom Out"), this);
    zoomOutAction->setCheckable(false);
    connect(zoomOutAction, &QAction::triggered, this, [=]() {
        highlightAction(zoomOutAction);
        emit zoomOutTriggered();
    });

    mapSelectLayerAction = new QAction(QIcon(withWhiteBg(":/icons/images/map.png")), tr("Map Layer"), this);
    mapSelectLayerAction->setCheckable(true);

    selectCenterAction = new QAction(QIcon(withWhiteBg(":/icons/images/centremap.png")), tr("Select Center"), this);
    selectCenterAction->setCheckable(true);
    connect(selectCenterAction, &QAction::triggered, this, [=]() {
        highlightAction(selectCenterAction);
        emit selectCenterTriggered();
    });

    searchPlaceAction = new QAction(QIcon(withWhiteBg(":/icons/images/search.png")), tr("Search Place"), this);
    searchPlaceAction->setCheckable(true);

    gridToggleAction = new QAction(QIcon(withWhiteBg(":/icons/images/grid.png")), tr("Toggle Grid"), this);
    gridToggleAction->setCheckable(true);
    connect(gridToggleAction, &QAction::triggered, this, [=](bool checked) {
        highlightAction(gridToggleAction);
        emit gridVisibilityToggled(checked);
    });

    gridPlaneXAction = new QAction(tr("X"), this);
    gridPlaneXAction->setCheckable(true);
    gridPlaneXAction->setChecked(true);
    connect(gridPlaneXAction, &QAction::triggered, this, [=](bool checked) {
        emit gridPlaneXToggled(checked);
    });

    gridPlaneYAction = new QAction(tr("Y"), this);
    gridPlaneYAction->setCheckable(true);
    gridPlaneYAction->setChecked(true);
    connect(gridPlaneYAction, &QAction::triggered, this, [=](bool checked) {
        emit gridPlaneYToggled(checked);
    });

    gridPlaneZAction = new QAction(tr("Z"), this);
    gridPlaneZAction->setCheckable(true);
    gridPlaneZAction->setChecked(true);
    connect(gridPlaneZAction, &QAction::triggered, this, [=](bool checked) {
        emit gridPlaneZToggled(checked);
    });

    snappingToggleAction = new QAction(QIcon(withWhiteBg(":/icons/images/grid1.png")), tr("Snap to Grid"), this);
    snappingToggleAction->setCheckable(true);
    connect(snappingToggleAction, &QAction::triggered, this, [=](bool checked) {
        highlightAction(snappingToggleAction);
        emit gridSnappingToggled(checked);
    });

    snapGridSizeXAction = new QAction(tr("X Size"), this);
    snapGridSizeXAction->setCheckable(true);
    snapGridSizeXAction->setChecked(true);
    connect(snapGridSizeXAction, &QAction::triggered, this, [=](bool checked) {
        emit snapGridSizeXToggled(checked);
    });

    snapGridSizeYAction = new QAction(tr("Y Size"), this);
    snapGridSizeYAction->setCheckable(true);
    snapGridSizeYAction->setChecked(true);
    connect(snapGridSizeYAction, &QAction::triggered, this, [=](bool checked) {
        emit snapGridSizeYToggled(checked);
    });

    snapGridSizeZAction = new QAction(tr("Z Size"), this);
    snapGridSizeZAction->setCheckable(true);
    snapGridSizeZAction->setChecked(true);
    connect(snapGridSizeZAction, &QAction::triggered, this, [=](bool checked) {
        emit snapGridSizeZToggled(checked);
    });

    snapRotateAction = new QAction(tr("Rotate"), this);
    snapRotateAction->setCheckable(true);
    connect(snapRotateAction, &QAction::triggered, this, [=](bool checked) {
        emit snapRotateToggled(checked);
    });

    snapScaleAction = new QAction(tr("Scale"), this);
    snapScaleAction->setCheckable(true);
    connect(snapScaleAction, &QAction::triggered, this, [=](bool checked) {
        emit snapScaleToggled(checked);
    });

    layerSelectAction = new QAction(QIcon(withWhiteBg(":/icons/images/layers.png")), tr("Select Layer"), this);
    layerSelectAction->setCheckable(true);

    StayOpenMenu* layerMenu = new StayOpenMenu(this);
    layerMenu->setStyleSheet("QMenu::item:checked { background-color: #d0e0ff; }");

    QAction* colliderAction = new QAction("Collider", this);
    QAction* meshAction = new QAction("Mesh", this);
    QAction* outlineAction = new QAction("Outline", this);
    QAction* informationAction = new QAction("Information", this);
    QAction* fpsAction = new QAction("FPS", this);
    QAction* imageAction = new QAction("Image", this);

    colliderAction->setCheckable(true);
    colliderAction->setChecked(true);
    meshAction->setCheckable(true);
    meshAction->setChecked(true);
    outlineAction->setCheckable(true);
    outlineAction->setChecked(true);
    informationAction->setCheckable(true);
    informationAction->setChecked(true);
    fpsAction->setCheckable(true);
    fpsAction->setChecked(true);
    imageAction->setCheckable(true);
    imageAction->setChecked(true);

    layerMenu->addAction(colliderAction);
    layerMenu->addAction(meshAction);
    layerMenu->addAction(outlineAction);
    layerMenu->addAction(informationAction);
    layerMenu->addAction(fpsAction);
    layerMenu->addAction(imageAction);

    layerSelectAction->setMenu(layerMenu);

    connect(colliderAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Collider", checked);
    });
    connect(meshAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Mesh", checked);
    });
    connect(outlineAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Outline", checked);
    });
    connect(informationAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Information", checked);
    });
    connect(fpsAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("FPS", checked);
    });
    connect(imageAction, &QAction::triggered, this, [=](bool checked) {
        emit layerOptionToggled("Image", checked);
    });

    // measureAreaAction = new QAction(QIcon(withWhiteBg(":/icons/images/area.png")), tr("Measure Area"), this);
    // measureAreaAction->setCheckable(true);
    // connect(measureAreaAction, &QAction::triggered, this, [=]() {
    //     highlightAction(measureAreaAction);
    //     emit measureAreaTriggered();
    // });

    // for measure distance
    measureDistanceAction = new QAction(QIcon(withWhiteBg(":/icons/images/measurement.png")), tr("Measure Distance"), this);
    measureDistanceAction->setCheckable(true);
    connect(measureDistanceAction, &QAction::triggered, this, [=]() {
        highlightAction(measureDistanceAction);
        emit measureDistanceTriggered();
    });
    presetLayersAction = new QAction(QIcon(withWhiteBg(":/icons/images/preset.png")), tr("Preset Layers"), this);
    presetLayersAction->setCheckable(true);
    StayOpenMenu* presetLayersMenu = new StayOpenMenu(this);
    presetLayersMenu->setStyleSheet("QMenu::item:checked { background-color: #d0e0ff; }");

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
        emit modeChanged(DrawTrajectory); // Use same mode as DrawTrajectory
        emit editTrajectoryTriggered();
    });

    databaseAction = new QAction(QIcon(withWhiteBg(":/icons/images/database (1).png")), tr("Database"), this);
    databaseAction->setCheckable(true);
    connect(databaseAction, &QAction::triggered, this, [=]() {
        highlightAction(databaseAction);
        emit databaseTriggered();
    });

    loadJsonAction = new QAction("Load JSON", this);
    saveJsonAction = new QAction("Save JSON", this);

    addCustomMapAction = new QAction("Add Custom Map", this);
    connect(addCustomMapAction, &QAction::triggered, this, [=]() {
        CustomMapDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString mapName = dialog.getMapName().trimmed();
            QString tileUrl = dialog.getTileUrl();
            if (!mapName.isEmpty() && !tileUrl.isEmpty()) {
                mapLayers.append({mapName, mapName, dialog.getZoomMin(), dialog.getZoomMax(),
                                  tileUrl, true, dialog.getOpacity(), dialog.getType()});
                emit customMapAdded(mapName, dialog.getZoomMin(), dialog.getZoomMax(), tileUrl, dialog.getOpacity());
            }
        }
    });

    layerInfoAction = new QAction(QIcon(withWhiteBg(":/icons/images/info.png")), tr("Layer Information"), this);
    layerInfoAction->setCheckable(false);
    connect(layerInfoAction, &QAction::triggered, this, [=]() {
        LayerInformationDialog dialog(mapLayers, this);
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
                qDebug() << "Layer edited, emitting mapLayerChanged with layers:" << activeLayers;
                emit customMapAdded(updatedLayer.name, updatedLayer.zoomMin, updatedLayer.zoomMax, updatedLayer.tileUrl, updatedLayer.opacity);
                emit mapLayerChanged(activeLayers.join(","));
            }
        });
        dialog.exec();
    });

    shapeAction = new QAction(QIcon(withWhiteBg(":/icons/images/shapes.png")), tr("Shape"), this);
    shapeAction->setCheckable(true);
    StayOpenMenu* shapeMenu = new StayOpenMenu(this);
    shapeMenu->setStyleSheet("QMenu::item:checked { background-color: #d0e0ff; }");

    QAction* drawLineAction = new QAction("Draw Line", this);
    QAction* drawCircleAction = new QAction("Circle", this);
    QAction* drawRectangleAction = new QAction("Rectangle", this);
    QAction* drawPolygonAction = new QAction("Polygon", this);
    QAction* drawPointsAction = new QAction("Points", this);
    QAction* drawCustomShapesAction = new QAction("Custom Shapes", this);

    drawLineAction->setCheckable(true);
    drawCircleAction->setCheckable(true);
    drawRectangleAction->setCheckable(true);
    drawPolygonAction->setCheckable(true);
    drawPointsAction->setCheckable(true);
    drawCustomShapesAction->setCheckable(true);

    shapeMenu->addAction(drawLineAction);
    shapeMenu->addAction(drawCircleAction);
    shapeMenu->addAction(drawRectangleAction);
    shapeMenu->addAction(drawPolygonAction);
    shapeMenu->addAction(drawPointsAction);
    shapeMenu->addAction(drawCustomShapesAction);

    shapeAction->setMenu(shapeMenu);

    QActionGroup* shapeGroup = new QActionGroup(this);
    shapeGroup->setExclusive(true);
    shapeGroup->addAction(drawLineAction);
    shapeGroup->addAction(drawCircleAction);
    shapeGroup->addAction(drawRectangleAction);
    shapeGroup->addAction(drawPolygonAction);
    shapeGroup->addAction(drawPointsAction);
    shapeGroup->addAction(drawCustomShapesAction);

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
    connect(drawCustomShapesAction, &QAction::triggered, this, [=]() {
        highlightAction(shapeAction);
        emit shapeSelected("Custom Shapes");
    });

    bitmapAction = new QAction(QIcon(withWhiteBg(":/icons/images/photo.png")), tr("Bitmaps"), this);
    bitmapAction->setCheckable(true);
    StayOpenMenu* bitmapMenu = new StayOpenMenu(this);
    bitmapMenu->setStyleSheet("QMenu::item:checked { background-color: #d0e0ff; }");

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

    selectBitmapAction = new QAction(QIcon(withWhiteBg(":/icons/images/picture.png")), tr("Select Bitmap Image"), this);
    selectBitmapAction->setCheckable(false);
    connect(selectBitmapAction, &QAction::triggered, this, [=]() {
        QFileDialog fileDialog(this, tr("Select Bitmap Image"));
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        fileDialog.setNameFilter(tr("Images (*.png *.jpg *.jpeg *.bmp)"));
        if (fileDialog.exec() == QDialog::Accepted) {
            QString selectedFile = fileDialog.selectedFiles().first();
            if (!selectedFile.isEmpty()) {
                emit bitmapImageSelected(selectedFile);
                highlightAction(selectBitmapAction);
            }
        }
    });

    StayOpenMenu* mapLayerMenu = new StayOpenMenu(this);
    mapLayerMenu->setStyleSheet("QMenu::item:checked { background-color: #d0e0ff; }");

    QActionGroup* layerGroup = new QActionGroup(this);
    layerGroup->setExclusive(false);

    layerActions.clear();
    for (const auto& layer : mapLayers) {
        QAction* action = new QAction(layer.name, this);
        action->setCheckable(true);
        action->setData(layer.id);
        if (layer.name == "OpenStreetMap") action->setChecked(true);
        mapLayerMenu->addAction(action);
        layerGroup->addAction(action);
        layerActions[layer.id] = action;

        connect(action, &QAction::triggered, this, [=]() {
            QStringList activeLayers;
            for (const auto& act : std::as_const(layerActions)) {
                if (act->isChecked()) {
                    activeLayers.append(act->data().toString());
                }
            }
            qDebug() << "Emitting mapLayerChanged with layers:" << activeLayers;
            emit mapLayerChanged(activeLayers.join(","));
        });
    }

    mapLayerMenu->addSeparator();
    mapLayerMenu->addAction(addCustomMapAction);
    mapSelectLayerAction->setMenu(mapLayerMenu);

    connect(this, &DesignToolBar::customMapAdded, this, [=](const QString &layerName, int /*zoomMin*/, int /*zoomMax*/, const QString &/*tileUrl*/) mutable {
        if (layerActions.contains(layerName)) {
            qDebug() << "Error: Layer name" << layerName << "already exists";
            return;
        }

        QAction* action = new QAction(layerName, this);
        action->setCheckable(true);
        action->setData(layerName);
        action->setChecked(true);
        mapLayerMenu->insertAction(mapLayerMenu->actions().last(), action);
        layerGroup->addAction(action);
        layerActions[layerName] = action;

        connect(action, &QAction::triggered, this, [=]() {
            QStringList activeLayers;
            for (const auto& act : std::as_const(layerActions)) {
                if (act->isChecked()) {
                    activeLayers.append(act->data().toString());
                }
            }
            qDebug() << "Custom layer action triggered, emitting mapLayerChanged with layers:" << activeLayers;
            emit mapLayerChanged(activeLayers.join(","));
        });

        QStringList activeLayers;
        QStringList activeLayerNames;
        for (const auto& act : std::as_const(layerActions)) {
            if (act->isChecked()) {
                activeLayers.append(act->data().toString());
                activeLayerNames.append(act->text());
            }
        }
        qDebug() << "Custom layer added, emitting mapLayerChanged with layers:" << activeLayers;
        emit mapLayerChanged(activeLayers.join(","));
    });

    QMenu* searchMenu = new QMenu(this);
    QWidgetAction* searchAction = new QWidgetAction(this);
    QLineEdit* searchInput = new QLineEdit();
    searchInput->setPlaceholderText("Enter location or coordinates (lat,lon)...");
    searchInput->setMinimumWidth(200);
    connect(searchInput, &QLineEdit::returnPressed, [this, searchInput]() {
        QString input = searchInput->text().trimmed();

        QRegExp coordRegex("^[-]?\\d*\\.?\\d+,[-]?\\d*\\.?\\d+$");
        if (coordRegex.exactMatch(input)) {
            QStringList coords = input.split(",");
            bool latOk, lonOk;
            double lat = coords[0].toDouble(&latOk);
            double lon = coords[1].toDouble(&lonOk);

            if (latOk && lonOk && lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180) {
                emit searchCoordinateTriggered(lat, lon);
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

void DesignToolBar::setupToolBar() {
    addAction(viewAction);
    addAction(moveAction);
    addAction(rotateAction);
    addAction(scaleAction);
    addSeparator();

    QToolButton* gridButton = new QToolButton(this);
    gridButton->setDefaultAction(gridToggleAction);
    gridButton->setPopupMode(QToolButton::InstantPopup);

    QPixmap arrowPixmap(12, 12);
    arrowPixmap.fill(Qt::transparent);
    QPainter painter(&arrowPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::white);
    painter.setBrush(Qt::white);
    painter.drawPolygon(QPolygonF() << QPointF(2,4) << QPointF(10,4) << QPointF(6,8));

    QToolButton* dropdownBtn = new QToolButton(this);
    dropdownBtn->setIcon(QIcon(arrowPixmap));
    dropdownBtn->setFixedSize(16, 16);
    dropdownBtn->setStyleSheet("QToolButton { border: none; background: transparent; }");
    connect(dropdownBtn, &QToolButton::clicked, [gridButton]() {
        gridButton->showMenu();
    });

    QWidget* gridContainer = new QWidget();
    QHBoxLayout* gridLayout = new QHBoxLayout(gridContainer);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);
    gridLayout->addWidget(gridButton);
    gridLayout->addWidget(dropdownBtn);

    QMenu* gridMenu = new QMenu(this);
    QWidget* gridPlaneWidget = new QWidget();
    QHBoxLayout* gridPlaneLayout = new QHBoxLayout(gridPlaneWidget);
    gridPlaneLayout->setContentsMargins(5, 5, 5, 5);
    gridPlaneLayout->setSpacing(5);

    QLabel* planeLabel = new QLabel("Grid Plane:");
    gridPlaneLayout->addWidget(planeLabel);

    QToolButton* xButton = new QToolButton();
    xButton->setDefaultAction(gridPlaneXAction);
    xButton->setAutoRaise(true);

    QToolButton* yButton = new QToolButton();
    yButton->setDefaultAction(gridPlaneYAction);
    yButton->setAutoRaise(true);

    QToolButton* zButton = new QToolButton();
    zButton->setDefaultAction(gridPlaneZAction);
    zButton->setAutoRaise(true);

    gridPlaneLayout->addWidget(xButton);
    gridPlaneLayout->addWidget(yButton);
    gridPlaneLayout->addWidget(zButton);

    QWidgetAction* gridPlaneAction = new QWidgetAction(this);
    gridPlaneAction->setDefaultWidget(gridPlaneWidget);
    gridMenu->addAction(gridPlaneAction);

    QWidgetAction* opacityAction = new QWidgetAction(this);
    QWidget* opacityWidget = new QWidget();
    QHBoxLayout* opacityLayout = new QHBoxLayout(opacityWidget);
    opacityLayout->addWidget(new QLabel("Opacity:"));
    QSlider* opacitySlider = new QSlider(Qt::Horizontal);
    opacitySlider->setRange(0, 100);
    opacitySlider->setValue(50);
    connect(opacitySlider, &QSlider::valueChanged, this, &DesignToolBar::gridOpacityChanged);
    opacityLayout->addWidget(opacitySlider);
    opacityWidget->setLayout(opacityLayout);
    opacityAction->setDefaultWidget(opacityWidget);

    gridMenu->addSeparator();
    gridMenu->addAction(opacityAction);

    gridButton->setMenu(gridMenu);
    addWidget(gridContainer);

    QToolButton* snapButton = new QToolButton(this);
    snapButton->setDefaultAction(snappingToggleAction);
    snapButton->setPopupMode(QToolButton::InstantPopup);

    QToolButton* snapDropdownBtn = new QToolButton(this);
    snapDropdownBtn->setIcon(QIcon(arrowPixmap));
    snapDropdownBtn->setFixedSize(16, 16);
    snapDropdownBtn->setStyleSheet("QToolButton { border: none; background: transparent; }");
    connect(snapDropdownBtn, &QToolButton::clicked, [snapButton]() {
        snapButton->showMenu();
    });

    QWidget* snapContainer = new QWidget();
    QHBoxLayout* snapLayout = new QHBoxLayout(snapContainer);
    snapLayout->setContentsMargins(0, 0, 0, 0);
    snapLayout->setSpacing(0);
    snapLayout->addWidget(snapButton);
    snapLayout->addWidget(snapDropdownBtn);

    QMenu* snapMenu = new QMenu(this);
    QWidget* snapSizeWidget = new QWidget();
    QHBoxLayout* snapSizeLayout = new QHBoxLayout(snapSizeWidget);
    snapSizeLayout->setContentsMargins(5, 5, 5, 5);
    snapSizeLayout->setSpacing(5);

    QLabel* snapSizeLabel = new QLabel("Grid Size:");
    snapSizeLayout->addWidget(snapSizeLabel);

    QToolButton* xSizeButton = new QToolButton();
    xSizeButton->setDefaultAction(snapGridSizeXAction);
    xSizeButton->setAutoRaise(true);

    QToolButton* ySizeButton = new QToolButton();
    ySizeButton->setDefaultAction(snapGridSizeYAction);
    ySizeButton->setAutoRaise(true);

    QToolButton* zSizeButton = new QToolButton();
    zSizeButton->setDefaultAction(snapGridSizeZAction);
    zSizeButton->setAutoRaise(true);

    snapSizeLayout->addWidget(xSizeButton);
    snapSizeLayout->addWidget(ySizeButton);
    snapSizeLayout->addWidget(zSizeButton);

    QWidgetAction* snapSizeAction = new QWidgetAction(this);
    snapSizeAction->setDefaultWidget(snapSizeWidget);
    snapMenu->addAction(snapSizeAction);

    snapMenu->addSeparator();

    QWidget* rotateWidget = new QWidget();
    QHBoxLayout* rotateLayout = new QHBoxLayout(rotateWidget);
    rotateLayout->setContentsMargins(5, 5, 5, 5);
    QToolButton* rotateButton = new QToolButton();
    rotateButton->setDefaultAction(snapRotateAction);
    rotateButton->setAutoRaise(true);
    rotateLayout->addWidget(rotateButton);
    QWidgetAction* rotateAction = new QWidgetAction(this);
    rotateAction->setDefaultWidget(rotateWidget);
    snapMenu->addAction(rotateAction);

    QWidget* scaleWidget = new QWidget();
    QHBoxLayout* scaleLayout = new QHBoxLayout(scaleWidget);
    scaleLayout->setContentsMargins(5, 5, 5, 5);
    QToolButton* scaleButton = new QToolButton();
    scaleButton->setDefaultAction(snapScaleAction);
    scaleButton->setAutoRaise(true);
    scaleLayout->addWidget(scaleButton);
    QWidgetAction* scaleAction = new QWidgetAction(this);
    scaleAction->setDefaultWidget(scaleWidget);
    snapMenu->addAction(scaleAction);

    snapButton->setMenu(snapMenu);
    addWidget(snapContainer);

    addSeparator();

    QToolButton* layerButton = new QToolButton(this);
    layerButton->setDefaultAction(layerSelectAction);
    layerButton->setPopupMode(QToolButton::InstantPopup);

    QToolButton* layerDropdownBtn = new QToolButton(this);
    layerDropdownBtn->setIcon(QIcon(arrowPixmap));
    layerDropdownBtn->setFixedSize(16, 16);
    layerDropdownBtn->setStyleSheet("QToolButton { border: none; background: transparent; }");
    connect(layerDropdownBtn, &QToolButton::clicked, [layerButton]() {
        layerButton->showMenu();
    });

    QWidget* layerContainer = new QWidget();
    QHBoxLayout* layerLayout = new QHBoxLayout(layerContainer);
    layerLayout->setContentsMargins(0, 0, 0, 0);
    layerLayout->setSpacing(0);
    layerLayout->addWidget(layerButton);
    layerLayout->addWidget(layerDropdownBtn);

    addWidget(layerContainer);

    addAction(databaseAction);
    addSeparator();
    addAction(zoomInAction);
    addAction(zoomOutAction);
    addAction(layerInfoAction);
    addAction(selectCenterAction);


    QToolButton* mapLayerButton = new QToolButton(this);
    mapLayerButton->setDefaultAction(mapSelectLayerAction);
    mapLayerButton->setPopupMode(QToolButton::InstantPopup);
    mapLayerButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    addWidget(mapLayerButton);

    QToolButton* searchPlaceButton = new QToolButton(this);
    searchPlaceButton->setDefaultAction(searchPlaceAction);
    searchPlaceButton->setPopupMode(QToolButton::InstantPopup);
    searchPlaceButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");

    addWidget(searchPlaceButton);
    QToolButton* shapeButton = new QToolButton(this);
    shapeButton->setDefaultAction(shapeAction);
    shapeButton->setPopupMode(QToolButton::InstantPopup);
    shapeButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    addWidget(shapeButton);

    QToolButton* bitmapButton = new QToolButton(this);
    bitmapButton->setDefaultAction(bitmapAction);
    bitmapButton->setPopupMode(QToolButton::InstantPopup);
    bitmapButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    addWidget(bitmapButton);

    QToolButton* selectBitmapButton = new QToolButton(this);
    selectBitmapButton->setDefaultAction(selectBitmapAction);
    selectBitmapButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    addWidget(selectBitmapButton);

    // for measure distance
    QToolButton* measureDistanceButton = new QToolButton(this);
    measureDistanceButton->setDefaultAction(measureDistanceAction);
    measureDistanceButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    addWidget(measureDistanceButton);

    QToolButton* presetLayersButton = new QToolButton(this);
    presetLayersButton->setDefaultAction(presetLayersAction);
    presetLayersButton->setPopupMode(QToolButton::InstantPopup);
    presetLayersButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    addWidget(presetLayersButton);
}

void DesignToolBar::highlightAction(QAction *activeAction) {
    QList<QAction*> actions = {
        viewAction, moveAction, rotateAction, scaleAction,
        zoomInAction, zoomOutAction,
        gridToggleAction, snappingToggleAction,
        layerSelectAction,
        editTrajectoryAction,
        databaseAction, mapSelectLayerAction, searchPlaceAction,
        selectCenterAction, addCustomMapAction, layerInfoAction,
        shapeAction, bitmapAction, selectBitmapAction,
        measureDistanceAction, presetLayersAction
    };

    for (QAction *action : actions) {
        QWidget *btn = widgetForAction(action);
        if (!btn) continue;
        if (action == activeAction) {
            btn->setStyleSheet("QToolButton { background-color: #d0e0ff; border: 1px solid #5070ff; border-radius: 4px; }");
        } else {
            btn->setStyleSheet("");
            action->setChecked(false); // Uncheck all other actions
        }
    }
}


void DesignToolBar::onModeChanged(TransformMode mode) {
    viewAction->setChecked(false);
    moveAction->setChecked(false);
    rotateAction->setChecked(false);
    scaleAction->setChecked(false);
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
    case Scale:
        scaleAction->setChecked(true);
        highlightAction(scaleAction);
        break;
    case DrawTrajectory:
        editTrajectoryAction->setChecked(true);
        highlightAction(editTrajectoryAction);
        break;
    case MeasureDistance:
        measureDistanceAction->setChecked(true);
        highlightAction(measureDistanceAction);
        break;
    }
}

void DesignToolBar::onMeasureDistanceTriggered() {
    if (measureDistanceAction->isChecked()) {
        emit modeChanged(MeasureDistance);
        Console::log("Measure Distance mode activated");
    } else {
        emit modeChanged(Translate);
        Console::log("Measure Distance mode deactivated");
    }
}
