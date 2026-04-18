#include "core/Hierarchy/Components/sensorprofile.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include <iostream>
#include <QJsonObject>

// --- Custom Testing Framework Variables (Extern) ---
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
// TEST SUITES FOR SENSORPROFILE
// ==========================================

void test_SensorProfile_initialization() {
    std::cout << "\n--- Running SensorProfile Initialization Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    SensorProfile* sp = new SensorProfile(h);

    ASSERT_NEQ(sp, nullptr, "SensorProfile should be initialized");
    ASSERT_TRUE(sp->Active, "SensorProfile should be active by default");
    ASSERT_NEQ(sp->sensors, nullptr, "Sensors map should be allocated");
    ASSERT_TRUE(sp->sensors->empty(), "Sensors map should be empty initially");

    delete sp;
    delete h;
}

void test_SensorProfile_Adding_Sensors() {
    std::cout << "\n--- Running Sensor Addition Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    SensorProfile* sp = new SensorProfile(h);

    // Test adding Radar (Generic)
    sp->addSubComponent("Main_Radar", "Generic");
    ASSERT_EQ(sp->sensors->size(), 1, "Should have 1 sensor after adding Radar");

    // Test adding AIS
    sp->addSubComponent("Coast_AIS", "AIS");
    ASSERT_EQ(sp->sensors->size(), 2, "Should have 2 sensors after adding AIS");

    // Verify pointers exist in map
    for(auto const& [id, sensor] : *sp->sensors) {
        ASSERT_NEQ(sensor, nullptr, "Sensor pointer should not be null");
    }

    delete sp;
    delete h;
}

void test_SensorProfile_Operations() {
    std::cout << "\n--- Running Sensor Operation Tests (Rename/Remove) ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    SensorProfile* sp = new SensorProfile(h);

    sp->addSubComponent("Temp_Sensor", "Sonar");
    std::string sensorID = sp->sensors->begin()->first;

    // Test rename
    sp->renameSubComponent(sensorID, "Main_Sonar");
    ASSERT_EQ(sp->getSensor(sensorID)->Name, "Main_Sonar", "Sensor name should be updated");

    // Test getSensor
    Sensor* s = sp->getSensor(sensorID);
    ASSERT_NEQ(s, nullptr, "getSensor should return the correct pointer");

    // Test remove
    sp->removeSubComponent(sensorID);
    ASSERT_TRUE(sp->sensors->empty(), "Sensors map should be empty after removal");
    ASSERT_EQ(sp->getSensor(sensorID), nullptr, "getSensor should return null after removal");

    delete sp;
    delete h;
}

void test_SensorProfile_JSON_Logic() {
    std::cout << "\n--- Running SensorProfile JSON Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    SensorProfile* sp = new SensorProfile(h);

    sp->addSubComponent("Radar_1", "Generic");
    sp->addSubComponent("AESA_1", "AESA");

    // Test toJson
    QJsonObject root = sp->toJson();
    ASSERT_TRUE(root.contains("sensors"), "JSON should contain 'sensors' object");

    QJsonObject sensorsObj = root["sensors"].toObject();
    ASSERT_EQ(sensorsObj.size(), 2, "JSON should contain exactly 2 sensors");

    // Test fromJson
    SensorProfile* sp2 = new SensorProfile(h);
    try {
        sp2->fromJson(root);
        ASSERT_EQ(sp2->sensors->size(), 2, "fromJson should recreate both sensors");

        // Check if types are preserved (AESA should be recreated as AESA)
        bool aesaFound = false;
        for(auto const& [id, sensor] : *sp2->sensors) {
            if(sensor->Name == "AESA_1") aesaFound = true;
        }
        ASSERT_TRUE(aesaFound, "Restored sensors should have correct names");
    } catch(...) {
        ASSERT_FALSE(true, "fromJson crashed");
    }

    delete sp;
    delete sp2;
    delete h;
}

void SensorProfile_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "      SENSORPROFILE CUSTOM UNIT TESTS    " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_SensorProfile_initialization();
    test_SensorProfile_Adding_Sensors();
    test_SensorProfile_Operations();
    test_SensorProfile_JSON_Logic();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
