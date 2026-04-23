// =============================================================================
// FILE:        missioncreator.h
// MODULE:      Mission Creation Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the MissionCreator class, which handles the creation,
//              deletion, file-based saving, and loading of missions. Maintains
//              maps of Folder and Mission objects representing mission structure.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef MISSIONCREATOR_H
#define MISSIONCREATOR_H

#include <QObject>
#include <core/Hierarchy/folder.h>
#include <core/Hierarchy/Components/mission.h>

// =============================================================================
// CLASS: MissionCreator
//
// DESCRIPTION: Provides mission lifecycle management: creation, deletion,
//              file serialisation, and deserialisation. Stores missions in
//              unordered maps keyed by string identifiers.
// =============================================================================
class MissionCreator: public QObject
{
    Q_OBJECT
public:
    MissionCreator();

    // =========================================================================
    // SECTION: Mission Data Containers
    // DESCRIPTION: Maps holding folder and mission objects.
    // =========================================================================
    std::unordered_map<std::string, Folder> Folders;      //!< Folders containing mission elements
    std::unordered_map<std::string, Mission> missionList; //!< List of missions by ID

    void createMission();           //!< Creates a new mission
    void deleteMission();           //!< Deletes an existing mission
    void saveMissionAsFile();       //!< Saves current mission to a file
    void addMissionFromFile();      //!< Loads a mission from a file
};

#endif // MISSIONCREATOR_H
