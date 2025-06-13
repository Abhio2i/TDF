
// #ifndef ScenarioEditor_H
// #define ScenarioEditor_H
// #include "GUI/Console/consoleview.h"
// #include "GUI/Hierarchytree/hierarchyconnector.h"
// #include "GUI/Hierarchytree/hierarchytree.h"
// #include "GUI/Inspector/inspector.h"
// #include "GUI/Tacticaldisplay/canvaswidget.h"
// #include "GUI/Tacticaldisplay/tacticaldisplay.h"
// #include "GUI/Toolbars/designtoolbar.h"
// #include "core/Debug/console.h"
// #include "core/structure/hierarchy.h"
// #include "qdockwidget.h"
// #include <QMainWindow>
// #include "GUI/scene3dwidget/scene3dwidget.h"

// class ScenarioEditor : public QMainWindow
// {
//     Q_OBJECT

// public:
//     explicit ScenarioEditor(QWidget *parent = nullptr);
//     ~ScenarioEditor();
//     Hierarchy* library;
//     HierarchyTree* libTreeView;
//     CanvasWidget* canvas;


// private slots:
//     void onItemSelected(QVariantMap data);
//     void onLibraryItemSelected(QVariantMap data);
//     void addInspectorTab();

//     void showFeedbackWindow();

// private:
//     void setupMenuBar();
//     void setupToolBars();
//     void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);

//     // Hierarchy components
//     HierarchyTree *treeView;
//     Inspector *inspector;
//     Console *console;

//     // Dock widgets
//     QDockWidget *hierarchyDock;
//     QDockWidget *navigationDock;
//     QDockWidget *tacticalDisplayDock;
//     QDockWidget *consoleDock;
//     QDockWidget *inspectorDock;
//     QDockWidget *libraryDock;
//     QDockWidget *overviewDock;
//     QDockWidget *sidebarDock;
//     QDockWidget *currentBottomDock;

//     // Views
//     ConsoleView *consoleView;
//     TacticalDisplay *tacticalDisplay;

//     HierarchyConnector* m_hierarchyConnector;
//     Hierarchy* hierarchy;
//     QVariantMap copydata;
//     Hierarchy* copyhirarchy = nullptr;

//     DesignToolBar *designToolBar;
//     void setupToolBarConnections();
//     Scene3DWidget *scene3dwidget;
//     QList<QDockWidget*> inspectorDocks;
//     int inspectorCount = 0;
//     QList<QMetaObject::Connection> inspectorConnections;
//     QList<Inspector*> inspectors;


// };

// #endif


#ifndef ScenarioEditor_H
#define ScenarioEditor_H

#include "GUI/Console/consoleview.h"
#include "GUI/Hierarchytree/hierarchyconnector.h"
#include "GUI/Hierarchytree/hierarchytree.h"
#include "GUI/Inspector/inspector.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "GUI/Tacticaldisplay/tacticaldisplay.h"
#include "GUI/Toolbars/designtoolbar.h"
#include "core/Debug/console.h"
#include "core/Hierarchy/hierarchy.h"
#include "qdockwidget.h"
#include <QMainWindow>
#include "GUI/scene3dwidget/scene3dwidget.h"

// Main window class for Scenario Editor
class ScenarioEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit ScenarioEditor(QWidget *parent = nullptr);
    ~ScenarioEditor();

    // Public components
    Hierarchy* library;
    HierarchyTree* libTreeView;
    CanvasWidget* canvas;

private slots:
    void onItemSelected(QVariantMap data);
    void onLibraryItemSelected(QVariantMap data);
    void addInspectorTab();
    void showFeedbackWindow();

private:
    // UI setup functions
    void setupMenuBar();
    void setupToolBars();
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);

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
    Scene3DWidget *scene3dwidget;

    // Hierarchy and connectors
    HierarchyConnector* m_hierarchyConnector;
    Hierarchy* hierarchy;
    QVariantMap copydata;
    Hierarchy* copyhirarchy = nullptr;

    // Toolbars
    DesignToolBar *designToolBar;
    void setupToolBarConnections();

    // Inspector management
    QList<QDockWidget*> inspectorDocks;
    int inspectorCount = 0;
    QList<QMetaObject::Connection> inspectorConnections;
    QList<Inspector*> inspectors;
};

#endif
