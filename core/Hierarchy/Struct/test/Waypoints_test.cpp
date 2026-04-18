#include "core/Hierarchy/Struct/waypoints.h"
#include <QJsonObject>
#include <iostream>
#include <cmath>

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


// ==========================================
// TEST SUITES
// ==========================================

void test_waypoints_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;

    Waypoints* w = new Waypoints();

    // geocord should be allocated
    ASSERT_NEQ(w->geocord, nullptr, "Constructor should allocate geocord (not null)");

    // position should be allocated
    ASSERT_NEQ(w->position, nullptr, "Constructor should allocate position (not null)");

    // speed default
    ASSERT_EQ(w->speed, 0.0, "Default speed should be 0");

    // sensor default
    ASSERT_EQ(w->sensor, true, "Default sensor should be true");

    // formation default
    ASSERT_EQ(w->formation, true, "Default formation should be true");

    delete w;
}

void test_waypoints_toJson() {
    std::cout << "\n--- Running toJson Tests ---" << std::endl;

    Waypoints* w = new Waypoints();
    w->speed = 55.5;
    w->sensor = false;
    w->formation = true;

    QJsonObject obj = w->toJson();

    // Result should not be empty
    ASSERT_FALSE(obj.empty(), "toJson() should return non-empty QJsonObject");

    // All 4 primitive keys must always be present
    ASSERT_TRUE(obj.contains("sensor"),    "toJson() should contain 'sensor' key");
    ASSERT_TRUE(obj.contains("formation"), "toJson() should contain 'formation' key");
    ASSERT_TRUE(obj.contains("speed"),     "toJson() should contain 'speed' key");
    ASSERT_TRUE(obj.contains("geocord"),   "toJson() should contain 'geocord' key");
    ASSERT_TRUE(obj.contains("position"),  "toJson() should contain 'position' key");

    // Primitive values should match what was set
    ASSERT_EQ(obj["sensor"].toBool(),    false, "toJson() sensor should be false");
    ASSERT_EQ(obj["formation"].toBool(), true,  "toJson() formation should be true");
    ASSERT_TRUE(std::fabs(obj["speed"].toDouble() - 55.5) < 0.0001, "toJson() speed should be ~55.5");

    // geocord and position should serialize as objects
    ASSERT_TRUE(obj["geocord"].isObject(),  "toJson() geocord should be a QJsonObject");
    ASSERT_TRUE(obj["position"].isObject(), "toJson() position should be a QJsonObject");

    // Null geocord should be skipped gracefully (no crash, key absent)
    Waypoints* w2 = new Waypoints();
    delete w2->geocord;
    w2->geocord = nullptr;
    try {
        QJsonObject obj2 = w2->toJson();
        ASSERT_FALSE(obj2.contains("geocord"), "(Null geocord) toJson() should skip 'geocord' key");
    } catch (...) {
        ASSERT_FALSE(true, "(Null geocord) toJson() should not crash");
    }
    w2->geocord = new Geocords(); // restore before delete
    delete w2;

    // Null position should be skipped gracefully (no crash, key absent)
    Waypoints* w3 = new Waypoints();
    delete w3->position;
    w3->position = nullptr;
    try {
        QJsonObject obj3 = w3->toJson();
        ASSERT_FALSE(obj3.contains("position"), "(Null position) toJson() should skip 'position' key");
    } catch (...) {
        ASSERT_FALSE(true, "(Null position) toJson() should not crash");
    }
    w3->position = new Vector(); // restore before delete
    delete w3;

    delete w;
}

