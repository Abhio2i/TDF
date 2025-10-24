
#include "trajectory.h"
#include "qjsondocument.h"
#include <QJsonArray>
#include <QDebug>

Trajectory::Trajectory() {
    Active = true; // Initialize Active
    current = 0;
    customParameters = QJsonObject(); // Initialize customParameters
}

QJsonObject Trajectory::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    obj["type"] = "component";

    // QJsonArray strArray;
    // for (const QJsonObject& s : array) {
    //     strArray.append(s);
    // }
    // obj["array"] = strArray;

    QJsonArray trajArray;
    for (const Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            trajArray.append(waypoint->toJson());
        }
    }
    obj["trajectories"] = trajArray;

    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    //qDebug() << "Trajectory::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}

void Trajectory::fromJson(const QJsonObject& obj) {
    //qDebug() << "Trajectory::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // Standard fields
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    // if (obj.contains("array") && obj["array"].isArray()) {
    //     array.clear();
    //     QJsonArray strArray = obj["array"].toArray();
    //     for (const QJsonValue& val : strArray) {
    //         if (val.isObject()) {
    //             array.push_back(val.toObject());
    //         }
    //     }
    // }
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

    // Custom parameters
    QStringList standardKeys = {"id", "active", "array", "trajectories"};
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!standardKeys.contains(it.key())) {
            customParameters[it.key()] = it.value();
        }
    }

    //qDebug() << "Trajectory::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
}

void Trajectory::addWaypoint(float x,float y, float z){
    Waypoints* newWaypoint = new Waypoints();
    newWaypoint->position = new Vector(x, y, z);
    addTrajectory(newWaypoint);
}

bool Trajectory::removeTrajectory(size_t index) {
    if (index >= Trajectories.size()) {
        qDebug() << "Error: Trajectory index out of bounds";
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
        qDebug() << "Trajectory::addTrajectory this=" << this;
        qDebug() << "Trajectories size=" << Trajectories.size();
        qDebug() << "Trajectories address=" << &Trajectories << ", capacity=" << Trajectories.capacity();
    } else {
        qDebug() << "Error: Attempted to add null waypoint to trajectory";
    }
}
