#include "core/Hierarchy/Components/radioprofile.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
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
        std::cerr << tests << " [FAIL] " << testName << " (Line: " << __LINE__ << ")" << std::endl; \
        testsFailed++; \
        tests++; \
}

#define ASSERT_NEQ(val1, val2, testName) ASSERT_TRUE((val1) != (val2), testName)

// ==========================================
// TEST SUITES FOR RADIOPROFILE
// ==========================================

void test_RadioProfile_initialization() {
    std::cout << "\n--- Running RadioProfile Initialization Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    RadioProfile* rp = new RadioProfile(h);

    ASSERT_NEQ(rp, nullptr, "RadioProfile object should be created");
    ASSERT_TRUE(rp->Active, "RadioProfile should be active by default");
    ASSERT_NEQ(rp->radios, nullptr, "Radios map should be initialized");
    ASSERT_TRUE(rp->radios->empty(), "Radios map should be empty initially");

    delete rp;
    delete h;
}

void test_RadioProfile_Subcomponent_Operations() {
    std::cout << "\n--- Running RadioProfile Subcomponent Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    RadioProfile* rp = new RadioProfile(h);

    // 1. Test addSubComponent
    rp->addSubComponent("Primary_Radio");
    ASSERT_EQ(rp->radios->size(), 1, "Radios size should be 1 after adding one radio");

    // Get the ID of the newly added radio
    std::string radioID = rp->radios->begin()->first;
    Radio* r = (*rp->radios)[radioID];
    ASSERT_EQ(r->Name, "Primary_Radio", "Radio name should match the input");

    // 2. Test renameSubComponent
    rp->renameSubComponent(radioID, "Backup_Radio");
    ASSERT_EQ(r->Name, "Backup_Radio", "Radio name should be updated to Backup_Radio");

    // 3. Test getsubComponentData
    QJsonObject data = rp->getsubComponentData(radioID);
    ASSERT_FALSE(data.isEmpty(), "getsubComponentData should return valid JSON");
    ASSERT_EQ(data["name"].toString().toStdString(), "Backup_Radio", "JSON data should have the correct name");

    // 4. Test updateSubComponent
    QJsonObject updateObj;
    updateObj["name"] = "Emergency_Radio";
    rp->updateSubComponent(radioID, updateObj);
    ASSERT_EQ(r->Name, "Emergency_Radio", "updateSubComponent should update the radio data");

    // 5. Test removeSubComponent
    rp->removeSubComponent(radioID);
    ASSERT_TRUE(rp->radios->empty(), "Radios map should be empty after removal");

    delete rp;
    delete h;
}

void test_RadioProfile_JSON_Serialization() {
    std::cout << "\n--- Running RadioProfile JSON Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    RadioProfile* rp = new RadioProfile(h);

    // Add some data
    rp->addSubComponent("Radio_A");
    rp->addSubComponent("Radio_B");

    // Test toJson
    QJsonObject root = rp->toJson();
    ASSERT_TRUE(root.contains("radios"), "JSON should contain 'radios' key");
    QJsonObject radiosObj = root["radios"].toObject();
    ASSERT_EQ(radiosObj.size(), 2, "JSON should contain 2 radio objects");

    // Test fromJson
    RadioProfile* rp2 = new RadioProfile(h);
    try {
        rp2->fromJson(root);
        ASSERT_EQ(rp2->radios->size(), 2, "fromJson should restore 2 radios from JSON");

        // Check if names are preserved
        bool foundA = false;
        for(auto const& [id, radio] : *rp2->radios) {
            if(radio->Name == "Radio_A") foundA = true;
        }
        ASSERT_TRUE(foundA, "fromJson should correctly restore radio names");

    } catch(...) {
        ASSERT_FALSE(true, "fromJson crashed during execution");
    }

    // Wrong Values Test
    try {
        rp2->fromJson(QJsonObject());
        ASSERT_TRUE(true, "fromJson with empty object should not crash");
    } catch(...) {
        ASSERT_FALSE(true, "fromJson with empty object crashed");
    }

    delete rp;
    delete rp2;
    delete h;
}

void RadioProfile_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "      RADIOPROFILE CUSTOM UNIT TESTS     " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_RadioProfile_initialization();
    test_RadioProfile_Subcomponent_Operations();
    test_RadioProfile_JSON_Serialization();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
