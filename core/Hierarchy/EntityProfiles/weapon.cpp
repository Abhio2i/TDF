
#include "weapon.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>
#include "core/Hierarchy/Utils/entityutils.h"
#include <core/GlobalRegistry.h>
#include <cmath>
#include <QtMath>
#include <QVector3D>
#include <algorithm>
#include "GUI/Tacticaldisplay/canvaswidget.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// CONSTRUCTOR  — creates entity + auto-integrates all 7 components
// =============================================================================
Weapon::Weapon(Hierarchy* h) : Entity(h)
{
    type  = Constants::EntityType::Weapon;
    safed = true;
    armed = false;

    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name  = "weapon_param";
    par->type  = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["weapon_param"] = par;

    addComponent("transform");
    addComponent("rigidbody");
    addComponent("collider");
    addComponent("trajectory");
    addComponent("bitmap");
    addComponent("dynamicModel");
    addComponent("crossSection");

    syncComponentsFromWeaponData();
}

// =============================================================================
// DESTRUCTOR
// =============================================================================
Weapon::~Weapon()
{
    removeComponent("crossSection");
    removeComponent("dynamicModel");
    removeComponent("bitmap");
    removeComponent("trajectory");
    removeComponent("collider");
    removeComponent("rigidbody");
    removeComponent("transform");
}

// =============================================================================
// spawn()
// =============================================================================
void Weapon::spawn()
{
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(
        QString::fromStdString(parentID),
        QString::fromStdString(ID),
        QString::fromStdString(Name));
}

// =============================================================================
// getSupportedComponents()
// =============================================================================
std::vector<std::string> Weapon::getSupportedComponents()
{
    return { "transform", "rigidbody", "collider", "trajectory",
            "bitmap", "dynamicModel", "crossSection" };
}

// =============================================================================
// addComponent()
// =============================================================================
void Weapon::addComponent(std::string name)
{
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parent) return;

    if (name == "transform") {
        if (!transform) {
            transform = new Transform();
            transform->parentEntity = this;
            parent->Components.insert({transform->ID, transform});
            emit parent->componentAdded(QString::fromStdString(ID),
                                        QString::fromStdString(transform->ID), "transform");
        }
    } else if (name == "rigidbody") {
        if (!rigidbody) {
            if (!transform) addComponent("transform");
            rigidbody = new Rigidbody();
            rigidbody->parentEntity = this;
            parent->Components.insert({rigidbody->ID, rigidbody});
            emit parent->componentAdded(QString::fromStdString(ID),
                                        QString::fromStdString(rigidbody->ID), "rigidbody");
        }
    } else if (name == "collider") {
        if (!collider) {
            if (!transform) addComponent("transform");
            collider = new Collider(parent);
            collider->parentEntity = this;
            collider->parentID     = ID;
            parent->Components.insert({collider->ID, collider});
            emit parent->componentAdded(QString::fromStdString(ID),
                                        QString::fromStdString(collider->ID), "collider");
        }
    } else if (name == "trajectory") {
        if (!trajectory) {
            if (!transform) addComponent("transform");
            trajectory = new Trajectory();
            trajectory->parentEntity = this;
            parent->Components.insert({trajectory->ID, trajectory});
            emit parent->componentAdded(QString::fromStdString(ID),
                                        QString::fromStdString(trajectory->ID), "trajectory");
        }
    } else if (name == "bitmap") {
        if (!meshRenderer2d) {
            if (!transform) addComponent("transform");
            if (!collider)  addComponent("collider");
            meshRenderer2d = new MeshRenderer2D();
            meshRenderer2d->parentEntity = this;
            meshRenderer2d->Sprite->clear();
            meshRenderer2d->Sprite->append(":/sea/images/sea/AUV.png");
            parent->Components.insert({meshRenderer2d->ID, meshRenderer2d});
            emit parent->componentAdded(QString::fromStdString(ID),
                                        QString::fromStdString(meshRenderer2d->ID), "bitmap");
            emit parent->entityMeshAdded(QString::fromStdString(parentID), this);
        }
    } else if (name == "dynamicModel") {
        if (!dynamicModel) {
            if (!transform)  addComponent("transform");
            if (!rigidbody)  addComponent("rigidbody");
            if (!collider)   addComponent("collider");
            if (!trajectory) addComponent("trajectory");
            dynamicModel = new DynamicModel();
            dynamicModel->parentEntity = this;
            dynamicModel->transform    = transform;
            dynamicModel->rigidbody    = rigidbody;
            dynamicModel->trajectory   = trajectory;
            dynamicModel->init();
            parent->Components.insert({dynamicModel->ID, dynamicModel});
            emit parent->componentAdded(QString::fromStdString(ID),
                                        QString::fromStdString(dynamicModel->ID), "dynamicModel");
            emit parent->entityPhysicsAdded(QString::fromStdString(parentID), this);
        }
    } else if (name == "crossSection") {
        if (!crossSection) {
            crossSection = new CrossSection();
            crossSection->parentEntity = this;
            parent->Components.insert({crossSection->ID, crossSection});
            emit parent->componentAdded(QString::fromStdString(ID),
                                        QString::fromStdString(crossSection->ID), "crossSection");
        }
    }
}

