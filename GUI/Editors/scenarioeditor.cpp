
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
#include <core/Components/transform.h>
#include <core/Components/mesh.h>


/*
 * ScenarioEditor Constructor
 * Initializes the main editor window with all UI components
 * and sets up core functionality connections
 */
ScenarioEditor::ScenarioEditor(QWidget *parent)
    : QMainWindow(parent)
{
    // Configure main window properties
    setWindowTitle("Scenario Editor");
    resize(1100, 600); // Initial window dimensions

    // Define docking widget behavior flags
    QDockWidget::DockWidgetFeatures dockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;

    // Initialize UI components
    setupMenuBar();     // Create main menu bar
    setupToolBars();    // Setup application toolbars
    setupDockWidgets(dockFeatures); // Configure docking areas

    // Create and initialize core application components
    Scenario *scenario = new Scenario();
    Hierarchy *hierarchy = scenario->hierarchy;
    SceneRenderer *renderer = scenario->scenerenderer;
    Console *console = scenario->console;

    // Configure hierarchy connector with essential components
    HierarchyConnector::instance()->setHierarchy(hierarchy);
    HierarchyConnector::instance()->setLibrary(library);
    HierarchyConnector::instance()->setLibTreeView(libTreeView);
    library = scenario->Library;

    // Connect console output to display
    connect(console, &Console::logUpdate, this, [=](std::string log) {
        consoleView->appendText(QString::fromStdString(log));
    });

    // Setup scene rendering connections
    connect(renderer, &SceneRenderer::addMesh, tacticalDisplay, &TacticalDisplay::addMesh);
    connect(hierarchy, &Hierarchy::entityRemoved, tacticalDisplay, &TacticalDisplay::removeMesh);

    // Connect renderer to canvas if available
    if (tacticalDisplay && tacticalDisplay->canvas) {
        connect(renderer, &SceneRenderer::Render, tacticalDisplay->canvas, &CanvasWidget::Render);
    }

    // Connect inspector changes to hierarchy updates
    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);

    // Initialize hierarchy data and connections
    HierarchyConnector::instance()->connectSignals(hierarchy, treeView);
    HierarchyConnector::instance()->connectLibrarySignals(library, libTreeView);
    HierarchyConnector::initializeDummyData(hierarchy);
    HierarchyConnector::initializeLibraryData(library);
    HierarchyConnector::setupFileOperations(this, hierarchy);

    /*
     * Handle selection changes in the hierarchy tree
     * Updates inspectors and tactical display when items are selected
     */
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

    // Setup inspector tab management
    connect(inspector, &Inspector::addTabRequested, this, &ScenarioEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);  // Track main inspector dock
    inspectors.append(inspector);          // Track main inspector instance
    inspector->setHierarchy(hierarchy);    // Provide hierarchy reference

    // Complete toolbar setup
    setupToolBarConnections();
}

/*
 * Configures toolbar button connections and behavior
 */
void ScenarioEditor::setupToolBarConnections()
{
    DesignToolBar *designToolBar = findChild<DesignToolBar*>();
    if (!designToolBar || !tacticalDisplay || !tacticalDisplay->canvas) {
        qWarning() << "Toolbar connection setup failed - required components missing";
        return;
    }

    // Connect save action between toolbar and menu
    MenuBar *menuBar = qobject_cast<MenuBar*>(this->menuBar());
    if (menuBar) {
        connect(designToolBar->getSaveAction(), &QAction::triggered,
                menuBar->getSaveAction(), &QAction::trigger);
    }

    // Transform mode handling
    connect(designToolBar, &DesignToolBar::modeChanged,
            this, [=](int mode) {
                tacticalDisplay->canvas->setTransformMode(static_cast<TransformMode>(mode));
            });

    // Grid display configuration
    connect(designToolBar, &DesignToolBar::gridPlaneXToggled,
            tacticalDisplay->canvas, &CanvasWidget::setXGridVisible);
    connect(designToolBar, &DesignToolBar::gridPlaneYToggled,
            tacticalDisplay->canvas, &CanvasWidget::setYGridVisible);
    connect(designToolBar, &DesignToolBar::gridOpacityChanged,
            tacticalDisplay->canvas, &CanvasWidget::setGridOpacity);

    // Layer visibility management
    connect(designToolBar, &DesignToolBar::layerOptionToggled,
            tacticalDisplay->canvas, &CanvasWidget::toggleLayerVisibility);

    if (tacticalDisplay && tacticalDisplay->mapWidget) {
        connect(designToolBar, &DesignToolBar::mapLayerChanged,
                tacticalDisplay->mapWidget, &GISlib::setLayer,
                Qt::QueuedConnection); // Use queued connection for safety
    } else {
        qCritical() << "Map widget not available for layer connections";
    }
    connect(designToolBar, &DesignToolBar::searchPlaceTriggered,
            tacticalDisplay->mapWidget, &GISlib::serachPlace);

    connect(designToolBar, &DesignToolBar::selectCenterTriggered, [this]() {
        if (tacticalDisplay && tacticalDisplay->mapWidget) {
            // Center on current position or default position
            tacticalDisplay->mapWidget->setCenter(0, 0);
        }
    });


    connect(designToolBar->zoomInAction, &QAction::triggered,
            tacticalDisplay, &TacticalDisplay::zoomIn);
    connect(designToolBar->zoomOutAction, &QAction::triggered,
            tacticalDisplay, &TacticalDisplay::zoomOut);


}

/*
 * Creates and configures the main menu bar
 */
void ScenarioEditor::setupMenuBar()
{
    MenuBar *menuBar = new MenuBar(this);
    setMenuBar(menuBar);
    connect(menuBar, &MenuBar::feedbackTriggered, this, &ScenarioEditor::showFeedbackWindow);
}

