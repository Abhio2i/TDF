

#include "trajectory.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qjsondocument.h"
#include <QJsonArray>
#include <QDebug>
#include <QCollator> // Header include karein

Trajectory::Trajectory():Component(nullptr) {
    Active = true; // Initialize Active
    FollowPath = true;
    current = 0;
    customParameters = QJsonObject(); // Initialize customParameters
}

void Trajectory::addSubComponent(std::string name, QString data1, QString data2, QString data3){

}

void Trajectory::removeSubComponent(std::string ID){

}

void Trajectory::updateSubComponent(std::string ID, const QJsonObject& obj){

}

QJsonObject Trajectory::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

void Trajectory::goHome(){
    reverse = true;
    if(current>0){
        current -=1;
    }
}

void Trajectory::activateSensors(){
    for (Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            waypoint->sensor = true;
        }
    }
}

void Trajectory::deactivateSensors(){
    for (Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            waypoint->sensor = false;
        }
    }
}

void Trajectory::makeFormation(){
    for (Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            waypoint->formation = true;
        }
    }
}

void Trajectory::deformation(){
    for (Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            waypoint->formation = false;;
        }
    }
}

QJsonObject Trajectory::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    obj["type"] = "component";
    obj["currentWaypoint"] = current;

    // QJsonArray strArray;
    // for (const QJsonObject& s : array) {
    //     strArray.append(s);
    // }
    // obj["array"] = strArray;

    // QJsonArray trajArray;
    int i = 0;
    for (const Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            i++;
            QJsonObject waypointObj;
            waypointObj["type"] = "Section";
            waypointObj["Latitude"] = toParm(waypoint->position->x,"deg");
            waypointObj["Longitude"] = toParm(waypoint->position->z,"deg");
            waypointObj["Altitude"] = toParm(waypoint->position->y,"ft");
            waypointObj["Speed"] = toParm(waypoint->speed,"km/h");
            waypointObj["ActivateSensor"] = waypoint->sensor;
            waypointObj["MakeForamation"] = waypoint->formation;
            obj["waypoint_"+QString::number(i)] = waypointObj;
            // trajArray.append(waypoint->toJson());
        }
    }
    // obj["trajectories"] = trajArray;

    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    ////qDebug() << "Trajectory::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}



void Trajectory::fromJson(const QJsonObject& obj) {
    ////qDebug() << "Trajectory::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // Standard fields
    // if (obj.contains("id"))
    //     ID = obj["id"].toString().toStdString();
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if(obj.contains("currentWaypoint"))
        current = obj["currentWaypoint"].toInt();
    if (obj.contains("waypoint_1")) {
        // for (Waypoints* wp : Trajectories) {
        //     delete wp->position;
        //     delete wp;
        // }
        // 2. Saari keys nikal kar filter karte hain
        QStringList waypointKeys;
        QList<int> numbers = {};

        // Sorting in Ascending Order (Chote se bada)

        QStringList allKeys = obj.keys();

        for (const QString &key : allKeys) {
            // Check karein ki key me "waypoint" hai ya nahi
            if (key.contains("waypoint")) {
                waypointKeys.append(key);
                bool ok1 = false;
                int n1 = key.mid(9).toInt(&ok1);
                if(ok1){
                    numbers.append(n1);
                }
            }
        }
        std::sort(numbers.begin(), numbers.end());

        // 3. Result print karein
        //qDebug() << "Filtered and Sorted Keys:";
        for (const int i : numbers) {
            QString key = "waypoint_"+QString::number(i);
            if (obj.contains(key) && obj[key].isObject()) {
                QJsonObject waypointObj = obj[key].toObject();
                Waypoints* wp = new Waypoints();
                int n1 = i;
                if( n1<=Trajectories.size()){
                    wp = Trajectories[n1-1];
                }else{
                    Trajectories.push_back(wp);
                }
                if (waypointObj.contains("Latitude") && waypointObj["Latitude"].isObject())
                    wp->position->x = valueFromParm(waypointObj["Latitude"].toObject());
                if (waypointObj.contains("Longitude") && waypointObj["Longitude"].isObject())
                    wp->position->z = valueFromParm(waypointObj["Longitude"].toObject());
                if (waypointObj.contains("Altitude") && waypointObj["Altitude"].isObject())
                    wp->position->y = valueFromParm(waypointObj["Altitude"].toObject());
                if (waypointObj.contains("Speed") && waypointObj["Speed"].isObject())
                    wp->speed = valueFromParm(waypointObj["Speed"].toObject());
                if (waypointObj.contains("ActivateSensor") && waypointObj["ActivateSensor"].isBool())
                    wp->sensor = waypointObj["ActivateSensor"].toBool();
                if (waypointObj.contains("MakeForamation") && waypointObj["MakeForamation"].isBool())
                    wp->formation = waypointObj["MakeForamation"].toBool();

            }
        }
    }

    // // Custom parameters
    // QStringList standardKeys = {"id", "active", "array", "trajectories"};
    // for (auto it = obj.begin(); it != obj.end(); ++it) {
    //     if (!standardKeys.contains(it.key())) {
    //         customParameters[it.key()] = it.value();
    //     }
    // }

    ////qDebug() << "Trajectory::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
}
// QJsonObject Trajectory::toJson() const {
//     QJsonObject obj;
//     obj["id"] = QString::fromStdString(ID);
//     obj["active"] = Active;
//     obj["type"] = "component";
//     obj["currentWaypoint"] = current;

