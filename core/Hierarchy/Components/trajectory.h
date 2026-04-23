// =============================================================================
// FILE:        trajectory.h
// MODULE:      Trajectory Following
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Trajectory class, which manages waypoint-based
//              path following for simulation entities. Supports forward/reverse
//              traversal, sensor activation at waypoints, formation changes,
//              and JSON serialization for persistence.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added custom parameters and enhanced waypoint methods.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include "./component.h"
#include <QObject>
#include <QJsonObject>
#include <core/Hierarchy/Struct/waypoints.h>

// =============================================================================
// CLASS: Trajectory
//
// DESCRIPTION: Component that defines a pre-planned path composed of waypoints.
//              Provides methods to navigate the path, activate/deactivate
//              sensors at waypoints, manage formation changes, and retrieve
//              current/target waypoints.
// =============================================================================
class Trajectory: public QObject, public Component
{
    Q_OBJECT
public:
    Trajectory();
    ComponentType Typo() const override { return ComponentType::Trajectory; }

    // =========================================================================
    // SECTION: Trajectory State
    // DESCRIPTION: Flags and navigation controls for path following.
    // =========================================================================
    bool Active;                    //!< Whether trajectory following is active
    bool FollowPath;                //!< Whether to actively follow the path
    // std::vector<QJsonObject> array; //!< (Legacy) Not used
    std::vector<Waypoints*> Trajectories; //!< List of waypoints defining the path
    int current = 0;                //!< Index of current waypoint
    bool reverse = false;           //!< Traverse path in reverse order

    // =========================================================================
    // SECTION: Behaviour Methods
    // DESCRIPTION: Actions triggered at waypoints or by mission logic.
    // =========================================================================
    void goHome();                  //!< Return to starting/home waypoint
    void activateSensors();         //!< Activate sensors at current waypoint
    void deactivateSensors();       //!< Deactivate sensors at current waypoint
    void makeFormation();           //!< Form up with formation leader
    void deformation();             //!!< Break formation

    // =========================================================================
    // SECTION: Custom Parameters
    // DESCRIPTION: Extensible key-value store for user-defined trajectory data.
    // =========================================================================
    QJsonObject AdditionalParameters;    //!< Custom key-value parameters

    // Component interface overrides
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    // =========================================================================
    // SECTION: Waypoint Management
    // DESCRIPTION: Operations for querying and modifying the waypoint list.
    // =========================================================================
    Waypoints* getCurrentWaypoint();                        //!< Returns current waypoint
    Waypoints* getTargetWaypoint();                         //!< Returns next waypoint (target)
    bool removeTrajectory(size_t index);                    //!< Removes waypoint at index
    void addTrajectory(Waypoints* waypoint);                //!< Adds waypoint to end
    void addWaypoint(float x, float y, float z);            //!< Creates and adds simple waypoint
    void addWaypoint(float x, float y, float z, bool activateSensor);  //!< Creates waypoint with sensor activation flag
};

#endif // TRAJECTORY_H
