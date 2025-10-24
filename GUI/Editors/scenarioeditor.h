/* ========================================================================= */
/* File: scenarioeditor.h                                                   */
/* Purpose: Defines the main window for the scenario editor application      */
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
    void markUnsavedChanges();

signals:
    // Signal unsaved changes state
    void unsavedChangesChanged(bool hasChanges);

private:
    // %%% Core Components %%%
    // Script engine instance
    ScriptEngine* scriptengine = nullptr;   // <-- add this
    // Hierarchy tree view widget
    HierarchyTree *treeView;
    // Inspector panel widget
    Inspector *inspector;
    // Console for debugging
    Console *console;
    // Hierarchy data structure
    Hierarchy* hierarchy;
    // Store copied data
    QVariantMap copydata;
    // Store copied hierarchy
    Hierarchy* copyhirarchy = nullptr;

    // %%% UI Components %%%
    // Dock widget for hierarchy
    QDockWidget *hierarchyDock;
    // Dock widget for tactical display
    QDockWidget *tacticalDisplayDock;
    // Dock widget for console
    QDockWidget *consoleDock;
    // Dock widget for inspector
    QDockWidget *inspectorDock;
    // Dock widget for library
    QDockWidget *libraryDock;
    // Dock widget for sidebar
    QDockWidget *sidebarDock;
    // Dock widget for text script
    QDockWidget *textScriptDock;
    // Console view widget
    ConsoleView *consoleView;
    // Tactical display widget (commented)
    // TacticalDisplay *tacticalDisplay;
    // Text script view widget
    TextScriptWidget *textScriptView;

    // %%% UI Setup Methods %%%
    // Configure menu bar
    void setupMenuBar();
    // Configure toolbars
    void setupToolBars();
    // Configure dock widgets
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    // Connect toolbar signals
    void setupToolBarConnections();

    // %%% Toolbar Components %%%
    // Design toolbar
    DesignToolBar *designToolBar;
    // Standard toolbar
    StandardToolBar *standardToolBar;
    // Menu bar
    MenuBar *menuBar;

    // %%% Inspector Management %%%
    // List of inspector docks
    QList<QDockWidget*> inspectorDocks;
    // Count inspector instances
    int inspectorCount = 0;
    // List of inspectors
    QList<Inspector*> inspectors;

    // %%% Status Bar %%%
    // Configure status bar
    void setupStatusBar();
    // Update status bar message
    void updateStatusBar(const QString &message);
    // Status bar widget
    QStatusBar *statusBar;

};

#endif // SCENARIOEDITOR_H
