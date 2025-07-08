
#include "../../GUI/Editors/RuntimeEditor.h"
#include "GUI/Menubars/menubar.h"
#include "GUI/Navigation/navigationpage.h"
#include "GUI/Overview/overview.h"
#include "GUI/Sidebar/sidebarwidget.h"
#include "GUI/Toolbars/standardtoolbar.h"
#include "GUI/Toolbars/networktoolbar.h"
#include "GUI/Feedback/feedback.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QListWidget>
#include <QDockWidget>
#include <core/structure/runtime.h>
#include <core/structure/scenario.h>
#include <core/Hierarchy//Components/transform.h>
#include <core/Hierarchy/Components/mesh.h>
#include <QSplitter>
#include "qstandardpaths.h"

/**
 * @brief RuntimeEditor constructor
 * @param parent Parent widget
 */
RuntimeEditor::RuntimeEditor(QWidget *parent)
    : QMainWindow(parent)
{
    // Set window properties
    setWindowTitle("Runtime Editor");
    resize(1100, 600);

    // Configure dock widget features
    QDockWidget::DockWidgetFeatures dockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;

    // Initialize UI components
    setupMenuBar();
    setupToolBars();
    setupDockWidgets(dockFeatures);

    // Create runtime scenario and get core components
    Runtime *runtime = new Runtime();
    Hierarchy *hierarchy = runtime->hierarchy;
    SceneRenderer *renderer = runtime->scenerenderer;
    Simulation *simulation = runtime->simulation;
    Console *console = runtime->console;
    NetworkManager *networkManager = runtime->networkManager;
    library = runtime->Library;

    // Connect hierarchy signals to tactical display
    connect(renderer, &SceneRenderer::addMesh, tacticalDisplay, &TacticalDisplay::addMesh);
    connect(hierarchy, &Hierarchy::entityRemoved, tacticalDisplay, &TacticalDisplay::removeMesh);

    // Connect renderer to canvas
    connect(renderer, &SceneRenderer::Render, tacticalDisplay->canvas, &CanvasWidget::Render);

    // Connect inspector to hierarchy updates
    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);

    // ================= RUNTIME TOOLBAR CONNECTIONS =================

    if (runtimeToolBar && tacticalDisplay && tacticalDisplay->canvas && simulation ) {
        connect(runtimeToolBar, &RuntimeToolBar::startTriggered, [=]() {
            tacticalDisplay->canvas->simulation();
            simulation->start();
            runtime->recorder->startRecording();  // Start recording
            qDebug() << "Simulation started and recording started.";
        });

        //connect the stop botton to handle simulation stop and saved to file
        connect(runtimeToolBar, &RuntimeToolBar::stopTriggered, [=]() {
            tacticalDisplay->canvas->editor();
            simulation->stop();
            runtime->recorder->stopRecording();     // Stop recording
            runtime->recorder->recordToJson();      // Save JSON to file
            qDebug() << "Simulation stopped and recording saved.";
        });

        // Connect the pause button to handle simulation stop
        connect(runtimeToolBar, &RuntimeToolBar::pauseTriggered, this, [=]() {
            simulation->pause();  // Just pause
            qDebug() << "Simulation paused.";
        });

        // Connect replay button to load and replay file
        connect(runtimeToolBar, &RuntimeToolBar::replayTriggered, this, [=]() {
            simulation->stop();  // Stop the simulation before replay
            tacticalDisplay->canvas->Render(0.016f);  // Optional: clear or reset rendering

            QString filePath = QFileDialog::getOpenFileName(
                this,
                "Select Replay File",
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recordings",
                "*.json"
                );

            if (!filePath.isEmpty() && runtime->recorder->loadFromFile(filePath)) {
                QVector<QJsonObject> frames = runtime->recorder->getRecordedFrames();
                if (!frames.isEmpty()) {
                    simulation->replay(frames);  // Send frames to simulation for replay
                    qDebug() << "Replay started using file:" << filePath;
                } else {
                    qWarning() << "Replay file loaded but contains no frames.";
                }
            } else {
                qWarning() << "Replay cancelled or file failed to load.";
            }
        });

        connect(runtimeToolBar, &RuntimeToolBar::speedChanged, simulation, &Simulation::setSpeed);
        connect(runtimeToolBar, &RuntimeToolBar::speedChanged, this, [=](int speed) {
            float moveSpeed = static_cast<float>(speed);
            for (auto& [id, comp] : simulation->physicsComponent) {
                if (comp.dynamicModel) {
                    comp.dynamicModel->setMoveSpeed(moveSpeed);
                }
            }
            qDebug() << "Simulation speed set to:" << speed << "(moveSpeed:" << moveSpeed << ")";
        });
            networkToolBar->setNetworkManager(networkManager);
    } else {
        qWarning() << "Failed to connect RuntimeToolBar signals - nullptr detected";
    }
    // ================= CONSOLE VIEW CONNECTION =====================
    // connect(console, &Console::logUpdate, this, [=](std::string log) {
    //     consoleView->appendText(QString::fromStdString(log));
    // });
    consoleView->setConsoleDock(consoleDock);


    connect(console, &Console::logUpdate, this, [=](std::string log) {
        if (consoleView) {
            consoleView->appendLog(QString::fromStdString(log));
            consoleView->appendText(QString::fromStdString(log)); // Also append to Console tab
        }
    });

    connect(console, &Console::errorUpdate, this, [=](std::string error) {
        if (consoleView) {
            consoleView->appendError(QString::fromStdString(error));
            consoleView->appendText(QString::fromStdString(error)); // Also append to Console tab
        }
    });

    connect(console, &Console::warningUpdate, this, [=](std::string warning) {
        if (consoleView) {
            consoleView->appendWarning(QString::fromStdString(warning));
            consoleView->appendText(QString::fromStdString(warning)); // Also append to Console tab
        }
    });

    connect(console, &Console::debugUpdate, this, [=](std::string debug) {
        if (consoleView) {
            consoleView->appendDebug(QString::fromStdString(debug));
            consoleView->appendText(QString::fromStdString(debug)); // Also append to Console tab
        }
    });

    // Initialize hierarchy connections
    HierarchyConnector::instance()->connectSignals(hierarchy, treeView);
    HierarchyConnector::instance()->connectLibrarySignals(library, libTreeView);
    HierarchyConnector::initializeDummyData(hierarchy);
    HierarchyConnector::initializeLibraryData(library);
    HierarchyConnector::setupFileOperations(this, hierarchy);

    // Connect tree view item selection
    connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
        QString type = data["type"].toString();
        QString name = data["name"].toString();
        QString ID = data["parentId"].toString();

        // Update all open inspectors with selected item data
        for (Inspector* inspector : inspectors) {
            if (type == "component") {
                QJsonObject componentData = hierarchy->getComponentData(ID, name);
                if (!componentData.isEmpty()) {
                    inspector->init(ID, name, componentData);
                }
            }
            else if (type == "entity") {
                inspector->init(ID, name, QJsonObject());
            }
        }

        // Ensure inspector is visible when items are selected
        if (!inspectorDock->isVisible()) {
            addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
            splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
            inspectorDock->show();
        }

        // Update tactical display selection
        if (tacticalDisplay && type == "entity") {
            tacticalDisplay->selectedMesh(data["ID"].toString());
        }
    });

    // Connect library tree view item selection (currently empty)
    connect(libTreeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
        // TODO: Implement library item selection handling
    });

    // Connect inspector value changes to hierarchy updates
    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);

    // Setup inspector tab management
    connect(inspector, &Inspector::addTabRequested, this, &RuntimeEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);  // Track main inspector dock
    inspectors.append(inspector);          // Track main inspector instance
    inspector->setHierarchy(hierarchy);

    setupToolBarConnections();
}

