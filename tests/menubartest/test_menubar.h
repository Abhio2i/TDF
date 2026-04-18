#ifndef TEST_MENUBAR_H
#define TEST_MENUBAR_H

#include <QObject>

class MenuBar;

class TestMenuBar : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Basic existence tests
    void testFileMenuExists();
    void testAllActionsCreated();

    // Visibility tests
    void testSetLibraryActionsVisible();
    void testUpdateFileMenuForDatabase();
    void testUpdateFileMenuForScenario();
    void testUpdateFileMenuForMission();
    void testUpdateFileMenuForAnalysis();

    // Signal emission tests (using QSignalSpy)
    void testNewFileSignal();
    void testRecentProjectSignal();
    void testRecentLibrarySignal();
    void testLoadSignal();
    void testLoadXmlSignal();
    void testLoadToLibrarySignal();
    void testOpenRuntimeInstanceSignal();
    void testOpenMissionFileSignal();
    void testSaveSignal();
    void testSameSaveSignal();
    void testExitSignal();
    void testUndoSignal();
    void testRedoSignal();
    void testCutCopyPasteSignals();
    void testProfileSignal();
    void testApplicationSignal();

private:
    MenuBar* menuBar = nullptr;
};

#endif
