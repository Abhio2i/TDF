#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QMenuBar>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPointer>
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
#include <QDateTime>          // ARTI CHANGE: needed for scenario->runtime staleness tracking (mtime compare)
#include <QSet>               // ARTI CHANGE: needed for per-target pending entity-sync sets

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum class EditorKey { Database, Scenario, Mission, Runtime, Analysis };
    static QString editorKeyToString(EditorKey key);
    static EditorKey editorKeyFromString(const QString& key);

    static ScenarioConfig* scenarioconfig;
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QMainWindow* getCurrentEditor() const;
    static MainWindow* instance();

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
    QString ensureTDFSubfolder(const QString& subfolderName) const;
    QString associatedDatabasePathForHierarchy(Hierarchy* hier) const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void switchEditor(const QString &editorKey);
    void onUnsavedChangesChanged(bool hasUnsaved);
    bool promptForSave(QMainWindow* editor, const QString& editorName);
    void handleLibraryProjectSelected(const QString& filePath, RecentProjectsManager::EditorType type);

private:
    // ---- setup ----
    void setupUI();
    void setupMenuBarConnections();
    void connectFileMenuActions();
    void connectLoadActions();
    void connectLibraryActions();
    void connectSettingsAndInfoActions();
    void connectPluginActions();
    QWidget* createTopBar();
    QPushButton* createHelpButton();
    void onHelpButtonClicked();
    void copyDirectoryRecursive(const QString& src, const QString& dst);

    // ---- unsaved-changes / save helpers ----
    bool handleUnsavedChanges();
    bool showUnsavedChangesPrompt(QMainWindow* currentEditor,
                                  const QString& editorName,
                                  const QString& lastSavedFilePath);
    void getEditorUnsavedState(QMainWindow* editor, bool& hasUnsaved,
                               QString& editorName, QString& lastSavedFilePath) const;
    void clearUnsavedForEditor(QMainWindow* editor);

    // ---- titles ----
    void updateWindowTitleForCurrentEditor();
    void updateWindowTitle(const QString& editorName, bool hasUnsavedChanges);
    QString buildTitleSuffixForEditor(QMainWindow* currentWidget, const QString& editorName) const;

    static MainWindow* s_instance;
    MenuBar *mainMenuBar = nullptr;

    // ---- TDF helpers ----
    void loadFileWithTDFSupport(QMainWindow* editor,
                                RecentProjectsManager::EditorType editorType,
                                const QString& extension,
                                const QString& filter);
    void saveFileWithTDFSupport(QMainWindow* editor);
    void saveToSameFileWithTDFSupport(QMainWindow* editor);
    QJsonObject buildSaveJson(QMainWindow* editor) const;
    void createScenarioInstanceCopy(const QString& runtimeFilePath, const QJsonObject& data);
    void loadLibraryFileForEditor(const QString& filePath, QMainWindow* editor);

    // Database-centric project folders under TDF/<dbName>/
    QString tdfRootPath() const;
    QString databaseProjectNameFromPath(const QString& dbFilePath) const;
    QString ensureDatabaseProjectFolder(const QString& dbName) const;
    QString ensureScenarioFolderForDb(const QString& dbFilePath) const;
    QString ensureRuntimeFolderForDb(const QString& dbFilePath, const QString& scenarioName = QString()) const;
    QString resolveActiveDatabasePath() const;

    QString startPathForEditorType(RecentProjectsManager::EditorType editorType) const;

    QWidget*      m_loadingOverlay = nullptr;
    QLabel*       m_loadingLabel   = nullptr;
    QProgressBar* m_loadingBar     = nullptr;
    QWidget*      m_loadingCard    = nullptr;

    void onDatabaseSettingsChanged(bool enabled, const QString& path);
    bool m_wasSimulationRunning = false;
    void pauseSimulationIfRunning();
    QString resolveLibraryDbPath() const;
    void loadLibraryForEditor(Hierarchy* library,
                              HierarchyTree* libTreeView,
                              const QString& dbPath);
    void reconnectLibrarySignals(Hierarchy* library,
                                 HierarchyTree* libTreeView);

    static const QString kScenarioInstanceFolder;
    static const QString kScenarioPrefix;
    static const QString kRuntimePrefix;
    static const QString kInstancePrefix;
    void syncDatabaseEntityChanges();
    void onDatabaseFileChanged(const QString& newDbPath);
    void unloadScenarioAndRuntimeEditors();
    void onScenarioFileLoaded(const QString& scenarioPath);
    void onRuntimeFileLoaded(const QString& runtimePath);
    QString resolveDatabasePathFromProjectFile(const QString& projectFilePath) const;
    QString resolveScenarioPathFromRuntimeFile(const QString& runtimeFilePath) const;
    bool isSameDatabaseProject(const QString& projectFilePath, const QString& dbFilePath) const;
    QString  m_scenarioPathPushedToRuntime;
    QDateTime m_scenarioMTimePushedToRuntime;
    QSet<QString> m_pendingEntitySyncToScenario;
    QSet<QString> m_pendingEntitySyncToRuntime;
    void captureGisSnapshotFor(QWidget* editorWidget);
    void restoreGisSnapshotFor(QWidget* editorWidget);
    QJsonObject m_scenarioGisSnapshot;
    QJsonObject m_runtimeGisSnapshot;
    bool m_hasScenarioGisSnapshot = false;
    bool m_hasRuntimeGisSnapshot  = false;
};
#endif // MAINWINDOW_H
