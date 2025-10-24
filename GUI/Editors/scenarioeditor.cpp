
#include "GUI/Editors/scenarioeditor.h"
#include "GUI/Menubars/menubar.h"
#include "GUI/Sidebar/sidebarwidget.h"
#include "GUI/Tacticaldisplay/tacticaldisplay.h"
#include "GUI/Feedback/feedback.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QListWidget>
#include <QDockWidget>
#include <QSplitter>
#include <core/structure/scenario.h>
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "GUI/Toolbars/standardtoolbar.h"
#include <core/Render/scenerenderer.h>
#include <core/structure/runtime.h>
#include <core/Hierarchy/Components/transform.h>
#include <core/Hierarchy/Components/mesh.h>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <GUI/measuredistance/measuredistancedialog.h>
#include <QApplication>

ScenarioEditor::ScenarioEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Scenario Editor");
    resize(1100, 600);
    QDockWidget::DockWidgetFeatures dockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;
    textScriptDock = new QDockWidget("Text Script", this);
    textScriptView = new TextScriptWidget(this);
    textScriptDock->setWidget(textScriptView);
    qDebug() << "textScriptDock initialized:" << textScriptDock << ", textScriptView:" << textScriptView;
    setupMenuBar();
    connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
    setupToolBars();
    setupDockWidgets(dockFeatures);
    setupStatusBar();
    Scenario *scenario = new Scenario();
    hierarchy = scenario->hierarchy;
    SceneRenderer *renderer = scenario->scenerenderer;
    Console *console = scenario->console;
    scriptengine = scenario->scriptengine;
    library = scenario->Library;
    lastSavedFilePath = "";
    scenario->scriptengine->setHierarchy(hierarchy, treeView, renderer);
    connect(textScriptView, &TextScriptWidget::runScriptstring, scriptengine, &ScriptEngine::loadAndCompileScript);
    HierarchyConnector::instance()->setHierarchy(hierarchy);
    HierarchyConnector::instance()->setLibrary(library);
    HierarchyConnector::instance()->setLibTreeView(libTreeView);
    connect(console, &Console::logUpdate, this, [=](std::string log) {
        if (consoleView) {
            consoleView->appendLog(QString::fromStdString(log));
            consoleView->appendText(QString::fromStdString(log));
        }
    });
    connect(console, &Console::errorUpdate, this, [=](std::string error) {
        if (consoleView) {
            consoleView->appendError(QString::fromStdString(error));
            consoleView->appendText(QString::fromStdString(error));
        }
    });
    connect(console, &Console::warningUpdate, this, [=](std::string warning) {
        if (consoleView) {
            consoleView->appendWarning(QString::fromStdString(warning));
            consoleView->appendText(QString::fromStdString(warning));
        }
    });
    connect(console, &Console::debugUpdate, this, [=](std::string debug) {
        if (consoleView) {
            consoleView->appendDebug(QString::fromStdString(debug));
            consoleView->appendText(QString::fromStdString(debug));
        }
    });
    if (tacticalDisplay && tacticalDisplay->canvas) {
        // set canvas for the script engine
        if (tacticalDisplay && tacticalDisplay->canvas) {
            scriptengine->setCanvas(tacticalDisplay->canvas);
            qDebug() << "ScriptEngine canvas set successfully!";
        }
        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated, inspector, &Inspector::updateTrajectory);
        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated, this, [=](QString entityId, QJsonArray /*waypoints*/) {
            auto it = tacticalDisplay->canvas->Meshes.find(entityId.toStdString());
            if (it != tacticalDisplay->canvas->Meshes.end() && it->second.trajectory) {
                QJsonObject trajData = it->second.trajectory->toJson();
                hierarchy->UpdateComponent(entityId, "Trajectory", trajData);
                Console::log("Trajectory updated for entity: " + entityId.toStdString());
                treeView->getTreeWidget()->update();
            } else {
                Console::error("Failed to update trajectory for entity: " + entityId.toStdString() +
                               " - entity or trajectory not found");
            }
        });
        connect(inspector, &Inspector::trajectoryWaypointsChanged, tacticalDisplay->canvas, &CanvasWidget::updateWaypointsFromInspector);
    }
    connect(renderer, &SceneRenderer::addMesh, tacticalDisplay, &TacticalDisplay::addMesh);
    connect(hierarchy, &Hierarchy::entityRemoved, tacticalDisplay, &TacticalDisplay::removeMesh);
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->canvas, &CanvasWidget::Render);
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->scene3dwidget, &Scene3DWidget::updateEntities);
    }
    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::valueChanged, this, [=]{ renderer->Render(0.01f); markUnsavedChanges(); });
    HierarchyConnector::instance()->connectSignals(hierarchy, treeView, tacticalDisplay, inspector);
    HierarchyConnector::instance()->connectLibrarySignals(library, libTreeView);
    HierarchyConnector::instance()->initializeDummyData(hierarchy);
    HierarchyConnector::instance()->initializeLibraryData(library);
    HierarchyConnector::instance()->setupFileOperations(this, hierarchy, tacticalDisplay);
    connect(libTreeView, &HierarchyTree::itemDropped, this, [=](QVariantMap sourceData, QVariantMap targetData) {
        HierarchyConnector::instance()->handleLibraryToHierarchyDrop(sourceData, targetData);
        markUnsavedChanges();
    });
    connect(treeView, &HierarchyTree::itemDropped, this, [=](QVariantMap sourceData, QVariantMap targetData) {
        HierarchyConnector::instance()->handleHierarchyToLibraryDrop(sourceData, targetData);
        markUnsavedChanges();
    });
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(tacticalDisplay->canvas, &CanvasWidget::selectEntitybyCursor,
                treeView, &HierarchyTree::selectEntityById);
    }
    connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
        QString type;
        if (data["type"].type() == QVariant::Map) {
            QVariantMap typeData = data["type"].toMap();
            if (typeData.contains("type") && typeData["type"].toString() == "option") {
                type = "profile";
            } else {
                qWarning() << "Invalid nested type structure in itemSelected:" << data["type"];
                return;
            }
        } else {
            type = data["type"].toString();
        }
        QString name = data["name"].toString();
        QString ID = data["parentId"].toString();
        for (Inspector* inspector : inspectors) {
            // if (inspector->isLocked()) {
            //     continue;
            // }
            if (type == "component") {
                QJsonObject componentData = hierarchy->getComponentData(ID, name);
                if (!componentData.isEmpty()) {
                    inspector->init(ID, name, componentData);
                }
            } else if (type == "profile") {
                inspector->init(ID, name + "_self", (hierarchy->ProfileCategories)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "folder") {
                inspector->init(ID, name + "_self", (*hierarchy->Folders)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "entity") {
                inspector->init(data["ID"].toString(), name + "_self", (*hierarchy->Entities)[data["ID"].toString().toStdString()]->toJson());
            } else {
                inspector->init(ID, name, QJsonObject());
            }
        }
        if (!inspectorDock->isVisible()) {
            addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
            splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
            inspectorDock->show();
            qDebug() << "Inspector dock shown on item selection, geometry:" << inspectorDock->geometry();
        }
        if (tacticalDisplay && type == "entity") {
            tacticalDisplay->selectedMesh(data["ID"].toString());
            standardToolBar->getAddTrajectoryAction()->setEnabled(true);
            Console::log("Entity selected: " + data["ID"].toString().toStdString());
        } else {
            standardToolBar->getAddTrajectoryAction()->setEnabled(false);
            Console::log("Non-entity selected, addTrajectoryAction disabled");
        }
    });
    connect(inspector, &Inspector::addTabRequested, this, &ScenarioEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
    inspector->setHierarchy(hierarchy);
    setupToolBarConnections();
    // Connect hierarchy signals to mark unsaved changes
    connect(hierarchy, &Hierarchy::profileAdded, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderAdded, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityAdded, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentAdded, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRemoved, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRemoved, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRemoved, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentRemoved, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRenamed, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRenamed, this, &ScenarioEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRenamed, this, &ScenarioEditor::markUnsavedChanges);
}

