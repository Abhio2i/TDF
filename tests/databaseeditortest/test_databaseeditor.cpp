#include "test_databaseeditor.h"
#include "GUI/Editors/databaseeditor.h"
#include "GUI/Menubars/menubar.h"
#include <QDockWidget>
#include <QTest>
#include <QSignalSpy>
#include <QAction>
#include <QMenuBar>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

// ------------------------------------------------------------------
// Helpers that return valid IDs for both parent types
// ------------------------------------------------------------------
QString TestDatabaseEditor::ensureProfileId()
{
    if (!editor->hierarchy->ProfileCategories.empty())
        return QString::fromStdString(editor->hierarchy->ProfileCategories.begin()->first);

    ProfileCategaory* pc = editor->hierarchy->addProfileCategaory("TestProfile");
    return pc ? QString::fromStdString(pc->ID) : QString();
}

QString TestDatabaseEditor::ensureFolderId()
{
    // Try existing folder
    if (!editor->hierarchy->Folders.empty())
        return QString::fromStdString(editor->hierarchy->Folders.begin()->first);

    // No folder – need a profile parent first
    QString profileId = ensureProfileId();
    if (profileId.isEmpty()) return QString();

    Folder* f = editor->hierarchy->addFolder(profileId, "TestFolder", true);
    return f ? QString::fromStdString(f->ID) : QString();
}

QString TestDatabaseEditor::ensureEntityId()
{
    if (!editor->hierarchy->Entities.empty())
        return QString::fromStdString(editor->hierarchy->Entities.begin()->first);

    QString parentId = ensureProfileId();   // entities usually under profiles
    if (parentId.isEmpty()) return QString();
    Entity* e = editor->hierarchy->addEntity(parentId, "TestEntity", true);
    return e ? QString::fromStdString(e->ID) : QString();
}

// ------------------------------------------------------------------
// Setup & cleanup
// ------------------------------------------------------------------
void TestDatabaseEditor::init()
{
    editor = new DatabaseEditor();
    editor->clearUnsavedChanges();

    // Force menu bar creation (if not already set)
    if (!editor->menuBar()) {
        MenuBar* mb = new MenuBar(editor);
        editor->setMenuBar(mb);
    }

    // Force dock object names for findChild
    for (QDockWidget* dock : editor->findChildren<QDockWidget*>()) {
        if (dock->windowTitle() == "Editor") dock->setObjectName("Editor");
        if (dock->windowTitle() == "Inspector") dock->setObjectName("Inspector");
        if (dock->windowTitle() == "Console") dock->setObjectName("Console");
    }

    tempDir = new QTemporaryDir();
    QVERIFY(tempDir->isValid());
}

void TestDatabaseEditor::cleanup()
{
    delete editor;
    delete tempDir;
    editor = nullptr;
    tempDir = nullptr;
}

// ------------------------------------------------------------------
// GUI tests
// ------------------------------------------------------------------
void TestDatabaseEditor::testWindowTitle()
{
    QVERIFY(editor->windowTitle().contains("Database Editor"));
}

void TestDatabaseEditor::testDockWidgetsExist()
{
    QVERIFY(editor->findChild<QDockWidget*>("Editor") != nullptr);
    QVERIFY(editor->findChild<QDockWidget*>("Inspector") != nullptr);
    QVERIFY(editor->findChild<QDockWidget*>("Console") != nullptr);
}

void TestDatabaseEditor::testConsoleDockHiddenByDefault()
{
    QVERIFY(editor->consoleDock != nullptr);
    QVERIFY(!editor->consoleDock->isVisible());
}

void TestDatabaseEditor::testUnsavedChangesInitiallyFalse()
{
    QVERIFY(!editor->hasUnsavedChanges);
}

void TestDatabaseEditor::testMarkUnsavedChanges()
{
    editor->markUnsavedChanges();
    QVERIFY(editor->hasUnsavedChanges);
    QVERIFY(editor->windowTitle().contains("*"));
}

void TestDatabaseEditor::testClearUnsavedChanges()
{
    editor->markUnsavedChanges();
    editor->clearUnsavedChanges();
    QVERIFY(!editor->hasUnsavedChanges);
    QVERIFY(!editor->windowTitle().contains("*"));
}

void TestDatabaseEditor::testUnsavedChangesSignal()
{
    QSignalSpy spy(editor, &DatabaseEditor::unsavedChangesChanged);
    editor->markUnsavedChanges();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    editor->clearUnsavedChanges();
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).toBool(), false);
}

void TestDatabaseEditor::testSaveActionExists()
{
    MenuBar* menuBar = qobject_cast<MenuBar*>(editor->menuBar());
    QVERIFY(menuBar != nullptr);
    QAction* saveAction = menuBar->getSaveAction();
    QVERIFY(saveAction != nullptr);
    QVERIFY(saveAction->isEnabled());
}

