#include "core/Hierarchy/Struct/vector.h"
#include <iostream>
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

#define ASSERT_NEAR(val1, val2, epsilon, testName) \
if (std::fabs((val1) - (val2)) <= (epsilon)) { \
        std::cout << tests << " [PASS] " << testName << std::endl; \
        testsPassed++; \
        tests++; \
} else { \
        std::cerr << tests << " [FAIL] " << testName << " (Expected ~" << (val2) << ", Got: " << (val1) << ") (Line: " << __LINE__ << ")" << std::endl; \
        testsFailed++; \
        tests++; \
}


// ==========================================
// TEST SUITES
// ==========================================

void test_vector_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;

    // Default constructor
    Vector v;
    ASSERT_EQ(v.x, 0.0f, "Default constructor x should be 0");
    ASSERT_EQ(v.y, 0.0f, "Default constructor y should be 0");
    ASSERT_EQ(v.z, 0.0f, "Default constructor z should be 0");

    // Parameterized constructor
    Vector v2(1.0f, 2.0f, 3.0f);
    ASSERT_EQ(v2.x, 1.0f, "Parameterized constructor x should be 1");
    ASSERT_EQ(v2.y, 2.0f, "Parameterized constructor y should be 2");
    ASSERT_EQ(v2.z, 3.0f, "Parameterized constructor z should be 3");

    // Negative values
    Vector v3(-1.5f, -2.5f, -3.5f);
    ASSERT_EQ(v3.x, -1.5f, "Constructor with negative x should be -1.5");
    ASSERT_EQ(v3.y, -2.5f, "Constructor with negative y should be -2.5");
    ASSERT_EQ(v3.z, -3.5f, "Constructor with negative z should be -3.5");
}

void test_vector_magnitude() {
    std::cout << "\n--- Running Magnitude Tests ---" << std::endl;

    // Known magnitude: (3, 4, 0) => 5
    Vector v(3.0f, 4.0f, 0.0f);
    ASSERT_NEAR(v.magnitude(), 5.0f, 0.0001f, "magnitude() of (3,4,0) should be 5");

    // sqrMagnitude
    ASSERT_NEAR(v.sqrMagnitude(), 25.0f, 0.0001f, "sqrMagnitude() of (3,4,0) should be 25");

    // magnitudeSq (alias)
    ASSERT_NEAR(v.magnitudeSq(), 25.0f, 0.0001f, "magnitudeSq() of (3,4,0) should be 25");

    // Zero vector magnitude
    Vector zero;
    ASSERT_NEAR(zero.magnitude(), 0.0f, 0.0001f, "magnitude() of zero vector should be 0");

    // Unit vector (1,0,0) magnitude
    Vector unit(1.0f, 0.0f, 0.0f);
    ASSERT_NEAR(unit.magnitude(), 1.0f, 0.0001f, "magnitude() of (1,0,0) should be 1");
}

void test_vector_normalize() {
    std::cout << "\n--- Running Normalize Tests ---" << std::endl;

    // normalized() — returns new vector
    Vector v(3.0f, 4.0f, 0.0f);
    Vector n = v.normalized();
    ASSERT_NEAR(n.magnitude(), 1.0f, 0.0001f, "normalized() result should have magnitude 1");
    ASSERT_NEAR(n.x, 0.6f, 0.0001f, "normalized() x should be 0.6");
    ASSERT_NEAR(n.y, 0.8f, 0.0001f, "normalized() y should be 0.8");

    // normalised() — alias
    Vector n2 = v.normalised();
    ASSERT_NEAR(n2.magnitude(), 1.0f, 0.0001f, "normalised() result should have magnitude 1");

    // Zero vector normalized() should return zero, not crash
    try {
        Vector zero;
        Vector nz = zero.normalized();
        ASSERT_EQ(nz.x, 0.0f, "(Zero vector) normalized() x should be 0");
        ASSERT_EQ(nz.y, 0.0f, "(Zero vector) normalized() y should be 0");
        ASSERT_EQ(nz.z, 0.0f, "(Zero vector) normalized() z should be 0");
    } catch (...) {
        ASSERT_FALSE(true, "(Zero vector) normalized() should not crash");
    }

    // normalize() — mutates in place
    Vector v2(0.0f, 5.0f, 0.0f);
    v2.normalize();
    ASSERT_NEAR(v2.magnitude(), 1.0f, 0.0001f, "normalize() in-place result should have magnitude 1");

    // Zero vector normalize() should not crash
    try {
        Vector zero2;
        zero2.normalize();
        ASSERT_TRUE(true, "(Zero vector) normalize() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Zero vector) normalize() crashed");
    }
}

