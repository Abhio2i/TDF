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

    if (treeView && hierarchy)

    if (treeView && hierarchy) {
        connect(treeView, &HierarchyTree::itemSelected, this, &DatabaseEditor::onTreeItemSelected);
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
    consoleDock->hide();

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
        int hierarchyWidth = static_cast<int>(totalWidth * 0.10);
        int inspectorWidth = static_cast<int>(totalWidth * 0.85);
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

        int hierarchyWidth = static_cast<int>(totalWidth * 0.10);
        int inspectorWidth = static_cast<int>(totalWidth * 0.85);
        int consoleHeight = static_cast<int>(totalHeight * 0.25);

        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);

        hierarchyDock->raise();
        inspectorDock->raise();
        consoleDock->raise();

        updateStatusBar("Layout reset to initial state");

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
    loadingDialog->setCancelButton(nullptr);
    loadingDialog->setMinimumDuration(0);
    loadingDialog->setRange(0, 0);
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

    QProgressDialog* loadingDialog = new QProgressDialog("Loading data...", nullptr, 0, 0, this);
    loadingDialog->setWindowTitle("");
    loadingDialog->setWindowModality(Qt::WindowModal);
    loadingDialog->setCancelButton(nullptr);
    loadingDialog->setMinimumDuration(0);
    loadingDialog->setRange(0, 0);
    loadingDialog->setValue(0);
    loadingDialog->show();
    QCoreApplication::processEvents();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
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


        if (treeView && treeView->getTreeWidget()) {
            treeView->getTreeWidget()->update();

        } else {

        }

        QCoreApplication::processEvents();
    } else {

    }
    lastSavedFilePath = filePath;
    clearUnsavedChanges();
    loadingDialog->close();
    loadingDialog->deleteLater();
    updateStatusBar("Project loaded: " + QFileInfo(filePath).fileName());
}

void DatabaseEditor::onTreeItemSelected(QVariantMap data)
{
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


    cleanupExtraInspectors();

    // ✅ Reset inspector state completely
    if (inspector) {
        inspector->resetState();
    }

    if (type == "entity") {
        QString entityId = data["ID"].toString();
        showAllEntityComponents(entityId, displayName);
        auto entityIt = hierarchy->Entities->find(entityId.toStdString());
        if (entityIt != hierarchy->Entities->end()) {
            QJsonObject entityData = entityIt->second->toJson();
            inspector->init(entityId, displayName + "_self", entityData);
        }
    }
    else if (type == "subcomponent") {
        QString subCompId = data["ID"].toString();
        QString parentCompId = data["parentId"].toString();
        QJsonObject subComponentData;
        auto compIt = hierarchy->Components->find(parentCompId.toStdString());
        if (compIt != hierarchy->Components->end()) {
            auto compPtr = compIt->second;
            if (compPtr) {
                subComponentData = compPtr->getsubComponentData(subCompId.toStdString());
            }
        }

        if (subComponentData.isEmpty()) {
            QString componentType = name.toLower();
            if (componentType.contains("sensor", Qt::CaseInsensitive)) {
                componentType = "sensors";
            } else if (componentType.contains("radio", Qt::CaseInsensitive)) {
                componentType = "radios";
            } else if (componentType.contains("iff", Qt::CaseInsensitive)) {
                componentType = "iffs";
            }

            QJsonObject parentData = hierarchy->getComponentData(parentCompId, componentType);
            if (parentData.contains(componentType) && parentData[componentType].isObject()) {
                QJsonObject container = parentData[componentType].toObject();
                if (container.contains(subCompId)) {
                    subComponentData = container[subCompId].toObject();
                }
            }
        }

        if (!subComponentData.isEmpty()) {
            inspector->init(ID, displayName + "_sub", subComponentData);
        }
    }
    else if (type == "component") {
        QJsonObject componentData = hierarchy->getComponentData(ID, name);
        if (!componentData.isEmpty()) {
            inspector->init(ID, displayName, componentData);
        }
    }
    else if (type == "profile") {
        auto profileIt = hierarchy->ProfileCategories.find(data["ID"].toString().toStdString());
        if (profileIt != hierarchy->ProfileCategories.end()) {
            inspector->init(ID, displayName + "_self", profileIt->second->toJson());
        }
    }
    else if (type == "folder") {
        auto folderIt = hierarchy->Folders->find(data["ID"].toString().toStdString());
        if (folderIt != hierarchy->Folders->end()) {
            inspector->init(ID, displayName + "_self", folderIt->second->toJson());
        }
    }
    else {
        inspector->init(ID, displayName, QJsonObject());
    }

    if (!inspectorDock->isVisible()) {
        addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
        inspectorDock->show();
    }
}