void ScenarioEditor::setupToolBars()
{
    standardToolBar = new StandardToolBar(this);
    addToolBar(Qt::TopToolBarArea, standardToolBar);
    addToolBarBreak(Qt::TopToolBarArea);
    designToolBar = new DesignToolBar(this);
    addToolBar(Qt::TopToolBarArea, designToolBar);
    if (menuBar && standardToolBar) {
        connect(standardToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }
     connect(menuBar, &MenuBar::feedbackTriggered, this, &ScenarioEditor::showFeedbackWindow);
}

void ScenarioEditor::setupToolBarConnections()
{
    DesignToolBar *designToolBar = findChild<DesignToolBar*>();
    if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
        qWarning() << "Toolbar connection setup failed - required components missing";
        return;
    }
    if (menuBar) {
        connect(standardToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }
    connect(designToolBar, &DesignToolBar::modeChanged,
            this, [=](int mode) {
                tacticalDisplay->canvas->setTransformMode(static_cast<TransformMode>(mode));
            });
    connect(designToolBar, &DesignToolBar::shapeSelected,
            this, [=](const QString &shape) {
                tacticalDisplay->canvas->setShapeDrawingMode(true, shape);
                Console::log("Shape selected: " + shape.toStdString());
            });
    connect(designToolBar, &DesignToolBar::gridPlaneXToggled,
            tacticalDisplay->canvas, &CanvasWidget::setXGridVisible);
    connect(designToolBar, &DesignToolBar::gridPlaneYToggled,
            tacticalDisplay->canvas, &CanvasWidget::setYGridVisible);
    connect(designToolBar, &DesignToolBar::gridOpacityChanged,
            this, [=](int opacity) {
                tacticalDisplay->canvas->setGridOpacity(opacity);
                Console::log("Grid opacity changed to: " + std::to_string(opacity));
            });
    connect(designToolBar, &DesignToolBar::layerOptionToggled,
            tacticalDisplay->canvas, &CanvasWidget::toggleLayerVisibility);
    connect(designToolBar, &DesignToolBar::bitmapImageSelected,
            tacticalDisplay->canvas, &CanvasWidget::onBitmapImageSelected);
    connect(designToolBar, &DesignToolBar::presetLayerSelected,
            tacticalDisplay->canvas, &CanvasWidget::onPresetLayerSelected);
    connect(designToolBar, &DesignToolBar::bitmapSelected,
            this, [=](const QString &fileName) {
                tacticalDisplay->canvas->onBitmapSelected(fileName);
                Console::log("Bitmap selected: " + fileName.toStdString());
            });
    if (tacticalDisplay && tacticalDisplay->mapWidget) {
        connect(designToolBar, &DesignToolBar::mapLayerChanged,
                this, [=](const QString &layers) {
                    tacticalDisplay->setMapLayers(layers.split(",", Qt::SkipEmptyParts));
                    Console::log("Map layers updated: " + layers.toStdString());
                });
        connect(designToolBar, &DesignToolBar::customMapAdded,
                tacticalDisplay, &TacticalDisplay::addCustomMap);
        connect(designToolBar, &DesignToolBar::customMapAdded,
                this, [=](const QString &name, int zoomMin, int zoomMax, const QString &url) {
                    qDebug() << "ScenarioEditor received customMapAdded: name =" << name
                             << ", zoomMin =" << zoomMin << ", zoomMax =" << zoomMax
                             << ", url =" << url;
                });
        connect(designToolBar, &DesignToolBar::searchPlaceTriggered,
                tacticalDisplay->mapWidget, &GISlib::serachPlace);
        connect(designToolBar, &DesignToolBar::searchCoordinatesTriggered,
                tacticalDisplay->mapWidget, &GISlib::searchByCoordinates); // New coordinate search
        connect(designToolBar->zoomInAction, &QAction::triggered,
                tacticalDisplay, &TacticalDisplay::zoomIn);
        connect(designToolBar->zoomOutAction, &QAction::triggered,
                tacticalDisplay, &TacticalDisplay::zoomOut);
        connect(designToolBar->selectCenterAction, &QAction::triggered, this, [=]() {
            if (tacticalDisplay && tacticalDisplay->mapWidget) {
                tacticalDisplay->mapWidget->setCenter(0, 0);
                Console::log("Map centered at (0, 0)");
            }
        });
    } else {
        qCritical() << "Map widget not available for layer connections";
    }
    connect(standardToolBar->getAddTrajectoryAction(), &QAction::triggered,
            this, [=]() {
                tacticalDisplay->canvas->setTrajectoryDrawingMode(true);
                Console::log("Add Trajectory action triggered");
            });

    // gor geojson function
    connect(designToolBar, &DesignToolBar::importGeoJsonTriggered,
            tacticalDisplay->canvas, &CanvasWidget::importGeoJsonLayer);
    // NEW: Connections for GeoJSON layers menu
    connect(tacticalDisplay->canvas, &CanvasWidget::geoJsonLayerAdded,
            designToolBar, &DesignToolBar::onGeoJsonLayerAdded);
    connect(designToolBar, &DesignToolBar::geoJsonLayerToggled,
            tacticalDisplay->canvas, &CanvasWidget::onGeoJsonLayerToggled);
    // measure distance
    connect(designToolBar->getMeasureDistanceAction(), &QAction::triggered, tacticalDisplay->canvas, [=]() {
        bool isChecked = designToolBar->getMeasureDistanceAction()->isChecked();
        tacticalDisplay->canvas->setTransformMode(isChecked ? MeasureDistance : Translate);
    });
}

void ScenarioEditor::setupMenuBar()
{
    menuBar = new MenuBar(this);
    setMenuBar(menuBar);
     connect(menuBar, &MenuBar::feedbackTriggered, this, &ScenarioEditor::showFeedbackWindow);
}

void ScenarioEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setFeatures(dockFeatures);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(100);
    QSplitter *rightSplitter = new QSplitter(Qt::Horizontal, this);
    QSplitter *tacticalSplitter = new QSplitter(Qt::Vertical, this);
    tacticalDisplayDock = new QDockWidget("Tactical Display", this);
    tacticalDisplayDock->setFeatures(dockFeatures);
    tacticalDisplayDock->setAllowedAreas(Qt::RightDockWidgetArea);
    tacticalDisplay = new TacticalDisplay(this);
    tacticalDisplayDock->setWidget(tacticalDisplay);

    tacticalDisplayDock->setMinimumWidth(200);
    consoleDock = new QDockWidget("", this);
    consoleDock->setFeatures(dockFeatures);
    consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    tacticalSplitter->addWidget(tacticalDisplayDock);
    tacticalSplitter->addWidget(consoleDock);
    sidebarDock = new QDockWidget("", this);
    sidebarDock->setFeatures(dockFeatures);
    sidebarDock->setAllowedAreas(Qt::RightDockWidgetArea);
    SidebarWidget *sidebar = new SidebarWidget(this);
    sidebarDock->setTitleBarWidget(new QWidget());
    sidebarDock->setWidget(sidebar);
    sidebarDock->setMinimumWidth(80);
    sidebarDock->setMinimumHeight(40);
    rightSplitter->addWidget(tacticalSplitter);
    rightSplitter->addWidget(sidebarDock);
    mainSplitter->addWidget(hierarchyDock);
    mainSplitter->addWidget(rightSplitter);
    QTimer::singleShot(0, this, [=]() {
        int totalWidth = width();
        mainSplitter->setSizes(QList<int>() << totalWidth * 0.20 << totalWidth * 0.80);
        rightSplitter->setSizes(QList<int>() << totalWidth * 0.60 << totalWidth * 0.20);
    });
    tacticalSplitter->setSizes(QList<int>() << height() * 0.9 << height() * 0.1);
    libraryDock = new QDockWidget("Library", this);
    libraryDock->setFeatures(dockFeatures);
    libraryDock->setAllowedAreas(Qt::RightDockWidgetArea);
    libTreeView = new HierarchyTree(this);
    libraryDock->setWidget(libTreeView);
    libraryDock->setMinimumWidth(200);
    libraryDock->hide();
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setFeatures(dockFeatures);
    inspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
    inspectorDock->show();
    textScriptDock->setFeatures(dockFeatures);
    textScriptDock->setAllowedAreas(Qt::RightDockWidgetArea);
    textScriptDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, textScriptDock);
    textScriptDock->hide();
    connect(sidebar, &SidebarWidget::viewSelected, this, [this](const QString &viewName) {
        qDebug() << "Sidebar viewSelected emitted, viewName:" << viewName;
        if (viewName == "Inspector") {
            qDebug() << "Showing Inspector dock";
            libraryDock->hide();
            textScriptDock->hide();
            if (!inspectorDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
                splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
                inspectorDock->show();
                qDebug() << "Inspector dock geometry:" << inspectorDock->geometry();
            }
        } else if (viewName == "Library") {
            qDebug() << "Showing Library dock";
            inspectorDock->hide();
            textScriptDock->hide();
            if (!libraryDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, libraryDock);
                splitDockWidget(sidebarDock, libraryDock, Qt::Vertical);
                libraryDock->show();
                qDebug() << "Library dock geometry:" << libraryDock->geometry();
            }
        } else if (viewName == "TextScript") {
            qDebug() << "Showing TextScript dock";
            inspectorDock->hide();
            libraryDock->hide();
            if (!textScriptDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, textScriptDock);
                splitDockWidget(sidebarDock, textScriptDock, Qt::Vertical);
                textScriptDock->show();
                qDebug() << "TextScript dock geometry:" << textScriptDock->geometry();
            }
        } else if (viewName == "Console") {
            qDebug() << "Toggling Console dock, current visibility:" << consoleDock->isVisible();
            consoleDock->setVisible(!consoleDock->isVisible());
            qDebug() << "Console dock geometry:" << consoleDock->geometry();
        } else {
            qDebug() << "Unknown viewName received:" << viewName;
        }
    });
}

