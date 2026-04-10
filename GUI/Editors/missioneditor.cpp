/* ========================================================================= */
/* File: MissionEditor.cpp                                                 */
/* Purpose: Implements database editor with hierarchy and doctrine views     */
//               Written by Arti Rajpoot
/* ========================================================================= */

#include "missioneditor.h"                        // For database editor class
#include "GUI/Feedback/projectinformation.h"      // For feedback window
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
#include <tests/missioneditortest/gui_mission_test.h>
#include <QProgressDialog>
#include <QScrollArea>


// %%% Utility Functions %%%
/* Capitalize the first letter of a string */
static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}

// %%% Constructor %%%
/* Initialize database editor with scenario and UI components */
MissionEditor::MissionEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Database Editor");
    resize(1100, 600);

    // %%% UI Setup %%%
    setupEnhancedDockWidgets();
    setupToolBars();


    // %%% Core Data Setup %%%
    scenario = new Scenario();
    scenario->hierarchy->isDatabase = true;
    hierarchy = scenario->hierarchy;
    console = scenario->console;
    lastSavedFilePath = "";

    // %%% Console Setup %%%
    consoleView->setConsoleDock(consoleDock);
    connect(console, &Console::logUpdate, this, [=](std::string log) {
        if (consoleView) consoleView->appendText(QString::fromStdString(log));
    });
    connect(console, &Console::errorUpdate, this, [=](std::string error) {
        if (consoleView) consoleView->appendText(QString::fromStdString(error));
    });
    connect(console, &Console::warningUpdate, this, [=](std::string warning) {
        if (consoleView) consoleView->appendText(QString::fromStdString(warning));
    });
    connect(console, &Console::debugUpdate, this, [=](std::string debug) {
        if (consoleView) consoleView->appendText(QString::fromStdString(debug));
    });

    // %%% Recent Projects Setup %%%
    connect(RecentProjectsManager::instance(), &RecentProjectsManager::projectSelected,
            this, [=](const QString& filePath, RecentProjectsManager::EditorType type) {
                if (type == RecentProjectsManager::MissionEditor) {
                    loadRecentProject(filePath);
                }
            });
    // %%% Hierarchy Change Tracking %%%
    connect(hierarchy, &Hierarchy::profileAdded,   this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderAdded,    this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityAdded,    this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentAdded, this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRemoved, this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRemoved,  this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRemoved,  this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::componentRemoved, this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::profileRenamed, this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::folderRenamed,  this, &MissionEditor::markUnsavedChanges);
    connect(hierarchy, &Hierarchy::entityRenamed,  this, &MissionEditor::markUnsavedChanges);

    // %%% Doctrine Panel Change Tracking %%%
    connect(doctrinePanel, &DoctrineParameters::valueChanged,
            this, &MissionEditor::markUnsavedChanges);

    // %%% Hierarchy Connector Setup %%%
    if (hierarchy && treeView) {
        HierarchyConnector::instance()->connectSignals(hierarchy, nullptr, treeView);
        HierarchyConnector::instance()->initializeDummyData(hierarchy);
        HierarchyConnector::instance()->setupFileOperations(this, hierarchy, nullptr);
    }

    // %%% Tree View Signals %%%
    if (treeView && hierarchy) {
        connect(treeView, &HierarchyTree::itemSelected, this, &MissionEditor::onTreeItemSelected);
    }

    // %%% Menu Bar Setup %%%
    MenuBar* menuBar = qobject_cast<MenuBar*>(this->menuBar());
    if (menuBar) {
        connect(menuBar->getSaveAction(),        &QAction::triggered, this, &MissionEditor::clearUnsavedChanges);
        connect(menuBar->getSameSaveAction(),    &QAction::triggered, this, &MissionEditor::clearUnsavedChanges);
        connect(menuBar->getRecentProjectAction(), &QAction::triggered, this, &MissionEditor::onRecentProjectTriggered);
        connect(menuBar, &MenuBar::profileTriggered,     this, &MissionEditor::showProfileInfo);
        connect(menuBar, &MenuBar::applicationTriggered, this, &MissionEditor::showApplicationDialog);
        connect(menuBar, &MenuBar::exitTriggered,        qApp, &QApplication::quit);
    }
    connect(doctrinePanel, &DoctrineParameters::forceTypeChanged,
            tacticalPanel, &TacticalRules::setForceType);
    // Constructor ke end mein yeh add karein:
    setStyleSheet(R"(
    QMainWindow::separator {
        background: #2a3f5a;
        width: 3px;
        height: 3px;
    }
    QMainWindow::separator:hover {
        background: #4a9eff;
    }
    QDockWidget {
        border: 1px solid #2a3f5a;
        titlebar-close-icon: none;
        titlebar-normal-icon: none;
    }
    QDockWidget::title {
        background: #1a2a3a;
        border-bottom: 1px solid #2a3f5a;
        padding: 4px 8px;
    }
)");
}


