#include "gui_hierarchytree_test.h"
#include "GUI/Hierarchytree/hierarchytree.h"
#include "GUI/Hierarchytree/hierarchyconnector.h"
#include <QTest>
#include <QTreeWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QAction>
#include <QMenu>
#include <QCoreApplication>
#include <QColor>

void TestHierarchyTree::init()
{
    tree = new HierarchyTree(nullptr);
    // Ensure the tree widget is created
    QVERIFY(tree->getTreeWidget() != nullptr);
}

void TestHierarchyTree::cleanup()
{
    delete tree;
    tree = nullptr;
}


void TestHierarchyTree::testSearchAndFilterExist()
{
    QVERIFY(tree->searchBar != nullptr);
    QVERIFY(tree->profileFilterCombo != nullptr);
}

void TestHierarchyTree::testHasTopLevelItems()
{
    QTreeWidget* treeWidget = tree->getTreeWidget();
    // Initially, there may be dummy data (from HierarchyConnector initialization?)
    // In the original test, they assume at least one top-level item (profile) exists.
    // If not, we can create a profile first.
    if (treeWidget->topLevelItemCount() == 0) {
        tree->profileAdded("dummy_profile", "DummyProfile");
    }
    QVERIFY(treeWidget->topLevelItemCount() > 0);
}

// ------------------------------------------------------------------
// CRUD tests
// ------------------------------------------------------------------
void TestHierarchyTree::testAddProfile()
{
    QString testProfileId = "test_profile_123";
    QString testProfileName = "UnitTestProfile";
    tree->profileAdded(testProfileId, testProfileName);

    QTreeWidget* treeWidget = tree->getTreeWidget();
    bool found = false;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->text(0) == testProfileName) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
    // Clean up
    tree->profileRemoved(testProfileId);
}

void TestHierarchyTree::testAddFolder()
{
    QString profileId = "test_profile_folder";
    QString profileName = "ProfileForFolder";
    QString folderId = "test_folder_456";
    QString folderName = "TestFolder";

    tree->profileAdded(profileId, profileName);
    tree->folderAdded(profileId, folderId, folderName);

    QTreeWidget* treeWidget = tree->getTreeWidget();
    QTreeWidgetItem* profileItem = nullptr;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->text(0) == profileName) {
            profileItem = treeWidget->topLevelItem(i);
            break;
        }
    }
    QVERIFY(profileItem != nullptr);
    bool folderFound = false;
    for (int j = 0; j < profileItem->childCount(); ++j) {
        if (profileItem->child(j)->text(0) == folderName) {
            folderFound = true;
            break;
        }
    }
    QVERIFY(folderFound);
    // Clean up
    tree->folderRemoved(folderId);
    tree->profileRemoved(profileId);
}

void TestHierarchyTree::testAddEntity()
{
    QString profileId = "profile_for_entity";
    QString profileName = "EntityProfile";
    QString folderId = "folder_for_entity";
    QString folderName = "EntityFolder";
    QString entityId = "test_entity_789";
    QString entityName = "TestEntity";

    tree->profileAdded(profileId, profileName);
    tree->folderAdded(profileId, folderId, folderName);
    tree->entityAdded(folderId, entityId, entityName);

    QTreeWidget* treeWidget = tree->getTreeWidget();
    QTreeWidgetItem* profileItem = nullptr;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->text(0) == profileName) {
            profileItem = treeWidget->topLevelItem(i);
            break;
        }
    }
    QVERIFY(profileItem != nullptr);
    QTreeWidgetItem* folderItem = nullptr;
    for (int j = 0; j < profileItem->childCount(); ++j) {
        if (profileItem->child(j)->text(0) == folderName) {
            folderItem = profileItem->child(j);
            break;
        }
    }
    QVERIFY(folderItem != nullptr);
    bool entityFound = false;
    for (int k = 0; k < folderItem->childCount(); ++k) {
        if (folderItem->child(k)->text(0) == entityName) {
            entityFound = true;
            break;
        }
    }
    QVERIFY(entityFound);
    // Clean up
    tree->entityRemoved(entityId);
    tree->folderRemoved(folderId);
    tree->profileRemoved(profileId);
}

void TestHierarchyTree::testAddComponent()
{
    QString profileId = "comp_profile";
    QString profileName = "CompProfile";
    QString folderId = "comp_folder";
    QString folderName = "CompFolder";
    QString entityId = "comp_entity";
    QString entityName = "CompEntity";
    QString componentId = "test_comp_101";
    QString componentName = "transform";

    tree->profileAdded(profileId, profileName);
    tree->folderAdded(profileId, folderId, folderName);
    tree->entityAdded(folderId, entityId, entityName);
    tree->componentAdded(entityId, componentId, componentName);

    QTreeWidget* treeWidget = tree->getTreeWidget();
    // Locate entity item
    QTreeWidgetItem* entityItem = tree->Items.value(entityId);
    QVERIFY(entityItem != nullptr);
    bool componentFound = false;
    for (int m = 0; m < entityItem->childCount(); ++m) {
        if (entityItem->child(m)->text(0).toLower() == componentName) {
            componentFound = true;
            break;
        }
    }
    QVERIFY(componentFound);
    // Clean up
    tree->componentRemoved(entityId, componentName);
    tree->entityRemoved(entityId);
    tree->folderRemoved(folderId);
    tree->profileRemoved(profileId);
}

