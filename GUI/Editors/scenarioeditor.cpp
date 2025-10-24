/* ========================================================================= */
/* File: scenarioeditor.cpp                                                 */
/* Purpose: Implements scenario editor with hierarchy and tactical display   */
/* ========================================================================= */

#include "GUI/Editors/scenarioeditor.h"            // For scenario editor class
#include "GUI/Menubars/menubar.h"                 // For menu bar
#include "GUI/Sidebar/sidebarwidget.h"             // For sidebar widget
#include "GUI/Tacticaldisplay/tacticaldisplay.h"   // For tactical display
#include "GUI/Feedback/feedback.h"                // For feedback window
#include <QFile>                                  // For file operations
#include <QTextStream>                            // For text streaming
#include <QDebug>                                 // For debug output
#include <QListWidget>                            // For list widget
#include <QDockWidget>                            // For dock widget
#include <QSplitter>                              // For splitter widget
#include <core/structure/scenario.h>              // For scenario structure
#include "GUI/Tacticaldisplay/canvaswidget.h"      // For canvas widget
#include "GUI/Toolbars/standardtoolbar.h"         // For standard toolbar
#include <core/Render/scenerenderer.h>            // For scene renderer
#include <core/structure/runtime.h>               // For runtime structure
#include <core/Hierarchy/Components/transform.h>  // For transform component
#include <core/Hierarchy/Components/mesh.h>       // For mesh component
#include <QJsonDocument>                          // For JSON document handling
#include <QJsonParseError>                        // For JSON parse errors
#include <QMessageBox>                            // For message box
#include <GUI/measuredistance/measuredistancedialog.h> // For measure distance dialog
#include <QApplication>                           // For application instance

