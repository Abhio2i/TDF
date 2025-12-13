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
#include <QMessageBox>
#include <GUI/Menubars/profileinfodialog.h>
#include <GUI/Editors/recentprojectsmanager.h>
#include <GUI/Settings/applicationdialog.h>
#include <QProgressDialog>

// Capitalize the first letter of a string
static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}

/* Initialize database editor */
DatabaseEditor::DatabaseEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Database Editor");
    resize(1100, 600);

   setupEnhancedDockWidgets();
    setupMenuBar();
    setupToolBars();
    setupStatusBar();

    scenario = new Scenario();
    scenario->hierarchy->isDatabase = true;
    hierarchy = scenario->hierarchy;
    console = scenario->console;
    lastSavedFilePath = "";

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

    // connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
    //         this, &DatabaseEditor::loadRecentProject);
    connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
            this, [=](const QString& filePath, RecentProjectsManager::EditorType type) {
                if (type == RecentProjectsManager::DatabaseEditor) {
                    loadRecentProject(filePath);
                }
            });

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

    if (hierarchy && treeView) {
        HierarchyConnector::instance()->connectSignals(hierarchy, treeView);
        HierarchyConnector::instance()->initializeDummyData(hierarchy);
        HierarchyConnector::instance()->setupFileOperations(this, hierarchy, nullptr);
    }

    if (treeView && hierarchy) {
        connect(treeView, &HierarchyTree::itemSelected, this, [=](QVariantMap data) {
            QString type;
            if (data["type"].type() == QVariant::Map) {
                QVariantMap typeData = data["type"].toMap();
                if (typeData.contains("type") && typeData["type"].toString() == "option") {
                    type = "profile";
                } else {
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
                inspectorDock->show();
            }
        });
    }

    if (inspector && hierarchy) {
        connect(inspector, &Inspector::valueChanged,
                hierarchy, &Hierarchy::UpdateComponent);
    }

    connect(inspector, &Inspector::addTabRequested, this, &DatabaseEditor::addInspectorTab);
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
    inspector->setHierarchy(hierarchy);

    MenuBar* menuBar = qobject_cast<MenuBar*>(this->menuBar());
    if (menuBar) {
        connect(menuBar->getSaveAction(), &QAction::triggered, this, &DatabaseEditor::clearUnsavedChanges);
        connect(menuBar->getSameSaveAction(), &QAction::triggered, this, &DatabaseEditor::clearUnsavedChanges);
        connect(menuBar->getRecentProjectAction(), &QAction::triggered, this, &DatabaseEditor::onRecentProjectTriggered);
        connect(menuBar, &MenuBar::profileTriggered, this, &DatabaseEditor::showProfileInfo);
        connect(menuBar, &MenuBar::applicationTriggered, this, &DatabaseEditor::showApplicationDialog);

        // connect(menuBar, &MenuBar::performanceTriggered, this, &DatabaseEditor::onPerformanceClicked);
        // connect(menuBar, &MenuBar::sensorsTriggered, this, &DatabaseEditor::onSensorsClicked);
        connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
    }
}

/* Enhanced dock widget setup for Linux compatibility */
void DatabaseEditor::setupEnhancedDockWidgets()
{
    QDockWidget::DockWidgetFeatures fullDockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;

    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                   Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    hierarchyDock->setFeatures(fullDockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);
    hierarchyDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                   Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    inspectorDock->setFeatures(fullDockFeatures);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(200);
    inspectorDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    consoleDock = new QDockWidget("Console", this);
    consoleDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                 Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    consoleDock->setFeatures(fullDockFeatures);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    consoleDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    connect(hierarchyDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);
    connect(inspectorDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);
    connect(consoleDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);

    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::GroupedDragging |
                   QMainWindow::AnimatedDocks);

    setCentralWidget(nullptr);

    splitDockWidget(hierarchyDock, inspectorDock, Qt::Horizontal);
    splitDockWidget(inspectorDock, consoleDock, Qt::Vertical);

    QTimer::singleShot(100, this, [=]() {
        int totalWidth = width();
        int hierarchyWidth = static_cast<int>(totalWidth * 0.25);
        int inspectorWidth = static_cast<int>(totalWidth * 0.5);
        int consoleHeight = static_cast<int>(height() * 0.3);

        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);
    });
}

/* Setup dock widgets - legacy method, using enhanced version instead */
void DatabaseEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    setupEnhancedDockWidgets();
}

/* Handle dock visibility changes */
void DatabaseEditor::onDockVisibilityChanged(bool visible)
{
    QDockWidget* dock = qobject_cast<QDockWidget*>(sender());
    if (dock && visible) {
        dock->raise();
    }
}

/* Setup menu bar */
void DatabaseEditor::setupMenuBar()
{
    MenuBar *menuBar = new MenuBar(this);
    setMenuBar(menuBar);

    connect(menuBar, &MenuBar::feedbackTriggered, this, &DatabaseEditor::showFeedbackWindow);

    QMenu *editMenu = menuBar->getEditMenu();
    if (editMenu) {
        QAction *resetLayoutAction = new QAction("Reset Layout", this);
        resetLayoutAction->setStatusTip("Reset all docks to initial positions");
        editMenu->addSeparator();
        editMenu->addAction(resetLayoutAction);
        connect(resetLayoutAction, &QAction::triggered, this, &DatabaseEditor::resetLayout);
    }

    connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
}