/**
 * @brief Handles item selection from hierarchy tree
 * @param data Selected item data
 */
void RuntimeEditor::onItemSelected(QVariantMap data) {
    qDebug() << data["type"].toString();
    QString type = data["type"].toString();
    QString name = data["name"].toString();
    QString ID = data["parentId"].toString();

    if (type == "component") {
        inspector->init(ID, name, hierarchy->getComponentData(ID, name));
    } else {
        inspector->init(ID, name, QJsonObject());
    }

    if (type == "entity") {
        tacticalDisplay->selectedMesh(data["ID"].toString());
    }
}

/**
 * @brief Handles library item selection
 * @param data Selected item data
 */
void RuntimeEditor::onLibraryItemSelected(QVariantMap data) {
    qDebug() << "Library item selected:" << data;
    // TODO: Implement library item selection functionality
}

/**
 * @brief Sets up connections between toolbar actions and their handlers
 */
// void RuntimeEditor::setupToolBarConnections()
// {
//     // Get design toolbar and check components
//     DesignToolBar *designToolBar = findChild<DesignToolBar*>();
//     if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
//         qWarning() << "Failed to setup toolbar connections - missing components";
//         return;
//     }

//     // Connect save action to menu bar
//     MenuBar *menuBar = qobject_cast<MenuBar*>(this->menuBar());
//     if (menuBar) {
//         connect(designToolBar->getSaveAction(), &QAction::triggered,
//                 menuBar->getSaveAction(), &QAction::trigger);
//     }

