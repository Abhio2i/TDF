#include "core/Hierarchy/Components/transform.h"
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

#define ASSERT_NEQ(val1, val2, testName) ASSERT_TRUE((val1) != (val2), testName)

// ==========================================
// TEST SUITES FOR TRANSFORM
// ==========================================

void test_Transform_initialization() {
    std::cout << "\n--- Running Transform Initialization Tests ---" << std::endl;
    Transform* t = new Transform();

    ASSERT_NEQ(t, nullptr, "Transform object should be created");
    ASSERT_TRUE(t->Active, "Transform should be active by default");
    ASSERT_FALSE(t->ID.empty(), "Transform should have a generated ID");

    // Default Delhi coordinates check (from constructor logic)
    ASSERT_NEQ(t->geocord, nullptr, "Geocords should be initialized");

    delete t;
}

void test_Transform_GeoLogic() {
    std::cout << "\n--- Running Transform Geo-Coordinate Tests ---" << std::endl;
    Transform* t = new Transform();

    t->setGeoCord(28.6139f, 77.2090f, 100.0f);

    ASSERT_EQ(t->getLatitude(), 28.6139f, "Latitude should be set correctly");
    ASSERT_EQ(t->getLongitude(), 77.2090f, "Longitude should be set correctly");
    // getAltitude() function check (if implemented, otherwise direct access)
    ASSERT_EQ(t->geocord->altitude, 100.0f, "Altitude should be set correctly");

    delete t;
}

void test_Transform_Rotation_Math() {
    std::cout << "\n--- Running Transform Rotation Math Tests ---" << std::endl;
    Transform* t = new Transform();

    QVector3D rotation(10.0f, 20.0f, 30.0f);
    t->setFromEulerAngles(rotation);

    QVector3D result = t->toEulerAngles();

    // Floating point values check (using a small margin or direct EQ if implementation is precise)
    ASSERT_TRUE(std::abs(result.x() - 10.0f) < 0.01f, "Pitch (X) should match");
    ASSERT_TRUE(std::abs(result.y() - 20.0f) < 0.01f, "Yaw (Y) should match");
    ASSERT_TRUE(std::abs(result.z() - 30.0f) < 0.01f, "Roll (Z) should match");

    delete t;
}

void test_Transform_JSON_Serialization() {
    std::cout << "\n--- Running Transform JSON Tests ---" << std::endl;
    Transform* t = new Transform();
    t->setGeoCord(15.0f, 25.0f, 500.0f);
    t->Active = false;

    // Test toJson
    QJsonObject obj = t->toJson();
    ASSERT_FALSE(obj.isEmpty(), "toJson should return a valid QJsonObject");
    ASSERT_TRUE(obj.contains("geocord"), "JSON should contain geocord data");
    ASSERT_EQ(obj["active"].toBool(), false, "JSON should reflect Active status");

    // Test fromJson
    Transform* t2 = new Transform();
    try {
        t2->fromJson(obj);
        ASSERT_FALSE(t2->Active, "fromJson should restore Active status");
        ASSERT_EQ(t2->getLatitude(), 15.0f, "fromJson should restore Latitude");
        ASSERT_EQ(t2->geocord->altitude, 500.0f, "fromJson should restore Altitude");
    } catch (...) {
        ASSERT_FALSE(true, "fromJson crashed");
    }

    delete t;
    delete t2;
}

void test_Transform_PDU_Network() {
    std::cout << "\n--- Running Transform PDU (Network) Tests ---" << std::endl;
    Transform* t = new Transform();
    TransformPDU pdu;

    t->setGeoCord(12.34f, 56.78f, 1000.0f);
    t->matrix->setTranslation(QVector3D(1, 2, 3));

    // toPDU
    t->toPDU(pdu, "entity_123", "parent_456");
    ASSERT_EQ(pdu.posX, 1.0f, "PDU should store correct X translation");
    ASSERT_EQ(pdu.posY, 2.0f, "PDU should store correct Y translation");

    // fromPDU
    Transform* t2 = new Transform();
    t2->fromPDU(pdu);
    ASSERT_EQ(t2->matrix->translation().x(), 1.0f, "fromPDU should restore X translation");

    delete t;
    delete t2;
}

// ==========================================
// MAIN RUNNER
// ==========================================
void Transform_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "      TRANSFORM CUSTOM UNIT TESTS       " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_Transform_initialization();
    test_Transform_GeoLogic();
    test_Transform_Rotation_Math();
    test_Transform_JSON_Serialization();
    test_Transform_PDU_Network();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
