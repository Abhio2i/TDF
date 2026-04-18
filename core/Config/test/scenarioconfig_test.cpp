#include "core/Config/scenarioconfig.h"
#include <QCoreApplication>
#include <QJsonObject>
#include <iostream>

// --- Custom Testing Framework Variables ---
extern int testsPassed;
extern int testsFailed;
extern int tests;

// --- Custom Assertion Macros ---
#define ASSERT_TRUE(condition, testName) \
if (condition) { \
        std::cout << tests << " [PASS] " << testName << std::endl; \
        testsPassed++; \
        tests++; \
} else { \
        std::cerr << tests << " [FAIL] " << testName << " (Line: " << __LINE__ << ")" << std::endl; \
        testsFailed++; \
        tests++; \
}

#define ASSERT_FALSE(condition, testName) ASSERT_TRUE(!(condition), testName)

#define ASSERT_EQ(val1, val2, testName) \
if ((val1) == (val2)) { \
        std::cout << tests << " [PASS] " << testName << std::endl; \
        testsPassed++; \
        tests++; \
} else { \
        std::cerr << tests << " [FAIL] " << testName << " (Expected: " << (val2) << ", Got: " << (val1) << ") (Line: " << __LINE__ << ")" << std::endl; \
        testsFailed++; \
        tests++; \
}

#define ASSERT_NEQ(val1, val2, testName) ASSERT_TRUE((val1) != (val2), testName)


// Helper: create a fresh ScenarioConfig with a unique test org/app name
// so QSettings doesn't bleed across tests or into production settings
static ScenarioConfig* makeClean(const QString& testKey = "scenarioconfig_test") {
    QCoreApplication::setOrganizationName("TDF_Test_" + testKey);
    QCoreApplication::setApplicationName("TDF_Test_App_" + testKey);
    // Wipe any leftover settings from previous runs
    QSettings wipe(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    wipe.clear();
    wipe.sync();
    return new ScenarioConfig();
}


// ==========================================
// TEST SUITES
// ==========================================

void test_scenarioconfig_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;

    ScenarioConfig* sc = makeClean("init");

    // All FPS fields should default to 60 when no settings saved
    ASSERT_EQ(sc->getSavedFPS(),            60, "Default savedFPS should be 60");
    ASSERT_EQ(sc->getSavedGUIFPS(),         60, "Default savedGUIFPS should be 60");
    ASSERT_EQ(sc->getSavedSimulationFPS(),  60, "Default savedSimulationFPS should be 60");
    ASSERT_EQ(sc->getSavedPhysicsFPS(),     60, "Default savedPhysicsFPS should be 60");

    // Image size should default to "100px"
    ASSERT_EQ(sc->getSavedImageSize().toStdString(), std::string("100px"),
              "Default savedImageSize should be '100px'");

    // Database settings should default to disabled and empty path
    ASSERT_EQ(sc->getSavedDatabaseEnabled(), false, "Default savedDatabaseEnabled should be false");
    ASSERT_EQ(sc->getSavedDatabasePath().toStdString(), std::string(""),
              "Default savedDatabasePath should be empty");

    // Recent projects should be empty on fresh settings
    ASSERT_TRUE(sc->getRecentProjects().isEmpty(), "Default recentProjects should be empty");

    // Static version fields
    ASSERT_FALSE(ScenarioConfig::software_version.isEmpty(),
                 "software_version should not be empty");
    ASSERT_EQ(ScenarioConfig::software_version.toStdString(), std::string("3.0.94"),
              "software_version should be '3.0.94'");

    delete sc;
}