void test_vector_staticMath() {
    std::cout << "\n--- Running Static Math Tests ---" << std::endl;

    Vector a(1.0f, 0.0f, 0.0f);
    Vector b(0.0f, 1.0f, 0.0f);
    Vector c(1.0f, 2.0f, 3.0f);
    Vector d(4.0f, 5.0f, 6.0f);

    // Dot product — perpendicular vectors should give 0
    ASSERT_NEAR(Vector::Dot(a, b), 0.0f, 0.0001f, "Dot(X, Y) of perpendicular vectors should be 0");

    // Dot product — parallel vectors
    ASSERT_NEAR(Vector::Dot(a, a), 1.0f, 0.0001f, "Dot(X, X) should be 1");

    // Dot product — general
    ASSERT_NEAR(Vector::Dot(c, d), 32.0f, 0.0001f, "Dot((1,2,3),(4,5,6)) should be 32");

    // dot() instance method
    ASSERT_NEAR(c.dot(d), 32.0f, 0.0001f, "dot() instance method should match Dot() static");

    // Cross product — standard axes
    Vector cross = Vector::Cross(a, b);
    ASSERT_NEAR(cross.x, 0.0f, 0.0001f, "Cross(X,Y) x should be 0");
    ASSERT_NEAR(cross.y, 0.0f, 0.0001f, "Cross(X,Y) y should be 0");
    ASSERT_NEAR(cross.z, 1.0f, 0.0001f, "Cross(X,Y) z should be 1");

    // Distance — known distance
    Vector p1(0.0f, 0.0f, 0.0f);
    Vector p2(3.0f, 4.0f, 0.0f);
    ASSERT_NEAR(Vector::Distance(p1, p2), 5.0f, 0.0001f, "Distance((0,0,0),(3,4,0)) should be 5");

    // Distance — same point
    ASSERT_NEAR(Vector::Distance(p1, p1), 0.0f, 0.0001f, "Distance of same point should be 0");

    // Lerp — t=0 returns a
    Vector lerp0 = Vector::Lerp(c, d, 0.0f);
    ASSERT_NEAR(lerp0.x, c.x, 0.0001f, "Lerp t=0 should return a.x");

    // Lerp — t=1 returns b
    Vector lerp1 = Vector::Lerp(c, d, 1.0f);
    ASSERT_NEAR(lerp1.x, d.x, 0.0001f, "Lerp t=1 should return b.x");

    // Lerp — t clamped below 0
    Vector lerpNeg = Vector::Lerp(c, d, -1.0f);
    ASSERT_NEAR(lerpNeg.x, c.x, 0.0001f, "Lerp t<0 should clamp to a");

    // Lerp — t clamped above 1
    Vector lerpOver = Vector::Lerp(c, d, 2.0f);
    ASSERT_NEAR(lerpOver.x, d.x, 0.0001f, "Lerp t>1 should clamp to b");

    // Angle — perpendicular vectors should give 90 degrees
    ASSERT_NEAR(Vector::Angle(a, b), 90.0f, 0.001f, "Angle between X and Y axes should be 90");

    // Angle — same direction
    ASSERT_NEAR(Vector::Angle(a, a), 0.0f, 0.001f, "Angle of vector with itself should be 0");

    // ClampMagnitude — within limit, returns same
    Vector small(1.0f, 0.0f, 0.0f);
    Vector clamped = Vector::ClampMagnitude(small, 5.0f);
    ASSERT_NEAR(clamped.magnitude(), 1.0f, 0.0001f, "ClampMagnitude within limit should not change magnitude");

    // ClampMagnitude — exceeds limit, clamps
    Vector big(10.0f, 0.0f, 0.0f);
    Vector clampedBig = Vector::ClampMagnitude(big, 3.0f);
    ASSERT_NEAR(clampedBig.magnitude(), 3.0f, 0.0001f, "ClampMagnitude should clamp to maxLength");

    // Reflect — vector hitting a flat Y-plane normal
    Vector inDir(1.0f, -1.0f, 0.0f);
    Vector normal(0.0f, 1.0f, 0.0f);
    Vector reflected = Vector::Reflect(inDir, normal);
    ASSERT_NEAR(reflected.x, 1.0f, 0.0001f, "Reflect x should be 1");
    ASSERT_NEAR(reflected.y, 1.0f, 0.0001f, "Reflect y should be flipped to 1");
    ASSERT_NEAR(reflected.z, 0.0f, 0.0001f, "Reflect z should be 0");

    // Project — project onto zero vector should return zero without crash
    try {
        Vector proj = Vector::Project(c, Vector(0, 0, 0));
        ASSERT_EQ(proj.x, 0.0f, "(Zero b) Project should return zero vector x");
    } catch (...) {
        ASSERT_FALSE(true, "(Zero b) Project should not crash");
    }

    // Project — project X onto X should return X
    Vector projSelf = Vector::Project(a, a);
    ASSERT_NEAR(projSelf.x, 1.0f, 0.0001f, "Project(X, X) should return X");
}

