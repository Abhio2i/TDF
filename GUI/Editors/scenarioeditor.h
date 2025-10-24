
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
#include <QTabWidget>
#include <QStatusBar>
#include "core/ScriptEngine/scriptengine.h"

class ScenarioEditor : public QMainWindow
{
    Q_OBJECT
public:
    explicit ScenarioEditor(QWidget *parent = nullptr);
    ~ScenarioEditor();
    QString lastSavedFilePath;
    Hierarchy* library;
    HierarchyTree* libTreeView;
    CanvasWidget* canvas;
    void loadFromJsonFile(const QString &filePath);
    bool hasUnsavedChanges = false;
    void clearUnsavedChanges();
    TacticalDisplay *tacticalDisplay;
private slots:
    void onItemSelected(QVariantMap data);
    void onLibraryItemSelected(QVariantMap data);
    void addInspectorTab();
    void showFeedbackWindow();
    void markUnsavedChanges();

signals:
    void unsavedChangesChanged(bool hasChanges);
private:
    ScriptEngine* scriptengine = nullptr;   // <-- add this
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
    // TacticalDisplay *tacticalDisplay;
    TextScriptWidget *textScriptView;
    HierarchyConnector* m_hierarchyConnector;
    Hierarchy* hierarchy;
    QVariantMap copydata;
    Hierarchy* copyhirarchy = nullptr;
    DesignToolBar *designToolBar;
    StandardToolBar *standardToolBar;
    MenuBar *menuBar;
    QList<QDockWidget*> inspectorDocks;
    int inspectorCount = 0;
    QList<Inspector*> inspectors;
    void setupStatusBar();
    void updateStatusBar(const QString &message);
    QStatusBar *statusBar;

};
#endif // SCENARIOEDITOR_H
