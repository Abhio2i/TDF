cmake_minimum_required(VERSION 3.16)

project(Test1 LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if ($ENV{GOOGLETEST_DIR})
    get_filename_component(_GTEST_BASE $ENV{GOOGLETEST_DIR} REALPATH)
elseif (EXISTS "") # set by QC
    get_filename_component(_GTEST_BASE "" REALPATH) # set by QC
endif()

if (NOT GTEST_ROOT)
    if (_GTEST_BASE)
        message("Setting GTEST_ROOT to ${_GTEST_BASE}")
        set(GTEST_ROOT ${_GTEST_BASE})
    else()
        message("No GTEST_ROOT specified - using system defaults.")
    endif()
endif()

find_package(GTest REQUIRED)
if (NOT GTest_FOUND)
    message (FATAL_ERROR "No GTest Found")
endif()

# --- Source Files ---
# Ensure these paths are correct relative to THIS CMakeLists.txt file
set(PROJECT_SOURCES
    ../EOSensorLib/eosensorlib.cpp
    ../IRSensorLib/irsensorlib.cpp
    ../EOIRCommonLib/eoircommonlib.cpp
)
# --- Include Directories ---
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../EOSensorLib
    ${CMAKE_CURRENT_SOURCE_DIR}/../IRSensorLib
    ${CMAKE_CURRENT_SOURCE_DIR}/../EOIRCommonLib
)

add_executable(Test1
    main.cpp
    ${PROJECT_SOURCES}
    tst_test1.cpp
    tst_detection_range.cpp
)
add_test(NAME Test1 COMMAND Test1)

target_link_libraries(Test1 PRIVATE GTest::GTest)
if (GMock_FOUND)
    target_link_libraries(Test1 INTERFACE GTest::GMock)
endif()



