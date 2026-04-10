#include "tacticalrules_test.h"
#include "GUI/DOCTRINE/tacticalrules.h"
#include "core/Debug/console.h"
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QJsonObject>
#include <QDebug>

#define TR_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runTacticalRulesTests(TacticalRules* panel, Console* console)
{
    if (!panel || !console) {
        if (console) console->error("TacticalRules or Console is null, cannot run tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("       TACTICAL RULES UNIT TESTS         "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: UI elements exist -----
    QDoubleSpinBox* rangeSpin = panel->findChild<QDoubleSpinBox*>();
    QComboBox* weaponCombo = panel->findChild<QComboBox*>("", Qt::FindDirectChildrenOnly);
    QPushButton* applyBtn = nullptr;
    QPushButton* resetBtn = nullptr;
    for (QPushButton* btn : panel->findChildren<QPushButton*>()) {
        if (btn->text() == "Apply Changes") applyBtn = btn;
        if (btn->text() == "Reset Rules") resetBtn = btn;
    }
    TR_TEST(rangeSpin != nullptr, "Max Engagement Range spinbox exists");
    TR_TEST(weaponCombo != nullptr, "Weapon Release Authority combo exists");
    TR_TEST(applyBtn != nullptr, "Apply Changes button exists");
    TR_TEST(resetBtn != nullptr, "Reset Rules button exists");

    // ----- Test 2: Default values are correct -----
    TR_TEST(panel->toJson()["maxEngagementRange"].toDouble() == 30.0,
            "Default maxEngagementRange is 30.0");
    TR_TEST(panel->toJson()["supportRequestThreshold"].toDouble() == 50.0,
            "Default supportRequestThreshold is 50.0");
    TR_TEST(panel->toJson()["fuelSafetyMargin"].toDouble() == 20.0,
            "Default fuelSafetyMargin is 20.0");
    TR_TEST(panel->toJson()["weaponReleaseAuthority"].toString() == "AUTOMATIC",
            "Default weaponReleaseAuthority is 'AUTOMATIC'");
    TR_TEST(panel->toJson()["sensorActivationRule"].toString() == "PASSIVE_SENSORS_ONLY",
            "Default sensorActivationRule is 'PASSIVE_SENSORS_ONLY'");
    TR_TEST(panel->toJson()["formationType"].toString() == "LINE_ABREAST",
            "Default formationType is 'LINE_ABREAST'");

    // ----- Test 3: Reset rules works -----
    // Change some values
    if (rangeSpin) rangeSpin->setValue(100.0);
    if (weaponCombo) weaponCombo->setCurrentIndex(2);
    // Click reset
    if (resetBtn) resetBtn->click();
    // Verify they reverted
    TR_TEST(panel->toJson()["maxEngagementRange"].toDouble() == 30.0,
            "Reset button restores default maxEngagementRange");
    TR_TEST(panel->toJson()["weaponReleaseAuthority"].toString() == "AUTOMATIC",
            "Reset button restores default weaponReleaseAuthority");

    // ----- Test 4: Apply button emits signal (we can't easily test, but check it exists) -----
    TR_TEST(applyBtn != nullptr, "Apply button exists (signal emission tested indirectly)");

    // ----- Test 5: Team switching (setForceType) works -----
    // Store current (Blue) data
    QJsonObject blueData = panel->toJson();
    // Switch to Red team
    panel->setForceType(1);  // RED
    // Red should have default values initially (since no data saved)
    TR_TEST(panel->toJson()["maxEngagementRange"].toDouble() == 30.0,
            "Switching to Red loads default values (no cached data)");
    // Modify Red values
    if (rangeSpin) rangeSpin->setValue(200.0);
    if (weaponCombo) weaponCombo->setCurrentIndex(3);
    QJsonObject redModified = panel->toJson();
    // Switch back to Blue
    panel->setForceType(0);  // BLUE
    TR_TEST(panel->toJson()["maxEngagementRange"].toDouble() == blueData["maxEngagementRange"].toDouble(),
            "Switching back to Blue restores previous Blue values");
    // Switch to Red again
    panel->setForceType(1);
    TR_TEST(panel->toJson()["maxEngagementRange"].toDouble() == redModified["maxEngagementRange"].toDouble(),
            "Switching back to Red restores previously modified Red values");

    // Reset to Blue for remaining tests
    panel->setForceType(0);

    // ----- Test 6: JSON serialization (single team) -----
    QJsonObject testObj;
    testObj["maxEngagementRange"] = 75.0;
    testObj["weaponReleaseAuthority"] = "WEAPON_FREE";
    testObj["sensorActivationRule"] = "ACTIVE_RADAR_ALLOWED";
    testObj["formationType"] = "WEDGE";
    testObj["supportRequestThreshold"] = 80.0;
    testObj["fuelSafetyMargin"] = 35.0;
    panel->loadFromJson(testObj);
    QJsonObject loaded = panel->toJson();
    TR_TEST(loaded["maxEngagementRange"].toDouble() == 75.0,
            "loadFromJson() loads maxEngagementRange correctly");
    TR_TEST(loaded["weaponReleaseAuthority"].toString() == "WEAPON_FREE",
            "loadFromJson() loads weaponReleaseAuthority correctly");
    TR_TEST(loaded["sensorActivationRule"].toString() == "ACTIVE_RADAR_ALLOWED",
            "loadFromJson() loads sensorActivationRule correctly");
    TR_TEST(loaded["formationType"].toString() == "WEDGE",
            "loadFromJson() loads formationType correctly");
    TR_TEST(loaded["supportRequestThreshold"].toDouble() == 80.0,
            "loadFromJson() loads supportRequestThreshold correctly");
    TR_TEST(loaded["fuelSafetyMargin"].toDouble() == 35.0,
            "loadFromJson() loads fuelSafetyMargin correctly");

    // ----- Test 7: JSON both teams serialization -----
    // Reset both teams
    panel->resetState();
    panel->setForceType(0);
    panel->loadFromJson(testObj);  // load into Blue
    panel->setForceType(1);
    QJsonObject redTest;
    redTest["maxEngagementRange"] = 120.0;
    panel->loadFromJson(redTest); // load into Red
    QJsonObject both = panel->toJsonBothTeams();
    TR_TEST(both.contains("blue") && both.contains("red"),
            "toJsonBothTeams() contains 'blue' and 'red' keys");
    TR_TEST(both["blue"].toObject()["maxEngagementRange"].toDouble() == 75.0,
            "toJsonBothTeams() preserves Blue team data");
    TR_TEST(both["red"].toObject()["maxEngagementRange"].toDouble() == 120.0,
            "toJsonBothTeams() preserves Red team data");

    // ----- Test 8: loadBothTeamsFromJson works -----
    QJsonObject newBoth;
    newBoth["blue"] = testObj;
    newBoth["red"] = redTest;
    panel->loadBothTeamsFromJson(newBoth);
    panel->setForceType(0);
    TR_TEST(panel->toJson()["maxEngagementRange"].toDouble() == 75.0,
            "loadBothTeamsFromJson() loads Blue correctly");
    panel->setForceType(1);
    TR_TEST(panel->toJson()["maxEngagementRange"].toDouble() == 120.0,
            "loadBothTeamsFromJson() loads Red correctly");

    // ----- Test 9: Legacy single-format loading (both teams call) -----
    panel->loadBothTeamsFromJson(testObj);
    panel->setForceType(0);
    TR_TEST(panel->toJson()["maxEngagementRange"].toDouble() == 75.0,
            "loadBothTeamsFromJson() handles legacy format (loads into Blue)");
    panel->setForceType(1);
    TR_TEST(panel->toJson()["maxEngagementRange"].toDouble() == 30.0,
            "Legacy format leaves Red unchanged (default)");

    // ----- Test 10: Signals exist -----
    const QMetaObject* mo = panel->metaObject();
    bool hasValueChanged = (mo->indexOfSignal("valueChanged(QJsonObject)") != -1);
    bool hasApplyRequested = (mo->indexOfSignal("applyRequested(QJsonObject)") != -1);
    TR_TEST(hasValueChanged, "valueChanged signal exists");
    TR_TEST(hasApplyRequested, "applyRequested signal exists");

    // ----- Test 11: getRulesCount() returns integer (placeholder) -----
    int count = panel->getRulesCount();
    TR_TEST(count >= 0, "getRulesCount() returns non-negative integer");

    // Final summary
    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("TACTICAL RULES TESTS: Some tests FAILED."));
    else
        console->log(std::string("TACTICAL RULES TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
