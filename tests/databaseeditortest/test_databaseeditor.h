#ifndef TEST_DATABASEEDITOR_H
#define TEST_DATABASEEDITOR_H

#include <QObject>
#include <QTemporaryDir>

class DatabaseEditor;

class TestDatabaseEditor : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // GUI tests
    void testWindowTitle();
    void testDockWidgetsExist();
    void testConsoleDockHiddenByDefault();
    void testUnsavedChangesInitiallyFalse();
    void testMarkUnsavedChanges();
    void testClearUnsavedChanges();
    void testUnsavedChangesSignal();
    void testSaveActionExists();
    void testRecentProjectsActionExists();
    void testApplicationDialogActionExists();

    // Folders
    void testAddFolder();
    void testRenameFolder();
    void testRemoveFolder();

    // Entities
    void testAddEntity();
    void testRenameEntity();
    void testRemoveEntity();

    // Components
    void testAddComponent();
    void testRemoveComponent();
    void testGetComponentData();

    // Profile categories
    void testAddProfileCategory();
    void testRenameProfileCategory();
    void testRemoveProfileCategory();

    // File I/O
    void testSaveToJsonFile();
    void testLoadFromJsonFile();
    void testLoadInvalidJson();
    void testLoadMissingFile();

    // Signals
    void testHierarchyLoadedSignal();
    void testActivatedSignal();

    // Attach operations
    void testAttachIff();
    void testAttachSensor();
    void testAttachRadio();
    void testAttachWeapon();

private:
    DatabaseEditor* editor = nullptr;
    QTemporaryDir* tempDir = nullptr;

    // Helper: ensure at least one folder exists
    QString ensureFolderExists();
    QString ensureEntityExists();

    // Helper methods
    QString ensureProfileId();
    QString ensureFolderId();
    QString ensureEntityId();
};

#endif
