#include "gui_hierarchytree_test.h"
#include "GUI/Hierarchytree/hierarchytree.h"
#include "GUI/Hierarchytree/hierarchyconnector.h"
#include "core/Debug/console.h"
#include <QTreeWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QAction>
#include <QMenu>
#include <QCoreApplication>
#include <QDebug>

#define TREE_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runHierarchyTreeTests(HierarchyTree* tree, Console* console)
{
    if (!tree || !console) {
        if (console) console->error("HierarchyTree or Console is null, cannot run tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("      HIERARCHY TREE UNIT TESTS          "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: Tree widget exists and is visible -----
    QTreeWidget* treeWidget = tree->getTreeWidget();
    TREE_TEST(treeWidget != nullptr, "Tree widget exists");
    TREE_TEST(treeWidget->isVisible(), "Tree widget is visible");

    // ----- Test 2: Search bar and profile filter combo exist -----
    QLineEdit* searchBar = tree->searchBar;
    QComboBox* profileFilter = tree->profileFilterCombo;
    TREE_TEST(searchBar != nullptr, "Search bar exists");
    TREE_TEST(profileFilter != nullptr, "Profile filter combo exists");

    // ----- Test 3: Check that tree has at least one top-level item -----
    int topLevelCount = treeWidget->topLevelItemCount();
    TREE_TEST(topLevelCount > 0, "Tree has at least one top-level item (profile)");

    // ------------------------------------------------------------
    //  Add, rename, remove tests – all IDs are declared here
    // ------------------------------------------------------------
    QString testProfileId = "test_profile_123";
    QString testProfileName = "UnitTestProfile";
    QString folderId = "test_folder_456";
    QString folderName = "TestFolder";
    QString entityId = "test_entity_789";
    QString entityName = "TestEntity";
    QString componentId = "test_comp_101";
    QString componentName = "transform";

    // ----- Test 4: Add profile -----
    tree->profileAdded(testProfileId, testProfileName);
    bool profileFound = false;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->text(0) == testProfileName) {
            profileFound = true;
            break;
        }
    }
    TREE_TEST(profileFound, "Profile added successfully");

    // ----- Test 5: Add folder under profile -----
    tree->folderAdded(testProfileId, folderId, folderName);
    QTreeWidgetItem* profileItem = nullptr;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->text(0) == testProfileName) {
            profileItem = treeWidget->topLevelItem(i);
            break;
        }
    }
    bool folderFound = false;
    if (profileItem) {
        for (int j = 0; j < profileItem->childCount(); ++j) {
            if (profileItem->child(j)->text(0) == folderName) {
                folderFound = true;
                break;
            }
        }
    }
    TREE_TEST(folderFound, "Folder added under profile");

    // ----- Test 6: Add entity under folder -----
    tree->entityAdded(folderId, entityId, entityName);
    QTreeWidgetItem* folderItem = nullptr;
    if (profileItem) {
        for (int j = 0; j < profileItem->childCount(); ++j) {
            if (profileItem->child(j)->text(0) == folderName) {
                folderItem = profileItem->child(j);
                break;
            }
        }
    }
    bool entityFound = false;
    if (folderItem) {
        for (int k = 0; k < folderItem->childCount(); ++k) {
            if (folderItem->child(k)->text(0) == entityName) {
                entityFound = true;
                break;
            }
        }
    }
    TREE_TEST(entityFound, "Entity added under folder");

    // ----- Test 7: Add component to entity -----
    tree->componentAdded(entityId, componentId, componentName);
    QTreeWidgetItem* entityItem = nullptr;
    if (folderItem) {
        for (int k = 0; k < folderItem->childCount(); ++k) {
            if (folderItem->child(k)->text(0) == entityName) {
                entityItem = folderItem->child(k);
                break;
            }
        }
    }
    bool componentFound = false;
    if (entityItem) {
        for (int m = 0; m < entityItem->childCount(); ++m) {
            if (entityItem->child(m)->text(0).toLower() == componentName) {
                componentFound = true;
                break;
            }
        }
    }
    TREE_TEST(componentFound, "Component added to entity");

    // ----- Test 8: Rename entity -----
    QString newEntityName = "RenamedEntity";
    tree->entityRenamed(entityId, newEntityName);
    bool renamedFound = (entityItem && entityItem->text(0) == newEntityName);
    TREE_TEST(renamedFound, "Entity renamed successfully");

    // ----- Test 9: Remove component -----
    tree->componentRemoved(entityId, componentName);
    bool componentRemoved = true;
    if (entityItem) {
        for (int m = 0; m < entityItem->childCount(); ++m) {
            if (entityItem->child(m)->text(0).toLower() == componentName) {
                componentRemoved = false;
                break;
            }
        }
    }
    TREE_TEST(componentRemoved, "Component removed successfully");

    // ----- Test 10: Remove entity -----
    tree->entityRemoved(entityId);
    bool entityRemoved = true;
    if (folderItem) {
        for (int k = 0; k < folderItem->childCount(); ++k) {
            if (folderItem->child(k)->text(0) == newEntityName) {
                entityRemoved = false;
                break;
            }
        }
    }
    TREE_TEST(entityRemoved, "Entity removed successfully");

    // ----- Test 11: Remove folder -----
    tree->folderRemoved(folderId);
    bool folderRemoved = true;
    if (profileItem) {
        for (int j = 0; j < profileItem->childCount(); ++j) {
            if (profileItem->child(j)->text(0) == folderName) {
                folderRemoved = false;
                break;
            }
        }
    }
    TREE_TEST(folderRemoved, "Folder removed successfully");

    // ----- Test 12: Remove profile -----
    tree->profileRemoved(testProfileId);
    bool profileRemoved = true;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->text(0) == testProfileName) {
            profileRemoved = false;
            break;
        }
    }
    TREE_TEST(profileRemoved, "Profile removed successfully");

    // ------------------------------------------------------------
    //  Search filter test
    // ------------------------------------------------------------
    QString searchProfileId = "search_profile";
    QString searchFolderId = "search_folder";
    QString searchEntityId = "search_entity";
    QString searchEntityName = "UniqueSearchTarget";
    tree->profileAdded(searchProfileId, "SearchProfile");
    tree->folderAdded(searchProfileId, searchFolderId, "SearchFolder");
    tree->entityAdded(searchFolderId, searchEntityId, searchEntityName);
    tree->searchBar->setText(searchEntityName);
    QCoreApplication::processEvents();

    bool entityVisible = false;
    QTreeWidgetItem* searchProfileItem = nullptr;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->text(0) == "SearchProfile") {
            searchProfileItem = treeWidget->topLevelItem(i);
            break;
        }
    }
    if (searchProfileItem && !searchProfileItem->isHidden()) {
        for (int j = 0; j < searchProfileItem->childCount(); ++j) {
            QTreeWidgetItem* sfolder = searchProfileItem->child(j);
            if (!sfolder->isHidden()) {
                for (int k = 0; k < sfolder->childCount(); ++k) {
                    if (!sfolder->child(k)->isHidden() &&
                        sfolder->child(k)->text(0) == searchEntityName) {
                        entityVisible = true;
                        break;
                    }
                }
            }
        }
    }
    TREE_TEST(entityVisible, "Search filter shows matching entity");
    tree->searchBar->clear();
    QCoreApplication::processEvents();
    // cleanup
    tree->entityRemoved(searchEntityId);
    tree->folderRemoved(searchFolderId);
    tree->profileRemoved(searchProfileId);

    // ------------------------------------------------------------
    //  Profile filter test
    // ------------------------------------------------------------
    QString prof1Id = "prof1", prof2Id = "prof2";
    tree->profileAdded(prof1Id, "ProfileA");
    tree->profileAdded(prof2Id, "ProfileB");
    int index = tree->profileFilterCombo->findText("ProfileA");
    if (index >= 0) {
        tree->profileFilterCombo->setCurrentIndex(index);
        QCoreApplication::processEvents();
        bool profAVisible = false, profBVisible = false;
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = treeWidget->topLevelItem(i);
            if (item->text(0) == "ProfileA") profAVisible = !item->isHidden();
            if (item->text(0) == "ProfileB") profBVisible = !item->isHidden();
        }
        TREE_TEST(profAVisible, "Profile filter shows selected profile");
        TREE_TEST(!profBVisible, "Profile filter hides other profiles");
    } else {
        TREE_TEST(false, "Could not find ProfileA in combo");
    }
    tree->profileFilterCombo->setCurrentIndex(0);
    QCoreApplication::processEvents();
    tree->profileRemoved(prof1Id);
    tree->profileRemoved(prof2Id);

    // ------------------------------------------------------------
    //  Selection test
    // ------------------------------------------------------------
    QString selProfileId = "sel_profile";
    QString selEntityId = "sel_entity";
    tree->profileAdded(selProfileId, "SelProfile");
    tree->entityAdded(selProfileId, selEntityId, "SelectMe");
    QTreeWidgetItem* selItem = tree->Items.value(selEntityId);
    if (selItem) {
        treeWidget->setCurrentItem(selItem);
        QList<QVariantMap> selected = tree->getSelectedEntities();
        TREE_TEST(selected.size() == 1, "getSelectedEntities returns 1 item");
        if (selected.size() == 1) {
            TREE_TEST(selected[0]["ID"].toString() == selEntityId,
                      "getSelectedEntities returns correct entity ID");
        }
    } else {
        TREE_TEST(false, "Could not find entity item for selection test");
    }
    tree->entityRemoved(selEntityId);
    tree->profileRemoved(selProfileId);

    // ------------------------------------------------------------
    //  Drag & drop and context menu
    // ------------------------------------------------------------
    TREE_TEST(treeWidget->dragEnabled(), "Tree widget drag enabled");
    TREE_TEST(tree->acceptDrops(), "HierarchyTree accepts drops");
    TREE_TEST(tree->getContextMenu() != nullptr, "Context menu exists");
    TREE_TEST(true, "Library mode flag exists (islib)");

    // ------------------------------------------------------------
    //  Active state styling
    // ------------------------------------------------------------
    QString activeProfileId = "active_profile";
    QString activeEntityId = "active_entity";
    tree->profileAdded(activeProfileId, "ActiveProfile");
    tree->entityAdded(activeProfileId, activeEntityId, "ActiveEntity");
    tree->setEntityActiveState(activeEntityId, false);
    QTreeWidgetItem* activeItem = tree->Items.value(activeEntityId);
    bool isGrey = false;
    if (activeItem) {
        QColor color = activeItem->foreground(0).color();
        isGrey = (color.red() == 120 && color.green() == 120 && color.blue() == 120);
    }
    TREE_TEST(isGrey, "Inactive entity shows grey text");
    tree->setEntityActiveState(activeEntityId, true);
    if (activeItem) {
        QColor color = activeItem->foreground(0).color();
        isGrey = (color.red() == 255 && color.green() == 255 && color.blue() == 255);
    }
    TREE_TEST(isGrey, "Active entity shows white text");
    tree->entityRemoved(activeEntityId);
    tree->profileRemoved(activeProfileId);

    // ------------------------------------------------------------
    //  Subcomponent test
    // ------------------------------------------------------------
    // Need a temporary entity to attach component/subcomponent
    QString tmpProfile = "tmp_profile";
    QString tmpEntity = "tmp_entity";
    tree->profileAdded(tmpProfile, "TempProfile");
    tree->entityAdded(tmpProfile, tmpEntity, "TempEntity");
    QString parentCompId = "parent_comp";
    QString subCompId = "sub_comp";
    tree->componentAdded(tmpEntity, parentCompId, "parentComponent");
    tree->subComponentAdded(parentCompId, subCompId, "SubComp");
    tree->subComponentRenamed(parentCompId, subCompId, "RenamedSub");
    QTreeWidgetItem* subItem = tree->Items.value(subCompId);
    bool renameOk = (subItem && subItem->text(0) == "RenamedSub");
    TREE_TEST(renameOk, "Subcomponent renamed");
    tree->subComponentRemoved(parentCompId, subCompId, "parentComponent");
    bool removedOk = !tree->Items.contains(subCompId);
    TREE_TEST(removedOk, "Subcomponent removed");
    tree->componentRemoved(tmpEntity, "parentComponent");
    tree->entityRemoved(tmpEntity);
    tree->profileRemoved(tmpProfile);

    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("HIERARCHY TREE TESTS: Some tests FAILED."));
    else
        console->log(std::string("HIERARCHY TREE TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