//     // Connect transform mode changes
//     connect(designToolBar, &DesignToolBar::modeChanged,
//             this, [=](int mode) {
//                 tacticalDisplay->canvas->setTransformMode(static_cast<TransformMode>(mode));
//             });

//     // Connect grid control signals
//     connect(designToolBar, &DesignToolBar::gridPlaneXToggled,
//             tacticalDisplay->canvas, &CanvasWidget::setXGridVisible);
//     connect(designToolBar, &DesignToolBar::gridPlaneYToggled,
//             tacticalDisplay->canvas, &CanvasWidget::setYGridVisible);
//     connect(designToolBar, &DesignToolBar::gridOpacityChanged,
//             tacticalDisplay->canvas, &CanvasWidget::setGridOpacity);
//     connect(designToolBar, &DesignToolBar::layerOptionToggled,
//             tacticalDisplay->canvas, &CanvasWidget::toggleLayerVisibility);




// }
void RuntimeEditor::setupToolBarConnections()
{
    // Get design toolbar and check components
    DesignToolBar *designToolBar = findChild<DesignToolBar*>();
    if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
        qWarning() << "Failed to setup toolbar connections - missing components";
        return;
    }

    // Connect save action to menu bar
    MenuBar *menuBar = qobject_cast<MenuBar*>(this->menuBar());
    if (menuBar) {
        connect(designToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }

    // Connect transform mode changes
    connect(designToolBar, &DesignToolBar::modeChanged,
            this, [=](int mode) {
                tacticalDisplay->canvas->setTransformMode(static_cast<TransformMode>(mode));
            });

    // Connect grid control signals
    connect(designToolBar, &DesignToolBar::gridPlaneXToggled,
            tacticalDisplay->canvas, &CanvasWidget::setXGridVisible);
    connect(designToolBar, &DesignToolBar::gridPlaneYToggled,
            tacticalDisplay->canvas, &CanvasWidget::setYGridVisible);
    connect(designToolBar, &DesignToolBar::gridOpacityChanged,
            tacticalDisplay->canvas, &CanvasWidget::setGridOpacity);
    connect(designToolBar, &DesignToolBar::layerOptionToggled,
            tacticalDisplay->canvas, &CanvasWidget::toggleLayerVisibility);

    // Connect map-related signals (search place, zoom, and layer selection)
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
                    qDebug() << "RuntimeEditor received customMapAdded: name =" << name
                             << ", zoomMin =" << zoomMin << ", zoomMax =" << zoomMax
                             << ", url =" << url;
                });
        connect(designToolBar, &DesignToolBar::searchPlaceTriggered,
                tacticalDisplay->mapWidget, &GISlib::serachPlace); // Assuming GISlib has searchPlace slot
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
}
/**
 * @brief Sets up the menu bar
 */
void RuntimeEditor::setupMenuBar() {
    MenuBar *menuBar = new MenuBar(this);
    setMenuBar(menuBar);
    connect(menuBar, &MenuBar::feedbackTriggered, this, &RuntimeEditor::showFeedbackWindow);
}

/**
 * @brief Sets up all toolbars
 */
