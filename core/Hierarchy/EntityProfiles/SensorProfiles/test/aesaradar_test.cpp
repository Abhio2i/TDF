#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"
#include "core/Hierarchy/hierarchy.h"
#include <QJsonObject>
#include <QObject>
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

void test_aesaradar_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    AESARadar* r = new AESARadar(h);

    // subType should be AESA
    ASSERT_TRUE(r->subType == Sensor::SubType::AESA,
                "AESARadar subType should be AESA");

    // Constructor should not crash
    ASSERT_TRUE(true, "AESARadar constructor should not crash");

    // getRadarConfig should return a valid config — verify key defaults
    aesa::RadarConfig cfg = r->getRadarConfig();
    ASSERT_TRUE(cfg.numElements > 0,    "numElements should be > 0 after construction");
    ASSERT_TRUE(cfg.beamWidth   > 0.0f, "beamWidth should be > 0 after construction");
    ASSERT_TRUE(cfg.maxAzimuth  > 0.0f, "maxAzimuth should be > 0 after construction");
    ASSERT_TRUE(cfg.minAzimuth  < 0.0f, "minAzimuth should be < 0 after construction");

    // maxDetectionAngle synced from cfg.maxAzimuth
    ASSERT_NEAR((double)r->maxDetectionAngle, (double)cfg.maxAzimuth, 0.001,
                "maxDetectionAngle should equal cfg.maxAzimuth after construction");

    // parentEntity should be null until engine sets it
    ASSERT_EQ(r->parentEntity, nullptr, "parentEntity should be null on construction");

    delete r;
    delete h;
}

