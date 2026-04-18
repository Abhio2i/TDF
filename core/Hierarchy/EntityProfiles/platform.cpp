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

Platform::Platform(Hierarchy* h) : Entity(h) {
    type = Constants::EntityType::Platform;
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "roll";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 1.0f;
    parameters["roll"] = par;
    m_hierarchy = h;
    taskgroup = new TaskGroup();
    taskgroup->hide();
}

Platform::~Platform(){
    std::vector<std::string>supportedComponents =  Platform::getSupportedComponents();
    for (const std::string &component : supportedComponents) {
        removeComponent(component);
    }
    delete taskgroup;
}

void Platform::Start(){
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
    switch (detection) {
    case Detection::STEALTH_MODE:
        if(sensors){
            for (auto const& pair :*sensors->sensors) {
                Sensor* s = pair.second;
                if(!s)continue;
                s->Active = false;
            }
        }
        break;
    case Detection::PASSIVE_SENSORS_ONLY:
        if(sensors){
            for (auto const& pair :*sensors->sensors) {
                Sensor* s = pair.second;
                if(!s)continue;
                if(s->subType == Sensor::SubType::CSM || s->subType == Sensor::SubType::ESM){
                    s->Active = true;
                }else{
                    s->Active = false;
                }
            }
        }
    case Detection::EMCON_PASSIVE:
        if(sensors){
            for (auto const& pair :*sensors->sensors) {
                Sensor* s = pair.second;
                if(!s)continue;
                s->Active = false;
            }
        }
        break;

    case Detection::ACTIVE_RADAR_ALLOWED:
        if(sensors){
            for (auto const& pair :*sensors->sensors) {
                Sensor* s = pair.second;
                if(!s)continue;
                if(s->subType == Sensor::SubType::Generic){
                    s->Active = true;
                }else{
                    s->Active = false;
                }
            }
        }
    case Detection::FULL_SENSOR_USAGE:
        if(sensors){
            for (auto const& pair :*sensors->sensors) {
                Sensor* s = pair.second;
                if(!s)continue;
                s->Active = true;
            }
        }
        break;

    case Detection::INTERMITTENT_RADAR:
        if(sensors){
            for (auto const& pair :*sensors->sensors) {
                Sensor* s = pair.second;
                if(!s)continue;
                if(s->subType == Sensor::SubType::Generic){
                    s->Active = true;
                }else{
                    s->Active = false;
                }
            }
        }
        break;
    }

    // --- PART 2: Mission Specific Logic ---
    // switch (mtype) {
    // case MissionType::PATROL:
    //     SetSensorScanArc(360.0f);        // Har taraf nazar rakho
    //     SetCruisingSpeed(0.6f);          // Moderate speed for endurance
    //     EnableAutoNavigation(true);       // Defined path par chalo
    //     break;

    // case MissionType::SURVEILLANCE:
    // case MissionType::RECONNAISSANCE:
    //     SetHighSensitivitySensors(true); // Max range detection
    //     SetSignalEmissions(0.1f);        // Low profile/Quiet operations
    //     SetDataLogging(true);            // Recording mission data
    //     break;

    // case MissionType::STRIKE:
    //     SetTargetLockPriority(true);     // Weapon systems priority
    //     SetCruisingSpeed(1.0f);          // Full speed to target
    //     SetSensorScanArc(60.0f);         // Narrow, focused beam for precision
    //     break;

    // case MissionType::INTERCEPTION:
    //     SetResponseTime(0.1f);           // Ultra-fast reaction
    //     SetCruisingSpeed(1.2f);          // Afterburners/Max thrust
    //     SetTargetTrackingMode(true);     // Constant lock-on
    //     break;

    // case MissionType::ESCORT:
    //     SetFormationFlying(true);        // Maintain distance with ally
    //     SetSensorScanArc(180.0f);        // Frontal and flank protection
    //     SetCollisionAvoidance(true);     // Don't hit the escorted unit
    //     break;

    // case MissionType::AREA_DENIAL:
    //     SetStationaryDefense(true);      // Hold a specific coordinate
    //     SetAggressiveTargeting(true);    // Pre-emptive scanning
    //     SetAutoDeployMines(true);        // If applicable
    //     break;

    // case MissionType::SEARCH_AND_RESCUE:
    //     SetSensorFilter("LifeSigns");    // Specific signal filtering
    //     SetCruisingSpeed(0.4f);          // Slow and steady for scanning
    //     EnableExternalLights(true);      // Visible for survivors
    //     break;

    // case MissionType::BLOCKADE:
    //     SetJammingSystems(true);         // Communication block
    //     SetInterdictionMode(true);       // Stop any passing object
    //     SetScanningIntensity(1.0f);      // Heavy sensor usage
    //     break;

    // case MissionType::DEFENSIVE_HOLD:
    //     SetFortificationMode(true);      // Shields/Armor priority
    //     SetEnergyManagement("Shields");  // Divert power to defense
    //     SetSensorScanArc(120.0f);        // Focused on approach vectors
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
    if (!weapons || !weapons->weapons || weapons->weapons->empty()) {
        Console::log("Platform::launchBombs() — no weapons on: " + Name);
        return;
    }
    if (!transform || !transform->geocord) {
        Console::error("Platform::launchBombs() — no transform: " + Name);
        return;
    }

    int total = 0, launched = 0;
    for (auto& [id, weapon] : *weapons->weapons) {
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
                 " at alt=" + std::to_string(static_cast<int>(transform->geocord->altitude)) + "m");
}

