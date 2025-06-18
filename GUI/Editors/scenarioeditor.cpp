
#include "../../GUI/Editors/scenarioeditor.h"
#include "GUI/Menubars/menubar.h"
#include "GUI/Navigation/navigationpage.h"
#include "GUI/Overview/overview.h"
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

ScenarioEditor::ScenarioEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Scenario Editor");
    resize(1100, 600);

    QDockWidget::DockWidgetFeatures dockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;

    setupMenuBar();
    setupToolBars();
    setupDockWidgets(dockFeatures);

    Scenario *scenario = new Scenario();
    Hierarchy *hierarchy = scenario->hierarchy;
    SceneRenderer *renderer = scenario->scenerenderer;
    Console *console = scenario->console;

    HierarchyConnector::instance()->setHierarchy(hierarchy);
    HierarchyConnector::instance()->setLibrary(library);
    HierarchyConnector::instance()->setLibTreeView(libTreeView);
    library = scenario->Library;

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

    // Connect CanvasWidget signals
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(tacticalDisplay->canvas, &CanvasWidget::selectEntitybyCursor, inspector, [=](QString /*ID*/) {
            // TODO: Implement entity selection logic
        });

        // Connect trajectory updated signals
        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated, inspector, &Inspector::updateTrajectory);

        // Remove redundant SIGNAL/SLOT syntax (already connected above)
        // connect(tacticalDisplay->canvas, SIGNAL(trajectoryUpdated(QString,QJsonArray)), inspector, SLOT(updateTrajectory(QString,QJsonArray)));

        connect(tacticalDisplay->canvas, &CanvasWidget::trajectoryUpdated, this, [=](QString entityId, QJsonArray /*waypoints*/) {
            auto it = tacticalDisplay->canvas->Meshes.find(entityId.toStdString());
            if (it != tacticalDisplay->canvas->Meshes.end() && it->second.trajectory) {
                QJsonObject trajData = it->second.trajectory->toJson();
                hierarchy->UpdateComponent(entityId, "Trajectory", trajData);
                Console::log("Trajectory updated for entity: " + entityId.toStdString());
                treeView->getTreeWidget()->update();
            } else {
                Console::error("Failed to update trajectory for entity: " + entityId.toStdString() + " - entity or trajectory not found");
            }
        });

        // Connect Inspector's trajectoryWaypointsChanged to CanvasWidget's updateWaypointsFromInspector
        connect(inspector, &Inspector::trajectoryWaypointsChanged, tacticalDisplay->canvas, &CanvasWidget::updateWaypointsFromInspector);
    }

    connect(renderer, &SceneRenderer::addMesh, tacticalDisplay, &TacticalDisplay::addMesh);
    connect(hierarchy, &Hierarchy::entityRemoved, tacticalDisplay, &TacticalDisplay::removeMesh);

    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->canvas, &CanvasWidget::Render);
    }

    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);

    HierarchyConnector::instance()->connectSignals(hierarchy, treeView);
    HierarchyConnector::instance()->connectLibrarySignals(library, libTreeView);
    HierarchyConnector::initializeDummyData(hierarchy);
    HierarchyConnector::initializeLibraryData(library);
    HierarchyConnector::setupFileOperations(this, hierarchy);

    connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
        QString type = data["type"].toString();
        QString name = data["name"].toString();
        QString ID = data["parentId"].toString();

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

        if (!inspectorDock->isVisible()) {
            addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
            splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
            inspectorDock->show();
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
}

