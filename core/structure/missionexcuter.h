// =============================================================================
// FILE:        missionexcuter.h
// MODULE:      Mission Execution
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the MissionExcuter class, which manages the execution
//              of a mission assigned to an entity. Tracks mission state
//              (Start, Running, Pause, Done) and provides methods to start,
//              pause, resume, complete, and execute tasks. Supports JSON
//              serialisation for persistence.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef MISSIONEXCUTER_H
#define MISSIONEXCUTER_H

#include <QObject>
#include <core/Hierarchy/entity.h>
// #include <core/Hierarchy/Struct/task.h>

// =============================================================================
// CLASS: MissionExcuter
//
// DESCRIPTION: Controls the lifecycle of a mission for a specific entity.
//              Maintains execution flags, a reference to the target entity,
//              and a mission table identifier. Provides core mission control
//              operations and JSON serialisation.
// =============================================================================
class MissionExcuter: public QObject
{
    Q_OBJECT
public:
    MissionExcuter();

    // =========================================================================
    // SECTION: Mission State Flags
    // DESCRIPTION: Current execution status of the mission.
    // =========================================================================
    bool Active;        //!< Whether the mission executor is active
    std::string ID;     //!< Unique identifier for this mission executor instance
    std::string Name;   //!< Human-readable mission name
    bool Start;         //!< Flag indicating mission has been started
    bool Running;       //!< Flag indicating mission is currently running
    bool Pause;         //!< Flag indicating mission is paused
    bool Done;          //!< Flag indicating mission is completed

    // =========================================================================
    // SECTION: Target Entity & Mission Data
    // DESCRIPTION: Entity that will execute the mission and associated table.
    // =========================================================================
    Entity *entity;                 //!< Entity to which this mission is assigned
    std::string missionTable;       //!< Identifier for the mission definition/table
    // std::unordered_map<std::string, Task> *taskGroup; //!< Task group (commented out)

    // Mission control methods
    void start();       //!< Starts the mission execution
    void pause();       //!< Pauses the mission
    void resume();      //!< Resumes a paused mission
    void complete();    //!< Marks the mission as complete
    void excuteTask();  //!< Executes the current task (note: typo in name)

    // Serialization
    void toJson();      //!< Serialises mission executor state to JSON
    void fromJson();    //!< Deserialises mission executor state from JSON
};

#endif // MISSIONEXCUTER_H