// %%% Enhanced Dock Setup %%%
void MissionEditor::setupEnhancedDockWidgets()
{
    QDockWidget::DockWidgetFeatures fullDockFeatures =
        QDockWidget::DockWidgetClosable  |
        QDockWidget::DockWidgetMovable   |
        QDockWidget::DockWidgetFloatable;

    // %%% Hierarchy Dock (Far Left) %%%
    hierarchyDock = new QDockWidget("Editor", this);
    hierarchyDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    hierarchyDock->setFeatures(fullDockFeatures);
    treeView = new HierarchyTree(this);
    hierarchyDock->setWidget(treeView);
    hierarchyDock->setMinimumWidth(150);
    hierarchyDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // %%% Doctrine Parameters Dock (Top Center Left) %%%
    doctrineDock = new QDockWidget("Doctrine Parameters", this);
    doctrineDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    doctrineDock->setFeatures(fullDockFeatures);
    doctrinePanel = new DoctrineParameters(this);
    doctrineDock->setWidget(doctrinePanel);
    doctrineDock->setMinimumWidth(380);
    doctrineDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, doctrineDock);


    // %%% Tactical Rules Dock (Top Center Right) %%%
    tacticalDock = new QDockWidget("Tactical Rules", this);
    tacticalDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    tacticalDock->setFeatures(fullDockFeatures);
    tacticalPanel = new TacticalRules(this);
    tacticalDock->setWidget(tacticalPanel);
    tacticalDock->setMinimumWidth(280);
    tacticalDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::RightDockWidgetArea, tacticalDock);

    // %%% Doctrine Assumptions / Notes Dock (Bottom Left) %%%
    assumptionsDock = new QDockWidget("Doctrine Assumptions / Notes", this);
    assumptionsDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    assumptionsDock->setFeatures(fullDockFeatures);
    assumptionsPanel = new DoctrineAssumptionsNotes(this);
    assumptionsDock->setWidget(assumptionsPanel);
    assumptionsDock->setMinimumWidth(380);
    assumptionsDock->setMinimumHeight(150);
    assumptionsDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::BottomDockWidgetArea, assumptionsDock);

    // %%% Doctrine Area Definition Dock (Bottom Right) %%%
    areaDefinitionDock = new QDockWidget("Doctrine Area Definition", this);
    areaDefinitionDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    areaDefinitionDock->setFeatures(fullDockFeatures);
    areaDefinitionPanel = new DoctrineAreaDefinition(this);
    areaDefinitionDock->setWidget(areaDefinitionPanel);
    areaDefinitionDock->setMinimumWidth(280);
    areaDefinitionDock->setMinimumHeight(150);
    areaDefinitionDock->setTitleBarWidget(nullptr);
    addDockWidget(Qt::BottomDockWidgetArea, areaDefinitionDock);

    // %%% Console Dock (Hidden by default) %%%
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
    connect(hierarchyDock, &QDockWidget::visibilityChanged, this, &MissionEditor::onDockVisibilityChanged);
    connect(doctrineDock,  &QDockWidget::visibilityChanged, this, &MissionEditor::onDockVisibilityChanged);
    connect(tacticalDock,  &QDockWidget::visibilityChanged, this, &MissionEditor::onDockVisibilityChanged);
    connect(assumptionsDock, &QDockWidget::visibilityChanged, this, &MissionEditor::onDockVisibilityChanged);
    connect(areaDefinitionDock, &QDockWidget::visibilityChanged, this, &MissionEditor::onDockVisibilityChanged);
    connect(consoleDock,   &QDockWidget::visibilityChanged, this, &MissionEditor::onDockVisibilityChanged);

    // %%% Dock Configuration %%%
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    setDockOptions(QMainWindow::AllowNestedDocks  |
                   QMainWindow::AllowTabbedDocks  |
                   QMainWindow::GroupedDragging   |
                   QMainWindow::AnimatedDocks);
    setCentralWidget(nullptr);

    // %%% Dock Layout - Three Column Layout with Bottom Panels %%%
    // Column 1: Hierarchy (full height)
    // Column 2: Doctrine Parameters (top) + Doctrine Assumptions/Notes (bottom)
    // Column 3: Tactical Rules (top) + Doctrine Area Definition (bottom)

    // First, split hierarchy and doctrine docks horizontally
    splitDockWidget(hierarchyDock, doctrineDock, Qt::Horizontal);

    // Then split doctrine and tactical docks horizontally
    splitDockWidget(doctrineDock, tacticalDock, Qt::Horizontal);

    // Now split doctrine dock vertically to add assumptions panel below it
    splitDockWidget(doctrineDock, assumptionsDock, Qt::Vertical);

    // Split tactical dock vertically to add area definition panel below it
    splitDockWidget(tacticalDock, areaDefinitionDock, Qt::Vertical);

    // Console can be added later if needed, but keep it hidden for now
    // splitDockWidget(areaDefinitionDock, consoleDock, Qt::Vertical);

    // %%% Delayed Size Adjustment %%%
    QTimer::singleShot(100, this, [=]() {
        int totalWidth = width();
        int totalHeight = height();

        // Column widths: 15% | 42.5% | 42.5%
        int hierarchyW = static_cast<int>(totalWidth * 0.15);
        int doctrineW  = static_cast<int>(totalWidth * 0.55);   // ~60% of remaining space
        int tacticalW  = static_cast<int>(totalWidth * 0.30);   // ~40% of remaining space

        // Row heights for each column
        int topRowHeight = static_cast<int>(totalHeight * 0.6);    // 60% for top panels
        int bottomRowHeight = static_cast<int>(totalHeight * 0.4); // 40% for bottom panels

        // Set column widths
        resizeDocks({hierarchyDock}, {hierarchyW}, Qt::Horizontal);
        resizeDocks({doctrineDock}, {doctrineW}, Qt::Horizontal);
        resizeDocks({tacticalDock}, {tacticalW}, Qt::Horizontal);

        // Set row heights for the right column panels
        resizeDocks({doctrineDock, assumptionsDock}, {topRowHeight, bottomRowHeight}, Qt::Vertical);
        resizeDocks({tacticalDock, areaDefinitionDock}, {topRowHeight, bottomRowHeight}, Qt::Vertical);

        // Ensure all docks are visible
        hierarchyDock->show();
        doctrineDock->show();
        tacticalDock->show();
        assumptionsDock->show();
        areaDefinitionDock->show();
    });
}
// %%% Legacy Dock Setup %%%
/* Legacy method replaced by enhanced version */
void MissionEditor::setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures)
{
    setupEnhancedDockWidgets();

}

