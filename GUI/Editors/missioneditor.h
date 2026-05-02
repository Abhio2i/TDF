/* =============================================================================
 * FILE:         MissionEditor.h
 * MODULE:       Mission Editor Main Window
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the MissionEditor class which serves as the main
 *               window for mission editing. It manages hierarchy tree view,
 *               doctrine parameters panel, tactical rules panel, assumptions
 *               notes panel, area definition panel, console view, menu bar,
 *               tool bars, dock widgets, status bar, and scenario data.
 *               Supports loading/saving JSON files, tracking unsaved changes,
 *               resetting layout, recent projects, and displaying profile/
 *               application information.
 *
 * REQUIREMENTS: REQ-MISSION-010  Main window with menu bar and tool bars
 *               REQ-MISSION-011  Hierarchy tree view dock widget
 *               REQ-MISSION-012  Doctrine parameters panel dock widget
 *               REQ-MISSION-013  Tactical rules panel dock widget
 *               REQ-MISSION-014  Doctrine assumptions/notes panel dock widget
 *               REQ-MISSION-015  Doctrine area definition panel dock widget
 *               REQ-MISSION-016  Console view dock widget
 *               REQ-MISSION-017  Load mission from JSON file
 *               REQ-MISSION-018  Track unsaved changes
 *               REQ-MISSION-019  Reset layout to default
 *               REQ-MISSION-020  Recent projects list
 *               REQ-MISSION-021  Show profile info dialog
 *               REQ-MISSION-022  Show application dialog
 *               REQ-MISSION-023  Show feedback window
 *               REQ-MISSION-024  Hierarchy loaded signal
 *               REQ-MISSION-025  Unsaved changes changed signal
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-MISSION-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

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
