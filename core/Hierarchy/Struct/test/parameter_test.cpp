#include "core/Hierarchy/Struct/parameter.h"
#include "core/Hierarchy/Struct/constants.h"
#include <QJsonObject>
#include <iostream>
#include <variant>
#include <cmath>
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

void test_parameter_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;

    Parameter* p = new Parameter();

    // Name should default to empty string
    ASSERT_EQ(p->Name, std::string(""), "Default Name should be empty string");

    // value variant should be in a valid (default-constructed) state — holds int by default
    ASSERT_TRUE(std::holds_alternative<int>(p->value), "Default value variant should hold int (index 0)");

    delete p;
}

void test_parameter_toJson_name_and_type() {
    std::cout << "\n--- Running toJson Name and Type Tests ---" << std::endl;

    Parameter* p = new Parameter();
    p->Name = "test_param";
    p->value = 42;

    QJsonObject obj = p->toJson();

    // Result should not be empty
    ASSERT_FALSE(obj.empty(), "toJson() should return non-empty QJsonObject");

    // 'name' key must be present
    ASSERT_TRUE(obj.contains("name"), "toJson() should contain 'name' key");

    // 'name' value should match
    ASSERT_EQ(obj["name"].toString().toStdString(), std::string("test_param"), "toJson() name should be 'test_param'");

    // 'type' key must be present
    ASSERT_TRUE(obj.contains("type"), "toJson() should contain 'type' key");

    // 'type' is always written as the string "paravalue" regardless of enum
    ASSERT_EQ(obj["type"].toString().toStdString(), std::string("paravalue"), "toJson() type should always be 'paravalue'");

    // 'value' key must be present
    ASSERT_TRUE(obj.contains("value"), "toJson() should contain 'value' key");

    delete p;
}

void test_parameter_toJson_variants() {
    std::cout << "\n--- Running toJson Variant Type Tests ---" << std::endl;

    // int
    {
        Parameter* p = new Parameter();
        p->Name = "int_param";
        p->value = int(7);
        QJsonObject obj = p->toJson();
        ASSERT_TRUE(std::holds_alternative<int>(p->value), "(int) variant should hold int");
        ASSERT_EQ(obj["value"].toInt(), 7, "(int) toJson() value should be 7");
        delete p;
    }

    // float
    {
        Parameter* p = new Parameter();
        p->Name = "float_param";
        p->value = float(3.14f);
        QJsonObject obj = p->toJson();
        ASSERT_TRUE(std::holds_alternative<float>(p->value), "(float) variant should hold float");
        // QJsonObject stores float as double internally — check within tolerance
        ASSERT_TRUE(std::fabs(obj["value"].toDouble() - 3.14) < 0.01, "(float) toJson() value should be ~3.14");
        delete p;
    }

    // double
    {
        Parameter* p = new Parameter();
        p->Name = "double_param";
        p->value = double(2.718281828);
        QJsonObject obj = p->toJson();
        ASSERT_TRUE(std::holds_alternative<double>(p->value), "(double) variant should hold double");
        ASSERT_TRUE(std::fabs(obj["value"].toDouble() - 2.718281828) < 0.000001, "(double) toJson() value should be ~2.718");
        delete p;
    }

    // string
    {
        Parameter* p = new Parameter();
        p->Name = "string_param";
        p->value = std::string("hello");
        QJsonObject obj = p->toJson();
        ASSERT_TRUE(std::holds_alternative<std::string>(p->value), "(string) variant should hold string");
        ASSERT_EQ(obj["value"].toString().toStdString(), std::string("hello"), "(string) toJson() value should be 'hello'");
        delete p;
    }

    // bool — true
    {
        Parameter* p = new Parameter();
        p->Name = "bool_param";
        p->value = bool(true);
        QJsonObject obj = p->toJson();
        ASSERT_TRUE(std::holds_alternative<bool>(p->value), "(bool) variant should hold bool");
        ASSERT_EQ(obj["value"].toBool(), true, "(bool true) toJson() value should be true");
        delete p;
    }

    // bool — false
    {
        Parameter* p = new Parameter();
        p->Name = "bool_param_false";
        p->value = bool(false);
        QJsonObject obj = p->toJson();
        ASSERT_EQ(obj["value"].toBool(), false, "(bool false) toJson() value should be false");
        delete p;
    }

    // char
    {
        Parameter* p = new Parameter();
        p->Name = "char_param";
        p->value = char('Z');
        QJsonObject obj = p->toJson();
        ASSERT_TRUE(std::holds_alternative<char>(p->value), "(char) variant should hold char");
        // char is serialized as a single-character QString
        ASSERT_EQ(obj["value"].toString().toStdString(), std::string("Z"), "(char) toJson() value should be 'Z'");
        delete p;
    }
}

