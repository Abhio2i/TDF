/* ========================================================================= */
/* File: mainwindow.h                                                        */
/* Purpose: Main application window with multi-editor navigation system      */
/* Written by: Arti Rajpoot                                                  */
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
    // NavigationPage *navigationPage = nullptr;
    static MainWindow* s_instance;
    MenuBar *mainMenuBar = nullptr;
    // %%% TDF helpers %%%
    // QString ensureTDFSubfolder(const QString& subfolderName);
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

};

#endif // MAINWINDOW_H
