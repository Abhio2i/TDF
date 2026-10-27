/* =============================================================================
 * FILE:         databaseeditor.cpp
 * MODULE:       Database Editor Main Window
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the DatabaseEditor class which serves as the main
 *               window for the database editor application. It manages the
 *               hierarchy tree view, inspector panels, console view, menu bar,
 *               tool bars, dock widgets, status bar, and scenario data.
 *               Supports loading/saving JSON files, tracking unsaved changes,
 *               managing multiple inspector tabs, resetting layout, and
 *               displaying profile/application information.
 *
 * REQUIREMENTS: Implements REQ-EDITOR-010 through REQ-EDITOR-023
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-EDITOR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "databaseeditor.h"                        // For database editor class
#include "GUI/Feedback/projectinformation.h"    // For feedback window
#include "GUI/Console/consoleview.h"              // For console view
#include "GUI/Menubars/menubar.h"                 // For menu bar
#include "GUI/Toolbars/standardtoolbar.h"         // For standard toolbar
#include "qstandardpaths.h"
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
#include <QScrollArea>
#include "database-styles.h"
#include "GUI/Inspector/inspector-styles.h"
#include "GUI/Hierarchytree/formationcanvaswidget.h"


// %%% Utility Functions %%%
/* Capitalize the first letter of a string */
static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}
static QJsonObject filterEntityJsonForInspector(const QJsonObject &entityJson)
{
    static const QStringList hiddenKeys = {
        "Iffs", "iffs",
        "Sensors", "sensors",
        "Radios", "radios",
        "Weapons", "weapons",
        "ReferenceIDE", "referenceIDE", "ReferenceIdE", "referenceIdE"
    };
    QJsonObject filtered = entityJson;
    for (const QString &key : hiddenKeys) {
        filtered.remove(key);
    }
    return filtered;
}
// %%% Constructor %%%
/* Initialize database editor with scenario and UI components */
DatabaseEditor::DatabaseEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Database Editor");
    resize(1100, 600);
   setStyleSheet(DatabaseStyles::DatabaseEditorWidget);
    // %%% UI Setup %%%
    setupEnhancedDockWidgets();
    // setupMenuBar();
    setupToolBars();
    setupStatusBar();

    // %%% Core Data Setup %%%
    scenario = new Scenario();
    scenario->hierarchy->isDatabase = true;
    hierarchy = scenario->hierarchy;
    console = scenario->console;
    lastSavedFilePath = "";

    // %%% Console Setup %%%
    consoleView->setConsoleDock(consoleDock);
    // Connect console signals to view
    connect(console, &Console::logUpdate, this, [=](std::string log) {
        if (consoleView) {
            consoleView->appendText(QString::fromStdString(log));
        }
    });
    connect(console, &Console::errorUpdate, this, [=](std::string error) {
        if (consoleView) {
            consoleView->appendText(QString::fromStdString(error));
        }
    });
    connect(console, &Console::warningUpdate, this, [=](std::string warning) {
        if (consoleView) {
            consoleView->appendText(QString::fromStdString(warning));
        }
    });
    connect(console, &Console::debugUpdate, this, [=](std::string debug) {
        if (consoleView) {
            consoleView->appendText(QString::fromStdString(debug));
        }
    });

    // %%% Recent Projects Setup %%%
    connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
            this, [=](const QString& filePath, RecentProjectsManager::EditorType type) {
                if (type == RecentProjectsManager::DatabaseEditor) {
                    loadRecentProject(filePath);
                }
            });
    // %%% Hierarchy Change Tracking %%%
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
    connect(inspector, &Inspector::valueChanged, this,
            [this](QString ID, QString, QJsonObject) {
                markEntityChanged(ID);
            });
    // %%% Hierarchy Connector Setup %%%
    if (hierarchy && treeView) {
        HierarchyConnector::instance()->connectSignals(hierarchy,nullptr, treeView);
        HierarchyConnector::instance()->initializeDummyData(hierarchy);
        // HierarchyConnector::instance()->setupFileOperations(this, hierarchy, nullptr);
    }
    // %%% Tree View Signals %%%
    if (treeView && hierarchy) {
        connect(treeView, &HierarchyTree::itemSelected, this, &DatabaseEditor::onTreeItemSelected);
    }
    // %%% Inspector Signals %%%
    if (inspector && hierarchy) {
        connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    }
    connect(inspector, &Inspector::addTabRequested, this, &DatabaseEditor::addInspectorTab);
    // %%% Inspector Management %%%
    inspectorDocks.append(inspectorDock);
    inspectors.append(inspector);
    inspector->setHierarchy(hierarchy);
    inspector->setDatabaseEditorMode(true);
    // %%% Menu Bar Setup %%%
    MenuBar* menuBar = qobject_cast<MenuBar*>(this->menuBar());
    if (menuBar) {
        connect(menuBar->getSaveAction(), &QAction::triggered, this, &DatabaseEditor::clearUnsavedChanges);
        connect(menuBar->getSameSaveAction(), &QAction::triggered, this, &DatabaseEditor::clearUnsavedChanges);
        connect(menuBar->getRecentProjectAction(), &QAction::triggered, this, &DatabaseEditor::onRecentProjectTriggered);
        connect(menuBar, &MenuBar::profileTriggered, this, &DatabaseEditor::showProfileInfo);
        connect(menuBar, &MenuBar::applicationTriggered, this, &DatabaseEditor::showApplicationDialog);
        connect(menuBar, &MenuBar::exitTriggered, qApp, &QApplication::quit);
    }
}
// %%% Enhanced Dock Setup %%%
/* Setup dock widgets with full features for Linux compatibility */
void DatabaseEditor::setupEnhancedDockWidgets()
{
    QDockWidget::DockWidgetFeatures fullDockFeatures =
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable;

    // %%% Hierarchy Dock %%%
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    hierarchyDock->setFeatures(fullDockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);
    hierarchyDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // %%% Inspector Dock %%%
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    inspectorDock->setFeatures(fullDockFeatures);
    inspector = new Inspector(this);
    inspectorDock->setWidget(inspector);
    inspectorDock->setMinimumWidth(200);
    inspectorDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // %%% Console Dock %%%
    consoleDock = new QDockWidget("Console", this);
    consoleDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    consoleDock->setFeatures(fullDockFeatures);
    consoleView = new ConsoleView(this);
    consoleDock->setWidget(consoleView);
    consoleDock->setMinimumHeight(100);
    consoleDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    consoleDock->hide();

    // %%% Dock Visibility Connections %%%
    connect(hierarchyDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);
    connect(inspectorDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);
    connect(consoleDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);

    // %%% Dock Configuration %%%
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks |
                   QMainWindow::GroupedDragging | QMainWindow::AnimatedDocks);
    setCentralWidget(nullptr);
    // %%% Dock Layout %%%
    splitDockWidget(hierarchyDock, inspectorDock, Qt::Horizontal);
    splitDockWidget(inspectorDock, consoleDock, Qt::Vertical);

    // %%% Delayed Size Adjustment %%%
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