void test_vector_operators() {
    std::cout << "\n--- Running Operator Tests ---" << std::endl;

    Vector a(1.0f, 2.0f, 3.0f);
    Vector b(4.0f, 5.0f, 6.0f);

    // operator+
    Vector sum = a + b;
    ASSERT_NEAR(sum.x, 5.0f, 0.0001f, "operator+ x should be 5");
    ASSERT_NEAR(sum.y, 7.0f, 0.0001f, "operator+ y should be 7");
    ASSERT_NEAR(sum.z, 9.0f, 0.0001f, "operator+ z should be 9");

    // operator-
    Vector diff = b - a;
    ASSERT_NEAR(diff.x, 3.0f, 0.0001f, "operator- x should be 3");
    ASSERT_NEAR(diff.y, 3.0f, 0.0001f, "operator- y should be 3");
    ASSERT_NEAR(diff.z, 3.0f, 0.0001f, "operator- z should be 3");

    // operator* scalar
    Vector scaled = a * 2.0f;
    ASSERT_NEAR(scaled.x, 2.0f, 0.0001f, "operator* x should be 2");
    ASSERT_NEAR(scaled.y, 4.0f, 0.0001f, "operator* y should be 4");
    ASSERT_NEAR(scaled.z, 6.0f, 0.0001f, "operator* z should be 6");

    // operator/ scalar
    Vector divided = b / 2.0f;
    ASSERT_NEAR(divided.x, 2.0f, 0.0001f, "operator/ x should be 2");
    ASSERT_NEAR(divided.y, 2.5f, 0.0001f, "operator/ y should be 2.5");
    ASSERT_NEAR(divided.z, 3.0f, 0.0001f, "operator/ z should be 3");

    // operator/ by zero should return zero vector, not crash
    try {
        Vector divZero = a / 0.0f;
        ASSERT_EQ(divZero.x, 0.0f, "(Divide by zero) operator/ x should be 0");
        ASSERT_EQ(divZero.y, 0.0f, "(Divide by zero) operator/ y should be 0");
        ASSERT_EQ(divZero.z, 0.0f, "(Divide by zero) operator/ z should be 0");
    } catch (...) {
        ASSERT_FALSE(true, "(Divide by zero) operator/ should not crash");
    }

    // unary operator-
    Vector neg = -a;
    ASSERT_NEAR(neg.x, -1.0f, 0.0001f, "unary operator- x should be -1");
    ASSERT_NEAR(neg.y, -2.0f, 0.0001f, "unary operator- y should be -2");
    ASSERT_NEAR(neg.z, -3.0f, 0.0001f, "unary operator- z should be -3");

    // operator+=
    Vector va(1.0f, 2.0f, 3.0f);
    va += b;
    ASSERT_NEAR(va.x, 5.0f, 0.0001f, "operator+= x should be 5");
    ASSERT_NEAR(va.y, 7.0f, 0.0001f, "operator+= y should be 7");
    ASSERT_NEAR(va.z, 9.0f, 0.0001f, "operator+= z should be 9");

    // operator-=
    Vector vb(4.0f, 5.0f, 6.0f);
    vb -= a;
    ASSERT_NEAR(vb.x, 3.0f, 0.0001f, "operator-= x should be 3");
    ASSERT_NEAR(vb.y, 3.0f, 0.0001f, "operator-= y should be 3");
    ASSERT_NEAR(vb.z, 3.0f, 0.0001f, "operator-= z should be 3");

    // operator*=
    Vector vc(1.0f, 2.0f, 3.0f);
    vc *= 3.0f;
    ASSERT_NEAR(vc.x, 3.0f, 0.0001f, "operator*= x should be 3");
    ASSERT_NEAR(vc.y, 6.0f, 0.0001f, "operator*= y should be 6");
    ASSERT_NEAR(vc.z, 9.0f, 0.0001f, "operator*= z should be 9");
}

