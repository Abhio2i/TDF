#include "core/Hierarchy/Components/meshrenderer2d.h"
#include "core/Hierarchy/hierarchy.h"
#include <iostream>
#include <QJsonObject>
#include <QJsonDocument>

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
// TEST SUITES FOR MESHRENDERER2D
// ==========================================

void test_MeshRenderer2D_initialization() {
    std::cout << "\n--- Running MeshRenderer2D Initialization Tests ---" << std::endl;

    MeshRenderer2D* mr = new MeshRenderer2D();

    ASSERT_NEQ(mr, nullptr, "MeshRenderer2D object should be created");
    ASSERT_TRUE(mr->Active, "Default Active status should be true");
    ASSERT_NEQ(mr->color, nullptr, "Color pointer should be initialized");
    ASSERT_EQ(mr->color->name().toStdString(), "#ff0000", "Default color should be red");

    // Check initial mesh
    ASSERT_EQ(mr->Meshes.size(), 1, "Should initialize with 1 default mesh");
    ASSERT_EQ(mr->Meshes[0]->lineWidth, 2.0f, "Default mesh lineWidth should be 2");

    delete mr;
}

void test_MeshRenderer2D_JSON_Logic() {
    std::cout << "\n--- Running MeshRenderer2D JSON Tests ---" << std::endl;

    MeshRenderer2D* mr = new MeshRenderer2D();
    mr->ID = "test_mr_001";
    mr->Active = false;
    *(mr->Sprite) = "custom_sprite.png";

    // 1. Test toJson
    QJsonObject root = mr->toJson();
    ASSERT_FALSE(root.isEmpty(), "toJson should return a valid object");
    ASSERT_EQ(root["active"].toBool(), false, "JSON should reflect updated Active status");
    ASSERT_EQ(root["sprite"].toObject()["value"].toString().toStdString(), "custom_sprite.png", "JSON should contain custom sprite path");

    // 2. Test fromJson
    MeshRenderer2D* mr2 = new MeshRenderer2D();
    try {
        mr2->fromJson(root);
        ASSERT_FALSE(mr2->Active, "fromJson should restore Active=false");
        ASSERT_EQ(*(mr2->Sprite), "custom_sprite.png", "fromJson should restore sprite path");
        ASSERT_EQ(mr2->color->name().toStdString(), "#ff0000", "fromJson should restore color");
    } catch (...) {
        ASSERT_FALSE(true, "fromJson crashed during execution");
    }

    // 3. Test Custom Parameters Persistence
    QJsonObject customData;
    customData["custom_param"] = 123;
    mr->fromJson(customData); // Injection via fromJson logic

    QJsonObject rootWithCustom = mr->toJson();
    ASSERT_TRUE(rootWithCustom.contains("custom_param"), "toJson should preserve custom parameters");

    delete mr;
    delete mr2;
}

void test_MeshRenderer2D_Component_Interface() {
    std::cout << "\n--- Running Component Interface Tests ---" << std::endl;
    MeshRenderer2D* mr = new MeshRenderer2D();

    // Testing unimplemented functions (should not crash)
    try {
        mr->addSubComponent("test");
        mr->removeSubComponent("id");
        mr->updateSubComponent("id", QJsonObject());
        ASSERT_TRUE(true, "Unimplemented sub-component functions did not crash");
    } catch (...) {
        ASSERT_FALSE(true, "Unimplemented functions crashed");
    }

    QJsonObject data = mr->getsubComponentData("any");
    ASSERT_TRUE(data.isEmpty(), "getsubComponentData should return empty JSON as it is unimplemented");

    delete mr;
}

// ==========================================
// MAIN RUNNER
// ==========================================
void MeshRenderer2D_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "    MESHRENDERER2D CUSTOM UNIT TESTS     " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_MeshRenderer2D_initialization();
    test_MeshRenderer2D_JSON_Logic();
    test_MeshRenderer2D_Component_Interface();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