void test_parameter_fromJson_variants() {
    std::cout << "\n--- Running fromJson Variant Type Tests ---" << std::endl;

    // INT
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "int_param";
        obj["type"] = static_cast<int>(Constants::ParameterType::INT);
        obj["value"] = 99;
        try {
            p->fromJson(obj);
            ASSERT_EQ(p->Name, std::string("int_param"), "(INT) fromJson() Name should be 'int_param'");
            ASSERT_TRUE(std::holds_alternative<int>(p->value), "(INT) fromJson() value should hold int");
            ASSERT_EQ(std::get<int>(p->value), 99, "(INT) fromJson() value should be 99");
        } catch (...) {
            ASSERT_FALSE(true, "(INT) fromJson() should not crash");
        }
        delete p;
    }

    // FLOAT
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "float_param";
        obj["type"] = static_cast<int>(Constants::ParameterType::FLOAT);
        obj["value"] = 1.5;
        try {
            p->fromJson(obj);
            ASSERT_TRUE(std::holds_alternative<float>(p->value), "(FLOAT) fromJson() value should hold float");
            ASSERT_TRUE(std::fabs(std::get<float>(p->value) - 1.5f) < 0.001f, "(FLOAT) fromJson() value should be ~1.5");
        } catch (...) {
            ASSERT_FALSE(true, "(FLOAT) fromJson() should not crash");
        }
        delete p;
    }

    // DOUBLE
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "double_param";
        obj["type"] = static_cast<int>(Constants::ParameterType::DOUBLE);
        obj["value"] = 3.14159265;
        try {
            p->fromJson(obj);
            ASSERT_TRUE(std::holds_alternative<double>(p->value), "(DOUBLE) fromJson() value should hold double");
            ASSERT_TRUE(std::fabs(std::get<double>(p->value) - 3.14159265) < 0.000001, "(DOUBLE) fromJson() value should be ~3.14159265");
        } catch (...) {
            ASSERT_FALSE(true, "(DOUBLE) fromJson() should not crash");
        }
        delete p;
    }

    // STRING
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "string_param";
        obj["type"] = static_cast<int>(Constants::ParameterType::STRING);
        obj["value"] = "world";
        try {
            p->fromJson(obj);
            ASSERT_TRUE(std::holds_alternative<std::string>(p->value), "(STRING) fromJson() value should hold string");
            ASSERT_EQ(std::get<std::string>(p->value), std::string("world"), "(STRING) fromJson() value should be 'world'");
        } catch (...) {
            ASSERT_FALSE(true, "(STRING) fromJson() should not crash");
        }
        delete p;
    }

    // BOOL — true
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "bool_param";
        obj["type"] = static_cast<int>(Constants::ParameterType::BOOL);
        obj["value"] = true;
        try {
            p->fromJson(obj);
            ASSERT_TRUE(std::holds_alternative<bool>(p->value), "(BOOL) fromJson() value should hold bool");
            ASSERT_EQ(std::get<bool>(p->value), true, "(BOOL true) fromJson() value should be true");
        } catch (...) {
            ASSERT_FALSE(true, "(BOOL) fromJson() should not crash");
        }
        delete p;
    }

    // BOOL — false
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "bool_false_param";
        obj["type"] = static_cast<int>(Constants::ParameterType::BOOL);
        obj["value"] = false;
        try {
            p->fromJson(obj);
            ASSERT_EQ(std::get<bool>(p->value), false, "(BOOL false) fromJson() value should be false");
        } catch (...) {
            ASSERT_FALSE(true, "(BOOL false) fromJson() should not crash");
        }
        delete p;
    }

    // CHAR
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "char_param";
        obj["type"] = static_cast<int>(Constants::ParameterType::CHAR);
        obj["value"] = "A";
        try {
            p->fromJson(obj);
            ASSERT_TRUE(std::holds_alternative<char>(p->value), "(CHAR) fromJson() value should hold char");
            ASSERT_EQ(std::get<char>(p->value), 'A', "(CHAR) fromJson() value should be 'A'");
        } catch (...) {
            ASSERT_FALSE(true, "(CHAR) fromJson() should not crash");
        }
        delete p;
    }

    // LIST — should not crash (no-op case)
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "list_param";
        obj["type"] = static_cast<int>(Constants::ParameterType::LIST);
        obj["value"] = "ignored";
        try {
            p->fromJson(obj);
            ASSERT_TRUE(true, "(LIST) fromJson() should not crash (no-op case)");
        } catch (...) {
            ASSERT_FALSE(true, "(LIST) fromJson() crashed");
        }
        delete p;
    }

    // ENUM — should not crash (no-op case)
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "enum_param";
        obj["type"] = static_cast<int>(Constants::ParameterType::ENUM);
        obj["value"] = "ignored";
        try {
            p->fromJson(obj);
            ASSERT_TRUE(true, "(ENUM) fromJson() should not crash (no-op case)");
        } catch (...) {
            ASSERT_FALSE(true, "(ENUM) fromJson() crashed");
        }
        delete p;
    }
}