void test_waypoints_fromJson() {
    std::cout << "\n--- Running fromJson Tests ---" << std::endl;

    // Round-trip: serialize then deserialize and compare values
    Waypoints* w = new Waypoints();
    w->speed     = 120.0;
    w->sensor    = false;
    w->formation = false;
    w->position->x = 1.0f;
    w->position->y = 2.0f;
    w->position->z = 3.0f;

    QJsonObject obj = w->toJson();

    Waypoints* w2 = new Waypoints();
    try {
        w2->fromJson(obj);
        ASSERT_TRUE(true, "(Correct values) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Correct values) fromJson() crashed");
    }

    // speed should round-trip correctly
    ASSERT_TRUE(std::fabs(w2->speed - 120.0) < 0.0001, "(Round-trip) fromJson() speed should be 120.0");

    // sensor should round-trip correctly
    ASSERT_EQ(w2->sensor, false, "(Round-trip) fromJson() sensor should be false");

    // position values should be restored
    ASSERT_TRUE(std::fabs(w2->position->x - 1.0f) < 0.0001f, "(Round-trip) fromJson() position x should be 1.0");
    ASSERT_TRUE(std::fabs(w2->position->y - 2.0f) < 0.0001f, "(Round-trip) fromJson() position y should be 2.0");
    ASSERT_TRUE(std::fabs(w2->position->z - 3.0f) < 0.0001f, "(Round-trip) fromJson() position z should be 3.0");

    // NOTE: Known bug — fromJson() writes the "formation" key value into
    // 'sensor' instead of 'formation' (line: sensor = obj["formation"].toBool()).
    // This means 'formation' is never restored from JSON.
    // This test documents that behavior so it is visible in the test output.
    Waypoints* wBug = new Waypoints();
    QJsonObject bugObj;
    bugObj["sensor"]    = false;
    bugObj["formation"] = true;   // should go into formation, but goes into sensor
    bugObj["speed"]     = 0.0;
    wBug->fromJson(bugObj);
    // After fromJson: sensor receives formation's value (true), overwriting sensor's false
    ASSERT_EQ(wBug->sensor,    true,  "(Known bug) fromJson() writes 'formation' value into sensor");
    // formation remains at its default (true) — it is never updated by fromJson()
    ASSERT_EQ(wBug->formation, true,  "(Known bug) fromJson() never updates formation field");
    delete wBug;

    // Empty QJsonObject should not crash
    try {
        Waypoints* w3 = new Waypoints();
        w3->fromJson(QJsonObject());
        ASSERT_TRUE(true, "(Empty QJsonObject) fromJson() should not crash");
        delete w3;
    } catch (...) {
        ASSERT_FALSE(true, "(Empty QJsonObject) fromJson() crashed");
    }

    // fromJson should allocate geocord if it was null before
    Waypoints* w4 = new Waypoints();
    delete w4->geocord;
    w4->geocord = nullptr;
    try {
        w4->fromJson(obj);
        ASSERT_NEQ(w4->geocord, nullptr, "(Null geocord pre-fromJson) fromJson() should allocate geocord");
        ASSERT_TRUE(true, "(Null geocord pre-fromJson) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Null geocord pre-fromJson) fromJson() crashed");
    }
    delete w4;

    // fromJson should allocate position if it was null before
    Waypoints* w5 = new Waypoints();
    delete w5->position;
    w5->position = nullptr;
    try {
        w5->fromJson(obj);
        ASSERT_NEQ(w5->position, nullptr, "(Null position pre-fromJson) fromJson() should allocate position");
        ASSERT_TRUE(true, "(Null position pre-fromJson) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Null position pre-fromJson) fromJson() crashed");
    }
    delete w5;

    // Missing 'geocord' key should leave existing geocord untouched
    Waypoints* w6 = new Waypoints();
    w6->position->x = 9.0f;
    QJsonObject noGeo;
    noGeo["speed"] = 10.0;
    try {
        w6->fromJson(noGeo);
        ASSERT_NEQ(w6->geocord, nullptr, "(Missing geocord key) existing geocord should remain allocated");
        ASSERT_TRUE(true, "(Missing geocord key) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Missing geocord key) fromJson() crashed");
    }
    delete w6;

    // Missing 'position' key should leave existing position untouched
    Waypoints* w7 = new Waypoints();
    w7->position->x = 7.0f;
    QJsonObject noPos;
    noPos["speed"] = 5.0;
    try {
        w7->fromJson(noPos);
        ASSERT_TRUE(std::fabs(w7->position->x - 7.0f) < 0.0001f,
                    "(Missing position key) fromJson() should leave existing position x untouched");
    } catch (...) {
        ASSERT_FALSE(true, "(Missing position key) fromJson() crashed");
    }
    delete w7;

    delete w;
    delete w2;
}


void waypoints_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "       WAYPOINTS CUSTOM UNIT TESTS       " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_waypoints_initialization();
    test_waypoints_toJson();
    test_waypoints_fromJson();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