//     // ✅ CRITICAL FIX: Save trajectories as proper JSON array
//     QJsonArray trajArray;
//     for (const Waypoints* waypoint : Trajectories) {
//         if (waypoint && waypoint->position) {
//             QJsonObject waypointObj;

//             // Position as nested object
//             QJsonObject posObj;
//             posObj["type"] = "vector";
//             posObj["x"] = waypoint->position->x;
//             posObj["y"] = waypoint->position->y;
//             posObj["z"] = waypoint->position->z;
//             waypointObj["position"] = posObj;

//             // Other waypoint properties
//             waypointObj["speed"] = waypoint->speed;
//             waypointObj["sensor"] = waypoint->sensor;
//             waypointObj["formation"] = waypoint->formation;

//             trajArray.append(waypointObj);
//         }
//     }
//     obj["trajectories"] = trajArray;

//     // ✅ ALSO KEEP: Individual waypoint sections for backward compatibility
//     for (int i = 0; i < Trajectories.size(); i++) {
//         const Waypoints* waypoint = Trajectories[i];
//         if (waypoint && waypoint->position) {
//             QJsonObject waypointObj;
//             waypointObj["type"] = "Section";
//             waypointObj["Latitude"] = toParm(waypoint->position->x, "deg");
//             waypointObj["Longitude"] = toParm(waypoint->position->z, "deg");
//             waypointObj["Altitude"] = toParm(waypoint->position->y, "ft");
//             waypointObj["Speed"] = toParm(waypoint->speed, "km/h");
//             waypointObj["ActivateSensor"] = waypoint->sensor;
//            waypointObj["MakeForamation"] = waypoint->formation;
//             obj["waypoint_" + QString::number(i + 1)] = waypointObj;
//         }
//     }

//     // Add custom parameters
//     for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
//         obj[it.key()] = it.value();
//     }

//     qDebug() << "Trajectory::toJson - Saved" << Trajectories.size() << "waypoints";
//     return obj;
// }

// void Trajectory::fromJson(const QJsonObject& obj) {
//     // Standard fields
//     if (obj.contains("active"))
//         Active = obj["active"].toBool();
//     if (obj.contains("currentWaypoint"))
//         current = obj["currentWaypoint"].toInt();

//     // ✅ CRITICAL FIX: Clear existing waypoints first
//     // for (Waypoints* wp : Trajectories) {
//     //     if (wp) {
//     //         delete wp->position;
//     //         delete wp;
//     //     }
//     // }
//     // Trajectories.clear();

//     // ✅ PRIORITY 1: Load from "trajectories" array (new format)
//     if (obj.contains("trajectories") && obj["trajectories"].isArray()) {
//         QJsonArray trajArray = obj["trajectories"].toArray();
//         for (const QJsonValue& val : trajArray) {
//             if (val.isObject()) {
//                 QJsonObject waypointObj = val.toObject();
//                 Waypoints* wp = new Waypoints();

//                 // Load position
//                 if (waypointObj.contains("position") && waypointObj["position"].isObject()) {
//                     QJsonObject posObj = waypointObj["position"].toObject();
//                     wp->position->x = posObj["x"].toDouble();
//                     wp->position->y = posObj["y"].toDouble();
//                     wp->position->z = posObj["z"].toDouble();
//                 }

