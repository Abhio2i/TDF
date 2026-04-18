#include "core/Debug/frame.h"
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

void test_frame_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;

    Frame* f = new Frame();

    // All int fields should default to 0 via in-class initializers
    ASSERT_EQ(f->excutionTime,      0, "Default excutionTime should be 0");
    ASSERT_EQ(f->GUITime,           0, "Default GUITime should be 0");
    ASSERT_EQ(f->canvasTime,        0, "Default canvasTime should be 0");
    ASSERT_EQ(f->physicsTime,       0, "Default physicsTime should be 0");
    ASSERT_EQ(f->dynamicTime,       0, "Default dynamicTime should be 0");
    ASSERT_EQ(f->SensorTime,        0, "Default SensorTime should be 0");
    ASSERT_EQ(f->RadarTime,         0, "Default RadarTime should be 0");
    ASSERT_EQ(f->EWTime,            0, "Default EWTime should be 0");
    ASSERT_EQ(f->CSMTime,           0, "Default CSMTime should be 0");
    ASSERT_EQ(f->ESMTime,           0, "Default ESMTime should be 0");
    ASSERT_EQ(f->IFFTime,           0, "Default IFFTime should be 0");
    ASSERT_EQ(f->RadioTime,         0, "Default RadioTime should be 0");
    ASSERT_EQ(f->totalEntity,       0, "Default totalEntity should be 0");
    ASSERT_EQ(f->totalSensor,       0, "Default totalSensor should be 0");
    ASSERT_EQ(f->totalRadio,        0, "Default totalRadio should be 0");
    ASSERT_EQ(f->totalIff,          0, "Default totalIff should be 0");
    ASSERT_EQ(f->csmdisplay,        0, "Default csmdisplay should be 0");
    ASSERT_EQ(f->esmdisplay,        0, "Default esmdisplay should be 0");
    ASSERT_EQ(f->totaldynamicModel, 0, "Default totaldynamicModel should be 0");

    // NOTE: excutesLogs is declared as a pointer but the constructor does NOT
    // allocate it (no 'new' in Frame()). It is uninitialized — accessing it
    // would be undefined behavior. This test documents that known state.
    // Do NOT dereference excutesLogs in tests.

    delete f;
}

