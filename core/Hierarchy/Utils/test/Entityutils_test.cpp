#include "core/Hierarchy/Utils/entityutils.h"
#include <QJsonObject>
#include <iostream>
#include <cmath>
#include <tuple>

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

void test_entityutils_toParm() {
    std::cout << "\n--- Running toParm Tests ---" << std::endl;

    // All 5 keys must always be present
    QJsonObject obj = toParm(5.0f, "m/s", 0.0f, 10.0f, "speed");
    ASSERT_FALSE(obj.empty(),                  "toParm() should return non-empty QJsonObject");
    ASSERT_TRUE(obj.contains("type"),          "toParm() should contain 'type' key");
    ASSERT_TRUE(obj.contains("min"),           "toParm() should contain 'min' key");
    ASSERT_TRUE(obj.contains("max"),           "toParm() should contain 'max' key");
    ASSERT_TRUE(obj.contains("value"),         "toParm() should contain 'value' key");
    ASSERT_TRUE(obj.contains("unit"),          "toParm() should contain 'unit' key");
    ASSERT_TRUE(obj.contains("description"),   "toParm() should contain 'description' key");

    // 'type' should always be "unitParam"
    ASSERT_EQ(obj["type"].toString().toStdString(), std::string("unitParam"),
              "toParm() type should always be 'unitParam'");

    // Value within range — should pass through unchanged
    ASSERT_NEAR((float)obj["value"].toDouble(), 5.0f, 0.001f,
                "(In-range) toParm() value should be 5.0");

    // Value below min — should be clamped to min
    QJsonObject clampLow = toParm(-5.0f, "m/s", 0.0f, 10.0f, "speed");
    ASSERT_NEAR((float)clampLow["value"].toDouble(), 0.0f, 0.001f,
                "(Below min) toParm() value should clamp to min=0");

    // Value above max — should be clamped to max
    QJsonObject clampHigh = toParm(99.0f, "m/s", 0.0f, 10.0f, "speed");
    ASSERT_NEAR((float)clampHigh["value"].toDouble(), 10.0f, 0.001f,
                "(Above max) toParm() value should clamp to max=10");

    // Value exactly at min — should pass through as min
    QJsonObject atMin = toParm(0.0f, "m/s", 0.0f, 10.0f, "speed");
    ASSERT_NEAR((float)atMin["value"].toDouble(), 0.0f, 0.001f,
                "(At min) toParm() value should be 0");

    // Value exactly at max — should pass through as max
    QJsonObject atMax = toParm(10.0f, "m/s", 0.0f, 10.0f, "speed");
    ASSERT_NEAR((float)atMax["value"].toDouble(), 10.0f, 0.001f,
                "(At max) toParm() value should be 10");

    // min == max (difference == 0) — clamping skipped, raw value stored
    QJsonObject noClamp = toParm(999.0f, "m/s", 5.0f, 5.0f, "speed");
    ASSERT_NEAR((float)noClamp["value"].toDouble(), 999.0f, 0.001f,
                "(min==max) toParm() should store raw value without clamping");

    // Unit and description stored correctly
    ASSERT_EQ(obj["unit"].toString().toStdString(),        std::string("m/s"),   "toParm() unit should be 'm/s'");
    ASSERT_EQ(obj["description"].toString().toStdString(), std::string("speed"), "toParm() description should be 'speed'");

    // min and max stored correctly
    ASSERT_NEAR((float)obj["min"].toDouble(), 0.0f,  0.001f, "toParm() min should be 0");
    ASSERT_NEAR((float)obj["max"].toDouble(), 10.0f, 0.001f, "toParm() max should be 10");

    // Default args — empty unit and description should not crash
    try {
        QJsonObject defaults = toParm(1.0f, "");
        ASSERT_TRUE(true, "(Default args) toParm() should not crash with empty unit");
    } catch (...) {
        ASSERT_FALSE(true, "(Default args) toParm() crashed with empty unit");
    }
}

