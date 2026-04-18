#include "GUI/Panel/radardisplay.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
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

void test_radar_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;

    RadarDisplay* d = new RadarDisplay();

    // sensor and entity should be null on construction
    ASSERT_EQ(d->sensor, nullptr, "Default sensor should be null");
    ASSERT_EQ(d->entity, nullptr, "Default entity should be null");

    // Constructor should not crash
    ASSERT_TRUE(true, "RadarDisplay constructor should not crash");

    // sizeHint
    QSize hint = d->sizeHint();
    ASSERT_TRUE(hint.width()  > 0, "sizeHint width should be > 0");
    ASSERT_TRUE(hint.height() > 0, "sizeHint height should be > 0");
    ASSERT_EQ(hint.width(), 520, "sizeHint width should be 520");

    // heightForWidth uses 16/9
    int h = d->heightForWidth(520);
    ASSERT_NEAR((double)h, 520.0 * (16.0/9.0), 2.0,
                "heightForWidth(520) should be ~924");

    // heightForWidth(0) should return 0
    ASSERT_EQ(d->heightForWidth(0), 0, "heightForWidth(0) should return 0");

    // minimumSize
    QSize minSz = d->minimumSize();
    ASSERT_TRUE(minSz.width()  > 0, "minimumSize width should be > 0");
    ASSERT_TRUE(minSz.height() > 0, "minimumSize height should be > 0");
    ASSERT_EQ(minSz.width(), 200, "minimumSize width should be 200");

    delete d;
}

void test_radar_heightForWidth() {
    std::cout << "\n--- Running heightForWidth Tests ---" << std::endl;

    RadarDisplay* d = new RadarDisplay();
    const double ASPECT = 16.0 / 9.0;

    ASSERT_NEAR((double)d->heightForWidth(400),  400.0  * ASPECT, 2.0, "heightForWidth(400)");
    ASSERT_NEAR((double)d->heightForWidth(800),  800.0  * ASPECT, 2.0, "heightForWidth(800)");
    ASSERT_NEAR((double)d->heightForWidth(200),  200.0  * ASPECT, 2.0, "heightForWidth(200)");
    ASSERT_NEAR((double)d->heightForWidth(1000), 1000.0 * ASPECT, 2.0, "heightForWidth(1000)");

    // Linear scaling
    int h1 = d->heightForWidth(300);
    int h2 = d->heightForWidth(600);
    ASSERT_NEAR((double)h2, (double)h1 * 2.0, 2.0, "heightForWidth should scale linearly");

    delete d;
}

