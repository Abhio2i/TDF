/* ========================================================================= */
/* File: databaseeditor.cpp                                                 */
/* Purpose: Implements database editor with hierarchy and inspector views    */
/* ========================================================================= */

#include "databaseeditor.h"                        // For database editor class
#include "GUI/Feedback/feedback.h"                // For feedback window
#include "GUI/Console/consoleview.h"              // For console view
#include "GUI/Menubars/menubar.h"                 // For menu bar
#include "GUI/Toolbars/standardtoolbar.h"         // For standard toolbar
#include <core/structure/scenario.h>              // For scenario structure
#include <QDockWidget>                            // For dock widget
#include <QSplitter>                              // For splitter widget
#include <QMenuBar>                               // For menu bar
#include <QApplication>                           // For application instance

// %%% Constructor %%%
/* Initialize database editor */
DatabaseEditor::DatabaseEditor(QWidget *parent)
    : QMainWindow(parent)
{
    // Set window title
    setWindowTitle("Database Editor");
    // Set window size
    resize(1100, 600);
    // Define dock widget features
    QDockWidget::DockWidgetFeatures dockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;

    // Setup UI components
    setupMenuBar();
    setupToolBars();
    setupDockWidgets(dockFeatures);
    setupStatusBar();

    // Initialize scenario
    scenario = new Scenario();
    hierarchy = scenario->hierarchy;
    console = scenario->console;
    lastSavedFilePath = "";

    // Set console dock
    consoleView->setConsoleDock(consoleDock);

    // Connect console log signals
    connect(console, &Console::logUpdate, this, [=](std::string log) {
        if (consoleView) {
            consoleView->appendLog(QString::fromStdString(log));
            consoleView->appendText(QString::fromStdString(log));
        }
    });
    // Connect console error signals
    connect(console, &Console::errorUpdate, this, [=](std::string error) {
        if (consoleView) {
            consoleView->appendError(QString::fromStdString(error));
            consoleView->appendText(QString::fromStdString(error));
        }
    });
    // Connect console warning signals
    connect(console, &Console::warningUpdate, this, [=](std::string warning) {
        if (consoleView) {
            consoleView->appendWarning(QString::fromStdString(warning));
            consoleView->appendText(QString::fromStdString(warning));
        }
    });
    // Connect console debug signals
    connect(console, &Console::debugUpdate, this, [=](std::string debug) {
        if (consoleView) {
            consoleView->appendDebug(QString::fromStdString(debug));
            consoleView->appendText(QString::fromStdString(debug));
        }
    });

    // Connect hierarchy signals for unsaved changes
    connect(hierarchy, &Hierarchy::profileAdded, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderAdded, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityAdded, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentAdded, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRemoved, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRemoved, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRemoved, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentRemoved, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRenamed, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRenamed, this, &DatabaseEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRenamed, this, &DatabaseEditor::markUnsavedChanges);
    connect(inspector, &Inspector::valueChanged, this, &DatabaseEditor::markUnsavedChanges);

    // Connect hierarchy and tree view
    if (hierarchy && treeView) {
        HierarchyConnector::instance()->connectSignals(hierarchy, treeView);
        HierarchyConnector::instance()->initializeDummyData(hierarchy);
        HierarchyConnector::instance()->setupFileOperations(this, hierarchy, nullptr);
    }

    // Connect tree view item selection
    if (treeView && hierarchy) {
        connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
            QString type;
            // Handle nested type data
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
            // Extract item data
            QString name = data["name"].toString();
            QString ID = data["parentId"].toString();
            // Update inspectors
            for (Inspector* inspector : inspectors) {
                // Skip locked inspectors (commented)
                // if (inspector->isLocked()) {
                //     continue;
                // }
                // Initialize inspector based on type
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
            // Show inspector dock if hidden
            if (!inspectorDock->isVisible()) {
                addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
                inspectorDock->show();
            }
        });
    }

    // Connect inspector to hierarchy
    if (inspector && hierarchy) {
        connect(inspector, &Inspector::valueChanged,
                hierarchy, &Hierarchy::UpdateComponent);
    }

    // Connect inspector signals
    connect(inspector, &Inspector::addTabRequested, this, &DatabaseEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
    inspector->setHierarchy(hierarchy);

    // Connect menu bar actions
    MenuBar* menuBar = qobject_cast<MenuBar*>(this->menuBar());
    if (menuBar) {
        connect(menuBar->getSaveAction(), &QAction::triggered, this, &DatabaseEditor::clearUnsavedChanges);
        connect(menuBar->getSameSaveAction(), &QAction::triggered, this, &DatabaseEditor::clearUnsavedChanges);
    } else {
        qWarning() << "Failed to cast menuBar to MenuBar in DatabaseEditor";
    }
    // Connect exit action
    connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
}

