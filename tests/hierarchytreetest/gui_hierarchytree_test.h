#ifndef GUI_HIERARCHYTREE_TEST_H
#define GUI_HIERARCHYTREE_TEST_H

#include <QObject>

class HierarchyTree;

class TestHierarchyTree : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();


    void testSearchAndFilterExist();
    void testHasTopLevelItems();

    void testAddProfile();
    void testAddFolder();
    void testAddEntity();
    void testAddComponent();
    void testRenameEntity();
    void testRemoveComponent();
    void testRemoveEntity();
    void testRemoveFolder();
    void testRemoveProfile();

    void testSearchFilter();
    void testProfileFilter();
    void testSelection();
    void testDragDropAndContextMenu();
    void testActiveStateStyling();
    void testSubcomponentOperations();

private:
    HierarchyTree* tree = nullptr;
};

#endif
