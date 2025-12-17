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
#include "qthread.h"
#include <core/Render/scenerenderer.h>            // For scene renderer
#include <core/structure/runtime.h>               // For runtime structure
#include <core/Hierarchy/Components/transform.h>  // For transform component
#include <core/Hierarchy/Components/mesh.h>       // For mesh component
#include <QJsonDocument>                          // For JSON document handling
#include <QJsonParseError>                        // For JSON parse errors
#include <QMessageBox>                            // For message box
#include <GUI/measuredistance/measuredistancedialog.h> // For measure distance dialog
#include <QApplication>                           // For application instance
#include <QTimer>                                 // For delayed operations
#include <GUI/Menubars/profileinfodialog.h>
#include <GUI/Editors/recentprojectsmanager.h>
#include <GUI/Settings/applicationdialog.h>
#include <QProgressDialog>

// %%% Constructor %%%
// Capitalize the first letter of a string
static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}
/* Initialize scenario editor */
ScenarioEditor::ScenarioEditor(QWidget *parent)
    : QMainWindow(parent)
{
    // Set window title
    setWindowTitle("Scenario Editor");
    // Set window size
    resize(1100, 600);

    // Use enhanced dock widget setup for Linux compatibility
    setupEnhancedDockWidgets();

    // Setup UI components
    setupMenuBar();
    connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
    setupToolBars();
    setupStatusBar();

    // Initialize scenario
    Scenario *scenario = new Scenario();
    hierarchy = scenario->hierarchy;
    SceneRenderer *renderer = scenario->scenerenderer;

    console = scenario->console;

    scriptengine = scenario->scriptengine;
    library = scenario->Library;
    lastSavedFilePath = "";

    // Setup script engine
    scenario->scriptengine->setHierarchy(hierarchy, treeView, renderer);
    connect(textScriptView, &TextScriptWidget::runScriptstring, scriptengine, &ScriptEngine::loadAndCompileScript);
    HierarchyConnector::instance()->setHierarchy(hierarchy);
    HierarchyConnector::instance()->setLibrary(library);
    HierarchyConnector::instance()->setLibTreeView(libTreeView);
    // connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
    //         this, &ScenarioEditor::loadRecentProject);
    connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
            this, [=](const QString& filePath, RecentProjectsManager::EditorType type) {
                if (type == RecentProjectsManager::ScenarioEditor) {
                    loadRecentProject(filePath);
                }
            });
    connect(this,&ScenarioEditor::Activated,tacticalDisplay->canvas,&CanvasWidget::ReInit);
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
                 QString displayName = capitalizeFirstLetter(name);
        for (Inspector* inspector : inspectors) {
            if (type == "component") {
                QJsonObject componentData = hierarchy->getComponentData(ID, name);
                if (!componentData.isEmpty()) {
                    inspector->init(ID, displayName, componentData);
                }
            } else if (type == "profile") {
                inspector->init(ID, displayName + "_self", (hierarchy->ProfileCategories)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "folder") {
                inspector->init(ID, displayName + "_self", (*hierarchy->Folders)[data["ID"].toString().toStdString()]->toJson());
            } else if (type == "entity") {
                inspector->init(data["ID"].toString(), displayName + "_self", (*hierarchy->Entities)[data["ID"].toString().toStdString()]->toJson());
            } else {
                inspector->init(ID, displayName, QJsonObject());
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
            // standardToolBar->getAddTrajectoryAction()->setEnabled(true);
            Console::log("Entity selected: " + data["ID"].toString().toStdString());
        } else {
            // standardToolBar->getAddTrajectoryAction()->setEnabled(false);
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


void ScenarioEditor::setupEnhancedDockWidgets()
{
    // Full dock features for complete movability
    QDockWidget::DockWidgetFeatures fullDockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;

    // Setup hierarchy dock with enhanced features
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                   Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    hierarchyDock->setFeatures(fullDockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);
    hierarchyDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // Setup tactical display dock with enhanced features
    tacticalDisplayDock = new QDockWidget("Tactical Display", this);
    tacticalDisplayDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                         Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    tacticalDisplayDock->setFeatures(fullDockFeatures);
    tacticalDisplay = new TacticalDisplay(this);
    tacticalDisplayDock->setWidget(tacticalDisplay);
    tacticalDisplayDock->setMinimumWidth(300);
    tacticalDisplayDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, tacticalDisplayDock);

    // Setup console dock with enhanced features
    consoleDock = new QDockWidget("Console", this);
    consoleDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                 Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    consoleDock->setFeatures(fullDockFeatures);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    consoleDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    consoleDock->hide();

    // Setup sidebar dock with enhanced features - CHANGED HERE
    sidebarDock = new QDockWidget("Sidebar", this);  // Title add kiya
    sidebarDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                 Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    sidebarDock->setFeatures(fullDockFeatures);
    SidebarWidget *sidebar = new SidebarWidget(this);

    // REMOVED: sidebarDock->setTitleBarWidget(new QWidget()); // Title bar enable kiya
    sidebarDock->setTitleBarWidget(nullptr); // Default title bar use karo

    sidebarDock->setWidget(sidebar);
    sidebarDock->setMinimumWidth(80);
    sidebarDock->setMinimumHeight(40);
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);

    // Setup inspector dock with enhanced features
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                   Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    inspectorDock->setFeatures(fullDockFeatures);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(10);
    inspectorDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // Setup library dock with enhanced features
    libraryDock = new QDockWidget("Library", this);
    libraryDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                 Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    libraryDock->setFeatures(fullDockFeatures);
    libTreeView = new HierarchyTree(this);
    libraryDock->setWidget(libTreeView);
    libraryDock->setMinimumWidth(200);
    libraryDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, libraryDock);
    libraryDock->hide();

    // Setup text script dock with enhanced features
    textScriptDock = new QDockWidget("Test Script", this);
    textScriptDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                    Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    textScriptDock->setFeatures(fullDockFeatures);
    textScriptView = new TextScriptWidget(this);
    textScriptDock->setWidget(textScriptView);
    textScriptDock->setMinimumWidth(200);
    textScriptDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, textScriptDock);
    textScriptDock->hide();

    // Connect dock visibility signals
    connect(hierarchyDock, &QDockWidget::visibilityChanged, this, &ScenarioEditor::onDockVisibilityChanged);
    connect(tacticalDisplayDock, &QDockWidget::visibilityChanged, this, &ScenarioEditor::onDockVisibilityChanged);
    connect(consoleDock, &QDockWidget::visibilityChanged, this, &ScenarioEditor::onDockVisibilityChanged);
    connect(inspectorDock, &QDockWidget::visibilityChanged, this, &ScenarioEditor::onDockVisibilityChanged);
    connect(libraryDock, &QDockWidget::visibilityChanged, this, &ScenarioEditor::onDockVisibilityChanged);
    connect(sidebarDock, &QDockWidget::visibilityChanged, this, &ScenarioEditor::onDockVisibilityChanged);
    connect(textScriptDock, &QDockWidget::visibilityChanged, this, &ScenarioEditor::onDockVisibilityChanged);

    // Set tabified docking to allow tabbed interface when docks are stacked
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    // Enable docking features
    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::GroupedDragging |
                   QMainWindow::AnimatedDocks);

    // Create initial layout
    // Remove central widget to use only docks
    setCentralWidget(nullptr);

    // Create initial splits - similar to original layout
    splitDockWidget(hierarchyDock, tacticalDisplayDock, Qt::Horizontal);
    splitDockWidget(tacticalDisplayDock, sidebarDock, Qt::Horizontal);
    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
    splitDockWidget(tacticalDisplayDock, consoleDock, Qt::Vertical);


    QTimer::singleShot(100, this, [=]() {
        int totalWidth = width();
        int totalHeight = height();

        // UPDATED: Inspector ko kam, tactical display ko zyada width
        int hierarchyWidth = static_cast<int>(totalWidth * 0.10);   // 10% for hierarchy
        int tacticalWidth = static_cast<int>(totalWidth * 0.78);    // 78% for tactical display ✅ (increased from 75%)
        int sidebarWidth = static_cast<int>(totalWidth * 0.05);     // 5% for sidebar (reduced from 6%)
        int inspectorWidth = static_cast<int>(totalWidth * 0.07);   // 7% for inspector ✅ (reduced from 9%)
        int consoleHeight = static_cast<int>(totalHeight * 0.15);   // 15% for console
        // Verify total width adds up to 100%
        int totalCalculatedWidth = hierarchyWidth + tacticalWidth + sidebarWidth + inspectorWidth;
        // qDebug() << "Width distribution - Hierarchy:" << hierarchyWidth
        //          << "Tactical:" << tacticalWidth
        //          << "Sidebar:" << sidebarWidth
        //          << "Inspector:" << inspectorWidth
        //          << "Total:" << totalCalculatedWidth << "/" << totalWidth;

        // Resize docks
        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({tacticalDisplayDock}, {tacticalWidth}, Qt::Horizontal);
        resizeDocks({sidebarDock}, {sidebarWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);
    });

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
/* Setup dock widgets - legacy method, using enhanced version instead */
void ScenarioEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    // This method is replaced by setupEnhancedDockWidgets()
    setupEnhancedDockWidgets();
}

