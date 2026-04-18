#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/profilecategaory.h"
#include <iostream>

// // --- Custom Testing Framework Variables ---
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

void test_profile_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);

    ASSERT_TRUE(p->ID.size()>0, "Initially ID should be define ");
    ASSERT_TRUE(p->Folders.empty(), "Initially folders should be empty");
    ASSERT_TRUE(p->Entities.empty(), "Initially folders should be empty");

    delete p;
    delete h;
}

void test_profile_folder(){
    std::cout << "\n--- Running profile_folder Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);
    //Test Add Folder Function
    //correct values
    Folder* folder = p->addFolder(p->ID,"test_Folder1");
    // Check if Folder is not null
    ASSERT_NEQ(folder, nullptr, "(Correct Values) addFolder Function should be created test_Folder1");

    //Test addFolderWithObject Function
    //correct values
    Folder* foldObj = new Folder(h);
    foldObj->Name = "folder2";
    p->addFolderWithObject(foldObj);
    // Check if Folder is not null
    auto it = p->Folders.find(foldObj->ID);
    bool found = (it != h->Folders.end());
    ASSERT_TRUE(found, "(Correct Values) addFolderWithObject Function should be Added folder2");

    //null values
    try{
        p->addFolderWithObject(nullptr);
        ASSERT_TRUE(true, "(null Values) addFolderWithObject Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(null Values) addFolderWithObject Function crashed");
    }

    //Test removeFolder Function
    //correct values
    std::string testID = foldObj->ID;
    p->removeFolder(testID);
    it = p->Folders.find(testID);
    found = (it != h->Folders.end());
    ASSERT_FALSE(found, "(Correct Values) removeFolder Function should be removed folder2");

    //wrong values
    try{
        p->removeFolder("wrong");
        ASSERT_TRUE(true, "(wrong Values) removeFolder Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(wrong Values) removeFolder Function crashed");
    }

    p->removeFolder(folder->ID);
    delete p;
    delete h;
}

void test_profile_entity(){
    std::cout << "\n--- Running profile_entity Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);

    //Test Add Entity Function
    //correct values
    Entity* entity = p->addEntity(p->ID,"test_entity1");
    // Check if Entity is not null
    ASSERT_NEQ(entity, nullptr, "(Correct Values) addEntity Function should be created test_entity1");

    //Test addEntityWithObject Function
    //correct values
    Entity* entObj = new Platform(h);
    entObj->Name = "entity2";
    p->addEntityWithObject(entObj);
    // Check if Entity is not null
    auto it = p->Entities.find(entObj->ID);
    bool found = (it != h->Entities.end());
    ASSERT_TRUE(found, "(Correct Values) addEntityWithObject Function should be Added entity2");

    //null values
    try{
        p->addEntityWithObject(nullptr);
        ASSERT_TRUE(true, "(null Values) addEntityWithObject Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(null Values) addEntityWithObject Function crashed");
    }

    //Test removeEntity Function
    //correct values
    std::string testID = entObj->ID;
    p->removeEntity(testID);
    it = p->Entities.find(testID);
    found = (it != h->Entities.end());
    ASSERT_FALSE(found, "(Correct Values) removeEntity Function should be removed entity2");

    //wrong values
    try{
        p->removeEntity("wrong");
        ASSERT_TRUE(true, "(wrong Values) removeEntity Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(wrong Values) removeEntity Function crashed");
    }

    p->removeEntity(entity->ID);
    delete p;
    delete h;
}

void test_profile_OthersFunction() {
    std::cout << "\n--- Running profile_OthersFunction Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = h->addProfileCategaory("test_Profile1");
    // Check map existence and pointer match
    auto it = h->ProfileCategories.find(p->ID);
    bool found = (it != h->ProfileCategories.end());
    if(found){

        //Test setProfileType Function
        p->setProfileType(Constants::EntityType::Sensor);
        ASSERT_EQ(p->type,Constants::EntityType::Sensor, " setProfileType Function should be change type ");

        //Test toJson Function
        Sensor* entity = new Sensor(h);
        p->addEntityWithObject(entity);
        QJsonObject obj = p->toJson();
        ASSERT_FALSE(obj.empty(), " toJson Function should be return Jsonobject ");

        //Test fromJson Function
        try{
            p->fromJson(obj);
            ASSERT_TRUE(true, "(Correct Values) fromJson Function worked successfully ");
        }catch(...){
            ASSERT_FALSE(true, "(Correct Values) fromJson Function failed ");
        }

        //Wrong values
        try{
            p->fromJson(QJsonObject());
            ASSERT_TRUE(true, "(wrong Values) fromJson Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(wrong Values) fromJson Function crashed");
        }

        p->removeEntity(entity->ID);
        delete p;

    }
    delete h;
}


void profileCategory_test(){
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "      PROFILECATEGAORY CUSTOM UNIT TESTS        " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_profile_initialization();
    test_profile_folder();
    test_profile_entity();
    test_profile_OthersFunction();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