void Platform::start(){
    //taskgroup->run();
}

void Platform::pause(){
    //taskgroup->pause();
}

void Platform::update(){
    float fuelconsumption = 0.02*(dynamicModel->currentSpeed/3000.0f);
    fuel -= fuelconsumption;
    fuel = fuel<0?0:fuel;
    if(Health <= 0){
        Active = false;
        engaged = false;
        detectionCount = 0;
    }
    // ── 300 ft bomb auto-release ──────────────────────────────────────────────
    // When the aircraft first climbs above DROP_ALTITUDE_M (300 ft = 91.44 m),
    // release all bombs in the WeaponProfile exactly once per flight.
    // m_bombsReleased prevents re-triggering on every subsequent update() call.
    if (!m_bombsReleased && transform && transform->geocord &&
        transform->geocord->altitude >= static_cast<double>(DROP_ALTITUDE_FT))
    {
        launchBombs();
        m_bombsReleased = true;   // bombs released — don't check again this flight
    }
    //qDebug()<<"update";
    int csmTime = 0;
    int esmTime = 0;
    int radarTime = 0;
    int ewTime = 0;
    if(!sensors)return;
    Waypoints* wp = trajectory->getCurrentWaypoint();
    for (auto const& pair :*sensors->sensors) {
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

    for (auto const& pair : *radios->radios) { // assuming you have a list of radios on this platform
        Radio* r = pair.second;
        if (r) {
            r->scan();
        }
    }

    qint64 elapsedMs = timer.elapsed();
    Profiler::currentFrame->RadioTime +=elapsedMs;

    timer.start();  // Start measuring
    for (auto const& pair : *iffs->iffs) {
        IFF* iff = pair.second;
        if (iff) {
            iff->scan();
        }
    }
    elapsedMs = timer.elapsed();
    Profiler::currentFrame->IFFTime +=elapsedMs;

    timer.start();
    if (weapons) {
        for (auto const& pair : *weapons->weapons) {
            Weapon* w = pair.second;
            if (w && w->Active) {
                w->scan();
            }
        }
    }
    // Note: add WeaponTime to Frame struct in profiler.h to enable weapon timing
    if(team == Team::RedTeam||team == Team::BlueTeam){
        Decision();
    }


}


void Platform::Decision(){

    float lowestRange = 11000000000;
    float healthchck = 1000;
    Platform* closestTarget = nullptr;
    Platform* lowestHealthTarget = nullptr;
    int enemyCount = 0;
    for (auto const& pair :*sensors->sensors) {
        Sensor* s = pair.second;
        if(!s||!s->Active)continue;
        for (int i = 0; i < s->targets.size(); ++i) {
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
     weaponcount = weapons->weapons->size();
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
        dynamicModel->followTarget = false;
        trajectory->goHome();
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

    if(primaryTarget && dynamicModel->followEntity!=primaryTarget){
        dynamicModel->followEntity = primaryTarget;
        dynamicModel->followTarget = true;
        engaged = true;
    }
    if(primaryTarget==nullptr || !primaryTarget->Active){
        dynamicModel->followTarget = false;
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
            if (weapons) {
                for (auto const& pair : *weapons->weapons) {
                    Weapon* w = pair.second;
                    if (w  && !w->isLaunched) {
                        w->setTarget(primaryTarget->transform,3000);
                        w->missileStart();
                        weapons->weapons->erase(pair.first);
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
    for (auto const& pair :*sensors->sensors) {
        Sensor* s = pair.second;
        if(!s||!s->Active)continue;
        for (int i = 0; i < s->targets.size(); ++i) {
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

    if (weapons && closestTarget!=nullptr && closestTarget->Active) {
        for (auto const& pair : *weapons->weapons) {
            Weapon* w = pair.second;
            if (w  && !w->isLaunched) {
                w->setTarget(closestTarget->transform,3000);
                w->missileStart();
                weapons->weapons->erase(pair.first);
                break;
            }
        }
    }
}


void Platform::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
    emit parent->entityAddedPointer(QString::fromStdString(parentID),this);
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
    supported.push_back("trajectory");
    supported.push_back("rigidbody");
    supported.push_back("dynamicModel");
    supported.push_back("collider");
    supported.push_back("crossSection");
    supported.push_back("networkObject");
    supported.push_back("bitmap");
    supported.push_back("mission");
    supported.push_back("radios");
    supported.push_back("sensors");
    supported.push_back("iffs");
    supported.push_back("weapons");
    return supported;
}

QJsonObject Platform::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;
    obj["health"] = toParm(Health,"%");
    obj["fuel"] = toParm(fuel,"%");
    obj["Mission"] = false;
    obj["illumination"] = toParm(illumination,"%",0,100);
    obj["glintFactor"] = toParm(glintFactor,"%",0,100);
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
    QJsonArray CountryoptionsArray;
    for (const std::string& opt : CountryNames)
        CountryoptionsArray.append(QString::fromStdString(opt));
    CountryObj["options"] = CountryoptionsArray;
    CountryObj["value"] = QString::fromStdString(CountryNames[country]);
    obj["Country"] = CountryObj;

    QJsonObject CategoryObj;
    CategoryObj["type"] = "option";
    QJsonArray CategoryoptionsArray;
    for (const std::string& opt : CategoryNames)
        CategoryoptionsArray.append(QString::fromStdString(opt));
    CategoryObj["options"] = CategoryoptionsArray;
    CategoryObj["value"] = QString::fromStdString(CategoryNames[category]);
    obj["Category"] = CategoryObj;

    QJsonObject paramMap;
    for (const auto& [key, param] : parameters) {
        if (param) {
            paramMap[QString::fromStdString(key)] = param->toJson();
        }
    }

    QJsonObject parObj;
    parObj["type"] = "parameter";
    parObj["value"] = paramMap;
    obj["parameters"] = parObj;

    if (transform) obj["transform"] = transform->toJson();
    if (trajectory) obj["trajectory"] = trajectory->toJson();
    if (crossSection) obj["crossSection"] = crossSection->toJson();
    if (rigidbody) obj["rigidbody"] = rigidbody->toJson();
    if (dynamicModel) obj["dynamicModel"] = dynamicModel->toJson();
    if (collider) obj["collider"] = collider->toJson();
    if (meshRenderer2d) obj["bitmap"] = meshRenderer2d->toJson();
    if (sensors) obj["sensors"] = sensors->toJson();
    if (iffs) obj["iffs"] = iffs->toJson();
    if (radios) obj["radios"] = radios->toJson();
    if (weapons) obj["weapons"] = weapons->toJson();
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
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    return obj;
}


void Platform::fromJson(const QJsonObject& obj) {

    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();
    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();
    if (obj.contains("health"))
        Health = valueFromParm(obj["health"].toObject());
    if (obj.contains("illumination"))
        illumination = valueFromParm(obj["illumination"].toObject());
    if (obj.contains("glintFactor"))
        glintFactor = valueFromParm(obj["glintFactor"].toObject());
    if (obj.contains("Mission")){
        bool mission = obj["Mission"].toBool();
        if(mission){
            taskgroup->show();
        }
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
            for (int i = 0; i < 5; i++) {
                if (CategoryNames[i] == CategoryObj["value"].toString().toStdString()) {
                    category = (Category)i;
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
        if (!transform) addComponent("transform");
        transform->fromJson(obj["transform"].toObject());
    }else{
        if (!transform) addComponent("transform");
    }

    if (obj.contains("crossSection") && obj["crossSection"].isObject()) {
        if (!crossSection) addComponent("crossSection");
        crossSection->fromJson(obj["crossSection"].toObject());
    }else{
        if (!crossSection) addComponent("crossSection");
    }

    if (obj.contains("trajectory") && obj["trajectory"].isObject()) {
        if (!trajectory) addComponent("trajectory");
        trajectory->fromJson(obj["trajectory"].toObject());
    }else{
        if (!trajectory) addComponent("trajectory");
    }

    if (obj.contains("rigidbody") && obj["rigidbody"].isObject()) {
        if (!rigidbody) addComponent("rigidbody");
        rigidbody->fromJson(obj["rigidbody"].toObject());
    }else{
        if (!rigidbody) addComponent("rigidbody");
    }

    if (obj.contains("dynamicModel") && obj["dynamicModel"].isObject()) {
        if (!dynamicModel) addComponent("dynamicModel");
        dynamicModel->fromJson(obj["dynamicModel"].toObject());
    }else{
        if (!dynamicModel) addComponent("dynamicModel");
    }

    if (obj.contains("collider") && obj["collider"].isObject()) {
        if (!collider) addComponent("collider");
        collider->fromJson(obj["collider"].toObject());
    }else{
        if (!collider) addComponent("collider");
    }

    if (obj.contains("bitmap") && obj["bitmap"].isObject()) { // Fix: Correct key
        if (!meshRenderer2d) addComponent("bitmap");
        meshRenderer2d->fromJson(obj["bitmap"].toObject());
        //Logger
        for(auto it = m_hierarchy->Platforms.begin();
             it != m_hierarchy->Platforms.end(); ++it){
            if(this == (*it).second){
                //qDebug()<<"Mesh Platform Found!!";
                emit m_hierarchy->meshRenderer2DisAdded((*it).first.c_str(),meshRenderer2d);
            }
        }
    }else{
        if (!meshRenderer2d) addComponent("bitmap");
    }

    // if (obj.contains("meshRenderer2d") && obj["meshRenderer2d"].isObject()) { // Fix: Correct key
    //     if (!meshRenderer2d) addComponent("bitmap");
    //     meshRenderer2d->fromJson(obj["meshRenderer2d"].toObject());
    // }

    if (obj.contains("sensors") && obj["sensors"].isObject()) { // Fix: Correct key
        if (!sensors) addComponent("sensors");
        sensors->fromJson(obj["sensors"].toObject());
    }else{
        if (!sensors) addComponent("sensors");
    }

    if (obj.contains("iffs") && obj["iffs"].isObject()) { // Fix: Correct key
        if (!iffs) addComponent("iffs");
        iffs->fromJson(obj["iffs"].toObject());
    }else{
        if (!iffs) addComponent("iffs");
    }

    if (obj.contains("radios") && obj["radios"].isObject()) { // Fix: Correct key
        if (!radios) addComponent("radios");
        radios->fromJson(obj["radios"].toObject());
    }else{
        if (!radios) addComponent("radios");
    }

    if (obj.contains("weapons") && obj["weapons"].isObject()) {
        if (!weapons) addComponent("weapons");
        weapons->fromJson(obj["weapons"].toObject());
    }else{
        if (!weapons) addComponent("weapons");
    }

    if (obj.contains("parameters")) {
        QJsonObject parObj = obj["parameters"].toObject();
        if (parObj.contains("value")) { // Fix: Check "value" instead of "array"
            QJsonObject paramMap = parObj["value"].toObject();
            for (const QString& key : paramMap.keys()) {
                QJsonObject paramObj = paramMap[key].toObject();
                std::shared_ptr<Parameter> param = std::make_shared<Parameter>();
                param->fromJson(paramObj);
                parameters[key.toStdString()] = param;
            }
        }
    }
    // Merge custom parameters
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key() != "name" &&
            it.key() != "id" &&
            it.key() != "parent_id" &&
            it.key() != "active" &&
            it.key() != "parameters" &&
            it.key() != "type" &&
            it.key() != "transform" &&
            it.key() != "trajectory" &&
            it.key() != "rigidbody" &&
            it.key() != "dynamicModel" &&
            it.key() != "crossSection" &&
            it.key() != "collider" &&
            it.key() != "meshRenderer2d" &&
            it.key() != "bitmap" &&
            it.key() != "radios" &&
            it.key() != "sensors" &&
            it.key() != "health" &&
            it.key() != "fuel" &&
            it.key() != "iffs" &&
            it.key() != "weapons" &&
            it.key() != "Mission" &&
            it.key() != "parent_id") {
            customParameters[it.key()] = it.value();
        }
    }

}


void Platform::addComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform){
            transform = new Transform();
            transform->parentEntity = this;
            parent->Components.insert({transform->ID, transform});
            emit parent->componentAdded(QString::fromStdString(ID), QString::fromStdString(transform->ID),"transform");
        }
    } else if (name == "crossSection") {
        if (!crossSection){
            crossSection = new CrossSection();
            crossSection->parentEntity = this;
            parent->Components.insert({crossSection->ID, crossSection});
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(crossSection->ID), "crossSection");
        }
    } else if (name == "trajectory") {
        if (!trajectory){
            trajectory = new Trajectory();
            trajectory->parentEntity = this;
            parent->Components.insert({trajectory->ID, trajectory});
            connect(taskgroup,&TaskGroup::goHome,trajectory,&Trajectory::goHome);
            connect(taskgroup,&TaskGroup::activateSensors,trajectory,&Trajectory::activateSensors);
            connect(taskgroup,&TaskGroup::deactivateSensors,trajectory,&Trajectory::deactivateSensors);
            connect(taskgroup,&TaskGroup::goHome,trajectory,&Trajectory::goHome);
            connect(taskgroup,&TaskGroup::goHome,trajectory,&Trajectory::goHome);
            emit parent->componentAdded(QString::fromStdString(ID), QString::fromStdString(trajectory->ID),"trajectory");
        }
    } else if (name == "rigidbody") {
        if (!rigidbody) {
            if (!transform)
                addComponent("transform");
            rigidbody = new Rigidbody();
            rigidbody->parentEntity = this;
            parent->Components.insert({rigidbody->ID, rigidbody});
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(rigidbody->ID), "rigidbody");
        }

    } else if (name == "sensors") {
        if (!sensors) {
            if (!transform)
                addComponent("transform");
            sensors = new SensorProfile(parent);
            sensors->parentEntity = this;
            sensors->parentID = ID;
            parent->Components.insert({sensors->ID, sensors});
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(sensors->ID), "sensors");
        }

    } else if (name == "iffs") {
        if (!iffs) {
            if (!transform)
                addComponent("transform");
            iffs = new IFFProfile(parent);
            iffs->parentEntity = this;
            iffs->parentID = ID;
            parent->Components.insert({iffs->ID, iffs});
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(iffs->ID), "iffs");
        }

    } else if (name == "radios") {
        if (!radios) {
            if (!transform)
                addComponent("transform");
            radios = new RadioProfile(parent);
            radios->parentEntity = this;
            radios->parentID = ID;
            parent->Components.insert({radios->ID, radios});
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(radios->ID), "radios");
        }

    } else if (name == "weapons") {
        if (!weapons) {
            if (!transform)
                addComponent("transform");
            weapons = new WeaponProfile(parent);
            weapons->parentEntity = this;
            weapons->parentID = ID;
            parent->Components.insert({weapons->ID, weapons});
            emit parent->componentAdded(QString::fromStdString(ID), QString::fromStdString(weapons->ID), "weapons");
        }
    } else if (name == "dynamicModel") {
        if (!dynamicModel) {
            if (!transform)
                addComponent("transform");
            if (!rigidbody)
                addComponent("rigidbody");
            if (!collider)
                addComponent("collider");
            if (!trajectory)
                addComponent("trajectory");
            dynamicModel = new DynamicModel();
            dynamicModel->parentEntity = this;
            dynamicModel->transform = transform;
            dynamicModel->rigidbody = rigidbody;
            dynamicModel->trajectory = trajectory;
            dynamicModel->init();
            parent->Components.insert({dynamicModel->ID, dynamicModel});
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(dynamicModel->ID), "dynamicModel");
            emit parent->entityPhysicsAdded(QString::fromStdString(parentID), this);
        }

    } else if (name == "collider") {
        if (!collider) {
            if (!transform)
                addComponent("transform");
            collider = new Collider(parent);
            collider->parentEntity = this;
            collider->parentID = ID;
            parent->Components.insert({collider->ID, collider});
            emit parent->componentAdded(QString::fromStdString(ID),QString::fromStdString(collider->ID), "collider");
        }

    } else if (name == "bitmap") {
        if (!meshRenderer2d) {
            if (!transform)
                addComponent("transform");
            if (!collider)
                addComponent("collider");
            meshRenderer2d = new MeshRenderer2D();
            meshRenderer2d->parentEntity = this;
            parent->Components.insert({meshRenderer2d->ID, meshRenderer2d});
            emit parent->componentAdded(QString::fromStdString(ID), QString::fromStdString(meshRenderer2d->ID),"bitmap");
            emit parent->entityMeshAdded(QString::fromStdString(parentID), this);
        }

    }
}

