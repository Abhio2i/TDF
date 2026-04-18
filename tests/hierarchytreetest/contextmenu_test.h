#ifndef CONTEXTMENU_TEST_H
#define CONTEXTMENU_TEST_H

#include <QObject>

class ContextMenu;
class QTreeWidgetItem;

class TestContextMenu : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Existing 8 tests
    void testProfileMenu();
    void testFolderMenu();
    void testEntityMenu();
    void testComponentMenu();
    void testWeaponsComponentMenu();
    void testSubComponentMenu();
    void testSignalsExist();
    void testSetHierarchyNoCrash();

    // Additional safe tests (to reach 30+)
    void testProfileMenuHasNoDuplicateActions();
    void testFolderMenuHasNoDuplicateActions();
    void testEntityMenuHasNoDuplicateActions();
    void testComponentMenuHasNoDuplicateActions();
    void testWeaponsComponentMenuHasNoDuplicateActions();
    void testSubComponentMenuHasNoDuplicateActions();

    void testProfileMenuActionsAreEnabled();
    void testFolderMenuActionsAreEnabled();
    void testEntityMenuActionsAreEnabled();
    void testComponentMenuActionsAreEnabled();
    void testWeaponsComponentMenuActionsAreEnabled();
    void testSubComponentMenuActionsAreEnabled();

    void testProfileMenuNotEmpty();
    void testFolderMenuNotEmpty();
    void testEntityMenuNotEmpty();
    void testComponentMenuNotEmpty();
    void testWeaponsComponentMenuNotEmpty();
    void testSubComponentMenuNotEmpty();

    void testNullItemDoesNotCrash();
    void testInvalidDataTypeDoesNotCrash();
    void testMenuClearsBeforeSetup();

private:
    ContextMenu* menu = nullptr;
    QTreeWidgetItem* createDummyItem(const QString& type,
                                     const QString& name = "TestItem",
                                     const QString& id = "test_id",
                                     const QString& parentId = "",
                                     const QString& profileValue = "");
    void deleteDummyItems();
    QList<QTreeWidgetItem*> dummyItems;
};

#endif