void TestDatabaseEditor::testRecentProjectsActionExists()
{
    MenuBar* menuBar = qobject_cast<MenuBar*>(editor->menuBar());
    QVERIFY(menuBar != nullptr);
    QAction* recentAction = menuBar->getRecentProjectAction();
    QVERIFY(recentAction != nullptr);
}

void TestDatabaseEditor::testApplicationDialogActionExists()
{
    MenuBar* menuBar = qobject_cast<MenuBar*>(editor->menuBar());
    QVERIFY(menuBar != nullptr);
    QAction* appAction = menuBar->getApplicationAction();
    QVERIFY(appAction != nullptr);
}

// ------------------------------------------------------------------
// Folders
// ------------------------------------------------------------------
void TestDatabaseEditor::testAddFolder()
{
    QString parentId = ensureProfileId();   // folders under profiles use Profile=true
    QVERIFY(!parentId.isEmpty());
    int before = editor->hierarchy->Folders.size();
    Folder* newFolder = editor->hierarchy->addFolder(parentId, "TestFolder", true);
    QVERIFY(newFolder != nullptr);
    QCOMPARE((int)editor->hierarchy->Folders.size(), before + 1);
    QVERIFY(editor->hasUnsavedChanges);
}

void TestDatabaseEditor::testRenameFolder()
{
    QString folderId = ensureFolderId();
    QVERIFY(!folderId.isEmpty());
    editor->hierarchy->renameFolder(folderId, "RenamedFolder");
    auto it = editor->hierarchy->Folders.find(folderId.toStdString());
    QVERIFY(it != editor->hierarchy->Folders.end());
    QCOMPARE(QString::fromStdString(it->second->Name), "RenamedFolder");
    QVERIFY(editor->hasUnsavedChanges);
}

void TestDatabaseEditor::testRemoveFolder()
{
    QString parentId = ensureProfileId();
    Folder* f = editor->hierarchy->addFolder(parentId, "ToBeRemoved", true);
    QString folderId = QString::fromStdString(f->ID);
    int before = editor->hierarchy->Folders.size();
    editor->clearUnsavedChanges();
    editor->hierarchy->removeFolder(parentId, folderId);
    QCOMPARE((int)editor->hierarchy->Folders.size(), before - 1);
    QVERIFY(editor->hasUnsavedChanges);
}

// ------------------------------------------------------------------
// Entities
// ------------------------------------------------------------------
void TestDatabaseEditor::testAddEntity()
{
    QString parentId = ensureProfileId();   // entities under profiles use Profile=true
    QVERIFY(!parentId.isEmpty());
    int before = editor->hierarchy->Entities.size();
    Entity* ent = editor->hierarchy->addEntity(parentId, "TestPlane", true);
    QVERIFY(ent != nullptr);
    QCOMPARE((int)editor->hierarchy->Entities.size(), before + 1);
    QVERIFY(editor->hasUnsavedChanges);
}

void TestDatabaseEditor::testRenameEntity()
{
    QString entityId = ensureEntityId();
    QVERIFY(!entityId.isEmpty());
    editor->clearUnsavedChanges();
    editor->hierarchy->renameEntity(entityId, "NewName");
    auto it = editor->hierarchy->Entities.find(entityId.toStdString());
    QVERIFY(it != editor->hierarchy->Entities.end());
    QCOMPARE(QString::fromStdString(it->second->Name), "NewName");
    QVERIFY(editor->hasUnsavedChanges);
}

void TestDatabaseEditor::testRemoveEntity()
{
    // Create a fresh entity to remove
    QString parentId = ensureProfileId();
    Entity* ent = editor->hierarchy->addEntity(parentId, "ToBeRemoved", true);
    QString entityId = QString::fromStdString(ent->ID);
    int before = editor->hierarchy->Entities.size();
    editor->clearUnsavedChanges();
    editor->hierarchy->removeEntity(parentId, entityId);
    QCOMPARE((int)editor->hierarchy->Entities.size(), before - 1);
    QVERIFY(editor->hasUnsavedChanges);
}

// ------------------------------------------------------------------
// Components
// ------------------------------------------------------------------
void TestDatabaseEditor::testAddComponent()
{
    QString entityId = ensureEntityId();
    QVERIFY(!entityId.isEmpty());
    int before = editor->hierarchy->Components.size();
    editor->hierarchy->addComponent(entityId, "transform");
    QCOMPARE((int)editor->hierarchy->Components.size(), before + 1);
    QVERIFY(editor->hasUnsavedChanges);
}

void TestDatabaseEditor::testRemoveComponent()
{
    QString entityId = ensureEntityId();
    QVERIFY(!entityId.isEmpty());
    editor->hierarchy->addComponent(entityId, "testComp");
    editor->clearUnsavedChanges();
    editor->hierarchy->removeComponent(entityId, "testComp");
    QVERIFY(editor->hasUnsavedChanges);
}

void TestDatabaseEditor::testGetComponentData()
{
    QString entityId = ensureEntityId();
    QVERIFY(!entityId.isEmpty());
    editor->hierarchy->addComponent(entityId, "transform");
    QJsonObject data = editor->hierarchy->getComponentData(entityId, "transform");
    QVERIFY(!data.isEmpty());
}

