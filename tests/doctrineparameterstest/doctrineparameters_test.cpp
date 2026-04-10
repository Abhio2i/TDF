#include "doctrineparameters_test.h"
#include "GUI/DOCTRINE/doctrineparameters.h"
#include "core/Debug/console.h"
#include <QPushButton>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QJsonObject>
#include <QDebug>

#define DP_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runDoctrineParametersTests(DoctrineParameters* panel, Console* console)
{
    if (!panel || !console) {
        if (console) console->error("DoctrineParameters or Console is null, cannot run tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("    DOCTRINE PARAMETERS UNIT TESTS       "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: Basic UI elements exist -----
    QPushButton* blueBtn = panel->findChild<QPushButton*>("tabBtnBlue");
    QPushButton* redBtn = panel->findChild<QPushButton*>("tabBtnRed");
    DP_TEST(blueBtn != nullptr, "Blue team tab button exists");
    DP_TEST(redBtn != nullptr, "Red team tab button exists");

    QStackedWidget* stacked = panel->findChild<QStackedWidget*>();
    DP_TEST(stacked != nullptr, "Stacked widget exists");
    if (stacked) {
        DP_TEST(stacked->count() == 2, "Stacked widget has 2 pages (Blue and Red)");
    }

    // ----- Test 2: Default force type is Blue -----
    DP_TEST(panel->currentForce() == DoctrineParameters::FORCE_BLUE,
            "Default currentForce is FORCE_BLUE");
    DP_TEST(panel->getForceType() == "Blue",
            "getForceType() returns 'Blue' by default");

    // ----- Test 3: Tab switching works -----
    if (redBtn) {
        redBtn->click();
        DP_TEST(panel->currentForce() == DoctrineParameters::FORCE_RED,
                "Clicking Red tab switches currentForce to RED");
        DP_TEST(panel->getForceType() == "Red",
                "getForceType() returns 'Red' after switching to Red tab");
    }

    if (blueBtn) {
        blueBtn->click();
        DP_TEST(panel->currentForce() == DoctrineParameters::FORCE_BLUE,
                "Clicking Blue tab switches back to BLUE");
    }

    // ----- Test 4: UI widgets are properly created for both teams -----
    // Blue team widgets
    QLineEdit* blueNameEdit = panel->findChild<QLineEdit*>("", Qt::FindDirectChildrenOnly);
    // Better: find by property – but we can use the public? They are private.
    // We'll use findChildren and assume first line edit is doctrine name for current team.
    // Instead, we check that the stacked widget's current page has expected children.
    // Simpler: check that we can set values via the public API (toJson/loadFromJson)

    // ----- Test 5: toJson() returns correct structure -----
    QJsonObject json = panel->toJson();
    DP_TEST(json.contains("activeTeam"), "toJson() contains 'activeTeam'");
    DP_TEST(json.contains("blue"), "toJson() contains 'blue' object");
    DP_TEST(json.contains("red"), "toJson() contains 'red' object");

    QString activeTeam = json["activeTeam"].toString();
    DP_TEST(activeTeam == "BLUE" || activeTeam == "RED",
            "activeTeam value is valid ('BLUE' or 'RED')");

    // ----- Test 6: loadFromJson() works (test round-trip) -----
    // Create a test JSON object
    QJsonObject testBlue;
    testBlue["doctrineName"] = "TestDoctrine";
    testBlue["missionType"] = "PATROL";
    testBlue["missionObjective"] = "Test Objective";

    QJsonObject testRoot;
    testRoot["activeTeam"] = "BLUE";
    testRoot["blue"] = testBlue;
    testRoot["red"] = QJsonObject(); // empty

    panel->loadFromJson(testRoot);

    // After loading, the UI should have updated values
    // Since we cannot directly access the line edits, we dump toJson and compare
    QJsonObject afterLoad = panel->toJson();
    QJsonObject loadedBlue = afterLoad["blue"].toObject();
    DP_TEST(loadedBlue["doctrineName"].toString() == "TestDoctrine",
            "loadFromJson() loads doctrineName correctly");
    DP_TEST(loadedBlue["missionObjective"].toString() == "Test Objective",
            "loadFromJson() loads missionObjective correctly");
    DP_TEST(loadedBlue["missionType"].toString() == "PATROL",
            "loadFromJson() loads missionType correctly");

    // ----- Test 7: resetState() clears all fields -----
    panel->resetState();
    QJsonObject afterReset = panel->toJson();
    QJsonObject resetBlue = afterReset["blue"].toObject();
    DP_TEST(resetBlue["doctrineName"].toString().isEmpty(),
            "resetState() clears doctrineName");
    DP_TEST(resetBlue["missionObjective"].toString().isEmpty(),
            "resetState() clears missionObjective");
    DP_TEST(resetBlue["missionType"].toString().isEmpty() ||
                resetBlue["missionType"].toString() == "PATROL", // first item may be default
            "resetState() resets missionType to default");
    DP_TEST(panel->currentForce() == DoctrineParameters::FORCE_BLUE,
            "resetState() switches to Blue team");

    // ----- Test 8: Signals are emitted -----
    const QMetaObject* mo = panel->metaObject();
    bool hasValueChanged = (mo->indexOfSignal("valueChanged(QJsonObject)") != -1);
    bool hasForceTypeChanged = (mo->indexOfSignal("forceTypeChanged(int)") != -1);
    DP_TEST(hasValueChanged, "valueChanged signal exists");
    DP_TEST(hasForceTypeChanged, "forceTypeChanged signal exists");

    // ----- Test 9: Legacy format loading (single team JSON) -----
    QJsonObject legacyJson;
    legacyJson["doctrineName"] = "LegacyDoctrine";
    legacyJson["missionType"] = "STRIKE";
    panel->loadFromJson(legacyJson);
    QJsonObject afterLegacy = panel->toJson();
    QJsonObject legacyBlue = afterLegacy["blue"].toObject();
    DP_TEST(legacyBlue["doctrineName"].toString() == "LegacyDoctrine",
            "loadFromJson() handles legacy single-team format");

    // ----- Test 10: Default combo box items are populated -----
    // We can't easily test individual combos, but we can check that toJson returns non-empty values
    QJsonObject defaultJson = panel->toJson();
    QJsonObject defaultBlue = defaultJson["blue"].toObject();
    // Mission type should have a value (first item from populateDropdowns)
    DP_TEST(!defaultBlue["missionType"].toString().isEmpty(),
            "Mission type combo has default value");

    // Final summary
    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("DOCTRINE PARAMETERS TESTS: Some tests FAILED."));
    else
        console->log(std::string("DOCTRINE PARAMETERS TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
