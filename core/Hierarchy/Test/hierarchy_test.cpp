#include "core/Hierarchy/hierarchy.h"
#include <iostream>

// --- Custom Testing Framework Variables ---
int testsPassed = 0;
int testsFailed = 0;
int tests = 1;

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

void test_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();

    ASSERT_EQ(Hierarchy::getCurrentContext(), h, "Current context should be set in constructor");
    ASSERT_TRUE(h->tempData.empty(), "TempData should be initially empty");
    ASSERT_FALSE(h->isDatabase, "IS Database  should be initially False");
    ASSERT_FALSE(h->isScenario, "IS Scenario  should be initially False");
    ASSERT_FALSE(h->isRuntime, "IS Runtime  should be initially False");
    ASSERT_TRUE(h->fixedProfiles, "IS fixedProfiles  should be initially True");
    ASSERT_TRUE(h->ProfileCategories.empty(), "ProfileCategories should be initially empty");
    ASSERT_TRUE(h->dictionry.empty(), "Dictionry should be initially empty");
    ASSERT_TRUE(h->Folders.empty(), "Folders should be initially empty");
    ASSERT_TRUE(h->Entities.empty(), "Entities should be initially empty");
    ASSERT_TRUE(h->Platforms.empty(), "Platforms should be initially empty");
    ASSERT_TRUE(h->Radios.empty(), "Radios should be initially empty");
    ASSERT_TRUE(h->Sensors.empty(), "Sensors should be initially empty");
    ASSERT_TRUE(h->FixedPointes.empty(), "FixedPointes should be initially empty");
    ASSERT_TRUE(h->Formations.empty(), "Formations should be initially empty");
    ASSERT_TRUE(h->Specialzones.empty(), "Specialzones should be initially empty");
    ASSERT_TRUE(h->Iffs.empty(), "Iffs should be initially empty");
    ASSERT_TRUE(h->Weapons.empty(), "Weapons should be initially empty");
    ASSERT_TRUE(h->Components.empty(), "Components should be initially empty");
    ASSERT_TRUE(h->Meshes.empty(), "Meshes should be initially empty");
    ASSERT_TRUE(h->missionList.empty(), "MissionList should be initially empty");

    ASSERT_TRUE(h->EntityPaths.empty(), "EntityPaths should be initially empty");
    ASSERT_TRUE(h->FolderPaths.empty(), "FolderPaths should be initially empty");
    ASSERT_TRUE(h->redengagements.empty(), "redengagements should be initially empty");
    ASSERT_TRUE(h->reddetections.empty(), "reddetections should be initially empty");
    ASSERT_TRUE(h->reddamages.empty(), "reddamages should be initially empty");
    ASSERT_TRUE(h->blueengagements.empty(), "blueengagements should be initially empty");
    ASSERT_TRUE(h->bluedetections.empty(), "bluedetections should be initially empty");
    ASSERT_TRUE(h->bluedamages.empty(), "bluedamages should be initially empty");

    ASSERT_EQ(h->redlastengagments, 0, "Red Team Last Engagement should be initially 0");
    ASSERT_EQ(h->bluelastengagments, 0, "Blue Team Last Engagement should be initially 0");

    ASSERT_EQ(h->redlastdetections, 0, "Red Team Last Detections should be initially 0");
    ASSERT_EQ(h->bluelastdetections, 0, "Blue Team Last Detections should be initially 0");

    ASSERT_EQ(h->redlastdamages, 0, "Red Team Last Detections should be initially 0");
    ASSERT_EQ(h->bluelastdamages, 0, "Blue Team Last Detections should be initially 0");


    delete h;
}