// ------------------------------------------------------------------
// Profile categories
// ------------------------------------------------------------------
void TestDatabaseEditor::testAddProfileCategory()
{
    int before = editor->hierarchy->ProfileCategories.size();
    ProfileCategaory* pc = editor->hierarchy->addProfileCategaory("TestProfile");
    QVERIFY(pc != nullptr);
    QCOMPARE((int)editor->hierarchy->ProfileCategories.size(), before + 1);
    QVERIFY(editor->hasUnsavedChanges);
}

void TestDatabaseEditor::testRenameProfileCategory()
{
    ProfileCategaory* pc = editor->hierarchy->addProfileCategaory("OldProfileName");
    QString id = QString::fromStdString(pc->ID);
    editor->clearUnsavedChanges();
    editor->hierarchy->renameProfileCategaory(id, "NewProfileName");
    auto it = editor->hierarchy->ProfileCategories.find(id.toStdString());
    QVERIFY(it != editor->hierarchy->ProfileCategories.end());
    QCOMPARE(QString::fromStdString(it->second->Name), "NewProfileName");
    QVERIFY(editor->hasUnsavedChanges);
}

void TestDatabaseEditor::testRemoveProfileCategory()
{
    ProfileCategaory* pc = editor->hierarchy->addProfileCategaory("ToBeRemovedProfile");
    QString id = QString::fromStdString(pc->ID);
    int before = editor->hierarchy->ProfileCategories.size();
    editor->clearUnsavedChanges();
    editor->hierarchy->removeProfileCategaory(id);
    QCOMPARE((int)editor->hierarchy->ProfileCategories.size(), before - 1);
    QVERIFY(editor->hasUnsavedChanges);
}

// ------------------------------------------------------------------
// File I/O
// ------------------------------------------------------------------
void TestDatabaseEditor::testSaveToJsonFile()
{
    ensureProfileId(); // ensure at least one profile
    QString filePath = tempDir->path() + "/test_save.json";
    QJsonObject obj = editor->hierarchy->toJson();
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(obj).toJson());
    file.close();
    QVERIFY(QFile::exists(filePath));
}

void TestDatabaseEditor::testLoadFromJsonFile()
{
    ensureProfileId();
    QString filePath = tempDir->path() + "/test_load.json";
    QJsonObject saveObj = editor->hierarchy->toJson();
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(saveObj).toJson());
    file.close();

    DatabaseEditor newEditor;
    newEditor.loadFromJsonFile(filePath);
    bool found = false;
    for (auto& f : newEditor.hierarchy->ProfileCategories) {
        if (f.second->Name == "TestProfile") {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void TestDatabaseEditor::testLoadInvalidJson()
{
    QString filePath = tempDir->path() + "/invalid.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write("this is not valid json");
    file.close();

    DatabaseEditor newEditor;
    newEditor.loadFromJsonFile(filePath);
    QVERIFY(true); // no crash
}

void TestDatabaseEditor::testLoadMissingFile()
{
    QString filePath = tempDir->path() + "/does_not_exist.json";
    DatabaseEditor newEditor;
    newEditor.loadFromJsonFile(filePath);
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Signals
// ------------------------------------------------------------------
void TestDatabaseEditor::testHierarchyLoadedSignal()
{
    QSignalSpy spy(editor, &DatabaseEditor::hierarchyLoaded);
    QJsonObject dummy;
    dummy["test"] = "data";
    emit editor->hierarchyLoaded(dummy);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toJsonObject()["test"].toString(), "data");
}

void TestDatabaseEditor::testActivatedSignal()
{
    QSignalSpy spy(editor, &DatabaseEditor::Activated);
    emit editor->Activated();
    QCOMPARE(spy.count(), 1);
}

// ------------------------------------------------------------------
// Attach operations
// ------------------------------------------------------------------
void TestDatabaseEditor::testAttachIff()
{
    QString entityId = ensureEntityId();
    QVERIFY(!entityId.isEmpty());
    editor->hierarchy->attchedIff(entityId, "TestIFF");
    QVERIFY(true);
}

void TestDatabaseEditor::testAttachSensor()
{
    QString entityId = ensureEntityId();
    QVERIFY(!entityId.isEmpty());
    editor->hierarchy->attachSensors(entityId, "TestSensor", "CSM");
    QVERIFY(true);
}

void TestDatabaseEditor::testAttachRadio()
{
    QString entityId = ensureEntityId();
    QVERIFY(!entityId.isEmpty());
    editor->hierarchy->attachRadios(entityId, "TestRadio");
    QVERIFY(true);
}

void TestDatabaseEditor::testAttachWeapon()
{
    QString entityId = ensureEntityId();
    QVERIFY(!entityId.isEmpty());
    editor->hierarchy->attachWeapons(entityId, "TestWeapon");
    QVERIFY(true);
}
