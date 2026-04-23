
/* =============================================================================
 * FILE:         runtimeeditor.h
 * MODULE:       Runtime Editor Main Window
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the RuntimeEditor class which serves as the main
 *               window for runtime simulation and editing. It manages hierarchy
 *               tree views (mission and library), tactical display (2D canvas),
 *               3D scene widget, inspector panels, console view, various sensor
 *               displays (radar, IFF, radio, ESM, CSM, EO, AIS, ADSB, sonar,
 *               AESA radar), logging, script execution, toolbars (standard,
 *               design, runtime, network), menu bar, and dock widgets.
 *               Supports loading JSON mission files, tracking unsaved changes,
 *               resetting layout, recent projects, and displaying profile/
 *               application information.
 *
 * REQUIREMENTS: REQ-RUNTIME-010  Main window with menu bar and toolbars
 *               REQ-RUNTIME-011  Hierarchy tree view (mission) dock widget
 *               REQ-RUNTIME-012  Library tree view dock widget
 *               REQ-RUNTIME-013  Tactical display (2D canvas) dock widget
 *               REQ-RUNTIME-014  Scene3D widget dock widget
 *               REQ-RUNTIME-015  Inspector panel dock widget (multiple tabs)
 *               REQ-RUNTIME-016  Console view dock widget
 *               REQ-RUNTIME-017  Text script widget dock widget
 *               REQ-RUNTIME-018  Sensor displays (radar, IFF, radio, etc.)
 *               REQ-RUNTIME-019  Logger dialog for data recording
 *               REQ-RUNTIME-020  Load mission from JSON file
 *               REQ-RUNTIME-021  Track unsaved changes
 *               REQ-RUNTIME-022  Reset layout to default
 *               REQ-RUNTIME-023  Recent projects list
 *               REQ-RUNTIME-024  Show profile info / application dialog
 *               REQ-RUNTIME-025  Show feedback window
 *               REQ-RUNTIME-026  Timing data export as JSON
 *               REQ-RUNTIME-027  Sidebar view and display tab switching
 *               REQ-RUNTIME-028  Duplicate sensors window
 *               REQ-RUNTIME-029  Filter sensor tabs by category
 *               REQ-RUNTIME-030  Canvas selecting mode
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-RUNTIME-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef RUNTIMEEDITOR_H
#define RUNTIMEEDITOR_H

#include "GUI/Console/consoleview.h"               // For console view
#include "GUI/Hierarchytree/hierarchyconnector.h"  // For hierarchy connections
#include "GUI/Hierarchytree/hierarchytree.h"      // For hierarchy tree view
#include "GUI/Inspector/inspector.h"               // For inspector panel
#include "GUI/Panel/adsbdisplay.h"
#include "GUI/Panel/aisdisplay.h"
#include "GUI/Panel/eodisplay.h"
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
#include "core/Hierarchy/Components/transform.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <QElapsedTimer>
#include "GUI/Panel/aesaradardisplay.h"
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
    Console *console;
    RuntimeToolBar *runtimeToolBar;
    DesignToolBar *designToolBar;
    void markUnsavedChanges();
    ConsoleView *consoleView;

public slots:
    void showProfileInfo();
    void showApplicationDialog();
    void createDuplicateSensorsWindow();
    void filterSensorTabsByCategory(const QString &category);


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
    // Toggle radar display
    void toggleRadarDisplay();
    // Toggle logger display
    void toggleLoggerDisplay(bool checked);
    void onRecentProjectTriggered();
    void loadRecentProject(const QString& filePath);
    void setupEnhancedDockWidgets();
    void onDockVisibilityChanged(bool visible);
    void resetLayout();
    void showPanelContextMenu(const QPoint &pos);
    void filterSensorTabsForEntity(const QString &entityId, const QString &category);


signals:
    // Signal unsaved changes state
    void unsavedChangesChanged(bool hasChanges);
    void Activated();


private:
    // %%% UI Setup Methods %%%
    GraphWidget *graphWidget = nullptr;
    ScriptEngine* scriptengine = nullptr;
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
    // Configure dock widgets
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    // Connect toolbar signals
    void setupToolBarConnections();

    // %%% UI Components %%%
    Inspector *inspector;
    Scene3DWidget *scene3dwidget;
    TextScriptWidget *textScriptView;
    HierarchyConnector* m_hierarchyConnector;
    QVariantMap copydata;
    Hierarchy* copyhirarchy = nullptr;
    NetworkToolbar *networkToolBar;
    LayerPanel *layerPanel = nullptr;
    MenuBar *menuBar;
    QList<QDockWidget*> inspectorDocks;
    int inspectorCount = 0;
    QList<Inspector*> inspectors;
    Runtime *runtime;
    RadarDisplay *radarDisplayUI;
    AESARadarDisplay* aesaRadarDisplayUI;
    void setupStatusBar();
    void updateStatusBar(const QString &message);
    QStatusBar *statusBar;
    QWidget *displayWindow;
    QTabWidget *displayTabs;
    IFFDisplay *iffDisplayUI;
    RADIODisplay *radioDisplayUI;
    ESMDisplay *esmDisplayUI;
    CSMDisplay *csmDisplayUI;
    EODisplay *eoDisplayUI;
    AISDisplay *aisDisplayUI;
    ADSBDisplay *adsbDisplayUI;
    SonarDisplay *sonarDisplayUI;  //  by amjad
    LoggerDialog *loggerDialog;
    QDateTime recordingStartTime;
    QTimer *recordingTimer = nullptr;
    ScenarioConfig* m_scenarioConfig;
    bool m_canvasSelecting = false;
    ActiveSonar   m_activeSonar; // by amjad
    QElapsedTimer m_sonarTimer;
    Entity* m_selectedSonarEntity = nullptr;
    QString m_lastSelectedEntityId;

private:
    qint64 pausedTimeMs = 0;
private slots:
    void onRunScriptFileRequested(const QString& filePath);
};

#endif // RUNTIMEEDITOR_H
