 #include "trajectory.h"
#include "core/Debug/console.h"
#include <QJsonArray>


Trajectory::Trajectory() {
    // array.push_back("Ram");
    // array.push_back("Sita");
    // array.push_back("Lakshman");
    // array.push_back("Bharat");
    // array.push_back("Sukhdev");


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


    QJsonArray outerArray;

    for (const auto& innerVec : Trajectories) {
        QJsonArray innerArray;
        for (const auto& waypoint : innerVec) {
            innerArray.append(waypoint->toJson());
        }
        outerArray.append(innerArray);
    }

    obj["trajectories"] = outerArray;
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
        Trajectories.clear();

        QJsonArray outerArray = obj["trajectories"].toArray();
        for (const QJsonValue& innerVal : outerArray) {
            if (!innerVal.isArray()) continue;

            QJsonArray innerArray = innerVal.toArray();
            std::vector<Waypoints*> innerVec;

            for (const QJsonValue& wpVal : innerArray) {
                if (wpVal.isObject()) {
                    Waypoints* wp = new Waypoints();
                    wp->fromJson(wpVal.toObject());
                    innerVec.push_back(wp);
                }
            }

            Trajectories.push_back(innerVec);
        }
    }
}
