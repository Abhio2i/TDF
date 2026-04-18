#ifndef RUNTIMEEDITOR_TEST_H
#define RUNTIMEEDITOR_TEST_H

#include <QObject>

class RuntimeEditor;

class TestRuntimeEditor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();      // called once before all tests
    void cleanupTestCase();   // called once after all tests

    void testWindowProperties();
    void testCoreComponents();
    void testDockWidgetsExist();
    void testTreeViewsExist();
    void testTacticalDisplay();
    void testToolbarsExist();
    void testUnsavedChanges();
    void testSignalsExist();
    void testStaticMissionData();
    void testSensorDisplayTabs();
    void testConsoleViewExists();
    void testResizeEventHandler();
    void testNetworkToolbarExists();

private:
    static RuntimeEditor* editor;   // shared instance for all tests
};

#endif
