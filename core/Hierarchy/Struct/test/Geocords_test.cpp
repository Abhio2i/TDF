#include "core/Hierarchy/Struct/geocords.h"
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

#define ASSERT_NEAR(val1, val2, epsilon, testName) \
if (std::fabs((val1) - (val2)) <= (epsilon)) { \
        std::cout << tests << " [PASS] " << testName << std::endl; \
        testsPassed++; \
        tests++; \
} else { \
        std::cerr << tests << " [FAIL] " << testName << " (Expected ~" << (val2) << ", Got: " << (val1) << ") (Line: " << __LINE__ << ")" << std::endl; \
        testsFailed++; \
        tests++; \
}


// ==========================================
// TEST SUITES
// ==========================================

void test_geocords_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;

    // Default constructor — all fields should be 0
    Geocords* g = new Geocords();
    ASSERT_EQ(g->latitude,  0.0, "Default latitude should be 0");
    ASSERT_EQ(g->longitude, 0.0, "Default longitude should be 0");
    ASSERT_EQ(g->altitude,  0.0, "Default altitude should be 0");
    ASSERT_EQ(g->Heading,   0.0, "Default Heading should be 0");
    delete g;

    // Parameterized constructor — all fields set correctly
    Geocords* g2 = new Geocords(33.6844, 73.0479, 500.0, 180.0);
    ASSERT_NEAR(g2->latitude,  33.6844, 0.0001, "Parameterized latitude should be 33.6844");
    ASSERT_NEAR(g2->longitude, 73.0479, 0.0001, "Parameterized longitude should be 73.0479");
    ASSERT_NEAR(g2->altitude,  500.0,   0.0001, "Parameterized altitude should be 500.0");
    ASSERT_NEAR(g2->Heading,   180.0,   0.0001, "Parameterized Heading should be 180.0");
    delete g2;

    // Negative values — valid for lat/lon
    Geocords* g3 = new Geocords(-33.8688, -70.6693, -10.0, 270.0);
    ASSERT_NEAR(g3->latitude,  -33.8688, 0.0001, "Negative latitude should be -33.8688");
    ASSERT_NEAR(g3->longitude, -70.6693, 0.0001, "Negative longitude should be -70.6693");
    ASSERT_NEAR(g3->altitude,  -10.0,    0.0001, "Negative altitude should be -10.0");
    delete g3;

    // Partial constructor — only latitude provided, rest default to 0
    Geocords* g4 = new Geocords(51.5074);
    ASSERT_NEAR(g4->latitude,  51.5074, 0.0001, "Partial constructor latitude should be 51.5074");
    ASSERT_EQ(g4->longitude, 0.0, "Partial constructor longitude should default to 0");
    ASSERT_EQ(g4->altitude,  0.0, "Partial constructor altitude should default to 0");
    ASSERT_EQ(g4->Heading,   0.0, "Partial constructor Heading should default to 0");
    delete g4;
}

void test_geocords_toJson() {
    std::cout << "\n--- Running toJson Tests ---" << std::endl;

    Geocords* g = new Geocords(33.6844, 73.0479, 500.0, 90.0);
    QJsonObject obj = g->toJson();

    // Result should not be empty
    ASSERT_FALSE(obj.empty(), "toJson() should return non-empty QJsonObject");

    // All 5 keys must always be present
    ASSERT_TRUE(obj.contains("type"),      "toJson() should contain 'type' key");
    ASSERT_TRUE(obj.contains("latitude"),  "toJson() should contain 'latitude' key");
    ASSERT_TRUE(obj.contains("longitude"), "toJson() should contain 'longitude' key");
    ASSERT_TRUE(obj.contains("altitude"),  "toJson() should contain 'altitude' key");
    ASSERT_TRUE(obj.contains("heading"),   "toJson() should contain 'heading' key");

    // 'type' should always be the string "geocord"
    ASSERT_EQ(obj["type"].toString().toStdString(), std::string("geocord"),
              "toJson() type should always be 'geocord'");

    // Values should match what was set
    ASSERT_NEAR(obj["latitude"].toDouble(),  33.6844, 0.0001, "toJson() latitude should be 33.6844");
    ASSERT_NEAR(obj["longitude"].toDouble(), 73.0479, 0.0001, "toJson() longitude should be 73.0479");
    ASSERT_NEAR(obj["altitude"].toDouble(),  500.0,   0.0001, "toJson() altitude should be 500.0");
    ASSERT_NEAR(obj["heading"].toDouble(),   90.0,    0.0001, "toJson() heading should be 90.0");

    // Default-constructed geocords should serialize all zeros
    Geocords* g2 = new Geocords();
    QJsonObject obj2 = g2->toJson();
    ASSERT_EQ(obj2["latitude"].toDouble(),  0.0, "(Default) toJson() latitude should be 0");
    ASSERT_EQ(obj2["longitude"].toDouble(), 0.0, "(Default) toJson() longitude should be 0");
    ASSERT_EQ(obj2["altitude"].toDouble(),  0.0, "(Default) toJson() altitude should be 0");
    ASSERT_EQ(obj2["heading"].toDouble(),   0.0, "(Default) toJson() heading should be 0");
    delete g2;

    // Negative coordinate values should serialize correctly
    Geocords* g3 = new Geocords(-33.8688, -70.6693, -50.0, 0.0);
    QJsonObject obj3 = g3->toJson();
    ASSERT_NEAR(obj3["latitude"].toDouble(),  -33.8688, 0.0001, "(Negative) toJson() latitude should be -33.8688");
    ASSERT_NEAR(obj3["longitude"].toDouble(), -70.6693, 0.0001, "(Negative) toJson() longitude should be -70.6693");
    ASSERT_NEAR(obj3["altitude"].toDouble(),  -50.0,    0.0001, "(Negative) toJson() altitude should be -50.0");
    delete g3;

    delete g;
}