void ScenarioEditor::setupToolBarConnections()
{
    DesignToolBar *designToolBar = findChild<DesignToolBar*>();
    if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
        qWarning() << "Toolbar connection setup failed - required components missing";
        return;
    }

    MenuBar *menuBar = qobject_cast<MenuBar*>(this->menuBar());
    if (menuBar) {
        connect(standardToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }

    connect(designToolBar, &DesignToolBar::modeChanged,
            this, [=](int mode) {
                tacticalDisplay->canvas->setTransformMode(static_cast<TransformMode>(mode));
            });

    connect(designToolBar, &DesignToolBar::gridPlaneXToggled,
            tacticalDisplay->canvas, &CanvasWidget::setXGridVisible);
    connect(designToolBar, &DesignToolBar::gridPlaneYToggled,
            tacticalDisplay->canvas, &CanvasWidget::setYGridVisible);
    connect(designToolBar, &DesignToolBar::gridOpacityChanged,
            tacticalDisplay->canvas, &CanvasWidget::setGridOpacity);

    connect(designToolBar, &DesignToolBar::layerOptionToggled,
            tacticalDisplay->canvas, &CanvasWidget::toggleLayerVisibility);

    if (tacticalDisplay && tacticalDisplay->mapWidget) {
        connect(designToolBar, &DesignToolBar::mapLayerChanged,
                tacticalDisplay->mapWidget, &GISlib::setLayer,
                Qt::QueuedConnection);
        connect(designToolBar, &DesignToolBar::searchPlaceTriggered,
                tacticalDisplay->mapWidget, &GISlib::serachPlace);
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
}

void ScenarioEditor::setupMenuBar()
{
    MenuBar *menuBar = new MenuBar(this);
    setMenuBar(menuBar);
    connect(menuBar, &MenuBar::feedbackTriggered, this, &ScenarioEditor::showFeedbackWindow);
}

void ScenarioEditor::setupToolBars()
{
    standardToolBar = new StandardToolBar(this);
    addToolBar(Qt::TopToolBarArea, standardToolBar);
    addToolBarBreak(Qt::TopToolBarArea);

    DesignToolBar *designToolBar = new DesignToolBar(this);
    addToolBar(Qt::TopToolBarArea, designToolBar);
}

void ScenarioEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    QDockWidget *hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    hierarchyDock->setFeatures(dockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    QDockWidget *navigationDock = new QDockWidget("Navigation", this);
    navigationDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    navigationDock->setFeatures(dockFeatures);
    NavigationPage *navPage = new NavigationPage(this);
    navigationDock->setWidget(navPage);
    navigationDock->setMinimumWidth(150);
    addDockWidget(Qt::LeftDockWidgetArea, navigationDock);

    tacticalDisplayDock = new QDockWidget("Tactical Display", this);
    tacticalDisplayDock->setAllowedAreas(Qt::RightDockWidgetArea);
    tacticalDisplayDock->setFeatures(dockFeatures);
    tacticalDisplay = new TacticalDisplay(this);
    tacticalDisplayDock->setWidget(tacticalDisplay);
    tacticalDisplayDock->setMinimumWidth(200);
    tacticalDisplayDock->setMaximumWidth(1400);
    addDockWidget(Qt::RightDockWidgetArea, tacticalDisplayDock);

    QSplitter *mainSplitter = findChild<QSplitter*>();
    if (mainSplitter) {
        mainSplitter->setSizes(QList<int>() << 300 << 700);
    }

    sidebarDock = new QDockWidget("", this);
    sidebarDock->setAllowedAreas(Qt::RightDockWidgetArea);
    sidebarDock->setFeatures(dockFeatures);
    SidebarWidget *sidebar = new SidebarWidget(this);
    sidebarDock->setTitleBarWidget(new QWidget());
    sidebarDock->setWidget(sidebar);
    sidebarDock->setMinimumWidth(20);
    sidebarDock->resize(80, sidebarDock->height());
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);

    consoleDock = new QDockWidget("", this);
    consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    consoleDock->setFeatures(dockFeatures);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

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
    inspectorDock->setMinimumWidth(100);
    inspectorDock->hide();

    splitDockWidget(hierarchyDock, navigationDock, Qt::Vertical);
    splitDockWidget(tacticalDisplayDock, sidebarDock, Qt::Horizontal);
    splitDockWidget(tacticalDisplayDock, consoleDock, Qt::Vertical);

    connect(sidebar, &SidebarWidget::viewSelected, this, [this](const QString &viewName) {
        if (viewName == "Inspector") {
            if (libraryDock->isVisible()) {
                removeDockWidget(libraryDock);
                libraryDock->hide();
            }
            for (QDockWidget* inspectorDock : inspectorDocks) {
                if (!inspectorDock->isVisible()) {
                    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
                    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
                    inspectorDock->show();
                }
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

    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
    inspectorDock->show();
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
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->show();
}

void ScenarioEditor::onItemSelected(QVariantMap /*data*/) {
    // TODO: Implement item selection logic
}

void ScenarioEditor::onLibraryItemSelected(QVariantMap /*data*/) {
    // TODO: Implement library item selection logic
}

ScenarioEditor::~ScenarioEditor()
{
    // Cleanup managed by Qt's parent-child relationships
}