void test_vector_json() {
    std::cout << "\n--- Running JSON Tests ---" << std::endl;

    Vector v(1.5f, 2.5f, 3.5f);

    // toJson — should not be empty
    QJsonObject obj = v.toJson();
    ASSERT_FALSE(obj.empty(), "toJson() should return non-empty QJsonObject");

    // toJson — should contain correct keys
    ASSERT_TRUE(obj.contains("x"), "toJson() should contain key 'x'");
    ASSERT_TRUE(obj.contains("y"), "toJson() should contain key 'y'");
    ASSERT_TRUE(obj.contains("z"), "toJson() should contain key 'z'");
    ASSERT_TRUE(obj.contains("type"), "toJson() should contain key 'type'");

    // toJson — values should match
    ASSERT_NEAR((float)obj["x"].toDouble(), 1.5f, 0.0001f, "toJson() x value should be 1.5");
    ASSERT_NEAR((float)obj["y"].toDouble(), 2.5f, 0.0001f, "toJson() y value should be 2.5");
    ASSERT_NEAR((float)obj["z"].toDouble(), 3.5f, 0.0001f, "toJson() z value should be 3.5");

    // fromJson — correct values round-trip
    try {
        Vector v2;
        v2.fromJson(obj);
        ASSERT_NEAR(v2.x, 1.5f, 0.0001f, "(Correct values) fromJson() x should be 1.5");
        ASSERT_NEAR(v2.y, 2.5f, 0.0001f, "(Correct values) fromJson() y should be 2.5");
        ASSERT_NEAR(v2.z, 3.5f, 0.0001f, "(Correct values) fromJson() z should be 3.5");
    } catch (...) {
        ASSERT_FALSE(true, "(Correct values) fromJson() should not crash");
    }

    // fromJson — empty object should not crash
    try {
        Vector v3;
        v3.fromJson(QJsonObject());
        ASSERT_TRUE(true, "(Empty QJsonObject) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Empty QJsonObject) fromJson() crashed");
    }

    // fromJson — partial object (only x present) should not crash
    try {
        Vector v4;
        QJsonObject partial;
        partial["x"] = 9.9;
        v4.fromJson(partial);
        ASSERT_NEAR(v4.x, 9.9f, 0.001f, "(Partial JSON) fromJson() should set x correctly");
        ASSERT_TRUE(true, "(Partial JSON) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Partial JSON) fromJson() crashed");
    }
}


void vector_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "         VECTOR CUSTOM UNIT TESTS        " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_vector_initialization();
    test_vector_magnitude();
    test_vector_normalize();
    test_vector_staticMath();
    test_vector_operators();
    test_vector_json();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
