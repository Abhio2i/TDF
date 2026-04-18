#include "test_menubar.h"
#include "GUI/Menubars/menubar.h"
#include <QTest>
#include <QSignalSpy>
#include <QMenu>

void TestMenuBar::init()
{
    menuBar = new MenuBar();
}

void TestMenuBar::cleanup()
{
    delete menuBar;
    menuBar = nullptr;
}

// ------------------------------------------------------------------
// Basic existence
// ------------------------------------------------------------------
void TestMenuBar::testFileMenuExists()
{
    QVERIFY(menuBar->getFileMenu() != nullptr);
}

void TestMenuBar::testAllActionsCreated()
{
    QVERIFY(menuBar->getNewFileAction() != nullptr);
    QVERIFY(menuBar->getRecentProjectAction() != nullptr);
    QVERIFY(menuBar->getrecentProjectLibraryAction() != nullptr);
    QVERIFY(menuBar->getLoadAction() != nullptr);
    QVERIFY(menuBar->getLoadXmlAction() != nullptr);
    QVERIFY(menuBar->getLoadToLibraryAction() != nullptr);
    QVERIFY(menuBar->getOpenRuntimeInstanceAction() != nullptr);
    QVERIFY(menuBar->getOpenMissionFileAction() != nullptr);
    QVERIFY(menuBar->getSameSaveAction() != nullptr);
    QVERIFY(menuBar->getSaveAction() != nullptr);
    QVERIFY(menuBar->getExitAction() != nullptr);
    QVERIFY(menuBar->getUndoAction() != nullptr);
    QVERIFY(menuBar->getRedoAction() != nullptr);
    QVERIFY(menuBar->getSelectAllAction() != nullptr);
    QVERIFY(menuBar->getDeselectAllAction() != nullptr);
    QVERIFY(menuBar->getCutAction() != nullptr);
    QVERIFY(menuBar->getCopyAction() != nullptr);
    QVERIFY(menuBar->getPasteAction() != nullptr);
    QVERIFY(menuBar->getDuplicateAction() != nullptr);
    QVERIFY(menuBar->getRenameAction() != nullptr);
    QVERIFY(menuBar->getDeleteAction() != nullptr);
    QVERIFY(menuBar->getFeedbackAction() != nullptr);
    QVERIFY(menuBar->getProfileAction() != nullptr);
    QVERIFY(menuBar->getApplicationAction() != nullptr);
}

// ------------------------------------------------------------------
// Visibility tests
// ------------------------------------------------------------------
void TestMenuBar::testSetLibraryActionsVisible()
{
    // Initially visible after construction
    QVERIFY(menuBar->getrecentProjectLibraryAction()->isVisible());
    QVERIFY(menuBar->getLoadToLibraryAction()->isVisible());
    QVERIFY(menuBar->getOpenRuntimeInstanceAction()->isVisible());

    // Hide
    menuBar->setLibraryActionsVisible(false);
    QVERIFY(!menuBar->getrecentProjectLibraryAction()->isVisible());
    QVERIFY(!menuBar->getLoadToLibraryAction()->isVisible());
    QVERIFY(!menuBar->getOpenRuntimeInstanceAction()->isVisible());

    // Show again
    menuBar->setLibraryActionsVisible(true);
    QVERIFY(menuBar->getrecentProjectLibraryAction()->isVisible());
    QVERIFY(menuBar->getLoadToLibraryAction()->isVisible());
    QVERIFY(menuBar->getOpenRuntimeInstanceAction()->isVisible());
}

void TestMenuBar::testUpdateFileMenuForDatabase()
{
    menuBar->updateFileMenuForEditor("database");
    QVERIFY(!menuBar->getOpenMissionFileAction()->isVisible());
    QVERIFY(menuBar->getNewFileAction()->isVisible());
    QVERIFY(menuBar->getLoadXmlAction()->isVisible());
    QVERIFY(menuBar->getLoadAction()->isVisible());
    QVERIFY(menuBar->getSameSaveAction()->isVisible());
    QVERIFY(menuBar->getSaveAction()->isVisible());
}