// %%% Dock Visibility Handler %%%
/* Handle dock widget visibility changes */
void MissionEditor::onDockVisibilityChanged(bool visible)
{
    QDockWidget* dock = qobject_cast<QDockWidget*>(sender());
    if (dock && visible) {
        dock->raise();
    }
}

// %%% Menu Bar Setup %%%
/* Setup main menu bar with actions */
void MissionEditor::setupMenuBar()
{
    MenuBar *menuBar = new MenuBar(this);
    setMenuBar(menuBar);
    menuBar->setLibraryActionsVisible(false);
    connect(menuBar, &MenuBar::feedbackTriggered, this, &MissionEditor::showFeedbackWindow);
    connect(menuBar, &MenuBar::newFileTriggered, this, [=]() {
        this->hierarchy->fromJson(QJsonObject());
        HierarchyConnector::instance()->initializeDummyData(this->hierarchy);
        if (doctrinePanel) doctrinePanel->resetState();
        this->clearUnsavedChanges();

    });
    connect(menuBar->getLoadXmlAction(), &QAction::triggered,
            this, [=]() {
                QString filePath = QFileDialog::getOpenFileName(this, "Open Xml",
                                                                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                                "Xml Files (*.xml)");
                if (!filePath.isEmpty()) {
                    HierarchyConnector::instance()->openXmlFile(this->hierarchy, filePath);
                }
            });
}