void DatabaseEditor::cleanupExtraInspectors()
{
    // ✅ Check if we're in "All Components Mode"
    bool isAllComponentsMode = inspectorDock->property("IsAllComponentsMode").toBool();

    if (isAllComponentsMode) {
        // Restore original inspector widget
        inspectorDock->setWidget(inspector);
        inspectorDock->setWindowTitle("Inspector");

        // Clear all properties
        inspectorDock->setProperty("AllComponentsWidget", QVariant());
        inspectorDock->setProperty("IsAllComponentsMode", false);
        inspectorDock->setProperty("EntityId", QVariant());

        // Delete all temporary inspectors
        QVariant inspectorsVar = inspectorDock->property("AllInspectors");
        if (inspectorsVar.isValid()) {
            QList<Inspector*> allInspectors = inspectorsVar.value<QList<Inspector*>>();
            for (Inspector* insp : allInspectors) {
                if (insp != inspector) {
                    insp->deleteLater();
                }
            }
            inspectorDock->setProperty("AllInspectors", QVariant());
        }

        // ✅ Reset the main inspector state
        if (inspector) {
            inspector->resetState();
        }
    }

    // ✅ Remove any extra inspector docks (from Add Tab feature)
    for (int i = inspectorDocks.size() - 1; i > 0; --i) {
        QDockWidget* dock = inspectorDocks[i];
        if (dock != inspectorDock) {
            inspectorDocks.removeAt(i);
            dock->deleteLater();
        }
    }

    // Reset lists to only contain main inspector
    inspectors = QList<Inspector*>{inspector};
    inspectorCount = 0;

    // ✅ Ensure inspector dock is visible and properly sized
    if (!inspectorDock->isVisible()) {
        inspectorDock->show();
    }
}
void DatabaseEditor::showAllEntityComponents(const QString& entityId, const QString& entityName)
{

    QString entityType = "";
    auto entityIt = hierarchy->Entities->find(entityId.toStdString());
    if (entityIt != hierarchy->Entities->end()) {
        QJsonObject entityData = entityIt->second->toJson();
        if (entityData.contains("type") && entityData["type"].isObject()) {
            QJsonObject typeObj = entityData["type"].toObject();
            if (typeObj.contains("value")) {
                entityType = typeObj["value"].toString();
            }
        }
    }


    if (entityType != "Platform") {

        if (entityIt != hierarchy->Entities->end()) {
            QJsonObject entityData = entityIt->second->toJson();
            QJsonObject displayData;
            displayData["name"] = entityData["name"];
            displayData["active"] = entityData["active"];
            if (entityData.contains("default") && entityData["default"].isObject()) {
                QJsonObject defaultData = entityData["default"].toObject();
                for (auto it = defaultData.begin(); it != defaultData.end(); ++it) {
                    displayData[it.key()] = it.value();
                }
            }

            inspector->init(entityId, entityName + "_self", displayData);
        }
        return;
    }
    QWidget *container = new QWidget();
    QGridLayout *gridLayout = new QGridLayout(container);
    gridLayout->setContentsMargins(10, 10, 10, 10);
    gridLayout->setHorizontalSpacing(15);
    gridLayout->setVerticalSpacing(15);


    QMap<QString, int> componentMinHeights = {
        {"entity", 400},
        {"transform", 200},
        {"trajectory", 200},
        {"dynamicModel", 400},
        {"collider", 250},
        {"rigidbody", 800},
        {"bitmap", 400},
        {"crossSection", 800},
        {"sensors", 800},
        {"radios", 400},
        {"iffs", 400},
        {"parameters", 150}
    };

    // Components to hide
    QStringList hiddenComponents = {
        "collider",
        "transform",
        "trajectory",
        "rigidbody",
        "entity",
        "crossSection"
    };
    QStringList dynamicModelBitmapColumn = {"dynamicModel", "bitmap"};
    QStringList iffRadioColumnComponents = {"iffs", "radios"};
    QStringList group1Components = {"transform", "sensors"};
    QStringList group2Components = {"trajectory", "collider"};
    QStringList exclusiveSingleColumnComponents = {
        "crossSection",
        "rigidbody",
        "sensors"
    };
    QStringList componentOrder = {
        "entity", "transform", "trajectory", "dynamicModel",
        "collider", "rigidbody", "bitmap", "crossSection",
        "sensors", "radios", "iffs", "parameters"
    };


    QList<QPair<QString, int>> availableComponents;
    for (const QString& compType : componentOrder) {
        if (hiddenComponents.contains(compType)) {
            continue;
        }

        QJsonObject componentData;
        if (compType == "entity") {
            auto entityIt = hierarchy->Entities->find(entityId.toStdString());
            if (entityIt != hierarchy->Entities->end()) {
                componentData = entityIt->second->toJson();
                availableComponents.append(qMakePair(compType, componentMinHeights[compType]));
            }
        } else {
            componentData = hierarchy->getComponentData(entityId, compType);
            if (!componentData.isEmpty()) {
                availableComponents.append(qMakePair(compType, componentMinHeights[compType]));
            }
        }
    }
    QList<QPair<QString, int>> exclusiveComponents;
    QList<QPair<QString, int>> dynamicModelBitmapList;
    QList<QPair<QString, int>> iffRadioColumnList;
    QList<QPair<QString, int>> group1ComponentsList;
    QList<QPair<QString, int>> group2ComponentsList;
    QList<QPair<QString, int>> otherComponents;

    for (const auto& comp : availableComponents) {
        QString compType = comp.first;

        if (exclusiveSingleColumnComponents.contains(compType)) {
            exclusiveComponents.append(comp);
        }
        else if (dynamicModelBitmapColumn.contains(compType) && !hiddenComponents.contains(compType)) {
            dynamicModelBitmapList.append(comp);
        }
        else if (iffRadioColumnComponents.contains(compType) && !hiddenComponents.contains(compType)) {
            iffRadioColumnList.append(comp);
        }
        else if (group1Components.contains(compType) && !hiddenComponents.contains(compType)) {
            group1ComponentsList.append(comp);
        } else if (group2Components.contains(compType) && !hiddenComponents.contains(compType)) {
            group2ComponentsList.append(comp);
        } else {
            otherComponents.append(comp);
        }
    }


    int totalColumns = 0;


    int exclusiveColumns = exclusiveComponents.size();
    totalColumns += exclusiveColumns;


    int dynamicModelBitmapColumnCount = !dynamicModelBitmapList.isEmpty() ? 1 : 0;
    totalColumns += dynamicModelBitmapColumnCount;


    int iffRadioColumnCount = !iffRadioColumnList.isEmpty() ? 1 : 0;
    totalColumns += iffRadioColumnCount;


    int groupColumns = 0;
    if (!group1ComponentsList.isEmpty()) {
        groupColumns++;
    }
    if (!group2ComponentsList.isEmpty()) {
        groupColumns++;
    }
    totalColumns += groupColumns;


    int otherComponentCount = otherComponents.size();
    int otherColumns = (otherComponentCount + 1) / 2;
    totalColumns += otherColumns;


    QVector<QWidget*> columnWidgets(totalColumns);
    QVector<QVBoxLayout*> columnLayouts(totalColumns);

    QMap<QString, int> componentColumnMap;
    int currentColumn = 0;


    for (const auto& comp : exclusiveComponents) {
        QString compType = comp.first;

        columnWidgets[currentColumn] = new QWidget();
        columnLayouts[currentColumn] = new QVBoxLayout(columnWidgets[currentColumn]);
        columnLayouts[currentColumn]->setContentsMargins(0, 0, 0, 0);
        columnLayouts[currentColumn]->setSpacing(0);
        columnLayouts[currentColumn]->addStretch(1);

        componentColumnMap[compType] = currentColumn;
        currentColumn++;
    }


    if (!dynamicModelBitmapList.isEmpty()) {
        columnWidgets[currentColumn] = new QWidget();
        columnLayouts[currentColumn] = new QVBoxLayout(columnWidgets[currentColumn]);
        columnLayouts[currentColumn]->setContentsMargins(0, 0, 0, 0);
        columnLayouts[currentColumn]->setSpacing(8);
        columnLayouts[currentColumn]->addStretch(1);

        for (const auto& comp : dynamicModelBitmapList) {
            componentColumnMap[comp.first] = currentColumn;
        }
        currentColumn++;
    }


    if (!iffRadioColumnList.isEmpty()) {
        columnWidgets[currentColumn] = new QWidget();
        columnLayouts[currentColumn] = new QVBoxLayout(columnWidgets[currentColumn]);
        columnLayouts[currentColumn]->setContentsMargins(0, 0, 0, 0);
        columnLayouts[currentColumn]->setSpacing(8);
        columnLayouts[currentColumn]->addStretch(1);

        for (const auto& comp : iffRadioColumnList) {
            componentColumnMap[comp.first] = currentColumn;
        }
        currentColumn++;
    }


    if (!group1ComponentsList.isEmpty()) {
        columnWidgets[currentColumn] = new QWidget();
        columnLayouts[currentColumn] = new QVBoxLayout(columnWidgets[currentColumn]);
        columnLayouts[currentColumn]->setContentsMargins(0, 0, 0, 0);
        columnLayouts[currentColumn]->setSpacing(8);
        columnLayouts[currentColumn]->addStretch(1);

        for (const auto& comp : group1ComponentsList) {
            componentColumnMap[comp.first] = currentColumn;
        }
        currentColumn++;
    }


    if (!group2ComponentsList.isEmpty()) {
        columnWidgets[currentColumn] = new QWidget();
        columnLayouts[currentColumn] = new QVBoxLayout(columnWidgets[currentColumn]);
        columnLayouts[currentColumn]->setContentsMargins(0, 0, 0, 0);
        columnLayouts[currentColumn]->setSpacing(8);
        columnLayouts[currentColumn]->addStretch(1);

        for (const auto& comp : group2ComponentsList) {
            componentColumnMap[comp.first] = currentColumn;
        }
        currentColumn++;
    }


    QVector<QList<QPair<QString, int>>> otherColumnComponents(otherColumns);

    for (int i = 0; i < otherComponentCount; i++) {
        int columnIndex = i % otherColumns;
        otherColumnComponents[columnIndex].append(otherComponents[i]);
    }

    for (int col = 0; col < otherColumns; col++) {
        int columnIndex = currentColumn + col;

        columnWidgets[columnIndex] = new QWidget();
        columnLayouts[columnIndex] = new QVBoxLayout(columnWidgets[columnIndex]);
        columnLayouts[columnIndex]->setContentsMargins(0, 0, 0, 0);
        columnLayouts[columnIndex]->setSpacing(10);
        columnLayouts[columnIndex]->addStretch(1);

        for (const auto& comp : otherColumnComponents[col]) {
            componentColumnMap[comp.first] = columnIndex;
        }
    }


    for (const auto& comp : availableComponents) {
        QString compType = comp.first;
        if (hiddenComponents.contains(compType)) {
            continue;
        }

        int minHeight = componentMinHeights[compType];
        int columnIndex = componentColumnMap[compType];

        bool needsDynamicHeight = false;
        int actualHeight = minHeight;


        if (compType == "sensors" || compType == "radios" || compType == "iffs") {
            needsDynamicHeight = true;
            QJsonObject componentData = hierarchy->getComponentData(entityId, compType);

            if (componentData.contains("items") && componentData["items"].isArray()) {
                QJsonArray items = componentData["items"].toArray();
                int itemCount = items.size();
                int itemHeight = 35;
                actualHeight = minHeight + (itemCount * itemHeight);
                actualHeight = qMin(actualHeight, 500);
                actualHeight = qMax(actualHeight, minHeight);
            }
        }

        // Get component data
        QJsonObject componentData;
        QString displayName = capitalizeFirstLetter(compType);

        if (compType == "entity") {
            auto entityIt = hierarchy->Entities->find(entityId.toStdString());
            if (entityIt != hierarchy->Entities->end()) {
                componentData = entityIt->second->toJson();

                Inspector *entityInspector = new Inspector();
                entityInspector->setHierarchy(hierarchy);
                entityInspector->init(entityId, "entity_self", componentData);

                connect(entityInspector, &Inspector::valueChanged,
                        hierarchy, &Hierarchy::UpdateComponent);
                connect(entityInspector, &Inspector::valueChanged,
                        this, &DatabaseEditor::markUnsavedChanges);

                entityInspector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                entityInspector->setMinimumWidth(350);
                entityInspector->setMinimumHeight(minHeight);
                entityInspector->setMaximumHeight(minHeight * 1.1);


                QFrame *frame = new QFrame();
                frame->setFrameStyle(QFrame::Box | QFrame::Raised);
                frame->setLineWidth(1);
                frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

                QVBoxLayout *frameLayout = new QVBoxLayout(frame);
                frameLayout->setContentsMargins(0, 0, 0, 0);
                frameLayout->setSpacing(1);
                frameLayout->addWidget(entityInspector);

                columnLayouts[columnIndex]->insertWidget(columnLayouts[columnIndex]->count() - 1, frame);
                continue;
            }
        } else {
            componentData = hierarchy->getComponentData(entityId, compType);
        }


        QWidget *compWidget = nullptr;
        if (needsDynamicHeight) {
            compWidget = createComponentInspectorWithDynamicHeight(
                entityId, displayName, componentData, actualHeight);
        } else {
            compWidget = createComponentInspector(
                entityId, displayName, componentData, actualHeight);
        }


        if (exclusiveSingleColumnComponents.contains(compType)) {
            compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        }
        else if (dynamicModelBitmapColumn.contains(compType)) {

            compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            compWidget->setMinimumHeight(minHeight);
            compWidget->setMaximumHeight(minHeight * 1.2);
        }
        else if (iffRadioColumnComponents.contains(compType)) {
            if (needsDynamicHeight) {
                compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                compWidget->setMinimumHeight(actualHeight);
            } else {
                compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            }
        }
        else if (group1Components.contains(compType) || group2Components.contains(compType)) {
            if (needsDynamicHeight) {
                compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                compWidget->setMinimumHeight(actualHeight);
            } else {
                compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            }
        } else {
            compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        }

        columnLayouts[columnIndex]->insertWidget(columnLayouts[columnIndex]->count() - 1, compWidget);
    }


    for (int col = 0; col < totalColumns; col++) {
        if (columnWidgets[col]) {
            gridLayout->addWidget(columnWidgets[col], 0, col, 1, 1);
            gridLayout->setColumnStretch(col, 1);
        }
    }


    gridLayout->setRowStretch(0, 0);
    gridLayout->setRowStretch(1, 1);


    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(container);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QString style = R"(
        QScrollArea {
            background-color: #f5f5f5;
            border: none;
        }
        QScrollArea > QWidget > QWidget {
            background-color: #f5f5f5;
        }
    )";
    scrollArea->setStyleSheet(style);

    QList<Inspector*> allInspectors;

    QList<Inspector*> inspectorsInLayout = container->findChildren<Inspector*>();
    for (Inspector* insp : inspectorsInLayout) {
        allInspectors.append(insp);
    }

    inspectorDock->setWidget(scrollArea);
    inspectorDock->setWindowTitle(entityName + " - All Components");

    inspectorDock->setProperty("IsAllComponentsMode", true);
    inspectorDock->setProperty("EntityId", entityId);
    inspectorDock->setProperty("AllComponentsWidget", QVariant::fromValue(scrollArea));
    inspectorDock->setProperty("AllInspectors", QVariant::fromValue(allInspectors));
}
QWidget* DatabaseEditor::createComponentInspector(
    const QString& entityId,
    const QString& title,
    const QJsonObject& data,
    int preferredHeight)
{
    Inspector *inspector = new Inspector();
    inspector->setHierarchy(hierarchy);
    QString componentName = title.toLower();
    inspector->init(entityId, componentName, data);
    inspector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    inspector->setMinimumWidth(350);
    inspector->setMinimumHeight(preferredHeight);
    inspector->setMaximumHeight(preferredHeight * 1.1);
    connect(inspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::valueChanged,
            this, &DatabaseEditor::markUnsavedChanges);
    QFrame *frame = new QFrame();
    frame->setFrameStyle(QFrame::Box | QFrame::Raised);
    frame->setLineWidth(1);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(1);
    frameLayout->addWidget(inspector);
    return frame;
}
QWidget* DatabaseEditor::createComponentInspectorWithDynamicHeight(
    const QString& entityId,
    const QString& title,
    const QJsonObject& data,
    int initialHeight)
{
    Inspector *inspector = new Inspector();
    inspector->setHierarchy(hierarchy);
    QString componentName = title.toLower();
    if (componentName == "entity") {

        componentName = "entity_self";
    }
    inspector->init(entityId, componentName, data);
    inspector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    inspector->setMinimumWidth(350);
    inspector->setMinimumHeight(initialHeight);
    inspector->setMaximumHeight(16777215);
    connect(inspector, &Inspector::valueChanged,
            hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::valueChanged,
            this, &DatabaseEditor::markUnsavedChanges);
    QFrame *frame = new QFrame();
    frame->setFrameStyle(QFrame::Box | QFrame::Raised);
    frame->setLineWidth(1);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(1);
    frameLayout->addWidget(inspector);
    return frame;
}


