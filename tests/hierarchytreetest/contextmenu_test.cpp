#include "contextmenu_test.h"
#include "GUI/Hierarchytree/contextmenu.h"
#include "core/Debug/console.h"
#include <QTreeWidgetItem>
#include <QVariantMap>
#include <QAction>
#include <QMenu>
#include <QDebug>

#define MENU_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    // Helper: create a dummy QTreeWidgetItem with given type and optional name/ID
    static QTreeWidgetItem* createDummyItem(const QString& type,
                    const QString& name = "TestItem",
                    const QString& id = "test_id",
                    const QString& parentId = "",
                    const QString& profileValue = "")
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, name);

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

void runContextMenuTests(ContextMenu* menu, Console* console)
{
    if (!menu || !console) {
        if (console) console->error("ContextMenu or Console is null, cannot run tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("        CONTEXT MENU UNIT TESTS          "));
    console->log(std::string("=========================================\n"));

    // ------------------------------------------------------------------
    // Test 1: Profile menu
    // ------------------------------------------------------------------
    QTreeWidgetItem* profileItem = createDummyItem("profile", "TestProfile", "prof1", "", "Platform");
    menu->setupMenu(profileItem);
    QList<QAction*> profileActions = menu->actions();
    bool hasAddFolder = false, hasAddEntity = false, hasDelete = false, hasPaste = false;
    for (QAction* act : profileActions) {
        QString text = act->text();
        if (text.contains("Add Folder", Qt::CaseInsensitive)) hasAddFolder = true;
        if (text.contains("Add Entity", Qt::CaseInsensitive) ||
            text.contains("Add Platform", Qt::CaseInsensitive)) hasAddEntity = true;
        if (text.contains("Delete", Qt::CaseInsensitive)) hasDelete = true;
        if (text.contains("Paste", Qt::CaseInsensitive)) hasPaste = true;
    }
    MENU_TEST(hasAddFolder, "Profile menu: 'Add Folder' action exists");
    MENU_TEST(hasAddEntity, "Profile menu: 'Add Entity' action exists");
    MENU_TEST(hasDelete,    "Profile menu: 'Delete Profile' action exists");
    MENU_TEST(hasPaste,     "Profile menu: 'Paste' action exists");

    // ------------------------------------------------------------------
    // Test 2: Folder menu
    // ------------------------------------------------------------------
    QTreeWidgetItem* folderItem = createDummyItem("folder", "TestFolder", "folder1", "prof1");
    menu->setupMenu(folderItem);
    QList<QAction*> folderActions = menu->actions();
    hasAddFolder = hasAddEntity = hasDelete = hasPaste = false;
    bool hasRename = false;
    for (QAction* act : folderActions) {
        QString text = act->text();
        if (text.contains("Add Folder", Qt::CaseInsensitive)) hasAddFolder = true;
        if (text.contains("Add Entity", Qt::CaseInsensitive)) hasAddEntity = true;
        if (text.contains("Delete Folder", Qt::CaseInsensitive)) hasDelete = true;
        if (text.contains("Paste", Qt::CaseInsensitive)) hasPaste = true;
        if (text.contains("Rename", Qt::CaseInsensitive)) hasRename = true;
    }
    MENU_TEST(hasAddFolder, "Folder menu: 'Add Folder' action exists");
    MENU_TEST(hasAddEntity, "Folder menu: 'Add Entity' action exists");
    MENU_TEST(hasDelete,    "Folder menu: 'Delete Folder' action exists");
    MENU_TEST(hasPaste,     "Folder menu: 'Paste' action exists");
    MENU_TEST(hasRename,    "Folder menu: 'Rename' action exists");

    // ------------------------------------------------------------------
    // Test 3: Entity menu (Platform entity)
    // ------------------------------------------------------------------
    QTreeWidgetItem* entityItem = createDummyItem("entity", "TestEntity", "ent1", "folder1");
    menu->setupMenu(entityItem);
    QList<QAction*> entityActions = menu->actions();
    bool hasCopy = false, hasDeleteEntity = false;
    hasRename = false;
    bool hasSetActive = false, hasSetInactive = false;
    bool hasAddComponent = false;
    for (QAction* act : entityActions) {
        QString text = act->text();
        if (text.contains("Copy", Qt::CaseInsensitive)) hasCopy = true;
        if (text.contains("Delete Entity", Qt::CaseInsensitive)) hasDeleteEntity = true;
        if (text.contains("Rename", Qt::CaseInsensitive)) hasRename = true;
        if (text.contains("Set Active", Qt::CaseInsensitive)) hasSetActive = true;
        if (text.contains("Set Inactive", Qt::CaseInsensitive)) hasSetInactive = true;
        if (text.contains("Add", Qt::CaseInsensitive)) hasAddComponent = true;
    }
    MENU_TEST(hasCopy,          "Entity menu: 'Copy' action exists");
    MENU_TEST(hasDeleteEntity,  "Entity menu: 'Delete Entity' action exists");
    MENU_TEST(hasRename,        "Entity menu: 'Rename' action exists");
    // These may be present only for Platform entities; test assumes Platform.
    MENU_TEST(hasSetActive,     "Entity menu: 'Set Active' action exists");
    MENU_TEST(hasSetInactive,   "Entity menu: 'Set Inactive' action exists");
    MENU_TEST(hasAddComponent,  "Entity menu: 'Add' submenu exists");

    // ------------------------------------------------------------------
    // Test 4: Component menu (e.g., "radios")
    // ------------------------------------------------------------------
    QTreeWidgetItem* compItem = createDummyItem("component", "radios", "comp1", "ent1");
    menu->setupMenu(compItem);
    QList<QAction*> compActions = menu->actions();
    bool hasAddComponentAction = false;
    for (QAction* act : compActions) {
        if (act->text().contains("Add", Qt::CaseInsensitive)) {
            hasAddComponentAction = true;
            break;
        }
    }
    MENU_TEST(hasAddComponentAction, "Component menu: 'Add' action exists (for radios/sensors/iffs)");

    // ------------------------------------------------------------------
    // Test 5: Weapons component menu (special)
    // ------------------------------------------------------------------
    QTreeWidgetItem* weaponsCompItem = createDummyItem("component", "weapons", "comp2", "ent1");
    menu->setupMenu(weaponsCompItem);
    QList<QAction*> weaponsActions = menu->actions();
    bool hasAddWeapon = false;
    for (QAction* act : weaponsActions) {
        if (act->text().contains("Add", Qt::CaseInsensitive)) {
            hasAddWeapon = true;
            break;
        }
    }
    MENU_TEST(hasAddWeapon, "Weapons component: 'Add' action exists");

    // ------------------------------------------------------------------
    // Test 6: Sub‑component menu
    // ------------------------------------------------------------------
    QTreeWidgetItem* subcompItem = createDummyItem("subcomponent", "SubComp", "sub1", "comp1");
    menu->setupMenu(subcompItem);
    QList<QAction*> subcompActions = menu->actions();
    bool hasRemove = false, hasRenameSub = false;
    for (QAction* act : subcompActions) {
        QString text = act->text();
        if (text.contains("Remove", Qt::CaseInsensitive)) hasRemove = true;
        if (text.contains("Rename", Qt::CaseInsensitive)) hasRenameSub = true;
    }
    MENU_TEST(hasRemove,    "Sub‑component menu: 'Remove' action exists");
    MENU_TEST(hasRenameSub, "Sub‑component menu: 'Rename' action exists");

    // ------------------------------------------------------------------
    // Test 7: Signal connections (basic check that signals exist)
    // ------------------------------------------------------------------
    // We cannot easily test signal emission without a full environment,
    // but we can verify that the signals are defined (compile‑time).
    // For runtime, we check that the menu object has the expected meta‑object.
    MENU_TEST(menu->inherits("QMenu"), "ContextMenu inherits QMenu");
    MENU_TEST(menu->metaObject()->indexOfSignal("addFolderRequested(QString,QString,bool,QVariantMap)") != -1,
              "addFolderRequested signal exists");
    MENU_TEST(menu->metaObject()->indexOfSignal("addEntityRequested(QString,QString,bool,QVariantMap,AddItemDialog*,QString,double,double,float)") != -1,
              "addEntityRequested signal exists");
    MENU_TEST(menu->metaObject()->indexOfSignal("removeEntityRequested(QString,QString,bool)") != -1,
              "removeEntityRequested signal exists");
    MENU_TEST(menu->metaObject()->indexOfSignal("copyItemRequested(QVariantMap)") != -1,
              "copyItemRequested signal exists");

    // ------------------------------------------------------------------
    // Test 8: Hierarchy setter/getter (if needed)
    // ------------------------------------------------------------------
    // We can set a dummy hierarchy pointer (nullptr) – just ensure no crash.
    menu->setHierarchy(nullptr);
    MENU_TEST(true, "setHierarchy does not crash (nullptr)");

    // Final summary
    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("CONTEXT MENU TESTS: Some tests FAILED."));
    else
        console->log(std::string("CONTEXT MENU TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));

    // Cleanup dummy items (they are not parented, so delete manually)
    delete profileItem;
    delete folderItem;
    delete entityItem;
    delete compItem;
    delete weaponsCompItem;
    delete subcompItem;
}
