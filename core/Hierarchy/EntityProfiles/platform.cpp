#include "platform.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Hierarchy/EntityProfiles/weapons/bomb.h"  // for launchBombs()
#include "qelapsedtimer.h"
#include "qjsonarray.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/Struct/parameter.h"
#include "core/GlobalRegistry.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>
#include "core/Debug/profiler.h"
#include "GUI/Editors/runtimeeditor.h"
#include "core/Hierarchy/ECS/Struct/Meta_Data_Component.h"

Platform::Platform(Hierarchy* h) : Entity(h), collider(),sensors(),radios(),weapons(),iffs() {
    type = Constants::EntityType::Platform;
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "roll";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 1.0f;
    parameters["roll"] = par;
    m_hierarchy = h;
    taskgroup = new TaskGroup();
    taskgroup->hide();
    if(root){
        auto& data = root->world.AddComponent<ECS::Entity_Data>(ecsID);
        data.active = true;
        data.destroy = false;
    }
}

Platform::~Platform(){
    std::vector<std::string>supportedComponents =  Platform::getSupportedComponents();
    for (const std::string &component : supportedComponents) {
        removeComponent(component);
    }
    delete taskgroup;
    if(root){
        auto* data = root->world.GetComponent<ECS::Entity_Data>(ecsID);
        data->active = false;
        data->destroy = true;
    }
}

void Platform::SaveState(){
    /*if(transform)*/transform.SaveState();
    /*if(trajectory)*/trajectory.SaveState();
    /*if(dynamicModel)*/dynamicModel.SaveState();
    /*if(weapons)*/weapons.SaveState();
}
void Platform::RestoreState(){
    /*if(transform)*/transform.RestoreState();
    /*if(trajectory)*/trajectory.RestoreState();
    /*if(dynamicModel)*/dynamicModel.RestoreState();
    /*if(weapons)*/weapons.RestoreState();
    dropcount = 0;
}

void Platform::Start(){
    return;
    QJsonObject obj = RuntimeEditor::s_missionData;
    if(obj.empty())return;
    QString tm = team == Team::RedTeam?"red":"blue";
    if (obj.contains("doctrine")){
        QJsonObject doctrine = obj["doctrine"].toObject();
        if(doctrine.contains(tm)){
            QJsonObject data = doctrine[tm].toObject();
            for (int i = 0; i < 6; i++) {
                if (DetectionNames[i] == data["detectionPolicy"].toString().toStdString()) {
                    detection = (Detection)i;
                }
            }
            for (int i = 0; i < 7; i++) {
                if (EngagementNames[i] == data["engagementPolicy"].toString().toStdString()) {
                    engagement = (Engagement)i;
                }
            }
            for (int i = 0; i < 10; i++) {
                if (MissionTypeNames[i] == data["missionType"].toString().toStdString()) {
                    mtype = (MissionType)i;
                }
            }
            for (int i = 0; i < 7; i++) {
                if (RetreatNames[i] == data["retreatPolicy"].toString().toStdString()) {
                    retreat = (Retreat)i;
                }
            }
            for (int i = 0; i < 7; i++) {
                if (ROINames[i] == data["rulesOfEngagement"].toString().toStdString()) {
                    roi = (ROI)i;
                }
            }
        }

        QJsonObject tactical = obj["tactical"].toObject();
        if(tactical.contains(tm)){
            QJsonObject data = tactical[tm].toObject();
            for (int i = 0; i < 6; i++) {
                if (WeaponReleaseNames[i] == data["weaponReleaseAuthority"].toString().toStdString()) {
                    weaponrelease = (WeaponRelease)i;
                }
            }
            fuelthreshold  = data["fuelSafetyMargin"].toVariant().toDouble();
            healthThreshold  = data["supportRequestThreshold"].toVariant().toDouble();
            engagementRange  = data["maxEngagementRange"].toVariant().toDouble();
        }
    }
    // switch (detection) {
    // case Detection::STEALTH_MODE:
    //     if(sensors){
    //         for (auto const& pair :*sensors->sensors) {
    //             Sensor* s = pair.second;
    //             if(!s)continue;
    //             s->Active = false;
    //         }
    //     }
    //     break;
    // case Detection::PASSIVE_SENSORS_ONLY:
    //     if(sensors){
    //         for (auto const& pair :*sensors->sensors) {
    //             Sensor* s = pair.second;
    //             if(!s)continue;
    //             if(s->subType == Sensor::SubType::CSM || s->subType == Sensor::SubType::ESM){
    //                 s->Active = true;
    //             }else{
    //                 s->Active = false;
    //             }
    //         }
    //     }
    // case Detection::EMCON_PASSIVE:
    //     if(sensors){
    //         for (auto const& pair :*sensors->sensors) {
    //             Sensor* s = pair.second;
    //             if(!s)continue;
    //             s->Active = false;
    //         }
    //     }
    //     break;

    // case Detection::ACTIVE_RADAR_ALLOWED:
    //     if(sensors){
    //         for (auto const& pair :*sensors->sensors) {
    //             Sensor* s = pair.second;
    //             if(!s)continue;
    //             if(s->subType == Sensor::SubType::Generic){
    //                 s->Active = true;
    //             }else{
    //                 s->Active = false;
    //             }
    //         }
    //     }
    // case Detection::FULL_SENSOR_USAGE:
    //     if(sensors){
    //         for (auto const& pair :*sensors->sensors) {
    //             Sensor* s = pair.second;
    //             if(!s)continue;
    //             s->Active = true;
    //         }
    //     }
    //     break;

    // case Detection::INTERMITTENT_RADAR:
    //     if(sensors){
    //         for (auto const& pair :*sensors->sensors) {
    //             Sensor* s = pair.second;
    //             if(!s)continue;
    //             if(s->subType == Sensor::SubType::Generic){
    //                 s->Active = true;
    //             }else{
    //                 s->Active = false;
    //             }
    //         }
    //     }
    //     break;
    // }


}



void Platform::reset(){
    m_bombsReleased = false;   // allow bombs to drop again on next flight
    //taskgroup->reset();
}

