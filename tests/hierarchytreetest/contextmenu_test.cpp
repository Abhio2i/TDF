#include "contextmenu_test.h"
#include "GUI/Hierarchytree/contextmenu.h"
#include <QTest>
#include <QTreeWidgetItem>
#include <QVariantMap>
#include <QAction>
#include <QMenu>

// Helper to create dummy tree items
QTreeWidgetItem* TestContextMenu::createDummyItem(const QString& type,
                                                  const QString& name,
                                                  const QString& id,
                                                  const QString& parentId,
                                                  const QString& profileValue)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, name);
    dummyItems.append(item);

    QVariantMap data;
    data["ID"] = id;
    data["parentId"] = parentId;
    data["name"] = name;

    if (type == "profile") {
        QVariantMap typeData;
        typeData["type"] = "option";
        typeData["value"] = profileValue.isEmpty() ? "Platform" : profileValue;
        data["type"] = typeData;
    } else if (type == "folder") {
        data["type"] = "folder";
    } else if (type == "entity") {
        data["type"] = "entity";
    } else if (type == "component") {
        data["type"] = "component";
    } else if (type == "subcomponent") {
        data["type"] = "subcomponent";
    }

    item->setData(0, Qt::UserRole, data);
    return item;
}

void TestContextMenu::deleteDummyItems()
{
    qDeleteAll(dummyItems);
    dummyItems.clear();
}

void TestContextMenu::init()
{
    menu = new ContextMenu(nullptr);
}

void TestContextMenu::cleanup()
{
    delete menu;
    deleteDummyItems();
    menu = nullptr;
}

// ============================================================================
// 1. Profile menu (action existence)
// ============================================================================
void TestContextMenu::testProfileMenu()
{
    QTreeWidgetItem* profileItem = createDummyItem("profile", "TestProfile", "prof1", "", "Platform");
    menu->setupMenu(profileItem);
    QList<QAction*> actions = menu->actions();

    bool hasAddFolder = false, hasAddEntity = false, hasDelete = false, hasPaste = false;
    for (QAction* act : actions) {
        QString text = act->text();
        if (text.contains("Add Folder", Qt::CaseInsensitive)) hasAddFolder = true;
        if (text.contains("Add Entity", Qt::CaseInsensitive) ||
            text.contains("Add Platform", Qt::CaseInsensitive)) hasAddEntity = true;
        if (text.contains("Delete", Qt::CaseInsensitive)) hasDelete = true;
        if (text.contains("Paste", Qt::CaseInsensitive)) hasPaste = true;
    }
    QVERIFY(hasAddFolder);
    QVERIFY(hasAddEntity);
    QVERIFY(hasDelete);
    QVERIFY(hasPaste);
}

// ============================================================================
// 2. Folder menu
// ============================================================================
void TestContextMenu::testFolderMenu()
{
    QTreeWidgetItem* folderItem = createDummyItem("folder", "TestFolder", "folder1", "prof1");
    menu->setupMenu(folderItem);
    QList<QAction*> actions = menu->actions();

    bool hasAddFolder = false, hasAddEntity = false, hasDelete = false, hasPaste = false, hasRename = false;
    for (QAction* act : actions) {
        QString text = act->text();
        if (text.contains("Add Folder", Qt::CaseInsensitive)) hasAddFolder = true;
        if (text.contains("Add Entity", Qt::CaseInsensitive)) hasAddEntity = true;
        if (text.contains("Delete Folder", Qt::CaseInsensitive)) hasDelete = true;
        if (text.contains("Paste", Qt::CaseInsensitive)) hasPaste = true;
        if (text.contains("Rename", Qt::CaseInsensitive)) hasRename = true;
    }
    QVERIFY(hasAddFolder);
    QVERIFY(hasAddEntity);
    QVERIFY(hasDelete);
    QVERIFY(hasPaste);
    QVERIFY(hasRename);
}

// ============================================================================
// 3. Entity menu (basic actions only, no platform detection needed)
// ============================================================================
void TestContextMenu::testEntityMenu()
{
    QTreeWidgetItem* entityItem = createDummyItem("entity", "TestEntity", "ent1", "folder1");
    menu->setupMenu(entityItem);
    QList<QAction*> actions = menu->actions();

    bool hasCopy = false, hasDeleteEntity = false, hasRename = false;
    for (QAction* act : actions) {
        QString text = act->text();
        if (text.contains("Copy", Qt::CaseInsensitive)) hasCopy = true;
        if (text.contains("Delete Entity", Qt::CaseInsensitive)) hasDeleteEntity = true;
        if (text.contains("Rename", Qt::CaseInsensitive)) hasRename = true;
    }
    QVERIFY(hasCopy);
    QVERIFY(hasDeleteEntity);
    QVERIFY(hasRename);
}

