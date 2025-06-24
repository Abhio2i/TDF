

#include "trajectory.h"
#include "core/Debug/console.h"
#include <QJsonArray>
#include <iostream>

Trajectory::Trajectory() {
    Waypoints* waypoin = new Waypoints();
    waypoin->position = new Vector(0, 0, 0);
    Trajectories.push_back(waypoin);
    Waypoints* waypoin2 = new Waypoints();
    waypoin2->position = new Vector(20, 30, 0);
    Trajectories.push_back(waypoin2);
    Waypoints* waypoin3 = new Waypoints();
    waypoin3->position = new Vector(40, 60, 0);
    Trajectories.push_back(waypoin3);
    Waypoints* waypoin4 = new Waypoints();
    waypoin4->position = new Vector(80, 20, 0);
    Trajectories.push_back(waypoin4);
    Waypoints* waypoin5 = new Waypoints();
    waypoin5->position = new Vector(100, 40, 0);
    Trajectories.push_back(waypoin5);
    Waypoints* waypoin6 = new Waypoints();
    waypoin6->position = new Vector(120, 0, 0);
    Trajectories.push_back(waypoin6);
}

QJsonObject Trajectory::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;

    QJsonArray strArray;
    for (const QJsonObject& s : array) {
        strArray.append(s);
    }
    obj["array"] = strArray;

    QJsonArray trajArray;
    for (const Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            trajArray.append(waypoint->toJson());
        }
    }
    obj["trajectories"] = trajArray;

    Console::log(obj);
    return obj;
}

void Trajectory::fromJson(const QJsonObject& obj) {
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();

    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("array") && obj["array"].isArray()) {
        array.clear();
        QJsonArray strArray = obj["array"].toArray();
        for (const QJsonValue& val : strArray) {
            if (val.isObject()) {
                array.push_back(val.toObject());
            }
        }
    }

    if (obj.contains("trajectories") && obj["trajectories"].isArray()) {
        for (Waypoints* wp : Trajectories) {
            delete wp->position;
            delete wp;
        }
        Trajectories.clear();

        QJsonArray trajArray = obj["trajectories"].toArray();
        for (const QJsonValue& wpVal : trajArray) {
            if (wpVal.isObject()) {
                Waypoints* wp = new Waypoints();
                wp->fromJson(wpVal.toObject());
                Trajectories.push_back(wp);
            }
        }
    }
}

bool Trajectory::removeTrajectory(size_t index) {
    if (index >= Trajectories.size()) {
        Console::log("Error: Trajectory index out of bounds");
        return false;
    }
    delete Trajectories[index]->position;
    delete Trajectories[index];
    Trajectories.erase(Trajectories.begin() + index);
    return true;
}

void Trajectory::addTrajectory(Waypoints* waypoint) {
    if (waypoint) {
        Trajectories.push_back(waypoint);
        std::cout << "Trajectory::addTrajectory this=" << this << std::endl;
        std::cout << "Trajectories size=" << Trajectories.size() << std::endl;
        std::cout << &Trajectories << ", capacity: " << Trajectories.capacity() << std::endl;
    } else {
        Console::error("Attempted to add null waypoint to trajectory");
    }
}