void test_frame_analyze() {
    std::cout << "\n--- Running analyze() Tests ---" << std::endl;

    // analyze() sets excutionTime = canvasTime + physicsTime
    Frame* f = new Frame();
    f->canvasTime  = 10;
    f->physicsTime = 20;
    f->analyze();
    ASSERT_EQ(f->excutionTime, 30, "(Correct) analyze() excutionTime should be canvasTime+physicsTime");

    // Both zero — result should be zero
    f->canvasTime  = 0;
    f->physicsTime = 0;
    f->analyze();
    ASSERT_EQ(f->excutionTime, 0, "(Both zero) analyze() excutionTime should be 0");

    // Only canvasTime set
    f->canvasTime  = 50;
    f->physicsTime = 0;
    f->analyze();
    ASSERT_EQ(f->excutionTime, 50, "(Only canvas) analyze() excutionTime should equal canvasTime");

    // Only physicsTime set
    f->canvasTime  = 0;
    f->physicsTime = 75;
    f->analyze();
    ASSERT_EQ(f->excutionTime, 75, "(Only physics) analyze() excutionTime should equal physicsTime");

    // Large values — should not overflow for reasonable frame times
    f->canvasTime  = 500;
    f->physicsTime = 500;
    f->analyze();
    ASSERT_EQ(f->excutionTime, 1000, "(Large values) analyze() excutionTime should be 1000");

    // analyze() should not touch any other timing fields
    f->canvasTime  = 10;
    f->physicsTime = 5;
    f->GUITime     = 99;
    f->dynamicTime = 88;
    f->analyze();
    ASSERT_EQ(f->GUITime,     99, "analyze() should not modify GUITime");
    ASSERT_EQ(f->dynamicTime, 88, "analyze() should not modify dynamicTime");

    // analyze() should not crash
    try {
        f->analyze();
        ASSERT_TRUE(true, "analyze() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "analyze() crashed");
    }

    delete f;
}

void test_frame_clean() {
    std::cout << "\n--- Running clean() Tests ---" << std::endl;

    Frame* f = new Frame();

    // Set all fields to non-zero values
    f->number          = 42;
    f->excutionTime    = 100;
    f->GUITime         = 200;
    f->canvasTime      = 300;
    f->physicsTime     = 400;
    f->dynamicTime     = 500;
    f->SensorTime      = 600;
    f->RadarTime       = 700;
    f->EWTime          = 800;
    f->CSMTime         = 900;
    f->ESMTime         = 110;
    f->IFFTime         = 120;
    f->RadioTime       = 130;
    f->totalEntity     = 10;
    f->totalSensor     = 20;
    f->totalRadio      = 30;
    f->totalIff        = 40;
    f->csmdisplay      = 50;
    f->esmdisplay      = 60;
    f->totaldynamicModel = 70;

    f->clean();

    // All int fields should be reset to 0
    ASSERT_EQ(f->number,           0, "clean() should reset number to 0");
    ASSERT_EQ(f->excutionTime,     0, "clean() should reset excutionTime to 0");
    ASSERT_EQ(f->GUITime,          0, "clean() should reset GUITime to 0");
    ASSERT_EQ(f->canvasTime,       0, "clean() should reset canvasTime to 0");
    ASSERT_EQ(f->physicsTime,      0, "clean() should reset physicsTime to 0");
    ASSERT_EQ(f->dynamicTime,      0, "clean() should reset dynamicTime to 0");
    ASSERT_EQ(f->SensorTime,       0, "clean() should reset SensorTime to 0");
    ASSERT_EQ(f->RadarTime,        0, "clean() should reset RadarTime to 0");
    ASSERT_EQ(f->EWTime,           0, "clean() should reset EWTime to 0");
    ASSERT_EQ(f->CSMTime,          0, "clean() should reset CSMTime to 0");
    ASSERT_EQ(f->ESMTime,          0, "clean() should reset ESMTime to 0");
    ASSERT_EQ(f->IFFTime,          0, "clean() should reset IFFTime to 0");
    ASSERT_EQ(f->RadioTime,        0, "clean() should reset RadioTime to 0");
    ASSERT_EQ(f->totalEntity,      0, "clean() should reset totalEntity to 0");
    ASSERT_EQ(f->totalSensor,      0, "clean() should reset totalSensor to 0");
    ASSERT_EQ(f->totalRadio,       0, "clean() should reset totalRadio to 0");
    ASSERT_EQ(f->totalIff,         0, "clean() should reset totalIff to 0");
    ASSERT_EQ(f->csmdisplay,       0, "clean() should reset csmdisplay to 0");
    ASSERT_EQ(f->esmdisplay,       0, "clean() should reset esmdisplay to 0");
    ASSERT_EQ(f->totaldynamicModel,0, "clean() should reset totaldynamicModel to 0");

    // clean() called on already-zero frame should not crash
    try {
        f->clean();
        ASSERT_TRUE(true, "clean() on already-clean frame should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "clean() on already-clean frame crashed");
    }

    // clean() followed by analyze() should still work correctly
    f->canvasTime  = 15;
    f->physicsTime = 25;
    f->analyze();
    ASSERT_EQ(f->excutionTime, 40, "analyze() after clean() should compute correctly");
    f->clean();
    ASSERT_EQ(f->excutionTime, 0, "clean() after analyze() should reset excutionTime to 0");

    delete f;
}

void test_frame_excutesLog_struct() {
    std::cout << "\n--- Running ExcutesLog Struct Tests ---" << std::endl;

    // ExcutesLog struct should be constructible and fields accessible
    ExcutesLog log;
    log.number        = 1;
    log.startTime     = "00:00:00";
    log.endTime       = "00:00:01";
    log.excutionTime  = "1000ms";

    ASSERT_EQ(log.number, 1, "ExcutesLog number should be 1");
    ASSERT_EQ(log.startTime,    std::string("00:00:00"), "ExcutesLog startTime should be '00:00:00'");
    ASSERT_EQ(log.endTime,      std::string("00:00:01"), "ExcutesLog endTime should be '00:00:01'");
    ASSERT_EQ(log.excutionTime, std::string("1000ms"),   "ExcutesLog excutionTime should be '1000ms'");

    // Struct should support copy
    ExcutesLog log2 = log;
    ASSERT_EQ(log2.number, 1, "ExcutesLog copy number should be 1");
    ASSERT_EQ(log2.startTime, std::string("00:00:00"), "ExcutesLog copy startTime should match");
}


void frame_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "        FRAME CUSTOM UNIT TESTS          " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_frame_initialization();
    test_frame_analyze();
    test_frame_clean();
    test_frame_excutesLog_struct();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