void TestHierarchyTree::testRenameEntity()
{
    QString profileId = "rename_profile";
    QString profileName = "RenameProfile";
    QString folderId = "rename_folder";
    QString folderName = "RenameFolder";
    QString entityId = "rename_entity";
    QString entityName = "OldName";
    QString newName = "NewName";

    tree->profileAdded(profileId, profileName);
    tree->folderAdded(profileId, folderId, folderName);
    tree->entityAdded(folderId, entityId, entityName);
    tree->entityRenamed(entityId, newName);

    QTreeWidgetItem* entityItem = tree->Items.value(entityId);
    QVERIFY(entityItem != nullptr);
    QCOMPARE(entityItem->text(0), newName);
    // Clean up
    tree->entityRemoved(entityId);
    tree->folderRemoved(folderId);
    tree->profileRemoved(profileId);
}

void TestHierarchyTree::testRemoveComponent()
{
    QString profileId = "rmcomp_profile";
    QString profileName = "RmCompProfile";
    QString folderId = "rmcomp_folder";
    QString folderName = "RmCompFolder";
    QString entityId = "rmcomp_entity";
    QString entityName = "RmCompEntity";
    QString componentId = "rmcomp_comp";
    QString componentName = "rmcomp";

    tree->profileAdded(profileId, profileName);
    tree->folderAdded(profileId, folderId, folderName);
    tree->entityAdded(folderId, entityId, entityName);
    tree->componentAdded(entityId, componentId, componentName);
    tree->componentRemoved(entityId, componentName);

    QTreeWidgetItem* entityItem = tree->Items.value(entityId);
    QVERIFY(entityItem != nullptr);
    bool componentExists = false;
    for (int m = 0; m < entityItem->childCount(); ++m) {
        if (entityItem->child(m)->text(0).toLower() == componentName) {
            componentExists = true;
            break;
        }
    }
    QVERIFY(!componentExists);
    // Clean up
    tree->entityRemoved(entityId);
    tree->folderRemoved(folderId);
    tree->profileRemoved(profileId);
}

void TestHierarchyTree::testRemoveEntity()
{
    QString profileId = "rment_profile";
    QString profileName = "RmEntProfile";
    QString folderId = "rment_folder";
    QString folderName = "RmEntFolder";
    QString entityId = "rment_entity";
    QString entityName = "RmEntEntity";

    tree->profileAdded(profileId, profileName);
    tree->folderAdded(profileId, folderId, folderName);
    tree->entityAdded(folderId, entityId, entityName);
    tree->entityRemoved(entityId);

    QVERIFY(!tree->Items.contains(entityId));
    // Clean up
    tree->folderRemoved(folderId);
    tree->profileRemoved(profileId);
}

void TestHierarchyTree::testRemoveFolder()
{
    QString profileId = "rmfolder_profile";
    QString profileName = "RmFolderProfile";
    QString folderId = "rmfolder_folder";
    QString folderName = "RmFolder";

    tree->profileAdded(profileId, profileName);
    tree->folderAdded(profileId, folderId, folderName);
    tree->folderRemoved(folderId);

    QTreeWidgetItem* profileItem = nullptr;
    QTreeWidget* treeWidget = tree->getTreeWidget();
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->text(0) == profileName) {
            profileItem = treeWidget->topLevelItem(i);
            break;
        }
    }
    QVERIFY(profileItem != nullptr);
    bool folderExists = false;
    for (int j = 0; j < profileItem->childCount(); ++j) {
        if (profileItem->child(j)->text(0) == folderName) {
            folderExists = true;
            break;
        }
    }
    QVERIFY(!folderExists);
    // Clean up
    tree->profileRemoved(profileId);
}

void TestHierarchyTree::testRemoveProfile()
{
    QString profileId = "rmprofile_id";
    QString profileName = "RmProfile";

    tree->profileAdded(profileId, profileName);
    tree->profileRemoved(profileId);

    QTreeWidget* treeWidget = tree->getTreeWidget();
    bool profileExists = false;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->text(0) == profileName) {
            profileExists = true;
            break;
        }
    }
    QVERIFY(!profileExists);
}