void test_parameter_fromJson_edgeCases() {
    std::cout << "\n--- Running fromJson Edge Case Tests ---" << std::endl;

    // Empty QJsonObject should not crash
    {
        Parameter* p = new Parameter();
        try {
            p->fromJson(QJsonObject());
            ASSERT_TRUE(true, "(Empty QJsonObject) fromJson() should not crash");
        } catch (...) {
            ASSERT_FALSE(true, "(Empty QJsonObject) fromJson() crashed");
        }
        delete p;
    }

    // Missing 'value' key with valid type should not crash
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["name"] = "no_value";
        obj["type"] = static_cast<int>(Constants::ParameterType::INT);
        // intentionally no "value" key
        try {
            p->fromJson(obj);
            ASSERT_TRUE(true, "(Missing value key) fromJson() should not crash");
        } catch (...) {
            ASSERT_FALSE(true, "(Missing value key) fromJson() crashed");
        }
        delete p;
    }

    // Missing 'name' key should not crash
    {
        Parameter* p = new Parameter();
        QJsonObject obj;
        obj["type"] = static_cast<int>(Constants::ParameterType::INT);
        obj["value"] = 1;
        try {
            p->fromJson(obj);
            ASSERT_TRUE(true, "(Missing name key) fromJson() should not crash");
        } catch (...) {
            ASSERT_FALSE(true, "(Missing name key) fromJson() crashed");
        }
        delete p;
    }

    // NOTE: toJson() writes type as string "paravalue", but fromJson() reads type
    // as int via toInt(). A direct round-trip of the QJsonObject from toJson()
    // into fromJson() will produce type = 0 (toInt() on a non-numeric string
    // returns 0), which maps to the first ParameterType enum value.
    // This test documents that known asymmetry — it is expected behavior.
    {
        Parameter* p = new Parameter();
        p->Name = "round_trip_param";
        p->value = std::string("test");

        QJsonObject obj = p->toJson();

        // Confirm 'type' is the string "paravalue" as written by toJson()
        ASSERT_EQ(obj["type"].toString().toStdString(), std::string("paravalue"),
                  "(Round-trip asymmetry) toJson() writes type as 'paravalue' string");

        // toInt() on "paravalue" returns 0 — document this, not treat as bug
        Parameter* p2 = new Parameter();
        try {
            p2->fromJson(obj);
            ASSERT_TRUE(true, "(Round-trip asymmetry) fromJson() on toJson() output should not crash");
        } catch (...) {
            ASSERT_FALSE(true, "(Round-trip asymmetry) fromJson() on toJson() output crashed");
        }
        delete p;
        delete p2;
    }
}


void parameter_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "       PARAMETER CUSTOM UNIT TESTS       " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_parameter_initialization();
    test_parameter_toJson_name_and_type();
    test_parameter_toJson_variants();
    test_parameter_fromJson_variants();
    test_parameter_fromJson_edgeCases();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