// ============================================================================
// 4. Component menu (radios)
// ============================================================================
void TestContextMenu::testComponentMenu()
{
    QTreeWidgetItem* compItem = createDummyItem("component", "radios", "comp1", "ent1");
    menu->setupMenu(compItem);
    QList<QAction*> actions = menu->actions();
    bool hasAdd = false;
    for (QAction* act : actions) {
        if (act->text().contains("Add", Qt::CaseInsensitive)) {
            hasAdd = true;
            break;
        }
    }
    QVERIFY(hasAdd);
}

// ============================================================================
// 5. Weapons component menu
// ============================================================================
void TestContextMenu::testWeaponsComponentMenu()
{
    QTreeWidgetItem* weaponsCompItem = createDummyItem("component", "weapons", "comp2", "ent1");
    menu->setupMenu(weaponsCompItem);
    QList<QAction*> actions = menu->actions();
    bool hasAdd = false;
    for (QAction* act : actions) {
        if (act->text().contains("Add", Qt::CaseInsensitive)) {
            hasAdd = true;
            break;
        }
    }
    QVERIFY(hasAdd);
}

// ============================================================================
// 6. Sub‑component menu
// ============================================================================
void TestContextMenu::testSubComponentMenu()
{
    QTreeWidgetItem* subcompItem = createDummyItem("subcomponent", "SubComp", "sub1", "comp1");
    menu->setupMenu(subcompItem);
    QList<QAction*> actions = menu->actions();
    bool hasRemove = false, hasRename = false;
    for (QAction* act : actions) {
        QString text = act->text();
        if (text.contains("Remove", Qt::CaseInsensitive)) hasRemove = true;
        if (text.contains("Rename", Qt::CaseInsensitive)) hasRename = true;
    }
    QVERIFY(hasRemove);
    QVERIFY(hasRename);
}

// ============================================================================
// 7. Signal existence
// ============================================================================
void TestContextMenu::testSignalsExist()
{
    QVERIFY(menu->inherits("QMenu"));
    const QMetaObject* mo = menu->metaObject();
    QVERIFY(mo->indexOfSignal("addFolderRequested(QString,QString,bool,QVariantMap)") != -1);
    QVERIFY(mo->indexOfSignal("addEntityRequested(QString,QString,bool,QVariantMap,AddItemDialog*,QString,double,double,float)") != -1);
    QVERIFY(mo->indexOfSignal("removeEntityRequested(QString,QString,bool)") != -1);
    QVERIFY(mo->indexOfSignal("copyItemRequested(QVariantMap)") != -1);
}

// ============================================================================
// 8. setHierarchy does not crash
// ============================================================================
void TestContextMenu::testSetHierarchyNoCrash()
{
    menu->setHierarchy(nullptr);
    QVERIFY(true);
}

// ============================================================================
// Additional safe tests (duplicate actions, enabled state, not empty)
// ============================================================================
void TestContextMenu::testProfileMenuHasNoDuplicateActions()
{
    QTreeWidgetItem* profileItem = createDummyItem("profile", "TestProfile", "prof1", "", "Platform");
    menu->setupMenu(profileItem);
    QSet<QString> uniqueTexts;
    for (QAction* act : menu->actions()) {
        uniqueTexts.insert(act->text());
    }
    QCOMPARE(uniqueTexts.size(), menu->actions().size());
}

void TestContextMenu::testFolderMenuHasNoDuplicateActions()
{
    QTreeWidgetItem* folderItem = createDummyItem("folder", "TestFolder", "folder1", "prof1");
    menu->setupMenu(folderItem);
    QSet<QString> uniqueTexts;
    for (QAction* act : menu->actions()) {
        uniqueTexts.insert(act->text());
    }
    QCOMPARE(uniqueTexts.size(), menu->actions().size());
}

void TestContextMenu::testEntityMenuHasNoDuplicateActions()
{
    QTreeWidgetItem* entityItem = createDummyItem("entity", "TestEntity", "ent1", "folder1");
    menu->setupMenu(entityItem);
    QSet<QString> uniqueTexts;
    for (QAction* act : menu->actions()) {
        uniqueTexts.insert(act->text());
    }
    QCOMPARE(uniqueTexts.size(), menu->actions().size());
}

void TestContextMenu::testComponentMenuHasNoDuplicateActions()
{
    QTreeWidgetItem* compItem = createDummyItem("component", "radios", "comp1", "ent1");
    menu->setupMenu(compItem);
    QSet<QString> uniqueTexts;
    for (QAction* act : menu->actions()) {
        uniqueTexts.insert(act->text());
    }
    QCOMPARE(uniqueTexts.size(), menu->actions().size());
}

void TestContextMenu::testWeaponsComponentMenuHasNoDuplicateActions()
{
    QTreeWidgetItem* weaponsCompItem = createDummyItem("component", "weapons", "comp2", "ent1");
    menu->setupMenu(weaponsCompItem);
    QSet<QString> uniqueTexts;
    for (QAction* act : menu->actions()) {
        uniqueTexts.insert(act->text());
    }
    QCOMPARE(uniqueTexts.size(), menu->actions().size());
}

