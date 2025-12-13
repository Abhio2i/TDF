
/* ========================================================================= */
/* File: databaseeditor.h                                                   */
/* Purpose: Defines the main window for the database editor application      */
/* ========================================================================= */

#ifndef DATABASEEDITOR_H
#define DATABASEEDITOR_H

#include "GUI/Hierarchytree/hierarchyconnector.h"  // For hierarchy connections
#include "GUI/Hierarchytree/hierarchytree.h"      // For hierarchy tree view
#include "GUI/Console/consoleview.h"               // For console view
#include "GUI/Inspector/inspector.h"               // For inspector panel
#include "core/Debug/console.h"                    // For console debugging
#include "core/structure/scenario.h"              // For scenario data structure
#include "qdockwidget.h"                          // For dock widget functionality
#include <QMainWindow>                            // For main window base class
#include <QTabWidget>                             // For tabbed interface
#include <QStatusBar>                             // For status bar display
#include <GUI/Menubars/profileinfodialog.h>

// %%% Class Definition %%%
/* Main window class for the database editor */
class DatabaseEditor : public QMainWindow
{
    Q_OBJECT

public:
    // Initialize database editor
    explicit DatabaseEditor(QWidget *parent = nullptr);
    // Clean up resources
    ~DatabaseEditor();
    // Track last saved file path
    QString lastSavedFilePath;
    // Mark unsaved changes
    void markUnsavedChanges();
    // Clear unsaved changes flag
    void clearUnsavedChanges();
    // Track unsaved changes
    bool hasUnsavedChanges = false;
        void loadFromJsonFile(const QString &filePath);
public slots:
    void showProfileInfo();
    void showApplicationDialog();
    // void onPerformanceClicked();
    // void onSensorsClicked();

private slots:
    // Add inspector tab
    void addInspectorTab();
    // Show feedback window
    void showFeedbackWindow();
    // Handle dock visibility changes
    void onDockVisibilityChanged(bool visible);

     void resetLayout();

    // NEW METHODS ADD HERE

    void onRecentProjectTriggered();
    // void clearRecentProjects();
    void loadRecentProject(const QString& filePath);

signals:
    // Signal unsaved changes state
    void unsavedChangesChanged(bool hasChanges);
    void Activated();

private:

    // %%% UI Setup Methods %%%
    // Configure menu bar
    void setupMenuBar();
    // Configure toolbars
    void setupToolBars();
    // Configure dock widgets
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    // Enhanced dock widget setup for Linux compatibility
    void setupEnhancedDockWidgets();

    // %%% Core Components %%%
    // Scenario data structure
    Scenario* scenario = nullptr;
    // Hierarchy data structure
    Hierarchy* hierarchy = nullptr;
    // Console for debugging
    Console* console = nullptr;

    // %%% UI Components %%%
    // Hierarchy tree view widget
    HierarchyTree *treeView = nullptr;
    // Inspector panel widget
    Inspector *inspector = nullptr;
    // Console view widget
    ConsoleView *consoleView = nullptr;

    // %%% Dock Widgets %%%
    // Dock widget for hierarchy
    QDockWidget *hierarchyDock = nullptr;
    // Dock widget for navigation
    QDockWidget *navigationDock = nullptr;
    // Dock widget for inspector
    QDockWidget *inspectorDock = nullptr;
    // Dock widget for console
    QDockWidget *consoleDock = nullptr;

    // %%% Inspector Management %%%
    // List of inspector docks
    QList<QDockWidget*> inspectorDocks;
    // List of inspectors
    QList<Inspector*> inspectors;
    // Count inspector instances
    int inspectorCount = 0;
    // Configure status bar
    void setupStatusBar();
    // Update status bar message
    void updateStatusBar(const QString &message);
    // Status bar widget
    QStatusBar *statusBar;
};

#endif // DATABASEEDITOR_H
