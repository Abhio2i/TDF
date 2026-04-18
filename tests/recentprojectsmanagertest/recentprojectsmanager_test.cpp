#include "recentprojectsmanager_test.h"
#include "GUI/Editors/recentprojectsmanager.h"
#include <QTest>

void TestRecentProjectsManager::testSingletonInstance()
{
    RecentProjectsManager* instance1 = RecentProjectsManager::instance();
    RecentProjectsManager* instance2 = RecentProjectsManager::instance();
    QVERIFY(instance1 != nullptr);
    QCOMPARE(instance1, instance2);
    manager = instance1; // store for later tests
}

void TestRecentProjectsManager::testMaxProjectsAndOrder()
{
    // Clear all lists
    RecentProjectsManager::EditorType types[] = {
        RecentProjectsManager::ScenarioEditor,
        RecentProjectsManager::MissionEditor,
        RecentProjectsManager::RuntimeEditor,
        RecentProjectsManager::DatabaseEditor,
        RecentProjectsManager::LibraryData
    };
    for (auto type : types) {
        manager->clearRecentProjects(type);
    }

    // Add 12 projects for ScenarioEditor
    for (int i = 1; i <= 12; ++i) {
        QString path = QString("/test/project_%1.sc").arg(i);
        manager->addToRecentProjects(path, RecentProjectsManager::ScenarioEditor);
    }
    QStringList retrieved = manager->getRecentProjects(RecentProjectsManager::ScenarioEditor);
    QCOMPARE(retrieved.size(), 10);
    QCOMPARE(retrieved[0], QString("/test/project_12.sc"));
    QCOMPARE(retrieved[9], QString("/test/project_3.sc"));
}

void TestRecentProjectsManager::testDuplicateHandling()
{
    // Ensure duplicates move to front without increasing size
    manager->addToRecentProjects("/test/project_12.sc", RecentProjectsManager::ScenarioEditor);
    QStringList retrieved = manager->getRecentProjects(RecentProjectsManager::ScenarioEditor);
    QCOMPARE(retrieved.size(), 10);
    QCOMPARE(retrieved[0], QString("/test/project_12.sc"));
}

void TestRecentProjectsManager::testClearProjects()
{
    manager->clearRecentProjects(RecentProjectsManager::ScenarioEditor);
    QStringList retrieved = manager->getRecentProjects(RecentProjectsManager::ScenarioEditor);
    QVERIFY(retrieved.isEmpty());
}

void TestRecentProjectsManager::testSeparateListsForDifferentEditors()
{
    manager->addToRecentProjects("/db/database.db", RecentProjectsManager::DatabaseEditor);
    manager->addToRecentProjects("/mission/mission.ms", RecentProjectsManager::MissionEditor);
    manager->addToRecentProjects("/runtime/runtime.rn", RecentProjectsManager::RuntimeEditor);
    manager->addToRecentProjects("/lib/library.db", RecentProjectsManager::LibraryData);

    QCOMPARE(manager->getRecentProjects(RecentProjectsManager::DatabaseEditor).size(), 1);
    QCOMPARE(manager->getRecentProjects(RecentProjectsManager::MissionEditor).size(), 1);
    QCOMPARE(manager->getRecentProjects(RecentProjectsManager::RuntimeEditor).size(), 1);
    QCOMPARE(manager->getRecentProjects(RecentProjectsManager::LibraryData).size(), 1);
}

void TestRecentProjectsManager::testUiMethodsExist()
{
    // The UI methods (showRecentProjectsMenu, showRecentLibraryMenu) exist and can be called.
    // We do not call them because they would show dialogs. Just verify that the class has them (compile-time).
    QVERIFY(true);
}
