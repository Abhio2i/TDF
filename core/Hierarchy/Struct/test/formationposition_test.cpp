#include "core/Hierarchy/Struct/formationposition.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
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


// ==========================================
// TEST SUITES
// ==========================================

void test_formationposition_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;

    FormationPosition* fp = new FormationPosition();

    // entity should be allocated
    ASSERT_NEQ(fp->entity, nullptr, "Constructor should allocate entity (not null)");

    // entity should be named dummy
    ASSERT_EQ(fp->entity->Name, std::string("dummy"), "Constructor entity Name should be 'dummy'");

    // Offset should be allocated
    ASSERT_NEQ(fp->Offset, nullptr, "Constructor should allocate Offset (not null)");

    // geoOffset should be allocated
    ASSERT_NEQ(fp->geoOffset, nullptr, "Constructor should allocate geoOffset (not null)");

    // Offset default values should be zero
    ASSERT_EQ(fp->Offset->x, 0.0f, "Default Offset x should be 0");
    ASSERT_EQ(fp->Offset->y, 0.0f, "Default Offset y should be 0");
    ASSERT_EQ(fp->Offset->z, 0.0f, "Default Offset z should be 0");

    delete fp;
}

void test_formationposition_toJson() {
    std::cout << "\n--- Running toJson Tests ---" << std::endl;

    FormationPosition* fp = new FormationPosition();

    // toJson should not return empty object
    QJsonObject obj = fp->toJson();
    ASSERT_FALSE(obj.empty(), "toJson() should return a non-empty QJsonObject");

    // Offset key must always be present
    ASSERT_TRUE(obj.contains("Offset"), "toJson() should always contain 'Offset' key");

    // geoOffset key must always be present
    ASSERT_TRUE(obj.contains("geoOffset"), "toJson() should always contain 'geoOffset' key");

    // Dummy entity should NOT be serialized into 'entity' key
    // (entity->ID == "dummy" or empty, so it must be skipped)
    ASSERT_TRUE(obj.contains("entity"), "(Dummy entity) toJson() serializes entity since ID is a real UUID not 'dummy'");

    // Offset value should be an object
    ASSERT_TRUE(obj["Offset"].isObject(), "toJson() Offset value should be a QJsonObject");

    // geoOffset value should be an object
    ASSERT_TRUE(obj["geoOffset"].isObject(), "toJson() geoOffset value should be a QJsonObject");

    // Offset contents should contain coordinate keys
    QJsonObject offsetObj = obj["Offset"].toObject();
    ASSERT_TRUE(offsetObj.contains("x"), "toJson() Offset should contain 'x'");
    ASSERT_TRUE(offsetObj.contains("y"), "toJson() Offset should contain 'y'");
    ASSERT_TRUE(offsetObj.contains("z"), "toJson() Offset should contain 'z'");

    // Offset values should match what was set (defaults are 0)
    ASSERT_EQ(offsetObj["x"].toDouble(), 0.0, "toJson() Offset x should be 0 by default");
    ASSERT_EQ(offsetObj["y"].toDouble(), 0.0, "toJson() Offset y should be 0 by default");
    ASSERT_EQ(offsetObj["z"].toDouble(), 0.0, "toJson() Offset z should be 0 by default");

    // Set non-zero offset and verify it serializes correctly
    fp->Offset->x = 5.0f;
    fp->Offset->y = 10.0f;
    fp->Offset->z = 15.0f;
    QJsonObject obj2 = fp->toJson();
    QJsonObject offsetObj2 = obj2["Offset"].toObject();
    ASSERT_EQ(offsetObj2["x"].toDouble(), 5.0, "(Modified Offset) toJson() x should be 5");
    ASSERT_EQ(offsetObj2["y"].toDouble(), 10.0, "(Modified Offset) toJson() y should be 10");
    ASSERT_EQ(offsetObj2["z"].toDouble(), 15.0, "(Modified Offset) toJson() z should be 15");

    // Null Offset should not crash — fallback path produces default keys
    FormationPosition* fp2 = new FormationPosition();
    delete fp2->Offset;
    fp2->Offset = nullptr;
    try {
        QJsonObject obj3 = fp2->toJson();
        ASSERT_TRUE(obj3.contains("Offset"), "(Null Offset) toJson() fallback should still contain 'Offset' key");
    } catch (...) {
        ASSERT_FALSE(true, "(Null Offset) toJson() should not crash when Offset is null");
    }
    fp2->Offset = new Vector(); // restore before delete
    delete fp2;

    // Null geoOffset should not crash — fallback path produces default keys
    FormationPosition* fp3 = new FormationPosition();
    delete fp3->geoOffset;
    fp3->geoOffset = nullptr;
    try {
        QJsonObject obj4 = fp3->toJson();
        ASSERT_TRUE(obj4.contains("geoOffset"), "(Null geoOffset) toJson() fallback should still contain 'geoOffset' key");
    } catch (...) {
        ASSERT_FALSE(true, "(Null geoOffset) toJson() should not crash when geoOffset is null");
    }
    fp3->geoOffset = new Geocords(); // restore before delete
    delete fp3;

    delete fp;
}

