#ifndef GUI_MISSION_TEST_H
#define GUI_MISSION_TEST_H

#include <QObject>

class MissionEditor;

class TestMissionEditor : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testPanelsExist();
    void testDoctrineDefaultForceType();
    void testTacticalRulesInitiallyEmpty();
    void testHierarchyTreeHasItems();

    void testUnsavedChangesFlag();

private:
    MissionEditor* editor = nullptr;
};

#endif // GUI_MISSION_TEST_H