// %%% Layout Reset %%%
/* Reset all dock widgets to initial positions */
void MissionEditor::resetLayout()
{
    // %%% Hide and Re-add Docks %%%
    hierarchyDock->hide();
    doctrineDock->hide();
    consoleDock->hide();
    removeDockWidget(hierarchyDock);
    removeDockWidget(doctrineDock);
    removeDockWidget(consoleDock);

    addDockWidget(Qt::LeftDockWidgetArea,   hierarchyDock);
    addDockWidget(Qt::RightDockWidgetArea,  doctrineDock);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    splitDockWidget(hierarchyDock, doctrineDock, Qt::Horizontal);
    splitDockWidget(doctrineDock,  consoleDock,  Qt::Vertical);

    hierarchyDock->show();
    doctrineDock->show();
    consoleDock->show();

    // %%% Delayed Size Restoration %%%
    QTimer::singleShot(100, this, [=]() {
        int totalWidth  = this->width();
        int totalHeight = this->height();
        resizeDocks({hierarchyDock}, {static_cast<int>(totalWidth  * 0.20)}, Qt::Horizontal);
        resizeDocks({doctrineDock},  {static_cast<int>(totalWidth  * 0.80)}, Qt::Horizontal);
        resizeDocks({consoleDock},   {static_cast<int>(totalHeight * 0.25)}, Qt::Vertical);

    });
}

// %%% Toolbar Setup %%%
/* Setup toolbars (currently empty) */
void MissionEditor::setupToolBars()
{
    // Toolbar setup placeholder
}

// %%% Feedback Window %%%
/* Show feedback dialog window */
void MissionEditor::showFeedbackWindow()
{
    Feedback *feedbackWindow = new Feedback(this);
    feedbackWindow->show();
}

// %%% Destructor %%%
/* Clean up database editor resources */
MissionEditor::~MissionEditor()
{
    if (scenario) {
        delete scenario;
    }
}

// %%% Unsaved Changes Management %%%
/* Mark editor as having unsaved changes */
void MissionEditor::markUnsavedChanges()
{
    if (!hasUnsavedChanges) {
        hasUnsavedChanges = true;
        emit unsavedChangesChanged(true);
        setWindowTitle("Database Editor *");
    }
}

/* Clear unsaved changes flag */
void MissionEditor::clearUnsavedChanges()
{
    if (hasUnsavedChanges) {
        hasUnsavedChanges = false;
        emit unsavedChangesChanged(false);
        setWindowTitle("Database Editor");
    }
}


