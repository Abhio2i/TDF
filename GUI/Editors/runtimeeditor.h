
#ifndef RUNTIMEEDITOR_H
#define RUNTIMEEDITOR_H
#include "GUI/Console/consoleview.h"
#include "GUI/Hierarchytree/hierarchyconnector.h"
#include "GUI/Hierarchytree/hierarchytree.h"
#include "GUI/Inspector/inspector.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "GUI/Tacticaldisplay/tacticaldisplay.h"
#include "GUI/Toolbars/standardtoolbar.h"
#include "GUI/Toolbars/designtoolbar.h"
#include "GUI/Toolbars/runtimetoolbar.h"
#include "GUI/Toolbars/networktoolbar.h"
#include "core/Debug/console.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/structure/runtime.h"
#include <QMainWindow>
#include <QDockWidget>
#include "GUI/Menubars/menubar.h"
#include "GUI/scene3dwidget/scene3dwidget.h"
#include "GUI/Testscript/textscriptwidget.h"
#include "GUI/Timing/graphwidgettime.h"
#include "GUI/Panel/radardisplay.h"
#include "GUI/Panel/ewdisplay.h"
#include "GUI/Logger/loggerdialog.h" // ADDED: Include LoggerDialog
#include <QTabWidget>
#include <QStatusBar>

class RuntimeEditor : public QMainWindow
{
    Q_OBJECT
public:
    explicit RuntimeEditor(QWidget *parent = nullptr);
    ~RuntimeEditor();
    Hierarchy* library;
    HierarchyTree* libTreeView;
    CanvasWidget* canvas;
    void loadFromJsonFile(const QString &filePath);
    QString lastSavedFilePath;
    QString getTimingJsonData() const;
        bool hasUnsavedChanges = false;

          void clearUnsavedChanges();
private slots:
    void onItemSelected(QVariantMap data);
    void onLibraryItemSelected(QVariantMap data);
    void addInspectorTab();
    void showFeedbackWindow();
    void markUnsavedChanges();
    void toggleRadarDisplay();
void toggleLoggerDisplay(bool checked); // ADDED___Logger

signals:
    void unsavedChangesChanged(bool hasChanges);
private:
    void setupMenuBar();
    void setupToolBars();
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    void setupToolBarConnections();
    HierarchyTree *treeView;
    Inspector *inspector;
    Console *console;
    QDockWidget *hierarchyDock;
    QDockWidget *tacticalDisplayDock;
    QDockWidget *consoleDock;
    QDockWidget *inspectorDock;
    QDockWidget *libraryDock;
    QDockWidget *sidebarDock;
    QDockWidget *textScriptDock;
    ConsoleView *consoleView;
    QDockWidget *displayDock;
    TacticalDisplay *tacticalDisplay;
    Scene3DWidget *scene3dwidget;
    TextScriptWidget *textScriptView;
    GraphWidgetTime *timingGraphWidget;
    HierarchyConnector* m_hierarchyConnector;
    Hierarchy* hierarchy;
    QVariantMap copydata;
    Hierarchy* copyhirarchy = nullptr;
    DesignToolBar *designToolBar;
    RuntimeToolBar *runtimeToolBar;
    NetworkToolbar *networkToolBar;
    StandardToolBar *standardToolBar;
    MenuBar *menuBar;
    QList<QDockWidget*> inspectorDocks;
    int inspectorCount = 0;
    QList<Inspector*> inspectors;
    Runtime *runtime;
    RadarDisplay *radarDisplayUI;
    Simulation *simulation;
    void setupStatusBar();
    void updateStatusBar(const QString &message);
    QStatusBar *statusBar;

    QWidget *displayWindow;
    QTabWidget *displayTabs;

    EWDisplay *ewDisplayUI;
QDockWidget *loggerDock; // ADDED: Dock widget for LoggerDialog
LoggerDialog *loggerDialog; // ADDED: LoggerDialog instance
QDateTime recordingStartTime; // Added for recording timestamp
QTimer *recordingTimer;       // Added for updating recording duration

};
#endif // RUNTIMEEDITOR_H