void test_geocords_fromJson() {
    std::cout << "\n--- Running fromJson Tests ---" << std::endl;

    // Round-trip: serialize then deserialize and compare values
    Geocords* g = new Geocords(48.8566, 2.3522, 35.0, 45.0);
    QJsonObject obj = g->toJson();

    Geocords* g2 = new Geocords();
    try {
        g2->fromJson(obj);
        ASSERT_TRUE(true, "(Correct values) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Correct values) fromJson() crashed");
    }

    ASSERT_NEAR(g2->latitude,  48.8566, 0.0001, "(Round-trip) fromJson() latitude should be 48.8566");
    ASSERT_NEAR(g2->longitude, 2.3522,  0.0001, "(Round-trip) fromJson() longitude should be 2.3522");
    ASSERT_NEAR(g2->altitude,  35.0,    0.0001, "(Round-trip) fromJson() altitude should be 35.0");
    ASSERT_NEAR(g2->Heading,   45.0,    0.0001, "(Round-trip) fromJson() Heading should be 45.0");

    // Empty QJsonObject should not crash, fields should remain at their prior values
    try {
        Geocords* g3 = new Geocords(1.0, 2.0, 3.0, 4.0);
        g3->fromJson(QJsonObject());
        ASSERT_EQ(g3->latitude,  1.0, "(Empty QJsonObject) fromJson() should leave latitude untouched");
        ASSERT_EQ(g3->longitude, 2.0, "(Empty QJsonObject) fromJson() should leave longitude untouched");
        ASSERT_EQ(g3->altitude,  3.0, "(Empty QJsonObject) fromJson() should leave altitude untouched");
        ASSERT_EQ(g3->Heading,   4.0, "(Empty QJsonObject) fromJson() should leave Heading untouched");
        delete g3;
    } catch (...) {
        ASSERT_FALSE(true, "(Empty QJsonObject) fromJson() crashed");
    }

    // Each key is independently guarded — missing keys should leave those fields untouched
    {
        Geocords* g4 = new Geocords(10.0, 20.0, 30.0, 40.0);
        QJsonObject partial;
        partial["latitude"] = 99.0;
        // longitude, altitude, heading intentionally absent
        try {
            g4->fromJson(partial);
            ASSERT_NEAR(g4->latitude,  99.0, 0.0001, "(Partial JSON) fromJson() should update latitude");
            ASSERT_NEAR(g4->longitude, 20.0, 0.0001, "(Partial JSON) fromJson() should leave longitude untouched");
            ASSERT_NEAR(g4->altitude,  30.0, 0.0001, "(Partial JSON) fromJson() should leave altitude untouched");
            ASSERT_NEAR(g4->Heading,   40.0, 0.0001, "(Partial JSON) fromJson() should leave Heading untouched");
        } catch (...) {
            ASSERT_FALSE(true, "(Partial JSON) fromJson() crashed");
        }
        delete g4;
    }

    // Negative coordinate values should deserialize correctly
    {
        Geocords* g5 = new Geocords();
        QJsonObject negObj;
        negObj["type"]      = "geocord";
        negObj["latitude"]  = -33.8688;
        negObj["longitude"] = -70.6693;
        negObj["altitude"]  = -100.0;
        negObj["heading"]   = 270.0;
        try {
            g5->fromJson(negObj);
            ASSERT_NEAR(g5->latitude,  -33.8688, 0.0001, "(Negative coords) fromJson() latitude should be -33.8688");
            ASSERT_NEAR(g5->longitude, -70.6693, 0.0001, "(Negative coords) fromJson() longitude should be -70.6693");
            ASSERT_NEAR(g5->altitude,  -100.0,   0.0001, "(Negative coords) fromJson() altitude should be -100.0");
            ASSERT_NEAR(g5->Heading,   270.0,    0.0001, "(Negative coords) fromJson() Heading should be 270.0");
        } catch (...) {
            ASSERT_FALSE(true, "(Negative coords) fromJson() crashed");
        }
        delete g5;
    }

    // NOTE: 'type' key in JSON is written by toJson() but is NOT read by fromJson()
    // This is intentional — type is a fixed tag for identification, not a stored field.
    // Passing a QJsonObject with a wrong 'type' value should not affect any field.
    {
        Geocords* g6 = new Geocords(5.0, 6.0, 7.0, 8.0);
        QJsonObject wrongType;
        wrongType["type"]      = "wrong_type";
        wrongType["latitude"]  = 55.0;
        wrongType["longitude"] = 66.0;
        wrongType["altitude"]  = 77.0;
        wrongType["heading"]   = 88.0;
        try {
            g6->fromJson(wrongType);
            ASSERT_NEAR(g6->latitude,  55.0, 0.0001, "(Wrong type key) fromJson() should still update latitude");
            ASSERT_NEAR(g6->longitude, 66.0, 0.0001, "(Wrong type key) fromJson() should still update longitude");
            ASSERT_TRUE(true, "(Wrong type key) fromJson() should not crash");
        } catch (...) {
            ASSERT_FALSE(true, "(Wrong type key) fromJson() crashed");
        }
        delete g6;
    }

    delete g;
    delete g2;
}


void geocords_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "       GEOCORDS CUSTOM UNIT TESTS        " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_geocords_initialization();
    test_geocords_toJson();
    test_geocords_fromJson();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