void ScenarioEditor::addInspectorTab()
{
    QDockWidget *newInspectorDock = new QDockWidget("Inspector " + QString::number(++inspectorCount), this);
    newInspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);
    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);
    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);
    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::addTabRequested,
            this, &ScenarioEditor::addInspectorTab);
    if (inspectorDock->isVisible()) {
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }
    connect(newInspectorDock, &QDockWidget::destroyed, this, [=]() {
        inspectorDocks.removeOne(newInspectorDock);
        inspectors.removeOne(newInspector);
    });
}

void ScenarioEditor::showFeedbackWindow()
{
    // Feedback *feedbackWindow = new Feedback(this);
    // feedbackWindow->show();
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->h = hierarchy;
    feedbackWindow->loadDashboardData("{}");
    feedbackWindow->show();
}

void ScenarioEditor::onItemSelected(QVariantMap /*data*/)
{
    // TODO: Implement item selection logic
}

void ScenarioEditor::onLibraryItemSelected(QVariantMap /*data*/)
{
    // TODO: Implement library item selection logic
}

ScenarioEditor::~ScenarioEditor()
{
    // Cleanup managed by Qt's parent-child relationships
}

void ScenarioEditor::loadFromJsonFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open JSON file:" << filePath;
        QMessageBox::warning(this, "Error", QString("Failed to open JSON file: %1").arg(filePath));
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << err.errorString();
        QMessageBox::warning(this, "Error", QString("Failed to parse JSON: %1").arg(err.errorString()));
        return;
    }
    QJsonObject obj = doc.object();
    if (obj.contains("hierarchy")) {
        QJsonObject hier = obj["hierarchy"].toObject();
        hierarchy->fromJson(hier);
        qDebug() << "Hierarchy loaded from file:" << filePath;
        if (treeView && treeView->getTreeWidget()) {
            treeView->getTreeWidget()->update();
            qDebug() << "HierarchyTree updated after loading JSON";
        } else {
            qWarning() << "Failed to update HierarchyTree: treeView or treeWidget is null";
        }
    } else {
        qWarning() << "JSON file does not contain 'hierarchy' key";
    }
    if (tacticalDisplay && obj.contains("tactical")) {
        QJsonObject tac = obj["tactical"].toObject();
        tacticalDisplay->canvas->fromJson(tac);
        qDebug() << "TacticalDisplay loaded from file:" << filePath;
    } else {
        qWarning() << "JSON file does not contain 'tactical' key or tacticalDisplay is null";
    }
    lastSavedFilePath = filePath;
    clearUnsavedChanges();
}