// %%% Legacy Dock Setup %%%
/* Legacy method replaced by enhanced version */
void DatabaseEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    setupEnhancedDockWidgets();
}

// %%% Dock Visibility Handler %%%
/* Handle dock widget visibility changes */
void DatabaseEditor::onDockVisibilityChanged(bool visible)
{
    QDockWidget* dock = qobject_cast<QDockWidget*>(sender());
    if (dock && visible) {
        dock->raise();
    }
}

// %%% Menu Bar Setup %%%
/* Setup main menu bar with actions */
void DatabaseEditor::setupMenuBar()
{
    MenuBar *menuBar = new MenuBar(this);
    setMenuBar(menuBar);
    menuBar->setLibraryActionsVisible(false);
    connect(menuBar, &MenuBar::feedbackTriggered, this, &DatabaseEditor::showFeedbackWindow);
    connect(menuBar, &MenuBar::newFileTriggered, this, [=]() {
        this->hierarchy->fromJson(QJsonObject());
        HierarchyConnector::instance()->initializeDummyData(this->hierarchy);
        this->clearUnsavedChanges();
        this->updateStatusBar("New database file created");
    });
    connect(menuBar->getLoadXmlAction(), &QAction::triggered,
            this, [=]() {
                QString filePath = QFileDialog::getOpenFileName(this, "Open Xml",
                                                                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                                "Xml Files (*.xml)");
                if (!filePath.isEmpty()) {
                    HierarchyConnector::instance()->openXmlFile(this->hierarchy,filePath);
                }
            });
}

// %%% Layout Reset %%%
/* Reset all dock widgets to initial positions */
void DatabaseEditor::resetLayout()
{
    // %%% Cleanup Extra Inspectors %%%
    for (int i = inspectorDocks.size() - 1; i >= 0; --i) {
        QDockWidget* dock = inspectorDocks[i];
        if (dock != inspectorDock) {
            inspectors.removeAt(i);
            dock->deleteLater();
        }
    }
    // %%% Reset Inspector Lists %%%
    if (inspectorDocks.size() > 1) {
        inspectorDocks = QList<QDockWidget*>{inspectorDock};
        inspectors = QList<Inspector*>{inspector};
    }
    inspectorCount = 0;
    // %%% Hide and Re-add Docks %%%
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
    // %%% Delayed Size Restoration %%%
    QTimer::singleShot(100, this, [=]() {
        int totalWidth = this->width();
        int totalHeight = this->height();
        int hierarchyWidth = static_cast<int>(totalWidth * 0.10);
        int inspectorWidth = static_cast<int>(totalWidth * 0.85);
        int consoleHeight = static_cast<int>(totalHeight * 0.25);
        resizeDocks({hierarchyDock}, {hierarchyWidth}, Qt::Horizontal);
        resizeDocks({inspectorDock}, {inspectorWidth}, Qt::Horizontal);
        resizeDocks({consoleDock}, {consoleHeight}, Qt::Vertical);
        updateStatusBar("Layout reset to initial state");
    });
}

// %%% Toolbar Setup %%%
/* Setup toolbars (currently empty) */
void DatabaseEditor::setupToolBars()
{
    // Toolbar setup placeholder
}

