#include "core/Hierarchy/Components/trajectory.h"
#include "core/Hierarchy/Struct/waypoints.h"
#include <iostream>
#include <QJsonObject>
#include <QJsonArray>

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
// TEST SUITES FOR TRAJECTORY
// ==========================================

void test_Trajectory_initialization() {
    std::cout << "\n--- Running Trajectory Initialization Tests ---" << std::endl;
    Trajectory* t = new Trajectory();

    ASSERT_NEQ(t, nullptr, "Trajectory object should be created");
    ASSERT_TRUE(t->Active, "Default Active status should be true");
    ASSERT_TRUE(t->FollowPath, "Default FollowPath should be true");
    ASSERT_EQ(t->current, 0, "Initial waypoint index should be 0");
    ASSERT_TRUE(t->Trajectories.empty(), "Trajectories list should be empty initially");

    delete t;
}

void test_Trajectory_Waypoint_Management() {
    std::cout << "\n--- Running Waypoint Management Tests ---" << std::endl;
    Trajectory* t = new Trajectory();

    // 1. Test addWaypoint
    t->addWaypoint(10.0f, 20.0f, 30.0f);
    ASSERT_EQ(t->Trajectories.size(), 1, "Should have 1 waypoint after addition");
    ASSERT_EQ(t->Trajectories[0]->position->x, 10.0f, "X coordinate should match");

    // 2. Test addWaypoint with sensor flag
    t->addWaypoint(40.0f, 50.0f, 60.0f, true);
    ASSERT_EQ(t->Trajectories.size(), 2, "Should have 2 waypoints");
    ASSERT_TRUE(t->Trajectories[1]->sensor, "Sensor flag should be true for 2nd waypoint");

    // 3. Test removeTrajectory
    bool removed = t->removeTrajectory(0);
    ASSERT_TRUE(removed, "removeTrajectory should return true for valid index");
    ASSERT_EQ(t->Trajectories.size(), 1, "Should have 1 waypoint after removal");
    ASSERT_EQ(t->Trajectories[0]->position->x, 40.0f, "Remaining waypoint should be the old 2nd one");

    // 4. Test remove out of bounds
    bool removedInvalid = t->removeTrajectory(10);
    ASSERT_FALSE(removedInvalid, "removeTrajectory should return false for invalid index");

    delete t;
}

void test_Trajectory_Navigation_Logic() {
    std::cout << "\n--- Running Navigation Logic Tests ---" << std::endl;
    Trajectory* t = new Trajectory();

    t->addWaypoint(0, 0, 0); // index 0
    t->addWaypoint(1, 1, 1); // index 1
    t->current = 1;

    // Test getTargetWaypoint
    Waypoints* target = t->getTargetWaypoint();
    ASSERT_NEQ(target, nullptr, "Should return valid target waypoint");
    ASSERT_EQ(target->position->x, 1.0f, "Target should be waypoint at current index");

    // Test goHome
    t->goHome();
    ASSERT_TRUE(t->reverse, "goHome should set reverse flag to true");
    ASSERT_EQ(t->current, 0, "goHome should decrement current index");

    // Test sensors toggle
    t->activateSensors();
    ASSERT_TRUE(t->Trajectories[0]->sensor, "activateSensors should enable sensor on all waypoints");

    t->deactivateSensors();
    ASSERT_FALSE(t->Trajectories[0]->sensor, "deactivateSensors should disable sensor on all waypoints");

    delete t;
}

void test_Trajectory_JSON_Serialization() {
    std::cout << "\n--- Running Trajectory JSON Tests ---" << std::endl;
    Trajectory* t = new Trajectory();
    t->addWaypoint(5, 5, 5);
    t->FollowPath = false;

    // 1. Test toJson
    QJsonObject obj = t->toJson();
    ASSERT_TRUE(obj.contains("waypoints"), "JSON should contain waypoints array");
    ASSERT_EQ(obj["followPath"].toBool(), false, "JSON should reflect FollowPath status");

    // 2. Test fromJson
    Trajectory* t2 = new Trajectory();
    try {
        t2->fromJson(obj);
        ASSERT_EQ(t2->Trajectories.size(), 1, "fromJson should restore waypoints");
        ASSERT_FALSE(t2->FollowPath, "fromJson should restore FollowPath status");
        ASSERT_EQ(t2->Trajectories[0]->position->y, 5.0f, "Waypoint data should be correct");
    } catch (...) {
        ASSERT_FALSE(true, "fromJson crashed");
    }

    delete t;
    delete t2;
}

// ==========================================
// MAIN RUNNER
// ==========================================
void Trajectory_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "      TRAJECTORY CUSTOM UNIT TESTS      " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_Trajectory_initialization();
    test_Trajectory_Waypoint_Management();
    test_Trajectory_Navigation_Logic();
    test_Trajectory_JSON_Serialization();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