void test_entityutils_valueFromParm() {
    std::cout << "\n--- Running valueFromParm Tests ---" << std::endl;

    // Normal case — value key present
    QJsonObject obj = toParm(7.5f, "km", 0.0f, 10.0f, "dist");
    ASSERT_NEAR(valueFromParm(obj), 7.5f, 0.001f, "(Correct) valueFromParm() should return 7.5");

    // Missing 'value' key — should return 0.0f
    QJsonObject empty;
    ASSERT_NEAR(valueFromParm(empty), 0.0f, 0.001f, "(Missing key) valueFromParm() should return 0.0");

    // QJsonObject with 'value' = 0
    QJsonObject zeroObj;
    zeroObj["value"] = 0.0;
    ASSERT_NEAR(valueFromParm(zeroObj), 0.0f, 0.001f, "(Zero value) valueFromParm() should return 0.0");

    // Negative value
    QJsonObject negObj;
    negObj["value"] = -3.5;
    ASSERT_NEAR(valueFromParm(negObj), -3.5f, 0.001f, "(Negative value) valueFromParm() should return -3.5");
}

void test_entityutils_toRadians() {
    std::cout << "\n--- Running toRadians Tests ---" << std::endl;

    ASSERT_NEAR(toRadians(0.0),   0.0,       0.00001, "toRadians(0) should be 0");
    ASSERT_NEAR(toRadians(90.0),  M_PI/2.0,  0.00001, "toRadians(90) should be PI/2");
    ASSERT_NEAR(toRadians(180.0), M_PI,      0.00001, "toRadians(180) should be PI");
    ASSERT_NEAR(toRadians(360.0), 2.0*M_PI,  0.00001, "toRadians(360) should be 2*PI");
    ASSERT_NEAR(toRadians(-90.0), -M_PI/2.0, 0.00001, "toRadians(-90) should be -PI/2");
}