// %%% Constructor %%%
/* Initialize scenario editor */
ScenarioEditor::ScenarioEditor(QWidget *parent)
    : QMainWindow(parent)
{
    // Set window title
    setWindowTitle("Scenario Editor");
    // Set window size
    resize(1100, 600);
    // Define dock widget features
    QDockWidget::DockWidgetFeatures dockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;
    // Initialize text script dock
    textScriptDock = new QDockWidget("Text Script", this);
    textScriptView = new TextScriptWidget(this);
    textScriptDock->setWidget(textScriptView);
    qDebug() << "textScriptDock initialized:" << textScriptDock << ", textScriptView:" << textScriptView;
    // Setup UI components
    setupMenuBar();
    connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
    setupToolBars();
    setupDockWidgets(dockFeatures);
    setupStatusBar();
    // Initialize scenario
    Scenario *scenario = new Scenario();
    hierarchy = scenario->hierarchy;
    SceneRenderer *renderer = scenario->scenerenderer;
    Console *console = scenario->console;
    scriptengine = scenario->scriptengine;
    library = scenario->Library;
    lastSavedFilePath = "";
    // Setup script engine
    scenario->scriptengine->setHierarchy(hierarchy, treeView, renderer);
    connect(textScriptView, &TextScriptWidget::runScriptstring, scriptengine, &ScriptEngine::loadAndCompileScript);
    HierarchyConnector::instance()->setHierarchy(hierarchy);
    HierarchyConnector::instance()->setLibrary(library);
    HierarchyConnector::instance()->setLibTreeView(libTreeView);
    // Connect console signals
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
    // Connect tactical display signals
    if (tacticalDisplay && tacticalDisplay->canvas) {
        // Set canvas for script engine
        if (tacticalDisplay && tacticalDisplay->canvas) {
            scriptengine->setCanvas(tacticalDisplay->canvas);
            qDebug() << "ScriptEngine canvas set successfully!";
        }
        // Connect trajectory signals
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
    // Connect renderer signals
    connect(renderer, &SceneRenderer::addMesh, tacticalDisplay, &TacticalDisplay::addMesh);
    connect(hierarchy, &Hierarchy::entityRemoved, tacticalDisplay, &TacticalDisplay::removeMesh);
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->canvas, &CanvasWidget::Render);
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->scene3dwidget, &Scene3DWidget::updateEntities);
    }
    // Connect inspector signals
    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::valueChanged, this, [=]{ renderer->Render(0.01f); markUnsavedChanges(); });
    // Connect hierarchy signals
    HierarchyConnector::instance()->connectSignals(hierarchy, treeView, tacticalDisplay, inspector);
    HierarchyConnector::instance()->connectLibrarySignals(library, libTreeView);
    HierarchyConnector::instance()->initializeDummyData(hierarchy);
    HierarchyConnector::instance()->initializeLibraryData(library);
    HierarchyConnector::instance()->setupFileOperations(this, hierarchy, tacticalDisplay);
    // Connect item drop signals
    connect(libTreeView, &HierarchyTree::itemDropped, this, [=](QVariantMap sourceData, QVariantMap targetData) {
        HierarchyConnector::instance()->handleLibraryToHierarchyDrop(sourceData, targetData);
        markUnsavedChanges();
    });
    connect(treeView, &HierarchyTree::itemDropped, this, [=](QVariantMap sourceData, QVariantMap targetData) {
        HierarchyConnector::instance()->handleHierarchyToLibraryDrop(sourceData, targetData);
        markUnsavedChanges();
    });
    // Connect canvas signals
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(tacticalDisplay->canvas, &CanvasWidget::selectEntitybyCursor,
                treeView, &HierarchyTree::selectEntityById);
    }
    // Connect tree view item selection
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
    // Connect inspector tab signals
    connect(inspector, &Inspector::addTabRequested, this, &ScenarioEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
    inspector->setHierarchy(hierarchy);
    // Setup toolbar connections
    setupToolBarConnections();
    // Connect hierarchy signals for unsaved changes
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

/* Setup toolbars */
void ScenarioEditor::setupToolBars()
{
    // Add standard toolbar
    standardToolBar = new StandardToolBar(this);
    addToolBar(Qt::TopToolBarArea, standardToolBar);
    addToolBarBreak(Qt::TopToolBarArea);
    // Add design toolbar
    designToolBar = new DesignToolBar(this);
    addToolBar(Qt::TopToolBarArea, designToolBar);
    // Connect save action
    if (menuBar && standardToolBar) {
        connect(standardToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }
    // Connect feedback trigger
    connect(menuBar, &MenuBar::feedbackTriggered, this, &ScenarioEditor::showFeedbackWindow);
}

/* Setup toolbar connections */
void ScenarioEditor::setupToolBarConnections()
{
    // Find design toolbar
    DesignToolBar *designToolBar = findChild<DesignToolBar*>();
    // Check for null components
    if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
        qWarning() << "Toolbar connection setup failed - required components missing";
        return;
    }
    // Connect save action
    if (menuBar) {
        connect(standardToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }
    // Connect transform mode
    connect(designToolBar, &DesignToolBar::modeChanged,
            this, [=](int mode) {
                tacticalDisplay->canvas->setTransformMode(static_cast<TransformMode>(mode));
            });
    // Connect shape selection
    connect(designToolBar, &DesignToolBar::shapeSelected,
            this, [=](const QString &shape) {
                tacticalDisplay->canvas->setShapeDrawingMode(true, shape);
                Console::log("Shape selected: " + shape.toStdString());
            });
    // Connect grid visibility
    connect(designToolBar, &DesignToolBar::gridPlaneXToggled,
            tacticalDisplay->canvas, &CanvasWidget::setXGridVisible);
    connect(designToolBar, &DesignToolBar::gridPlaneYToggled,
            tacticalDisplay->canvas, &CanvasWidget::setYGridVisible);
    // Connect grid opacity
    connect(designToolBar, &DesignToolBar::gridOpacityChanged,
            this, [=](int opacity) {
                tacticalDisplay->canvas->setGridOpacity(opacity);
                Console::log("Grid opacity changed to: " + std::to_string(opacity));
            });
    // Connect layer visibility
    connect(designToolBar, &DesignToolBar::layerOptionToggled,
            tacticalDisplay->canvas, &CanvasWidget::toggleLayerVisibility);
    // Connect bitmap selection
    connect(designToolBar, &DesignToolBar::bitmapImageSelected,
            tacticalDisplay->canvas, &CanvasWidget::onBitmapImageSelected);
    connect(designToolBar, &DesignToolBar::presetLayerSelected,
            tacticalDisplay->canvas, &CanvasWidget::onPresetLayerSelected);
    connect(designToolBar, &DesignToolBar::bitmapSelected,
            this, [=](const QString &fileName) {
                tacticalDisplay->canvas->onBitmapSelected(fileName);
                Console::log("Bitmap selected: " + fileName.toStdString());
            });
    // Connect map layer changes
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
                tacticalDisplay->mapWidget, &GISlib::searchByCoordinates);
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
    // Connect trajectory action
    connect(standardToolBar->getAddTrajectoryAction(), &QAction::triggered,
            this, [=]() {
                tacticalDisplay->canvas->setTrajectoryDrawingMode(true);
                Console::log("Add Trajectory action triggered");
            });
    // Connect GeoJSON signals
    connect(designToolBar, &DesignToolBar::importGeoJsonTriggered,
            tacticalDisplay->canvas, &CanvasWidget::importGeoJsonLayer);
    connect(tacticalDisplay->canvas, &CanvasWidget::geoJsonLayerAdded,
            designToolBar, &DesignToolBar::onGeoJsonLayerAdded);
    connect(designToolBar, &DesignToolBar::geoJsonLayerToggled,
            tacticalDisplay->canvas, &CanvasWidget::onGeoJsonLayerToggled);
    // Connect measure distance
    connect(designToolBar->getMeasureDistanceAction(), &QAction::triggered, tacticalDisplay->canvas, [=]() {
        bool isChecked = designToolBar->getMeasureDistanceAction()->isChecked();
        tacticalDisplay->canvas->setTransformMode(isChecked ? MeasureDistance : Translate);
    });
}

/* Setup menu bar */
void ScenarioEditor::setupMenuBar()
{
    // Create and set menu bar
    menuBar = new MenuBar(this);
    setMenuBar(menuBar);
    // Connect feedback trigger
    connect(menuBar, &MenuBar::feedbackTriggered, this, &ScenarioEditor::showFeedbackWindow);
}

/* Setup dock widgets */
void ScenarioEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    // Setup main splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    // Setup hierarchy dock
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setFeatures(dockFeatures);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(100);
    // Setup right splitter
    QSplitter *rightSplitter = new QSplitter(Qt::Horizontal, this);
    // Setup tactical splitter
    QSplitter *tacticalSplitter = new QSplitter(Qt::Vertical, this);
    // Setup tactical display dock
    tacticalDisplayDock = new QDockWidget("Tactical Display", this);
    tacticalDisplayDock->setFeatures(dockFeatures);
    tacticalDisplayDock->setAllowedAreas(Qt::RightDockWidgetArea);
    tacticalDisplay = new TacticalDisplay(this);
    tacticalDisplayDock->setWidget(tacticalDisplay);
    tacticalDisplayDock->setMinimumWidth(200);
    // Setup console dock
    consoleDock = new QDockWidget("", this);
    consoleDock->setFeatures(dockFeatures);
    consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    // Setup splitters
    tacticalSplitter->addWidget(tacticalDisplayDock);
    tacticalSplitter->addWidget(consoleDock);
    // Setup sidebar dock
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
    // Set splitter sizes
    QTimer::singleShot(0, this, [=]() {
        int totalWidth = width();
        mainSplitter->setSizes(QList<int>() << totalWidth * 0.20 << totalWidth * 0.80);
        rightSplitter->setSizes(QList<int>() << totalWidth * 0.60 << totalWidth * 0.20);
    });
    tacticalSplitter->setSizes(QList<int>() << height() * 0.9 << height() * 0.1);
    // Setup library dock
    libraryDock = new QDockWidget("Library", this);
    libraryDock->setFeatures(dockFeatures);
    libraryDock->setAllowedAreas(Qt::RightDockWidgetArea);
    libTreeView = new HierarchyTree(this);
    libraryDock->setWidget(libTreeView);
    libraryDock->setMinimumWidth(200);
    libraryDock->hide();
    // Setup inspector dock
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
    // Setup text script dock
    textScriptDock->setFeatures(dockFeatures);
    textScriptDock->setAllowedAreas(Qt::RightDockWidgetArea);
    textScriptDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, textScriptDock);
    textScriptDock->hide();
    // Connect sidebar view selection
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