// =============================================================================
// removeComponent()
// =============================================================================
void Weapon::removeComponent(std::string name)
{
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parent) return;

    if (name == "transform") {
        if (!transform) return;
        parent->Components.erase(transform->ID);
        delete transform; transform = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "transform");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "rigidbody") {
        if (!rigidbody) return;
        parent->Components.erase(rigidbody->ID);
        delete rigidbody; rigidbody = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "rigidbody");
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "collider") {
        if (!collider) return;
        parent->Components.erase(collider->ID);
        delete collider; collider = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "collider");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "trajectory") {
        if (!trajectory) return;
        parent->Components.erase(trajectory->ID);
        delete trajectory; trajectory = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "trajectory");
    } else if (name == "bitmap") {
        if (!meshRenderer2d) return;
        parent->Components.erase(meshRenderer2d->ID);
        delete meshRenderer2d; meshRenderer2d = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "bitmap");
        emit parent->entityMeshRemoved(QString::fromStdString(parentID));
    } else if (name == "dynamicModel") {
        if (!dynamicModel) return;
        parent->Components.erase(dynamicModel->ID);
        delete dynamicModel; dynamicModel = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "dynamicModel");
        emit parent->entityPhysicsRemoved(QString::fromStdString(parentID));
    } else if (name == "crossSection") {
        if (!crossSection) return;
        parent->Components.erase(crossSection->ID);
        delete crossSection; crossSection = nullptr;
        emit parent->componentRemoved(QString::fromStdString(ID), "crossSection");
    }
}

// =============================================================================
// getComponent() / updateComponent()
// =============================================================================
QJsonObject Weapon::getComponent(std::string name)
{
    if (name == "_self")    return toJson();
    if (name == "transform")    { if (!transform)     { Console::error(name+": not exist"); return {}; } return transform->toJson(); }
    if (name == "rigidbody")    { if (!rigidbody)     { Console::error(name+": not exist"); return {}; } return rigidbody->toJson(); }
    if (name == "collider")     { if (!collider)      { Console::error(name+": not exist"); return {}; } return collider->toJson(); }
    if (name == "trajectory")   { if (!trajectory)    { Console::error(name+": not exist"); return {}; } return trajectory->toJson(); }
    if (name == "bitmap")       { if (!meshRenderer2d){ Console::error(name+": not exist"); return {}; } return meshRenderer2d->toJson(); }
    if (name == "dynamicModel" || name == "dynamicmodel") { if (!dynamicModel) { Console::error(name+": not exist"); return {}; } return dynamicModel->toJson(); }
    if (name == "crossSection" || name == "crosssection") { if (!crossSection) { Console::error(name+": not exist"); return {}; } return crossSection->toJson(); }
    Console::error("Weapon::getComponent – unknown: " + name);
    return {};
}

void Weapon::updateComponent(QString name, const QJsonObject& obj)
{
    if (name == "transform")    { if (transform)      transform->fromJson(obj);      return; }
    if (name == "rigidbody")    { if (rigidbody)      rigidbody->fromJson(obj);      return; }
    if (name == "collider")     { if (collider)       collider->fromJson(obj);       return; }
    if (name == "trajectory")   { if (trajectory)     trajectory->fromJson(obj);     return; }
    if (name == "bitmap")       { if (meshRenderer2d) meshRenderer2d->fromJson(obj); return; }
    if (name == "dynamicModel" || name == "dynamicmodel") { if (dynamicModel) dynamicModel->fromJson(obj); return; }
    if (name == "crossSection" || name == "crosssection") { if (crossSection) crossSection->fromJson(obj); return; }
    Console::error(name.toStdString() + ": Weapon – unknown component");
}

