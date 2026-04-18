#ifndef TACTICALRULES_TEST_H
#define TACTICALRULES_TEST_H

#include <QObject>

class TacticalRules;

class TestTacticalRules : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();


    void testDefaultValues();
    void testResetRules();
    void testApplyButtonExists();
    void testTeamSwitching();
    void testJsonSerializationSingleTeam();
    void testJsonSerializationBothTeams();
    void testLegacyFormatLoading();
    void testSignalsExist();
    void testGetRulesCount();

private:
    TacticalRules* panel = nullptr;
};

#endif
