#include "databaseeditor.h"
#include "GUI/Feedback/feedback.h"
#include "GUI/Console/consoleview.h"
#include "GUI/Menubars/menubar.h"
#include "GUI/Toolbars/standardtoolbar.h"
#include <core/structure/scenario.h>
#include <QDockWidget>
#include <QSplitter>

DatabaseEditor::DatabaseEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Database Editor");
    resize(1100, 600);

    QDockWidget::DockWidgetFeatures dockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;

    setupMenuBar();
    setupToolBars();
    setupDockWidgets(dockFeatures);

    // Initialize core components
    scenario = new Scenario();
    hierarchy = scenario->hierarchy;
    console = scenario->console;

    consoleView->setConsoleDock(consoleDock);

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

    // Initialize hierarchy
    if (hierarchy && treeView) {
        HierarchyConnector::instance()->connectSignals(hierarchy, treeView);
        HierarchyConnector::initializeDummyData(hierarchy);
        HierarchyConnector::setupFileOperations(this, hierarchy,nullptr);
    }

    // Connect hierarchy tree selection to inspector
    if (treeView && hierarchy) {
        connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {

            QString type = data["type"].toString();
            QString name = data["name"].toString();
            QString ID = data["parentId"].toString();

            // Update all inspectors with selected item data
            for (Inspector* inspector : inspectors) {
                if (type == "component") {
                    QJsonObject componentData = hierarchy->getComponentData(ID, name);
                    if (!componentData.isEmpty()) {
                        inspector->init(ID, name, componentData);
                    }
                }
                else
                if(type == "profile"){
                    inspector->init(ID, name+"_self", (hierarchy->ProfileCategories)[data["ID"].toString().toStdString()]->toJson());

                }else
                if(type == "folder"){
                    inspector->init(ID, name+"_self", (*hierarchy->Folders)[data["ID"].toString().toStdString()]->toJson());

                }else
                if(type == "entity"){
                    inspector->init(data["ID"].toString(), name+"_self", (*hierarchy->Entities)[data["ID"].toString().toStdString()]->toJson());

                }else{
                    inspector->init(ID, name, QJsonObject());
                }
            }

            // Ensure inspector is visible when items are selected
            if (!inspectorDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
                inspectorDock->show();
            }
        });
    }

    // Connect inspector changes
    if (inspector && hierarchy) {
        connect(inspector, &Inspector::valueChanged,
                hierarchy, &Hierarchy::UpdateComponent);
    }

    // Setup inspector tab management
    connect(inspector, &Inspector::addTabRequested, this, &DatabaseEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
    inspector->setHierarchy(hierarchy);
}

void DatabaseEditor::setupMenuBar()
{
    MenuBar *menuBar = new MenuBar(this);
    setMenuBar(menuBar);

    // Connect the feedbackTriggered signal to the showFeedbackWindow slot
    connect(menuBar, &MenuBar::feedbackTriggered, this, &DatabaseEditor::showFeedbackWindow);
}

void DatabaseEditor::setupToolBars()
{
    StandardToolBar *standardToolBar = new StandardToolBar(this);
    addToolBar(Qt::TopToolBarArea, standardToolBar);
}

void DatabaseEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    // Hierarchy Dock
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    hierarchyDock->setFeatures(dockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // Inspector Dock
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    inspectorDock->setFeatures(dockFeatures);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // Console Dock
    consoleDock = new QDockWidget("", this);
    consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    consoleDock->setFeatures(dockFeatures);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    // Layout configuration
    splitDockWidget(hierarchyDock, inspectorDock, Qt::Horizontal);
    splitDockWidget(inspectorDock, consoleDock, Qt::Vertical);

    // Calculate and set initial sizes (30% for hierarchy, 70% for inspector)
    int totalWidth = width();
    int hierarchyWidth = static_cast<int>(totalWidth * 0.3);
    int inspectorWidth = totalWidth - hierarchyWidth;

    // Resize the docks
    resizeDocks({hierarchyDock, inspectorDock}, {hierarchyWidth, inspectorWidth}, Qt::Horizontal);
    resizeDocks({consoleDock}, {150}, Qt::Vertical);
}

void DatabaseEditor::addInspectorTab()
{
    // Create new dock widget
    QDockWidget *newInspectorDock = new QDockWidget("Inspector " + QString::number(++inspectorCount), this);
    newInspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);

    // Create new inspector
    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);

    // Track new inspector
    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);

    // Connect signals
    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::addTabRequested,
            this, &DatabaseEditor::addInspectorTab);

    // Position new inspector
    if (inspectorDock->isVisible()) {
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }

    // Handle cleanup
    connect(newInspectorDock, &QDockWidget::destroyed, this, [=]() {
        inspectorDocks.removeOne(newInspectorDock);
        inspectors.removeOne(newInspector);
    });
}

void DatabaseEditor::showFeedbackWindow()
{
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->show();
}

DatabaseEditor::~DatabaseEditor()
{
    // Cleanup
    if (scenario) {
        delete scenario;
    }
}