void Platform::removeComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (name == "transform") {
        if (!transform) return;
        parent->Components.erase(transform->ID);
        delete transform;
        transform = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "transform");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "crossSection") {
        if (!crossSection) return;
        parent->Components.erase(crossSection->ID);
        delete crossSection;
        crossSection = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "crossSection");
    } else if (name == "trajectory") {
        if (!trajectory) return;
        parent->Components.erase(trajectory->ID);
        delete trajectory;
        trajectory = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "trajectory");
    } else if (name == "sensors") {
        if (!sensors) return;
        parent->Components.erase(sensors->ID);
        delete sensors;
        sensors = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "sensors");
    } else if (name == "iffs") {
        if (!iffs) return;
        parent->Components.erase(iffs->ID);
        delete iffs;
        iffs = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "iffs");
    } else if (name == "radios") {
        if (!radios) return;
        parent->Components.erase(radios->ID);
        delete radios;
        radios = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "radios");
    } else if (name == "weapons") {
        if (!weapons) return;
        parent->Components.erase(weapons->ID);
        delete weapons;
        weapons = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "weapons");
    } else if (name == "rigidbody") {
        if (!rigidbody) return;
        parent->Components.erase(rigidbody->ID);
        delete rigidbody;
        rigidbody = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "rigidbody");
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "dynamicModel") {
        if (!dynamicModel) return;
        parent->Components.erase(dynamicModel->ID);
        delete dynamicModel;
        dynamicModel = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "dynamicModel");
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "collider") {
        if (!collider) return;
        parent->Components.erase(collider->ID);
        delete collider;
        collider = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "collider");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "bitmap") {
        if (!meshRenderer2d) return;
        parent->Components.erase(meshRenderer2d->ID);
        delete meshRenderer2d;
        meshRenderer2d = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "bitmap");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
    }
}

