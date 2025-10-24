/* ========================================================================= */
/* File: mainwindow.h                                                       */
/* Purpose: Defines main application window with editor navigation           */
/* ========================================================================= */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>                            // For main window base class
#include <QStackedWidget>                         // For stacked widget
#include "GUI/Navigation/navigationpage.h"        // For navigation page
#include "GUI/Editors/databaseeditor.h"           // For database editor
#include "GUI/Editors/scenarioeditor.h"           // For scenario editor
#include "GUI/Editors/runtimeeditor.h"            // For runtime editor

// %%% Class Definition %%%
/* Main application window */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Initialize main window
    explicit MainWindow(QWidget *parent = nullptr);
    // Clean up resources
    ~MainWindow();

private slots:
    // Switch to editor by key
    void switchEditor(const QString &editorKey);

private:
    // %%% Utility Methods %%%
    // Setup user interface
    void setupUI();
    // Get current editor
    QMainWindow* getCurrentEditor() const;
    // Handle unsaved changes
    bool handleUnsavedChanges();

    // %%% UI Components %%%
    // Stacked widget for editors
    QStackedWidget *stackedWidget;
    // Navigation page
    NavigationPage *navigationPage;
    // Database editor
    DatabaseEditor *databaseEditor;
    // Scenario editor
    ScenarioEditor *scenarioEditor;
    // Runtime editor
    RuntimeEditor *runtimeEditor;
};

#endif // MAINWINDOW_H