void test_scenarioconfig_saveAppSettings_5param() {
    std::cout << "\n--- Running saveAppSettings (5-param) Tests ---" << std::endl;

    ScenarioConfig* sc = makeClean("save5");

    // Correct values — all should be saved and readable via getters
    sc->saveAppSettings(30, 45, 60, 120, "200px");
    ASSERT_EQ(sc->getSavedFPS(),           30,  "(Correct) savedFPS should be 30");
    ASSERT_EQ(sc->getSavedGUIFPS(),        45,  "(Correct) savedGUIFPS should be 45");
    ASSERT_EQ(sc->getSavedSimulationFPS(), 60,  "(Correct) savedSimulationFPS should be 60");
    ASSERT_EQ(sc->getSavedPhysicsFPS(),    120, "(Correct) savedPhysicsFPS should be 120");
    ASSERT_EQ(sc->getSavedImageSize().toStdString(), std::string("200px"),
              "(Correct) savedImageSize should be '200px'");

    // FPS below range (0) — should be rejected, prior value kept
    sc->saveAppSettings(30, 45, 60, 120, "200px"); // set known state
    sc->saveAppSettings(0, 0, 0, 0, "200px");
    ASSERT_EQ(sc->getSavedFPS(),           30,  "(FPS=0) savedFPS should be unchanged at 30");
    ASSERT_EQ(sc->getSavedGUIFPS(),        45,  "(FPS=0) savedGUIFPS should be unchanged at 45");
    ASSERT_EQ(sc->getSavedSimulationFPS(), 60,  "(FPS=0) savedSimulationFPS should be unchanged at 60");
    ASSERT_EQ(sc->getSavedPhysicsFPS(),    120, "(FPS=0) savedPhysicsFPS should be unchanged at 120");

    // FPS above range (1001) — should be rejected, prior value kept
    sc->saveAppSettings(1001, 1001, 1001, 1001, "200px");
    ASSERT_EQ(sc->getSavedFPS(),           30,  "(FPS=1001) savedFPS should be unchanged at 30");
    ASSERT_EQ(sc->getSavedGUIFPS(),        45,  "(FPS=1001) savedGUIFPS should be unchanged at 45");
    ASSERT_EQ(sc->getSavedSimulationFPS(), 60,  "(FPS=1001) savedSimulationFPS should be unchanged at 60");
    ASSERT_EQ(sc->getSavedPhysicsFPS(),    120, "(FPS=1001) savedPhysicsFPS should be unchanged at 120");

    // FPS boundary: exactly 1 — should be accepted
    sc->saveAppSettings(1, 1, 1, 1, "50px");
    ASSERT_EQ(sc->getSavedFPS(),           1, "(FPS=1) savedFPS boundary should be accepted");
    ASSERT_EQ(sc->getSavedGUIFPS(),        1, "(FPS=1) savedGUIFPS boundary should be accepted");
    ASSERT_EQ(sc->getSavedSimulationFPS(), 1, "(FPS=1) savedSimulationFPS boundary should be accepted");
    ASSERT_EQ(sc->getSavedPhysicsFPS(),    1, "(FPS=1) savedPhysicsFPS boundary should be accepted");

    // FPS boundary: exactly 1000 — should be accepted
    sc->saveAppSettings(1000, 1000, 1000, 1000, "50px");
    ASSERT_EQ(sc->getSavedFPS(),           1000, "(FPS=1000) savedFPS boundary should be accepted");
    ASSERT_EQ(sc->getSavedGUIFPS(),        1000, "(FPS=1000) savedGUIFPS boundary should be accepted");
    ASSERT_EQ(sc->getSavedSimulationFPS(), 1000, "(FPS=1000) savedSimulationFPS boundary should be accepted");
    ASSERT_EQ(sc->getSavedPhysicsFPS(),    1000, "(FPS=1000) savedPhysicsFPS boundary should be accepted");

    // Empty imageSize — should be rejected, prior value kept
    sc->saveAppSettings(30, 45, 60, 120, "300px");
    sc->saveAppSettings(30, 45, 60, 120, "");
    ASSERT_EQ(sc->getSavedImageSize().toStdString(), std::string("300px"),
              "(Empty imageSize) savedImageSize should be unchanged at '300px'");

    delete sc;
}