void test_entityutils_entityTypeConversion() {
    std::cout << "\n--- Running EntityType Conversion Tests ---" << std::endl;

    // entityTypeToString — all implemented enum values
    ASSERT_EQ(entityTypeToString(Constants::EntityType::Platform).toStdString(),   std::string("Platform"),   "entityTypeToString Platform");
    ASSERT_EQ(entityTypeToString(Constants::EntityType::Radio).toStdString(),      std::string("Radio"),      "entityTypeToString Radio");
    ASSERT_EQ(entityTypeToString(Constants::EntityType::Sensor).toStdString(),     std::string("Sensor"),     "entityTypeToString Sensor");
    ASSERT_EQ(entityTypeToString(Constants::EntityType::SpecialZone).toStdString(),std::string("SpecialZone"),"entityTypeToString SpecialZone");
    ASSERT_EQ(entityTypeToString(Constants::EntityType::Weapon).toStdString(),     std::string("Weapon"),     "entityTypeToString Weapon");
    ASSERT_EQ(entityTypeToString(Constants::EntityType::IFF).toStdString(),        std::string("IFF"),        "entityTypeToString IFF");
    ASSERT_EQ(entityTypeToString(Constants::EntityType::FixedPoint).toStdString(), std::string("FixedPoint"), "entityTypeToString FixedPoint");
    ASSERT_EQ(entityTypeToString(Constants::EntityType::Formation).toStdString(),  std::string("Formation"),  "entityTypeToString Formation");

    // stringToEntityType — all supported strings
    ASSERT_EQ(stringToEntityType("Platform"),   Constants::EntityType::Platform,   "stringToEntityType Platform");
    ASSERT_EQ(stringToEntityType("Radio"),      Constants::EntityType::Radio,      "stringToEntityType Radio");
    ASSERT_EQ(stringToEntityType("Sensor"),     Constants::EntityType::Sensor,     "stringToEntityType Sensor");
    ASSERT_EQ(stringToEntityType("SpecialZone"),Constants::EntityType::SpecialZone,"stringToEntityType SpecialZone");
    ASSERT_EQ(stringToEntityType("Weapon"),     Constants::EntityType::Weapon,     "stringToEntityType Weapon");
    ASSERT_EQ(stringToEntityType("IFF"),        Constants::EntityType::IFF,        "stringToEntityType IFF");
    ASSERT_EQ(stringToEntityType("FixedPoint"), Constants::EntityType::FixedPoint, "stringToEntityType FixedPoint");
    ASSERT_EQ(stringToEntityType("Formation"),  Constants::EntityType::Formation,  "stringToEntityType Formation");

    // Unknown string — should fall back to Platform
    ASSERT_EQ(stringToEntityType("Unknown"),    Constants::EntityType::Platform,   "(Unknown string) stringToEntityType should fallback to Platform");
    ASSERT_EQ(stringToEntityType(""),           Constants::EntityType::Platform,   "(Empty string) stringToEntityType should fallback to Platform");

    // NOTE: Known asymmetry — stringToEntityType() handles "Supply" but
    // entityTypeToString() has no case for Supply (falls through to default "Unknown").
    // Round-trip for Supply will not restore the original enum value.
    ASSERT_EQ(stringToEntityType("Supply"), Constants::EntityType::Supply, "(Supply) stringToEntityType should return Supply");
    ASSERT_EQ(entityTypeToString(Constants::EntityType::Supply).toStdString(), std::string("Unknown"),
              "(Supply asymmetry) entityTypeToString Supply has no case — returns 'Unknown'");

    // Round-trip for all symmetric types
    ASSERT_EQ(stringToEntityType(entityTypeToString(Constants::EntityType::Platform)),   Constants::EntityType::Platform,   "Round-trip Platform");
    ASSERT_EQ(stringToEntityType(entityTypeToString(Constants::EntityType::Sensor)),     Constants::EntityType::Sensor,     "Round-trip Sensor");
    ASSERT_EQ(stringToEntityType(entityTypeToString(Constants::EntityType::Formation)),  Constants::EntityType::Formation,  "Round-trip Formation");

    // entityTypeOptions() should return a non-empty list
    QStringList options = entityTypeOptions();
    ASSERT_TRUE(options.size() > 0, "entityTypeOptions() should return non-empty list");
}

void test_entityutils_formationTypeConversion() {
    std::cout << "\n--- Running FormationType Conversion Tests ---" << std::endl;

    // formationTypeToString — all 3 types
    ASSERT_EQ(formationTypeToString(Constants::FormationType::Line).toStdString(),    std::string("Line"),    "formationTypeToString Line");
    ASSERT_EQ(formationTypeToString(Constants::FormationType::V).toStdString(),       std::string("V"),       "formationTypeToString V");
    ASSERT_EQ(formationTypeToString(Constants::FormationType::Diamond).toStdString(), std::string("Diamond"), "formationTypeToString Diamond");

    // stringToFormationType — all 3 strings
    ASSERT_EQ(stringToFormationType("Line"),    Constants::FormationType::Line,    "stringToFormationType Line");
    ASSERT_EQ(stringToFormationType("V"),       Constants::FormationType::V,       "stringToFormationType V");
    ASSERT_EQ(stringToFormationType("Diamond"), Constants::FormationType::Diamond, "stringToFormationType Diamond");

    // Unknown string — should fall back to Line
    ASSERT_EQ(stringToFormationType("Unknown"), Constants::FormationType::Line, "(Unknown) stringToFormationType should fallback to Line");
    ASSERT_EQ(stringToFormationType(""),        Constants::FormationType::Line, "(Empty)   stringToFormationType should fallback to Line");

    // Round-trip for all 3 types
    ASSERT_EQ(stringToFormationType(formationTypeToString(Constants::FormationType::Line)),    Constants::FormationType::Line,    "Round-trip Line");
    ASSERT_EQ(stringToFormationType(formationTypeToString(Constants::FormationType::V)),       Constants::FormationType::V,       "Round-trip V");
    ASSERT_EQ(stringToFormationType(formationTypeToString(Constants::FormationType::Diamond)), Constants::FormationType::Diamond, "Round-trip Diamond");

    // formationTypeOptions() should return a non-empty list
    QStringList options = formationTypeOptions();
    ASSERT_TRUE(options.size() > 0, "formationTypeOptions() should return non-empty list");
}