void TestContextMenu::testSubComponentMenuHasNoDuplicateActions()
{
    QTreeWidgetItem* subcompItem = createDummyItem("subcomponent", "SubComp", "sub1", "comp1");
    menu->setupMenu(subcompItem);
    QSet<QString> uniqueTexts;
    for (QAction* act : menu->actions()) {
        uniqueTexts.insert(act->text());
    }
    QCOMPARE(uniqueTexts.size(), menu->actions().size());
}

void TestContextMenu::testProfileMenuActionsAreEnabled()
{
    QTreeWidgetItem* profileItem = createDummyItem("profile", "TestProfile", "prof1", "", "Platform");
    menu->setupMenu(profileItem);
    for (QAction* act : menu->actions()) {
        QVERIFY(act->isEnabled());
    }
}

void TestContextMenu::testFolderMenuActionsAreEnabled()
{
    QTreeWidgetItem* folderItem = createDummyItem("folder", "TestFolder", "folder1", "prof1");
    menu->setupMenu(folderItem);
    for (QAction* act : menu->actions()) {
        QVERIFY(act->isEnabled());
    }
}

void TestContextMenu::testEntityMenuActionsAreEnabled()
{
    QTreeWidgetItem* entityItem = createDummyItem("entity", "TestEntity", "ent1", "folder1");
    menu->setupMenu(entityItem);
    for (QAction* act : menu->actions()) {
        QVERIFY(act->isEnabled());
    }
}

void TestContextMenu::testComponentMenuActionsAreEnabled()
{
    QTreeWidgetItem* compItem = createDummyItem("component", "radios", "comp1", "ent1");
    menu->setupMenu(compItem);
    for (QAction* act : menu->actions()) {
        QVERIFY(act->isEnabled());
    }
}

void TestContextMenu::testWeaponsComponentMenuActionsAreEnabled()
{
    QTreeWidgetItem* weaponsCompItem = createDummyItem("component", "weapons", "comp2", "ent1");
    menu->setupMenu(weaponsCompItem);
    for (QAction* act : menu->actions()) {
        QVERIFY(act->isEnabled());
    }
}

void TestContextMenu::testSubComponentMenuActionsAreEnabled()
{
    QTreeWidgetItem* subcompItem = createDummyItem("subcomponent", "SubComp", "sub1", "comp1");
    menu->setupMenu(subcompItem);
    for (QAction* act : menu->actions()) {
        QVERIFY(act->isEnabled());
    }
}

void TestContextMenu::testProfileMenuNotEmpty()
{
    QTreeWidgetItem* item = createDummyItem("profile", "P", "p", "", "Platform");
    menu->setupMenu(item);
    QVERIFY(!menu->actions().isEmpty());
}

void TestContextMenu::testFolderMenuNotEmpty()
{
    QTreeWidgetItem* item = createDummyItem("folder", "F", "f", "p");
    menu->setupMenu(item);
    QVERIFY(!menu->actions().isEmpty());
}

void TestContextMenu::testEntityMenuNotEmpty()
{
    QTreeWidgetItem* item = createDummyItem("entity", "E", "e", "f");
    menu->setupMenu(item);
    QVERIFY(!menu->actions().isEmpty());
}

void TestContextMenu::testComponentMenuNotEmpty()
{
    QTreeWidgetItem* item = createDummyItem("component", "radios", "c", "e");
    menu->setupMenu(item);
    QVERIFY(!menu->actions().isEmpty());
}

void TestContextMenu::testWeaponsComponentMenuNotEmpty()
{
    QTreeWidgetItem* item = createDummyItem("component", "weapons", "w", "e");
    menu->setupMenu(item);
    QVERIFY(!menu->actions().isEmpty());
}

void TestContextMenu::testSubComponentMenuNotEmpty()
{
    QTreeWidgetItem* item = createDummyItem("subcomponent", "S", "s", "c");
    menu->setupMenu(item);
    QVERIFY(!menu->actions().isEmpty());
}

void TestContextMenu::testNullItemDoesNotCrash()
{
    menu->setupMenu(nullptr);
    QVERIFY(true);
}

void TestContextMenu::testInvalidDataTypeDoesNotCrash()
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    dummyItems.append(item);
    QVariantMap data;
    data["type"] = "invalid_type";
    item->setData(0, Qt::UserRole, data);
    menu->setupMenu(item);
    QVERIFY(true);
}

void TestContextMenu::testMenuClearsBeforeSetup()
{
    QTreeWidgetItem* profileItem = createDummyItem("profile", "TestProfile", "prof1", "", "Platform");
    menu->setupMenu(profileItem);
    int firstCount = menu->actions().size();
    QTreeWidgetItem* folderItem = createDummyItem("folder", "TestFolder", "folder1", "prof1");
    menu->setupMenu(folderItem);
    int secondCount = menu->actions().size();
    QVERIFY(secondCount != firstCount);
}