void RuntimeEditor::setupToolBars() {
    // Standard toolbar
    StandardToolBar *standardToolBar = new StandardToolBar(this);
    addToolBar(Qt::TopToolBarArea, standardToolBar);
    addToolBarBreak(Qt::TopToolBarArea);

    // Design toolbar
    DesignToolBar *designToolBar = new DesignToolBar(this);
    addToolBar(Qt::TopToolBarArea, designToolBar);

    // Runtime toolbar
    runtimeToolBar = new RuntimeToolBar(this);
    addToolBar(Qt::TopToolBarArea, runtimeToolBar);

    // Add a toolbar break to place NetworkToolbar on a new line
    addToolBarBreak(Qt::TopToolBarArea);

    // Network toolbar
    networkToolBar = new NetworkToolbar(this);
    addToolBar(Qt::TopToolBarArea, networkToolBar);

    // Connect runtime toolbar signals
    if (runtimeToolBar) {
        connect(runtimeToolBar, &RuntimeToolBar::speedChanged, this, [](int speed) {
            qDebug() << "Speed changed to:" << speed;
        });
    }

    // Connect menu bar save action to design toolbar
    MenuBar *menuBar = qobject_cast<MenuBar*>(this->menuBar());
    if (menuBar && designToolBar) {
        connect(designToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }
}

/**
 * @brief Sets up all dock widgets
 * @param dockFeatures Features for dock widgets
 */
void RuntimeEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures) {
    // ================= HIERARCHY DOCK =================
    QDockWidget *hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    hierarchyDock->setFeatures(dockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // ================= NAVIGATION DOCK =================
    QDockWidget *navigationDock = new QDockWidget("Navigation", this);
    navigationDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    navigationDock->setFeatures(dockFeatures);
    NavigationPage *navPage = new NavigationPage(this);
    navigationDock->setWidget(navPage);
    addDockWidget(Qt::LeftDockWidgetArea, navigationDock);

    // ================= TACTICAL DISPLAY DOCK =================
    tacticalDisplayDock = new QDockWidget("Tactical Display", this);
    tacticalDisplayDock->setAllowedAreas(Qt::RightDockWidgetArea);
    tacticalDisplayDock->setFeatures(dockFeatures);
    tacticalDisplay = new TacticalDisplay(this);
    tacticalDisplayDock->setWidget(tacticalDisplay);
    addDockWidget(Qt::RightDockWidgetArea, tacticalDisplayDock);
    tacticalDisplayDock->setMinimumWidth(200);
    tacticalDisplayDock->setMaximumWidth(1200);

    // Adjust splitter sizes
    QSplitter *mainSplitter = findChild<QSplitter*>();
    if (mainSplitter) {
        mainSplitter->setSizes(QList<int>() << 300 << 700);
    }

    // Sidebar on right of Tactical Display
    sidebarDock = new QDockWidget("", this);
    sidebarDock->setAllowedAreas(Qt::RightDockWidgetArea);
    sidebarDock->setFeatures(dockFeatures);
    SidebarWidget *sidebar = new SidebarWidget(this);
    sidebarDock->setTitleBarWidget(new QWidget());
    sidebarDock->setWidget(sidebar);
    sidebarDock->setMinimumWidth(80);
    sidebarDock->resize(80, sidebarDock->height());
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);

    // Console at bottom of Tactical Display
    consoleDock = new QDockWidget("", this);
    consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    consoleDock->setFeatures(dockFeatures);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    // Utility docks (Inspector/Library)
    libraryDock = new QDockWidget("Library", this);
    libraryDock->setAllowedAreas(Qt::RightDockWidgetArea);
    libraryDock->setFeatures(dockFeatures);
    libTreeView = new HierarchyTree(this);
    libraryDock->setWidget(libTreeView);
    libraryDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, libraryDock);
    libraryDock->hide();

    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    inspectorDock->setFeatures(dockFeatures);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(200);
    inspectorDock->hide();

    // Layout setup
    splitDockWidget(hierarchyDock, navigationDock, Qt::Vertical);
    splitDockWidget(tacticalDisplayDock, sidebarDock, Qt::Horizontal);
    splitDockWidget(tacticalDisplayDock, consoleDock, Qt::Vertical);

    connect(sidebar, &SidebarWidget::viewSelected, this, [this](const QString &viewName) {
        if (viewName == "Inspector") {
            for (QDockWidget* inspectorDock : inspectorDocks) {
                if (!inspectorDock->isVisible()) {
                    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
                    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
                    inspectorDock->show();
                }
            }
            if (libraryDock->isVisible()) {
                removeDockWidget(libraryDock);
                libraryDock->hide();
            }
        }
        else if (viewName == "Library") {
            for (QDockWidget* inspectorDock : inspectorDocks) {
                if (inspectorDock->isVisible()) {
                    removeDockWidget(inspectorDock);
                    inspectorDock->hide();
                }
            }
            if (!libraryDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, libraryDock);
                splitDockWidget(sidebarDock, libraryDock, Qt::Vertical);
                libraryDock->show();
            }
        }
        else if (viewName == "Console") {
            consoleDock->setVisible(!consoleDock->isVisible());
        }
    });

    // Show Inspector by default
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
    inspectorDock->show();
}

/**
 * @brief Adds a new inspector tab
 */
void RuntimeEditor::addInspectorTab()
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
            this, &RuntimeEditor::addInspectorTab);

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

void RuntimeEditor::showFeedbackWindow()
{
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->show();
}

/**
 * @brief RuntimeEditor destructor
 */
RuntimeEditor::~RuntimeEditor() {
    // Cleanup resources if needed
}