void test_entityutils_distanceBetween() {
    std::cout << "\n--- Running distanceBetween Tests ---" << std::endl;

    // Same point — distance should be 0
    ASSERT_NEAR(distanceBetween(0.0, 0.0, 0.0, 0.0), 0.0, 1.0,
                "(Same point) distanceBetween should be 0");

    // Islamabad to Rawalpindi — roughly 14 km apart
    double dist = distanceBetween(33.6844, 73.0479, 33.5651, 73.0169);
    ASSERT_TRUE(dist > 12000.0 && dist < 16000.0,
                "(Islamabad-Rawalpindi) distanceBetween should be ~14km (12-16km range)");

    // Equator crossing — London (51.5N, 0.1W) to Paris (48.8N, 2.3E) ~341 km
    double londonParis = distanceBetween(51.5074, -0.1278, 48.8566, 2.3522);
    ASSERT_TRUE(londonParis > 330000.0 && londonParis < 360000.0,
                "(London-Paris) distanceBetween should be ~341km (330-360km range)");

    // Symmetric: distance(A,B) == distance(B,A)
    double ab = distanceBetween(33.6844, 73.0479, 48.8566, 2.3522);
    double ba = distanceBetween(48.8566, 2.3522, 33.6844, 73.0479);
    ASSERT_NEAR(ab, ba, 1.0, "(Symmetry) distanceBetween(A,B) should equal distanceBetween(B,A)");

    // Result should be in meters (EARTH_RADIUS = 6371000)
    ASSERT_TRUE(londonParis < EARTH_RADIUS,
                "distanceBetween result should be less than Earth radius (in meters)");
}

void test_entityutils_flatXYZ() {
    std::cout << "\n--- Running geoToFlatXYZ / flatXYZToGeo Tests ---" << std::endl;

    // Origin point (hardcoded base: 21.1458, 79.0882) should map to approx (0,0,0)
    FlatXYZ origin = geoToFlatXYZ(21.1458, 79.0882, 0.0);
    ASSERT_NEAR(origin.x, 0.0, 1.0, "(Origin) geoToFlatXYZ x should be ~0");
    ASSERT_NEAR(origin.z, 0.0, 1.0, "(Origin) geoToFlatXYZ z should be ~0");
    ASSERT_NEAR(origin.y, 0.0, 1.0, "(Origin) geoToFlatXYZ y (altitude) should be 0");

    // Altitude should map directly to y
    FlatXYZ withAlt = geoToFlatXYZ(21.1458, 79.0882, 500.0);
    ASSERT_NEAR(withAlt.y, 500.0, 1.0, "geoToFlatXYZ altitude should map to y");

    // Round-trip: geo -> flat -> geo should restore original coords
    double lat = 21.5, lon = 79.5, alt = 100.0;
    FlatXYZ flat = geoToFlatXYZ(lat, lon, alt);
    GeoPos restored = flatXYZToGeo(flat.x, flat.y, flat.z);
    ASSERT_NEAR(restored.lat, lat, 0.001, "(Round-trip) flatXYZToGeo lat should restore original");
    ASSERT_NEAR(restored.lon, lon, 0.001, "(Round-trip) flatXYZToGeo lon should restore original");
    ASSERT_NEAR(restored.alt, alt, 1.0,   "(Round-trip) flatXYZToGeo alt should restore original");

    // Moving north (increasing lat) should increase z
    FlatXYZ north = geoToFlatXYZ(22.0, 79.0882, 0.0);
    ASSERT_TRUE(north.z > 0.0, "Moving north should produce positive z");

    // Moving east (increasing lon) should increase x
    FlatXYZ east = geoToFlatXYZ(21.1458, 80.0, 0.0);
    ASSERT_TRUE(east.x > 0.0, "Moving east should produce positive x");
}