//                 // ✅ FIX: Qt5 compatible default value handling
//                 wp->speed = waypointObj.contains("speed") ? waypointObj["speed"].toDouble() : 800.0;
//                 wp->sensor = waypointObj.contains("sensor") ? waypointObj["sensor"].toBool() : false;
//                 if (waypointObj.contains("MakeForamation") && waypointObj["MakeForamation"].isBool())
//                     wp->formation = waypointObj["MakeForamation"].toBool();

//                 Trajectories.push_back(wp);
//             }
//         }
//         qDebug() << "Trajectory::fromJson - Loaded" << Trajectories.size() << "waypoints from 'trajectories' array";
//     }
//     // ✅ FALLBACK: Load from individual waypoint sections (old format)
//     else if (obj.contains("waypoint_1")) {
//         QStringList waypointKeys;
//         QStringList allKeys = obj.keys();

//         for (const QString &key : allKeys) {
//             if (key.startsWith("waypoint_")) {
//                 waypointKeys.append(key);
//             }
//         }

//         // Sort keys numerically
//         std::sort(waypointKeys.begin(), waypointKeys.end(),
//                   [](const QString &a, const QString &b) {
//                       bool ok1, ok2;
//                       int n1 = a.mid(9).toInt(&ok1);
//                       int n2 = b.mid(9).toInt(&ok2);
//                       if (ok1 && ok2) return n1 < n2;
//                       return a < b;
//                   });

//         for (const QString &key : waypointKeys) {
//             if (obj[key].isObject()) {
//                 QJsonObject waypointObj = obj[key].toObject();
//                 Waypoints* wp = new Waypoints();

//                 if (waypointObj.contains("Latitude") && waypointObj["Latitude"].isObject())
//                     wp->position->x = valueFromParm(waypointObj["Latitude"].toObject());
//                 if (waypointObj.contains("Longitude") && waypointObj["Longitude"].isObject())
//                     wp->position->z = valueFromParm(waypointObj["Longitude"].toObject());
//                 if (waypointObj.contains("Altitude") && waypointObj["Altitude"].isObject())
//                     wp->position->y = valueFromParm(waypointObj["Altitude"].toObject());
//                 if (waypointObj.contains("Speed") && waypointObj["Speed"].isObject())
//                     wp->speed = valueFromParm(waypointObj["Speed"].toObject());
//                 wp->sensor = waypointObj.contains("ActivateSensor") ? waypointObj["ActivateSensor"].toBool() : false;
//                 wp->formation = waypointObj.contains("MakeFormation") ? waypointObj["MakeFormation"].toBool() : false;

//                 Trajectories.push_back(wp);
//             }
//         }
//         // qDebug() << "Trajectory::fromJson - Loaded" << Trajectories.size() << "waypoints from individual sections";
//     }
// }

void Trajectory::addWaypoint(float x,float y, float z){
    Waypoints* newWaypoint = new Waypoints();
    newWaypoint->position = new Vector(x, y, z);
    addTrajectory(newWaypoint);
}
void Trajectory::addWaypoint(float x, float y, float z, bool activateSensor) {
    Waypoints* wp = new Waypoints();
    wp->position = new Vector(x, y, z);
    wp->sensor = activateSensor;   // set the flag
    addTrajectory(wp);
}
bool Trajectory::removeTrajectory(size_t index) {
    if (index >= Trajectories.size()) {
        //qDebug() << "Error: Trajectory index out of bounds";
        return false;
    }
    delete Trajectories[index]->position;
    delete Trajectories[index];
    Trajectories.erase(Trajectories.begin() + index);
    return true;
}

Waypoints* Trajectory::getCurrentWaypoint(){
    if((current-1)<Trajectories.size()){
        int c = (current-1)<0?0:(current-1);
        return Trajectories[c];
    }
    return nullptr;
}

Waypoints* Trajectory::getTargetWaypoint(){
    if((current)<Trajectories.size()){
        return Trajectories[current];
    }
    return nullptr;
}

void Trajectory::addTrajectory(Waypoints* waypoint) {
    if (waypoint) {
        Trajectories.push_back(waypoint);
        //qDebug() << "Trajectory::addTrajectory this=" << this;
        //qDebug() << "Trajectories size=" << Trajectories.size();
        //qDebug() << "Trajectories address=" << &Trajectories << ", capacity=" << Trajectories.capacity();
    } else {
        //qDebug() << "Error: Attempted to add null waypoint to trajectory";
    }
}