void DatabaseEditor::resetLayout()
{
    console->log("Resetting layout to initial state...");

    for (int i = inspectorDocks.size() - 1; i >= 0; --i) {
        QDockWidget* dock = inspectorDocks[i];
        if (dock != inspectorDock) {
            inspectors.removeAt(i);
            dock->deleteLater();
        }
    }

    if (inspectorDocks.size() > 1) {
        inspectorDocks = QList<QDockWidget*>{inspectorDock};
        inspectors = QList<Inspector*>{inspector};
    }
    inspectorCount = 0;

    hierarchyDock->hide();
    inspectorDock->hide();
    consoleDock->hide();

    removeDockWidget(hierarchyDock);
    removeDockWidget(inspectorDock);
    removeDockWidget(consoleDock);

    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    splitDockWidget(hierarchyDock, inspectorDock, Qt::Horizontal);
    splitDockWidget(inspectorDock, consoleDock, Qt::Vertical);

    hierarchyDock->show();
    inspectorDock->show();
    consoleDock->show();

    QTimer::singleShot(100, this, [=]() {
        QMainWindow::resize(1100, 600);

        int totalWidth = this->width();
        int totalHeight = this->height();

        int hierarchyWidth = static_cast<int>(totalWidth * 0.25);
        int inspectorWidth = static_cast<int>(totalWidth * 0.50);
        int consoleHeight = static_cast<int>(totalHeight * 0.25);

        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);

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
}

/* Add new inspector tab */
void DatabaseEditor::addInspectorTab()
{
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

    connect(newInspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::valueChanged,
            this, &DatabaseEditor::markUnsavedChanges);
    connect(newInspector, &Inspector::addTabRequested,
            this, &DatabaseEditor::addInspectorTab);

    connect(newInspectorDock, &QDockWidget::visibilityChanged,
            this, &DatabaseEditor::onDockVisibilityChanged);

    if (inspectorDock->isVisible()) {
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else if (hierarchyDock->isVisible()) {
        splitDockWidget(hierarchyDock, newInspectorDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }

    connect(newInspectorDock, &QDockWidget::destroyed, this, [=]() {
        inspectorDocks.removeOne(newInspectorDock);
        inspectors.removeOne(newInspector);
    });

    newInspectorDock->show();
    newInspectorDock->raise();
}

/* Show feedback window */
void DatabaseEditor::showFeedbackWindow()
{
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->h = hierarchy;
    feedbackWindow->loadDashboardData("{}");
    feedbackWindow->show();
}

/* Destructor */
DatabaseEditor::~DatabaseEditor()
{
    if (scenario) {
        delete scenario;
    }
}

/* Mark unsaved changes */
void DatabaseEditor::markUnsavedChanges()
{
    if (!hasUnsavedChanges) {
        hasUnsavedChanges = true;
        emit unsavedChangesChanged(true);
        setWindowTitle("Database Editor *");
    }
}

/* Clear unsaved changes */
void DatabaseEditor::clearUnsavedChanges()
{
    if (hasUnsavedChanges) {
        hasUnsavedChanges = false;
        emit unsavedChangesChanged(false);
        setWindowTitle("Database Editor");
    }
}

/* Setup status bar */
void DatabaseEditor::setupStatusBar()
{
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");
}

/* Update status bar message */
void DatabaseEditor::updateStatusBar(const QString &message)
{
    if (statusBar) {
        statusBar->showMessage(message);
    }
}

void DatabaseEditor::loadRecentProject(const QString& filePath)
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

    // // Load tactical display if present
    // if (tacticalDisplay && obj.contains("tactical")) {
    //     QJsonObject tac = obj["tactical"].toObject();
    //     tacticalDisplay->canvas->fromJson(tac);
    // }

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

void DatabaseEditor::onRecentProjectTriggered()
{
    RecentProjectsManager::instance()->showRecentProjectsMenu(this,
                                                              RecentProjectsManager::DatabaseEditor);
}
// void DatabaseEditor::clearRecentProjects()
// {
//     RecentProjectsManager::instance()->clearRecentProjects();
//     updateStatusBar("Recent projects list cleared");
//     console->log("Recent projects list cleared");
// }

void DatabaseEditor::showProfileInfo()
{
    ProfileInfoDialog::showProfileInfo(this);
}

// void DatabaseEditor::onRecentProjectTriggered()
// {
//     RecentProjectsManager::instance()->showRecentProjectsMenu(this);
// }
void DatabaseEditor::showApplicationDialog()
{
    ApplicationDialog dialog(this);
    dialog.exec();


}
void DatabaseEditor::loadFromJsonFile(const QString &filePath)
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



    lastSavedFilePath = filePath;
    clearUnsavedChanges();

    // Close loading dialog
    loadingDialog->close();
    loadingDialog->deleteLater();

    updateStatusBar("Project loaded: " + QFileInfo(filePath).fileName());
}
