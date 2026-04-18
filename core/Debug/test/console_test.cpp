#include "core/Debug/console.h"
#include <QJsonObject>
#include <QObject>
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

void test_console_singleton() {
    std::cout << "\n--- Running Singleton Tests ---" << std::endl;

    // internalInstance() should never return null
    ASSERT_NEQ(Console::internalInstance(), nullptr,
               "internalInstance() should return non-null pointer");

    // Same pointer every call — true singleton
    Console* a = Console::internalInstance();
    Console* b = Console::internalInstance();
    ASSERT_EQ(a, b, "internalInstance() should return the same pointer every time");

    // logList should be allocated
    ASSERT_NEQ(Console::internalInstance()->logList, nullptr,
               "logList should be allocated in constructor");
}

void test_console_log_string() {
    std::cout << "\n--- Running log(string) Tests ---" << std::endl;

    // Correct value — should not crash
    try {
        Console::log("test log message");
        ASSERT_TRUE(true, "(Correct) log(string) should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Correct) log(string) crashed");
    }

    // logList should contain "log" key after logging
    ASSERT_TRUE(Console::internalInstance()->logList->count("log") > 0,
                "log(string) should insert 'log' key into logList");

    // Empty string — should not crash
    try {
        Console::log("");
        ASSERT_TRUE(true, "(Empty string) log('') should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Empty string) log('') crashed");
    }

    // Long string — should not crash
    try {
        std::string longMsg(100, 'x');
        Console::log(longMsg);
        ASSERT_TRUE(true, "(Long string) log() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Long string) log() crashed");
    }

    // Special characters — should not crash
    try {
        Console::log("special: \n\t\r\\\"{}[]<>");
        ASSERT_TRUE(true, "(Special chars) log() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Special chars) log() crashed");
    }
}

void test_console_log_signal() {
    std::cout << "\n--- Running log Signal Tests ---" << std::endl;

    // Connect lambda to logUpdate signal — flag set when emitted
    bool logFired = false;
    std::string receivedMsg;
    QMetaObject::Connection conn = QObject::connect(
        Console::internalInstance(), &Console::logUpdate,
        [&](std::string msg) {
            logFired = true;
            receivedMsg = msg;
        }
        );

    Console::log("signal test message");
    ASSERT_TRUE(logFired, "log(string) should emit logUpdate signal");
    ASSERT_FALSE(receivedMsg.empty(), "logUpdate signal should carry non-empty message");

    // Signal fires again on second call
    logFired = false;
    Console::log("second message");
    ASSERT_TRUE(logFired, "Each log() call should emit logUpdate signal");

    QObject::disconnect(conn);
}

void test_console_log_json() {
    std::cout << "\n--- Running log(QJsonObject) Tests ---" << std::endl;

    // Correct value — non-empty QJsonObject
    try {
        QJsonObject obj;
        obj["key"] = "value";
        obj["number"] = 42;
        Console::log(obj);
        ASSERT_TRUE(true, "(Correct) log(QJsonObject) should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Correct) log(QJsonObject) crashed");
    }

    // logList should contain "log" key after json logging
    ASSERT_TRUE(Console::internalInstance()->logList->count("log") > 0,
                "log(QJsonObject) should insert 'log' key into logList");

    // Empty QJsonObject — should not crash
    try {
        Console::log(QJsonObject());
        ASSERT_TRUE(true, "(Empty QJsonObject) log() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Empty QJsonObject) log() crashed");
    }

    // Nested QJsonObject — should not crash
    try {
        QJsonObject inner;
        inner["x"] = 1.0;
        QJsonObject outer;
        outer["nested"] = inner;
        Console::log(outer);
        ASSERT_TRUE(true, "(Nested QJsonObject) log() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Nested QJsonObject) log() crashed");
    }

    // logUpdate signal emitted for json log
    bool jsonLogFired = false;
    QMetaObject::Connection conn = QObject::connect(
        Console::internalInstance(), &Console::logUpdate,
        [&](std::string) { jsonLogFired = true; }
        );
    QJsonObject obj;
    obj["test"] = "json signal";
    Console::log(obj);
    ASSERT_TRUE(jsonLogFired, "log(QJsonObject) should emit logUpdate signal");
    QObject::disconnect(conn);
}

