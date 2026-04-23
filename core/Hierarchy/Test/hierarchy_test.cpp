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
    ASSERT_NEQ(nullProfile, nullptr, "Profile should not be null (pass nullptr hierarchy)");
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

void test_Folder() {
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
        ASSERT_NEQ(folder, nullptr, "(Correct Values) addFolder Function should be created test_Folder1");
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

        //Folder to Folder create
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
        ASSERT_NEQ(it->second->Name,"renameFolder2", "(null Values) renameFolder Function should not be rename");

        //Wrong values
        h->renameFolder("wrong","renameFolder3");
        // Check if Folder is not null
        it = h->Folders.find(folder->ID);
        ASSERT_NEQ(it->second->Name,"renameFolder3", "(wrong Values) renameFolder Function should not be rename");

        //Test removeFolder Function
        //correct values
        h->removeFolder(QString::fromStdString(folder->ID),"fdID8");
        // Check if Folder is not null
        it = h->Folders.find("fdID8");
        found = (it != h->Folders.end());
        ASSERT_FALSE(found, " removeFolder Function should be deleted folder8");

        //null values
        h->removeFolder(nullptr,"fdID5");
        // Check if Folder is not null
        it = h->Folders.find("fdID5");
        found = (it != h->Folders.end());
        ASSERT_TRUE(found, "(null Values) removeFolder Function should not be deleted folder5");

        //wrong values
        h->removeFolder("wrong","fdID5");
        // Check if Folder is not null
        it = h->Folders.find("fdID5");
        found = (it != h->Folders.end());
        ASSERT_TRUE(found, "(wrong Values) removeFolder Function should not be deleted folder5");

        //Test removeFolderViaNetwork Function
        //correct values
        h->removeFolderViaNetwork("fdID5");
        // Check if Folder is not null
        it = h->Folders.find("fdID5");
        found = (it != h->Folders.end());
        ASSERT_FALSE(found, " removeFolderViaNetwork Function should be deleted folder5");

        //null values
        h->removeFolderViaNetwork(nullptr);
        ASSERT_TRUE(true, "(null Values) removeFolderViaNetwork Function should not be deleted folder5");

        //wrong values
        h->removeFolderViaNetwork("wrong");
        ASSERT_TRUE(true, "(wrong Values) removeFolderViaNetwork Function should not be deleted folder5");

        h->removeFolder(QString::fromStdString(profile->ID),QString::fromStdString(folder->ID));
        h->removeFolder(QString::fromStdString(folder->ID),QString::fromStdString(folder4->ID));
        h->removeProfileCategaory(QString::fromStdString(profile->ID));
        ASSERT_TRUE(false, "removeProfileCategaory Function clear folders and profile (Memory Leak Test)");
    }

    delete h;
}


