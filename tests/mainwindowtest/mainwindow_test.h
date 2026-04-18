#ifndef MAINWINDOW_TEST_H
#define MAINWINDOW_TEST_H

#include <QObject>

class MainWindow;

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Existing tests (11)
    void testWindowProperties();
    void testCentralAndStackedWidget();
    void testEditorsInitialState();
    void testMenuBar();

    void testStatusBar();
    void testSingletonInstance();
    void testCurrentEditor();
    void testScenarioConfigExists();
    void testLoadingOverlay();
    void testHelperMethodsExist();

    // New tests (to reach 30+)
    void testWindowTitleContainsVersion();
    void testDatabaseEditorActivatedSignal();
    void testStackedWidgetContainsDatabaseEditor();
    void testStatusBarInitialMessage();
    void testEnsureTDFSubfolderReturnsNonEmpty();
    void testLoadFileWithTDFSupportExists();
    void testSaveFileWithTDFSupportExists();
    void testSaveToSameFileWithTDFSupportExists();
    void testCreateScenarioInstanceCopyExists();

    void testSwitchEditorToScenarioLazyCreates();
    void testSwitchEditorToMissionLazyCreates();
    void testSwitchEditorToRuntimeLazyCreates();
    void testSwitchEditorToAnalysisLazyCreates();
    void testGetDatabaseHierarchyReturnsNonNull();
    void testUnsavedChangesSignalConnected();
    void testEditorSwitchEmitsActivated();
    void testShowHideLoadingOverlayMultipleCalls();
    void testCloseEventHasHandler();

private:
    MainWindow* mainWindow = nullptr;
};

#endif