// =============================================================================
// Platform::launchBombs()
// Iterates the WeaponProfile and calls Bomb::launch() on every unlaunched Bomb.
// Called automatically by update() when altitude first crosses DROP_ALTITUDE_M.
// Safe to call manually (e.g. from a script) — isLaunched guard prevents re-launch.
// =============================================================================
void Platform::launchBombs()
{
    if (/*!weapons ||*/ !weapons.weapons || weapons.weapons->empty()) {
        Console::log("Platform::launchBombs() — no weapons on: " + Name);
        return;
    }
    // if (/*!transform ||*/ !transform.geocord) {
    //     Console::error("Platform::launchBombs() — no transform: " + Name);
    //     return;
    // }

    int total = 0, launched = 0;
    for (auto& [id, weapon] : *weapons.weapons) {
        if (!weapon) continue;
        Bomb* bomb = dynamic_cast<Bomb*>(weapon);
        if (!bomb) continue;
        total++;
        if (bomb->isLaunched) continue;
        bomb->parentEntity = this;
        bomb->launch();
        launched++;
    }

    Console::log("Platform::launchBombs() — " + Name + " released " +
                 std::to_string(launched) + "/" + std::to_string(total) + " bombs" +
                 " at alt=" + std::to_string(static_cast<int>(transform.geocord.altitude)) + "m");
}

void Platform::start(){
    //taskgroup->run();
}

void Platform::pause(){
    //taskgroup->pause();
}

void Platform::update(){
    if (isRemoteDISEntity) return;//by Aman

    time =Simulation::simulationTime;
    float fuelconsumption = 0.002*(dynamicModel.currentSpeed/3000.0f);
    // fuel -= fuelconsumption;
    // fuel = fuel<0?0:fuel;
    if(Health <= 0){
        Active = false;
        engaged = false;
        detectionCount = 0;
    }
    // // ── 300 ft bomb auto-release ──────────────────────────────────────────────
    // // When the aircraft first climbs above DROP_ALTITUDE_M (300 ft = 91.44 m),
    // // release all bombs in the WeaponProfile exactly once per flight.
    // // m_bombsReleased prevents re-triggering on every subsequent update() call.
    // if (!m_bombsReleased && transform && transform->geocord &&
    //     transform->geocord->altitude >= static_cast<double>(DROP_ALTITUDE_FT))
    // {
    //     launchBombs();
    //     m_bombsReleased = true;   // bombs released — don't check again this flight
    // }
    int cur = trajectory.current;
    Waypoints* waypoint = trajectory.getCurrentWaypoint();
    if(cur == 0 && _lastcurrent == trajectory.Trajectories.size()-1){
        waypoint = trajectory.getWaypointByIndex(_lastcurrent+1);
    }
    if((cur!=_lastcurrent && waypoint && waypoint->dropWeapon) || (autoDrop && dropTime<time && dropcount > 0) ){
        autoDrop = false;
        if(cur!=_lastcurrent && waypoint&& waypoint->dropWeapon){
            dropcount += waypoint->dropcount;
        }
        for (auto const& pair :*weapons.weapons) {
            Weapon* s = pair.second;
            if(!s)continue;
            Sonobuoy* sono = dynamic_cast<Sonobuoy*>(s);
            if(sono && !sono->drop && dropcount>0){
                autoDrop = true;
                dropTime = time+dropInterval;
                sono->dropped();
                dropcount--;

                break;
            }
        }
    }
    _lastcurrent = cur;

    if(Simulation::fastupdate) return;

    for (auto const& pair :*weapons.weapons) {
        if(Simulation::fastupdate)continue;
        Weapon* s = pair.second;
        if(!s)continue;
        s->Update();
    }
    //qDebug()<<"update";
    int csmTime = 0;
    int esmTime = 0;
    int radarTime = 0;
    int ewTime = 0;
    // if(!sensors)return;
    Waypoints* wp = trajectory.getCurrentWaypoint();
    for (auto const& pair :sensors.sensors) {
        if(Simulation::fastupdate)continue;
        Sensor* s = pair.second;
        if(!s)continue;
        if(wp){
            //s->Active = wp->sensor;
            s->clearTargets();
        }
        if(!s->Active)continue;
        if(s->subType == Sensor::SubType::CSM){
            //qDebug() << "[Platform::update] calling csmScan";
            QElapsedTimer timer;
            timer.start();  // Start measuring
            s->scan();
            qint64 elapsedMs = timer.elapsed();
            csmTime +=elapsedMs;

        }else
            if(s->subType == Sensor::SubType::EO){
                //qDebug() << "[Platform::update] calling csmScan";
                QElapsedTimer timer;
                timer.start();  // Start measuring
                s->scan();
                qint64 elapsedMs = timer.elapsed();
                csmTime +=elapsedMs;
            }else
                if(s->subType == Sensor::SubType::IR){
                    //qDebug() << "[Platform::update] calling csmScan";
                    QElapsedTimer timer;
                    timer.start();  // Start measuring
                    s->scan();
                    qint64 elapsedMs = timer.elapsed();
                    csmTime +=elapsedMs;

                }else
                    if(s->subType == Sensor::SubType::AIS){
                        //qDebug() << "[Platform::update] calling csmScan";
                        QElapsedTimer timer;
                        timer.start();  // Start measuring
                        s->scan();
                        qint64 elapsedMs = timer.elapsed();
                        csmTime +=elapsedMs;

                    }else
                        if(s->subType == Sensor::SubType::ADSB){
                            //qDebug() << "[Platform::update] calling csmScan";
                            QElapsedTimer timer;
                            timer.start();  // Start measuring
                            s->scan();
                            qint64 elapsedMs = timer.elapsed();
                            csmTime +=elapsedMs;

                        }else
                            if(s->subType == Sensor::SubType::ESM){
                                QElapsedTimer timer;
                                timer.start();  // Start measuring
                                //qDebug() << "[Platform::update] calling esmScan";
                                s->scan();
                                qint64 elapsedMs = timer.elapsed();
                                esmTime +=elapsedMs;
                            }else{
                                //qDebug() << "[Platform::update] calling scan + ewscan (Generic)";
                                QElapsedTimer timer;
                                timer.start();  // Start measuring
                                s->scan();
                                qint64 elapsedMs = timer.elapsed();
                                radarTime +=elapsedMs;
                            }
    }

    Profiler::currentFrame->CSMTime +=csmTime;
    Profiler::currentFrame->ESMTime +=esmTime;
    Profiler::currentFrame->RadarTime +=radarTime;
    Profiler::currentFrame->EWTime +=ewTime;

    QElapsedTimer timer;
    timer.start();  // Start measuring

    for (auto const& pair : radios.radios) { // assuming you have a list of radios on this platform
        if(Simulation::fastupdate)continue;
        Radio* r = pair.second;
        if (r) {
            r->scan();
        }
    }

    qint64 elapsedMs = timer.elapsed();
    Profiler::currentFrame->RadioTime +=elapsedMs;

    timer.start();  // Start measuring
    for (auto const& pair : iffs.iffs) {
        if(Simulation::fastupdate)continue;
        IFF* iff = pair.second;
        if (iff) {
            iff->scan();
        }
    }
    elapsedMs = timer.elapsed();
    Profiler::currentFrame->IFFTime +=elapsedMs;

    timer.start();
    // if (/*weapons &&*/ !Simulation::fastupdate) {

    //     for (auto const& pair : *weapons.weapons) {
    //         Weapon* w = pair.second;
    //         if (w && w->Active) {
    //             w->scan();
    //         }
    //     }
    // }
    // Note: add WeaponTime to Frame struct in profiler.h to enable weapon timing
    // if(team == Team::RedTeam||team == Team::BlueTeam){
    //     Decision();
    // }


}

