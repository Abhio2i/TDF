#ifndef RECENTPROJECTSMANAGER_TEST_H
#define RECENTPROJECTSMANAGER_TEST_H

#include <QObject>

class RecentProjectsManager;

class TestRecentProjectsManager : public QObject
{
    Q_OBJECT

private slots:
    void testSingletonInstance();
    void testMaxProjectsAndOrder();
    void testDuplicateHandling();
    void testClearProjects();
    void testSeparateListsForDifferentEditors();
    void testUiMethodsExist();

private:
    RecentProjectsManager* manager = nullptr;
};

#endif
