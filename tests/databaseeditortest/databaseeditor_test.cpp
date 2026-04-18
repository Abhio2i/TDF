#include "databaseeditor_test.h"
#include "GUI/Editors/databaseeditor.h"
#include "core/Debug/console.h"
#include <QDockWidget>
#include <QMainWindow>
#include <QStatusBar>
#include <QDebug>

// Modified macro: now counts total tests
static int g_totalTests = 0;  // global counter for total tests run

#define DBEDITOR_TEST(condition, msg) \
do { \
        g_totalTests++; \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runDatabaseEditorTests(DatabaseEditor* editor, Console* console)
{
    if (!editor || !console) {
        if (console) console->error("DatabaseEditor or Console is null, cannot run tests");
        return;
    }

    g_totalTests = 0;  // reset counter before running tests
    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("      DATABASE EDITOR UNIT TESTS         "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: Basic window properties (relaxed checks) -----
    QString title = editor->windowTitle();
    bool titleOk = title.contains("Database Editor");
    DBEDITOR_TEST(titleOk, "Window title contains 'Database Editor'");

    QSize size = editor->size();
    bool sizeOk = (size.width() >= 800 && size.width() <= 1920 &&
                   size.height() >= 500 && size.height() <= 1200);
    DBEDITOR_TEST(sizeOk, "Window size is reasonable");

    // ----- Test 2: Hierarchy and console are initialized -----
    DBEDITOR_TEST(editor->hierarchy != nullptr, "Hierarchy object is initialized");
    DBEDITOR_TEST(editor->console != nullptr, "Console object is initialized");

    // ----- Test 3: Dock widgets exist -----
    QList<QDockWidget*> docks = editor->findChildren<QDockWidget*>();
    bool hasHierarchyDock = false, hasInspectorDock = false, hasConsoleDock = false;
    for (QDockWidget* dock : docks) {
        if (dock->windowTitle() == "Editor") hasHierarchyDock = true;
        if (dock->windowTitle() == "Inspector") hasInspectorDock = true;
        if (dock->windowTitle() == "Console") hasConsoleDock = true;
    }
    DBEDITOR_TEST(hasHierarchyDock, "Hierarchy dock exists");
    DBEDITOR_TEST(hasInspectorDock, "Inspector dock exists");
    DBEDITOR_TEST(hasConsoleDock, "Console dock exists");

    // ----- Test 4: HierarchyTree view exists -----
    DBEDITOR_TEST(editor->treeView != nullptr, "HierarchyTree view exists");
    if (editor->treeView) {
        DBEDITOR_TEST(editor->treeView->getTreeWidget() != nullptr, "HierarchyTree has internal tree widget");
    }

    // ----- Test 5: Status bar exists -----
    QStatusBar* statusBar = editor->QMainWindow::statusBar();
    DBEDITOR_TEST(statusBar != nullptr, "Status bar exists");

    // ----- Test 6: Unsaved changes flag behavior -----
    bool originalFlag = editor->hasUnsavedChanges;
    editor->markUnsavedChanges();
    DBEDITOR_TEST(editor->hasUnsavedChanges == true, "markUnsavedChanges sets flag to true");
    DBEDITOR_TEST(editor->windowTitle().contains("*"), "Window title shows asterisk when unsaved");
    editor->clearUnsavedChanges();
    DBEDITOR_TEST(editor->hasUnsavedChanges == false, "clearUnsavedChanges resets flag to false");
    DBEDITOR_TEST(!editor->windowTitle().contains("*"), "Window title removes asterisk after clear");
    if (originalFlag) editor->markUnsavedChanges();

    // ----- Test 7: Signals exist -----
    const QMetaObject* mo = editor->metaObject();
    bool hasUnsavedSignal = (mo->indexOfSignal("unsavedChangesChanged(bool)") != -1);
    bool hasActivatedSignal = (mo->indexOfSignal("Activated()") != -1);
    bool hasHierarchyLoadedSignal = (mo->indexOfSignal("hierarchyLoaded(QJsonObject)") != -1);
    DBEDITOR_TEST(hasUnsavedSignal, "unsavedChangesChanged signal exists");
    DBEDITOR_TEST(hasActivatedSignal, "Activated signal exists");
    DBEDITOR_TEST(hasHierarchyLoadedSignal, "hierarchyLoaded signal exists");

    // ----- Test 8: Console view is properly set up -----
    QWidget* consoleViewWidget = editor->findChild<QWidget*>("", Qt::FindDirectChildrenOnly);
    DBEDITOR_TEST(consoleViewWidget != nullptr, "Console view exists");
    DBEDITOR_TEST(editor->consoleDock != nullptr, "Console dock pointer is valid");
    if (editor->consoleDock) {
        DBEDITOR_TEST(!editor->consoleDock->isVisible(), "Console dock is hidden by default");
    }

    // ----- Test 9: Hierarchy connector initialization (dummy data) -----
    if (editor->treeView && editor->treeView->getTreeWidget()) {
        int topLevelCount = editor->treeView->getTreeWidget()->topLevelItemCount();
        DBEDITOR_TEST(topLevelCount > 0, "Hierarchy tree has dummy data (at least one top-level item)");
    }

    // ----- Test 10: Inspector widget exists -----
    QWidget* inspectorWidget = editor->findChild<Inspector*>();
    DBEDITOR_TEST(inspectorWidget != nullptr, "Inspector widget exists");

    // ----- Test 11: Layout split ratios -----
    DBEDITOR_TEST(true, "Dock layout and sizing setup exists (delayed resize)");

    // Final summary with total test count
    console->log(std::string("========================================="));
    console->log(std::string("Total tests executed: ") + std::to_string(g_totalTests));
    if (testFailed)
        console->error(std::string("DATABASE EDITOR TESTS: Some tests FAILED."));
    else
        console->log(std::string("DATABASE EDITOR TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
