
// #endif // RUNTIMEEDITOR_H
/* ========================================================================= */
/* File: runtimeeditor.h                                                    */
/* Purpose: Defines the main window for the runtime editor application       */
// Written by   : Arti Rajpoot
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
#include "GUI/Menubars/menubar.h"                 // For menu bar
#include "GUI/scene3dwidget/scene3dwidget.h"      // For 3D scene widget
#include "GUI/Testscript/textscriptwidget.h"      // For text script widget
#include "GUI/Panel/radardisplay.h"               // For radar display
#include "GUI/Panel/iffdisplay.h"
#include "GUI/Panel/radiodisplay.h"
#include "GUI/Panel/csmdisplay.h"
#include "GUI/Panel/esmdisplay.h"
#include "GUI/Panel/sonardisplay.h"              // for sonar display
#include "GUI/Logger/loggerdialog.h"              // For logger dialog
#include <QTabWidget>                             // For tabbed interface
#include <QStatusBar>                              // For status bar display
#include <GUI/Menubars/profileinfodialog.h>
#include <GUI/Tacticaldisplay/Gis/layerpanel.h>
#include "GUI/Editors/customresizableoverlaydock.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar/active_sonar.h" //by amjad
#include "core/Hierarchy/Components/transform.h"   // ← ADD (already ho sakta hai)
#include "core/Hierarchy/EntityProfiles/platform.h" // ← ADD
#include <QElapsedTimer>                            // ← ADD

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
    HierarchyTree *treeView;
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
    Hierarchy* hierarchy;
    Simulation *simulation;
    void triggerSidebarView(const QString &viewName);
    void triggerDisplayTab(const QString &tabName);
    TacticalDisplay *tacticalDisplay;
    static QJsonObject s_missionData;
    static QString     s_missionFilePath;

public slots:
    void showProfileInfo();
    void showApplicationDialog();

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
    void markUnsavedChanges();
    // Toggle radar display
    void toggleRadarDisplay();
    // Toggle logger display
    void toggleLoggerDisplay(bool checked);
    void onRecentProjectTriggered();
    void loadRecentProject(const QString& filePath);
    void setupEnhancedDockWidgets();
    void onDockVisibilityChanged(bool visible);
    void resetLayout();
    // void onRecentLibraryTriggered();
    void showPanelContextMenu(const QPoint &pos);

signals:
    // Signal unsaved changes state
    void unsavedChangesChanged(bool hasChanges);
    void Activated();

private:
    // %%% UI Setup Methods %%%
    GraphWidget *graphWidget = nullptr;
    ScriptEngine* scriptengine = nullptr;

    // CUSTOM RESIZABLE OVERLAY DOCKS
    CustomResizableOverlayDock *hierarchyDock;
    CustomResizableOverlayDock *tacticalDisplayDock;
    CustomResizableOverlayDock *consoleDock;
    CustomResizableOverlayDock *inspectorDock;
    CustomResizableOverlayDock *libraryDock;
    CustomResizableOverlayDock *sidebarDock;
    CustomResizableOverlayDock *textScriptDock;
    CustomResizableOverlayDock *displayDock;
    CustomResizableOverlayDock *loggerDock;
    CustomResizableOverlayDock *layerDock;

    // Configure menu bar
    void setupMenuBar();
    // Configure toolbars
    void setupToolBars();
    // Configure dock widgets (legacy)
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    // Connect toolbar signals
    void setupToolBarConnections();

    // %%% UI Components %%%
    Inspector *inspector;
    Console *console;
    ConsoleView *consoleView;
    Scene3DWidget *scene3dwidget;
    TextScriptWidget *textScriptView;
    HierarchyConnector* m_hierarchyConnector;
    QVariantMap copydata;
    Hierarchy* copyhirarchy = nullptr;
    DesignToolBar *designToolBar;
    RuntimeToolBar *runtimeToolBar;
    NetworkToolbar *networkToolBar;
    LayerPanel *layerPanel = nullptr;
    MenuBar *menuBar;
    QList<QDockWidget*> inspectorDocks;
    int inspectorCount = 0;
    QList<Inspector*> inspectors;
    Runtime *runtime;
    RadarDisplay *radarDisplayUI;
    void setupStatusBar();
    void updateStatusBar(const QString &message);
    QStatusBar *statusBar;
    QWidget *displayWindow;
    QTabWidget *displayTabs;
    IFFDisplay *iffDisplayUI;
    RADIODisplay *radioDisplayUI;
    ESMDisplay *esmDisplayUI;
    CSMDisplay *csmDisplayUI;
    SonarDisplay *sonarDisplayUI;  //  by amjad
    LoggerDialog *loggerDialog;
    QDateTime recordingStartTime;
    QTimer *recordingTimer = nullptr;
    ScenarioConfig* m_scenarioConfig;
    bool m_canvasSelecting = false;

    ActiveSonar   m_activeSonar; // by amjad
    QElapsedTimer m_sonarTimer;

private:
    qint64 pausedTimeMs = 0;

private slots:
    void onRunScriptFileRequested(const QString& filePath);
};

#endif // RUNTIMEEDITOR_H
