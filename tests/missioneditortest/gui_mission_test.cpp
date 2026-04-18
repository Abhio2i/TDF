#include "gui_mission_test.h"
#include "GUI/Editors/missioneditor.h"
#include <QTest>
#include <QTreeWidget>

void TestMissionEditor::init()
{
    // Create a fresh MissionEditor for each test
    // The editor may need a parent (e.g., central widget). We can pass nullptr for standalone testing.
    editor = new MissionEditor(nullptr);
    // Wait a moment for UI to settle (if needed)
    QTest::qWait(100);
}

void TestMissionEditor::cleanup()
{
    delete editor;
    editor = nullptr;
}

void TestMissionEditor::testPanelsExist()
{
    QVERIFY(editor->doctrinePanel != nullptr);
    QVERIFY(editor->tacticalPanel != nullptr);
    QVERIFY(editor->assumptionsPanel != nullptr);
    QVERIFY(editor->areaDefinitionPanel != nullptr);
    QVERIFY(editor->treeView != nullptr);
}

void TestMissionEditor::testDoctrineDefaultForceType()
{
    if (editor->doctrinePanel) {
        QString forceType = editor->doctrinePanel->getForceType();
        // Default force type may be "Red" or "Red Force" depending on implementation
        QVERIFY(forceType == "Red" || forceType == "Red Force");
    } else {
        QSKIP("doctrinePanel is null, cannot test force type");
    }
}

void TestMissionEditor::testTacticalRulesInitiallyEmpty()
{
    if (editor->tacticalPanel) {
        // Assuming getRulesCount() returns the number of tactical rules
        bool isEmpty = (editor->tacticalPanel->getRulesCount() == 0);
        QVERIFY(isEmpty);
    } else {
        QSKIP("tacticalPanel is null, cannot test rule count");
    }
}

void TestMissionEditor::testHierarchyTreeHasItems()
{
    if (editor->treeView && editor->treeView->getTreeWidget()) {
        int topLevelCount = editor->treeView->getTreeWidget()->topLevelItemCount();
        // After syncing from Scenario, there should be at least one top-level item (e.g., a profile)
        QVERIFY(topLevelCount > 0);
    } else {
        QSKIP("treeView or tree widget is null");
    }
}


void TestMissionEditor::testUnsavedChangesFlag()
{
    bool initial = editor->hasUnsavedChanges;
    editor->markUnsavedChanges();
    QVERIFY(editor->hasUnsavedChanges == true);
    editor->clearUnsavedChanges();
    QVERIFY(editor->hasUnsavedChanges == false);
    if (initial) editor->markUnsavedChanges(); // restore if needed
}