void test_Entity() {
    std::cout << "\n--- Running Entity Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* profile = h->addProfileCategaory("test_Profile1");
    // Check map existence and pointer match
    auto it = h->ProfileCategories.find(profile->ID);
    bool found = (it != h->ProfileCategories.end());
    if(found){
        Folder* folder = h->addFolder(QString::fromStdString(profile->ID),"test_Folder1",true);

        //Test Add Entity Function
        //correct values
        Entity* entity = h->addEntity(QString::fromStdString(profile->ID),"test_entity1",true);
        // Check if Entity is not null
        ASSERT_NEQ(entity, nullptr, "(Correct Values) addEntity Function should be created test_entity1");
        //Null values
        Entity* entity2 = h->addEntity(nullptr,nullptr,true);
        // Check if Entity is not null
        ASSERT_EQ(entity2, nullptr, "(Null Values) addEntity Function should be return nullptr");

        //Wrong values
        Entity* entity3 = h->addEntity("wrong","entity3",true);
        // Check if Entity is not null
        ASSERT_EQ(entity3, nullptr, "(wrong Values) addEntity Function should be return nullptr");

        //Entity to Folder create
        Entity* entity4 = h->addEntity(QString::fromStdString(folder->ID),"entity4",false);
        // Check if Entity is not null
        ASSERT_NEQ(entity4, nullptr, "addEntity Function should be created entity4 in test_Folder1");


        //Test addEntityViaNetwork  Function
        //correct values
        h->addEntityViaNetwork(QString::fromStdString(profile->ID),"enID5","entity5",true);
        // Check if Entity is not null
        auto it = h->Entities.find("enID5");
        bool found = (it != h->Entities.end());
        ASSERT_TRUE(found, "(Correct Values) addEntityViaNetwork Function should be created entity5");

        //Null values
        h->addEntityViaNetwork(nullptr,"enID6",nullptr,true);
        // Check if Entity is not null
        it = h->Entities.find("fdID6");
        found = (it != h->Entities.end());
        ASSERT_FALSE(found, "(Null Values) addEntityViaNetwork Function should be return nullptr");

        //wrong values
        h->addEntityViaNetwork("wrong","enID7","entity7",true);
        // Check if Entity is not null
        it = h->Entities.find("enID7");
        found = (it != h->Entities.end());
        ASSERT_FALSE(found, "(wrong Values) addEntityViaNetwork Function should be return nullptr");

        //Folder to Entity create
        h->addEntityViaNetwork(QString::fromStdString(folder->ID),"enID8","entity8",false);
        // Check if Entity is not null
        it = h->Entities.find("enID8");
        found = (it != h->Entities.end());
        ASSERT_TRUE(found, " addEntityViaNetwork Function should be created entity8 in test_Folder1");

        //Test addEntityFromJson  Function
        //correct value
        Entity* entityobj = new Platform(h);
        QJsonObject obj = entityobj->toJson();
        Entity* entity9 = h->addEntityFromJson(QString::fromStdString(profile->ID),obj,true);
        // Check if Entity is not null
        ASSERT_NEQ(entity9, nullptr, "(Correct Values) addEntityFromJson Function should be created entity9");

        //Null values
        Entity* entity10 = h->addEntityFromJson(nullptr,obj,true);
        // Check if Entity is not null
        ASSERT_EQ(entity10, nullptr, "(Null Values) addEntityFromJson Function should be return nullptr");

        //wrong values
        Entity* entity11 = h->addEntityFromJson("wrong",obj,true);
        // Check if Entity is not null
        ASSERT_EQ(entity11, nullptr, "(wrong Values) addEntityFromJson Function should be return nullptr");

        //Folder to Entity create
        Entity* entity12 = h->addEntityFromJson(QString::fromStdString(folder->ID),obj,false);
        // Check if Entity is not null
        ASSERT_NEQ(entity12, nullptr, " addEntityFromJson Function should be created entity12 in test_Folder1");

        //Test renameEntity Function
        //correct values
        h->renameEntity(QString::fromStdString(entity->ID),"renameEntity1");
        // Check if Folder is not null
        it = h->Entities.find(entity->ID);
        ASSERT_EQ(it->second->Name,"renameEntity1", "(Correct Values) renameEntity Function should be rename");

        //null values
        h->renameEntity(nullptr,"renameEntity2");
        // Check if Folder is not null
        it = h->Entities.find(entity->ID);
        ASSERT_NEQ(it->second->Name,"renameEntity2", "(null Values) renameEntity Function should not be rename");

        //Wrong values
        h->renameEntity("wrong","renameEntity3");
        // Check if Folder is not null
        it = h->Entities.find(entity->ID);
        ASSERT_NEQ(it->second->Name,"renameEntity3", "(wrong Values) renameEntity Function should not be rename");


        Entity* entity13 = h->getEntityById(QString::fromStdString(entity->ID));
        // Check if Entity is not null
        ASSERT_NEQ(entity13, nullptr, "(Correct Values) getEntityById Function should be created entity13");

        //Null values
        Entity* entity14 = h->getEntityById(nullptr);
        // Check if Entity is not null
        ASSERT_EQ(entity14, nullptr, "(Null Values) getEntityById Function should be return nullptr");

        //wrong values
        Entity* entity15 = h->getEntityById("wrong");
        // Check if Entity is not null
        ASSERT_EQ(entity15, nullptr, "(wrong Values) getEntityById Function should be return nullptr");


        //Test removeEntity Function
        //correct values
        h->removeEntity(QString::fromStdString(folder->ID),"enID8");
        // Check if Entity is not null
        it = h->Entities.find("enID8");
        found = (it != h->Entities.end());
        ASSERT_FALSE(found, " removeEntity Function should be deleted entity8");

        //null values
        h->removeEntity(nullptr,"enID5");
        // Check if Entity is not null
        it = h->Entities.find("enID5");
        found = (it != h->Entities.end());
        ASSERT_TRUE(found, "(null Values) removeEntity Function should not be deleted entity5");

        //wrong values
        h->removeEntity("wrong","enID5");
        // Check if Entity is not null
        it = h->Entities.find("enID5");
        found = (it != h->Entities.end());
        ASSERT_TRUE(found, "(wrong Values) removeEntity Function should not be deleted entity5");

        h->removeProfileCategaory(QString::fromStdString(profile->ID));
        ASSERT_TRUE(false, "removeProfileCategaory Function clear folders and profile (Memory Leak Test)");


    }

    delete h;
}