void Platform::Decision(){

    float lowestRange = 11000000000;
    float healthchck = 1000;
    Platform* closestTarget = nullptr;
    Platform* lowestHealthTarget = nullptr;
    int enemyCount = 0;
    for (auto const& pair :sensors.sensors) {
        Sensor* s = pair.second;
        if(!s||!s->Active)continue;
        for (int i = 0; i < s->targets.size(); ++i) {
            if (!s->targets.at(i).entity) continue;   // external/datalink track
            if(s->targets.at(i).entity->team != team){
                if(s->targets.at(i).radius<lowestRange ){
                    lowestRange = s->targets.at(i).radius;
                    qDebug()<<lowestRange;
                    closestTarget = s->targets.at(i).entity;
                }
                if(s->targets.at(i).entity->Health<healthchck ){
                    healthchck = s->targets.at(i).entity->Health;
                    lowestHealthTarget = s->targets.at(i).entity;
                }
                enemyCount++;
            }
        }
        for (int i = 0; i < s->ewtargets.size(); ++i) {
            if (!s->ewtargets.at(i).entity) continue;   // external/datalink track
            if(s->ewtargets.at(i).entity->team != team){
                if(s->ewtargets.at(i).radius<lowestRange ){
                    lowestRange = s->ewtargets.at(i).radius;
                    qDebug()<<lowestRange;
                    closestTarget = s->ewtargets.at(i).entity;
                }
                if(s->ewtargets.at(i).entity->Health<healthchck ){
                    healthchck = s->ewtargets.at(i).entity->Health;
                    lowestHealthTarget = s->ewtargets.at(i).entity;
                }
                enemyCount++;
            }
        }
    }
    detectionCount = enemyCount;
    weaponcount = weapons.weapons->size();
    // --- STEP 1: Retreat Logic (Same as before) ---
    bool shouldRetreat = false;
    if (retreat != Retreat::NEVER_RETREAT) {
        if (retreat == Retreat::RETREAT_IF_DAMAGE_EXCEEDS_THRESHOLD && Health < healthThreshold) shouldRetreat = true;
        else if (retreat == Retreat::RETREAT_IF_FUEL_LOW && fuel < fuelthreshold) shouldRetreat = true;
        else if (retreat == Retreat::RETREAT_IF_OUTNUMBERED && enemyCount > 3) shouldRetreat = true;
        else if (retreat == Retreat::RETREAT_IF_AMMO_DEPLETED && weaponcount <= 0) shouldRetreat = true;
        else if (retreat == Retreat::TACTICAL_WITHDRAWAL && isVictom) shouldRetreat = true;
    }



    if (shouldRetreat) {
        dynamicModel.followTarget = false;
        trajectory.goHome();
        return;
    }

    // --- STEP 2: Target Selection (Using Engagement Policy) ---
    // Yahan hum engagement policy ke basis par "Primary Target" pick karenge
    Platform* primaryTarget = nullptr;

    switch (engagement) {
    case Engagement::NEAREST_TARGET:
        primaryTarget = closestTarget;
        break;

    case Engagement::HIGHEST_THREAT:
        primaryTarget = closestTarget; // e.g., Boss ya Heavy Tank
        break;

    case Engagement::LOWEST_HEALTH_TARGET:
        primaryTarget = lowestHealthTarget; // "Finish him" strategy
        break;

    case Engagement::HIGH_VALUE_TARGET:
        primaryTarget = closestTarget; // e.g., Healer ya Commander
        break;

    case Engagement::ASSIGNED_TARGET_ONLY:
        // primaryTarget = closestTarget;
        break;

    case Engagement::GROUP_ENGAGEMENT:
        primaryTarget = closestTarget; // Jahan zyada dushman hon
        break;

    case Engagement::SEQUENTIAL_ENGAGEMENT:
        primaryTarget = closestTarget;
        break;
    }

    if(primaryTarget && dynamicModel.followEntity!=primaryTarget){
        dynamicModel.followEntity = primaryTarget;
        dynamicModel.followTarget = true;
        engaged = true;
    }
    if(primaryTarget==nullptr || !primaryTarget->Active){
        dynamicModel.followTarget = false;
        engaged = false;
    }

    // --- STEP 3: Engagement & ROI Check ---
    bool canFire = false;
    float distToTarget = lowestRange/1.0f;

    // ROI Check logic using the specific primaryTarget distance
    switch (roi) {
    case ROI::FREE_FIRE:
        canFire = true;
    case ROI::FIRE_ON_DETECTION:
        canFire = (distToTarget <= engagementRange);
        break;

    case ROI::FIRE_ON_IDENTIFICATION:
        canFire = true;
        break;

    case ROI::RETURN_FIRE_ONLY:
        if (isVictom) canFire = true;
        break;

    case ROI::COMMAND_AUTHORIZATION_REQUIRED:
        canFire = false;
        break;
    }

    // --- STEP 4: Weapon Release ---
    if (primaryTarget!=nullptr && primaryTarget->Active && canFire && primaryTarget && !primaryTarget->isVictom ) {
        if (weaponrelease == WeaponRelease::AUTOMATIC || weaponrelease == WeaponRelease::WEAPON_FREE) {
            ////ExecuteFireSequence(primaryTarget); // Target ko shoot karo
            /*if (weapons)*/ {
                for (auto const& pair : *weapons.weapons) {
                    Weapon* w = pair.second;
                    if (w  && !w->isLaunched) {
                        w->setTarget(&primaryTarget->transform,3000);
                        w->missileStart();
                        weapons.weapons->erase(pair.first);
                        break;
                    }
                }
            }
        }
        else if (weaponrelease == WeaponRelease::WEAPON_TIGHT) {
            //if (primaryTarget.isHostileConfirmed) ExecuteFireSequence(primaryTarget);
        }
        else if (weaponrelease == WeaponRelease::SEMI_AUTOMATIC) {
            //RequestOperatorConsent(primaryTarget);
        }
    }
}