void test_ProfileCategaory() {
    std::cout << "\n--- Running ProfileCategaory Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* profile = h->addProfileCategaory("test_Profile1");
    //create profile with nullptr
    ProfileCategaory* profile1 = h->addProfileCategaory(nullptr);

    // Check if profile is not null
    ASSERT_NEQ(profile, nullptr, "Profile should be created test_Profile1");
    ASSERT_EQ(profile1, nullptr, "Profile should not be created ");

    // Check map existence and pointer match
    auto it = h->ProfileCategories.find(profile->ID);
    bool found = (it != h->ProfileCategories.end());

    ASSERT_TRUE(found, "ProfileCategory should exist in the map");
    if (found) {
        ASSERT_EQ(it->second, profile, "Pointer in map should match the created profile");
    }

    /////////////////////////

    ProfileCategaory* nullProfile = new ProfileCategaory(nullptr);
    ASSERT_NEQ(nullProfile, nullptr, "Profile should be not be null (pass nullptr hierarchy)");
    delete nullProfile;

    ProfileCategaory* manualProfile = new ProfileCategaory(h);
    manualProfile->ID = "ManualID";
    manualProfile->Name = "ManualName";

    h->addProfileCategaoryWithObject(manualProfile);

    // Check if it exists in the map
    ASSERT_EQ(h->getProfileById("ManualID"), manualProfile, "Manual profile should be added correctly");

    ////////////////////////////
    ProfileCategaory* p1 = h->addProfileCategaory("UniqueName");
    std::string generatedId = p1->ID;

    // Test getProfileById
    ASSERT_EQ(h->getProfileById(QString::fromStdString(generatedId)), p1, "Should find profile by ID");
    ASSERT_EQ(h->getProfileById("NonExistentID"), nullptr, "Should return null for invalid ID");
    ASSERT_EQ(h->getProfileById(nullptr), nullptr, "Should return null for invalid ID");


    // Test getProfileByName
    ASSERT_EQ(h->getProfileByName("UniqueName"), p1, "Should find profile by Name");
    ASSERT_EQ(h->getProfileByName("WrongName"), nullptr, "Should return null for invalid Name");
    ASSERT_EQ(h->getProfileByName(nullptr), nullptr, "Should return null for invalid Name");


    // removeProfileCategaory
    h->removeProfileCategaory(QString::fromStdString(profile->ID));
    h->removeProfileCategaory(QString::fromStdString(manualProfile->ID));
    h->removeProfileCategaory(QString::fromStdString(generatedId));

    // Ab confirm karein ki delete ho gaya hai
    ASSERT_EQ(h->getProfileById(QString::fromStdString(profile->ID)), nullptr, "test_Profile1 should be null after removal");
    ASSERT_EQ(h->getProfileById(QString::fromStdString(manualProfile->ID)), nullptr, " ManualName Profile should be null after removal");
    ASSERT_EQ(h->getProfileById(QString::fromStdString(generatedId)), nullptr, "UniqueName Profile should be null after removal");

    // Safety check: Make sure size of map decreased
    ASSERT_EQ(h->ProfileCategories.size(), 0, "Map should be empty after removing the only element");

    delete h;
}