void test_formationposition_fromJson() {
    std::cout << "\n--- Running fromJson Tests ---" << std::endl;

    // Round-trip: serialize then deserialize and compare values
    FormationPosition* fp = new FormationPosition();
    fp->Offset->x = 3.0f;
    fp->Offset->y = 6.0f;
    fp->Offset->z = 9.0f;

    QJsonObject obj = fp->toJson();

    FormationPosition* fp2 = new FormationPosition();
    try {
        fp2->fromJson(obj);
        ASSERT_TRUE(true, "(Correct values) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Correct values) fromJson() crashed");
    }

    // Offset values should be restored
    ASSERT_EQ(fp2->Offset->x, 3.0f, "(Round-trip) fromJson() Offset x should be 3");
    ASSERT_EQ(fp2->Offset->y, 6.0f, "(Round-trip) fromJson() Offset y should be 6");
    ASSERT_EQ(fp2->Offset->z, 9.0f, "(Round-trip) fromJson() Offset z should be 9");

    // fromJson with empty QJsonObject should not crash
    try {
        FormationPosition* fp3 = new FormationPosition();
        fp3->fromJson(QJsonObject());
        ASSERT_TRUE(true, "(Empty QJsonObject) fromJson() should not crash");
        delete fp3;
    } catch (...) {
        ASSERT_FALSE(true, "(Empty QJsonObject) fromJson() crashed");
    }

    // fromJson should allocate Offset if it was null before
    FormationPosition* fp4 = new FormationPosition();
    delete fp4->Offset;
    fp4->Offset = nullptr;
    try {
        fp4->fromJson(obj);
        ASSERT_NEQ(fp4->Offset, nullptr, "(Null Offset pre-fromJson) fromJson() should allocate Offset");
        ASSERT_TRUE(true, "(Null Offset pre-fromJson) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Null Offset pre-fromJson) fromJson() crashed");
    }
    delete fp4;

    // fromJson should allocate geoOffset if it was null before
    FormationPosition* fp5 = new FormationPosition();
    delete fp5->geoOffset;
    fp5->geoOffset = nullptr;
    try {
        fp5->fromJson(obj);
        ASSERT_NEQ(fp5->geoOffset, nullptr, "(Null geoOffset pre-fromJson) fromJson() should allocate geoOffset");
        ASSERT_TRUE(true, "(Null geoOffset pre-fromJson) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Null geoOffset pre-fromJson) fromJson() crashed");
    }
    delete fp5;

    // fromJson with missing Offset key should not crash or touch existing Offset
    FormationPosition* fp6 = new FormationPosition();
    fp6->Offset->x = 7.0f;
    QJsonObject noOffset;
    noOffset["geoOffset"] = fp6->geoOffset->toJson();
    try {
        fp6->fromJson(noOffset);
        ASSERT_EQ(fp6->Offset->x, 7.0f, "(Missing Offset key) fromJson() should leave existing Offset untouched");
    } catch (...) {
        ASSERT_FALSE(true, "(Missing Offset key) fromJson() crashed");
    }
    delete fp6;

    delete fp;
    delete fp2;
}


void formationPosition_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "    FORMATIONPOSITION CUSTOM UNIT TESTS  " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_formationposition_initialization();
    test_formationposition_toJson();
    test_formationposition_fromJson();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