// =============================================================================
// VIRTUAL STUBS  — subclasses override what they need
// (Same pattern: Sensor::scan() is a stub, ESM/CSM override it)
// =============================================================================
void Weapon::launch()         {}   // Missile, Rocket etc. override
void Weapon::flyToTarget()    {}
void Weapon::updateGuidance() {}
void Weapon::checkDetonation(){}
void Weapon::scan()           {}
void Weapon::clearTargets()   { detects.clear(); targets.clear(); }

bool Weapon::canEngage(Platform* target)
{
    if (!target || safed || !armed || !transform) return false;
    float distance = transform->translation().distanceToPoint(target->transform->translation());
    return distance <= maxRange && distance >= minRange;
}

float Weapon::calculateImpactTime(Platform* target)
{
    if (!target || maxVelocity == 0.0f || !transform) return -1.0f;
    float distance = transform->translation().distanceToPoint(target->transform->translation());
    return distance / maxVelocity;
}

float Weapon::calculateLaunchPoint() { return maxRange * 0.5f; }
void  Weapon::disarmWeapon()         { armed = false; safed = true; }
void  Weapon::rearmWeapon()          { armed = true;  safed = false; }

// =============================================================================
// ENUM HELPERS  — detonation only (guidance/propulsion live in subclasses)
// =============================================================================
QString Weapon::detonationTypeToString() const {
    switch (detonationType) {
    case DetonationType::Impact:    return "Impact";
    case DetonationType::Proximity: return "Proximity";
    case DetonationType::Timed:     return "Timed";
    case DetonationType::Command:   return "Command";
    default:                        return "Proximity";
    }
}
void Weapon::setDetonationTypeFromString(const QString& str) {
    if      (str == "Impact")    detonationType = DetonationType::Impact;
    else if (str == "Proximity") detonationType = DetonationType::Proximity;
    else if (str == "Timed")     detonationType = DetonationType::Timed;
    else if (str == "Command")   detonationType = DetonationType::Command;
    else                          detonationType = DetonationType::Proximity;
}

QString Weapon::weaponTypeToString() const {
    return weaponTypeNameFromEnum(weaponType);
}

QString Weapon::weaponTypeNameFromEnum(WeaponType t) const {
    switch (t) {
    case WeaponType::Missile:   return "Missile";
    case WeaponType::Bomb:      return "Bomb";
    case WeaponType::Torpedo:   return "Torpedo";
    case WeaponType::Artillery: return "Artillery";
    case WeaponType::Rocket:    return "Rocket";
    case WeaponType::Flare:     return "Flare";
    case WeaponType::Chaff:     return "Chaff";
    default:                    return "Missile";
    }
}

// =============================================================================
// syncComponentsFromWeaponData()
// =============================================================================
void Weapon::syncComponentsFromWeaponData()
{
    if (rigidbody) {
        rigidbody->Mass = totalMass;
        rigidbody->Drag = (diameter > 0.0f) ? (diameter * 0.1f) : 0.02f;
    }
    if (dynamicModel) {
        dynamicModel->maxSpeed    = maxVelocity * 3.6f;
        dynamicModel->moveSpeed   = maxVelocity * 3.6f;
        dynamicModel->minSpeed    = minVelocity * 3.6f;
        dynamicModel->maxAltitude = maxAltitude * 3.28084f;
        dynamicModel->mass        = totalMass;
    }
    if (transform) {
        if (transform->geocord && transform->geocord->altitude == 0.0f)
            transform->geocord->altitude = minAltitude;
    }
}