/* Handle dock visibility changes */
void ScenarioEditor::onDockVisibilityChanged(bool visible)
{
    QDockWidget* dock = qobject_cast<QDockWidget*>(sender());
    if (dock) {
        if (visible) {
            dock->raise(); // Bring to front when shown
        }
    }
}

/* Setup menu bar */
void ScenarioEditor::setupMenuBar()
{
    // Create and set menu bar
    menuBar = new MenuBar(this);
    setMenuBar(menuBar);

    // Connect feedback trigger
    connect(menuBar, &MenuBar::feedbackTriggered, this, &ScenarioEditor::showFeedbackWindow);

    // Get the Edit menu from MenuBar and add reset layout action
    QMenu *editMenu = menuBar->getEditMenu();
    if (editMenu) {
        // Create reset layout action
        QAction *resetLayoutAction = new QAction("Reset Layout", this);
        // resetLayoutAction->setShortcut(QKeySequence("Ctrl+R"));
        resetLayoutAction->setStatusTip("Reset all docks to initial positions");

        // Add separator and then reset layout action to Edit menu
        editMenu->addSeparator();
        editMenu->addAction(resetLayoutAction);

        // Connect the action
        connect(resetLayoutAction, &QAction::triggered, this, &ScenarioEditor::resetLayout);
    }
}