/*
 * Initializes application toolbars
 */
void ScenarioEditor::setupToolBars()
{
    // Create and add standard toolbar
    StandardToolBar *standardToolBar = new StandardToolBar(this);
    addToolBar(Qt::TopToolBarArea, standardToolBar);
    addToolBarBreak(Qt::TopToolBarArea); // Separate toolbar groups

    // Create and add design-specific toolbar
    DesignToolBar *designToolBar = new DesignToolBar(this);
    addToolBar(Qt::TopToolBarArea, designToolBar);
}

/*
 * Configures docking widgets and layout
 * @param dockFeatures: Flags controlling docking widget behavior
 */
void ScenarioEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    /* Left Dock Area - Hierarchy and Navigation */

    // Hierarchy tree dock
    QDockWidget *hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    hierarchyDock->setFeatures(dockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // Navigation panel dock
    QDockWidget *navigationDock = new QDockWidget("Navigation", this);
    navigationDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    navigationDock->setFeatures(dockFeatures);
    NavigationPage *navPage = new NavigationPage(this);
    navigationDock->setWidget(navPage);
    navigationDock->setMinimumWidth(150);
    addDockWidget(Qt::LeftDockWidgetArea, navigationDock);

    /* Central Area - Tactical Display */

    tacticalDisplayDock = new QDockWidget("Tactical Display", this);
    tacticalDisplayDock->setAllowedAreas(Qt::RightDockWidgetArea);
    tacticalDisplayDock->setFeatures(dockFeatures);
    tacticalDisplay = new TacticalDisplay(this);
    tacticalDisplayDock->setWidget(tacticalDisplay);
    tacticalDisplayDock->setMinimumWidth(200);
    tacticalDisplayDock->setMaximumWidth(1400);
    addDockWidget(Qt::RightDockWidgetArea, tacticalDisplayDock);

    // Configure main area splitter sizes
    QSplitter *mainSplitter = findChild<QSplitter*>();
    if (mainSplitter) {
        mainSplitter->setSizes(QList<int>() << 300 << 700); // Left/Right allocation
    }

    /* Right Dock Area - Sidebar and Inspector */

    // Sidebar dock (minimized by default)
    sidebarDock = new QDockWidget("", this);
    sidebarDock->setAllowedAreas(Qt::RightDockWidgetArea);
    sidebarDock->setFeatures(dockFeatures);
    SidebarWidget *sidebar = new SidebarWidget(this);
    sidebarDock->setTitleBarWidget(new QWidget()); // Hide title bar
    sidebarDock->setWidget(sidebar);
    sidebarDock->setMinimumWidth(20);
    sidebarDock->resize(80, sidebarDock->height());
    addDockWidget(Qt::RightDockWidgetArea, sidebarDock);

    /* Bottom Dock Area - Console */

    consoleDock = new QDockWidget("", this);
    consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    consoleDock->setFeatures(dockFeatures);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    /* Utility Docks - Library and Inspector */

    // Library dock (hidden by default)
    libraryDock = new QDockWidget("Library", this);
    libraryDock->setAllowedAreas(Qt::RightDockWidgetArea);
    libraryDock->setFeatures(dockFeatures);
    libTreeView = new HierarchyTree(this);
    libraryDock->setWidget(libTreeView);
    libraryDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, libraryDock);
    libraryDock->hide();

    // Inspector dock (shown by default)
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    inspectorDock->setFeatures(dockFeatures);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(100);
    inspectorDock->hide();

    /* Layout Configuration */

    // 1. Vertical split for left docks
    splitDockWidget(hierarchyDock, navigationDock, Qt::Vertical);

    // 2. Horizontal split for main area and sidebar
    splitDockWidget(tacticalDisplayDock, sidebarDock, Qt::Horizontal);

    // 3. Vertical split for main area and console
    splitDockWidget(tacticalDisplayDock, consoleDock, Qt::Vertical);

    /* View Management */

    // Handle view selection from sidebar
    connect(sidebar, &SidebarWidget::viewSelected, this, [this](const QString &viewName) {
        if (viewName == "Inspector") {
            // Show all inspectors, hide library
            if (libraryDock->isVisible()) {
                removeDockWidget(libraryDock);
                libraryDock->hide();
            }
            // Show all inspector docks
            for (QDockWidget* inspectorDock : inspectorDocks) {
                if (!inspectorDock->isVisible()) {
                    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
                    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
                    inspectorDock->show();
                }
            }
        }
        else if (viewName == "Library") {
            // Hide all inspectors, show library
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
            // Toggle console visibility
            consoleDock->setVisible(!consoleDock->isVisible());
        }
    });

    // Default view configuration
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    splitDockWidget(sidebarDock, inspectorDock, Qt::Vertical);
    inspectorDock->show();
}

/*
 * Creates a new inspector tab/dock
 */
void ScenarioEditor::addInspectorTab()
{
    // Create new dock widget
    QDockWidget *newInspectorDock = new QDockWidget("Inspector " + QString::number(++inspectorCount), this);
    newInspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);

    // Create and configure new inspector instance
    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);

    // Track new inspector instances
    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);

    // Connect inspector signals
    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::addTabRequested,
            this, &ScenarioEditor::addInspectorTab);

    // Position new inspector dock
    if (inspectorDock->isVisible()) {
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }

    // Handle dock cleanup
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
void ScenarioEditor::onItemSelected(QVariantMap data) {
    // Implementation would go here
}

void ScenarioEditor::onLibraryItemSelected(QVariantMap data) {
    // Implementation would go here
}
/*
 * Cleanup resources on window close
 */
ScenarioEditor::~ScenarioEditor()
{
    // TODO: Implement cleanup logic for all allocated resources
}
