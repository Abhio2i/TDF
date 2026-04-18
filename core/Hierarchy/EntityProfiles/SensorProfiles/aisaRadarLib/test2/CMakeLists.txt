cmake_minimum_required(VERSION 3.16)
project(aesa_radar_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Core)

find_package(GTest REQUIRED)

set(RADAR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_library(aesa_radar_lib STATIC
    ${RADAR_DIR}/radarantenna_aesa.cpp
    ${RADAR_DIR}/radarsignalprocessor_aesa.cpp
    ${RADAR_DIR}/radarscheduler.cpp
    ${RADAR_DIR}/radartracker_aesa.cpp
    ${RADAR_DIR}/radarsignallibrary_aesa.cpp
    ${RADAR_DIR}/radarmodel_aesa.cpp
)
target_include_directories(aesa_radar_lib PUBLIC ${RADAR_DIR})
target_link_libraries(aesa_radar_lib PUBLIC Qt5::Core)


enable_testing()

function(add_radar_test TEST_NAME SOURCE_FILE)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE aesa_radar_lib GTest::GTest GTest::Main Qt5::Core)

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_radar_test(test_radarantenna         test_radarantenna.cpp)
add_radar_test(test_radarsignalprocessor test_radarsignalprocessor.cpp)
add_radar_test(test_radarscheduler       test_radarscheduler.cpp)
add_radar_test(test_radartracker         test_radartracker.cpp)
add_radar_test(test_radarsignallibrary   test_radarsignallibrary.cpp)
add_radar_test(test_radarmodel           test_radarmodel.cpp)