void test_entityutils_ecef() {
    std::cout << "\n--- Running geoToXYZ / xyzToGeo Tests ---" << std::endl;

    // Round-trip: geo -> ECEF -> geo should restore original coords
    double lat = 33.6844, lon = 73.0479, alt = 500.0;
    ECEF ecef = geoToXYZ(lat, lon, alt);
    GeoPos restored = xyzToGeo(ecef.x, ecef.y, ecef.z);

    ASSERT_NEAR(restored.lat, lat, 0.0001, "(Round-trip ECEF) xyzToGeo lat should restore original");
    ASSERT_NEAR(restored.lon, lon, 0.0001, "(Round-trip ECEF) xyzToGeo lon should restore original");
    // Altitude restoration is less precise with Bowring's formula — use wider tolerance
    ASSERT_NEAR(restored.alt, alt, 1.0, "(Round-trip ECEF) xyzToGeo alt should restore original within 1m");

    // ECEF coordinates for equator/prime meridian (0,0,0 geo) should be close to (6378137, 0, 0)
    ECEF equator = geoToXYZ(0.0, 0.0, 0.0);
    ASSERT_NEAR(equator.x, 6378137.0, 1.0, "(Equator) geoToXYZ x should be ~Earth equatorial radius");
    ASSERT_NEAR(equator.y, 0.0, 1.0, "(Equator) geoToXYZ y should be 0");
    ASSERT_NEAR(equator.z, 0.0, 1.0, "(Equator) geoToXYZ z should be 0");

    // Altitude increases radial distance from center
    ECEF withAlt  = geoToXYZ(0.0, 0.0, 1000.0);
    ECEF noAlt    = geoToXYZ(0.0, 0.0, 0.0);
    double rWith  = std::sqrt(withAlt.x*withAlt.x + withAlt.y*withAlt.y + withAlt.z*withAlt.z);
    double rNoAlt = std::sqrt(noAlt.x*noAlt.x   + noAlt.y*noAlt.y   + noAlt.z*noAlt.z);
    ASSERT_NEAR(rWith - rNoAlt, 1000.0, 1.0, "Adding 1000m altitude should increase radial distance by ~1000m");

    // Second round-trip with negative lat/lon
    double lat2 = -33.8688, lon2 = -70.6693, alt2 = 567.0;
    ECEF ecef2 = geoToXYZ(lat2, lon2, alt2);
    GeoPos res2 = xyzToGeo(ecef2.x, ecef2.y, ecef2.z);
    ASSERT_NEAR(res2.lat, lat2, 0.0001, "(Negative coords ECEF) lat should restore");
    ASSERT_NEAR(res2.lon, lon2, 0.0001, "(Negative coords ECEF) lon should restore");
}

