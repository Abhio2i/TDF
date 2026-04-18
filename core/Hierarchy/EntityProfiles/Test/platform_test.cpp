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
void test_PLatform_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);
    Platform* e = new Platform(h);

    ASSERT_EQ(e->transform,nullptr, "Initially Transform should be null");
    ASSERT_EQ(e->trajectory,nullptr, "Initially trajectory should be null");
    ASSERT_EQ(e->rigidbody,nullptr, "Initially rigidbody should be null");
    ASSERT_EQ(e->dynamicModel,nullptr, "Initially dynamicModel should be null");
    ASSERT_EQ(e->collider,nullptr, "Initially collider should be null");
    ASSERT_EQ(e->meshRenderer2d,nullptr, "meshRenderer2d Transform should be null");
    ASSERT_EQ(e->crossSection,nullptr, "Initially crossSection should be null");
    ASSERT_EQ(e->sensors,nullptr, "Initially sensors should be null");
    ASSERT_EQ(e->radios,nullptr, "Initially radios should be null");
    ASSERT_EQ(e->weapons,nullptr, "Initially weapons should be null");
    ASSERT_EQ(e->iffs,nullptr, "Initially iffs should be null");

    delete e;
    delete p;
    delete h;
}

void test_Platform_component_function() {
    std::cout << "\n--- Running Platform_component Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);
    Platform* e = new Platform(h);

    //Test getSupportedComponents function
    std::vector<std::string> list = e->getSupportedComponents();
    ASSERT_FALSE(list.empty(), "getSupportedComponents function return componet list");

    //Test addComponent function
    // correct values
    e->addComponent("transform");
    ASSERT_NEQ(e->transform,nullptr, "(Correct Values) addComponent Function should be return component");

    // wrong values
    try{
        e->addComponent("wrong");
        ASSERT_TRUE(true, "(wrong Values) addComponent Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(wrong Values) addComponent Function crashed");
    }

    //Test getComponent function
    // correct values
    QJsonObject obj = e->getComponent("transform");
    ASSERT_FALSE(obj.isEmpty(), "(Correct Values) getComponent Function should be return json data");

    // wrong values
    obj = e->getComponent("wrong");
    ASSERT_TRUE(obj.empty(), "(wrong Values) getComponent Function should be return empty json data");

    //Test updateComponent Function
    //correct values
    QJsonObject delta;
    delta["active"] = false;
    e->updateComponent("transform",delta);
    ASSERT_FALSE(e->transform->Active, "(Correct Values) UpdateComponent Function should be update transform Json Data");

    //null values
    delta["active"] = true;
    e->updateComponent(nullptr,delta);
    ASSERT_FALSE(e->transform->Active, "(null Values) UpdateComponent Function should not be update transform Json Data");

    //Wrong values
    delta["active"] = true;
    e->updateComponent("wrong",delta);
    ASSERT_FALSE(e->transform->Active, "(wrong Values) UpdateComponent Function should not be update transform Json Data");

    //Test removeComponent function
    // correct values
    e->removeComponent("transform");
    ASSERT_EQ(e->transform,nullptr, "(Correct Values) removeComponent Function should be return null");

    // wrong values
    try{
        e->removeComponent("wrong");
        ASSERT_TRUE(true, "(wrong Values) removeComponent Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(wrong Values) removeComponent Function crashed");
    }

    delete e;
    delete p;
    delete h;
}

void test_platform_OthersFunction() {
    std::cout << "\n--- Running platform_OthersFunction Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = h->addProfileCategaory("test_Profile1");
    Platform* e = new Platform(h);
    // Check map existence and pointer match
    auto it = h->ProfileCategories.find(p->ID);
    bool found = (it != h->ProfileCategories.end());
    if(found){

        //Test toJson Function
        QJsonObject obj = e->toJson();
        ASSERT_FALSE(obj.empty(), " toJson Function should be return Jsonobject ");

        //Test fromJson Function
        try{
            e->fromJson(obj);
            ASSERT_TRUE(true, "(Correct Values) fromJson Function worked successfully ");
        }catch(...){
            ASSERT_FALSE(true, "(Correct Values) fromJson Function failed ");
        }

        //Wrong values
        try{
            e->fromJson(QJsonObject());
            ASSERT_TRUE(true, "(wrong Values) fromJson Function not crashed ");
        }catch(...){
            ASSERT_FALSE(true, "(wrong Values) fromJson Function crashed");
        }


        delete e;
        delete p;

    }
    delete h;
}


void platform_test(){
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "      PLATFORM CUSTOM UNIT TESTS        " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_PLatform_initialization();
    test_Platform_component_function();
    test_platform_OthersFunction();


    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
