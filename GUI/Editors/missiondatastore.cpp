/* ========================================================================= */
/* File: missiondatastore.cpp                                                */
/* Purpose: Static member definitions for MissionDataStore                   */
/* ========================================================================= */
#include "missiondatastore.h"

// Static members ki definition (required for linker)
QJsonObject MissionDataStore::missionData;
QString     MissionDataStore::missionFilePath;