void test_scenarioconfig_saveAppSettings_2param() {
    std::cout << "\n--- Running saveAppSettings (2-param) Tests ---" << std::endl;

    ScenarioConfig* sc = makeClean("save2");

    // Correct values
    sc->saveAppSettings(90, "512px");
    ASSERT_EQ(sc->getSavedFPS(), 90, "(2-param correct) savedFPS should be 90");
    ASSERT_EQ(sc->getSavedImageSize().toStdString(), std::string("512px"),
              "(2-param correct) savedImageSize should be '512px'");

    // Invalid FPS — rejected
    sc->saveAppSettings(0, "512px");
    ASSERT_EQ(sc->getSavedFPS(), 90, "(2-param FPS=0) savedFPS should be unchanged at 90");

    sc->saveAppSettings(1001, "512px");
    ASSERT_EQ(sc->getSavedFPS(), 90, "(2-param FPS=1001) savedFPS should be unchanged at 90");

    // Empty imageSize — rejected
    sc->saveAppSettings(90, "");
    ASSERT_EQ(sc->getSavedImageSize().toStdString(), std::string("512px"),
              "(2-param empty imageSize) savedImageSize should be unchanged");

    delete sc;
}

void test_scenarioconfig_loadAppSettings() {
    std::cout << "\n--- Running loadAppSettings Persistence Tests ---" << std::endl;

    // Save via one instance, load via another (same QSettings key)
    QCoreApplication::setOrganizationName("TDF_Test_persist");
    QCoreApplication::setApplicationName("TDF_Test_App_persist");
    QSettings wipe(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    wipe.clear(); wipe.sync();

    ScenarioConfig* sc1 = new ScenarioConfig();
    sc1->saveAppSettings(24, 48, 72, 96, "800px");
    delete sc1;

    // New instance should pick up saved values
    ScenarioConfig* sc2 = new ScenarioConfig();
    ASSERT_EQ(sc2->getSavedFPS(),           24,  "(Persist) loadAppSettings FPS should be 24");
    ASSERT_EQ(sc2->getSavedGUIFPS(),        48,  "(Persist) loadAppSettings GUIFPS should be 48");
    ASSERT_EQ(sc2->getSavedSimulationFPS(), 72,  "(Persist) loadAppSettings SimFPS should be 72");
    ASSERT_EQ(sc2->getSavedPhysicsFPS(),    96,  "(Persist) loadAppSettings PhysicsFPS should be 96");
    ASSERT_EQ(sc2->getSavedImageSize().toStdString(), std::string("800px"),
              "(Persist) loadAppSettings imageSize should be '800px'");
    delete sc2;
}

void test_scenarioconfig_recentProjects() {
    std::cout << "\n--- Running Recent Projects Tests ---" << std::endl;

    ScenarioConfig* sc = makeClean("recent");

    // Empty path should be ignored
    sc->addToRecentProjects("");
    ASSERT_TRUE(sc->getRecentProjects().isEmpty(),
                "(Empty path) addToRecentProjects should not add empty string");

    // Add first project
    sc->addToRecentProjects("/path/to/project1.tdf");
    ASSERT_EQ(sc->getRecentProjects().size(), 1, "(Add 1) recentProjects should have 1 entry");
    ASSERT_EQ(sc->getRecentProjects().first().toStdString(), std::string("/path/to/project1.tdf"),
              "(Add 1) first entry should be project1");

    // Add second project — should be prepended
    sc->addToRecentProjects("/path/to/project2.tdf");
    ASSERT_EQ(sc->getRecentProjects().size(), 2, "(Add 2) recentProjects should have 2 entries");
    ASSERT_EQ(sc->getRecentProjects().first().toStdString(), std::string("/path/to/project2.tdf"),
              "(Add 2) most recent should be project2 at index 0");

    // Adding duplicate should move it to front, not add a second entry
    sc->addToRecentProjects("/path/to/project1.tdf");
    ASSERT_EQ(sc->getRecentProjects().size(), 2, "(Duplicate) size should remain 2");
    ASSERT_EQ(sc->getRecentProjects().first().toStdString(), std::string("/path/to/project1.tdf"),
              "(Duplicate) re-added project1 should be at front");

    // Fill to 10 then add one more — oldest should be dropped
    sc->clearRecentProjects();
    for (int i = 1; i <= 10; i++) {
        sc->addToRecentProjects(QString("/path/project%1.tdf").arg(i));
    }
    ASSERT_EQ(sc->getRecentProjects().size(), 10, "(Max 10) recentProjects should have 10 entries");
    sc->addToRecentProjects("/path/project11.tdf");
    ASSERT_EQ(sc->getRecentProjects().size(), 10, "(Max 10 + 1) recentProjects should still be 10");
    ASSERT_EQ(sc->getRecentProjects().first().toStdString(), std::string("/path/project11.tdf"),
              "(Max 10 + 1) newest should be at front");

    // lastOpenedProject should be updated
    ASSERT_EQ(sc->lastOpenedProject.toStdString(), std::string("/path/project11.tdf"),
              "lastOpenedProject should be the most recently added path");

    // clearRecentProjects — list should be empty
    sc->clearRecentProjects();
    ASSERT_TRUE(sc->getRecentProjects().isEmpty(),
                "clearRecentProjects() should empty the list");

    delete sc;
}

void test_scenarioconfig_tooltipFields() {
    std::cout << "\n--- Running TooltipFields Tests ---" << std::endl;

    ScenarioConfig* sc = makeClean("tooltip");

    // loadTooltipFields on fresh settings should return 5 default fields
    QSet<QString> defaults = sc->loadTooltipFields();
    ASSERT_EQ(defaults.size(), 5, "(Default) loadTooltipFields should return 5 default fields");
    ASSERT_TRUE(defaults.contains("Name"),      "(Default) should contain 'Name'");
    ASSERT_TRUE(defaults.contains("Speed"),     "(Default) should contain 'Speed'");
    ASSERT_TRUE(defaults.contains("Altitude"),  "(Default) should contain 'Altitude'");
    ASSERT_TRUE(defaults.contains("Latitude"),  "(Default) should contain 'Latitude'");
    ASSERT_TRUE(defaults.contains("Longitude"), "(Default) should contain 'Longitude'");

    // Save custom fields and reload
    QSet<QString> custom = {"Heading", "Range", "Target"};
    sc->saveTooltipFields(custom);
    QSet<QString> loaded = sc->loadTooltipFields();
    ASSERT_EQ(loaded.size(), 3, "(Custom) loadTooltipFields should return 3 saved fields");
    ASSERT_TRUE(loaded.contains("Heading"), "(Custom) should contain 'Heading'");
    ASSERT_TRUE(loaded.contains("Range"),   "(Custom) should contain 'Range'");
    ASSERT_TRUE(loaded.contains("Target"),  "(Custom) should contain 'Target'");

    // Empty set saved — should still load defaults (empty list triggers default branch)
    QSet<QString> empty;
    sc->saveTooltipFields(empty);
    QSet<QString> afterEmpty = sc->loadTooltipFields();
    ASSERT_EQ(afterEmpty.size(), 5, "(Empty save) loadTooltipFields should return 5 defaults");

    // saveTooltipFields should not crash with large set
    try {
        QSet<QString> large;
        for (int i = 0; i < 50; i++) large.insert(QString("Field%1").arg(i));
        sc->saveTooltipFields(large);
        ASSERT_TRUE(true, "(Large set) saveTooltipFields should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Large set) saveTooltipFields crashed");
    }

    delete sc;
}

void test_scenarioconfig_databaseSettings() {
    std::cout << "\n--- Running DatabaseSettings Tests ---" << std::endl;

    ScenarioConfig* sc = makeClean("database");

    // Default state
    ASSERT_EQ(sc->getSavedDatabaseEnabled(), false, "(Default) database enabled should be false");
    ASSERT_EQ(sc->getSavedDatabasePath().toStdString(), std::string(""),
              "(Default) database path should be empty");

    // Save enabled=true with path
    sc->saveDatabaseSettings(true, "/var/db/tdf.db");
    ASSERT_EQ(sc->getSavedDatabaseEnabled(), true, "(Saved) database enabled should be true");
    ASSERT_EQ(sc->getSavedDatabasePath().toStdString(), std::string("/var/db/tdf.db"),
              "(Saved) database path should be '/var/db/tdf.db'");

    // Save enabled=false with empty path
    sc->saveDatabaseSettings(false, "");
    ASSERT_EQ(sc->getSavedDatabaseEnabled(), false, "(Disabled) database enabled should be false");
    ASSERT_EQ(sc->getSavedDatabasePath().toStdString(), std::string(""),
              "(Disabled) database path should be empty");

    // Persistence: save then reload via new instance
    QCoreApplication::setOrganizationName("TDF_Test_dbpersist");
    QCoreApplication::setApplicationName("TDF_Test_App_dbpersist");
    QSettings wipe(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    wipe.clear(); wipe.sync();

    ScenarioConfig* sc2 = new ScenarioConfig();
    sc2->saveDatabaseSettings(true, "/data/mydb.sqlite");
    delete sc2;

    ScenarioConfig* sc3 = new ScenarioConfig();
    ASSERT_EQ(sc3->getSavedDatabaseEnabled(), true,
              "(DB Persist) getSavedDatabaseEnabled should be true after reload");
    ASSERT_EQ(sc3->getSavedDatabasePath().toStdString(), std::string("/data/mydb.sqlite"),
              "(DB Persist) getSavedDatabasePath should persist after reload");
    delete sc3;

    delete sc;
}

void test_scenarioconfig_staticFields() {
    std::cout << "\n--- Running Static Fields Tests ---" << std::endl;

    ScenarioConfig::file_Version = "";

    // software_version should be readable without an instance
    ASSERT_FALSE(ScenarioConfig::software_version.isEmpty(),
                 "software_version should not be empty");
    ASSERT_EQ(ScenarioConfig::software_version.toStdString(), std::string("3.0.94"),
              "software_version should be '3.0.94'");

    // file_Version starts empty
    ASSERT_TRUE(ScenarioConfig::file_Version.isEmpty(),
                "file_Version should be empty by default");

    // file_Version can be modified
    ScenarioConfig::file_Version = "1.0.0";
    ASSERT_EQ(ScenarioConfig::file_Version.toStdString(), std::string("1.0.0"),
              "file_Version should be '1.0.0' after assignment");

    // Reset for other tests
}

void test_scenarioconfig_noopMethods() {
    std::cout << "\n--- Running No-op Methods Tests ---" << std::endl;

    ScenarioConfig* sc = makeClean("noop");

    // toJson and fromJson are currently no-ops — should not crash
    try {
        sc->toJson();
        ASSERT_TRUE(true, "toJson() should not crash (no-op)");
    } catch (...) {
        ASSERT_FALSE(true, "toJson() crashed");
    }

    try {
        sc->fromJson();
        ASSERT_TRUE(true, "fromJson() should not crash (no-op)");
    } catch (...) {
        ASSERT_FALSE(true, "fromJson() crashed");
    }

    delete sc;
}


void scenarioConfig_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "     SCENARIOCONFIG CUSTOM UNIT TESTS    " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_scenarioconfig_initialization();
    test_scenarioconfig_saveAppSettings_5param();
    test_scenarioconfig_saveAppSettings_2param();
    test_scenarioconfig_loadAppSettings();
    test_scenarioconfig_recentProjects();
    test_scenarioconfig_tooltipFields();
    test_scenarioconfig_databaseSettings();
    test_scenarioconfig_staticFields();
    test_scenarioconfig_noopMethods();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
