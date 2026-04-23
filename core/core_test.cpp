#include "core_test.h"
#include <iostream>
#include <ostream>
extern void hierarchy_test();
extern void profileCategory_test();
extern void folder_test();

extern void platform_test();
extern void Specialzone_test();
extern void FixedPoints_test();

extern void Collider_test();
extern void CrossSection_test();
extern void Mesh_test();
extern void IFFProfile_test();
extern void RadioProfile_test();
extern void SensorProfile_test();
extern void MeshRenderer2D_test();
extern void Trajectory_test();
extern void WeaponProfile_test();
extern void DynamicModel_test();

extern void vector_test();
extern void formationPosition_test();
extern void parameter_test();
extern void waypoints_test();
extern void geocords_test();
extern void entityUtils_test();
extern void Transform_test();

extern void scenarioConfig_test();
extern void console_test();
extern void frame_test();

extern void aesaRadarBridge_test();
extern void radarAntenna_test();
extern void radarModel_test();
extern void radarScheduler_test();
extern void radarSignalLibrary_test();
extern void radarSignalProcessor_test();
extern void radarTracker_test();

extern int testsPassed;
extern int testsFailed;
extern int tests;
Core_Test::Core_Test() {

    std::cout << "CoreTest Test Start" << std::endl;

    int totaltestsPassed = 0;
    int totaltestsFailed = 0;
    int totaltests = 0;
    //Test Hierarchy
    hierarchy_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    profileCategory_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    folder_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    platform_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    Specialzone_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    FixedPoints_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    Collider_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    CrossSection_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    Mesh_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    IFFProfile_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    RadioProfile_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    SensorProfile_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    MeshRenderer2D_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    Transform_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    Trajectory_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    WeaponProfile_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    DynamicModel_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    vector_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    formationPosition_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    parameter_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    waypoints_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    geocords_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    entityUtils_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    scenarioConfig_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    console_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    frame_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    aesaRadarBridge_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    radarAntenna_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    radarModel_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    radarScheduler_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    radarSignalLibrary_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    radarSignalProcessor_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;

    radarTracker_test();
    totaltestsPassed += testsPassed;
    totaltestsFailed += testsFailed;
    totaltests += tests;


    std::cout << "\n=========================================" << std::endl;
    std::cout << "FINAL TEST SUMMARY:" << std::endl;
    std::cout << "Total Passed: " << totaltestsPassed << std::endl;
    std::cout << "Total Failed: " << totaltestsFailed << std::endl;
    std::cout << "Total Tests: " << totaltests << std::endl;
    std::cout << "Percent: " << ((totaltestsPassed*1.0f)/(totaltests*1.0f))*100.0f << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "CoreTest Test Completed" << std::endl;
}