void test_entityutils_calculateNewLatLong() {
    std::cout << "\n--- Running calculateNewLatLong Tests ---" << std::endl;

    // Due north (heading=0) — longitude should stay the same, latitude should increase
    auto [lat1, lon1] = calculateNewLatLong(21.1458, 79.0882, 0.0, 1.0);
    ASSERT_TRUE(lat1 > 21.1458, "(Due north) latitude should increase");
    ASSERT_NEAR(lon1, 79.0882, 0.01, "(Due north) longitude should stay approximately the same");

    // Due south (heading=180) — longitude should stay the same, latitude should decrease
    auto [lat2, lon2] = calculateNewLatLong(21.1458, 79.0882, 180.0, 1.0);
    ASSERT_TRUE(lat2 < 21.1458, "(Due south) latitude should decrease");
    ASSERT_NEAR(lon2, 79.0882, 0.01, "(Due south) longitude should stay approximately the same");

    // Due east (heading=90) — latitude should stay the same, longitude should increase
    auto [lat3, lon3] = calculateNewLatLong(21.1458, 79.0882, 90.0, 1.0);
    ASSERT_NEAR(lat3, 21.1458, 0.01, "(Due east) latitude should stay approximately the same");
    ASSERT_TRUE(lon3 > 79.0882, "(Due east) longitude should increase");

    // Zero distance — should return original coords
    auto [lat4, lon4] = calculateNewLatLong(21.1458, 79.0882, 45.0, 0.0);
    ASSERT_NEAR(lat4, 21.1458, 0.0001, "(Zero distance) latitude should not change");
    ASSERT_NEAR(lon4, 79.0882, 0.0001, "(Zero distance) longitude should not change");

    // Larger distance should produce larger displacement than smaller distance
    auto [latBig, lonBig]     = calculateNewLatLong(0.0, 0.0, 0.0, 100.0);
    auto [latSmall, lonSmall] = calculateNewLatLong(0.0, 0.0, 0.0, 1.0);
    ASSERT_TRUE(latBig > latSmall, "(Larger distance) should produce larger latitude displacement heading north");
}

void test_entityutils_convertToClockwise360() {
    std::cout << "\n--- Running convertToClockwise360 Tests ---" << std::endl;

    // Positive values — passed through unchanged (unless >= 360)
    ASSERT_NEAR(convertToClockwise360(0.0),   0.0,   0.0001, "convertToClockwise360(0) should be 0");
    ASSERT_NEAR(convertToClockwise360(90.0),  90.0,  0.0001, "convertToClockwise360(90) should be 90");
    ASSERT_NEAR(convertToClockwise360(180.0), 180.0, 0.0001, "convertToClockwise360(180) should be 180");
    ASSERT_NEAR(convertToClockwise360(359.9), 359.9, 0.0001, "convertToClockwise360(359.9) should be 359.9");

    // Negative values — shifted by +360
    ASSERT_NEAR(convertToClockwise360(-90.0),  270.0, 0.0001, "convertToClockwise360(-90) should be 270");
    ASSERT_NEAR(convertToClockwise360(-180.0), 180.0, 0.0001, "convertToClockwise360(-180) should be 180");
    ASSERT_NEAR(convertToClockwise360(-1.0),   359.0, 0.0001, "convertToClockwise360(-1) should be 359");

    // Boundary: exactly 360 — should return 0
    ASSERT_NEAR(convertToClockwise360(360.0), 0.0, 0.0001, "convertToClockwise360(360) should return 0");

    // Result should always be in [0, 360)
    double r1 = convertToClockwise360(45.0);
    double r2 = convertToClockwise360(-45.0);
    double r3 = convertToClockwise360(-179.9);
    ASSERT_TRUE(r1 >= 0.0 && r1 < 360.0, "convertToClockwise360(45) result should be in [0, 360)");
    ASSERT_TRUE(r2 >= 0.0 && r2 < 360.0, "convertToClockwise360(-45) result should be in [0, 360)");
    ASSERT_TRUE(r3 >= 0.0 && r3 < 360.0, "convertToClockwise360(-179.9) result should be in [0, 360)");
}


void entityUtils_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "      ENTITYUTILS CUSTOM UNIT TESTS      " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_entityutils_toParm();
    test_entityutils_valueFromParm();
    test_entityutils_toRadians();
    test_entityutils_entityTypeConversion();
    test_entityutils_formationTypeConversion();
    test_entityutils_distanceBetween();
    test_entityutils_flatXYZ();
    test_entityutils_ecef();
    test_entityutils_calculateNewLatLong();
    test_entityutils_convertToClockwise360();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
