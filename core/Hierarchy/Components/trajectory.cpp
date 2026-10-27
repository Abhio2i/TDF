/**
 * @file trajectory.cpp
 * @brief Implementation of the Trajectory component for waypoint‑based path following.
 */

#include "trajectory.h"
#include "core/Hierarchy/ECS/Struct/transform_component.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/entity.h"
#include "core/Hierarchy/hierarchy.h"
#include <QJsonArray>
#include <QDebug>
#include <QCollator>

/**
 * @brief Constructs a Trajectory component with default values.
 */
Trajectory::Trajectory():Component() {
    Active = true; // Initialize Active
    FollowPath = true;
    current = 0;
}

//fix by yogesh
void Trajectory::init(Entity *Entity)
{
    if (!Entity || !Entity->root)
    {
        qWarning() << "[TRAJECTORY INIT] Invalid Entity/root";
        return;
    }

    parentEntity = Entity;

    auto& world = parentEntity->root->world;
    ECS::Entity ecsID = parentEntity->ecsID;

    qDebug()
        << "[TRAJECTORY INIT]"
        << "Entity =" << QString::fromStdString(parentEntity->ID)
        << "ECS ID =" << ecsID;

    // Add ECS trajectory component only if it does not already exist.
    if (!world.HasComponent<ECS::trajectoryPathComponent>(ecsID))
    {
        world.AddComponent<ECS::trajectoryPathComponent>(ecsID);

        qDebug()
            << "[TRAJECTORY INIT] ECS trajectory component CREATED";
    }
    else
    {
        qDebug()
        << "[TRAJECTORY INIT] ECS trajectory component already exists";
    }
}
void Trajectory::Awake(){}
void Trajectory::Start(){}



//fix by yogesh
ECS::trajectoryPathComponent& Trajectory::getTrajectoryPath()
{
    // ------------------------------------------------------------
    // Safety: parentEntity must already be bound.
    // ------------------------------------------------------------
    Q_ASSERT(parentEntity != nullptr);
    Q_ASSERT(parentEntity->root != nullptr);

    auto& world = parentEntity->root->world;
    ECS::Entity ecsID = parentEntity->ecsID;
    if (!world.HasComponent<ECS::trajectoryPathComponent>(ecsID))
    {
        qDebug()
        << "[TRAJECTORY] ECS component missing."
        << "Creating now for Entity ="
        << QString::fromStdString(parentEntity->ID);

        world.AddComponent<ECS::trajectoryPathComponent>(ecsID);
    }

    auto* component =
        world.GetComponent<ECS::trajectoryPathComponent>(ecsID);

    Q_ASSERT(component != nullptr);

    return *component;
}



/**
 * @brief Adds a sub‑component (unused for Trajectory).
 */
void Trajectory::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){
}

/**
 * @brief Removes a sub‑component (unused).
 */
void Trajectory::removeSubComponent(std::string ID){
}

/**
 * @brief Updates a sub‑component (unused).
 */
void Trajectory::updateSubComponent(std::string ID, const QJsonObject& obj){
}

/**
 * @brief Gets sub‑component data (unused).
 */
QJsonObject Trajectory::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

/**
 * @brief Reverses the trajectory direction and moves one waypoint back.
 */
void Trajectory::goHome(){
    reverse = true;
    if(current>0){
        current -=1;
        getTrajectoryPath().currentWaypoint = current;
    }
}

/**
 * @brief Activates sensors for all waypoints in the trajectory.
 */
void Trajectory::activateSensors(){
    for (Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            waypoint->sensor = true;
        }
    }
}

/**
 * @brief Deactivates sensors for all waypoints.
 */
void Trajectory::deactivateSensors(){
    for (Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            waypoint->sensor = false;
        }
    }
}

/**
 * @brief Enables formation mode for all waypoints.
 */
void Trajectory::makeFormation(){
    for (Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            waypoint->formation = true;
        }
    }
}

/**
 * @brief Disables formation mode for all waypoints.
 */
void Trajectory::deformation(){
    for (Waypoints* waypoint : Trajectories) {
        if (waypoint) {
            waypoint->formation = false;
        }
    }
}