void Platform::fireMissile(){
    if(roi != ROI::COMMAND_AUTHORIZATION_REQUIRED) return;
    float lowestRange = 11000000000;
    float healthchck = 1000;
    Platform* closestTarget = nullptr;
    Platform* lowestHealthTarget = nullptr;
    int enemyCount = 0;
    for (auto const& pair :sensors.sensors) {
        Sensor* s = pair.second;
        if(!s||!s->Active)continue;
        for (int i = 0; i < s->targets.size(); ++i) {
            if (!s->targets.at(i).entity) continue;   // external/datalink track
            if(s->targets.at(i).entity->team != team){
                if(s->targets.at(i).radius<lowestRange ){
                    lowestRange = s->targets.at(i).radius;
                    qDebug()<<lowestRange;
                    closestTarget = s->targets.at(i).entity;
                }
                if(s->targets.at(i).entity->Health<healthchck ){
                    healthchck = s->targets.at(i).entity->Health;
                    lowestHealthTarget = s->targets.at(i).entity;
                }
                enemyCount++;
            }
        }
        for (int i = 0; i < s->ewtargets.size(); ++i) {
            if (!s->ewtargets.at(i).entity) continue;   // external/datalink track
            if(s->ewtargets.at(i).entity->team != team){
                if(s->ewtargets.at(i).radius<lowestRange ){
                    lowestRange = s->ewtargets.at(i).radius;
                    qDebug()<<lowestRange;
                    closestTarget = s->ewtargets.at(i).entity;
                }
                if(s->ewtargets.at(i).entity->Health<healthchck ){
                    healthchck = s->ewtargets.at(i).entity->Health;
                    lowestHealthTarget = s->ewtargets.at(i).entity;
                }
                enemyCount++;
            }
        }
    }

    if (/*weapons && */closestTarget!=nullptr && closestTarget->Active) {
        for (auto const& pair : *weapons.weapons) {
            Weapon* w = pair.second;
            if (w  && !w->isLaunched) {
                w->setTarget(&closestTarget->transform,3000);
                w->missileStart();
                weapons.weapons->erase(pair.first);
                break;
            }
        }
    }
}

void Platform::spawn() {
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit root->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
    emit root->entityAddedPointer(QString::fromStdString(parentID),this);
}

void Platform::addParam(std::string key,std::string value){
    customParameters[QString::fromStdString(key)] = QString::fromStdString(value);
}

void Platform::editParam(std::string key,std::string value){
    customParameters[QString::fromStdString(key)] = QString::fromStdString(value);
}

std::string Platform::getParam(std::string key){
    return customParameters[QString::fromStdString(key)].toString().toStdString();
}

void Platform::removeParam(std::string key){
    customParameters.remove(QString::fromStdString(key));
}

std::vector<std::string> Platform::getSupportedComponents() {
    std::vector<std::string> supported;
    supported.push_back("transform");
    supported.push_back("collider");
    supported.push_back("bitmap");
    supported.push_back("trajectory");
    supported.push_back("rigidbody");
    supported.push_back("dynamicModel");
    supported.push_back("crossSection");
    supported.push_back("distype");
    supported.push_back("networkObject");
    supported.push_back("mission");
    supported.push_back("radios");
    supported.push_back("sensors");
    supported.push_back("iffs");
    supported.push_back("weapons");
    return supported;
}

void Platform::addSupportedcomponent(){
    for (const auto& item : getSupportedComponents()) {
        addComponent(item.c_str());
        // 'item' har ek string ko represent karega
        qDebug() << item.c_str(); // Ya std::cout << item << std::endl;
    }
}

void Platform::SyncByJson(const QJsonObject& obj){
    if (obj.contains("dynamicModel") && obj["dynamicModel"].isObject()) {
        dynamicModel.parentEntity = this;
        dynamicModel.SyncByJson(obj["dynamicModel"].toObject());
    }
}