void test_Component() {
    std::cout << "\n--- Running Component Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* profile = h->addProfileCategaory("test_Profile1");
    // Check map existence and pointer match
    auto it = h->ProfileCategories.find(profile->ID);
    bool found = (it != h->ProfileCategories.end());
    if(found){
        //Test Add Entity Function
        //correct values
        Platform* entity = new Platform(h);
        profile->addEntityWithObject(entity);
        // Check if Entity is not null

        //Test addComponent Function
        //correct values
        h->addComponent(QString::fromStdString(entity->ID),"transform");
        // Check if Component is not null
        ASSERT_NEQ(entity->transform,nullptr, "(Correct Values) addComponent Function should be added transform");

        //null values
        h->addComponent(nullptr,"crossSection");
        // Check if Folder is not null
        ASSERT_EQ(entity->crossSection,nullptr, "(null Values) addComponent Function should not be added crossSection");

        //Wrong values
        h->addComponent("wrong","trajectory");
        // Check if Folder is not null
        ASSERT_EQ(entity->trajectory,nullptr, "(wrong Values) addComponent Function should  not be added trajectory");


        //Test getComponentData Function
        //correct values
        QJsonObject obj1 = h->getComponentData(QString::fromStdString(entity->ID),"transform");
        // Check if QJsonObject is not null
        ASSERT_FALSE(obj1.isEmpty(), "(Correct Values) getComponentData Function should be return transform Json Data");

        //null values
        QJsonObject obj2 = h->getComponentData(nullptr,"crossSection");
        // Check if QJsonObject is not null
        ASSERT_TRUE(obj2.isEmpty(), "(null Values) getComponentData Function should be return empty");

        //Wrong values
        QJsonObject obj3 = h->getComponentData("wrong","trajectory");
        // Check if QJsonObject is not null
        ASSERT_TRUE(obj3.isEmpty(), "(wrong Values) getComponentData Function should not be return empty");


        //Test UpdateComponent Function
        //correct values
        QJsonObject delta;
        delta["active"] = false;
        h->UpdateComponent(QString::fromStdString(entity->ID),"transform",delta);
        ASSERT_FALSE(entity->transform->Active, "(Correct Values) UpdateComponent Function should be update transform Json Data");

        //null values
        delta["active"] = true;
        h->UpdateComponent(nullptr,"transform",delta);
        ASSERT_FALSE(entity->transform->Active, "(null Values) UpdateComponent Function should not be update transform Json Data");

        //Wrong values
        delta["active"] = true;
        h->UpdateComponent("wrong","transform",delta);
        ASSERT_FALSE(entity->transform->Active, "(wrong Values) UpdateComponent Function should not be update transform Json Data");

        //Test removeComponent Function
        //correct values
        h->removeComponent(QString::fromStdString(entity->ID),"transform");
        ASSERT_EQ(entity->transform,nullptr, "(Correct Values) removeComponent Function should be removed component");

        //null values
        try{
            h->removeComponent(nullptr,"transform");
            ASSERT_FALSE(true, "(null Values) removeComponent Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(null Values) removeComponent Function crashed");
        }

        //Wrong values
        try{
            h->removeComponent("wrong","transform");
            ASSERT_TRUE(true, "(Wrong Values) removeComponent Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(Wrong Values) removeComponent Function crashed");
        }

        h->removeProfileCategaory(QString::fromStdString(profile->ID));
        ASSERT_TRUE(false, "removeProfileCategaory Function clear folders and profile (Memory Leak Test)");


    }
    delete h;
}


