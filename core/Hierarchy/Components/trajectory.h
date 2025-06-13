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
    std::vector<std::vector<Waypoints*>> Trajectories;

    QJsonObject toJson()const override;
    void fromJson(const QJsonObject &obj) override;

};

#endif // TRAJECTORY_H
