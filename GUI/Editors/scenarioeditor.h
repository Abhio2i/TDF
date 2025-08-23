
#ifndef SCENARIOEDITOR_H
#define SCENARIOEDITOR_H

#include "GUI/Console/consoleview.h"
#include "GUI/Hierarchytree/hierarchyconnector.h"
#include "GUI/Hierarchytree/hierarchytree.h"
#include "GUI/Inspector/inspector.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "GUI/Tacticaldisplay/tacticaldisplay.h"
#include "GUI/Toolbars/designtoolbar.h"
#include "GUI/Toolbars/standardtoolbar.h"
#include "core/Debug/console.h"
#include "core/Hierarchy/hierarchy.h"
#include <QMainWindow>
#include "GUI/Menubars/menubar.h"
#include "GUI/Testscript/textscriptwidget.h"

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
    void setupToolBarConnections();

    // Hierarchy components
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
    QDockWidget *textScriptDock; // For TextScript

    // Views
    ConsoleView *consoleView;
    TacticalDisplay *tacticalDisplay;
    TextScriptWidget *textScriptView; // For TextScript

    // Hierarchy and connectors
    HierarchyConnector* m_hierarchyConnector;
    Hierarchy* hierarchy;
    QVariantMap copydata;
    Hierarchy* copyhirarchy = nullptr;

    // Toolbars
    DesignToolBar *designToolBar;
    StandardToolBar *standardToolBar;
    MenuBar *menuBar;

    // Inspector management
    QList<QDockWidget*> inspectorDocks;
    int inspectorCount = 0;
    QList<Inspector*> inspectors;
};

#endif // SCENARIOEDITOR_H