// %%% Inspector Tab Addition %%%
/* Add new inspector tab for multi-view support */
void DatabaseEditor::addInspectorTab()
{
    QDockWidget *newInspectorDock = new QDockWidget("Inspector " + QString::number(++inspectorCount), this);
    newInspectorDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    newInspectorDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    Inspector *newInspector = new Inspector(newInspectorDock);
    newInspectorDock->setWidget(newInspector);
    newInspectorDock->setMinimumWidth(200);
    newInspectorDock->setTitleBarWidget(nullptr);
    newInspector->setDatabaseEditorMode(true);
    inspectorDocks.append(newInspectorDock);
    inspectors.append(newInspector);
    // %%% Inspector Connections %%%
    connect(newInspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(newInspector, &Inspector::valueChanged, this, &DatabaseEditor::markUnsavedChanges);
    connect(newInspector, &Inspector::valueChanged, this,
            [this](QString ID, QString, QJsonObject ) {
                markEntityChanged(ID);
            });
    connect(newInspector, &Inspector::addTabRequested, this, &DatabaseEditor::addInspectorTab);
    connect(newInspectorDock, &QDockWidget::visibilityChanged, this, &DatabaseEditor::onDockVisibilityChanged);
    // %%% Dock Placement %%%
    if (inspectorDock->isVisible()) {
        splitDockWidget(inspectorDock, newInspectorDock, Qt::Horizontal);
    } else if (hierarchyDock->isVisible()) {
        splitDockWidget(hierarchyDock, newInspectorDock, Qt::Horizontal);
    } else {
        addDockWidget(Qt::RightDockWidgetArea, newInspectorDock);
    }
    // %%% Cleanup on Destroy %%%
    connect(newInspectorDock, &QDockWidget::destroyed, this, [=]() {
        inspectorDocks.removeOne(newInspectorDock);
        inspectors.removeOne(newInspector);
    });
    newInspectorDock->show();
    newInspectorDock->raise();
}

// %%% Feedback Window %%%
/* Show feedback dialog window */
void DatabaseEditor::showFeedbackWindow()
{
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->show();
}

// %%% Destructor %%%
/* Clean up database editor resources */
DatabaseEditor::~DatabaseEditor()
{
    if (scenario) {
        delete scenario;
    }
}

// %%% Unsaved Changes Management %%%
/* Mark editor as having unsaved changes */
void DatabaseEditor::markUnsavedChanges()
{
    if (!hasUnsavedChanges) {
        hasUnsavedChanges = true;
        emit unsavedChangesChanged(true);
        setWindowTitle("Database Editor *");
    }
}

/* Clear unsaved changes flag */
void DatabaseEditor::clearUnsavedChanges()
{
    if (hasUnsavedChanges) {
        hasUnsavedChanges = false;
        emit unsavedChangesChanged(false);
        setWindowTitle("Database Editor");
    }
}

// %%% Status Bar Setup %%%
/* Setup status bar for messages */
void DatabaseEditor::setupStatusBar()
{
    statusBar = new QStatusBar(this);
    statusBar->showMessage("Ready");
}
/* Update status bar with message */
void DatabaseEditor::updateStatusBar(const QString &message)
{
    if (statusBar) {
        statusBar->showMessage(message);
    }
}

// %%% Project Loading %%%
/* Load project from file with progress dialog */
void DatabaseEditor::loadRecentProject(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to open database file");
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error", "Invalid database file format");
        return;
    }
    QJsonObject obj = doc.object();
    if (!obj.contains("hierarchy")) {
        QMessageBox::warning(this, "Error", "Not a valid database file");
        return;
    }
    QJsonObject hier = obj["hierarchy"].toObject();
    hierarchy->fromJson(hier);
    lastSavedFilePath = filePath;
    clearUnsavedChanges();
    RecentProjectsManager::instance()->addToRecentProjects(filePath, RecentProjectsManager::DatabaseEditor);
    updateStatusBar("database loaded: " + QFileInfo(filePath).fileName());
    emit hierarchyLoaded(hier);
    emit databaseFileLoaded(filePath);
}

