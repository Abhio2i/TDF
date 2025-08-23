
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

class RuntimeEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit RuntimeEditor(QWidget *parent = nullptr);
    ~RuntimeEditor();
    Hierarchy* library;
    HierarchyTree* libTreeView;
    CanvasWidget* canvas;

private slots:
    void onItemSelected(QVariantMap data);
    void onLibraryItemSelected(QVariantMap data);
    void addInspectorTab();
    void showFeedbackWindow();

private:
    void setupMenuBar();
    void setupToolBars();
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    void setupToolBarConnections();

    // Core Hierarchy components
    HierarchyTree *treeView;
    Inspector *inspector;
    Console *console;

    // Dock widgets
    QDockWidget *hierarchyDock;
    QDockWidget *tacticalDisplayDock;
    QDockWidget *consoleDock;
    QDockWidget *inspectorDock;
    QDockWidget *libraryDock;
    QDockWidget *sidebarDock;
    QDockWidget *textScriptDock;

    // Views
    ConsoleView *consoleView;
    TacticalDisplay *tacticalDisplay;
    Scene3DWidget *scene3dwidget;
      TextScriptWidget *textScriptView;

    // Hierarchy and connectors
    HierarchyConnector* m_hierarchyConnector;
    Hierarchy* hierarchy;
    QVariantMap copydata;
    Hierarchy* copyhirarchy = nullptr;

    // Toolbars
    DesignToolBar *designToolBar;
    RuntimeToolBar *runtimeToolBar;
    NetworkToolbar *networkToolBar;
    StandardToolBar *standardToolBar;
    MenuBar *menuBar;

    // Inspector management
    QList<QDockWidget*> inspectorDocks;
    int inspectorCount = 0;
    QList<Inspector*> inspectors;

    Runtime *runtime;
};

#endif // RUNTIMEEDITOR_H