/* Setup menu bar */
void DatabaseEditor::setupMenuBar()
{
    // Create and set menu bar
    MenuBar *menuBar = new MenuBar(this);
    setMenuBar(menuBar);
    // Connect feedback trigger
    connect(menuBar, &MenuBar::feedbackTriggered, this, &DatabaseEditor::showFeedbackWindow);
}

/* Setup toolbars */
void DatabaseEditor::setupToolBars()
{
    // Create and add standard toolbar
    StandardToolBar *standardToolBar = new StandardToolBar(this);
    addToolBar(Qt::TopToolBarArea, standardToolBar);
}

/* Setup dock widgets */
void DatabaseEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    // Setup hierarchy dock
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    hierarchyDock->setFeatures(dockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // Setup inspector dock
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setAllowedAreas(Qt::RightDockWidgetArea);
    inspectorDock->setFeatures(dockFeatures);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // Setup console dock
    consoleDock = new QDockWidget("", this);
    consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    consoleDock->setFeatures(dockFeatures);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    // Split docks
    splitDockWidget(hierarchyDock, inspectorDock, Qt::Horizontal);
    splitDockWidget(inspectorDock, consoleDock, Qt::Vertical);

    // Resize docks
    int totalWidth = width();
    int hierarchyWidth = static_cast<int>(totalWidth * 0.3);
    int inspectorWidth = totalWidth - hierarchyWidth;
    resizeDocks({hierarchyDock, inspectorDock}, {hierarchyWidth, inspectorWidth}, Qt::Horizontal);
    resizeDocks({consoleDock}, {150}, Qt::Vertical);
}

/* Add new inspector tab */
void DatabaseEditor::addInspectorTab()
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
    connect(newInspector, &Inspector::valueChanged,
            this, &DatabaseEditor::markUnsavedChanges);
    connect(newInspector, &Inspector::addTabRequested,
            this, &DatabaseEditor::addInspectorTab);
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
void DatabaseEditor::showFeedbackWindow()
{
    // Create and show feedback window
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->h = hierarchy;
    feedbackWindow->loadDashboardData("{}");
    feedbackWindow->show();
}

/* Destructor */
DatabaseEditor::~DatabaseEditor()
{
    // Clean up scenario
    if (scenario) {
        delete scenario;
    }
}

/* Mark unsaved changes */
void DatabaseEditor::markUnsavedChanges()
{
    // Update unsaved changes state
    if (!hasUnsavedChanges) {
        hasUnsavedChanges = true;
        emit unsavedChangesChanged(true);
        setWindowTitle("Database Editor *");
    }
}

/* Clear unsaved changes */
void DatabaseEditor::clearUnsavedChanges()
{
    // Reset unsaved changes state
    if (hasUnsavedChanges) {
        hasUnsavedChanges = false;
        emit unsavedChangesChanged(false);
        setWindowTitle("Database Editor");
    }
}

/* Setup status bar */
void DatabaseEditor::setupStatusBar()
{
    // Create and set status bar
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");
}

/* Update status bar message */
void DatabaseEditor::updateStatusBar(const QString &message)
{
    // Update status bar
    if (statusBar) {
        statusBar->showMessage(message);
    }
}
