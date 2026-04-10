#include "gui_mission_test.h"
#include "GUI/Editors/missioneditor.h"
#include "core/Debug/console.h"
#include <QTimer>

// Helper macros (console pointer must be valid)
#define GUI_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runMissionEditorTests(MissionEditor* editor, Console* console)
{
    if (!editor || !console) {
        if (console) console->error("MissionEditor or Console is null, cannot run GUI tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("   MISSION EDITOR GUI UNIT TESTS        "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: Check if main panels are created -----
    GUI_TEST(editor->doctrinePanel != nullptr, "DoctrineParameters panel exists");
    GUI_TEST(editor->tacticalPanel != nullptr, "TacticalRules panel exists");
    GUI_TEST(editor->assumptionsPanel != nullptr, "DoctrineAssumptionsNotes panel exists");
    GUI_TEST(editor->areaDefinitionPanel != nullptr, "DoctrineAreaDefinition panel exists");
    GUI_TEST(editor->treeView != nullptr, "HierarchyTree view exists");

    // ----- Test 2: DoctrineParameters force type (using public getter) -----
    if (editor->doctrinePanel) {
        // Assuming you added getForceType()
        QString forceType = editor->doctrinePanel->getForceType();
        GUI_TEST(forceType == "Red" || forceType == "Red Force",
                 "DoctrineParameters default force type is Red");
    }

    // ----- Test 3: TacticalRules initial empty state -----
    if (editor->tacticalPanel) {
        // Assuming you added getRulesCount()
        bool isEmpty = (editor->tacticalPanel->getRulesCount() == 0);
        GUI_TEST(isEmpty, "TacticalRules initially empty");
    }

    // ----- Test 4: Hierarchy tree has items (after sync from Scenario) -----
    if (editor->treeView && editor->treeView->getTreeWidget()) {
        int topLevelCount = editor->treeView->getTreeWidget()->topLevelItemCount();
        GUI_TEST(topLevelCount > 0, "Hierarchy tree has at least one top-level item (synced from Scenario)");
    }

    // ----- Test 5: Dock widgets exist and are visible (using public getters) -----
    GUI_TEST(editor->isHierarchyDockVisible(), "Hierarchy dock is visible");
    GUI_TEST(editor->isDoctrineDockVisible(), "Doctrine dock is visible");
    GUI_TEST(editor->isTacticalDockVisible(), "Tactical dock is visible");
    GUI_TEST(editor->isAssumptionsDockVisible(), "Assumptions dock is visible");
    GUI_TEST(editor->isAreaDefinitionDockVisible(), "AreaDefinition dock is visible");

    // ----- Test 6: Unsaved changes flag works -----
    bool initial = editor->hasUnsavedChanges;
    editor->markUnsavedChanges();
    GUI_TEST(editor->hasUnsavedChanges == true, "markUnsavedChanges sets flag to true");
    editor->clearUnsavedChanges();
    GUI_TEST(editor->hasUnsavedChanges == false, "clearUnsavedChanges resets flag to false");
    if (initial) editor->markUnsavedChanges();

    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("MISSION EDITOR GUI TESTS: Some tests FAILED."));
    else
        console->log(std::string("MISSION EDITOR GUI TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