QJsonObject Platform::getComponent(std::string name) {
    if (name == "transform") {
        if (!transform) { Console::error(name + ": not exist"); return QJsonObject(); }
        return transform->toJson();
    } else if (name == "crossSection") {
        if (!crossSection) { Console::error(name + ": not exist"); return QJsonObject(); }
        return crossSection->toJson();
    } else if (name == "trajectory") {
        if (!trajectory) { Console::error(name + ": not exist"); return QJsonObject(); }
        return trajectory->toJson();
    } else if (name == "rigidbody") {
        if (!rigidbody) { Console::error(name + ": not exist"); return QJsonObject(); }
        return rigidbody->toJson();
    } else if (name == "dynamicModel") {
        if (!dynamicModel) { Console::error(name + ": not exist"); return QJsonObject(); }
        return dynamicModel->toJson();
    } else if (name == "collider") {
        if (!collider) { Console::error(name + ": not exist"); return QJsonObject(); }
        return collider->toJson();
    } else if (name == "bitmap") {
        if (!meshRenderer2d) { Console::error(name + ": not exist"); return QJsonObject(); }
        return meshRenderer2d->toJson();
    } else if (name == "sensors") {
        if (!sensors) { Console::error(name + ": not exist"); return QJsonObject(); }
        return sensors->toJson();
    }else if (name == "iffs") {
        if (!iffs) { Console::error(name + ": not exist"); return QJsonObject(); }
        return iffs->toJson();
    }else if (name == "radios") {
        if (!radios) { Console::error(name + ": not exist"); return QJsonObject(); }
        return radios->toJson();
    }else if (name == "weapons") {
        if (!weapons) { Console::error(name + ": not exist"); return QJsonObject(); }
        return weapons->toJson();
    }
    return QJsonObject();
}

