
// #ifndef TRAJECTORY_H
// #define TRAJECTORY_H
// #include "./component.h"
// #include <QObject>
// #include <QJsonObject>
// #include <core/Hierarchy/Struct/waypoints.h>

// class Trajectory: public QObject, public Component
// {
//     Q_OBJECT
// public:
//     Trajectory();
//     ComponentType Typo() const override { return ComponentType::Trajectory; }
//     bool Active;
//     bool FollowPath;
//     // std::vector<QJsonObject> array;
//     std::vector<Waypoints*> Trajectories;
//     int current = 0;
//     QJsonObject customParameters; // Added to store custom parameters
//     void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QString data3 = "") override;
//     void removeSubComponent(std::string ID) override;
//     QJsonObject getsubComponentData(std::string ID) const override;
//     void updateSubComponent(std::string ID, const QJsonObject& obj) override;
//     QJsonObject toJson() const override;
//     void fromJson(const QJsonObject &obj) override;

//     // Function declarations
//     Waypoints* getCurrentWaypoint();
//     Waypoints* getTargetWaypoint();
//     bool removeTrajectory(size_t index);
//     void addTrajectory(Waypoints* waypoint);
//     void addWaypoint(float x,float y, float z);
// };

// #endif // TRAJECTORY_H

#ifndef TRAJECTORY_H
#define TRAJECTORY_H
#include "./component.h"
#include <QObject>
#include <QJsonObject>
#include <core/Hierarchy/Struct/waypoints.h>

class Trajectory: public QObject, public Component
{
    Q_OBJECT
public:
    Trajectory();
    ComponentType Typo() const override { return ComponentType::Trajectory; }
    bool Active;
    bool FollowPath;
    // std::vector<QJsonObject> array;
    std::vector<Waypoints*> Trajectories;
    int current = 0;
    bool reverse = false;
    void goHome();
    void activateSensors();
    void deactivateSensors();
    void makeFormation();
    void deformation();
    QJsonObject customParameters; // Added to store custom parameters
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QString data3 = "") override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    // Function declarations
    Waypoints* getCurrentWaypoint();
    Waypoints* getTargetWaypoint();
    bool removeTrajectory(size_t index);
    void addTrajectory(Waypoints* waypoint);
    void addWaypoint(float x,float y, float z);
    void addWaypoint(float x, float y, float z, bool activateSensor);  // NEW
};

#endif // TRAJECTORY_H
