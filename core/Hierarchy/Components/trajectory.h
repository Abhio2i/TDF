
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
    std::string ID;
    std::vector<QJsonObject> array;
    std::vector<Waypoints*> Trajectories;
    int current = 0;
    QJsonObject customParameters; // Added to store custom parameters

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    // Function declarations
    bool removeTrajectory(size_t index);
    void addTrajectory(Waypoints* waypoint);
    void addWaypoint(float x,float y, float z);
};

#endif // TRAJECTORY_H

