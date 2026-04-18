#include "core/Hierarchy/Components/weaponprofile.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"
#include <iostream>
#include <QJsonObject>

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
// TEST SUITES FOR WEAPONPROFILE
// ==========================================

void test_WeaponProfile_initialization() {
    std::cout << "\n--- Running WeaponProfile Initialization Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    WeaponProfile* wp = new WeaponProfile(h);

    ASSERT_NEQ(wp, nullptr, "WeaponProfile object should be created");
    ASSERT_TRUE(wp->Active, "WeaponProfile should be active by default");
    ASSERT_NEQ(wp->weapons, nullptr, "Weapons map should be initialized");
    ASSERT_TRUE(wp->weapons->empty(), "Weapons map should be empty initially");

    delete wp;
    delete h;
}

void test_WeaponProfile_Subcomponent_Operations() {
    std::cout << "\n--- Running WeaponProfile Subcomponent Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    WeaponProfile* wp = new WeaponProfile(h);

    // 1. Test addSubComponent with different types
    wp->addSubComponent("Main_Missile", "Missile");
    wp->addSubComponent("Heavy_Bomb", "Bomb");
    wp->addSubComponent("Sub_Torpedo", "Torpedo");

    ASSERT_EQ(wp->weapons->size(), 3, "Should have 3 weapons after additions");

    // Get an ID for further testing
    std::string weaponID = wp->weapons->begin()->first;
    Weapon* w = (*wp->weapons)[weaponID];

    // 2. Test renameSubComponent
    wp->renameSubComponent(weaponID, "Updated_Weapon_Name");
    ASSERT_EQ(w->Name, "Updated_Weapon_Name", "Weapon name should be updated correctly");

    // 3. Test getWeapon helper
    Weapon* retrieved = wp->getWeapon(weaponID);
    ASSERT_EQ(retrieved, w, "getWeapon should return the correct pointer");

    // 4. Test removeSubComponent
    wp->removeSubComponent(weaponID);
    ASSERT_EQ(wp->weapons->size(), 2, "Map size should decrease after removal");
    ASSERT_EQ(wp->getWeapon(weaponID), nullptr, "Removed weapon should not be retrievable");

    delete wp;
    delete h;
}

void test_WeaponProfile_JSON_Logic() {
    std::cout << "\n--- Running WeaponProfile JSON Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    WeaponProfile* wp = new WeaponProfile(h);

    wp->addSubComponent("Rocket_Alpha", "Rocket");
    wp->addSubComponent("Artillery_Beta", "Artillery");

    // Test toJson
    QJsonObject root = wp->toJson();
    ASSERT_TRUE(root.contains("weapons"), "JSON should contain 'weapons' key");
    QJsonObject weaponsObj = root["weapons"].toObject();
    ASSERT_EQ(weaponsObj.size(), 2, "JSON should contain data for 2 weapons");

    // Test fromJson
    WeaponProfile* wp2 = new WeaponProfile(h);
    try {
        wp2->fromJson(root);
        ASSERT_EQ(wp2->weapons->size(), 2, "fromJson should restore 2 weapons from JSON");

        // Verify specific weapon type restoration
        bool foundRocket = false;
        for(auto const& [id, weapon] : *wp2->weapons) {
            if(weapon->Name == "Rocket_Alpha") foundRocket = true;
        }
        ASSERT_TRUE(foundRocket, "fromJson should preserve weapon names and data");
    } catch(...) {
        ASSERT_FALSE(true, "fromJson crashed during execution");
    }

    delete wp;
    delete wp2;
    delete h;
}

void test_WeaponProfile_Edge_Cases() {
    std::cout << "\n--- Running WeaponProfile Edge Case Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    WeaponProfile* wp = new WeaponProfile(h);

    // Test adding unknown type (should fall back to default logic)
    wp->addSubComponent("Unknown_Thing", "NonExistentType");
    ASSERT_EQ(wp->weapons->size(), 1, "Should still add a component even with unknown type");

    // Test updateSubComponent
    std::string id = wp->weapons->begin()->first;
    QJsonObject updateData;
    updateData["name"] = "New_Internal_Name";
    wp->updateSubComponent(id, updateData);
    ASSERT_EQ((*wp->weapons)[id]->Name, "New_Internal_Name", "updateSubComponent should modify weapon data");

    delete wp;
    delete h;
}

// ==========================================
// MAIN RUNNER
// ==========================================
void WeaponProfile_test() {
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;

    std::cout << "=========================================" << std::endl;
    std::cout << "      WEAPONPROFILE CUSTOM UNIT TESTS    " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_WeaponProfile_initialization();
    test_WeaponProfile_Subcomponent_Operations();
    test_WeaponProfile_JSON_Logic();
    test_WeaponProfile_Edge_Cases();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