/* Setup toolbars */
void ScenarioEditor::setupToolBars()
{

    // Add design toolbar
    designToolBar = new DesignToolBar(this);
    addToolBar(Qt::TopToolBarArea, designToolBar);

    // Allow toolbars to be movable
    // standardToolBar->setMovable(true);
    designToolBar->setMovable(true);
    connect(menuBar->getSaveAction(), &QAction::triggered, this, &ScenarioEditor::clearUnsavedChanges);
    connect(menuBar->getSameSaveAction(), &QAction::triggered, this, &ScenarioEditor::clearUnsavedChanges);
    connect(menuBar->getRecentProjectAction(), &QAction::triggered,
            this, &ScenarioEditor::onRecentProjectTriggered);

    qDebug() << "MenuBar actions connected successfully";
    qDebug() << "Recent Project Action:" << menuBar->getRecentProjectAction()->text();
    connect(menuBar, &MenuBar::profileTriggered,  // Signal use करें
            this, &ScenarioEditor::showProfileInfo);
    connect(menuBar, &MenuBar::applicationTriggered, this, &ScenarioEditor::showApplicationDialog);

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
    connect(designToolBar, &DesignToolBar::coordinateSystemChanged,
            tacticalDisplay->mapWidget, &GISlib::setCoordinateSystem);
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
    connect(designToolBar->getAddTrajectoryAction(), &QAction::triggered,
            this, [=]() {
                tacticalDisplay->canvas->setTrajectoryDrawingMode(true);
                Console::log("Add Trajectory action triggered from DesignToolBar");
            });
    // Connect GeoJSON signals
    connect(designToolBar, &DesignToolBar::importGeoJsonTriggered,
            tacticalDisplay->canvas, &CanvasWidget::importGeoJsonLayer);
    connect(tacticalDisplay->canvas, &CanvasWidget::geoJsonLayerAdded,
            designToolBar, &DesignToolBar::onGeoJsonLayerAdded);
    connect(designToolBar, &DesignToolBar::geoJsonLayerToggled,
            tacticalDisplay->canvas, &CanvasWidget::onGeoJsonLayerToggled);

    // Connect measure distance
    if (designToolBar->getMeasureDistanceAction()) {
        connect(designToolBar->getMeasureDistanceAction(), &QAction::triggered, tacticalDisplay->canvas, [=]() {
            bool isChecked = designToolBar->getMeasureDistanceAction()->isChecked();
            tacticalDisplay->canvas->setTransformMode(isChecked ? MeasureDistance : Translate);
        });
    }
}