void test_FolderCategaory() {
    std::cout << "\n--- Running Folder Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* profile = h->addProfileCategaory("test_Profile1");
    // Check map existence and pointer match
    auto it = h->ProfileCategories.find(profile->ID);
    bool found = (it != h->ProfileCategories.end());
    if (found) {
        //Test Add Folder Function
        //correct values
        Folder* folder = h->addFolder(QString::fromStdString(profile->ID),"test_Folder1",true);
        // Check if Folder is not null
        ASSERT_NEQ(folder, nullptr, "(Correct Values) addFolder Function should be created test_Profile1");
        //Null values
        Folder* folder2 = h->addFolder(nullptr,nullptr,true);
        // Check if Folder is not null
        ASSERT_EQ(folder2, nullptr, "(Null Values) addFolder Function should be return nullptr");

        //Wrong values
        Folder* folder3 = h->addFolder("wrong","folder2",true);
        // Check if Folder is not null
        ASSERT_EQ(folder3, nullptr, "(wrong Values) addFolder Function should be return nullptr");

        //Folder to Folder create
        Folder* folder4 = h->addFolder(QString::fromStdString(folder->ID),"folder4",false);
        // Check if Folder is not null
        ASSERT_NEQ(folder4, nullptr, "addFolder Function should be created folder4 in test_Folder1");

        //Test addFolderViaNetwork  Function
        //correct values
        h->addFolderViaNetwork(QString::fromStdString(profile->ID),"fdID5","folder5",true);
        // Check if Folder is not null
        auto it = h->Folders.find("fdID5");
        bool found = (it != h->Folders.end());
        ASSERT_TRUE(found, "(Correct Values) addFolderViaNetwork Function should be created folder5");

        //Null values
        h->addFolderViaNetwork(nullptr,"fdID6",nullptr,true);
        // Check if Folder is not null
        it = h->Folders.find("fdID6");
        found = (it != h->Folders.end());
        ASSERT_FALSE(found, "(Null Values) addFolderViaNetwork Function should be return nullptr");

        //wrong values
        h->addFolderViaNetwork("wrong","fdID7","folder7",true);
        // Check if Folder is not null
        it = h->Folders.find("fdID7");
        found = (it != h->Folders.end());
        ASSERT_FALSE(found, "(wrong Values) addFolderViaNetwork Function should be return nullptr");

        //wrong values
        h->addFolderViaNetwork(QString::fromStdString(folder->ID),"fdID8","folder8",false);
        // Check if Folder is not null
        it = h->Folders.find("fdID8");
        found = (it != h->Folders.end());
        ASSERT_TRUE(found, " addFolderViaNetwork Function should be created folder8 in test_Folder1");

        //Test renameFolder Function
        //correct values
        h->renameFolder(QString::fromStdString(folder->ID),"renameFolder1");
        // Check if Folder is not null
        it = h->Folders.find(folder->ID);
        ASSERT_EQ(it->second->Name,"renameFolder1", "(Correct Values) renameFolder Function should be rename");

        //null values
        h->renameFolder(nullptr,"renameFolder2");
        // Check if Folder is not null
        it = h->Folders.find(folder->ID);
        ASSERT_NEQ(it->second->Name,"renameFolder2", "(null Values) renameFolder Function should be not be rename");


    }




    // /////////////////////////
    // ProfileCategaory* manualProfile = new ProfileCategaory(h);
    // manualProfile->ID = "ManualID";
    // manualProfile->Name = "ManualName";

    // h->addProfileCategaoryWithObject(manualProfile);

    // // Check if it exists in the map
    // ASSERT_EQ(h->getProfileById("ManualID"), manualProfile, "Manual profile should be added correctly");

    // ////////////////////////////
    // ProfileCategaory* p1 = h->addProfileCategaory("UniqueName");
    // std::string generatedId = p1->ID;

    // // Test getProfileById
    // ASSERT_EQ(h->getProfileById(QString::fromStdString(generatedId)), p1, "Should find profile by ID");
    // ASSERT_EQ(h->getProfileById("NonExistentID"), nullptr, "Should return null for invalid ID");

    // // Test getProfileByName
    // ASSERT_EQ(h->getProfileByName("UniqueName"), p1, "Should find profile by Name");
    // ASSERT_EQ(h->getProfileByName("WrongName"), nullptr, "Should return null for invalid Name");


    // // removeProfileCategaory
    // h->removeProfileCategaory(QString::fromStdString(profile->ID));
    // h->removeProfileCategaory(QString::fromStdString(manualProfile->ID));
    // h->removeProfileCategaory(QString::fromStdString(generatedId));

    // // Ab confirm karein ki delete ho gaya hai
    // ASSERT_EQ(h->getProfileById(QString::fromStdString(profile->ID)), nullptr, "test_Profile1 should be null after removal");
    // ASSERT_EQ(h->getProfileById(QString::fromStdString(manualProfile->ID)), nullptr, " ManualName Profile should be null after removal");
    // ASSERT_EQ(h->getProfileById(QString::fromStdString(generatedId)), nullptr, "UniqueName Profile should be null after removal");

    // // Safety check: Make sure size of map decreased
    // ASSERT_EQ(h->ProfileCategories.size(), 0, "Map should be empty after removing the only element");

    delete h;
}
void hierarchy_test(){

    std::cout << "=========================================" << std::endl;
    std::cout << "      HIERARCHY CUSTOM UNIT TESTS        " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_initialization();
    test_ProfileCategaory();
    test_FolderCategaory();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
