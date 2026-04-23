
/* =============================================================================
 * FILE:         databaseeditor.h
 * MODULE:       Database Editor Main Window
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the DatabaseEditor class which serves as the main
 *               window for the database editor application. It manages the
 *               hierarchy tree view, inspector panels, console view, menu bar,
 *               tool bars, dock widgets, status bar, and scenario data.
 *               Supports loading/saving JSON files, tracking unsaved changes,
 *               managing multiple inspector tabs, resetting layout, and
 *               displaying profile/application information.
 *
 * REQUIREMENTS: REQ-EDITOR-010  Main window with menu bar and tool bars
 *               REQ-EDITOR-011  Hierarchy tree view dock widget
 *               REQ-EDITOR-012  Inspector panel dock widget with multiple tabs
 *               REQ-EDITOR-013  Console view dock widget
 *               REQ-EDITOR-014  Load scenario from JSON file
 *               REQ-EDITOR-015  Track unsaved changes
 *               REQ-EDITOR-016  Reset layout to default
 *               REQ-EDITOR-017  Recent projects list
 *               REQ-EDITOR-018  Show profile info dialog
 *               REQ-EDITOR-019  Show application dialog
 *               REQ-EDITOR-020  Show feedback window
 *               REQ-EDITOR-021  Hierarchy loaded signal
 *               REQ-EDITOR-022  Unsaved changes changed signal
 *               REQ-EDITOR-023  Component inspector creation
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-EDITOR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

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
    Hierarchy* hierarchy = nullptr;
    Console* console = nullptr;
    HierarchyTree *treeView = nullptr;
    QDockWidget *consoleDock = nullptr;
    QStatusBar *statusBar;
public slots:
    void showProfileInfo();
    void showApplicationDialog();
private slots:
    // Add inspector tab
    void addInspectorTab();
    // Show feedback window
    void showFeedbackWindow();
    // Handle dock visibility changes
    void onDockVisibilityChanged(bool visible);
    void resetLayout();
    void onRecentProjectTriggered();
    void loadRecentProject(const QString& filePath);
    void onTreeItemSelected(QVariantMap data);
    void cleanupExtraInspectors();
    void showAllEntityComponents(const QString& entityId, const QString& entityName);
signals:
    // Signal unsaved changes state
    void unsavedChangesChanged(bool hasChanges);
    void Activated();
      void hierarchyLoaded(QJsonObject hierarchyData);
private:
    // %%% UI Setup Methods %%%
    // Configure menu bar
    void setupMenuBar();
    // Configure toolbars
    void setupToolBars();
    // Configure dock widgets
    void setupDockWidgets(QDockWidget::DockWidgetFeatures dockFeatures);
    void setupEnhancedDockWidgets();
    // %%% Core Components %%%
    // Scenario data structure
    Scenario* scenario = nullptr;
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
    QWidget* createComponentInspector(
        const QString& entityId,
        const QString& title,
        const QJsonObject& data,
        int preferredHeight);
    QWidget* createComponentInspectorWithDynamicHeight(
        const QString& entityId,
        const QString& title,
        const QJsonObject& data,
        int initialHeight);
};

#endif