// =============================================================================
// toJson()  —  writes shared fields + all 7 components
// Subclass calls Weapon::toJson() first, then appends its own keys.
// "weaponTypeName" key is written here so sensorprofile-style fromJson can
// pick the correct subclass on load.
// =============================================================================
QJsonObject Weapon::toJson() const
{
    QJsonObject obj;
    obj["name"]           = QString::fromStdString(Name);
    obj["branch"]         = "Entity";
    obj["id"]             = QString::fromStdString(ID);
    obj["parent_id"]      = QString::fromStdString(parentID);
    obj["active"]         = Active;
    obj["designation"]    = QString::fromStdString(designation);
    obj["armed"]          = armed;
    obj["safed"]          = safed;
    obj["weaponTypeName"] = weaponTypeName();   // subclass override → "Missile", "Bomb" …

    // Shared numeric fields
    obj["length"]          = length;
    obj["diameter"]        = diameter;
    obj["totalMass"]       = totalMass;
    obj["payloadMass"]     = payloadMass;
    obj["fuelMass"]        = fuelMass;
    obj["maxVelocity"]     = maxVelocity;
    obj["minVelocity"]     = minVelocity;
    obj["maxRange"]        = maxRange;
    obj["minRange"]        = minRange;
    obj["maxAltitude"]     = maxAltitude;
    obj["minAltitude"]     = minAltitude;
    obj["maximumG"]        = maximumG;
    obj["flightTimeMax"]   = flightTimeMax;
    obj["blastRadius"]     = blastRadius;
    obj["effectiveRadius"] = effectiveRadius;
    obj["peakPressure"]    = peakPressure;
    obj["warheadType"]     = QString::fromStdString(warheadType);
    obj["detonationType"]  = detonationTypeToString();
    obj["proximityRange"]  = proximityRange;
    obj["timerDelay"]      = timerDelay;
    obj["launchG"]         = launchG;
    obj["rearmTime"]       = rearmTime;

    // Parameters map
    QJsonObject paramMap;
    for (const auto& [key, param] : parameters)
        if (param) paramMap[QString::fromStdString(key)] = param->toJson();
    QJsonObject parObj;
    parObj["type"]  = "parameter";
    parObj["value"] = paramMap;
    obj["parameters"] = parObj;

    // 7 auto-components
    if (transform)      obj["transform"]    = transform->toJson();
    if (rigidbody)      obj["rigidbody"]    = rigidbody->toJson();
    if (collider)       obj["collider"]     = collider->toJson();
    if (trajectory)     obj["trajectory"]   = trajectory->toJson();
    if (meshRenderer2d) obj["bitmap"]       = meshRenderer2d->toJson();
    if (dynamicModel)   obj["dynamicModel"] = dynamicModel->toJson();
    if (crossSection)   obj["crossSection"] = crossSection->toJson();

    return obj;
    // NOTE: subclass appends its own type-specific keys after this.
}

// =============================================================================
// fromJson()  —  restores shared fields + all 7 components
// Subclass calls Weapon::fromJson() first, then reads its own keys.
// =============================================================================
void Weapon::fromJson(const QJsonObject& obj)
{
    if (obj.contains("name"))          Name        = obj["name"].toString().toStdString();
    if (obj.contains("id"))            ID          = obj["id"].toString().toStdString();
    if (obj.contains("parent_id"))     parentID    = obj["parent_id"].toString().toStdString();
    if (obj.contains("active"))        Active      = obj["active"].toBool();
    if (obj.contains("designation"))   designation = obj["designation"].toString().toStdString();
    if (obj.contains("armed"))         armed       = obj["armed"].toBool();
    if (obj.contains("safed"))         safed       = obj["safed"].toBool();

    if (obj.contains("length"))          length          = obj["length"].toDouble(length);
    if (obj.contains("diameter"))        diameter        = obj["diameter"].toDouble(diameter);
    if (obj.contains("totalMass"))       totalMass       = obj["totalMass"].toDouble(totalMass);
    if (obj.contains("payloadMass"))     payloadMass     = obj["payloadMass"].toDouble(payloadMass);
    if (obj.contains("fuelMass"))        fuelMass        = obj["fuelMass"].toDouble(fuelMass);
    if (obj.contains("maxVelocity"))     maxVelocity     = obj["maxVelocity"].toDouble(maxVelocity);
    if (obj.contains("minVelocity"))     minVelocity     = obj["minVelocity"].toDouble(minVelocity);
    if (obj.contains("maxRange"))        maxRange        = obj["maxRange"].toDouble(maxRange);
    if (obj.contains("minRange"))        minRange        = obj["minRange"].toDouble(minRange);
    if (obj.contains("maxAltitude"))     maxAltitude     = obj["maxAltitude"].toDouble(maxAltitude);
    if (obj.contains("minAltitude"))     minAltitude     = obj["minAltitude"].toDouble(minAltitude);
    if (obj.contains("maximumG"))        maximumG        = obj["maximumG"].toDouble(maximumG);
    if (obj.contains("flightTimeMax"))   flightTimeMax   = obj["flightTimeMax"].toDouble(flightTimeMax);
    if (obj.contains("blastRadius"))     blastRadius     = obj["blastRadius"].toDouble(blastRadius);
    if (obj.contains("effectiveRadius")) effectiveRadius = obj["effectiveRadius"].toDouble(effectiveRadius);
    if (obj.contains("peakPressure"))    peakPressure    = obj["peakPressure"].toDouble(peakPressure);
    if (obj.contains("warheadType"))     warheadType     = obj["warheadType"].toString().toStdString();
    if (obj.contains("detonationType"))  setDetonationTypeFromString(obj["detonationType"].toString());
    if (obj.contains("proximityRange"))  proximityRange  = obj["proximityRange"].toDouble(proximityRange);
    if (obj.contains("timerDelay"))      timerDelay      = obj["timerDelay"].toDouble(timerDelay);
    if (obj.contains("launchG"))         launchG         = obj["launchG"].toDouble(launchG);
    if (obj.contains("rearmTime"))       rearmTime       = obj["rearmTime"].toInt(rearmTime);

    // 7 auto-components
    if (obj.contains("transform")    && transform)     transform->fromJson(obj["transform"].toObject());
    if (obj.contains("rigidbody")    && rigidbody)     rigidbody->fromJson(obj["rigidbody"].toObject());
    if (obj.contains("collider")     && collider)      collider->fromJson(obj["collider"].toObject());
    if (obj.contains("trajectory")   && trajectory)    trajectory->fromJson(obj["trajectory"].toObject());
    if (obj.contains("bitmap")       && meshRenderer2d) meshRenderer2d->fromJson(obj["bitmap"].toObject());
    if (obj.contains("dynamicModel") && dynamicModel)  dynamicModel->fromJson(obj["dynamicModel"].toObject());
    if (obj.contains("crossSection") && crossSection)  crossSection->fromJson(obj["crossSection"].toObject());

    syncComponentsFromWeaponData();
    // NOTE: subclass reads its own type-specific keys after this.
}