/**
 * @brief Serializes the Trajectory component to JSON.
 * @return QJsonObject containing waypoint data and parameters.
 */
QJsonObject Trajectory::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    obj["type"] = "component";
    QJsonObject wayobj = toParm(current,"NUM");
    wayobj["editable"] = false;
    obj["currentWaypoint"] = wayobj;


    QJsonObject followruleObj;
    followruleObj["type"] = "option";

    QJsonArray followruleArray;
    for (const std::string& opt : followRuleNames)
        followruleArray.append(QString::fromStdString(opt));
    followruleObj["options"] = followruleArray;
    followruleObj["value"] = QString::fromStdString(followRuleNames[rule]);
    obj["Rule"] = followruleObj;

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
            waypointObj["dropWeapon"] = waypoint->dropWeapon;
            waypointObj["dropcount"] = toParm(waypoint->dropcount,"int");
            waypointObj["MakeFormation"] = waypoint->formation;
            obj["waypoint_"+QString::number(i)] = waypointObj;
        }
    }
    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    return obj;
}

/**
 * @brief Deserializes the Trajectory component from JSON.
 * @param obj JSON object containing waypoint data.
 */
void Trajectory::fromJson(const QJsonObject& obj) {
    if (!parentEntity || !parentEntity->root)
    {
        return;
    }
    // Standard fields
    if (obj.contains("active")){
        Active = obj["active"].toBool();
        if(parentEntity)
        getTrajectoryPath().active = Active;
    }
    if (obj.contains("currentWaypoint") && obj["currentWaypoint"].isObject()){
        current = valueFromParm(obj["currentWaypoint"].toObject());
        current = current>(Trajectories.size()-1)?(Trajectories.size()-1):current;

    }

    // if(obj.contains("currentWaypoint"))
    //     current = obj["currentWaypoint"].toInt();

    if (obj.contains("Rule") && obj["Rule"].isObject()) {
        QJsonObject followruleObj = obj["Rule"].toObject();
        if (followruleObj.contains("value")){
            for (int i = 0; i < 3; i++) {
                if (followRuleNames[i] == followruleObj["value"].toString().toStdString()) {
                    rule = (FollowRule)i;
                }
            }
            if(rule != FollowRule::Reverse){
                reverse = false;
            }
        }
    }

    if (obj.contains("waypoint_1")) {
        // Collect all waypoint keys
        QStringList waypointKeys;
        QList<int> numbers = {};

        QStringList allKeys = obj.keys();

        for (const QString &key : allKeys) {
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
        auto& path = getTrajectoryPath();
        for (const int i : numbers) {
            QString key = "waypoint_"+QString::number(i);
            if (obj.contains(key) && obj[key].isObject()) {
                QJsonObject waypointObj = obj[key].toObject();
                Waypoints* wp = new Waypoints();
                int n1 = i;
                if( n1 <= Trajectories.size()){
                    wp = Trajectories[n1-1];
                }else{
                    Trajectories.push_back(wp);
                    path.activeCount++;
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
                if (waypointObj.contains("dropWeapon") && waypointObj["dropWeapon"].isBool())
                    wp->dropWeapon = waypointObj["dropWeapon"].toBool();
                if (waypointObj.contains("dropcount")&& waypointObj["dropcount"].isObject() )
                    wp->dropcount = valueFromParm(waypointObj["dropcount"].toObject());
                if (waypointObj.contains("MakeFormation") && waypointObj["MakeFormation"].isBool())
                    wp->formation = waypointObj["MakeFormation"].toBool();
                path.path[n1-1].lat = wp->position->x;
                path.path[n1-1].lon = wp->position->z;
                path.delta = true;
            }
        }
    }

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }

    if(!statesaved){
        statesaved = true;
        InitalState["currentWaypoint"] = current;
    }
    getTrajectoryPath().currentWaypoint = current;
}
/**
 * @brief Adds a waypoint with given coordinates.
 * @param x Latitude (or X coordinate).
 * @param y Altitude (or Y coordinate).
 * @param z Longitude (or Z coordinate).
 */
void Trajectory::addWaypoint(float x,float y, float z){
    Waypoints* newWaypoint = new Waypoints();
    newWaypoint->position = new Vector(x, y, z);
    addTrajectory(newWaypoint);
}

/**
 * @brief Adds a waypoint with sensor activation flag.
 * @param x Latitude.
 * @param y Altitude.
 * @param z Longitude.
 * @param activateSensor Whether sensors should be active at this waypoint.
 */

void Trajectory::addWaypoint(float x, float y, float z, bool activateSensor) {
    Waypoints* wp = new Waypoints();
    wp->position = new Vector(x, y, z);
    wp->sensor = activateSensor;
    addTrajectory(wp);
}

/**
 * @brief Removes a waypoint at the given index.
 * @param index Zero‑based index of the waypoint to remove.
 * @return True if successful, false if index out of bounds.
 */
bool Trajectory::removeTrajectory(size_t index) {
    if (index >= Trajectories.size()) {
        return false;
    }
    delete Trajectories[index]->position;
    delete Trajectories[index];
    Trajectories.erase(Trajectories.begin() + index);
    getTrajectoryPath().delta = true;
    return true;
}

/**
 * @brief Returns the current waypoint (the one the entity is heading towards).
 * @return Pointer to the current Waypoints, or nullptr if none.
 */
Waypoints* Trajectory::getCurrentWaypoint(){
    if((current-1)<Trajectories.size()){
        int c = (current-1)<0?0:(current-1);
        return Trajectories[c];
    }
    return nullptr;
}

Waypoints* Trajectory::getWaypointByIndex(int current){
    if((current-1)<Trajectories.size()){
        int c = (current-1)<0?0:(current-1);
        return Trajectories[c];
    }
    return nullptr;
}

/**
 * @brief Returns the target waypoint (the next waypoint to reach).
 * @return Pointer to the target Waypoints, or nullptr if none.
 */
Waypoints* Trajectory::getTargetWaypoint(){
    if((current)<Trajectories.size()){
        return Trajectories[current];
    }else{
        current = Trajectories.size()-1;
        getTrajectoryPath().currentWaypoint = current;
        if(current >=0){
            return Trajectories[current];
        }
    }
    return nullptr;
}

/**
 * @brief Appends a waypoint to the trajectory list.
 * @param waypoint Pointer to the Waypoints object to add.
 */
// void Trajectory::addTrajectory(Waypoints* waypoint) {
//     if (waypoint) {
//         Trajectories.push_back(waypoint);
//         auto& path = getTrajectoryPath();
//         path.path[path.activeCount].lat = waypoint->position->x;
//         path.path[path.activeCount].lon = waypoint->position->z;
//         path.activeCount ++;
//         path.delta = true;
//     }
// }


//fix by yogesh
void Trajectory::addTrajectory(Waypoints* waypoint)
{
    if (!waypoint)
        return;

    // Keep the normal trajectory list updated.
    Trajectories.push_back(waypoint);

    // ECS path cannot be updated until Trajectory is attached
    // to a valid Entity.
    if (!parentEntity || !parentEntity->root)
    {
        qWarning()
        << "[TRAJECTORY] addTrajectory() called before init."
        << "Waypoint stored, ECS path update skipped.";

        return;
    }

    auto& path = getTrajectoryPath();

    // trajectoryPathComponent::path is a fixed C-style array.
    constexpr int PATH_CAPACITY =
        sizeof(path.path) / sizeof(path.path[0]);

    if (path.activeCount >= PATH_CAPACITY)
    {
        qWarning()
        << "[TRAJECTORY] ECS path capacity exceeded."
        << "activeCount =" << path.activeCount
        << "capacity =" << PATH_CAPACITY;

        return;
    }

    path.path[path.activeCount].lat =
        waypoint->position->x;

    path.path[path.activeCount].lon =
        waypoint->position->z;

    path.activeCount++;
    path.delta = true;
}

void Trajectory::SaveState(){
    if(!statesaved||true){
        statesaved = true;
        QJsonObject wayobj = toParm(current,"NUM");
        wayobj["editable"] = false;
        InitalState["currentWaypoint"] = wayobj;
    }
}

void Trajectory::RestoreState(){
    fromJson(InitalState);
}
