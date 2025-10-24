/* ========================================================================= */
/* File: runtimeeditor.h                                                    */
/* Purpose: Defines the main window for the runtime editor application       */
/* ========================================================================= */

#ifndef RUNTIMEEDITOR_H
#define RUNTIMEEDITOR_H

#include "GUI/Console/consoleview.h"               // For console view
#include "GUI/Hierarchytree/hierarchyconnector.h"  // For hierarchy connections
#include "GUI/Hierarchytree/hierarchytree.h"      // For hierarchy tree view
#include "GUI/Inspector/inspector.h"               // For inspector panel
#include "GUI/Tacticaldisplay/canvaswidget.h"      // For canvas widget
#include "GUI/Tacticaldisplay/tacticaldisplay.h"   // For tactical display
#include "GUI/Toolbars/standardtoolbar.h"          // For standard toolbar
#include "GUI/Toolbars/designtoolbar.h"           // For design toolbar
#include "GUI/Toolbars/runtimetoolbar.h"          // For runtime toolbar
#include "GUI/Toolbars/networktoolbar.h"           // For network toolbar
#include "core/Debug/console.h"                    // For console debugging
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include "core/structure/runtime.h"               // For runtime data structure
#include <QMainWindow>                             // For main window base class
#include <QDockWidget>                             // For dock widget functionality
#include "GUI/Menubars/menubar.h"                 // For menu bar
#include "GUI/scene3dwidget/scene3dwidget.h"      // For 3D scene widget
#include "GUI/Testscript/textscriptwidget.h"      // For text script widget
#include "GUI/Timing/graphwidgettime.h"           // For timing graph widget
#include "GUI/Panel/radardisplay.h"               // For radar display
#include "GUI/Panel/ewdisplay.h"                  // For EW display
#include "GUI/Logger/loggerdialog.h"              // For logger dialog
#include <QTabWidget>                             // For tabbed interface
#include <QStatusBar>                             // For status bar display

// %%% Class Definition %%%
/* Main window class for the runtime editor */
class RuntimeEditor : public QMainWindow
{
    Q_OBJECT

public:
    // Initialize runtime editor
    explicit RuntimeEditor(QWidget *parent = nullptr);
    // Clean up resources
    ~RuntimeEditor();
    // Library hierarchy data
    Hierarchy* library;
    // Library tree view widget
    HierarchyTree* libTreeView;
    // Canvas widget for display
    CanvasWidget* canvas;
    // Load data from JSON file
    void loadFromJsonFile(const QString &filePath);
    // Track last saved file path
    QString lastSavedFilePath;
    // Get timing data as JSON
    QString getTimingJsonData() const;
    // Track unsaved changes
    bool hasUnsavedChanges = false;
    // Clear unsaved changes flag
    void clearUnsavedChanges();

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
    // Toggle radar display
    void toggleRadarDisplay();
    // Toggle logger display
    void toggleLoggerDisplay(bool checked);

signals:
    // Signal unsaved changes state
    void unsavedChangesChanged(bool hasChanges);

private:
    // %%% UI Setup Methods %%%
    // Configure menu bar
    void setupMenuBar();
    // Configure toolbars
    void setupToolBars();
    // Configure dock widgets
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    // Connect toolbar signals
    void setupToolBarConnections();

    // %%% UI Components %%%
    // Hierarchy tree view widget
    HierarchyTree *treeView;
    // Inspector panel widget
    Inspector *inspector;
    // Console for debugging
    Console *console;
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
    // Dock widget for display
    QDockWidget *displayDock;
    // Tactical display widget
    TacticalDisplay *tacticalDisplay;
    // 3D scene widget
    Scene3DWidget *scene3dwidget;
    // Text script view widget
    TextScriptWidget *textScriptView;
    // Timing graph widget
    GraphWidgetTime *timingGraphWidget;
    // Hierarchy connector
    HierarchyConnector* m_hierarchyConnector;
    // Hierarchy data structure
    Hierarchy* hierarchy;
    // Store copied data
    QVariantMap copydata;
    // Store copied hierarchy
    Hierarchy* copyhirarchy = nullptr;
    // Design toolbar
    DesignToolBar *designToolBar;
    // Runtime toolbar
    RuntimeToolBar *runtimeToolBar;
    // Network toolbar
    NetworkToolbar *networkToolBar;
    // Standard toolbar
    StandardToolBar *standardToolBar;
    // Menu bar
    MenuBar *menuBar;
    // List of inspector docks
    QList<QDockWidget*> inspectorDocks;
    // Count inspector instances
    int inspectorCount = 0;
    // List of inspectors
    QList<Inspector*> inspectors;
    // Runtime data structure
    Runtime *runtime;
    // Radar display UI
    RadarDisplay *radarDisplayUI;
    // Simulation instance
    Simulation *simulation;
    // Configure status bar
    void setupStatusBar();
    // Update status bar message
    void updateStatusBar(const QString &message);
    // Status bar widget
    QStatusBar *statusBar;
    // Display window widget
    QWidget *displayWindow;
    // Tab widget for displays
    QTabWidget *displayTabs;
    // EW display UI
    EWDisplay *ewDisplayUI;
    // Dock widget for logger dialog
    QDockWidget *loggerDock;
    // Logger dialog instance
    LoggerDialog *loggerDialog;
    // Store recording start time
    QDateTime recordingStartTime;
    // Timer for updating recording duration
    QTimer *recordingTimer;
};

#endif // RUNTIMEEDITOR_H
