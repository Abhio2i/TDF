// =============================================================================
// FILE:         issst_test_framework.h
// MODULE:       Test Framework — Shared Assertion Macros
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
//
// DESCRIPTION:  Provides assertion macros for use in ISSST unit test files.
//               This header contains ONLY macro definitions — no variable
//               definitions and no extern declarations.
//
//               The counter variables (testsPassed, testsFailed, tests) are
//               already declared extern and defined in the legacy test build
//               (aesaradar_test.cpp / core_test.cpp). New test files must
//               declare them extern themselves if needed, or rely on the
//               legacy build providing them at link time.
//
//               DO NOT modify aesaradar_test.cpp or core_test.cpp.
//               DO NOT redefine counters here — that would cause duplicate
//               definition linker errors.
//
// USAGE:
//   In every NEW test .cpp (e.g. radarantenna_aesa_test.cpp):
//
//     #include "issst_test_framework.h"
//     extern int testsPassed;
//     extern int testsFailed;
//     extern int tests;
//
//   Then declare your test entry function extern in core_test.cpp:
//
//     extern void radarAntenna_test();
//
//   And call it in Core_Test::Core_Test() alongside the other suites,
//   accumulating totals the same way all other suites do.
//
// REQUIREMENTS: REQ-AESA-TEST-001  Consistent assertion macro behaviour
//               REQ-AESA-TEST-002  No duplicate symbol definitions
//
// CHANGE HISTORY:
//   Rev 1  20 Apr 2026  Created. Macros extracted to shared header so new
//                       test translation units can use them without touching
//                       legacy aesaradar_test.cpp. Counter variables are
//                       intentionally NOT declared here — they live in the
//                       legacy build. NS-05 compliant — no dead code.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
// =============================================================================

#pragma once
#ifndef ISSST_TEST_FRAMEWORK_H
#define ISSST_TEST_FRAMEWORK_H

#include <iostream>
#include <cmath>

// =============================================================================
// ASSERT_TRUE
//
// DESCRIPTION: Passes if condition evaluates to true.
//              Prints [PASS] with test name on success.
//              Prints [FAIL] with test name and source line on failure.
//              Increments shared counters testsPassed/testsFailed/tests.
//              Counters must be declared extern by the including .cpp.
//
// PARAMETERS:
//   condition  — any boolean-convertible expression
//   testName   — string literal or std::string identifying the test case
//
// REQUIREMENT: REQ-AESA-TEST-001
// =============================================================================
#define ASSERT_TRUE(condition, testName)                                         \
do {                                                                         \
        if ((condition)) {                                                       \
            std::cout << tests << " [PASS] " << (testName) << std::endl;        \
            testsPassed++;                                                       \
    } else {                                                                 \
            std::cerr << tests << " [FAIL] " << (testName)                      \
            << " (Line: " << __LINE__ << ")" << std::endl;            \
            testsFailed++;                                                       \
    }                                                                        \
        tests++;                                                                 \
} while (0)

// =============================================================================
// ASSERT_FALSE
//
// DESCRIPTION: Passes if condition evaluates to false.
//              Implemented as ASSERT_TRUE with negated condition.
//
// REQUIREMENT: REQ-AESA-TEST-001
// =============================================================================
#define ASSERT_FALSE(condition, testName) \
    ASSERT_TRUE(!(condition), testName)

// =============================================================================
// ASSERT_EQ
//
// DESCRIPTION: Passes if val1 == val2 using operator==.
//              On failure prints both expected and actual values.
//
// PARAMETERS:
//   val1      — actual value produced by code under test
//   val2      — expected value from the requirement
//   testName  — string literal identifying the test case
//
// REQUIREMENT: REQ-AESA-TEST-001
// =============================================================================
#define ASSERT_EQ(val1, val2, testName)                                          \
    do {                                                                         \
        if ((val1) == (val2)) {                                                  \
            std::cout << tests << " [PASS] " << (testName) << std::endl;        \
            testsPassed++;                                                       \
    } else {                                                                 \
            std::cerr << tests << " [FAIL] " << (testName)                      \
            << " (Expected: " << (val2)                               \
            << ", Got: "      << (val1)                               \
            << ") (Line: "   << __LINE__ << ")" << std::endl;         \
            testsFailed++;                                                       \
    }                                                                        \
        tests++;                                                                 \
} while (0)

// =============================================================================
// ASSERT_NEQ
//
// DESCRIPTION: Passes if val1 != val2.
//
// REQUIREMENT: REQ-AESA-TEST-001
// =============================================================================
#define ASSERT_NEQ(val1, val2, testName) \
    ASSERT_TRUE((val1) != (val2), testName)

// =============================================================================
// ASSERT_NEAR
//
// DESCRIPTION: Passes if |val1 - val2| <= epsilon.
//              DO-178C FL-02 compliant — no direct == on floating-point.
//              Both values cast to double before comparison to avoid
//              implicit float/double mismatch warnings (TS-05 compliance).
//
// PARAMETERS:
//   val1      — actual value (double-convertible)
//   val2      — expected value (double-convertible)
//   epsilon   — maximum acceptable absolute difference (positive double)
//   testName  — string literal identifying the test case
//
// REQUIREMENT: REQ-AESA-TEST-001
// =============================================================================
#define ASSERT_NEAR(val1, val2, epsilon, testName)                               \
do {                                                                         \
        if (std::fabs((double)(val1) - (double)(val2)) <= (double)(epsilon)) {  \
            std::cout << tests << " [PASS] " << (testName) << std::endl;        \
            testsPassed++;                                                       \
    } else {                                                                 \
            std::cerr << tests << " [FAIL] " << (testName)                      \
            << " (Expected ~" << (double)(val2)                       \
            << ", Got: "      << (double)(val1)                       \
            << ", Eps: "      << (double)(epsilon)                    \
            << ") (Line: "   << __LINE__ << ")" << std::endl;         \
            testsFailed++;                                                       \
    }                                                                        \
        tests++;                                                                 \
} while (0)

#endif // ISSST_TEST_FRAMEWORK_H