void test_radar_setHierarchy() {
    std::cout << "\n--- Running setHierarchy Tests ---" << std::endl;

    RadarDisplay* d = new RadarDisplay();
    Hierarchy* h = new Hierarchy();

    try {
        d->setHierarchy(h);
        ASSERT_TRUE(true, "setHierarchy(valid) should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "setHierarchy(valid) crashed");
    }

    try {
        d->setHierarchy(nullptr);
        ASSERT_TRUE(true, "setHierarchy(nullptr) should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "setHierarchy(nullptr) crashed");
    }

    delete d;
    delete h;
}

void test_radar_setDisplayMode() {
    std::cout << "\n--- Running setDisplayMode Tests ---" << std::endl;

    RadarDisplay* d = new RadarDisplay();

    try {
        d->setDisplayMode(RadarDisplay::DisplayMode::SURFACE);
        ASSERT_TRUE(true, "setDisplayMode(SURFACE) should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "setDisplayMode(SURFACE) crashed");
    }

    try {
        d->setDisplayMode(RadarDisplay::DisplayMode::AIR);
        ASSERT_TRUE(true, "setDisplayMode(AIR) should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "setDisplayMode(AIR) crashed");
    }

    // Toggle sequence
    d->setDisplayMode(RadarDisplay::DisplayMode::AIR);
    d->setDisplayMode(RadarDisplay::DisplayMode::SURFACE);
    d->setDisplayMode(RadarDisplay::DisplayMode::AIR);
    ASSERT_TRUE(true, "setDisplayMode toggle sequence should not crash");

    delete d;
}

void test_radar_selectEntity() {
    std::cout << "\n--- Running selectEntity Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    RadarDisplay* d = new RadarDisplay();
    d->setHierarchy(h);

    // nullptr — no crash, entity stays null
    try {
        d->selectEntity(nullptr);
        ASSERT_TRUE(true, "(nullptr) selectEntity should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(nullptr) selectEntity crashed");
    }
    ASSERT_EQ(d->entity, nullptr, "(nullptr) entity should remain null");
    ASSERT_EQ(d->sensor, nullptr, "(nullptr) sensor should remain null");

    // Non-Platform rejected
    Sensor* nonPlatform = new Sensor(h);
    nonPlatform->Name = "not_a_platform";
    try {
        d->selectEntity(nonPlatform);
        ASSERT_TRUE(true, "(non-Platform) selectEntity should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(non-Platform) selectEntity crashed");
    }
    ASSERT_EQ(d->entity, nullptr, "(non-Platform) entity should remain null");

    // Valid Platform accepted
    Platform* platform = new Platform(h);
    platform->Name = "radar_test_platform";
    try {
        d->selectEntity(platform);
        ASSERT_TRUE(true, "(Platform) selectEntity should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Platform) selectEntity crashed");
    }
    ASSERT_EQ(d->entity, platform, "(Platform) entity should be set");

    // sensor null since no Generic sensor attached
    ASSERT_EQ(d->sensor, nullptr, "(Platform no sensor) sensor should be null when no Generic sensor found");

    // Repeated calls should not crash
    try {
        d->selectEntity(platform);
        d->selectEntity(platform);
        ASSERT_TRUE(true, "(Repeated) selectEntity should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Repeated) selectEntity crashed");
    }

    delete d;
    delete h;
}

void test_radar_removeEntity() {
    std::cout << "\n--- Running RemoveEntity Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    RadarDisplay* d = new RadarDisplay();
    d->setHierarchy(h);

    Platform* platform = new Platform(h);
    platform->Name = "remove_radar_test";
    d->selectEntity(platform);

    QString correctID = QString::fromStdString(platform->ID);

    // Wrong ID should not clear entity
    d->RemoveEntity("wrong_id_abc");
    ASSERT_EQ(d->entity, platform, "(Wrong ID) RemoveEntity should not clear entity");

    // Correct ID should clear entity and sensor
    d->RemoveEntity(correctID);
    ASSERT_EQ(d->entity, nullptr, "(Correct ID) RemoveEntity should clear entity");
    ASSERT_EQ(d->sensor, nullptr, "(Correct ID) RemoveEntity should clear sensor");

    // Empty string should not crash
    try {
        d->RemoveEntity("");
        ASSERT_TRUE(true, "(Empty ID) RemoveEntity should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Empty ID) RemoveEntity crashed");
    }

    // Already null should not crash
    try {
        d->RemoveEntity(correctID);
        ASSERT_TRUE(true, "(Already null) RemoveEntity should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Already null) RemoveEntity crashed");
    }

    delete d;
    delete h;
}

void test_radar_updateRadar() {
    std::cout << "\n--- Running updateRadar Tests ---" << std::endl;

    Hierarchy* h = new Hierarchy();
    RadarDisplay* d = new RadarDisplay();
    d->setHierarchy(h);

    // No entity/sensor — early return, no crash
    try {
        d->updateRadar();
        ASSERT_TRUE(true, "(No entity) updateRadar should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(No entity) updateRadar crashed");
    }

    // Entity without sensor
    Platform* platform = new Platform(h);
    platform->Name = "update_radar_test";
    d->selectEntity(platform);
    try {
        d->updateRadar();
        ASSERT_TRUE(true, "(Entity no sensor) updateRadar should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Entity no sensor) updateRadar crashed");
    }

    // Multiple calls
    try {
        for (int i = 0; i < 10; i++) d->updateRadar();
        ASSERT_TRUE(true, "(10 calls) updateRadar should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(10 calls) updateRadar crashed");
    }

    delete d;
    delete h;
}

void test_radar_updateFromJson() {
    std::cout << "\n--- Running updateFromJson Tests ---" << std::endl;

    RadarDisplay* d = new RadarDisplay();
    d->resize(520, 400);

    // Empty QJsonObject should not crash
    try {
        d->updateFromJson(QJsonObject());
        ASSERT_TRUE(true, "(Empty JSON) updateFromJson should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Empty JSON) updateFromJson crashed");
    }

    // Full JSON with all keys — should not crash
    QJsonObject full;
    full["range"]         = 150.0;
    full["azimuth"]       = 45.0;
    full["current_speed"] = 250;
    full["max_speed"]     = 500;
    full["height"]        = 1000;
    try {
        d->updateFromJson(full);
        ASSERT_TRUE(true, "(Full JSON no targets) updateFromJson should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Full JSON no targets) updateFromJson crashed");
    }

    // JSON with valid targets array
    QJsonObject withTargets;
    QJsonArray targets;
    QJsonObject t1;
    t1["angle"]     = 30.0;
    t1["radius"]    = 50.0;
    t1["speed"]     = 200.0;
    t1["direction"] = 90.0;
    t1["altitude"]  = 5000.0;
    targets.append(t1);
    QJsonObject t2;
    t2["angle"]  = -15.0;
    t2["radius"] = 75.0;
    targets.append(t2);
    withTargets["targets"] = targets;
    try {
        d->updateFromJson(withTargets);
        ASSERT_TRUE(true, "(JSON with targets) updateFromJson should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(JSON with targets) updateFromJson crashed");
    }

    // Targets array with malformed entry (missing radius) — should not crash
    QJsonArray badTargets;
    QJsonObject bad;
    bad["angle"] = 10.0; // missing "radius"
    badTargets.append(bad);
    QJsonObject badJson;
    badJson["targets"] = badTargets;
    try {
        d->updateFromJson(badJson);
        ASSERT_TRUE(true, "(Malformed target) updateFromJson should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(Malformed target) updateFromJson crashed");
    }

    // Multiple sequential calls should not crash
    try {
        for (int i = 0; i < 5; i++) d->updateFromJson(full);
        ASSERT_TRUE(true, "(5 calls) updateFromJson should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(5 calls) updateFromJson crashed");
    }

    delete d;
}

void test_radar_paintEvent() {
    std::cout << "\n--- Running paintEvent Tests ---" << std::endl;

    RadarDisplay* d = new RadarDisplay();
    d->resize(520, 400);

    // AIR mode, no entity
    try {
        d->setDisplayMode(RadarDisplay::DisplayMode::AIR);
        d->repaint();
        ASSERT_TRUE(true, "(AIR, no entity) paintEvent should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(AIR, no entity) paintEvent crashed");
    }

    // SURFACE mode, no entity
    try {
        d->setDisplayMode(RadarDisplay::DisplayMode::SURFACE);
        d->repaint();
        ASSERT_TRUE(true, "(SURFACE, no entity) paintEvent should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(SURFACE, no entity) paintEvent crashed");
    }

    // With entity, no sensor — AIR
    Hierarchy* h = new Hierarchy();
    Platform* platform = new Platform(h);
    platform->Name = "paint_radar_test";
    d->setHierarchy(h);
    d->selectEntity(platform);

    try {
        d->setDisplayMode(RadarDisplay::DisplayMode::AIR);
        d->repaint();
        ASSERT_TRUE(true, "(AIR, entity no sensor) paintEvent should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(AIR, entity no sensor) paintEvent crashed");
    }

    // With entity, no sensor — SURFACE
    try {
        d->setDisplayMode(RadarDisplay::DisplayMode::SURFACE);
        d->repaint();
        ASSERT_TRUE(true, "(SURFACE, entity no sensor) paintEvent should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(SURFACE, entity no sensor) paintEvent crashed");
    }

    // paintEvent after updateFromJson populates fallback targets
    QJsonObject json;
    QJsonArray tgts;
    QJsonObject tgt;
    tgt["angle"] = 45.0; tgt["radius"] = 30.0;
    tgts.append(tgt);
    json["targets"] = tgts;
    json["range"] = 100.0;
    d->updateFromJson(json);
    try {
        d->repaint();
        ASSERT_TRUE(true, "(JSON targets) paintEvent should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(JSON targets) paintEvent crashed");
    }

    // After RemoveEntity
    d->RemoveEntity(QString::fromStdString(platform->ID));
    try {
        d->repaint();
        ASSERT_TRUE(true, "(After remove) paintEvent should not crash");
    } catch (...) {
        ASSERT_FALSE(true, "(After remove) paintEvent crashed");
    }

    delete d;
    delete h;
}


void radarDisplay_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "     RADARDISPLAY CUSTOM UNIT TESTS      " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_radar_initialization();
    test_radar_heightForWidth();
    test_radar_setHierarchy();
    test_radar_setDisplayMode();
    test_radar_selectEntity();
    test_radar_removeEntity();
    test_radar_updateRadar();
    test_radar_updateFromJson();
    test_radar_paintEvent();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