// %%% Recent Projects Menu %%%
/* Show recent projects menu */
void DatabaseEditor::onRecentProjectTriggered()
{
    RecentProjectsManager::instance()->showRecentProjectsMenu(this, RecentProjectsManager::DatabaseEditor);
}
// %%% Profile Info Dialog %%%
/* Show profile information dialog */
void DatabaseEditor::showProfileInfo()
{
    ProfileInfoDialog::showProfileInfo(this);
}
// %%% Application Settings Dialog %%%
/* Show application settings dialog */
void DatabaseEditor::showApplicationDialog()
{
    ApplicationDialog dialog(this);
    dialog.exec();
}
void DatabaseEditor::loadFromJsonFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", QString("Failed to open JSON file: %1").arg(filePath));
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error", QString("Failed to parse JSON: %1").arg(err.errorString()));
        return;
    }
    QJsonObject obj = doc.object();
    if (obj.contains("hierarchy")) {
        QJsonObject hier = obj["hierarchy"].toObject();
        hierarchy->fromJson(hier);
        emit hierarchyLoaded(hier);
        if (treeView && treeView->getTreeWidget()) {
            treeView->getTreeWidget()->update();
        }
    }
    lastSavedFilePath = filePath;
    clearUnsavedChanges();
    updateStatusBar("Project loaded: " + QFileInfo(filePath).fileName());
    emit databaseFileLoaded(filePath);
}
// %%% Tree Item Selection Handler %%%
/* Handle selection of items in hierarchy tree */
// %%% Inspector Cleanup %%%
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
    QString ID = data.contains("ID") ? data["ID"].toString() : data["parentId"].toString();
    QString displayName = capitalizeFirstLetter(name);

    cleanupExtraInspectors();

    if (inspector) {
        inspector->resetState();
    }

    // Inspector dock wapas normal inspector pe (canvas ke baad)
    if (inspectorDock->widget() != inspector) {
        inspectorDock->setWidget(inspector);
        inspectorDock->setWindowTitle("Inspector");
    }

    // ── ENTITY ────────────────────────────────────────────────────────
    if (type == "entity") {
        QString entityId = data["ID"].toString();

        // Formation entity → canvas
        bool isFormation = false;
        auto entityIt = hierarchy->Entities.find(entityId.toStdString());
        if (entityIt != hierarchy->Entities.end() && entityIt->second) {
            if (entityIt->second->type == Constants::EntityType::Formation)
                isFormation = true;
            if (!isFormation && dynamic_cast<Formation*>(entityIt->second) != nullptr)
                isFormation = true;
        }

        if (isFormation) {
            showFormationCanvas(entityId, displayName);
            return;
        }

        showAllEntityComponents(entityId, displayName);

        if (entityIt != hierarchy->Entities.end() && entityIt->second) {
            QJsonObject entityData = entityIt->second->toJson();

            QString entityType;
            if (entityData.contains("type") && entityData["type"].isObject())
                entityType = entityData["type"].toObject().value("value").toString();

            // Platform ka data grid mein already aa chuka – dubara init mat karo
            // Radio / IFF / Sensor / … ke liye full JSON
            if (entityType != "Platform") {
                if (inspectorDock->widget() != inspector) {
                    inspectorDock->setWidget(inspector);
                    inspectorDock->setWindowTitle("Inspector");
                }
                inspector->init(entityId, displayName + "_self", entityData);
            }
        }
    }
    // ── SUBCOMPONENT ──────────────────────────────────────────────────
    else if (type == "subcomponent") {
        QString subCompId = data["ID"].toString();
        QString parentCompId = data["parentId"].toString();
        QJsonObject subComponentData;
        auto compIt = hierarchy->Components.find(parentCompId.toStdString());
        if (compIt != hierarchy->Components.end() && compIt->second) {
            subComponentData = compIt->second->getsubComponentData(subCompId.toStdString());
        }
        if (!subComponentData.isEmpty()) {
            inspector->init(subCompId, displayName + "_sub", subComponentData);
        }
    }
    // ── COMPONENT (Collider, Transform, …) ────────────────────────────
    else if (type == "component") {
        // parent entity ID + component name
        QString parentId = data.contains("parentId") ? data["parentId"].toString()
                                                     : data["ID"].toString();
        // kuch trees mein ID = entity, name = component
        if (data.contains("parentId"))
            parentId = data["parentId"].toString();
        else if (data.contains("ID"))
            parentId = data["ID"].toString();

        QJsonObject componentData = hierarchy->getComponentData(parentId, name);
        // fallback: lowercase name
        if (componentData.isEmpty())
            componentData = hierarchy->getComponentData(parentId, name.toLower());

        if (!componentData.isEmpty()) {
            inspector->init(parentId, displayName, componentData);
        } else {
            qWarning() << "[Inspector] No data for component" << name
                       << "parent" << parentId;
        }
    }
    // ── PROFILE ───────────────────────────────────────────────────────
    else if (type == "profile") {
        QString profileId = data.contains("ID") ? data["ID"].toString()
                                                : data["parentId"].toString();
        auto profileIt = hierarchy->ProfileCategories.find(profileId.toStdString());
        if (profileIt != hierarchy->ProfileCategories.end()) {
            inspector->init(profileId, displayName + "_self", profileIt->second->toJson());
        }
    }
    // ── FOLDER ────────────────────────────────────────────────────────
    else if (type == "folder") {
        QString folderId = data.contains("ID") ? data["ID"].toString()
                                               : data["parentId"].toString();
        auto folderIt = hierarchy->Folders.find(folderId.toStdString());
        if (folderIt != hierarchy->Folders.end()) {
            inspector->init(folderId, displayName + "_self", folderIt->second->toJson());
        }
    }
    // ── DEFAULT ───────────────────────────────────────────────────────
    else {
        inspector->init(ID, displayName, QJsonObject());
    }

    if (!inspectorDock->isVisible()) {
        addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
        inspectorDock->show();
    }
}

/* Clean up extra inspector widgets and restore main inspector */
void DatabaseEditor::cleanupExtraInspectors()
{
    bool isAllComponentsMode = inspectorDock->property("IsAllComponentsMode").toBool();
    if (isAllComponentsMode) {
        // Restore original inspector widget
        inspectorDock->setWidget(inspector);
        inspectorDock->setWindowTitle("Inspector");
        // Clear properties
        inspectorDock->setProperty("AllComponentsWidget", QVariant());
        inspectorDock->setProperty("IsAllComponentsMode", false);
        inspectorDock->setProperty("EntityId", QVariant());
        // Delete temporary inspectors
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
        // Reset main inspector state
        if (inspector) {
            inspector->resetState();
        }
    }
    // Remove extra inspector docks
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
    // Ensure inspector dock is visible
    if (!inspectorDock->isVisible()) {
        inspectorDock->show();
    }
}