QJsonObject Platform::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["referenceIDE"] = QString::fromStdString(ReferenceID);
    obj["active"] = Active;
    // obj["follow_me"] = followme;
    obj["health"] = toParm(Health,"%",0,100);
    obj["dropInterval"] = toParm(dropInterval,"s",0,3600);
    obj["fuel"] = toParm(fuel,"%",0,100);
    // obj["Mission"] = false;
    obj["illumination"] = toParm(illumination,"%",0,100);
    obj["glintFactor"] = toParm(glintFactor,"%",0,100);

    // By Himanshu For EO/IR Sensor
    obj["surface temp"] = toParm(surfaceTemp,"deg cel",-50,800);
    obj["specular reflectivity"] = toParm(specularReflectivity,"delta",0,1);

    QJsonObject TeamObj;
    TeamObj["type"] = "option";
    QJsonArray TeamoptionsArray;
    for (const std::string& opt : TeamNames)
        TeamoptionsArray.append(QString::fromStdString(opt));
    TeamObj["options"] = TeamoptionsArray;
    TeamObj["value"] = QString::fromStdString(TeamNames[team]);
    obj["Team"] = TeamObj;

    QJsonObject CountryObj;
    CountryObj["type"] = "option";
    CountryObj["visible"] = false;
    QJsonArray CountryoptionsArray;
    for (const std::string& opt : CountryNames)
        CountryoptionsArray.append(QString::fromStdString(opt));
    CountryObj["options"] = CountryoptionsArray;
    CountryObj["value"] = QString::fromStdString(CountryNames[country]);
    obj["Country"] = CountryObj;

    QJsonObject CategoryObj;
    CategoryObj["type"] = "option";
    CategoryObj["refresh"] = true;
    CategoryObj["editable"] = false;
    QJsonArray CategoryoptionsArray;
    for (const std::string& opt : CategoryNames)
        CategoryoptionsArray.append(QString::fromStdString(opt));
    CategoryObj["options"] = CategoryoptionsArray;
    CategoryObj["value"] = QString::fromStdString(CategoryNames[category]);
    obj["Category"] = CategoryObj;

    QJsonObject subCategoryObj;
    subCategoryObj["type"] = "option";
    subCategoryObj["editable"] = false;
    QJsonArray subCategoryoptionsArray;
    if(category == Entity::Category::Air){
        for (const std::string& opt : AirCategoryNames)
            subCategoryoptionsArray.append(QString::fromStdString(opt));
        subCategoryObj["value"] = QString::fromStdString(AirCategoryNames[airCategory]);
    }else
        if(category == Entity::Category::Ground){
            for (const std::string& opt : GroundCategoryNames)
                subCategoryoptionsArray.append(QString::fromStdString(opt));
            subCategoryObj["value"] = QString::fromStdString(GroundCategoryNames[groundCategory]);
        }else
            if(category == Entity::Category::Marine){
                for (const std::string& opt : MarineCategoryNames)
                    subCategoryoptionsArray.append(QString::fromStdString(opt));
                subCategoryObj["value"] = QString::fromStdString(MarineCategoryNames[marineCategory]);
            }
    subCategoryObj["options"] = subCategoryoptionsArray;

    obj["SubCategory"] = subCategoryObj;


    // QJsonObject paramMap;
    // for (const auto& [key, param] : parameters) {
    //     if (param) {
    //         paramMap[QString::fromStdString(key)] = param->toJson();
    //     }
    // }

    // QJsonObject parObj;
    // parObj["type"] = "parameter";
    // parObj["value"] = paramMap;
    // obj["parameters"] = parObj;

    /*if (transform)*/ obj["transform"] = transform.toJson();
    /*if (trajectory)*/ obj["trajectory"] = trajectory.toJson();
    /*if (crossSection)*/ obj["crossSection"] = crossSection.toJson();
    obj["disType"] = disType.toJson();//by aman
    /*if (rigidbody)*/ obj["rigidbody"] = rigidbody.toJson();
    /*if (dynamicModel) */obj["dynamicModel"] = dynamicModel.toJson();
    /*if (collider)*/ obj["collider"] = collider.toJson();
    /*if (meshRenderer2d)*/ obj["bitmap"] = meshRenderer2d.toJson();
    /*if (sensors)*/ obj["sensors"] = sensors.toJson();
    /*if (iffs)*/ obj["iffs"] = iffs.toJson();
    /*if (radios)*/ obj["radios"] = radios.toJson();
    /*if (weapons)*/ obj["weapons"] = weapons.toJson();
    // QJsonArray radioArray;
    // for (Radio* r : radioList) {
    //     if (r) radioArray.append(r->toJson());
    // }
    // obj["radios"] = radioArray;

    // // QJsonArray sensorArray;
    // // for (Sensor* s : sensorList) {
    // //     if (s) sensorArray.append(s->toJson());
    // // }
    // // obj["sensors"] = sensorArray;

    // QJsonArray iffArray;
    // for (IFF* i : iffList) {
    //     if (i) iffArray.append(i->toJson());
    // }
    // obj["iffs"] = iffArray;

    QJsonObject entityObj;
    entityObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : entityTypeOptions())
        optionsArray.append(opt);

    entityObj["options"] = optionsArray;
    entityObj["value"] = entityTypeToString(type);
    obj["type"] = entityObj;

    // Include custom parameters
    // for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
    //     obj[it.key()] = it.value();
    // }

    return obj;
}


