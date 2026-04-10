#include "gui_menubar_test.h"
#include "GUI/Menubars/menubar.h"
#include "core/Debug/console.h"
#include <QAction>
#include <QMenu>

// Helper macros (same as before)
#define MENU_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runMenuBarTests(MenuBar* menuBar, Console* console)
{
    if (!menuBar || !console) {
        if (console) console->error("MenuBar or Console is null, cannot run tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("         MENUBAR UNIT TESTS               "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: File menu exists -----
    QMenu* fileMenu = menuBar->getFileMenu();
    MENU_TEST(fileMenu != nullptr, "File menu exists");

    // ----- Test 2: All expected actions are created -----
    QAction* newAction = menuBar->getNewFileAction();
    QAction* recentProjectAction = menuBar->getRecentProjectAction();
    QAction* recentLibAction = menuBar->getrecentProjectLibraryAction();
    QAction* loadAction = menuBar->getLoadAction();
    QAction* loadXmlAction = menuBar->getLoadXmlAction();
    QAction* loadToLibAction = menuBar->getLoadToLibraryAction();
    QAction* openRuntimeAction = menuBar->getOpenRuntimeInstanceAction();
    QAction* openMissionAction = menuBar->getOpenMissionFileAction();
    QAction* saveAction = menuBar->getSaveAction();
    QAction* sameSaveAction = menuBar->getSameSaveAction();
    QAction* exitAction = menuBar->getExitAction();

    MENU_TEST(newAction != nullptr, "New File action exists");
    MENU_TEST(recentProjectAction != nullptr, "Recent Project action exists");
    MENU_TEST(recentLibAction != nullptr, "Recent Library action exists");
    MENU_TEST(loadAction != nullptr, "Open File action exists");
    MENU_TEST(loadXmlAction != nullptr, "Open XML File action exists");
    MENU_TEST(loadToLibAction != nullptr, "Open File to Library action exists");
    MENU_TEST(openRuntimeAction != nullptr, "Open Runtime Instance action exists");
    MENU_TEST(openMissionAction != nullptr, "Open Mission File action exists");
    MENU_TEST(saveAction != nullptr, "Save As action exists");
    MENU_TEST(sameSaveAction != nullptr, "Save action exists");
    MENU_TEST(exitAction != nullptr, "Exit action exists");

    // ----- Test 3: Feedback/About menu and action -----
    QAction* feedbackAction = menuBar->getFeedbackAction();
    MENU_TEST(feedbackAction != nullptr, "Feedback action exists");

    // ----- Test 4: Performance and Settings actions -----
    QAction* profileAction = menuBar->getProfileAction();
    QAction* settingsAction = menuBar->getApplicationAction();
    MENU_TEST(profileAction != nullptr, "Performance action exists");
    MENU_TEST(settingsAction != nullptr, "Settings action exists");

    // ----- Test 5: Edit actions exist (undo, redo, cut, copy, paste, etc.) -----
    QAction* undoAction = menuBar->getUndoAction();
    QAction* redoAction = menuBar->getRedoAction();
    QAction* selectAllAction = menuBar->getSelectAllAction();
    QAction* deselectAllAction = menuBar->getDeselectAllAction();
    QAction* cutAction = menuBar->getCutAction();
    QAction* copyAction = menuBar->getCopyAction();
    QAction* pasteAction = menuBar->getPasteAction();
    QAction* duplicateAction = menuBar->getDuplicateAction();
    QAction* renameAction = menuBar->getRenameAction();
    QAction* deleteAction = menuBar->getDeleteAction();

    MENU_TEST(undoAction != nullptr, "Undo action exists");
    MENU_TEST(redoAction != nullptr, "Redo action exists");
    MENU_TEST(selectAllAction != nullptr, "Select All action exists");
    MENU_TEST(deselectAllAction != nullptr, "Deselect All action exists");
    MENU_TEST(cutAction != nullptr, "Cut action exists");
    MENU_TEST(copyAction != nullptr, "Copy action exists");
    MENU_TEST(pasteAction != nullptr, "Paste action exists");
    MENU_TEST(duplicateAction != nullptr, "Duplicate action exists");
    MENU_TEST(renameAction != nullptr, "Rename action exists");
    MENU_TEST(deleteAction != nullptr, "Delete action exists");

    // ----- Test 6: setLibraryActionsVisible works -----
    // Store initial visibility
    bool initialLibVisible = recentLibAction->isVisible();
    bool initialLoadToLibVisible = loadToLibAction->isVisible();
    bool initialOpenRuntimeVisible = openRuntimeAction->isVisible();

    // Set to false
    menuBar->setLibraryActionsVisible(false);
    MENU_TEST(!recentLibAction->isVisible(), "Recent Library action hidden after setLibraryActionsVisible(false)");
    MENU_TEST(!loadToLibAction->isVisible(), "Load To Library action hidden after setLibraryActionsVisible(false)");
    MENU_TEST(!openRuntimeAction->isVisible(), "Open Runtime Instance action hidden after setLibraryActionsVisible(false)");

    // Set back to true
    menuBar->setLibraryActionsVisible(true);
    MENU_TEST(recentLibAction->isVisible(), "Recent Library action visible after setLibraryActionsVisible(true)");
    MENU_TEST(loadToLibAction->isVisible(), "Load To Library action visible after setLibraryActionsVisible(true)");
    MENU_TEST(openRuntimeAction->isVisible(), "Open Runtime Instance action visible after setLibraryActionsVisible(true)");

    // Restore original state
    menuBar->setLibraryActionsVisible(initialLibVisible);

    // ----- Test 7: updateFileMenuForEditor for each editor type -----
    // Test "database" editor
    menuBar->updateFileMenuForEditor("database");
    MENU_TEST(openMissionAction->isVisible() == false, "updateFileMenuForEditor('database'): Open Mission File hidden");
    // (Other actions should be visible; we can test a couple)
    MENU_TEST(newAction->isVisible() == true, "updateFileMenuForEditor('database'): New File visible");
    MENU_TEST(loadXmlAction->isVisible() == true, "updateFileMenuForEditor('database'): Load XML visible");

    // Test "mission" editor
    menuBar->updateFileMenuForEditor("mission");
    MENU_TEST(newAction->isVisible() == false, "updateFileMenuForEditor('mission'): New File hidden");
    MENU_TEST(loadXmlAction->isVisible() == false, "updateFileMenuForEditor('mission'): Load XML hidden");
    MENU_TEST(openMissionAction->isVisible() == false, "updateFileMenuForEditor('mission'): Open Mission File hidden");
    MENU_TEST(loadAction->isVisible() == true, "updateFileMenuForEditor('mission'): Open File visible");
    MENU_TEST(saveAction->isVisible() == true, "updateFileMenuForEditor('mission'): Save As visible");

    // Test "analysis" editor
    menuBar->updateFileMenuForEditor("analysis");
    MENU_TEST(newAction->isVisible() == false, "updateFileMenuForEditor('analysis'): New File hidden");
    MENU_TEST(recentProjectAction->isVisible() == false, "updateFileMenuForEditor('analysis'): Recent Project hidden");
    MENU_TEST(loadAction->isVisible() == false, "updateFileMenuForEditor('analysis'): Open File hidden");
    MENU_TEST(loadXmlAction->isVisible() == false, "updateFileMenuForEditor('analysis'): Load XML hidden");
    MENU_TEST(loadToLibAction->isVisible() == false, "updateFileMenuForEditor('analysis'): Load To Library hidden");
    MENU_TEST(openRuntimeAction->isVisible() == false, "updateFileMenuForEditor('analysis'): Open Runtime hidden");
    MENU_TEST(openMissionAction->isVisible() == false, "updateFileMenuForEditor('analysis'): Open Mission hidden");
    MENU_TEST(sameSaveAction->isVisible() == false, "updateFileMenuForEditor('analysis'): Save hidden");
    MENU_TEST(saveAction->isVisible() == false, "updateFileMenuForEditor('analysis'): Save As hidden");
    MENU_TEST(exitAction->isVisible() == true, "updateFileMenuForEditor('analysis'): Exit visible");

    // Restore to a default state (e.g., database) so UI looks normal
    menuBar->updateFileMenuForEditor("database");

    // ----- Test 8: Verify signals are properly connected (optional) -----
    // We can't easily test signal emission without mocking, but we can check that actions are not null
    // and that they are enabled. For a more advanced test, you could use QSignalSpy.

    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("MENUBAR TESTS: Some tests FAILED."));
    else
        console->log(std::string("MENUBAR TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
