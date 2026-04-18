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
void test_folder_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);
    Folder* f = new Folder(h);

    ASSERT_TRUE(f->ID.size()>0, "Initially ID should be define ");
    ASSERT_TRUE(f->Folders.empty(), "Initially folders should be empty");
    ASSERT_TRUE(f->Entities.empty(), "Initially folders should be empty");

    delete p;
    delete h;
}

void test_folder_folder(){
    std::cout << "\n--- Running folder_folder Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);
    Folder* f = new Folder(h);
    //Test Add Folder Function
    //correct values
    Folder* folder = f->addFolder(f->ID,"test_Folder1");
    // Check if Folder is not null
    ASSERT_NEQ(folder, nullptr, "(Correct Values) addFolder Function should be created test_Folder1");

    //Test addFolderWithObject Function
    //correct values
    Folder* foldObj = new Folder(h);
    foldObj->Name = "folder2";
    f->addFolderWithObject(foldObj);
    // Check if Folder is not null
    auto it = f->Folders.find(foldObj->ID);
    bool found = (it != h->Folders.end());
    ASSERT_TRUE(found, "(Correct Values) addFolderWithObject Function should be Added folder2");

    //null values
    try{
        f->addFolderWithObject(nullptr);
        ASSERT_TRUE(true, "(null Values) addFolderWithObject Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(null Values) addFolderWithObject Function crashed");
    }

    //Test removeFolder Function
    //correct values
    std::string testID = foldObj->ID;
    f->removeFolder(testID);
    it = f->Folders.find(testID);
    found = (it != h->Folders.end());
    ASSERT_FALSE(found, "(Correct Values) removeFolder Function should be removed folder2");

    //wrong values
    try{
        f->removeFolder("wrong");
        ASSERT_TRUE(true, "(wrong Values) removeFolder Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(wrong Values) removeFolder Function crashed");
    }

    f->removeFolder(folder->ID);
    delete f;
    delete p;
    delete h;
}

void test_folder_entity(){
    std::cout << "\n--- Running folder_entity Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);
    Folder* f = new Folder(h);
    //Test Add Entity Function
    //correct values
    Entity* entity = f->addEntity(f->ID,"test_entity1");
    // Check if Entity is not null
    ASSERT_NEQ(entity, nullptr, "(Correct Values) addEntity Function should be created test_entity1");

    //Test addEntityWithObject Function
    //correct values
    Entity* entObj = new Platform(h);
    entObj->Name = "entity2";
    f->addEntityWithObject(entObj);
    // Check if Entity is not null
    auto it = f->Entities.find(entObj->ID);
    bool found = (it != h->Entities.end());
    ASSERT_TRUE(found, "(Correct Values) addEntityWithObject Function should be Added entity2");

    //null values
    try{
        f->addEntityWithObject(nullptr);
        ASSERT_TRUE(true, "(null Values) addEntityWithObject Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(null Values) addEntityWithObject Function crashed");
    }

    //Test removeEntity Function
    //correct values
    std::string testID = entObj->ID;
    f->removeEntity(testID);
    it = f->Entities.find(testID);
    found = (it != h->Entities.end());
    ASSERT_FALSE(found, "(Correct Values) removeEntity Function should be removed entity2");

    //wrong values
    try{
        f->removeEntity("wrong");
        ASSERT_TRUE(true, "(wrong Values) removeEntity Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(wrong Values) removeEntity Function crashed");
    }

    f->removeEntity(entity->ID);
    delete f;
    delete p;
    delete h;
}

void test_folder_OthersFunction() {
    std::cout << "\n--- Running folder_OthersFunction Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = h->addProfileCategaory("test_Profile1");
    Folder* f = new Folder(h);
    // Check map existence and pointer match
    auto it = h->ProfileCategories.find(p->ID);
    bool found = (it != h->ProfileCategories.end());
    if(found){

        //Test setProfileType Function
        f->setProfileType(Constants::EntityType::Sensor);
        ASSERT_EQ(f->type,Constants::EntityType::Sensor, " setProfileType Function should be change type ");

        //Test toJson Function
        Sensor* entity = new Sensor(h);
        f->addEntityWithObject(entity);
        QJsonObject obj = f->toJson();
        ASSERT_FALSE(obj.empty(), " toJson Function should be return Jsonobject ");

        //Test fromJson Function
        try{
            f->fromJson(obj);
            ASSERT_TRUE(true, "(Correct Values) fromJson Function worked successfully ");
        }catch(...){
            ASSERT_FALSE(true, "(Correct Values) fromJson Function failed ");
        }

        //Wrong values
        try{
            f->fromJson(QJsonObject());
            ASSERT_TRUE(true, "(wrong Values) fromJson Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(wrong Values) fromJson Function crashed");
        }

        f->removeEntity(entity->ID);
        delete f;
        delete p;

    }
    delete h;
}



void folder_test(){
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "      FOLDER CUSTOM UNIT TESTS        " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_folder_initialization();
    test_folder_folder();
    test_folder_entity();
    test_folder_OthersFunction();

    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