void Platform::fromJson(const QJsonObject& obj) {

    if (obj.contains("active")){
        Active = obj["active"].toBool();
        if(root){
            auto* data = root->world.GetComponent<ECS::Entity_Data>(ecsID);
            data->active = Active;
        }
    }

    if(obj.contains("follow_me")){
        followme = obj["follow_me"].toBool();
    }

    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();
    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();
    if (obj.contains("referenceIDE"))
        ReferenceID = obj["referenceIDE"].toString().toStdString();
    if (obj.contains("health"))
        Health = valueFromParm(obj["health"].toObject());
    if (obj.contains("dropInterval"))
        dropInterval = valueFromParm(obj["dropInterval"].toObject());
    if (obj.contains("fuel"))
        fuel = valueFromParm(obj["fuel"].toObject());

    if (obj.contains("illumination"))
        illumination = valueFromParm(obj["illumination"].toObject());
    if (obj.contains("glintFactor"))
        glintFactor = valueFromParm(obj["glintFactor"].toObject());

    // By Himanshu For EO/IR Sensor
    if (obj.contains("surface temp"))
        surfaceTemp = valueFromParm(obj["surface temp"].toObject());
    if (obj.contains("specular reflectivity"))
        specularReflectivity = valueFromParm(obj["specular reflectivity"].toObject());

    if (obj.contains("Mission")){
        bool mission = obj["Mission"].toBool();
        // if(mission){
        //     taskgroup->show();
        // }
    }
    if (obj.contains("Team") && obj["Team"].isObject()) {
        QJsonObject TeamObj = obj["Team"].toObject();
        if (TeamObj.contains("value")){
            for (int i = 0; i < 8; i++) {
                if (TeamNames[i] == TeamObj["value"].toString().toStdString()) {
                    team = (Team)i;
                }
            }
        }
    }

    if (obj.contains("Country") && obj["Country"].isObject()) {
        QJsonObject CountryObj = obj["Country"].toObject();
        if (CountryObj.contains("value")){
            for (int i = 0; i < 195; i++) {
                if (CountryNames[i] == CountryObj["value"].toString().toStdString()) {
                    country = (Country)i;
                }
            }
        }
    }
    if (obj.contains("Category") && obj["Category"].isObject()) {
        QJsonObject CategoryObj = obj["Category"].toObject();
        if (CategoryObj.contains("value")){
            for (int i = 0; i < 3 ; i++) {
                if (CategoryNames[i] == CategoryObj["value"].toString().toStdString()) {
                    category = (Category)i;
                }
            }
        }
    }

    if (obj.contains("SubCategory") && obj["SubCategory"].isObject()) {
        QJsonObject subCategoryObj = obj["SubCategory"].toObject();
        if (subCategoryObj.contains("value")){
            if(category == Entity::Category::Air){
                for (int i = 0; i < 7 ; i++) {
                    if (AirCategoryNames[i] == subCategoryObj["value"].toString().toStdString()) {
                        airCategory = (SubAirCategory)i;
                    }
                }
            }else
                if(category == Entity::Category::Ground){
                    for (int i = 0; i < 6 ; i++) {
                        if (GroundCategoryNames[i] == subCategoryObj["value"].toString().toStdString()) {
                            groundCategory = (SubGroundCategory)i;
                        }
                    }
                }else
                    if(category == Entity::Category::Marine){
                        for (int i = 0; i < 7 ; i++) {
                            if (MarineCategoryNames[i] == subCategoryObj["value"].toString().toStdString()) {
                                marineCategory = (SubMarineCategory)i;
                            }
                        }
                    }
        }
    }

    if (obj.contains("type") && obj["type"].isObject()) {
        QJsonObject entityObj = obj["type"].toObject();
        if (entityObj.contains("value"))
            type = stringToEntityType(entityObj["value"].toString());
    }

    if (obj.contains("transform") && obj["transform"].isObject()) {
        transform.parentEntity = this;
        transform.fromJson(obj["transform"].toObject());
    }

    if (obj.contains("crossSection") && obj["crossSection"].isObject()) {
        crossSection.parentEntity = this;
        crossSection.fromJson(obj["crossSection"].toObject());
    }
    if (obj.contains("disType") && obj["disType"].isObject()) {
        disType.parentEntity = this;
        disType.fromJson(obj["disType"].toObject());
    }//by aman
    if (obj.contains("trajectory") && obj["trajectory"].isObject()) {
        trajectory.parentEntity = this;
        trajectory.fromJson(obj["trajectory"].toObject());
    }

    if (obj.contains("rigidbody") && obj["rigidbody"].isObject()) {
        rigidbody.parentEntity = this;
        rigidbody.fromJson(obj["rigidbody"].toObject());
    }

    if (obj.contains("dynamicModel") && obj["dynamicModel"].isObject()) {
        dynamicModel.parentEntity = this;
        dynamicModel.fromJson(obj["dynamicModel"].toObject());
    }

    if (obj.contains("collider") && obj["collider"].isObject()) {
        collider.parentEntity = this;
        collider.fromJson(obj["collider"].toObject());
    }

    if (obj.contains("bitmap") && obj["bitmap"].isObject()) {
        meshRenderer2d.parentEntity = this;
        meshRenderer2d.fromJson(obj["bitmap"].toObject());
        //Logger
        for(auto it = m_hierarchy->Platforms.begin();
             it != m_hierarchy->Platforms.end(); ++it){
            if(this == (*it).second){
                //qDebug()<<"Mesh Platform Found!!";
                emit m_hierarchy->meshRenderer2DisAdded((*it).first.c_str(),&meshRenderer2d);
            }
        }
    }

    // if (obj.contains("meshRenderer2d") && obj["meshRenderer2d"].isObject()) { // Fix: Correct key
    //     if (!meshRenderer2d) addComponent("bitmap");
    //     meshRenderer2d->fromJson(obj["meshRenderer2d"].toObject());
    // }

    if (obj.contains("sensors") && obj["sensors"].isObject()) { // Fix: Correct key
        sensors.parentEntity = this;
        sensors.fromJson(obj["sensors"].toObject());
    }

    if (obj.contains("iffs") && obj["iffs"].isObject()) { // Fix: Correct key
        iffs.parentEntity = this;
        iffs.fromJson(obj["iffs"].toObject());
    }

    if (obj.contains("radios") && obj["radios"].isObject()) { // Fix: Correct key
        radios.parentEntity = this;
        radios.fromJson(obj["radios"].toObject());
    }

    if (obj.contains("weapons") && obj["weapons"].isObject()) {
        weapons.parentEntity = this;
        weapons.fromJson(obj["weapons"].toObject());
    }



}


