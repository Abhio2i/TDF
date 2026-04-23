// =============================================================================
// FILE:        action.h
// MODULE:      Action System
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Action class, which represents a discrete action
//              that can be triggered in the simulation (e.g., fire weapon,
//              deploy countermeasure, change mode). Provides a signal for
//              action execution and JSON serialisation for persistence.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef ACTION_H
#define ACTION_H

#include <QObject>
#include "./constants.h"

// =============================================================================
// CLASS: Action
//
// DESCRIPTION: Represents a simulation action that can be triggered by
//              mission logic, user input, or script. Contains an action type
//              enum and emits a signal when the action occurs.
// =============================================================================
class Action: public QObject
{
    Q_OBJECT
public:
    Action();
    Constants::ActionType actionType;   //!< Type of action (fire, jam, etc.)

    void toJson();                      //!< Serialises action to JSON
    void fromJson();                    //!< Deserialises action from JSON

signals:
    void action();                      //!< Emitted when the action is triggered
};

#endif // ACTION_H