void test_console_error() {
    std::cout << "\n--- Running error() Tests ---" << std::endl;

    // Correct value — should not crash
    try {
        Console::error("test error message");
        ASSERT_TRUE(true, "(Correct) error() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Correct) error() crashed");
    }

    // logList should contain "error" key
    ASSERT_TRUE(Console::internalInstance()->logList->count("error") > 0,
                "error() should insert 'error' key into logList");

    // errorUpdate signal should be emitted
    bool errorFired = false;
    std::string receivedError;
    QMetaObject::Connection conn = QObject::connect(
        Console::internalInstance(), &Console::errorUpdate,
        [&](std::string msg) {
            errorFired = true;
            receivedError = msg;
        }
        );
    Console::error("signal error");
    ASSERT_TRUE(errorFired, "error() should emit errorUpdate signal");
    ASSERT_FALSE(receivedError.empty(), "errorUpdate signal should carry non-empty message");
    QObject::disconnect(conn);

    // Empty string — should not crash
    try {
        Console::error("");
        ASSERT_TRUE(true, "(Empty string) error('') should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Empty string) error('') crashed");
    }

    // Long string — should not crash
    try {
        Console::error(std::string(100, 'e'));
        ASSERT_TRUE(true, "(Long string) error() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Long string) error() crashed");
    }
}

void test_console_warning() {
    std::cout << "\n--- Running warning() Tests ---" << std::endl;

    // Correct value — should not crash
    try {
        Console::warning("test warning message");
        ASSERT_TRUE(true, "(Correct) warning() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Correct) warning() crashed");
    }

    // logList should contain "warning" key
    ASSERT_TRUE(Console::internalInstance()->logList->count("warning") > 0,
                "warning() should insert 'warning' key into logList");

    // warningUpdate signal should be emitted
    bool warningFired = false;
    std::string receivedWarning;
    QMetaObject::Connection conn = QObject::connect(
        Console::internalInstance(), &Console::warningUpdate,
        [&](std::string msg) {
            warningFired = true;
            receivedWarning = msg;
        }
        );
    Console::warning("signal warning");
    ASSERT_TRUE(warningFired, "warning() should emit warningUpdate signal");
    ASSERT_FALSE(receivedWarning.empty(), "warningUpdate signal should carry non-empty message");
    QObject::disconnect(conn);

    // Empty string — should not crash
    try {
        Console::warning("");
        ASSERT_TRUE(true, "(Empty string) warning('') should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Empty string) warning('') crashed");
    }
}

void test_console_logList_behavior() {
    std::cout << "\n--- Running logList Behavior Tests ---" << std::endl;

    // logList should be a valid pointer
    ASSERT_NEQ(Console::internalInstance()->logList, nullptr,
               "logList pointer should be non-null");

    // NOTE: Known behavior — unordered_map::insert does NOT overwrite existing keys.
    // Only the FIRST message for each key type ("log", "error", "warning") is stored.
    // Subsequent calls with the same key type are silently dropped from the map.
    // This test documents that behavior.
    Console::internalInstance()->logList->clear();

    Console::log("first log");
    std::string firstStored = Console::internalInstance()->logList->at("log");

    Console::log("second log");
    std::string afterSecond = Console::internalInstance()->logList->at("log");

    // Because insert() doesn't overwrite, the stored value should still be "first log"
    ASSERT_EQ(firstStored, afterSecond,
              "(Known behavior) logList insert() does not overwrite — first log wins");

    // Each category stored under its own key
    Console::internalInstance()->logList->clear();
    Console::log("log entry");
    Console::error("error entry");
    Console::warning("warning entry");

    ASSERT_TRUE(Console::internalInstance()->logList->count("log")     > 0,
                "logList should have 'log' key after Console::log()");
    ASSERT_TRUE(Console::internalInstance()->logList->count("error")   > 0,
                "logList should have 'error' key after Console::error()");
    ASSERT_TRUE(Console::internalInstance()->logList->count("warning") > 0,
                "logList should have 'warning' key after Console::warning()");
}

void test_console_noopMethods() {
    std::cout << "\n--- Running No-op Methods Tests ---" << std::endl;

    Console* c = Console::internalInstance();

    try {
        c->saveLog();
        ASSERT_TRUE(true, "saveLog() should not crash (no-op)");
    } catch (...) {
        ASSERT_FALSE(true, "saveLog() crashed");
    }

    try {
        c->getLog();
        ASSERT_TRUE(true, "getLog() should not crash (no-op)");
    } catch (...) {
        ASSERT_FALSE(true, "getLog() crashed");
    }

    try {
        c->clearLog();
        ASSERT_TRUE(true, "clearLog() should not crash (no-op)");
    } catch (...) {
        ASSERT_FALSE(true, "clearLog() crashed");
    }
}


void console_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "       CONSOLE CUSTOM UNIT TESTS         " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_console_singleton();
    test_console_log_string();
    test_console_log_signal();
    test_console_log_json();
    test_console_error();
    test_console_warning();
    test_console_logList_behavior();
    test_console_noopMethods();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
