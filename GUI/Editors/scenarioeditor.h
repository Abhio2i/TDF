

/* =============================================================================
 * FILE:         scenarioeditor.h
 * MODULE:       Scenario Editor Main Window
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the ScenarioEditor class which serves as the main
 *               window for scenario creation and editing. It manages hierarchy
 *               tree views (mission and library), tactical display (2D canvas),
 *               inspector panels, console view, toolbars (standard, design),
 *               menu bar, text script widget, layer panel, and dock widgets.
 *               Supports loading/saving JSON scenario files, tracking unsaved
 *               changes, resetting layout, recent projects, script execution,
 *               and displaying profile/application information.
 *
 * REQUIREMENTS: REQ-SCENARIO-010  Main window with menu bar and toolbars
 *               REQ-SCENARIO-011  Hierarchy tree view (mission) dock widget
 *               REQ-SCENARIO-012  Library tree view dock widget
 *               REQ-SCENARIO-013  Tactical display (2D canvas) dock widget
 *               REQ-SCENARIO-014  Inspector panel dock widget (multiple tabs)
 *               REQ-SCENARIO-015  Console view dock widget
 *               REQ-SCENARIO-016  Text script widget dock widget
 *               REQ-SCENARIO-017  Layer panel dock widget
 *               REQ-SCENARIO-018  Load scenario from JSON file
 *               REQ-SCENARIO-019  Track unsaved changes
 *               REQ-SCENARIO-020  Reset layout to default
 *               REQ-SCENARIO-021  Recent projects list
 *               REQ-SCENARIO-022  Show profile info / application dialog
 *               REQ-SCENARIO-023  Show feedback window
 *               REQ-SCENARIO-024  Script engine integration
 *               REQ-SCENARIO-025  Simulation control
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-SCENARIO-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef SCENARIOEDITOR_H
#define SCENARIOEDITOR_H

#include "GUI/Console/consoleview.h"               // For console view
#include "GUI/Hierarchytree/hierarchyconnector.h"  // For hierarchy connections
#include "GUI/Hierarchytree/hierarchytree.h"      // For hierarchy tree view
#include "GUI/Inspector/inspector.h"               // For inspector panel
#include "GUI/Tacticaldisplay/canvaswidget.h"      // For canvas widget
#include "GUI/Tacticaldisplay/tacticaldisplay.h"   // For tactical display
#include "GUI/Toolbars/designtoolbar.h"           // For design toolbar
#include "GUI/Toolbars/standardtoolbar.h"          // For standard toolbar
#include "core/Debug/console.h"                    // For console debugging
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include <QMainWindow>                             // For main window base class
#include "GUI/Menubars/menubar.h"                 // For menu bar
#include "GUI/Testscript/textscriptwidget.h"      // For text script widget
#include <QTabWidget>                             // For tabbed interface
#include <QStatusBar>                             // For status bar display
#include "core/ScriptEngine/scriptengine.h"       // For script engine
#include "core/Simulation/simulation.h"
#include <core/Config/scenarioconfig.h>
#include <GUI/Tacticaldisplay/Gis/layerpanel.h>
#include "GUI/Editors/customresizableoverlaydock.h"

// %%% Class Definition %%%
/* Main window class for the scenario editor */
class ScenarioEditor : public QMainWindow
{
    Q_OBJECT
public:
    // Initialize scenario editor
    explicit ScenarioEditor(QWidget *parent = nullptr);
    // Clean up resources
    ~ScenarioEditor();
    // Track last saved file path
    QString lastSavedFilePath;
    // Library hierarchy data
    Hierarchy* library;
    // Hierarchy tree view widget
    HierarchyTree *treeView;
    // Library tree view widget
    HierarchyTree* libTreeView;
    // Canvas widget for display
    CanvasWidget* canvas;
    // Load data from JSON file
    void loadFromJsonFile(const QString &filePath);
    // Track unsaved changes
    bool hasUnsavedChanges = false;
    // Clear unsaved changes flag
    void clearUnsavedChanges();
    // Tactical display widget
    TacticalDisplay *tacticalDisplay;
    Hierarchy* hierarchy;
     Console *console;
     DesignToolBar *designToolBar;
     ConsoleView *consoleView;
  LayerPanel *layerPanel = nullptr;
     CustomResizableOverlayDock *hierarchyDock;
     CustomResizableOverlayDock *tacticalDisplayDock;
     CustomResizableOverlayDock *consoleDock;
     CustomResizableOverlayDock *inspectorDock;
     CustomResizableOverlayDock *libraryDock;
     CustomResizableOverlayDock *sidebarDock;
     CustomResizableOverlayDock *textScriptDock;
     CustomResizableOverlayDock *layerDock = nullptr;

public slots:
    void showProfileInfo();
    void showApplicationDialog();
    void markUnsavedChanges();
protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
private slots:
    // Handle item selection
    void onItemSelected(QVariantMap data);
    // Handle library item selection
    void onLibraryItemSelected(QVariantMap data);
    // Add inspector tab
    void addInspectorTab();
    // Show feedback window
    void showFeedbackWindow();
    // Mark unsaved changes
    // Handle dock visibility changes
    void onDockVisibilityChanged(bool visible);
    // Reset layout to initial state
    void resetLayout();
    void onRecentProjectTriggered();
    void loadRecentProject(const QString& filePath);

signals:
    // Signal unsaved changes state
    void unsavedChangesChanged(bool hasChanges);
    void Activated();
private:
    // %%% Core Components %%%
    ScriptEngine* scriptengine = nullptr;
    // Inspector panel widget
    Inspector *inspector;
    QVariantMap copydata;
    // Store copied hierarchy
    Hierarchy* copyhirarchy = nullptr;
    TextScriptWidget *textScriptView;
    // %%% UI Setup Methods %%%
    // Configure menu bar
    void setupMenuBar();
    // Configure toolbars
    void setupToolBars();
    // Configure dock widgets
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    void setupEnhancedDockWidgets();
    // Connect toolbar signals
    void setupToolBarConnections();
    // %%% Toolbar Components %%%
    // Design toolbar
    QList<QDockWidget*> inspectorDocks;
    // Count inspector instances
    int inspectorCount = 0;
    // List of inspectors
    QList<Inspector*> inspectors;
    QStatusBar *statusBar;
    Simulation *simulation;
    ScenarioConfig* m_scenarioConfig;
    void showPanelContextMenu(const QPoint &pos);
    QToolBar *rightToolBar = nullptr;
private slots:
    void onRunScriptFileRequested(const QString& filePath);

};

#endif // SCENARIOEDITOR_H
