#include "doctrineparameters_test.h"
#include "GUI/DOCTRINE/doctrineparameters.h"
#include <QTest>
#include <QPushButton>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QJsonObject>
#include <QSignalSpy>

void TestDoctrineParameters::init()
{
    panel = new DoctrineParameters(nullptr);
}

void TestDoctrineParameters::cleanup()
{
    delete panel;
    panel = nullptr;
}

// ------------------------------------------------------------------
// UI structure tests
// ------------------------------------------------------------------
void TestDoctrineParameters::testBasicUIElements()
{
    QPushButton* blueBtn = panel->findChild<QPushButton*>("tabBtnBlue");
    QPushButton* redBtn = panel->findChild<QPushButton*>("tabBtnRed");
    QVERIFY(blueBtn != nullptr);
    QVERIFY(redBtn != nullptr);

    QStackedWidget* stacked = panel->findChild<QStackedWidget*>();
    QVERIFY(stacked != nullptr);
    QCOMPARE(stacked->count(), 2);
}

void TestDoctrineParameters::testDefaultForceType()
{
    QCOMPARE(panel->currentForce(), DoctrineParameters::FORCE_BLUE);
    QCOMPARE(panel->getForceType(), QString("Blue"));
}

void TestDoctrineParameters::testTabSwitching()
{
    QPushButton* blueBtn = panel->findChild<QPushButton*>("tabBtnBlue");
    QPushButton* redBtn = panel->findChild<QPushButton*>("tabBtnRed");
    QVERIFY(blueBtn != nullptr);
    QVERIFY(redBtn != nullptr);

    // Switch to Red
    redBtn->click();
    QCOMPARE(panel->currentForce(), DoctrineParameters::FORCE_RED);
    QCOMPARE(panel->getForceType(), QString("Red"));

    // Switch back to Blue
    blueBtn->click();
    QCOMPARE(panel->currentForce(), DoctrineParameters::FORCE_BLUE);
    QCOMPARE(panel->getForceType(), QString("Blue"));
}

// ------------------------------------------------------------------
// JSON serialization tests
// ------------------------------------------------------------------
void TestDoctrineParameters::testToJsonStructure()
{
    QJsonObject json = panel->toJson();
    QVERIFY(json.contains("activeTeam"));
    QVERIFY(json.contains("blue"));
    QVERIFY(json.contains("red"));

    QString activeTeam = json["activeTeam"].toString();
    QVERIFY(activeTeam == "BLUE" || activeTeam == "RED");
}

void TestDoctrineParameters::testLoadFromJsonRoundTrip()
{
    // Create test JSON
    QJsonObject testBlue;
    testBlue["doctrineName"] = "TestDoctrine";
    testBlue["missionType"] = "PATROL";
    testBlue["missionObjective"] = "Test Objective";

    QJsonObject testRoot;
    testRoot["activeTeam"] = "BLUE";
    testRoot["blue"] = testBlue;
    testRoot["red"] = QJsonObject();

    panel->loadFromJson(testRoot);

    QJsonObject afterLoad = panel->toJson();
    QJsonObject loadedBlue = afterLoad["blue"].toObject();

    QCOMPARE(loadedBlue["doctrineName"].toString(), QString("TestDoctrine"));
    QCOMPARE(loadedBlue["missionObjective"].toString(), QString("Test Objective"));
    QCOMPARE(loadedBlue["missionType"].toString(), QString("PATROL"));
}

void TestDoctrineParameters::testResetState()
{
    // First load some data
    QJsonObject testBlue;
    testBlue["doctrineName"] = "SomeName";
    testBlue["missionObjective"] = "SomeObjective";
    QJsonObject testRoot;
    testRoot["activeTeam"] = "BLUE";
    testRoot["blue"] = testBlue;
    panel->loadFromJson(testRoot);

    // Reset
    panel->resetState();

    QJsonObject afterReset = panel->toJson();
    QJsonObject resetBlue = afterReset["blue"].toObject();

    QVERIFY(resetBlue["doctrineName"].toString().isEmpty());
    QVERIFY(resetBlue["missionObjective"].toString().isEmpty());
    // missionType may be default (first item) – we just check it's not empty? Actually reset should set to default first value
    // We'll only verify it exists; the exact default depends on implementation.
    QVERIFY(!resetBlue["missionType"].toString().isEmpty());

    QCOMPARE(panel->currentForce(), DoctrineParameters::FORCE_BLUE);
}

void TestDoctrineParameters::testLegacyFormatLoading()
{
    QJsonObject legacyJson;
    legacyJson["doctrineName"] = "LegacyDoctrine";
    legacyJson["missionType"] = "STRIKE";
    panel->loadFromJson(legacyJson);

    QJsonObject afterLoad = panel->toJson();
    QJsonObject loadedBlue = afterLoad["blue"].toObject();
    QCOMPARE(loadedBlue["doctrineName"].toString(), QString("LegacyDoctrine"));
    QCOMPARE(loadedBlue["missionType"].toString(), QString("STRIKE"));
}

// ------------------------------------------------------------------
// Signal tests
// ------------------------------------------------------------------
void TestDoctrineParameters::testSignalsExist()
{
    const QMetaObject* mo = panel->metaObject();
    bool hasValueChanged = (mo->indexOfSignal("valueChanged(QJsonObject)") != -1);
    bool hasForceTypeChanged = (mo->indexOfSignal("forceTypeChanged(int)") != -1);
    QVERIFY(hasValueChanged);
    QVERIFY(hasForceTypeChanged);
}

// ------------------------------------------------------------------
// Default values
// ------------------------------------------------------------------
void TestDoctrineParameters::testDefaultComboValues()
{
    QJsonObject defaultJson = panel->toJson();
    QJsonObject defaultBlue = defaultJson["blue"].toObject();
    // Mission type should have a value (first item from populateDropdowns)
    QVERIFY(!defaultBlue["missionType"].toString().isEmpty());
}