/* Add new inspector tab */
void ScenarioEditor::addInspectorTab()
{
    // Create new inspector dock
    QDockWidget *newInspectorDock = new QDockWidget("Inspector " + QString::number(++inspectorCount), this);
    newInspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);
    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);
    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);
    // Connect inspector signals
    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::addTabRequested,
            this, &ScenarioEditor::addInspectorTab);
    // Add or split dock
    if (inspectorDock->isVisible()) {
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }
    // Handle dock destruction
    connect(newInspectorDock, &QDockWidget::destroyed, this, [=]() {
        inspectorDocks.removeOne(newInspectorDock);
        inspectors.removeOne(newInspector);
    });
}

/* Show feedback window */
void ScenarioEditor::showFeedbackWindow()
{
    // Create and show feedback window
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->h = hierarchy;
    feedbackWindow->loadDashboardData("{}");
    feedbackWindow->show();
}

/* Handle item selection */
void ScenarioEditor::onItemSelected(QVariantMap /*data*/)
{
    // TODO: Implement item selection logic
}

/* Handle library item selection */
void ScenarioEditor::onLibraryItemSelected(QVariantMap /*data*/)
{
    // TODO: Implement library item selection logic
}

/* Destructor */
ScenarioEditor::~ScenarioEditor()
{
    // Cleanup managed by Qt's parent-child relationships
}