void TestMenuBar::testUpdateFileMenuForScenario()
{
    menuBar->updateFileMenuForEditor("scenario");
    QVERIFY(!menuBar->getOpenMissionFileAction()->isVisible());
    QVERIFY(menuBar->getNewFileAction()->isVisible());
    QVERIFY(menuBar->getLoadXmlAction()->isVisible());
}

void TestMenuBar::testUpdateFileMenuForMission()
{
    menuBar->updateFileMenuForEditor("mission");
    QVERIFY(!menuBar->getNewFileAction()->isVisible());
    QVERIFY(!menuBar->getLoadXmlAction()->isVisible());
    QVERIFY(!menuBar->getOpenMissionFileAction()->isVisible());
    QVERIFY(menuBar->getLoadAction()->isVisible());
    QVERIFY(menuBar->getSaveAction()->isVisible());
}

void TestMenuBar::testUpdateFileMenuForAnalysis()
{
    menuBar->updateFileMenuForEditor("analysis");
    QVERIFY(!menuBar->getNewFileAction()->isVisible());
    QVERIFY(!menuBar->getRecentProjectAction()->isVisible());
    QVERIFY(!menuBar->getLoadAction()->isVisible());
    QVERIFY(!menuBar->getLoadXmlAction()->isVisible());
    QVERIFY(!menuBar->getLoadToLibraryAction()->isVisible());
    QVERIFY(!menuBar->getOpenRuntimeInstanceAction()->isVisible());
    QVERIFY(!menuBar->getOpenMissionFileAction()->isVisible());
    QVERIFY(!menuBar->getSameSaveAction()->isVisible());
    QVERIFY(!menuBar->getSaveAction()->isVisible());
    QVERIFY(menuBar->getExitAction()->isVisible());
}

// ------------------------------------------------------------------
// Signal tests
// ------------------------------------------------------------------
void TestMenuBar::testNewFileSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::newFileTriggered);
    menuBar->getNewFileAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testRecentProjectSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::recentProjectTriggered);
    menuBar->getRecentProjectAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testRecentLibrarySignal()
{
    QSignalSpy spy(menuBar, &MenuBar::recentProjectLibraryTriggered);
    menuBar->getrecentProjectLibraryAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testLoadSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::loadTriggered);
    menuBar->getLoadAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testLoadXmlSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::loadXmlTriggered);
    menuBar->getLoadXmlAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testLoadToLibrarySignal()
{
    QSignalSpy spy(menuBar, &MenuBar::loadToLibraryTriggered);
    menuBar->getLoadToLibraryAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testOpenRuntimeInstanceSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::openRuntimeInstanceTriggered);
    menuBar->getOpenRuntimeInstanceAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testOpenMissionFileSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::openMissionFileTriggered);
    menuBar->getOpenMissionFileAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testSaveSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::saveTriggered);
    menuBar->getSaveAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testSameSaveSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::sameSaveTriggered);
    menuBar->getSameSaveAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testExitSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::exitTriggered);
    menuBar->getExitAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testUndoSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::undoTriggered);
    menuBar->getUndoAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testRedoSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::redoTriggered);
    menuBar->getRedoAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testCutCopyPasteSignals()
{
    QSignalSpy cutSpy(menuBar, &MenuBar::cutTriggered);
    QSignalSpy copySpy(menuBar, &MenuBar::copyTriggered);
    QSignalSpy pasteSpy(menuBar, &MenuBar::pasteTriggered);
    menuBar->getCutAction()->trigger();
    menuBar->getCopyAction()->trigger();
    menuBar->getPasteAction()->trigger();
    QCOMPARE(cutSpy.count(), 1);
    QCOMPARE(copySpy.count(), 1);
    QCOMPARE(pasteSpy.count(), 1);
}

void TestMenuBar::testProfileSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::profileTriggered);
    menuBar->getProfileAction()->trigger();
    QCOMPARE(spy.count(), 1);
}

void TestMenuBar::testApplicationSignal()
{
    QSignalSpy spy(menuBar, &MenuBar::applicationTriggered);
    menuBar->getApplicationAction()->trigger();
    QCOMPARE(spy.count(), 1);
}
