#include "tacticalrules_test.h"
#include "GUI/DOCTRINE/tacticalrules.h"
#include <QTest>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QJsonObject>

void TestTacticalRules::init()
{
    panel = new TacticalRules(nullptr);
    // Ensure the UI is built
    QTest::qWait(50);
}

void TestTacticalRules::cleanup()
{
    delete panel;
    panel = nullptr;
}



void TestTacticalRules::testDefaultValues()
{
    QJsonObject json = panel->toJson();
    QCOMPARE(json["maxEngagementRange"].toDouble(), 30.0);
    QCOMPARE(json["supportRequestThreshold"].toDouble(), 50.0);
    QCOMPARE(json["fuelSafetyMargin"].toDouble(), 20.0);
    QCOMPARE(json["weaponReleaseAuthority"].toString(), QString("AUTOMATIC"));
    QCOMPARE(json["sensorActivationRule"].toString(), QString("PASSIVE_SENSORS_ONLY"));
    QCOMPARE(json["formationType"].toString(), QString("LINE_ABREAST"));
}

void TestTacticalRules::testResetRules()
{
    QDoubleSpinBox* rangeSpin = panel->findChild<QDoubleSpinBox*>();
    QComboBox* weaponCombo = panel->findChild<QComboBox*>("", Qt::FindDirectChildrenOnly);
    QPushButton* resetBtn = nullptr;
    for (QPushButton* btn : panel->findChildren<QPushButton*>()) {
        if (btn->text() == "Reset Rules") resetBtn = btn;
    }
    QVERIFY(resetBtn != nullptr);

    if (rangeSpin) rangeSpin->setValue(100.0);
    if (weaponCombo) weaponCombo->setCurrentIndex(2);
    resetBtn->click();
    QJsonObject json = panel->toJson();
    QCOMPARE(json["maxEngagementRange"].toDouble(), 30.0);
    QCOMPARE(json["weaponReleaseAuthority"].toString(), QString("AUTOMATIC"));
}

void TestTacticalRules::testApplyButtonExists()
{
    QPushButton* applyBtn = nullptr;
    for (QPushButton* btn : panel->findChildren<QPushButton*>()) {
        if (btn->text() == "Apply Changes") applyBtn = btn;
    }
    QVERIFY(applyBtn != nullptr);
}

void TestTacticalRules::testTeamSwitching()
{
    // Store Blue data
    QJsonObject blueData = panel->toJson();
    // Switch to Red (assuming setForceType(1) = Red)
    panel->setForceType(1);
    QJsonObject redDefault = panel->toJson();
    QCOMPARE(redDefault["maxEngagementRange"].toDouble(), 30.0);

    // Modify Red values
    QDoubleSpinBox* rangeSpin = panel->findChild<QDoubleSpinBox*>();
    QComboBox* weaponCombo = panel->findChild<QComboBox*>("", Qt::FindDirectChildrenOnly);
    if (rangeSpin) rangeSpin->setValue(200.0);
    if (weaponCombo) weaponCombo->setCurrentIndex(3);
    QJsonObject redModified = panel->toJson();

    // Switch back to Blue
    panel->setForceType(0);
    QJsonObject blueRestored = panel->toJson();
    QCOMPARE(blueRestored["maxEngagementRange"].toDouble(), blueData["maxEngagementRange"].toDouble());

    // Switch to Red again
    panel->setForceType(1);
    QJsonObject redRestored = panel->toJson();
    QCOMPARE(redRestored["maxEngagementRange"].toDouble(), redModified["maxEngagementRange"].toDouble());

    // Restore to Blue for remaining tests
    panel->setForceType(0);
}

void TestTacticalRules::testJsonSerializationSingleTeam()
{
    QJsonObject testObj;
    testObj["maxEngagementRange"] = 75.0;
    testObj["weaponReleaseAuthority"] = "WEAPON_FREE";
    testObj["sensorActivationRule"] = "ACTIVE_RADAR_ALLOWED";
    testObj["formationType"] = "WEDGE";
    testObj["supportRequestThreshold"] = 80.0;
    testObj["fuelSafetyMargin"] = 35.0;

    panel->loadFromJson(testObj);
    QJsonObject loaded = panel->toJson();
    QCOMPARE(loaded["maxEngagementRange"].toDouble(), 75.0);
    QCOMPARE(loaded["weaponReleaseAuthority"].toString(), QString("WEAPON_FREE"));
    QCOMPARE(loaded["sensorActivationRule"].toString(), QString("ACTIVE_RADAR_ALLOWED"));
    QCOMPARE(loaded["formationType"].toString(), QString("WEDGE"));
    QCOMPARE(loaded["supportRequestThreshold"].toDouble(), 80.0);
    QCOMPARE(loaded["fuelSafetyMargin"].toDouble(), 35.0);
}

void TestTacticalRules::testJsonSerializationBothTeams()
{
    // Reset and load Blue
    panel->resetState();
    panel->setForceType(0);
    QJsonObject blueData;
    blueData["maxEngagementRange"] = 75.0;
    panel->loadFromJson(blueData);

    // Load Red
    panel->setForceType(1);
    QJsonObject redData;
    redData["maxEngagementRange"] = 120.0;
    panel->loadFromJson(redData);

    QJsonObject both = panel->toJsonBothTeams();
    QVERIFY(both.contains("blue") && both.contains("red"));
    QCOMPARE(both["blue"].toObject()["maxEngagementRange"].toDouble(), 75.0);
    QCOMPARE(both["red"].toObject()["maxEngagementRange"].toDouble(), 120.0);

    // Test loadBothTeamsFromJson
    QJsonObject newBoth;
    newBoth["blue"] = blueData;
    newBoth["red"] = redData;
    panel->loadBothTeamsFromJson(newBoth);
    panel->setForceType(0);
    QCOMPARE(panel->toJson()["maxEngagementRange"].toDouble(), 75.0);
    panel->setForceType(1);
    QCOMPARE(panel->toJson()["maxEngagementRange"].toDouble(), 120.0);
}

void TestTacticalRules::testLegacyFormatLoading()
{
    QJsonObject legacy;
    legacy["maxEngagementRange"] = 75.0;
    panel->loadBothTeamsFromJson(legacy);
    panel->setForceType(0);
    QCOMPARE(panel->toJson()["maxEngagementRange"].toDouble(), 75.0);
    panel->setForceType(1);
    QCOMPARE(panel->toJson()["maxEngagementRange"].toDouble(), 30.0);
}

void TestTacticalRules::testSignalsExist()
{
    const QMetaObject* mo = panel->metaObject();
    QVERIFY(mo->indexOfSignal("valueChanged(QJsonObject)") != -1);
    QVERIFY(mo->indexOfSignal("applyRequested(QJsonObject)") != -1);
}

void TestTacticalRules::testGetRulesCount()
{
    int count = panel->getRulesCount();
    QVERIFY(count >= 0);
}
