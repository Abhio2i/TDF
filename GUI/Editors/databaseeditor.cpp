
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
#include <QSettings>                              // For saving dock state
#include <QTimer>
#include <GUI/Settingsmanager/settingsmanager.h>    // For delayed operations
#include <QMessageBox>

// %%% Constructor %%%
/* Initialize database editor */
DatabaseEditor::DatabaseEditor(QWidget *parent)
    : QMainWindow(parent)
{
    // Set window title
    setWindowTitle("Database Editor");
    // Set window size
    resize(1100, 600);
    SettingsManager& settings = SettingsManager::instance();
    restoreGeometry(settings.getWindowGeometry());
    restoreState(settings.getWindowState());

    // If no saved geometry, use default
    if (settings.getWindowGeometry().isEmpty()) {
        resize(1100, 600);
    }
    // Use enhanced dock widget setup for Linux compatibility
    setupEnhancedDockWidgets();

    // Setup UI components
    setupMenuBar();
    setupToolBars();
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

    // NEW: Connect recent project action
    // RECENT PROJECT CONNECTION - ADD THIS LINE
    connect(menuBar->getRecentProjectAction(), &QAction::triggered,
            this, &DatabaseEditor::onRecentProjectTriggered);

    qDebug() << "MenuBar actions connected successfully";
    qDebug() << "Recent Project Action:" << menuBar->getRecentProjectAction()->text();


    // Connect exit action
    connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
}

/* Enhanced dock widget setup for Linux compatibility */
void DatabaseEditor::setupEnhancedDockWidgets()
{
    // Full dock features for complete movability - REMOVED VerticalTitleBar
    QDockWidget::DockWidgetFeatures fullDockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;
    // REMOVED: QDockWidget::DockWidgetVerticalTitleBar

    // Setup hierarchy dock with enhanced features
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                   Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    hierarchyDock->setFeatures(fullDockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);

    // Set title bar to appear at top
    hierarchyDock->setTitleBarWidget(nullptr); // Use default title bar (top)
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // Setup inspector dock with enhanced features
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                   Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    inspectorDock->setFeatures(fullDockFeatures);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(200);

    // Set title bar to appear at top
    inspectorDock->setTitleBarWidget(nullptr); // Use default title bar (top)
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // Setup console dock with enhanced features
    consoleDock = new QDockWidget("Console", this);
    consoleDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                 Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    consoleDock->setFeatures(fullDockFeatures);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);

    // Set title bar to appear at top
    consoleDock->setTitleBarWidget(nullptr); // Use default title bar (top)
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    // Connect dock visibility signals
    connect(hierarchyDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);
    connect(inspectorDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);
    connect(consoleDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);

    // Set tabified docking to allow tabbed interface when docks are stacked
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    // Enable docking features
    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::GroupedDragging |
                   QMainWindow::AnimatedDocks);

    // Initial layout with proper splitting
    // Remove central widget to use only docks
    setCentralWidget(nullptr);

    // Create initial split layout
    splitDockWidget(hierarchyDock, inspectorDock, Qt::Horizontal);
    splitDockWidget(inspectorDock, consoleDock, Qt::Vertical);

    // Set initial sizes with proper proportions
    QTimer::singleShot(100, this, [=]() {
        int totalWidth = width();
        int hierarchyWidth = static_cast<int>(totalWidth * 0.25);  // 25% for hierarchy
        int inspectorWidth = static_cast<int>(totalWidth * 0.5);   // 50% for inspector
        int consoleHeight = static_cast<int>(height() * 0.3);      // 30% for console

        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);
    });
}

/* Setup dock widgets - legacy method, using enhanced version instead */
void DatabaseEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    // This method is replaced by setupEnhancedDockWidgets()
    setupEnhancedDockWidgets();
}

/* Handle dock visibility changes */
void DatabaseEditor::onDockVisibilityChanged(bool visible)
{
    QDockWidget* dock = qobject_cast<QDockWidget*>(sender());
    if (dock) {
        if (visible) {
            dock->raise(); // Bring to front when shown
        }
    }
}


