#include "core/Hierarchy/Components/iffprofile.h"
#include "core/Hierarchy/hierarchy.h"
#include <iostream>
#include <QJsonObject>

// // --- Custom Testing Framework Variables ---
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
// TEST SUITES FOR IFFPROFILE
// ==========================================

void test_IFFProfile_initialization() {
    std::cout << "\n--- Running IFFProfile Initialization Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    IFFProfile* ip = new IFFProfile(h);

    ASSERT_NEQ(ip, nullptr, "IFFProfile should be a valid object");
    ASSERT_TRUE(ip->Active, "IFFProfile should be active by default");
    ASSERT_NEQ(ip->iffs, nullptr, "IFFProfile internal map (iffs) should be initialized");
    ASSERT_TRUE(ip->iffs->empty(), "IFFProfile should start with an empty map");

    delete ip;
    delete h;
}

void test_IFFProfile_Subcomponent_Management() {
    std::cout << "\n--- Running IFFProfile Subcomponent Management Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    IFFProfile* ip = new IFFProfile(h);

    // Test addSubComponent
    try {
        ip->addSubComponent("Test_IFF_1");
        ASSERT_EQ(ip->iffs->size(), 1, "addSubComponent should add one IFF object");
    } catch(...) {
        ASSERT_FALSE(true, "addSubComponent crashed during execution");
    }

    // Get the ID of the added component for further tests
    std::string addedID = ip->iffs->begin()->first;

    // Test getsubComponentData
    QJsonObject data = ip->getsubComponentData(addedID);
    ASSERT_FALSE(data.isEmpty(), "getsubComponentData should return valid JSON for existing ID");

    QJsonObject wrongData = ip->getsubComponentData("non_existent_id");
    ASSERT_TRUE(wrongData.isEmpty(), "getsubComponentData should return empty JSON for wrong ID");

    // Test renameSubComponent
    ip->renameSubComponent(addedID, "Renamed_IFF");
    ASSERT_EQ((*ip->iffs)[addedID]->Name, std::string("Renamed_IFF"), "renameSubComponent should update the Name property");

    // Test removeSubComponent
    ip->removeSubComponent(addedID);
    ASSERT_TRUE(ip->iffs->empty(), "removeSubComponent should remove the item from the map");

    delete ip;
    delete h;
}

void test_IFFProfile_Serialization() {
    std::cout << "\n--- Running IFFProfile Serialization (JSON) Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    IFFProfile* ip = new IFFProfile(h);

    // Setup initial state
    ip->addSubComponent("Serial_Test");
    std::string originalID = ip->iffs->begin()->first;

    // Test toJson
    QJsonObject root = ip->toJson();
    ASSERT_TRUE(root.contains("iffs"), "toJson should contain 'iffs' key");
    ASSERT_TRUE(root["iffs"].toObject().contains(QString::fromStdString(originalID)), "JSON should contain the specific IFF ID");

    // Test fromJson
    IFFProfile* ip2 = new IFFProfile(h);
    try {
        ip2->fromJson(root);
        ASSERT_EQ(ip2->iffs->size(), 1, "fromJson should restore subcomponents from JSON");

        // Check if data is consistent
        std::string restoredID = ip2->iffs->begin()->first;
        ASSERT_EQ(restoredID, originalID, "Restored IFF ID should match original");
    } catch(...) {
        ASSERT_FALSE(true, "fromJson crashed during execution");
    }

    // Test fromJson with Empty Object (Robustness)
    try {
        ip2->fromJson(QJsonObject());
        ASSERT_TRUE(true, "fromJson with empty object should not crash");
    } catch(...) {
        ASSERT_FALSE(true, "fromJson with empty object crashed");
    }

    delete ip;
    delete ip2;
    delete h;
}

void IFFProfile_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "      IFFProfile CUSTOM UNIT TESTS       " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_IFFProfile_initialization();
    test_IFFProfile_Subcomponent_Management();
    test_IFFProfile_Serialization();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
