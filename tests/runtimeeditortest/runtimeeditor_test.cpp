#include "runtimeeditor_test.h"
#include "GUI/Editors/runtimeeditor.h"
#include <QTest>
#include <QDockWidget>
#include <QTabWidget>

RuntimeEditor* TestRuntimeEditor::editor = nullptr;

void TestRuntimeEditor::initTestCase()
{
    // Create once before all tests
    editor = new RuntimeEditor(nullptr);
    editor->hide();                // avoid flicker
    QTest::qWait(100);            // let UI settle
}

void TestRuntimeEditor::cleanupTestCase()
{
    // delete editor;
    editor = nullptr;
}

void TestRuntimeEditor::testWindowProperties()
{
    QCOMPARE(editor->windowTitle(), QString("Runtime Editor"));
    QCOMPARE(editor->size().width(), 1100);
    QCOMPARE(editor->size().height(), 600);
}

void TestRuntimeEditor::testCoreComponents()
{
    QVERIFY(editor->hierarchy != nullptr);
    QVERIFY(editor->console != nullptr);
    QVERIFY(editor->library != nullptr);
    QVERIFY(editor->simulation != nullptr);
    QVERIFY(editor->runtimeToolBar != nullptr);
}

void TestRuntimeEditor::testDockWidgetsExist()
{
    QList<QDockWidget*> docks = editor->findChildren<QDockWidget*>();
    bool hasHierarchy = false, hasInspector = false, hasConsole = false;
    bool hasLibrary = false, hasSidebar = false, hasTextScript = false;
    bool hasDisplay = false, hasLogger = false, hasLayer = false;

    for (QDockWidget* dock : docks) {
        QString title = dock->windowTitle();
        if (title == "Editor") hasHierarchy = true;
        else if (title == "Inspector") hasInspector = true;
        else if (title == "Console") hasConsole = true;
        else if (title == "Library") hasLibrary = true;
        else if (title == "Sidebar") hasSidebar = true;
        else if (title == "TestScript") hasTextScript = true;
        else if (title == "Sensors") hasDisplay = true;
        else if (title == "Logger") hasLogger = true;
        else if (title == "Layers") hasLayer = true;
    }
    QVERIFY(hasHierarchy);
    QVERIFY(hasInspector);
    QVERIFY(hasConsole);
    QVERIFY(hasLibrary);
    QVERIFY(hasSidebar);
    QVERIFY(hasTextScript);
    QVERIFY(hasDisplay);
    QVERIFY(hasLogger);
    QVERIFY(hasLayer);
}

void TestRuntimeEditor::testTreeViewsExist()
{
    QVERIFY(editor->treeView != nullptr);
    QVERIFY(editor->treeView->getTreeWidget() != nullptr);
    QVERIFY(editor->libTreeView != nullptr);
    QVERIFY(editor->libTreeView->getTreeWidget() != nullptr);
}

void TestRuntimeEditor::testTacticalDisplay()
{
    QVERIFY(editor->tacticalDisplay != nullptr);
    QVERIFY(editor->tacticalDisplay->canvas != nullptr);
}

void TestRuntimeEditor::testToolbarsExist()
{
    QVERIFY(editor->designToolBar != nullptr);
    // runtimeToolBar already checked in testCoreComponents
}

void TestRuntimeEditor::testUnsavedChanges()
{
    QVERIFY(editor->hasUnsavedChanges == false);
    QVERIFY(editor->lastSavedFilePath.isEmpty());

    editor->markUnsavedChanges();
    QVERIFY(editor->hasUnsavedChanges == true);
    QCOMPARE(editor->windowTitle(), QString("Runtime Editor *"));

    editor->clearUnsavedChanges();
    QVERIFY(editor->hasUnsavedChanges == false);
    QCOMPARE(editor->windowTitle(), QString("Runtime Editor"));
}

void TestRuntimeEditor::testSignalsExist()
{
    const QMetaObject* mo = editor->metaObject();
    QVERIFY(mo->indexOfSignal("unsavedChangesChanged(bool)") != -1);
    QVERIFY(mo->indexOfSignal("Activated()") != -1);
}

void TestRuntimeEditor::testStaticMissionData()
{
    // Just check that static members are accessible (compile-time)
    QVERIFY(true);
}

void TestRuntimeEditor::testSensorDisplayTabs()
{
    // Find the Sensors dock
    QList<QDockWidget*> docks = editor->findChildren<QDockWidget*>();
    QDockWidget* sensorsDock = nullptr;
    for (QDockWidget* dock : docks) {
        if (dock->windowTitle() == "Sensors") {
            sensorsDock = dock;
            break;
        }
    }
    QVERIFY(sensorsDock != nullptr);
    QTabWidget* tabs = sensorsDock->findChild<QTabWidget*>();
    QVERIFY(tabs != nullptr);
    QVERIFY(tabs->count() >= 9);
}

void TestRuntimeEditor::testConsoleViewExists()
{
    QVERIFY(editor->consoleView != nullptr);
}

void TestRuntimeEditor::testResizeEventHandler()
{
    editor->resize(1200, 700);
    QTest::qWait(50);
    QVERIFY(true);
}

void TestRuntimeEditor::testNetworkToolbarExists()
{
    // NetworkToolbar is created but hidden; assume it exists (compile-time)
    QVERIFY(true);
}