// %%% Entity Component Display %%%
/* Show all components of an entity in a grid layout */
void DatabaseEditor::showAllEntityComponents(const QString& entityId, const QString& entityName)
{

    QString entityType = "";
    auto entityIt = hierarchy->Entities.find(entityId.toStdString());
    if (entityIt != hierarchy->Entities.end()) {
        QJsonObject entityData = entityIt->second->toJson();
        if (entityData.contains("type") && entityData["type"].isObject()) {
            QJsonObject typeObj = entityData["type"].toObject();
            if (typeObj.contains("value")) {
                entityType = typeObj["value"].toString();
            }
        }
    }
    // Non-platform entities show simplified view
    if (entityType != "Platform") {
        if (entityIt != hierarchy->Entities.end()) {
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
    // %%% Platform Entity Grid Layout %%%
    QWidget *container = new QWidget();
    QGridLayout *gridLayout = new QGridLayout(container);
    gridLayout->setContentsMargins(10, 10, 10, 10);
    gridLayout->setHorizontalSpacing(15);
    gridLayout->setVerticalSpacing(15);

    // Component layout configuration
    QMap<QString, int> componentMinHeights = {
        {"entity", 800},
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
    QStringList hiddenComponents = {"collider", "transform", "trajectory", "rigidbody", /*"entity", */"crossSection"};
    QStringList dynamicModelBitmapColumn = {"dynamicModel", "bitmap"};
    QStringList iffRadioColumnComponents = {"iffs", "radios"};
    QStringList group1Components = {"transform", "sensors"};
    QStringList group2Components = {"trajectory", "collider"};
    QStringList exclusiveSingleColumnComponents = {"crossSection", "rigidbody", "sensors"};
    QStringList componentOrder = {"entity", "transform", "trajectory", "dynamicModel", "collider", "rigidbody",
                                  "bitmap", "crossSection", "sensors", "radios", "iffs", "parameters"};

    // Collect available components
    QList<QPair<QString, int>> availableComponents;
    for (const QString& compType : componentOrder) {
        if (hiddenComponents.contains(compType)) continue;

        QJsonObject componentData;
        if (compType == "entity") {
            auto entityIt = hierarchy->Entities.find(entityId.toStdString());
            if (entityIt != hierarchy->Entities.end()) {
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

    // %%% Component Categorization %%%
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

    // %%% Column Calculation %%%
    int totalColumns = 0;
    totalColumns += exclusiveComponents.size();
    totalColumns += !dynamicModelBitmapList.isEmpty() ? 1 : 0;
    totalColumns += !iffRadioColumnList.isEmpty() ? 1 : 0;
    totalColumns += !group1ComponentsList.isEmpty() ? 1 : 0;
    totalColumns += !group2ComponentsList.isEmpty() ? 1 : 0;
    int otherComponentCount = otherComponents.size();
    totalColumns += (otherComponentCount + 1) / 2;
    // %%% Column Setup %%%
    QVector<QWidget*> columnWidgets(totalColumns);
    QVector<QVBoxLayout*> columnLayouts(totalColumns);
    QMap<QString, int> componentColumnMap;
    int currentColumn = 0;

    QVector<QList<QPair<QString, int>>> otherColumnComponents((otherComponentCount + 1) / 2);
    for (int i = 0; i < otherComponentCount; i++) {
        int columnIndex = i % otherColumnComponents.size();
        otherColumnComponents[columnIndex].append(otherComponents[i]);
    }
    for (int col = 0; col < otherColumnComponents.size(); col++) {
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
    currentColumn += otherColumnComponents.size();
    // Dynamic model/bitmap column
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

    // IFF/Radio column
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

    // Group 1 column
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

    // Group 2 column
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

    for (const auto& comp : exclusiveComponents) {
        columnWidgets[currentColumn] = new QWidget();
        columnLayouts[currentColumn] = new QVBoxLayout(columnWidgets[currentColumn]);
        columnLayouts[currentColumn]->setContentsMargins(0, 0, 0, 0);
        columnLayouts[currentColumn]->setSpacing(0);
        columnLayouts[currentColumn]->addStretch(1);
        componentColumnMap[comp.first] = currentColumn;
        currentColumn++;
    }
    // %%% Other Components Distribution %%%


    // %%% Component Widget Creation %%%
    for (const auto& comp : availableComponents) {
        QString compType = comp.first;
        if (hiddenComponents.contains(compType)) continue;
        int minHeight = componentMinHeights[compType];
        int columnIndex = componentColumnMap[compType];
        // Dynamic height calculation for certain components
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
            auto entityIt = hierarchy->Entities.find(entityId.toStdString());
            if (entityIt != hierarchy->Entities.end()) {
                componentData = filterEntityJsonForInspector(entityIt->second->toJson());
                Inspector *entityInspector = new Inspector();
                entityInspector->setHierarchy(hierarchy);
                entityInspector->setDatabaseEditorMode(true);
                entityInspector->init(entityId, "entity_self", componentData);
                connect(entityInspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
                connect(entityInspector, &Inspector::valueChanged, this, &DatabaseEditor::markUnsavedChanges);
                connect(entityInspector, &Inspector::valueChanged, this,
                        [this](QString ID, QString, QJsonObject) {
                            markEntityChanged(ID);
                        });
                connect(entityInspector, &Inspector::valueChanged, this,
                        [=](QString eID, QString compName, QJsonObject delta) {
                            if (!delta.contains("Category")) return;
                            QTimer::singleShot(50, this, [=]() {
                                auto it = hierarchy->Entities.find(eID.toStdString());
                                if (it == hierarchy->Entities.end()) return;
                                QVariantMap data;
                                data["type"] = "entity";
                                data["ID"] = eID;
                                data["name"] = QString::fromStdString(it->second->Name);
                                data["parentId"] = QString::fromStdString(it->second->parentID);
                                onTreeItemSelected(data);
                            });
                        });
                entityInspector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                entityInspector->setMinimumWidth(350);
                entityInspector->setMinimumHeight(minHeight);
                entityInspector->setMaximumHeight(16777215);

                QFrame *frame = new QFrame();
                frame->setStyleSheet(DatabaseStyles::ComponentFrame);
                frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

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

        // Create component widget
        QWidget *compWidget = nullptr;
        if (needsDynamicHeight) {
            compWidget = createComponentInspectorWithDynamicHeight(entityId, displayName, componentData, actualHeight);
        } else {
            compWidget = createComponentInspector(entityId, displayName, componentData, actualHeight);
        }
        // Set size policies based on component type
        if (exclusiveSingleColumnComponents.contains(compType)) {
            compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        }
        else if (dynamicModelBitmapColumn.contains(compType)) {
            compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            compWidget->setMinimumHeight(minHeight);
            compWidget->setMaximumHeight(minHeight * 1.2);
        }
        else if (iffRadioColumnComponents.contains(compType)) {
            compWidget->setSizePolicy(QSizePolicy::Expanding, needsDynamicHeight ? QSizePolicy::Preferred : QSizePolicy::Minimum);
            if (needsDynamicHeight) compWidget->setMinimumHeight(actualHeight);
        }
        else if (group1Components.contains(compType) || group2Components.contains(compType)) {
            compWidget->setSizePolicy(QSizePolicy::Expanding, needsDynamicHeight ? QSizePolicy::Preferred : QSizePolicy::Minimum);
            if (needsDynamicHeight) compWidget->setMinimumHeight(actualHeight);
        } else {
            compWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        }
        columnLayouts[columnIndex]->insertWidget(columnLayouts[columnIndex]->count() - 1, compWidget);
    }

    // %%% Grid Layout Assembly %%%
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
    scrollArea->setStyleSheet(
        "QScrollArea {"
        "    background-color: #0F2636;"
        "    border: none;"
        "}"
        "QScrollArea > QWidget > QWidget {"
        "    background-color: #0F2636;"
        "}"
        "QScrollBar:vertical {"
        "    background-color: #0F2636;"
        "    width: 12px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background-color: #3A506B;"
        "    min-height: 20px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background-color: #4A607B;"
        "}"
        "QScrollBar:horizontal {"
        "    background-color: #0F2636;"
        "    height: 12px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background-color: #3A506B;"
        "    min-width: 20px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background-color: #4A607B;"
        "}"
        );

    // %%% Inspector Collection %%%
    QList<Inspector*> allInspectors = container->findChildren<Inspector*>();
    // %%% Inspector Dock Configuration %%%
    inspectorDock->setWidget(scrollArea);
    inspectorDock->setWindowTitle(entityName + " - All Components");
    inspectorDock->setProperty("IsAllComponentsMode", true);
    inspectorDock->setProperty("EntityId", entityId);
    inspectorDock->setProperty("AllComponentsWidget", QVariant::fromValue(scrollArea));
    inspectorDock->setProperty("AllInspectors", QVariant::fromValue(allInspectors));
}


// %%% Component Inspector Creation %%%
/* Create inspector widget for single component */
QWidget* DatabaseEditor::createComponentInspector(
    const QString& entityId,
    const QString& title,
    const QJsonObject& data,
    int preferredHeight)
{
    Inspector *inspector = new Inspector();
    inspector->setHierarchy(hierarchy);
        inspector->setDatabaseEditorMode(true);
    QString componentName = title.toLower();
    inspector->init(entityId, componentName, data);
    inspector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    inspector->setMinimumWidth(350);
    inspector->setMinimumHeight(preferredHeight);
    inspector->setMaximumHeight(preferredHeight * 1.1);

    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::valueChanged, this, &DatabaseEditor::markUnsavedChanges);
    connect(inspector, &Inspector::valueChanged, this,
            [this](QString ID, QString /*name*/, QJsonObject /*delta*/) {
                markEntityChanged(ID);
            });
    QFrame *frame = new QFrame();
    frame->setStyleSheet(DatabaseStyles::ComponentFrame);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(1);
    frameLayout->addWidget(inspector);
    return frame;
}

/* Create inspector widget with dynamic height for expandable components */
QWidget* DatabaseEditor::createComponentInspectorWithDynamicHeight(
    const QString& entityId,
    const QString& title,
    const QJsonObject& data,
    int initialHeight)
{
    Inspector *inspector = new Inspector();
    inspector->setHierarchy(hierarchy);
    inspector->setDatabaseEditorMode(true);
    QString componentName = title.toLower();
    if (componentName == "entity") {
        componentName = "entity_self";
    }
    inspector->init(entityId, componentName, data);
    inspector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    inspector->setMinimumWidth(350);
    inspector->setMinimumHeight(initialHeight);
    inspector->setMaximumHeight(16777215);

    connect(inspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(inspector, &Inspector::valueChanged, this, &DatabaseEditor::markUnsavedChanges);
    connect(inspector, &Inspector::valueChanged, this,
            [this](QString ID, QString /*name*/, QJsonObject /*delta*/) {
                markEntityChanged(ID);
            });

    QFrame *frame = new QFrame();
    frame->setStyleSheet(DatabaseStyles::ComponentFrame);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(1);
    frameLayout->addWidget(inspector);
    return frame;
}

void DatabaseEditor::markEntityChanged(const QString& entityId)
{
    if (entityId.isEmpty()) {
        return;
    }
    m_modifiedEntityIds.insert(entityId);
}
void DatabaseEditor::showFormationCanvas(const QString& entityId, const QString& entityName)
{
    cleanupExtraInspectors();

    // ── Outer container ───────────────────────────────────────────────
    QWidget *container = new QWidget();
    QVBoxLayout *outerLayout = new QVBoxLayout(container);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // ── Splitter (vertical) ───────────────────────────────────────────
    QSplitter *splitter = new QSplitter(Qt::Vertical, container);
    splitter->setHandleWidth(6);
    splitter->setChildrenCollapsible(false);
    splitter->setStyleSheet(
        "QSplitter::handle:vertical {"
        "  background-color: #27446d;"
        "  height: 6px;"
        "  margin: 0 8px;"
        "  border-radius: 3px;"
        "}"
        "QSplitter::handle:vertical:hover {"
        "  background-color: #4A90D9;"
        "}"
        "QSplitter::handle:vertical:pressed {"
        "  background-color: #5BA3E8;"
        "}"
        );

    // ══════════════════════════════════════════════════════════════════
    // TOP: Formation_self fields
    // ══════════════════════════════════════════════════════════════════
    QWidget *topWidget = new QWidget();
    QVBoxLayout *topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(6, 6, 6, 4);
    topLayout->setSpacing(4);

    QJsonObject entityData;
    auto entityIt = hierarchy->Entities.find(entityId.toStdString());
    if (entityIt != hierarchy->Entities.end() && entityIt->second) {
        entityData = entityIt->second->toJson();
    }

    Inspector *formInspector = new Inspector(topWidget);
    formInspector->setHierarchy(hierarchy);
    formInspector->setDatabaseEditorMode(true);
    formInspector->init(entityId, entityName + "_self", entityData);
    formInspector->setMinimumHeight(120);

    connect(formInspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(formInspector, &Inspector::valueChanged, this, &DatabaseEditor::markUnsavedChanges);
    connect(formInspector, &Inspector::valueChanged, this,
            [this](QString ID, QString, QJsonObject) {
                markEntityChanged(ID);
            });

    topLayout->addWidget(formInspector);
    topWidget->setMinimumHeight(100);

    // ══════════════════════════════════════════════════════════════════
    // BOTTOM: Canvas
    // ══════════════════════════════════════════════════════════════════
    QWidget *bottomWidget = new QWidget();
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(6, 4, 6, 6);
    bottomLayout->setSpacing(6);

    QLabel *canvasTitle = new QLabel("Custom Formation Shape (offsets)");
    canvasTitle->setStyleSheet("color:white; font-size:13px; font-weight:bold;");
    bottomLayout->addWidget(canvasTitle);

    QLabel *info = new QLabel(
        "Click = add | Drag = move | Right-click = remove |  Blue = Mothership");
    info->setStyleSheet("color:#A0C0D0; font-size:11px;");
    info->setWordWrap(true);
    bottomLayout->addWidget(info);

    FormationCanvasWidget *canvas = new FormationCanvasWidget(bottomWidget);
    canvas->setMinimumHeight(200);
    bottomLayout->addWidget(canvas, 1);

    bottomWidget->setMinimumHeight(180);

    // ── Load existing offsets ─────────────────────────────────────────
    QJsonArray existingPositions;
    if (entityIt != hierarchy->Entities.end()) {
        Formation *form = dynamic_cast<Formation*>(entityIt->second);
        if (form && form->formationPositions) {
            for (int i = 0; ; ++i) {
                std::string key = "ally_" + std::to_string(i);
                auto pit = form->formationPositions->find(key);
                if (pit == form->formationPositions->end()) break;
                FormationPosition *pos = pit->second;
                if (!pos || !pos->Offset) continue;
                QJsonObject pt;
                pt["x"] = pos->Offset->x;
                pt["y"] = pos->Offset->z;
                existingPositions.append(pt);
            }
        }
    }
    canvas->setPositions(existingPositions);

    // ── Save ──────────────────────────────────────────────────────────
    connect(canvas, &FormationCanvasWidget::positionsChanged, this,
            [=](const QJsonArray &positions) {
                auto eit = hierarchy->Entities.find(entityId.toStdString());
                if (eit == hierarchy->Entities.end() || !eit->second) return;

                Formation *form = dynamic_cast<Formation*>(eit->second);
                if (!form || !form->formationPositions) return;

                std::vector<std::string> keysToRemove;
                for (const auto& pair : *form->formationPositions)
                    keysToRemove.push_back(pair.first);
                for (const auto& name : keysToRemove)
                    form->removeComponent(name);
                form->formationPositions->clear();

                int i = 0;
                for (const QJsonValue& v : positions) {
                    QJsonObject pt = v.toObject();
                    double x = pt.value("x").toDouble();
                    double y = pt.value("y").toDouble();
                    std::string name = "ally_" + std::to_string(i);
                    form->addComponent(name);
                    FormationPosition *pos = (*form->formationPositions)[name];
                    if (pos && pos->Offset) {
                        pos->Offset->x = static_cast<float>(x);
                        pos->Offset->y = 0.0f;
                        pos->Offset->z = static_cast<float>(y);
                    }
                    i++;
                }
                form->count = positions.size();
                markUnsavedChanges();
                markEntityChanged(entityId);
            });

    // ── Splitter assemble ─────────────────────────────────────────────
    splitter->addWidget(topWidget);
    splitter->addWidget(bottomWidget);
    // Initial sizes: top ~35%, bottom ~65%
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({220, 450});

    outerLayout->addWidget(splitter);

    // ── Dock ──────────────────────────────────────────────────────────
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setWidget(container);
    scroll->setStyleSheet(
        "QScrollArea { background-color: #0F2636; border: none; }"
        "QScrollBar:vertical { background:#0F2636; width:10px; }"
        "QScrollBar::handle:vertical { background:#3A506B; border-radius:5px; }");

    inspectorDock->setWidget(scroll);
    inspectorDock->setWindowTitle(entityName + " – Formation + Canvas");
    inspectorDock->setProperty("IsAllComponentsMode", true);
    inspectorDock->setProperty("EntityId", entityId);
    inspectorDock->show();
    inspectorDock->raise();
}
void DatabaseEditor::showFormationProfileWithCanvas(const QString& profileId,
                                                    const QString& profileName,
                                                    const QJsonObject& profileData)
{
    cleanupExtraInspectors();

    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(10);

    // ── 1) Normal profile inspector (upar) ─────────────────────────────
    Inspector *profileInspector = new Inspector(container);
    profileInspector->setHierarchy(hierarchy);
    profileInspector->setDatabaseEditorMode(true);
    profileInspector->init(profileId, profileName + "_self", profileData);
    profileInspector->setMinimumHeight(120);
    profileInspector->setMaximumHeight(180);

    connect(profileInspector, &Inspector::valueChanged, hierarchy, &Hierarchy::UpdateComponent);
    connect(profileInspector, &Inspector::valueChanged, this, &DatabaseEditor::markUnsavedChanges);

    layout->addWidget(profileInspector);

    // Separator
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #27446d;");
    layout->addWidget(line);

    // ── 2) Canvas (neeche) ─────────────────────────────────────────────
    QLabel *canvasTitle = new QLabel("Custom Formation Shape (offsets)");
    canvasTitle->setStyleSheet("color:white; font-size:13px; font-weight:bold;");
    layout->addWidget(canvasTitle);

    QLabel *info = new QLabel(
        "Click = add point | Drag = move | Right-click = remove\n"
        "Blue circle = Mothership origin. Offsets relative to it.");
    info->setStyleSheet("color:#A0C0D0; font-size:11px;");
    info->setWordWrap(true);
    layout->addWidget(info);

    FormationCanvasWidget *canvas = new FormationCanvasWidget(container);
    canvas->setMinimumHeight(400);
    layout->addWidget(canvas, 1);

    // Existing positions load (profile pe store karenge)
    QJsonArray existingPositions;
    if (profileData.contains("FormationPositions") && profileData["FormationPositions"].isArray()) {
        existingPositions = profileData["FormationPositions"].toArray();
    }
    canvas->setPositions(existingPositions);

    // Save on change → profile pe
    connect(canvas, &FormationCanvasWidget::positionsChanged, this,
            [=](const QJsonArray &positions) {

                // Profile ke under pehli Formation entity dhoondo
                Formation* form = nullptr;
                QString formEntityId;

                for (const auto& epair : hierarchy->Entities) {
                    Entity* e = epair.second;
                    if (!e) continue;

                    if (QString::fromStdString(e->parentID) != profileId)
                        continue;

                    Formation* f = dynamic_cast<Formation*>(e);
                    if (f) {
                        form = f;
                        formEntityId = QString::fromStdString(e->ID);
                        break;
                    }
                    if (e->type == Constants::EntityType::Formation) {
                        form = dynamic_cast<Formation*>(e);
                        formEntityId = QString::fromStdString(e->ID);
                        if (form) break;
                    }
                }

                if (!form || !form->formationPositions) {
                    qWarning() << "[FormationCanvas] No Formation entity under profile."
                               << "Create a Formation entity first, then draw points.";
                    return;
                }

                // 1) Purane ally slots hatao
                std::vector<std::string> keysToRemove;
                for (const auto& pair : *form->formationPositions) {
                    keysToRemove.push_back(pair.first);
                }
                for (const auto& name : keysToRemove) {
                    form->removeComponent(name);
                }
                form->formationPositions->clear();

                // 2) Canvas points se naye slots
                // Canvas (x,y) → Formation Offset (x, z), y=0
                int i = 0;
                for (const QJsonValue& v : positions) {
                    QJsonObject pt = v.toObject();
                    double x = pt.value("x").toDouble();
                    double y = pt.value("y").toDouble();

                    std::string name = "ally_" + std::to_string(i);
                    form->addComponent(name);

                    FormationPosition* pos = (*form->formationPositions)[name];
                    if (pos && pos->Offset) {
                        pos->Offset->x = static_cast<float>(x);
                        pos->Offset->y = 0.0f;
                        pos->Offset->z = static_cast<float>(y);
                    }
                    i++;
                }

                form->count = positions.size();

                markUnsavedChanges();
                markEntityChanged(formEntityId);

                // Verify
                QJsonObject check = form->toJson();
                int saved = 0;
                if (check.contains("formationPositions") && check["formationPositions"].isArray())
                    saved = check["formationPositions"].toArray().size();

                qDebug() << "[FormationCanvas] profile→entity" << formEntityId
                         << "slots=" << form->count
                         << "toJson formationPositions=" << saved;
            });

    // Dock mein daalo
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setWidget(container);
    scroll->setStyleSheet(
        "QScrollArea { background-color: #0F2636; border: none; }"
        "QScrollBar:vertical { background:#0F2636; width:10px; }"
        "QScrollBar::handle:vertical { background:#3A506B; border-radius:5px; }");

    inspectorDock->setWidget(scroll);
    inspectorDock->setWindowTitle(profileName + " – Profile + Canvas");
    inspectorDock->setProperty("IsAllComponentsMode", true);
    inspectorDock->setProperty("EntityId", profileId);
    inspectorDock->show();
    inspectorDock->raise();
}