/* Load data from JSON file */
void ScenarioEditor::loadFromJsonFile(const QString &filePath)
{
    // Open JSON file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open JSON file:" << filePath;
        QMessageBox::warning(this, "Error", QString("Failed to open JSON file: %1").arg(filePath));
        return;
    }
    // Read and parse JSON
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << err.errorString();
        QMessageBox::warning(this, "Error", QString("Failed to parse JSON: %1").arg(err.errorString()));
        return;
    }
    // Load hierarchy data
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
    // Load tactical display data
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

/* Mark unsaved changes */
void ScenarioEditor::markUnsavedChanges()
{
    // Update unsaved changes state
    if (!hasUnsavedChanges) {
        hasUnsavedChanges = true;
        emit unsavedChangesChanged(true);
        setWindowTitle("Scenario Editor *");
    }
}

/* Clear unsaved changes */
void ScenarioEditor::clearUnsavedChanges()
{
    // Reset unsaved changes state
    if (hasUnsavedChanges) {
        hasUnsavedChanges = false;
        emit unsavedChangesChanged(false);
        setWindowTitle("Scenario Editor");
    }
}

/* Setup status bar */
void ScenarioEditor::setupStatusBar()
{
    // Create and set status bar
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");
}

/* Update status bar message */
void ScenarioEditor::updateStatusBar(const QString &message)
{
    // Update status bar
    if (statusBar) {
        statusBar->showMessage(message);
    }
}

/* Commented-out feedback window implementation */
// void ScenarioEditor::showFeedbackWindow()
// {
//     // Create feedback window
//     Feedback *feedbackWindow = new Feedback(this);
//
//     // Load hierarchy data for feedback
//     if (hierarchy) {
//         QJsonObject feedbackData = HierarchyConnector::instance()->getFeedbackData(hierarchy);
//         QJsonDocument doc(feedbackData);
//         QString jsonString = doc.toJson(QJsonDocument::Compact);
//
//         feedbackWindow->loadDashboardData(jsonString);
//         qDebug() << "Feedback window loaded with hierarchy data";
//     } else {
//         qWarning() << "No hierarchy available for feedback window";
//         feedbackWindow->loadDashboardData("{}");
//     }
//
//     feedbackWindow->show();
//     qDebug() << "Feedback window shown";
// }
