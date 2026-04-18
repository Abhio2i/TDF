

/* ========================================================================= */
/* File: scenarioeditor.h                                                   */
/* Purpose: Defines the main window for the scenario editor application      */
// Written by   : Arti Rajpoot
/* ========================================================================= */

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
     // static void runUnitTestsOnce();
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

    // CustomResizableOverlayDock *hierarchyDock;
    // CustomResizableOverlayDock *tacticalDisplayDock;
    // CustomResizableOverlayDock *consoleDock;
    // CustomResizableOverlayDock *inspectorDock;
    // CustomResizableOverlayDock *libraryDock;
    // CustomResizableOverlayDock *sidebarDock;
    // CustomResizableOverlayDock *textScriptDock;
    // Script engine instance
    ScriptEngine* scriptengine = nullptr;
    // Inspector panel widget
    Inspector *inspector;
    QVariantMap copydata;
    // Store copied hierarchy
    Hierarchy* copyhirarchy = nullptr;
    // LayerPanel *layerPanel = nullptr;
    // CustomResizableOverlayDock *layerDock = nullptr;
    // ConsoleView *consoleView;
    // Text script view widget
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
    // DesignToolBar *designToolBar;
    QList<QDockWidget*> inspectorDocks;
    // Count inspector instances
    int inspectorCount = 0;
    // List of inspectors
    QList<Inspector*> inspectors;
    // Update status bar message
    // Status bar widget
    QStatusBar *statusBar;
    Simulation *simulation;
    ScenarioConfig* m_scenarioConfig;
     void showPanelContextMenu(const QPoint &pos);
private slots:
    void onRunScriptFileRequested(const QString& filePath);

};

#endif // SCENARIOEDITOR_H
