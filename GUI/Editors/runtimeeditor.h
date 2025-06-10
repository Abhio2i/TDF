
#ifndef RUNTIMEEDITOR_H
#define RUNTIMEEDITOR_H

#include "GUI/Console/consoleview.h"
#include "GUI/Hierarchytree/hierarchyconnector.h"
#include "GUI/Hierarchytree/hierarchytree.h"
#include "GUI/Inspector/inspector.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "GUI/Tacticaldisplay/tacticaldisplay.h"
#include "GUI/Toolbars/designtoolbar.h"
#include "GUI/Toolbars/runtimetoolbar.h"
#include "GUI/Toolbars/networktoolbar.h"
#include "core/Debug/console.h"
#include "core/structure/hierarchy.h"
#include <QMainWindow>
#include <QDockWidget>

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

    // Hierarchy components
    HierarchyTree *treeView;
    Inspector *inspector;
    Console *console;

    // Dock widgets
    QDockWidget *hierarchyDock;
    QDockWidget *navigationDock;
    QDockWidget *tacticalDisplayDock;
    QDockWidget *consoleDock;
    QDockWidget *inspectorDock;
    QDockWidget *libraryDock;
    QDockWidget *overviewDock;
    QDockWidget *sidebarDock;
    QDockWidget *currentBottomDock;

    // Views
    ConsoleView *consoleView;
    TacticalDisplay *tacticalDisplay;

    HierarchyConnector* m_hierarchyConnector;
    Hierarchy* hierarchy;
    QVariantMap copydata;
    Hierarchy* copyhirarchy = nullptr;

    // Toolbars
    DesignToolBar *designToolBar;
    RuntimeToolBar *runtimeToolBar;
    NetworkToolbar *networkToolBar; // Added for NetworkToolbar

    QList<QDockWidget*> inspectorDocks; // To keep track of all inspector docks
    int inspectorCount = 0;
    QList<QMetaObject::Connection> inspectorConnections;
    QList<Inspector*> inspectors;
};

#endif // RUNTIMEEDITOR_H