void test_aesaradar_getSetRadarConfig() {
    std::cout << "\n--- Running getRadarConfig / setRadarConfig Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    AESARadar* r = new AESARadar(h);

    // Read default config
    aesa::RadarConfig cfg = r->getRadarConfig();
    ASSERT_TRUE(true, "getRadarConfig() should not crash");

    // Modify a field and set it back
    float origBeamWidth = cfg.beamWidth;
    cfg.beamWidth = 5.0f;
    try {
        r->setRadarConfig(cfg);
        ASSERT_TRUE(true, "setRadarConfig() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "setRadarConfig() crashed");
    }

    // Verify the change was applied
    aesa::RadarConfig updated = r->getRadarConfig();
    ASSERT_NEAR((double)updated.beamWidth, 5.0, 0.001,
                "beamWidth should be 5.0 after setRadarConfig");

    // Restore and verify
    cfg.beamWidth = origBeamWidth;
    r->setRadarConfig(cfg);
    aesa::RadarConfig restored = r->getRadarConfig();
    ASSERT_NEAR((double)restored.beamWidth, (double)origBeamWidth, 0.001,
                "beamWidth should restore to original after second setRadarConfig");

    // Modify multiple fields
    cfg = r->getRadarConfig();
    cfg.numElements           = 500;
    cfg.peakPowerPerElement_W = 50.0f;
    cfg.maxDutyCycle          = 0.30f;
    try {
        r->setRadarConfig(cfg);
        aesa::RadarConfig multi = r->getRadarConfig();
        ASSERT_EQ(multi.numElements,           500,   "(Multi-field) numElements should be 500");
        ASSERT_NEAR((double)multi.peakPowerPerElement_W, 50.0, 0.001, "(Multi-field) peakPower should be 50");
        ASSERT_NEAR((double)multi.maxDutyCycle, 0.30,  0.001, "(Multi-field) maxDutyCycle should be 0.30");
    } catch (...) {
        ASSERT_FALSE(true, "(Multi-field) setRadarConfig crashed");
    }

    delete r;
    delete h;
}

void test_aesaradar_markDisplayRangeDirty() {
    std::cout << "\n--- Running markDisplayRangeDirty Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    AESARadar* r = new AESARadar(h);

    // Should not crash — simple flag setter
    try {
        r->markDisplayRangeDirty();
        ASSERT_TRUE(true, "markDisplayRangeDirty() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "markDisplayRangeDirty() crashed");
    }

    // Multiple calls should not crash
    try {
        for (int i = 0; i < 10; i++) r->markDisplayRangeDirty();
        ASSERT_TRUE(true, "(10 calls) markDisplayRangeDirty should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(10 calls) markDisplayRangeDirty crashed");
    }

    delete r;
    delete h;
}

void test_aesaradar_lockBreak() {
    std::cout << "\n--- Running lockOn / breakLock Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    AESARadar* r = new AESARadar(h);

    // breakLock with no active lock should not crash
    try {
        r->breakLock();
        ASSERT_TRUE(true, "breakLock() with no active lock should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "breakLock() with no active lock crashed");
    }

    // lockOn with a target ID should not crash
    try {
        r->lockOn(12345u);
        ASSERT_TRUE(true, "lockOn(12345) should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "lockOn(12345) crashed");
    }

    // breakLock after lockOn should not crash
    try {
        r->breakLock();
        ASSERT_TRUE(true, "breakLock() after lockOn should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "breakLock() after lockOn crashed");
    }

    // lockOn with zero ID should not crash
    try {
        r->lockOn(0u);
        ASSERT_TRUE(true, "lockOn(0) should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "lockOn(0) crashed");
    }

    // lockOn with max ID should not crash
    try {
        r->lockOn(UINT32_MAX);
        ASSERT_TRUE(true, "lockOn(UINT32_MAX) should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "lockOn(UINT32_MAX) crashed");
    }

    delete r;
    delete h;
}

void test_aesaradar_toJson() {
    std::cout << "\n--- Running toJson Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    AESARadar* r = new AESARadar(h);

    // toJson should not crash
    QJsonObject obj;
    try {
        obj = r->toJson();
        ASSERT_TRUE(true, "toJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "toJson() crashed");
    }

    // Result should not be empty
    ASSERT_FALSE(obj.empty(), "toJson() should return non-empty QJsonObject");

    // Top-level identity keys
    ASSERT_TRUE(obj.contains("id"),         "toJson() should contain 'id' key");
    ASSERT_TRUE(obj.contains("name"),       "toJson() should contain 'name' key");
    ASSERT_TRUE(obj.contains("SensorType"), "toJson() should contain 'SensorType' key");
    ASSERT_EQ(obj["SensorType"].toString().toStdString(), std::string("AESARadar"),
              "toJson() SensorType should be 'AESARadar'");

    // All major sections should be present
    ASSERT_TRUE(obj.contains("array"),       "toJson() should contain 'array' section");
    ASSERT_TRUE(obj.contains("transmitter"), "toJson() should contain 'transmitter' section");
    ASSERT_TRUE(obj.contains("scan"),        "toJson() should contain 'scan' section");
    ASSERT_TRUE(obj.contains("waveform"),    "toJson() should contain 'waveform' section");
    ASSERT_TRUE(obj.contains("detection"),   "toJson() should contain 'detection' section");
    ASSERT_TRUE(obj.contains("platform"),    "toJson() should contain 'platform' section");
    ASSERT_TRUE(obj.contains("tracking"),    "toJson() should contain 'tracking' section");
    ASSERT_TRUE(obj.contains("propagation"), "toJson() should contain 'propagation' section");
    ASSERT_TRUE(obj.contains("noise"),       "toJson() should contain 'noise' section");
    ASSERT_TRUE(obj.contains("iff"),         "toJson() should contain 'iff' section");
    ASSERT_TRUE(obj.contains("nullSteering"),"toJson() should contain 'nullSteering' section");
    ASSERT_TRUE(obj.contains("mode"),        "toJson() should contain 'mode' key");

    // Sections should be objects
    ASSERT_TRUE(obj["array"].isObject(),       "toJson() 'array' should be a QJsonObject");
    ASSERT_TRUE(obj["transmitter"].isObject(), "toJson() 'transmitter' should be a QJsonObject");
    ASSERT_TRUE(obj["scan"].isObject(),        "toJson() 'scan' should be a QJsonObject");
    ASSERT_TRUE(obj["waveform"].isObject(),    "toJson() 'waveform' should be a QJsonObject");

    // Verify a nested value — array.numElements
    QJsonObject array = obj["array"].toObject();
    ASSERT_TRUE(array.contains("numElements"), "toJson() array should contain 'numElements'");

    delete r;
    delete h;
}

void test_aesaradar_fromJson() {
    std::cout << "\n--- Running fromJson Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    AESARadar* r = new AESARadar(h);

    // Round-trip: toJson then fromJson — config values should survive
    QJsonObject obj = r->toJson();

    // Modify config, then restore via fromJson
    aesa::RadarConfig cfg = r->getRadarConfig();
    cfg.beamWidth = 10.0f;
    r->setRadarConfig(cfg);

    try {
        r->fromJson(obj);
        ASSERT_TRUE(true, "(Round-trip) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Round-trip) fromJson() crashed");
    }

    // beamWidth should be restored from JSON
    aesa::RadarConfig restored = r->getRadarConfig();
    ASSERT_TRUE(restored.beamWidth < 10.0f,
                "(Round-trip) fromJson() should restore beamWidth to pre-modification value");

    // Empty QJsonObject should not crash
    try {
        r->fromJson(QJsonObject());
        ASSERT_TRUE(true, "(Empty JSON) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Empty JSON) fromJson() crashed");
    }

    // Partial JSON — only scan section
    QJsonObject partial;
    QJsonObject scan;
    scan["type"] = "Section";
    partial["scan"] = scan;
    try {
        r->fromJson(partial);
        ASSERT_TRUE(true, "(Partial JSON) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Partial JSON) fromJson() crashed");
    }

    // Wrong types (non-object section) should not crash
    QJsonObject wrongTypes;
    wrongTypes["array"] = "not_an_object";
    wrongTypes["mode"]  = 99;
    try {
        r->fromJson(wrongTypes);
        ASSERT_TRUE(true, "(Wrong types) fromJson() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Wrong types) fromJson() crashed");
    }

    delete r;
    delete h;
}

void test_aesaradar_signals() {
    std::cout << "\n--- Running Signal Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    AESARadar* r = new AESARadar(h);

    // schedulerDutyCycle signal — connect and verify it can be emitted
    bool dutyCycleFired = false;
    float receivedDuty  = -1.0f;
    QMetaObject::Connection c1 = QObject::connect(
        r, &AESARadar::schedulerDutyCycle,
        [&](float dc) { dutyCycleFired = true; receivedDuty = dc; }
        );
    emit r->schedulerDutyCycle(0.45f);
    ASSERT_TRUE(dutyCycleFired, "schedulerDutyCycle signal should be connectable and emittable");
    ASSERT_NEAR((double)receivedDuty, 0.45, 0.001, "schedulerDutyCycle should carry correct value");
    QObject::disconnect(c1);

    // iffResult signal
    bool iffFired = false;
    uint32_t receivedTrackID = 0;
    int receivedCode = -1;
    QMetaObject::Connection c2 = QObject::connect(
        r, &AESARadar::iffResult,
        [&](uint32_t tid, int code, uint32_t, float) {
            iffFired = true;
            receivedTrackID = tid;
            receivedCode = code;
        }
        );
    emit r->iffResult(99u, 1, 7700u, 0.95f);
    ASSERT_TRUE(iffFired, "iffResult signal should be connectable and emittable");
    ASSERT_EQ(receivedTrackID, 99u, "iffResult should carry correct trackID");
    ASSERT_EQ(receivedCode, 1,      "iffResult should carry correct responseCode");
    QObject::disconnect(c2);

    // drfmGhostDetected signal
    bool drfmFired = false;
    uint32_t drfmTargetID = 0;
    QMetaObject::Connection c3 = QObject::connect(
        r, &AESARadar::drfmGhostDetected,
        [&](uint32_t tid, float, float, float) {
            drfmFired = true;
            drfmTargetID = tid;
        }
        );
    emit r->drfmGhostDetected(42u, 5000.0f, 30.0f, 5.0f);
    ASSERT_TRUE(drfmFired, "drfmGhostDetected signal should be connectable and emittable");
    ASSERT_EQ(drfmTargetID, 42u, "drfmGhostDetected should carry correct targetID");
    QObject::disconnect(c3);

    // trackBelowDopplerNotch signal
    bool notchFired = false;
    QMetaObject::Connection c4 = QObject::connect(
        r, &AESARadar::trackBelowDopplerNotch,
        [&](uint32_t) { notchFired = true; }
        );
    emit r->trackBelowDopplerNotch(77u);
    ASSERT_TRUE(notchFired, "trackBelowDopplerNotch signal should be connectable and emittable");
    QObject::disconnect(c4);

    // externalTrackInjected signal
    bool extFired = false;
    uint32_t extTrackID = 0;
    QMetaObject::Connection c5 = QObject::connect(
        r, &AESARadar::externalTrackInjected,
        [&](uint32_t tid) { extFired = true; extTrackID = tid; }
        );
    emit r->externalTrackInjected(55u);
    ASSERT_TRUE(extFired, "externalTrackInjected signal should be connectable and emittable");
    ASSERT_EQ(extTrackID, 55u, "externalTrackInjected should carry correct trackID");
    QObject::disconnect(c5);

    delete r;
    delete h;
}

void test_aesaradar_stop() {
    std::cout << "\n--- Running stop() Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    AESARadar* r = new AESARadar(h);

    // stop() should not crash
    try {
        r->stop();
        ASSERT_TRUE(true, "stop() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "stop() crashed");
    }

    // stop() called multiple times should not crash
    try {
        r->stop();
        r->stop();
        ASSERT_TRUE(true, "(Multiple calls) stop() should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Multiple calls) stop() crashed");
    }

    delete r;
    delete h;
}

void test_aesaradar_scan_noParent() {
    std::cout << "\n--- Running scan() Without Parent Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    AESARadar* r = new AESARadar(h);

    // scan() with no parentEntity should return immediately — no crash
    // NOTE: parentEntity is null by default (set only by engine)
    ASSERT_EQ(r->parentEntity, nullptr, "parentEntity should be null (engine not running)");
    try {
        r->scan();
        ASSERT_TRUE(true, "scan() with null parentEntity should not crash (early return)");
    } catch (...) {
        ASSERT_FALSE(true, "scan() with null parentEntity crashed");
    }

    delete r;
    delete h;
}


void aesaRadar_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "       AESARADAR CUSTOM UNIT TESTS       " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_aesaradar_initialization();
    test_aesaradar_getSetRadarConfig();
    test_aesaradar_markDisplayRangeDirty();
    test_aesaradar_lockBreak();
    test_aesaradar_toJson();
    test_aesaradar_fromJson();
    test_aesaradar_signals();
    test_aesaradar_stop();
    test_aesaradar_scan_noParent();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