// %%% Project Loading %%%
/* Load project from file */
void MissionEditor::loadRecentProject(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to open mission file");
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error", "Invalid mission file format");
        return;
    }

    QJsonObject obj = doc.object();

    if (obj.contains("doctrine") && doctrinePanel)
        doctrinePanel->loadFromJson(obj["doctrine"].toObject());

    if (obj.contains("tactical") && tacticalPanel)
        tacticalPanel->loadBothTeamsFromJson(obj["tactical"].toObject());

    // if (obj.contains("assumptions") && assumptionsPanel)
    //     assumptionsPanel->loadFromJson(obj["assumptions"].toObject());

    // if (obj.contains("areaDefinition") && areaDefinitionPanel)
    //     areaDefinitionPanel->loadFromJson(obj["areaDefinition"].toObject());

    lastSavedFilePath = filePath;
    clearUnsavedChanges();
    RecentProjectsManager::instance()->addToRecentProjects(
        filePath, RecentProjectsManager::MissionEditor);

}
// %%% Recent Projects Menu %%%
/* Show recent projects menu */
void MissionEditor::onRecentProjectTriggered()
{
    RecentProjectsManager::instance()->showRecentProjectsMenu(this, RecentProjectsManager::MissionEditor);
}

// %%% Profile Info Dialog %%%
/* Show profile information dialog */
void MissionEditor::showProfileInfo()
{
    ProfileInfoDialog::showProfileInfo(this);
}

// %%% Application Settings Dialog %%%
/* Show application settings dialog */
void MissionEditor::showApplicationDialog()
{
    ApplicationDialog dialog(this);
    dialog.exec();
}

// %%% Load from JSON File %%%
void MissionEditor::loadFromJsonFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", QString("Failed to open file: %1").arg(filePath));
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error", QString("Failed to parse file: %1").arg(err.errorString()));
        return;
    }

    QJsonObject obj = doc.object();

    if (obj.contains("doctrine") && doctrinePanel)
        doctrinePanel->loadFromJson(obj["doctrine"].toObject());           // handles both old/new format

    if (obj.contains("tactical") && tacticalPanel)
        tacticalPanel->loadBothTeamsFromJson(obj["tactical"].toObject());  // handles both old/new format


    // if (obj.contains("assumptions") && assumptionsPanel)
    //     assumptionsPanel->loadFromJson(obj["assumptions"].toObject());

    // if (obj.contains("areaDefinition") && areaDefinitionPanel)
    //     areaDefinitionPanel->loadFromJson(obj["areaDefinition"].toObject());

    // NOTE: Hierarchy is NOT loaded from .ms file.
    // It is inherited from ScenarioEditor at switch time (in mainwindow.cpp).

    lastSavedFilePath = filePath;
    clearUnsavedChanges();

}
// %%% Tree Item Selection Handler %%%
/* Handle selection of items in hierarchy tree */
void MissionEditor::onTreeItemSelected(QVariantMap data)
{
    QString type = data["type"].toString();

    // %%% Show Doctrine Dock on relevant selections %%%
    if (type == "doctrine" || type == "entity" || type == "folder") {
        // Load doctrine data for this node if available
        QString nodeId = data["ID"].toString();
        QJsonObject doctrineData = hierarchy->getComponentData(nodeId, "doctrine");
        if (!doctrineData.isEmpty() && doctrinePanel) {
            doctrinePanel->loadFromJson(doctrineData);
        }

        if (doctrineDock && !doctrineDock->isVisible()) {
            addDockWidget(Qt::RightDockWidgetArea, doctrineDock);
            doctrineDock->show();
        }
    }
}
void MissionEditor::runGUITests()
{
      qDebug() << "=== runGUITests called ===";
    if (!console) {
        console = scenario->console;
    }
    static bool testsRun = false;
    if (testsRun) return;
    testsRun = true;

    runMissionEditorTests(this, console);
}
bool MissionEditor::isHierarchyDockVisible() const {
    return hierarchyDock && hierarchyDock->isVisible();
}

bool MissionEditor::isDoctrineDockVisible() const {
    return doctrineDock && doctrineDock->isVisible();
}

bool MissionEditor::isTacticalDockVisible() const {
    return tacticalDock && tacticalDock->isVisible();
}

bool MissionEditor::isAssumptionsDockVisible() const {
    return assumptionsDock && assumptionsDock->isVisible();
}

bool MissionEditor::isAreaDefinitionDockVisible() const {
    return areaDefinitionDock && areaDefinitionDock->isVisible();
}
