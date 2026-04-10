#include "core_test.h"
#include "qdebug.h"
#include "qglobal.h"
#include <iostream>
#include <ostream>
extern void hierarchy_test();

Core_Test::Core_Test() {

    std::cout << "CoreTest Test Start" << std::endl;

    //Test Hierarchy
    hierarchy_test();

    std::cout << "CoreTest Test Completed" << std::endl;
}

