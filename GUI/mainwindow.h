/* ========================================================================= */
/* File: mainwindow.h                                                       */
/* Purpose: Main application window with multi-editor navigation system      */
//               Written by Arti Rajpoot
/* ========================================================================= */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QMenuBar>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include "GUI/Editors/databaseeditor.h"
#include "GUI/Editors/scenarioeditor.h"
#include "GUI/Editors/runtimeeditor.h"
#include "GUI/Menubars/menubar.h"
#include "GUI/Navigation/navigationpage.h"
#include "GUI/Editors/recentprojectsmanager.h"
#include "GUI/Feedback/projectinformation.h"
#include "GUI/Menubars/profileinfodialog.h"
#include "GUI/Settings/applicationdialog.h"
#include <core/Config/scenarioconfig.h>

// %%% Class Definition %%%
/* Main application window managing multiple editors and navigation */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // %%% Static Configuration %%%
    /* Shared scenario configuration instance */
    static ScenarioConfig* scenarioconfig;

    // %%% Constructor %%%
    /* Initialize main window with parent widget */
    explicit MainWindow(QWidget *parent = nullptr);

    // %%% Destructor %%%
    /* Clean up main window resources */
    ~MainWindow();

    // %%% Accessor Methods %%%
    /* Get current active editor window */
    QMainWindow* getCurrentEditor() const;
    static MainWindow* instance();
    DatabaseEditor *databaseEditor;    // Database editing interface
    ScenarioEditor *scenarioEditor;    // Scenario editing interface
    RuntimeEditor *runtimeEditor;      // Runtime editing interface
    QStackedWidget *stackedWidget;
    Hierarchy* getDatabaseHierarchy() const {
        return databaseEditor ? databaseEditor->hierarchy : nullptr;
    }
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    // %%% Editor Management Slots %%%
    /* Switch between different editor interfaces */
    void switchEditor(const QString &editorKey);

    // %%% Change Handling Slots %%%
    /* Handle unsaved changes state updates */
    void onUnsavedChangesChanged(bool hasUnsaved);
    bool promptForSave(QMainWindow* editor, const QString& editorName);
private:
    // %%% UI Setup Methods %%%
    /* Set up user interface components */
    void setupUI();
    void setupMenuBarConnections();
    // %%% Change Management Methods %%%
    /* Handle unsaved changes before switching editors */
    bool handleUnsavedChanges();
    // %%% Title Management Methods %%%
    /* Update window title for current editor */
    void updateWindowTitleForCurrentEditor();
    /* Update window title with editor and file information */
    void updateWindowTitle(const QString& editorName, const QString& filePath,
                           bool hasUnsavedChanges = false);

    // %%% UI Component Members %%%
    /*    QStackedWidget *stackedWidget; */
    NavigationPage *navigationPage;    // Navigation interface page
    // DatabaseEditor *databaseEditor;    // Database editing interface
    // ScenarioEditor *scenarioEditor;    // Scenario editing interface
    // RuntimeEditor *runtimeEditor;      // Runtime editing interface
    static MainWindow* s_instance;
    MenuBar*mainMenuBar;
    QString ensureTDFSubfolder(const QString& subfolderName);
    void loadFileWithTDFSupport(QMainWindow* editor,
                                RecentProjectsManager::EditorType editorType,
                                const QString& extension,
                                const QString& filter);
    void saveFileWithTDFSupport(QMainWindow* editor);
    void saveToSameFileWithTDFSupport(QMainWindow* editor);
    void createScenarioInstanceCopy(const QString& runtimeFilePath, const QJsonObject& data);
};

#endif // MAINWINDOW_H