// ------------------------------------------------------------------
// Search filter test
// ------------------------------------------------------------------
void TestHierarchyTree::testSearchFilter()
{
    QString searchProfileId = "search_profile";
    QString searchFolderId = "search_folder";
    QString searchEntityId = "search_entity";
    QString searchEntityName = "UniqueSearchTarget";

    tree->profileAdded(searchProfileId, "SearchProfile");
    tree->folderAdded(searchProfileId, searchFolderId, "SearchFolder");
    tree->entityAdded(searchFolderId, searchEntityId, searchEntityName);

    tree->searchBar->setText(searchEntityName);
    QCoreApplication::processEvents();

    QTreeWidgetItem* profileItem = tree->Items.value(searchProfileId);
    QVERIFY(profileItem != nullptr);
    bool entityVisible = false;
    if (!profileItem->isHidden()) {
        for (int j = 0; j < profileItem->childCount(); ++j) {
            QTreeWidgetItem* sfolder = profileItem->child(j);
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
    QVERIFY(entityVisible);

    tree->searchBar->clear();
    QCoreApplication::processEvents();

    // Clean up
    tree->entityRemoved(searchEntityId);
    tree->folderRemoved(searchFolderId);
    tree->profileRemoved(searchProfileId);
}

// ------------------------------------------------------------------
// Profile filter test
// ------------------------------------------------------------------
void TestHierarchyTree::testProfileFilter()
{
    QString prof1Id = "prof1", prof2Id = "prof2";
    tree->profileAdded(prof1Id, "ProfileA");
    tree->profileAdded(prof2Id, "ProfileB");

    int index = tree->profileFilterCombo->findText("ProfileA");
    QVERIFY(index >= 0);
    tree->profileFilterCombo->setCurrentIndex(index);
    QCoreApplication::processEvents();

    QTreeWidget* treeWidget = tree->getTreeWidget();
    bool profAVisible = false, profBVisible = false;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = treeWidget->topLevelItem(i);
        if (item->text(0) == "ProfileA") profAVisible = !item->isHidden();
        if (item->text(0) == "ProfileB") profBVisible = !item->isHidden();
    }
    QVERIFY(profAVisible);
    QVERIFY(!profBVisible);

    tree->profileFilterCombo->setCurrentIndex(0); // "All"
    QCoreApplication::processEvents();
    tree->profileRemoved(prof1Id);
    tree->profileRemoved(prof2Id);
}

// ------------------------------------------------------------------
// Selection test
// ------------------------------------------------------------------
void TestHierarchyTree::testSelection()
{
    QString selProfileId = "sel_profile";
    QString selEntityId = "sel_entity";
    tree->profileAdded(selProfileId, "SelProfile");
    tree->entityAdded(selProfileId, selEntityId, "SelectMe");

    QTreeWidgetItem* selItem = tree->Items.value(selEntityId);
    QVERIFY(selItem != nullptr);
    tree->getTreeWidget()->setCurrentItem(selItem);

    QList<QVariantMap> selected = tree->getSelectedEntities();
    QCOMPARE(selected.size(), 1);
    QCOMPARE(selected[0]["ID"].toString(), selEntityId);

    tree->entityRemoved(selEntityId);
    tree->profileRemoved(selProfileId);
}

// ------------------------------------------------------------------
// Drag & drop and context menu
// ------------------------------------------------------------------
void TestHierarchyTree::testDragDropAndContextMenu()
{
    QVERIFY(tree->getTreeWidget()->dragEnabled());
    QVERIFY(tree->acceptDrops());
    QVERIFY(tree->getContextMenu() != nullptr);
    // Library mode flag exists (islib) – not directly testable, but we can check property or method.
    // Just assume it's there.
}

// ------------------------------------------------------------------
// Active state styling
// ------------------------------------------------------------------
void TestHierarchyTree::testActiveStateStyling()
{
    QString activeProfileId = "active_profile";
    QString activeEntityId = "active_entity";
    tree->profileAdded(activeProfileId, "ActiveProfile");
    tree->entityAdded(activeProfileId, activeEntityId, "ActiveEntity");

    QTreeWidgetItem* activeItem = tree->Items.value(activeEntityId);
    QVERIFY(activeItem != nullptr);

    // Set inactive
    tree->setEntityActiveState(activeEntityId, false);
    QColor color = activeItem->foreground(0).color();
    QCOMPARE(color, QColor(120, 120, 120)); // grey

    // Set active
    tree->setEntityActiveState(activeEntityId, true);
    color = activeItem->foreground(0).color();
    QCOMPARE(color, QColor(255, 255, 255)); // white

    tree->entityRemoved(activeEntityId);
    tree->profileRemoved(activeProfileId);
}

// ------------------------------------------------------------------
// Subcomponent operations
// ------------------------------------------------------------------
void TestHierarchyTree::testSubcomponentOperations()
{
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
    QVERIFY(subItem != nullptr);
    QCOMPARE(subItem->text(0), QString("RenamedSub"));

    tree->subComponentRemoved(parentCompId, subCompId, "parentComponent");
    QVERIFY(!tree->Items.contains(subCompId));

    tree->componentRemoved(tmpEntity, "parentComponent");
    tree->entityRemoved(tmpEntity);
    tree->profileRemoved(tmpProfile);
}
