
/* ========================================================================= */
/* File: MissionEditor.h                                                   */
/* Purpose: Defines the main window for the database editor application      */
// Written by   : Arti Rajpoot
/* ========================================================================= */




#ifndef MissionEditor_H
#define MissionEditor_H

#include "GUI/Hierarchytree/hierarchyconnector.h"  // For hierarchy connections
#include "GUI/Hierarchytree/hierarchytree.h"       // For hierarchy tree view
#include "GUI/Console/consoleview.h"                // For console view
#include "core/Debug/console.h"                     // For console debugging
#include "core/structure/scenario.h"               // For scenario data structure
#include "qdockwidget.h"                           // For dock widget functionality
#include "GUI/DOCTRINE/doctrineparameters.h"
#include "GUI/DOCTRINE/tacticalrules.h"
#include "GUI/DOCTRINE/doctrineassumptionsnotes.h"
#include "GUI/DOCTRINE/doctrineareadefinition.h"
#include <QMainWindow>                             // For main window base class
#include <QTabWidget>                              // For tabbed interface
#include <QStatusBar>                              // For status bar display
#include <GUI/Menubars/profileinfodialog.h>

// %%% Class Definition %%%
/* Main window class for the database editor */
class MissionEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit MissionEditor(QWidget *parent = nullptr);
    ~MissionEditor();

    QString lastSavedFilePath;
    void markUnsavedChanges();
    void clearUnsavedChanges();
    bool hasUnsavedChanges = false;
    void loadFromJsonFile(const QString &filePath);
    Hierarchy*    hierarchy = nullptr;
    HierarchyTree *treeView = nullptr;
    DoctrineParameters          *doctrinePanel = nullptr;
    TacticalRules               *tacticalPanel = nullptr;
    DoctrineAssumptionsNotes    *assumptionsPanel = nullptr;
    DoctrineAreaDefinition      *areaDefinitionPanel = nullptr;
        void runGUITests();

    // GUI Test helper methods
    bool isHierarchyDockVisible() const;
    bool isDoctrineDockVisible() const;
    bool isTacticalDockVisible() const;
    bool isAssumptionsDockVisible() const;
    bool isAreaDefinitionDockVisible() const;


public slots:
    void showProfileInfo();
    void showApplicationDialog();

private slots:
    void showFeedbackWindow();
    void onDockVisibilityChanged(bool visible);
    void resetLayout();
    void onRecentProjectTriggered();
    void loadRecentProject(const QString& filePath);
    void onTreeItemSelected(QVariantMap data);

signals:
    void unsavedChangesChanged(bool hasChanges);
    void Activated();
    void hierarchyLoaded(QJsonObject hierarchyData);

private:
    // %%% UI Setup %%%
    void setupMenuBar();
    void setupToolBars();
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    void setupEnhancedDockWidgets();
    void setupStatusBar();
    void updateStatusBar(const QString &message);

    // %%% Core Components %%%
    Scenario* scenario = nullptr;
    Console*  console  = nullptr;

    // %%% UI Components %%%
    ConsoleView                 *consoleView   = nullptr;
    // DoctrineParameters          *doctrinePanel = nullptr;
    // TacticalRules               *tacticalPanel = nullptr;
    // DoctrineAssumptionsNotes    *assumptionsPanel = nullptr;
    // DoctrineAreaDefinition      *areaDefinitionPanel = nullptr;

    // %%% Dock Widgets %%%
    QDockWidget *hierarchyDock = nullptr;
    QDockWidget *navigationDock = nullptr;
    QDockWidget *doctrineDock  = nullptr;
    QDockWidget *tacticalDock  = nullptr;
    QDockWidget *assumptionsDock = nullptr;
    QDockWidget *areaDefinitionDock = nullptr;
    QDockWidget *consoleDock   = nullptr;

    QStatusBar *statusBar;
};

#endif