void Platform::addComponent(std::string name)
{
    if (!root)
        return;

    // ============================================================
    // Transform
    // ============================================================

    if (name == "transform")
    {
        if (root->Components.find(transform.ID) !=
            root->Components.end())
        {
            return;
        }

        transform.init(this);

        root->Components.insert({
            transform.ID,
            &transform
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(transform.ID),
            "transform"
            );
    }

    // ============================================================
    // Cross Section
    // ============================================================

    else if (name == "crossSection")
    {
        if (root->Components.find(crossSection.ID) !=
            root->Components.end())
        {
            return;
        }

        crossSection.init(this);

        root->Components.insert({
            crossSection.ID,
            &crossSection
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(crossSection.ID),
            "crossSection"
            );
    }

    // ============================================================
    // Trajectory
    // ============================================================

    else if (name == "trajectory")
    {
        // Always bind the value-member component to this platform.
        trajectory.parentEntity = this;
        trajectory.parentID = ID;

        // init() handles ECS component creation.
        trajectory.init(this);

        // Register only once in global component map.
        auto it = root->Components.find(trajectory.ID);

        if (it == root->Components.end())
        {
            root->Components.insert({
                trajectory.ID,
                &trajectory
            });

            // Preserve new-version TaskGroup connections.
            connect(
                taskgroup,
                &TaskGroup::goHome,
                &trajectory,
                &Trajectory::goHome
                );

            connect(
                taskgroup,
                &TaskGroup::activateSensors,
                &trajectory,
                &Trajectory::activateSensors
                );

            connect(
                taskgroup,
                &TaskGroup::deactivateSensors,
                &trajectory,
                &Trajectory::deactivateSensors
                );

            emit root->componentAdded(
                QString::fromStdString(ID),
                QString::fromStdString(trajectory.ID),
                "trajectory"
                );
        }

        return;
    }

    // ============================================================
    // Rigidbody
    // ============================================================

    else if (name == "rigidbody")
    {
        if (root->Components.find(rigidbody.ID) !=
            root->Components.end())
        {
            return;
        }

        rigidbody.init(this);

        root->Components.insert({
            rigidbody.ID,
            &rigidbody
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(rigidbody.ID),
            "rigidbody"
            );
    }

    // ============================================================
    // Sensors
    // ============================================================

    else if (name == "sensors")
    {
        if (!sensors.ID.empty())
        {
            // Existing value-member SensorProfile.
            //
            // Do not blindly create/register it again.
            sensors.init(this);
            sensors.parentID = ID;

            auto it =
                root->Components.find(sensors.ID);

            if (it == root->Components.end())
            {
                root->Components.insert({
                    sensors.ID,
                    &sensors
                });

                qDebug()
                    << "[PLATFORM SENSORS]"
                    << "SensorProfile registered:"
                    << QString::fromStdString(sensors.ID);

                emit root->componentAdded(
                    QString::fromStdString(ID),
                    QString::fromStdString(sensors.ID),
                    "sensors"
                    );
            }
            else
            {
                qDebug()
                << "[PLATFORM SENSORS]"
                << "SensorProfile already registered:"
                << QString::fromStdString(sensors.ID);
            }

            return;
        }

        // Genuine first-time creation.
        sensors.init(this);
        sensors.parentID = ID;

        root->Components.insert({
            sensors.ID,
            &sensors
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(sensors.ID),
            "sensors"
            );
    }

    // ============================================================
    // IFF
    // ============================================================

    else if (name == "iffs")
    {
        if (root->Components.find(iffs.ID) !=
            root->Components.end())
        {
            return;
        }

        iffs.init(this);
        iffs.parentID = ID;

        root->Components.insert({
            iffs.ID,
            &iffs
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(iffs.ID),
            "iffs"
            );
    }

    // ============================================================
    // Radio
    // ============================================================

    else if (name == "radios")
    {
        if (root->Components.find(radios.ID) !=
            root->Components.end())
        {
            return;
        }

        radios.init(this);
        radios.parentID = ID;

        root->Components.insert({
            radios.ID,
            &radios
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(radios.ID),
            "radios"
            );
    }

    // ============================================================
    // Weapons
    // ============================================================

    else if (name == "weapons")
    {
        if (root->Components.find(weapons.ID) !=
            root->Components.end())
        {
            return;
        }

        weapons.init(this);
        weapons.parentID = ID;

        root->Components.insert({
            weapons.ID,
            &weapons
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(weapons.ID),
            "weapons"
            );
    }

    // ============================================================
    // Dynamic Model
    // ============================================================

    else if (name == "dynamicModel")
    {
        if (root->Components.find(dynamicModel.ID) !=
            root->Components.end())
        {
            return;
        }

        dynamicModel.transform = &transform;
        dynamicModel.rigidbody = &rigidbody;
        dynamicModel.trajectory = &trajectory;

        dynamicModel.init(this);

        root->Components.insert({
            dynamicModel.ID,
            &dynamicModel
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(dynamicModel.ID),
            "dynamicModel"
            );

        emit root->entityPhysicsAdded(
            QString::fromStdString(parentID),
            this
            );
    }

    // ============================================================
    // Collider
    // ============================================================

    else if (name == "collider")
    {
        if (root->Components.find(collider.ID) !=
            root->Components.end())
        {
            return;
        }

        collider.init(this);
        collider.parentID = ID;

        root->Components.insert({
            collider.ID,
            &collider
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(collider.ID),
            "collider"
            );
    }

    else if (name == "distype")//by aman
    {
        if (root->Components.find(disType.ID) !=
            root->Components.end())
        {
            return;
        }

        disType.init(this);
        disType.parentID = ID;

        root->Components.insert({
            disType.ID,
            &disType
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(disType.ID),
            "distype"
            );
    }
    // ============================================================
    // Bitmap
    // ============================================================

    else if (name == "bitmap")
    {
        if (root->Components.find(meshRenderer2d.ID) !=
            root->Components.end())
        {
            return;
        }

        meshRenderer2d.init(this);

        root->Components.insert({
            meshRenderer2d.ID,
            &meshRenderer2d
        });

        emit root->componentAdded(
            QString::fromStdString(ID),
            QString::fromStdString(meshRenderer2d.ID),
            "bitmap"
            );

        emit root->entityMeshAdded(
            QString::fromStdString(parentID),
            this
            );
    }
}

void Platform::removeComponent(std::string name) {
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if(!root)return;
    if (name == "transform") {
        // if (!transform) return;
        root->Components.erase(transform.ID);
        // delete transform;
        // transform = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "transform");
        emit root->entityMeshRemoved(QString::fromStdString(parentID));
        emit root->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "crossSection") {
        // if (!crossSection) return;
        root->Components.erase(crossSection.ID);
        // delete crossSection;
        // crossSection = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "crossSection");
    } else if (name == "trajectory") {
        // if (!trajectory) return;
        root->Components.erase(trajectory.ID);
        // delete trajectory;
        // trajectory = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "trajectory");
    } else if (name == "sensors") {
        // if (!sensors) return;
        root->Components.erase(sensors.ID);
        // delete sensors;
        // sensors = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "sensors");
    } else if (name == "iffs") {
        // if (!iffs) return;
        root->Components.erase(iffs.ID);
        // delete iffs;
        // iffs = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "iffs");
    } else if (name == "radios") {
        // if (!radios) return;
        root->Components.erase(radios.ID);
        // delete radios;
        // radios = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "radios");
    } else if (name == "weapons") {
        // if (!weapons) return;
        root->Components.erase(weapons.ID);
        // delete weapons;
        // weapons = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "weapons");
    } else if (name == "rigidbody") {
        // if (!rigidbody) return;
        root->Components.erase(rigidbody.ID);
        // delete rigidbody;
        // rigidbody = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "rigidbody");
        emit root->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "dynamicModel") {
        // if (!dynamicModel) return;
        root->Components.erase(dynamicModel.ID);
        // delete dynamicModel;
        // dynamicModel = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "dynamicModel");
        emit root->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "collider") {
        // if (!collider) return;
        root->Components.erase(collider.ID);
        // delete collider;
        // collider = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "collider");
        emit root->entityMeshRemoved(QString::fromStdString(parentID));
        emit root->entityPhysicsRemoved(QString::fromStdString(parentID));
    }
 else if (name == "distype") {//by aman
    root->Components.erase(disType.ID);
    emit root->componentRemoved(QString::fromStdString(ID), "distype");
}

    else if (name == "bitmap") {
        // if (!meshRenderer2d) return;
        root->Components.erase(meshRenderer2d.ID);
        // delete meshRenderer2d;
        // meshRenderer2d = nullptr;
        emit root->componentRemoved(QString::fromStdString(ID), "bitmap");
        emit root->entityMeshRemoved(QString::fromStdString(parentID));
    }
}

QJsonObject Platform::getComponent(std::string name) {
    if (name == "transform") {
        // if (!transform) { Console::error(name + ": not exist"); return QJsonObject(); }
        return transform.toJson();
    } else if (name == "crossSection") {
        // if (!crossSection) { Console::error(name + ": not exist"); return QJsonObject(); }
        return crossSection.toJson();
    } else if (name == "trajectory") {
        // if (!trajectory) { Console::error(name + ": not exist"); return QJsonObject(); }
        return trajectory.toJson();
    } else if (name == "rigidbody") {
        // if (!rigidbody) { Console::error(name + ": not exist"); return QJsonObject(); }
        return rigidbody.toJson();
    } else if (name == "dynamicModel") {
        // if (!dynamicModel) { Console::error(name + ": not exist"); return QJsonObject(); }
        return dynamicModel.toJson();
    } else if (name == "collider") {
        // if (!collider) { Console::error(name + ": not exist"); return QJsonObject(); }
        return collider.toJson();
    } else if (name == "bitmap") {
        // if (!meshRenderer2d) { Console::error(name + ": not exist"); return QJsonObject(); }
        return meshRenderer2d.toJson();
    } else if (name == "sensors") {
        // if (!sensors) { Console::error(name + ": not exist"); return QJsonObject(); }
        return sensors.toJson();
    }else if (name == "iffs") {
        // if (!iffs) { Console::error(name + ": not exist"); return QJsonObject(); }
        return iffs.toJson();
    }else if (name == "radios") {
        // if (!radios) { Console::error(name + ": not exist"); return QJsonObject(); }
        return radios.toJson();
    }else if (name == "weapons") {
        // if (!weapons) { Console::error(name + ": not exist"); return QJsonObject(); }
        return weapons.toJson();
    }else if (name == "distype") {
    return disType.toJson();
}
    return QJsonObject();
}

void Platform::updateComponent(QString name, const QJsonObject& obj) {
    if (name == "transform") {
        //if (!transform) { Console::error(name.toStdString() + ": not exist"); return; }
        transform.fromJson(obj);
    } else if (name == "crossSection") {
        // if (!crossSection) { Console::error(name.toStdString() + ": not exist"); return; }
        crossSection.fromJson(obj);
    } else if (name == "trajectory") {
        // if (!trajectory) { Console::error(name.toStdString() + ": not exist"); return; }
        trajectory.fromJson(obj);
    } else if (name == "rigidbody") {
        // if (!rigidbody) { Console::error(name.toStdString() + ": not exist"); return; }
        rigidbody.fromJson(obj);
    } else if (name == "dynamicModel") {
        // if (!dynamicModel) { Console::error(name.toStdString() + ": not exist"); return; }
        dynamicModel.fromJson(obj);
    } else if (name == "collider") {
        // if (!collider) { Console::error(name.toStdString() + ": not exist"); return; }
        collider.fromJson(obj);
    } else if (name == "bitmap") {
        // if (!meshRenderer2d) { Console::error(name.toStdString() + ": not exist"); return; }
        meshRenderer2d.fromJson(obj);
    } else if (name == "sensors") {
        // if (!sensors) { Console::error(name.toStdString() + ": not exist"); return; }
        sensors.fromJson(obj);
    } else if (name == "iffs") {
        // if (!iffs) { Console::error(name.toStdString() + ": not exist"); return; }
        iffs.fromJson(obj);
    } else if (name == "radios") {
        // if (!radios) { Console::error(name.toStdString() + ": not exist"); return; }
        radios.fromJson(obj);
    } else if (name == "weapons") {
        // if (!weapons) { Console::error(name.toStdString() + ": not exist"); return; }
        weapons.fromJson(obj);
    } else if (name == "distype") {
    disType.fromJson(obj);
}
}

Sensor* Platform::getSensorByName(const std::string& name) const
{
    // if (/*!sensors ||*/ !sensors.sensors)
    //     return nullptr;

    for (const auto& [id, sensor] : sensors.sensors)
    {
        if (sensor && sensor->Name == name)
            return sensor;
    }

    return nullptr;
}
Radio* Platform::getRadioByName(const std::string& name) const
{
    // if (/*!radios ||*/ !radios.radios)
    //     return nullptr;

    // Iterate through the radios map in the RadioProfile component
    for (const auto& [id, radio] : radios.radios)
    {
        if (radio && radio->Name == name)
            return radio;
    }

    return nullptr;
}
IFF* Platform::getIFFByName(const std::string& name) const
{
    // if (/*!iffs || */!iffs.iffs)
    //     return nullptr;

    // Iterate through the iffs map in the IFFProfile component
    for (const auto& [id, iff] : iffs.iffs)
    {
        if (iff && iff->Name == name)
            return iff;
    }

    return nullptr;
}
Weapon* Platform::getWeaponByName(const std::string& name) const {
    // if (!weapons) return nullptr;
    for (auto& [key, weapon] : *weapons.weapons) {
        if (weapon && weapon->Name == name) {
            return weapon;
        }
    }
    return nullptr;
}