void Platform::updateComponent(QString name, const QJsonObject& obj) {
    if (name == "transform") {
        if (!transform) { Console::error(name.toStdString() + ": not exist"); return; }
        transform->fromJson(obj);
    } else if (name == "crossSection") {
        if (!crossSection) { Console::error(name.toStdString() + ": not exist"); return; }
        crossSection->fromJson(obj);
    } else if (name == "trajectory") {
        if (!trajectory) { Console::error(name.toStdString() + ": not exist"); return; }
        trajectory->fromJson(obj);
    } else if (name == "rigidbody") {
        if (!rigidbody) { Console::error(name.toStdString() + ": not exist"); return; }
        rigidbody->fromJson(obj);
    } else if (name == "dynamicModel") {
        if (!dynamicModel) { Console::error(name.toStdString() + ": not exist"); return; }
        dynamicModel->fromJson(obj);
    } else if (name == "collider") {
        if (!collider) { Console::error(name.toStdString() + ": not exist"); return; }
        collider->fromJson(obj);
    } else if (name == "bitmap") {
        if (!meshRenderer2d) { Console::error(name.toStdString() + ": not exist"); return; }
        meshRenderer2d->fromJson(obj);
    } else if (name == "sensors") {
        if (!sensors) { Console::error(name.toStdString() + ": not exist"); return; }
        sensors->fromJson(obj);
    } else if (name == "iffs") {
        if (!iffs) { Console::error(name.toStdString() + ": not exist"); return; }
        iffs->fromJson(obj);
    } else if (name == "radios") {
        if (!radios) { Console::error(name.toStdString() + ": not exist"); return; }
        radios->fromJson(obj);
    } else if (name == "weapons") {
        if (!weapons) { Console::error(name.toStdString() + ": not exist"); return; }
        weapons->fromJson(obj);
    }
}

Sensor* Platform::getSensorByName(const std::string& name) const
{
    if (!sensors || !sensors->sensors)
        return nullptr;

    for (const auto& [id, sensor] : *sensors->sensors)
    {
        if (sensor && sensor->Name == name)
            return sensor;
    }

    return nullptr;
}
Radio* Platform::getRadioByName(const std::string& name) const
{
    if (!radios || !radios->radios)
        return nullptr;

    // Iterate through the radios map in the RadioProfile component
    for (const auto& [id, radio] : *radios->radios)
    {
        if (radio && radio->Name == name)
            return radio;
    }

    return nullptr;
}
IFF* Platform::getIFFByName(const std::string& name) const
{
    if (!iffs || !iffs->iffs)
        return nullptr;

    // Iterate through the iffs map in the IFFProfile component
    for (const auto& [id, iff] : *iffs->iffs)
    {
        if (iff && iff->Name == name)
            return iff;
    }

    return nullptr;
}
Weapon* Platform::getWeaponByName(const std::string& name) const {
    if (!weapons) return nullptr;
    for (auto& [key, weapon] : *weapons->weapons) {
        if (weapon && weapon->Name == name) {
            return weapon;
        }
    }
    return nullptr;
}
