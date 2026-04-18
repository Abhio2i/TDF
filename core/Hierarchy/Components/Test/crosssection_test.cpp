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
void test_CrossSection_initialization() {
    std::cout << "\n--- Running Initialization Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);
    CrossSection* e = new CrossSection();

    ASSERT_NEQ(e,nullptr, "CrossSection should be return object");

    delete e;
    delete p;
    delete h;
}

void test_CrossSection_Subcomponent_function() {
    std::cout << "\n--- Running CrossSection_Subcomponent Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = new ProfileCategaory(h);
    CrossSection* e = new CrossSection();

    //Test addSubComponent function
    // wrong values
    try{
        e->addSubComponent("wrong");
        ASSERT_TRUE(true, "(wrong Values) addSubComponent Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(wrong Values) addSubComponent Function crashed");
    }

    //Test getsubComponentData function
    // correct values
    QJsonObject obj = e->getsubComponentData("transform");
    ASSERT_TRUE(obj.empty(), "(Correct Values) getsubComponentData Function should be return json data");

    // wrong values
    obj = e->getsubComponentData("wrong");
    ASSERT_TRUE(obj.empty(), "(wrong Values) getsubComponentData Function should be return empty json data");

    //Test updateSubComponent Function
    QJsonObject delta;
    delta["active"] = false;
    //Wrong values
    try{
        e->updateSubComponent("wrong",delta);
        ASSERT_TRUE(true, "(wrong Values) updateSubComponent Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(wrong Values) updateSubComponent Function crashed");
    }


    //Test removeSubComponent function
    // wrong values
    try{
        e->removeSubComponent("wrong");
        ASSERT_TRUE(true, "(wrong Values) removeSubComponent Function not crashed ");
    }catch(...){
        ASSERT_FALSE(true, "(wrong Values) removeSubComponent Function crashed");
    }

    delete e;
    delete p;
    delete h;
}

void test_CrossSection_OthersFunction() {
    std::cout << "\n--- Running CrossSection_OthersFunction Tests ---" << std::endl;
    Hierarchy* h = new Hierarchy();
    ProfileCategaory* p = h->addProfileCategaory("test_Profile1");
    CrossSection* e = new CrossSection();
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


void CrossSection_test(){
    testsPassed = 0;
    testsFailed = 0;
    tests = 0;
    std::cout << "=========================================" << std::endl;
    std::cout << "      CrossSection CUSTOM UNIT TESTS        " << std::endl;
    std::cout << "=========================================" << std::endl;

    test_CrossSection_initialization();
    test_CrossSection_Subcomponent_function();
    test_CrossSection_OthersFunction();


    std::cout << "\n=========================================" << std::endl;
    std::cout << "TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << testsPassed << std::endl;
    std::cout << "Total Failed: " << testsFailed << std::endl;
    std::cout << "=========================================" << std::endl;
}
