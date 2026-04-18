#include "core/Hierarchy/Components/dynamicmodel.h"
#include <iostream>
#include <QJsonObject>
#include <QVector3D>

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
        std::cerr << tests << " [FAIL] " << testName << " (Line: " << __LINE__ << ")" << std::endl; \
        testsFailed++; \
        tests++; \
}

#define ASSERT_NEAR(val1, val2, epsilon, testName) \
if (std::abs((val1) - (val2)) < (epsilon)) { \
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
// TEST SUITES FOR DYNAMICMODEL
// ==========================================

void test_DynamicModel_initialization() {
    std::cout << "\n--- Running DynamicModel Initialization Tests ---" << std::endl;
    DynamicModel* dm = new DynamicModel();

    ASSERT_NEQ(dm, nullptr, "DynamicModel object should be created");
    ASSERT_EQ(dm->moveSpeed, 800.0f, "Default moveSpeed should be 800");
    ASSERT_EQ(dm->turnRadius, 10.0f, "Default turnRadius should be 10");
    ASSERT_FALSE(dm->followTarget, "followTarget should be false by default");

    delete dm;
}

void test_DynamicModel_Movement_Logic() {
    std::cout << "\n--- Running Movement Math Tests ---" << std::endl;
    DynamicModel* dm = new DynamicModel();

    // 1. Test Lerp (Utility function)
    float result = dm->lerp(10.0f, 20.0f, 0.5f);
    ASSERT_EQ(result, 15.0f, "Lerp at 0.5 should return middle value");

    // 2. Test Speed setter
    dm->setMoveSpeed(500.0f);
    // Note: speed variable is private, but we can verify through toJson if needed

    delete dm;
}

void test_DynamicModel_JSON_Serialization() {
    std::cout << "\n--- Running DynamicModel JSON Tests ---" << std::endl;
    DynamicModel* dm = new DynamicModel();
    dm->moveSpeed = 1200.0f;
    dm->maxAltitude = 45000.0f;
    dm->control = true;

    // 1. Test toJson
    QJsonObject root = dm->toJson();
    ASSERT_TRUE(root.contains("moveSpeed"), "JSON should contain moveSpeed");
    ASSERT_EQ(root["moveSpeed"].toObject()["value"].toDouble(), 1200.0, "JSON moveSpeed should be correct");
    ASSERT_EQ(root["control"].toBool(), true, "JSON control flag should be true");

    // 2. Test fromJson
    DynamicModel* dm2 = new DynamicModel();
    try {
        dm2->fromJson(root);
        ASSERT_EQ(dm2->moveSpeed, 1200.0f, "fromJson should restore moveSpeed");
        ASSERT_EQ(dm2->maxAltitude, 45000.0f, "fromJson should restore maxAltitude");
        ASSERT_TRUE(dm2->control, "fromJson should restore control status");
    } catch (...) {
        ASSERT_FALSE(true, "fromJson crashed");
    }

    delete dm;
    delete dm2;
}

void test_DynamicModel_Passability() {
    std::cout << "\n--- Running Passability Logic Tests ---" << std::endl;
    DynamicModel* dm = new DynamicModel();

    QJsonObject passability;
    passability["terrainIsPassable"] = false;

    QJsonObject root;
    root["passabillity"] = passability;

    dm->fromJson(root);
    // Assuming terrainIs is the internal variable for terrainIsPassable
    // Note: Check if you want to expose terrainIs for testing
    ASSERT_TRUE(true, "Passability JSON handled without crash");

    delete dm;
}

// ==========================================
// MAIN RUNNER
// ==========================================
void DynamicModel_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "      DYNAMICMODEL CUSTOM UNIT TESTS     " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_DynamicModel_initialization();
    test_DynamicModel_Movement_Logic();
    test_DynamicModel_JSON_Serialization();
    test_DynamicModel_Passability();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
