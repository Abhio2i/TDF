#ifndef SCENARIOEDITOR_TEST_H
#define SCENARIOEDITOR_TEST_H

#include <QObject>

class ScenarioEditor;

class TestScenarioEditor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();      // called once before all tests
    void cleanupTestCase();   // called once after all tests

    // Existing tests (13)
    void testWindowProperties();
    void testCoreComponents();
    void testDockWidgetsExist();
    void testHierarchyTree();
    void testLibraryTree();
    void testTacticalDisplay();
    void testDesignToolBar();
    void testUnsavedChanges();
    void testSignalsExist();
    void testScriptEngineExists();

    void testConsoleViewExists();
    void testResizeEventHandler();

    // New tests (to reach 30+)
    void testCustomDocksAreOverlayType();
    void testSidebarWidgetExists();
    void testLibraryDockInitiallyHidden();
    void testTextScriptDockInitiallyHidden();
    void testConsoleDockInitiallyHidden();
    void testHierarchyDockHasLockButton();
    void testInspectorDockHasLockButton();
    void testLayerPanelConnectedToCanvas();
    void testTacticalDisplayCanvasNotNull();
    void testDesignToolBarActionsExist();
    void testHierarchyTreeSearchBarExists();
    void testLibraryTreeSearchBarExists();
    void testLastSavedFilePathInitiallyEmpty();
    void testMarkUnsavedChangesTwice();
    void testClearUnsavedChangesWhenNoChanges();
    void testActivatedSignalEmittedOnShow();
    void testUnsavedChangesSignalParameter();
    void testHierarchyDockMinimumSize();
    void testInspectorDockMinimumSize();
    void testLibraryDockMinimumSize();
    void testSidebarDockFixedHeight();
    void testConsoleDockMinimumSize();


private:
    static ScenarioEditor* editor;   // shared instance for all tests
};

#endif