void ScenarioEditor::markUnsavedChanges()
{
    if (!hasUnsavedChanges) {
        hasUnsavedChanges = true;
        emit unsavedChangesChanged(true);
        setWindowTitle("Scenario Editor *");
    }
}

void ScenarioEditor::clearUnsavedChanges()
{
    if (hasUnsavedChanges) {
        hasUnsavedChanges = false;
        emit unsavedChangesChanged(false);
        setWindowTitle("Scenario Editor");
    }
}

void ScenarioEditor::setupStatusBar() {
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");
}
void ScenarioEditor::updateStatusBar(const QString &message) {
    if (statusBar) {
        statusBar->showMessage(message);
    }
}
// void ScenarioEditor::showFeedbackWindow()
// {
//     Feedback *feedbackWindow = new Feedback(this);

//     // Hierarchy se data lekar feedback window ko pass karen
//     if (hierarchy) {
//         QJsonObject feedbackData = HierarchyConnector::instance()->getFeedbackData(hierarchy);
//         QJsonDocument doc(feedbackData);
//         QString jsonString = doc.toJson(QJsonDocument::Compact);

//         feedbackWindow->loadDashboardData(jsonString);
//         qDebug() << "Feedback window loaded with hierarchy data";
//     } else {
//         qWarning() << "No hierarchy available for feedback window";
//         feedbackWindow->loadDashboardData("{}");
//     }

//     feedbackWindow->show();
//     qDebug() << "Feedback window shown";
// }