/* Setup menu bar */
void DatabaseEditor::setupMenuBar()
{
    // Create and set menu bar
    MenuBar *menuBar = new MenuBar(this);
    setMenuBar(menuBar);

    // Connect feedback trigger
    connect(menuBar, &MenuBar::feedbackTriggered, this, &DatabaseEditor::showFeedbackWindow);

    // Get the Edit menu from MenuBar
    QMenu *editMenu = menuBar->getEditMenu();
    if (editMenu) {
        // Create reset layout action
        QAction *resetLayoutAction = new QAction("Reset Layout", this);
        resetLayoutAction->setShortcut(QKeySequence("Ctrl+R"));
        resetLayoutAction->setStatusTip("Reset all docks to initial positions");

        // Add separator and then reset layout action to Edit menu
        editMenu->addSeparator();
        editMenu->addAction(resetLayoutAction);

        // Connect the action
        connect(resetLayoutAction, &QAction::triggered, this, &DatabaseEditor::resetLayout);
    }

    // Connect other menu bar signals as before
    connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
}
void DatabaseEditor::resetLayout()
{
    // Show message in console
    console->log("Resetting layout to initial state...");

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
    inspectorDock->hide();
    consoleDock->hide();

    // Remove all docks from main window
    removeDockWidget(hierarchyDock);
    removeDockWidget(inspectorDock);
    removeDockWidget(consoleDock);

    // Add docks back to initial positions
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    // Recreate initial split configuration
    splitDockWidget(hierarchyDock, inspectorDock, Qt::Horizontal);
    splitDockWidget(inspectorDock, consoleDock, Qt::Vertical);

    // Show all docks
    hierarchyDock->show();
    inspectorDock->show();
    consoleDock->show();

    // Reset to initial sizes with a small delay
    QTimer::singleShot(100, this, [=]() {
        QMainWindow::resize(1100, 600); // Reset window size

        int totalWidth = this->width();
        int totalHeight = this->height();

        int hierarchyWidth = static_cast<int>(totalWidth * 0.25);   // 25% width
        int inspectorWidth = static_cast<int>(totalWidth * 0.50);   // 50% width
        int consoleHeight = static_cast<int>(totalHeight * 0.25);   // 25% height

        // Resize docks
        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);

        // Bring all docks to front
        hierarchyDock->raise();
        inspectorDock->raise();
        consoleDock->raise();

        updateStatusBar("Layout reset to initial state");
        console->log("Layout successfully reset to initial configuration");
    });
}

/* Setup toolbars */
void DatabaseEditor::setupToolBars()
{
    // Create and add standard toolbar
    StandardToolBar *standardToolBar = new StandardToolBar(this);
    addToolBar(Qt::TopToolBarArea, standardToolBar);

    // Allow toolbar to be movable
    standardToolBar->setMovable(true);
}

