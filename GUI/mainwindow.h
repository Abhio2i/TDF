/* =============================================================================
 * FILE:         mainwindow.h
 * MODULE:       Main Application Window
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the MainWindow class which serves as the main
 *               application window with a multi‑editor navigation system.
 *               It manages switching between different editors (DatabaseEditor,
 *               ScenarioEditor, MissionEditor, RuntimeEditor, AnalysisEditor)
 *               via a stacked widget and navigation page. Handles unsaved
 *               changes, file operations (load/save) with TDF subfolder support,
 *               recent projects, loading overlay, status bar, and application/
 *               profile information dialogs. Integrates with ScenarioConfig
 *               for global configuration and RecentProjectsManager for recent
 *               file lists.
 *
 * REQUIREMENTS: REQ-MAIN-010  Main window with multi‑editor navigation
 *               REQ-MAIN-011  Switch between editors (database, scenario,
 *                             mission, runtime, analysis)
 *               REQ-MAIN-012  Stacked widget to hold editor instances
 *               REQ-MAIN-013  Navigation page for editor selection
 *               REQ-MAIN-014  Unsaved changes tracking and prompts
 *               REQ-MAIN-015  File operations: open, save, save as with TDF
 *                             subfolder management
 *               REQ-MAIN-016  Recent projects integration
 *               REQ-MAIN-017  Loading overlay with progress bar
 *               REQ-MAIN-018  Status bar for messages
 *               REQ-MAIN-019  Application and profile information dialogs
 *               REQ-MAIN-020  Singleton instance access
 *               REQ-MAIN-021  Close event with unsaved changes handling
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-MAIN-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

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
#include "GUI/Editors/missioneditor.h"
#include "GUI/Editors/runtimeeditor.h"
#include "GUI/Menubars/menubar.h"
#include "GUI/Navigation/navigationpage.h"
#include "GUI/Editors/recentprojectsmanager.h"
#include "GUI/Feedback/projectinformation.h"
#include "GUI/Menubars/profileinfodialog.h"
#include "GUI/Settings/applicationdialog.h"
#include <core/Config/scenarioconfig.h>
#include "GUI/Editors/analysiseditor.h"
#include <QProgressBar>
#include <GUI/statusbar.h>
// #include "GUI/plugins/pluginmanagerdialog.h"
// %%% Class Definition %%%
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // %%% Static Configuration %%%
    static ScenarioConfig* scenarioconfig;
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    // %%% Accessor Methods %%%
    QMainWindow* getCurrentEditor() const;
    static MainWindow* instance();
    // %%% Editor instances %%%
    DatabaseEditor *databaseEditor = nullptr;
    ScenarioEditor *scenarioEditor = nullptr;
    MissionEditor  *missionEditor  = nullptr;
    RuntimeEditor  *runtimeEditor  = nullptr;
    QStackedWidget *stackedWidget  = nullptr;
    AnalysisEditor *analysisEditor  = nullptr;
    Hierarchy* getDatabaseHierarchy() const {
        return databaseEditor ? databaseEditor->hierarchy : nullptr;
    }
    void showLoadingOverlay(const QString& message = "Loading...");
    void hideLoadingOverlay();
    NavigationPage *navigationPage = nullptr;
    StatusBar *m_statusBar = nullptr;
    QString ensureTDFSubfolder(const QString& subfolderName);
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void switchEditor(const QString &editorKey);
    void onUnsavedChangesChanged(bool hasUnsaved);
    bool promptForSave(QMainWindow* editor, const QString& editorName);
private:
    void setupUI();
    void setupMenuBarConnections();
    bool handleUnsavedChanges();
    void updateWindowTitleForCurrentEditor();
    void updateWindowTitle(const QString& editorName, bool hasUnsavedChanges);
    static MainWindow* s_instance;
    MenuBar *mainMenuBar = nullptr;
    // %%% TDF helpers %%%
    void loadFileWithTDFSupport(QMainWindow* editor,
                                RecentProjectsManager::EditorType editorType,
                                const QString& extension,
                                const QString& filter);
    void saveFileWithTDFSupport(QMainWindow* editor);
    void saveToSameFileWithTDFSupport(QMainWindow* editor);
    void createScenarioInstanceCopy(const QString& runtimeFilePath, const QJsonObject& data);
    QWidget*      m_loadingOverlay = nullptr;
    QLabel*       m_loadingLabel   = nullptr;
    QProgressBar* m_loadingBar     = nullptr;
    void onDatabaseSettingsChanged(bool enabled, const QString& path);
    bool m_wasSimulationRunning = false;
    void pauseSimulationIfRunning();
};

#endif // MAINWINDOW_H