// =============================================================================
// FLIGHT MONITOR  — infrastructure shared by Missile (and any future subclass
// that needs active flight). setTarget / startFlightMonitor / stopFlightMonitor
// stay here because they are pure infrastructure with no type-specific logic.
// =============================================================================
void Weapon::setTarget(double lat, double lon, float detonationRangeMetres)
{
    m_targetLat       = lat;
    m_targetLon       = lon;
    m_detonationRange = detonationRangeMetres;
}

void Weapon::setTarget(Transform* tr, float detonationRangeMetres)
{
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if (!parent ) {
        qDebug() << "Error: Hierarchy or Platforms Map is null!";
        return;
    }

    std::string targetID = tr->parentEntity->ID;

    // Check if ID exists in map
    if (parent->Platforms.find(targetID) != parent->Platforms.end()) {

        // Original pointer access karein
        m_targetplatform = (parent->Platforms)[targetID];
        m_targetLat = m_targetplatform->transform->getLatitude();
        m_targetLon = m_targetplatform->transform->getLongitude();
    } else {
        qDebug() << "Error: Target ID not found in Platforms map!";
    }

    m_target          = tr;
    m_detonationRange = detonationRangeMetres;
}

void Weapon::startFlightMonitor()
{
    if (!m_flightCheckTimer) {
        m_flightCheckTimer = new QTimer(this);
        // Lambda so virtual dispatch fires at call time → Missile::checkFlightState()
        // (a direct &Weapon::checkFlightState pointer would bypass the vtable)
        connect(m_flightCheckTimer, &QTimer::timeout, this, [this]() {
            this->checkFlightState();
        });
    }
    m_flightCheckTimer->start(100);
}

void Weapon::stopFlightMonitor()
{
    if (m_flightCheckTimer) m_flightCheckTimer->stop();
    if (m_updateTimer)      m_updateTimer->stop();
    m_flightActive  = false;
    m_updateRunning = false;
}

void Weapon::pauseFlightMonitor()
{
    // Suspend the 100ms tick without clearing isLaunched / m_flightActive.
    // The weapon resumes exactly where it left off when resumeFlightMonitor()
    // is called (e.g. when simulation unpauses).
    if (m_flightCheckTimer && m_flightCheckTimer->isActive())
        m_flightCheckTimer->stop();
    if (m_updateTimer && m_updateTimer->isActive())
        m_updateTimer->stop();
}

void Weapon::resumeFlightMonitor()
{
    // Restart the tick only if the weapon is genuinely in flight.
    // Guards against accidentally restarting already-dead weapons.
    if (!isLaunched || isDead || !m_flightActive) return;
    if (m_flightCheckTimer && !m_flightCheckTimer->isActive())
        m_flightCheckTimer->start(100);
    if (m_updateTimer && !m_updateTimer->isActive())
        m_updateTimer->start(100);
}

// ── Empty stubs — Missile overrides all five ──────────────────────────────────
void      Weapon::missileStart()              {
    int i = 0;
}
void      Weapon::checkFlightState()          {}
void      Weapon::missileUpdate(float)        {}
QVector3D Weapon::calcTargetVector()    const { return QVector3D(); }
void      Weapon::missileEnd()                {}