void ScenarioEditor::resetLayout()
{

    console->log("Resetting Scenario Editor layout to initial state...");

    // Close all additional inspector docks
    for (int i = inspectorDocks.size() - 1; i >= 0; --i) {
        QDockWidget* dock = inspectorDocks[i];
        if (dock != inspectorDock) { // Keep the main inspector
            inspectors.removeAt(i);
            dock->deleteLater();
        }
    }

    // Keep only main inspector
    if (inspectorDocks.size() > 1) {
        inspectorDocks = QList<QDockWidget*>{inspectorDock};
        inspectors = QList<Inspector*>{inspector};
    }
    inspectorCount = 0;

    // Hide all docks first
    hierarchyDock->hide();
    tacticalDisplayDock->hide();
    consoleDock->hide();
    inspectorDock->hide();
    libraryDock->hide();
    sidebarDock->hide();
    textScriptDock->hide();

    // Remove all docks from main window
    removeDockWidget(hierarchyDock);
    removeDockWidget(tacticalDisplayDock);
    removeDockWidget(consoleDock);
    removeDockWidget(inspectorDock);
    removeDockWidget(libraryDock);
    removeDockWidget(sidebarDock);
    removeDockWidget(textScriptDock);

    // Add docks back to initial positions
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
    addDockWidget(Qt::RightDockWidgetArea, tacticalDisplayDock);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    addDockWidget(Qt::RightDockWidgetArea, libraryDock);
    addDockWidget(Qt::RightDockWidgetArea, textScriptDock);

    // Hide library and text script by default (as per original)
    libraryDock->hide();
    textScriptDock->hide();

    // Recreate initial split configuration
    splitDockWidget(hierarchyDock, tacticalDisplayDock, Qt::Horizontal);
    splitDockWidget(tacticalDisplayDock, sidebarDock, Qt::Horizontal);
    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
    splitDockWidget(tacticalDisplayDock, consoleDock, Qt::Vertical);

    // Show main docks
    hierarchyDock->show();
    tacticalDisplayDock->show();
    consoleDock->show();
    sidebarDock->show();
    inspectorDock->show();

    // Reset to initial sizes with a small delay - UPDATED TO 75%
    QTimer::singleShot(100, this, [=]() {
        QMainWindow::resize(1100, 600); // Reset window size

        int totalWidth = this->width();
        int totalHeight = this->height();

        // UPDATED: 75% for tactical display
        int hierarchyWidth = static_cast<int>(totalWidth * 0.10);   // 10% for hierarchy
        int tacticalWidth = static_cast<int>(totalWidth * 0.75);    // 75% for tactical display ✅
        int sidebarWidth = static_cast<int>(totalWidth * 0.06);     // 6% for sidebar
        int inspectorWidth = static_cast<int>(totalWidth * 0.09);   // 9% for inspector
        int consoleHeight = static_cast<int>(totalHeight * 0.15);   // 15% for console

        // Verify total width adds up to 100%
        int totalCalculatedWidth = hierarchyWidth + tacticalWidth + sidebarWidth + inspectorWidth;
        // qDebug() << "Reset Layout - Width distribution - Hierarchy:" << hierarchyWidth
        //          << "Tactical:" << tacticalWidth
        //          << "Sidebar:" << sidebarWidth
        //          << "Inspector:" << inspectorWidth
        //          << "Total:" << totalCalculatedWidth << "/" << totalWidth;

        // Resize docks
        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({tacticalDisplayDock}, {tacticalWidth}, Qt::Horizontal);
        resizeDocks({sidebarDock}, {sidebarWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);

        // Bring all docks to front
        hierarchyDock->raise();
        tacticalDisplayDock->raise();
        consoleDock->raise();
        sidebarDock->raise();
        inspectorDock->raise();

        updateStatusBar("Scenario Editor layout reset to initial state");
        console->log("Scenario Editor layout successfully reset to initial configuration");
    });
}

/* Add new inspector tab */
void ScenarioEditor::addInspectorTab()
{
    // Create new inspector dock with full features
    QDockWidget *newInspectorDock = new QDockWidget("Inspector " + QString::number(++inspectorCount), this);
    newInspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                      Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);

    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);
    newInspectorDock->setMinimumWidth(200);
    newInspectorDock->setTitleBarWidget(nullptr);

    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);

    // Connect inspector signals
    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::valueChanged,
            this, &ScenarioEditor::markUnsavedChanges);
    connect(newInspector, &Inspector::addTabRequested,
            this, &ScenarioEditor::addInspectorTab);

    // Connect dock visibility
    connect(newInspectorDock, &QDockWidget::visibilityChanged,
            this, &ScenarioEditor::onDockVisibilityChanged);

    // Add or split dock - try to find the best placement
    if (inspectorDock->isVisible()) {
        // Try to split with existing inspector dock
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else {
        // Default to right dock area
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }

    // Handle dock destruction
    connect(newInspectorDock, &QDockWidget::destroyed, this, [=]() {
        inspectorDocks.removeOne(newInspectorDock);
        inspectors.removeOne(newInspector);
    });

    // Show the new dock
    newInspectorDock->show();
    newInspectorDock->raise();
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


void ScenarioEditor::loadFromJsonFile(const QString &filePath)
{
    // Create loading dialog with indeterminate progress
    QProgressDialog* loadingDialog = new QProgressDialog("Loading data...", nullptr, 0, 0, this);
    loadingDialog->setWindowTitle("");
    loadingDialog->setWindowModality(Qt::WindowModal);
    loadingDialog->setCancelButton(nullptr); // No cancel button
    loadingDialog->setMinimumDuration(0); // Show immediately
    loadingDialog->setRange(0, 0); // Indeterminate mode
    loadingDialog->setValue(0);
    loadingDialog->show();

    // Force UI update
    QCoreApplication::processEvents();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open JSON file:" << filePath;
        QMessageBox::warning(this, "Error", QString("Failed to open JSON file: %1").arg(filePath));
        loadingDialog->deleteLater();
        return;
    }

    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << err.errorString();
        QMessageBox::warning(this, "Error", QString("Failed to parse JSON: %1").arg(err.errorString()));
        loadingDialog->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("hierarchy")) {
        loadingDialog->setLabelText("Loading...");
        QCoreApplication::processEvents();

        QJsonObject hier = obj["hierarchy"].toObject();
        hierarchy->fromJson(hier);

        QCoreApplication::processEvents();

        qDebug() << "Hierarchy loaded from file:" << filePath;
        if (treeView && treeView->getTreeWidget()) {
            treeView->getTreeWidget()->update();
            qDebug() << "HierarchyTree updated after loading JSON";
        } else {
            qWarning() << "Failed to update HierarchyTree: treeView or treeWidget is null";
        }

        QCoreApplication::processEvents();
    } else {
        qWarning() << "JSON file does not contain 'hierarchy' key";
    }

    if (tacticalDisplay && obj.contains("tactical")) {
        loadingDialog->setLabelText("Loading tactical display...");
        QCoreApplication::processEvents();

        QJsonObject tac = obj["tactical"].toObject();
        tacticalDisplay->canvas->fromJson(tac);
        qDebug() << "TacticalDisplay loaded from file:" << filePath;
    } else {
        qWarning() << "JSON file does not contain 'tactical' key or tacticalDisplay is null";
    }

    lastSavedFilePath = filePath;
    clearUnsavedChanges();

    // Close loading dialog
    loadingDialog->close();
    loadingDialog->deleteLater();

    updateStatusBar("Project loaded: " + QFileInfo(filePath).fileName());
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
    if (statusBar) {
        statusBar->showMessage(message);
    }
}