/* Add new inspector tab */
void DatabaseEditor::addInspectorTab()
{
    // Create new inspector dock with full features - REMOVED VerticalTitleBar
    QDockWidget *newInspectorDock = new QDockWidget("Inspector " + QString::number(++inspectorCount), this);
    newInspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                      Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);
    // REMOVED: QDockWidget::DockWidgetVerticalTitleBar

    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);
    newInspectorDock->setMinimumWidth(200);

    // Set title bar to appear at top
    newInspectorDock->setTitleBarWidget(nullptr); // Use default title bar (top)

    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);

    // Connect inspector signals
    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::valueChanged,
            this, &DatabaseEditor::markUnsavedChanges);
    connect(newInspector, &Inspector::addTabRequested,
            this, &DatabaseEditor::addInspectorTab);

    // Connect dock visibility
    connect(newInspectorDock, &QDockWidget::visibilityChanged,
            this, &DatabaseEditor::onDockVisibilityChanged);

    // Add or split dock - try to find the best placement
    if (inspectorDock->isVisible()) {
        // Try to split with existing inspector dock
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else if (hierarchyDock->isVisible()) {
        // Split with hierarchy if inspector is not visible
        splitDockWidget(hierarchyDock, newInspectorDock, Qt::Horizontal);
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
    statusBar->showMessage("Ready - Fully Dockable Interface Active");
}

/* Update status bar message */
void DatabaseEditor::updateStatusBar(const QString &message)
{
    // Update status bar
    if (statusBar) {
        statusBar->showMessage(message);
    }
}
/* Handle recent project menu click - IMPROVED VERSION */
void DatabaseEditor::onRecentProjectTriggered()
{
    qDebug() << "Recent Project menu clicked - showing recent projects list";

    // Get recent projects from HierarchyConnector
    QStringList recentProjects = HierarchyConnector::instance()->getRecentProjects();
    if (recentProjects.size() > 10) {
        recentProjects = recentProjects.mid(0, 10); // First 10 (most recent)
    }
    qDebug() << "Recent projects found:" << recentProjects.size();

    // Filter only existing files
    QStringList existingProjects;
    for (const QString& projectPath : recentProjects) {
        if (QFile::exists(projectPath)) {
            existingProjects << projectPath;
            qDebug() << "✅ Available:" << QFileInfo(projectPath).fileName();
        } else {
            qDebug() << "❌ Not found:" << projectPath;
        }
    }

    // Remove non-existing projects from the list
    if (existingProjects.size() < recentProjects.size()) {
        for (const QString& projectPath : existingProjects) {
            HierarchyConnector::instance()->addToRecentProjects(projectPath);
        }
    }

    if (existingProjects.isEmpty()) {
        QMessageBox::information(this, "Recent Projects",
                                 "📂 No recent projects found!\n\n"
                                 "To see projects here:\n"
                                 );
        return;
    }

    // Create recent projects menu
    QMenu recentMenu(this);
    recentMenu.setTitle("Recent Projects");

    // Add header
    QAction* headerAction = recentMenu.addAction("📋 Recently Opened Projects");
    headerAction->setEnabled(false);
    recentMenu.addSeparator();

    // Add recent projects to menu with numbers
    int count = 1;
    for (const QString& projectPath : existingProjects) {
        QFileInfo fileInfo(projectPath);
        QString displayText = QString("%1. 📄 %2\n    📍 %3")
                                  .arg(count)
                                  .arg(fileInfo.fileName())
                                  .arg(fileInfo.path());

        QAction* projectAction = recentMenu.addAction(displayText);
        projectAction->setData(projectPath);
        projectAction->setToolTip(projectPath);


    }

    recentMenu.addSeparator();


    recentMenu.addAction("🗑️ Clear All Recent Projects", this, &DatabaseEditor::clearRecentProjects);


    // Show menu at cursor position
    QPoint menuPos = QCursor::pos();
    QAction* selectedAction = recentMenu.exec(menuPos);

    if (selectedAction && selectedAction->data().isValid()) {
        QString filePath = selectedAction->data().toString();
        loadRecentProject(filePath);
    }
}

/* Load recent project manually */
void DatabaseEditor::loadRecentProject(const QString& filePath)
{
    if (!QFile::exists(filePath)) {
        QMessageBox::warning(this, "File Not Found",
                             QString("The project file was not found:\n\n%1\n\nIt will be removed from recent list.").arg(filePath));

        // Remove from recent list
        HierarchyConnector::instance()->addToRecentProjects(""); // This will cleanup
        return;
    }

    qDebug() << "Loading recent project manually:" << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error",
                             QString("Failed to open file:\n\n%1\n\nError: %2").arg(filePath).arg(file.errorString()));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error",
                             QString("Failed to parse JSON file:\n\n%1\n\nError: %2").arg(filePath).arg(err.errorString()));
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("hierarchy")) {
        QMessageBox::warning(this, "Error",
                             QString("Invalid project file (missing 'hierarchy'):\n\n%1").arg(filePath));
        return;
    }

    // Load the project
    QJsonObject hier = obj["hierarchy"].toObject();
    hierarchy->fromJson(hier);
    lastSavedFilePath = filePath;
    clearUnsavedChanges();

    // Update UI
    updateStatusBar("Loaded: " + QFileInfo(filePath).fileName());
    console->log("Recent project loaded: " + filePath.toStdString());


}

// Clear recent projects list
void DatabaseEditor::clearRecentProjects()
{

    HierarchyConnector::instance()->clearRecentProjects();

    // Optional: Just update status bar
    updateStatusBar("Recent projects list cleared");
    console->log("Recent projects list cleared");
}