void test_SubComponent() {
    std::cout << "\n--- Running SubComponent Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* profile = h->addProfileCategaory("test_Profile1");
    // Check map existence and pointer match
    auto it = h->ProfileCategories.find(profile->ID);
    bool found = (it != h->ProfileCategories.end());
    if(found){
        //Test Add Entity Function
        Platform* entity = new Platform(h);
        profile->addEntityWithObject(entity);
        // Check if Entity is not null

        //Test addComponent Function
        h->addComponent(QString::fromStdString(entity->ID),"sensors");

        //Test addSubComponent Function
        //correct values
        h->addSubComponent(QString::fromStdString(entity->sensors->ID),"radar","Generic");
        Sensor *sensor = nullptr;
        for (auto& it : *entity->sensors->sensors) {
            if(it.second->Name == "radar"){
                sensor = it.second;
                break;
            }
        }
        // Check if Component is not null
        ASSERT_NEQ(sensor,nullptr, "(Correct Values) addSubComponent Function should be added Radar");

        //null values
        try{
            h->addSubComponent(nullptr,"radar");
            ASSERT_TRUE(true, "(null Values) addSubComponent Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(null Values) addSubComponent Function crashed");
        }

        //Wrong values
        try{
            h->addSubComponent("wrong","radar");
            ASSERT_TRUE(true, "(wrong Values) addSubComponent Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(wrong Values) addSubComponent Function crashed");
        }

        //Test renameSubComponent Function
        //correct values
        h->renameSubComponent(QString::fromStdString(entity->sensors->ID),QString::fromStdString(sensor->ID),"aesaradar");
        ASSERT_EQ(sensor->Name,"aesaradar", "(Correct Values) renameSubComponent Function should be rename sensor");

        //null values
        h->renameSubComponent(nullptr,QString::fromStdString(sensor->ID),"aesaradar2");
        ASSERT_NEQ(sensor->Name,"aesaradar2", "(null Values) renameSubComponent Function should not be rename sensor ");

        //Wrong values
        h->renameSubComponent("wrong",QString::fromStdString(sensor->ID),"aesaradar2");
        ASSERT_NEQ(sensor->Name,"aesaradar2", "(wrong Values) renameSubComponent Function should not be rename sensor ");


        //Test removeSubComponent Function
        //correct values
        h->removeSubComponent(QString::fromStdString(entity->sensors->ID),QString::fromStdString(sensor->ID));
        // Check if Component is not null
        bool found = false;
        for (auto& it : *entity->sensors->sensors) {
            if(it.second->Name == "radar"){
                found = true;
                break;
            }
        }
        ASSERT_FALSE(found, "(Correct Values) removeSubComponent Function should be removed Radar");

        //null values
        try{
            h->removeSubComponent(nullptr,nullptr);
            ASSERT_TRUE(true, "(null Values) removeSubComponent Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(null Values) removeSubComponent Function crashed");
        }

        //Wrong values
        try{
            h->removeSubComponent("wrong","wrong");
            ASSERT_TRUE(true, "(wrong Values) removeSubComponent Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(wrong Values) removeSubComponent Function crashed");
        }

        h->removeProfileCategaory(QString::fromStdString(profile->ID));
        ASSERT_TRUE(false, "removeProfileCategaory Function clear folders and profile (Memory Leak Test)");



    }
    delete h;
}

void test_OthersFunction() {
    std::cout << "\n--- Running OthersFunction Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* profile = h->addProfileCategaory("test_Profile1");
    // Check map existence and pointer match
    auto it = h->ProfileCategories.find(profile->ID);
    bool found = (it != h->ProfileCategories.end());
    if(found){
        //Test toJson Function
        Platform* entity = new Platform(h);
        profile->addEntityWithObject(entity);
        QJsonObject obj = h->toJson();
        ASSERT_FALSE(obj.empty(), " toJson Function should be return Jsonobject ");

        //Test fromJson Function
        try{
            h->fromJson(obj);
            ASSERT_TRUE(true, "(Correct Values) fromJson Function worked successfully ");
        }catch(...){
            ASSERT_FALSE(true, "(Correct Values) fromJson Function failed ");
        }

        //Wrong values
        try{
            h->fromJson(QJsonObject());
            ASSERT_TRUE(true, "(wrong Values) fromJson Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(wrong Values) fromJson Function crashed");
        }
        h->removeProfileCategaory(QString::fromStdString(profile->ID));
        ASSERT_TRUE(false, "removeProfileCategaory Function clear folders and profile (Memory Leak Test)");


    }
    delete h;
}

void hierarchy_test(){

    std::cout << "=========================================" << std::endl;
    std::cout << "      HIERARCHY CUSTOM UNIT TESTS        " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_initialization();
    test_ProfileCategaory();
    test_Folder();
    test_Entity();
    test_Component();
    test_SubComponent();
    test_OthersFunction();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