void ScenarioEditor::loadRecentProject(const QString& filePath)
{
    // Create loading dialog with indeterminate progress
    QProgressDialog* loadingDialog = new QProgressDialog("Loading data...", nullptr, 0, 0, this);
    loadingDialog->setWindowTitle("");
    loadingDialog->setWindowModality(Qt::WindowModal);
    loadingDialog->setCancelButton(nullptr); // No cancel button
    loadingDialog->setMinimumDuration(0); // Show immediately
    loadingDialog->setRange(0, 0); // Indeterminate mode
    loadingDialog->setValue(0);
    loadingDialog->show();

    // Force UI update
    QCoreApplication::processEvents();


    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to open scenario file");
        loadingDialog->deleteLater();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error", "Invalid scenario file format");
        loadingDialog->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("hierarchy")) {
        QMessageBox::warning(this, "Error", "Not a valid scenario file");
        loadingDialog->deleteLater();
        return;
    }

    loadingDialog->setLabelText("Loading...");
    QCoreApplication::processEvents();

    // Load hierarchy
    QJsonObject hier = obj["hierarchy"].toObject();
    hierarchy->fromJson(hier);

    loadingDialog->setLabelText("Loading tactical display...");
    QCoreApplication::processEvents();

    // Load tactical display if present
    if (tacticalDisplay && obj.contains("tactical")) {
        QJsonObject tac = obj["tactical"].toObject();
        tacticalDisplay->canvas->fromJson(tac);
    }

    lastSavedFilePath = filePath;
    clearUnsavedChanges();

    // Add to ScenarioEditor-specific recent projects
    RecentProjectsManager::instance()->addToRecentProjects(filePath,
                                                           RecentProjectsManager::ScenarioEditor);

    // Close loading dialog
    loadingDialog->close();
    loadingDialog->deleteLater();

    updateStatusBar("Scenario loaded: " + QFileInfo(filePath).fileName());
    console->log("Scenario project loaded: " + filePath.toStdString());
}
void ScenarioEditor::onRecentProjectTriggered()
{
    RecentProjectsManager::instance()->showRecentProjectsMenu(this,
                                                              RecentProjectsManager::ScenarioEditor);
}

// void ScenarioEditor::clearRecentProjects()
// {
//     RecentProjectsManager::instance()->clearRecentProjects();
//     updateStatusBar("Recent projects list cleared");
//     console->log("Recent projects list cleared");
// }

void ScenarioEditor::showProfileInfo()
{
    ProfileInfoDialog::showProfileInfo(this);
}
void ScenarioEditor::showApplicationDialog()
{
    ApplicationDialog dialog(this);
    connect(&dialog,&ApplicationDialog::fpsState,simulation,&Simulation::setFps);
    connect(&dialog,&ApplicationDialog::canvasIconState,tacticalDisplay->canvas,&CanvasWidget::setImageScale);
    dialog.exec();

}

