#include "core/Hierarchy/Components/mesh.h"
#include <iostream>
#include <string>

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
// TEST SUITES FOR MESH
// ==========================================

void test_Mesh_initialization() {
    std::cout << "\n--- Running Mesh Initialization Tests ---" << std::endl;
    Mesh* m = new Mesh();

    ASSERT_NEQ(m, nullptr, "Mesh should return an object");
    ASSERT_EQ(m->ImageScale, 0, "Default ImageScale should be 0");
    ASSERT_TRUE(m->polygen.empty(), "Default polygen vector should be empty");

    delete m;
}

void test_Mesh_polygen_functions() {
    std::cout << "\n--- Running Mesh Polygen Tests ---" << std::endl;
    Mesh* m = new Mesh();

    // Create dummy Vector pointers to test the logic
    // (Assuming Vector is properly defined elsewhere)
    Vector* dummyV1 = (Vector*)0x1;
    Vector* dummyV2 = (Vector*)0x2;

    // Test addPoint
    m->addPoint(dummyV1);
    ASSERT_EQ(m->polygen.size(), 1, "addPoint should increase polygen size to 1");

    m->addPoint(dummyV2);
    ASSERT_EQ(m->polygen.size(), 2, "addPoint should increase polygen size to 2");
    ASSERT_EQ(m->polygen.back(), dummyV2, "Last point added should be dummyV2");

    // Test removePoint
    m->removePoint();
    ASSERT_EQ(m->polygen.size(), 1, "removePoint should decrease polygen size to 1");
    ASSERT_EQ(m->polygen.back(), dummyV1, "Last point remaining should be dummyV1");

    // Test clear
    m->clear();
    ASSERT_TRUE(m->polygen.empty(), "clear should empty the polygen vector completely");

    // Edge case: removePoint on empty vector
    try {
        m->removePoint();
        ASSERT_TRUE(true, "(Edge Case) removePoint on empty vector did not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Edge Case) removePoint on empty vector crashed");
    }

    delete m;
}

void test_Mesh_getPixmap() {
    std::cout << "\n--- Running Mesh getPixmap Tests ---" << std::endl;
    Mesh* m = new Mesh();

    // Prepare string pointer to avoid crash on "lastpath != Sprite->data()"
    std::string testPath = "dummy_path.png";
    m->Sprite = &testPath;

    try {
        // Test first allocation and scaling
        QPixmap* px1 = m->getPixmap(10);
        ASSERT_NEQ(px1, nullptr, "getPixmap should return a valid QPixmap pointer");
        ASSERT_EQ(m->ImageScale, 10, "ImageScale should update to 10");
        ASSERT_EQ(m->lastpath, testPath, "lastpath should update to Sprite data");

        // Test resize logic
        QPixmap* px2 = m->getPixmap(25);
        ASSERT_NEQ(px2, nullptr, "getPixmap should return valid QPixmap after resize");
        ASSERT_EQ(m->ImageScale, 25, "ImageScale should update to new size 25");

    } catch (...) {
        // This can happen if Qt's QGuiApplication is not instantiated in the test runner
        ASSERT_FALSE(true, "getPixmap crashed (Check if QGuiApplication is running)");
    }

    delete m;
}

// NOTE: toJson() and fromJson() are skipped as they are not implemented in mesh.cpp

// ==========================================
// MAIN RUNNER
// ==========================================
void Mesh_test() {
    // Reset global test counters
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "          MESH CUSTOM UNIT TESTS         " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_Mesh_initialization();
    test_Mesh_polygen_functions();
    test_Mesh_getPixmap();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
