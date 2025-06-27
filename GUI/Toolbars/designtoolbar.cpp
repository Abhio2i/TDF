

#include "GUI/Toolbars/designtoolbar.h"
#include "GUI/Tacticaldisplay/Gis/custommapdialog.h"
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
    loadJsonAction = new QAction("📂 Load JSON", this);
    saveJsonAction = new QAction("💾 Save JSON", this);
    databaseMenu->addAction(loadJsonAction);
    databaseMenu->addAction(saveJsonAction);
    databaseAction->setMenu(databaseMenu);

    QToolButton* dbButton = dynamic_cast<QToolButton*>(widgetForAction(databaseAction));
    if (dbButton) {
        dbButton->setPopupMode(QToolButton::InstantPopup);
    }
}

void DesignToolBar::createActions() {
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

    measureAreaAction = new QAction(QIcon(withWhiteBg(":/icons/images/area.png")), tr("Measure Area"), this);
    measureAreaAction->setCheckable(true);
    connect(measureAreaAction, &QAction::triggered, this, [=]() {
        highlightAction(measureAreaAction);
        emit measureAreaTriggered();
    });

    drawAction = new QAction(QIcon(withWhiteBg(":/icons/images/technical-drawing.png")), tr("Draw"), this);
    drawAction->setCheckable(true);
    connect(drawAction, &QAction::triggered, this, [=]() {
        highlightAction(drawAction);
        emit drawTriggered();
    });

    databaseAction = new QAction(QIcon(withWhiteBg(":/icons/images/database (1).png")), tr("Database"), this);
    databaseAction->setCheckable(true);
    connect(databaseAction, &QAction::triggered, this, [=]() {
        highlightAction(databaseAction);
        emit databaseTriggered();
    });

    loadJsonAction = new QAction("📂 Load JSON", this);
    saveJsonAction = new QAction("💾 Save JSON", this);

    addCustomMapAction = new QAction("Add Custom Map", this);
    connect(addCustomMapAction, &QAction::triggered, this, [=]() {
        CustomMapDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString mapName = dialog.getMapName().trimmed();
            QString tileUrl = dialog.getTileUrl();
            int zoomMin = dialog.getZoomMin();
            int zoomMax = dialog.getZoomMax();
            if (!mapName.isEmpty() && !tileUrl.isEmpty()) {
                emit customMapAdded(mapName, zoomMin, zoomMax, tileUrl);
            }
        }
    });
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

    addAction(measureAreaAction);
    addAction(drawAction);
    addAction(databaseAction);
    addSeparator();
    addAction(zoomInAction);
    addAction(zoomOutAction);

    QToolButton* mapLayerButton = new QToolButton(this);
    mapLayerButton->setDefaultAction(mapSelectLayerAction);
    mapLayerButton->setPopupMode(QToolButton::InstantPopup);


    StayOpenMenu* mapLayerMenu = new StayOpenMenu(this);
    mapLayerMenu->setStyleSheet("QMenu::item:checked { background-color: #d0e0ff; }");

    QActionGroup* layerGroup = new QActionGroup(this);
    layerGroup->setExclusive(false); // Allow multiple layers to be selected

    QVector<QPair<QString, QString>> layers = {
        {"OpenStreetMap", "osm"},
        {"OpenTopoMap", "opentopo"},
        {"Carto Light", "carto-light"},
        {"Carto Dark", "carto-dark"},
        {"World Imagery", "World Imagery"}
    };

    QMap<QString, QAction*> layerActions; // Store actions for dynamic updates
    for (const auto& layer : layers) {
        QAction* action = new QAction(layer.first, this);
        action->setCheckable(true);
        action->setData(layer.second);
        if (layer.first == "OpenStreetMap") action->setChecked(true);
        mapLayerMenu->addAction(action);
        layerGroup->addAction(action);
        layerActions[layer.second] = action;

        connect(action, &QAction::triggered, this, [=]() {
            QStringList activeLayers;
            for (const auto& act : qAsConst(layerActions)) {
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
    addWidget(mapLayerButton);

    // Store layer actions for custom map additions
    connect(this, &DesignToolBar::customMapAdded, this, [=](const QString &layerName, int zoomMin, int zoomMax, const QString &tileUrl) mutable {
        // Prevent duplicate layer names
        if (layerActions.contains(layerName)) {
            qDebug() << "Error: Layer name" << layerName << "already exists";
            return;
        }

        QAction* action = new QAction(layerName, this);
        action->setCheckable(true);
        action->setData(layerName);
        action->setChecked(true); // Activate new layer immediately
        mapLayerMenu->insertAction(mapLayerMenu->actions().last(), action); // Add before separator
        layerGroup->addAction(action);
        layerActions[layerName] = action;

        connect(action, &QAction::triggered, this, [=]() {
            QStringList activeLayers;
            for (const auto& act : qAsConst(layerActions)) {
                if (act->isChecked()) {
                    activeLayers.append(act->data().toString());
                }
            }
            qDebug() << "Custom layer action triggered, emitting mapLayerChanged with layers:" << activeLayers;
            emit mapLayerChanged(activeLayers.join(","));
        });

        // Trigger layer update immediately after adding
        QStringList activeLayers;
        for (const auto& act : qAsConst(layerActions)) {
            if (act->isChecked()) {
                activeLayers.append(act->data().toString());
            }
        }
        qDebug() << "Custom layer added, emitting mapLayerChanged with layers:" << activeLayers;
        emit mapLayerChanged(activeLayers.join(","));
    });



    // Select Center action
    addAction(selectCenterAction);
    connect(selectCenterAction, &QAction::triggered, this, [this]() {
        emit selectCenterTriggered();
    });

    // Search Place action with input box
    QToolButton* searchPlaceButton = new QToolButton(this);
    searchPlaceButton->setDefaultAction(searchPlaceAction);
    searchPlaceButton->setPopupMode(QToolButton::InstantPopup);

    QMenu* searchMenu = new QMenu(this);
    QWidgetAction* searchAction = new QWidgetAction(this);

    QLineEdit* searchInput = new QLineEdit();
    searchInput->setPlaceholderText("Enter location...");
    searchInput->setMinimumWidth(200);
    connect(searchInput, &QLineEdit::returnPressed, [this, searchInput]() {
        emit searchPlaceTriggered(searchInput->text());
        searchInput->clear();
    });

    searchAction->setDefaultWidget(searchInput);
    searchMenu->addAction(searchAction);
    searchPlaceAction->setMenu(searchMenu);

    addWidget(searchPlaceButton);

}
void DesignToolBar::highlightAction(QAction *activeAction) {
    QList<QAction*> actions = {
        viewAction, moveAction, rotateAction, scaleAction,
        zoomInAction, zoomOutAction,
        gridToggleAction, snappingToggleAction,
        layerSelectAction, measureAreaAction, drawAction,
        databaseAction, mapSelectLayerAction, searchPlaceAction,
        selectCenterAction, addCustomMapAction
    };

    for (QAction *action : actions) {
        QWidget *btn = widgetForAction(action);
        if (!btn) continue;
        if (action == activeAction) {
            btn->setStyleSheet("QToolButton { background-color: #d0e0ff; border: 1px solid #5070ff; border-radius: 4px; }");
        } else {
            btn->setStyleSheet("");
        }
    }
}

void DesignToolBar::onModeChanged(int mode) {
    moveAction->setChecked(false);
    rotateAction->setChecked(false);
    scaleAction->setChecked(false);

    switch(mode) {
    case 0: moveAction->setChecked(true); break;
    case 1: rotateAction->setChecked(true); break;
    case 2: scaleAction->setChecked(true); break;
    }

    highlightAction(mode == 0 ? moveAction :
                        mode == 1 ? rotateAction : scaleAction);
}
