
#ifndef DATABASEEDITOR_H
#define DATABASEEDITOR_H

#include "GUI/Hierarchytree/hierarchyconnector.h"
#include "GUI/Hierarchytree/hierarchytree.h"
#include "GUI/Console/consoleview.h"
#include "GUI/Inspector/inspector.h"
#include "core/Debug/console.h"
#include "core/structure/scenario.h"
#include "qdockwidget.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QStatusBar>

class DatabaseEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit DatabaseEditor(QWidget *parent = nullptr);
    ~DatabaseEditor();
    QString lastSavedFilePath;
    void markUnsavedChanges();
    void clearUnsavedChanges();
    bool hasUnsavedChanges = false;
private slots:
    void addInspectorTab();
    void showFeedbackWindow();

signals:
    void unsavedChangesChanged(bool hasChanges);

private:
    void setupMenuBar();
    void setupToolBars();
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);

    // Core components
    Scenario* scenario = nullptr;
    Hierarchy* hierarchy = nullptr;
    Console* console = nullptr;

    // UI components
    HierarchyTree *treeView = nullptr;
    Inspector *inspector = nullptr;
    ConsoleView *consoleView = nullptr;

    // Dock widgets
    QDockWidget *hierarchyDock = nullptr;
    QDockWidget *navigationDock = nullptr;
    QDockWidget *inspectorDock = nullptr;
    QDockWidget *consoleDock = nullptr;

    // Inspector management
    QList<QDockWidget*> inspectorDocks;
    QList<Inspector*> inspectors;
    int inspectorCount = 0;
    void setupStatusBar();
    void updateStatusBar(const QString &message);
    QStatusBar *statusBar;
};

#endif // DATABASEEDITOR_H
