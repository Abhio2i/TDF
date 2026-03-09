// #include "weapon.h"
// #include <core/Hierarchy/hierarchy.h>
// #include <core/Debug/console.h>
// #include "core/Hierarchy/Utils/entityutils.h"
// #include <core/GlobalRegistry.h>
// #include <cmath>
// #include <QtMath>
// #include <QVector2D>
// #include <QVector3D>
// #include <QDebug>

// #ifndef M_PI
// #define M_PI 3.14159265358979323846
// #endif

// const float RAD2DEG = 180.0f / M_PI;
// const float DEG2RAD = M_PI / 180.0f;

// Weapon::Weapon(Hierarchy* h) : Entity(h) {
//     type = Constants::EntityType::Weapon;

//     // Initialize default parameter (same pattern as Radio)
//     std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
//     par->Name = "weapon_param";
//     par->type = Constants::ParameterType::FLOAT;
//     par->value = 0.0f;
//     parameters["weapon_param"] = par;

//     // Initialize with safe state
//     safed = true;
//     armed = false;
//     isActive = false;
// }

// void Weapon::spawn() {
//     Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//     emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
// }

// std::vector<std::string> Weapon::getSupportedComponents() {
//     return std::vector<std::string>{};
// }

// void Weapon::addComponent(std::string name) {
//     Console::error("Weapon does not support components: " + name);
// }

// void Weapon::removeComponent(std::string name) {
//     Console::error("Weapon does not support components: " + name);
// }

// // QJsonObject Weapon::getComponent(std::string name) {
// //     Console::error("Weapon does not support components: " + name);
// //     return QJsonObject();
// // }

// QJsonObject Weapon::getComponent(std::string name) {
//     if (name == "_self") {
//         return toJson();  // ✅ Return weapon's JSON data for Inspector
//     }
//     Console::error("Weapon does not support components: " + name);
//     return QJsonObject();
// }

// void Weapon::updateComponent(QString name, const QJsonObject& /*obj*/) {
//     Console::error(name.toStdString() + ": Weapon does not support components");
// }

// // Weapon lifecycle methods
// void Weapon::launch() {
//     if (safed) {
//         Console::error("Cannot launch safed weapon: " + Name);
//         return;
//     }
//     if (!armed) {
//         Console::error("Weapon not armed: " + Name);
//         return;
//     }
//     isActive = true;
//     Console::log("Weapon launched: " + Name);
// }

// void Weapon::disarmWeapon() {
//     safed = true;
//     armed = false;
//     isActive = false;
//     Console::log("Weapon disarmed: " + Name);
// }

// void Weapon::rearmWeapon() {
//     safed = false;
//     armed = true;
//     Console::log("Weapon armed: " + Name);
// }

// // Flight and guidance methods
// void Weapon::flyToTarget() {
//     if (!isActive || !targetEntity) return;

//     if (!parentEntity) return;

//     // Safely get parent platform
//     auto it = root->Platforms->find(parentEntity->ID);
//     if (it == root->Platforms->end()) return;

//     Platform* platform = it->second;
//     if (!platform || !platform->transform) return;

//     // Calculate vector to target using translation() method
//     QVector3D platformPos = platform->transform->translation();
//     QVector3D targetPos = targetEntity->transform->translation();
//     QVector3D direction = (targetPos - platformPos).normalized();

//     // Move towards target
//     float distance = platformPos.distanceToPoint(targetPos);
//     if (distance > 0) {
//         float timeStep = 0.016f;  // 16ms time step
//         float newX = platformPos.x() + direction.x() * maxVelocity * timeStep;
//         float newY = platformPos.y() + direction.y() * maxVelocity * timeStep;
//         float newZ = platformPos.z() + direction.z() * maxVelocity * timeStep;
//     }
// }

// void Weapon::updateGuidance() {
//     if (!isActive || !targetEntity) return;
//     if (!isLocked) return;

//     // Update tracking
//     seekerTrackingRate = 30.0f;
// }

// void Weapon::checkDetonation() {
//     if (!isActive || !targetEntity) return;

//     if (!parentEntity) return;

//     auto it = root->Platforms->find(parentEntity->ID);
//     if (it == root->Platforms->end()) return;

//     Platform* platform = it->second;
//     if (!platform || !platform->transform) return;

//     QVector3D platformPos = platform->transform->translation();
//     QVector3D targetPos = targetEntity->transform->translation();
//     float distance = platformPos.distanceToPoint(targetPos);

//     if (distance < proximityRange) {
//         isActive = false;
//         Console::log("Weapon detonated: " + Name);
//     }
// }

// void Weapon::scan() {
// }

// bool Weapon::canEngage(Platform* target) {
//     if (!target) return false;
//     if (safed || !armed) return false;

//     if (!parentEntity) return false;

//     auto it = root->Platforms->find(parentEntity->ID);
//     if (it == root->Platforms->end()) return false;

//     Platform* platform = it->second;
//     if (!platform || !platform->transform) return false;

//     QVector3D platformPos = platform->transform->translation();
//     QVector3D targetPos = target->transform->translation();
//     float distance = platformPos.distanceToPoint(targetPos);

//     return distance <= maxRange && distance >= minRange;
// }

// float Weapon::calculateImpactTime(Platform* target) {
//     if (!target || maxVelocity == 0) return -1.0f;

//     if (!parentEntity) return -1.0f;

//     auto it = root->Platforms->find(parentEntity->ID);
//     if (it == root->Platforms->end()) return -1.0f;

//     Platform* platform = it->second;
//     if (!platform || !platform->transform) return -1.0f;

//     QVector3D platformPos = platform->transform->translation();
//     QVector3D targetPos = target->transform->translation();
//     float distance = platformPos.distanceToPoint(targetPos);

//     return distance / maxVelocity;
// }

// float Weapon::calculateLaunchPoint() {
//     return maxRange * 0.5f;
// }

// // ============================================================================
// // TYPE-SPECIFIC PARAMETER HELPERS
// // ============================================================================

// QString Weapon::guidanceTypeToString() const {
//     switch(guidanceType) {
//     case GuidanceType::Unguided:          return "Unguided";
//     case GuidanceType::SemiActive:        return "SemiActive";
//     case GuidanceType::FullyActive:       return "FullyActive";
//     case GuidanceType::PassiveInfrared:   return "PassiveInfrared";
//     case GuidanceType::CommandGuided:     return "CommandGuided";
//     case GuidanceType::InertialGuidance:  return "InertialGuidance";
//     default:                              return "Unknown";
//     }
// }

// QString Weapon::propulsionTypeToString() const {
//     switch(propulsionType) {
//     case PropulsionType::SolidRocket:  return "SolidRocket";
//     case PropulsionType::LiquidRocket: return "LiquidRocket";
//     case PropulsionType::Turbofan:     return "Turbofan";
//     case PropulsionType::Ramjet:       return "Ramjet";
//     case PropulsionType::Turboprop:    return "Turboprop";
//     case PropulsionType::Gravity:      return "Gravity";
//     default:                           return "Unknown";
//     }
// }

// QString Weapon::detonationTypeToString() const {
//     switch(detonationType) {
//     case DetonationType::Impact:    return "Impact";
//     case DetonationType::Proximity: return "Proximity";
//     case DetonationType::Timed:     return "Timed";
//     case DetonationType::Command:   return "Command";
//     default:                        return "Unknown";
//     }
// }

// void Weapon::setGuidanceTypeFromString(const QString& str) {
//     if (str == "Unguided")               guidanceType = GuidanceType::Unguided;
//     else if (str == "SemiActive")        guidanceType = GuidanceType::SemiActive;
//     else if (str == "FullyActive")       guidanceType = GuidanceType::FullyActive;
//     else if (str == "PassiveInfrared")   guidanceType = GuidanceType::PassiveInfrared;
//     else if (str == "CommandGuided")     guidanceType = GuidanceType::CommandGuided;
//     else if (str == "InertialGuidance")  guidanceType = GuidanceType::InertialGuidance;
//     else                                 guidanceType = GuidanceType::FullyActive;
// }

// void Weapon::setPropulsionTypeFromString(const QString& str) {
//     if (str == "SolidRocket")       propulsionType = PropulsionType::SolidRocket;
//     else if (str == "LiquidRocket") propulsionType = PropulsionType::LiquidRocket;
//     else if (str == "Turbofan")     propulsionType = PropulsionType::Turbofan;
//     else if (str == "Ramjet")       propulsionType = PropulsionType::Ramjet;
//     else if (str == "Turboprop")    propulsionType = PropulsionType::Turboprop;
//     else if (str == "Gravity")      propulsionType = PropulsionType::Gravity;
//     else                            propulsionType = PropulsionType::SolidRocket;
// }

// void Weapon::setDetonationTypeFromString(const QString& str) {
//     if (str == "Impact")          detonationType = DetonationType::Impact;
//     else if (str == "Proximity")  detonationType = DetonationType::Proximity;
//     else if (str == "Timed")      detonationType = DetonationType::Timed;
//     else if (str == "Command")    detonationType = DetonationType::Command;
//     else                          detonationType = DetonationType::Proximity;
// }

// // ============================================================================
// // SERIALIZATION: toJson() with TYPE-AWARE DISPLAY SECTIONS
// // ============================================================================

// QJsonObject Weapon::toJson() const {
//     QJsonObject obj;
//     obj["name"]       = QString::fromStdString(Name);
//     obj["branch"]     = QString("Entity");
//     obj["id"]         = QString::fromStdString(ID);
//     obj["parent_id"]  = QString::fromStdString(parentID);
//     obj["active"]     = Active;
//     obj["designation"]= QString::fromStdString(designation);
//     obj["armed"]      = armed;
//     obj["safed"]      = safed;
//     obj["isActive"]   = isActive;

//     // ── Weapon type ──────────────────────────────────────────────────────
//     switch(weaponType) {
//     case WeaponType::Missile:   obj["weaponType"] = "Missile";   break;
//     case WeaponType::Bomb:      obj["weaponType"] = "Bomb";      break;
//     case WeaponType::Torpedo:   obj["weaponType"] = "Torpedo";   break;
//     case WeaponType::Artillery: obj["weaponType"] = "Artillery"; break;
//     case WeaponType::Rocket:    obj["weaponType"] = "Rocket";    break;
//     case WeaponType::Flare:     obj["weaponType"] = "Flare";     break;
//     case WeaponType::Chaff:     obj["weaponType"] = "Chaff";     break;
//     default:                    obj["weaponType"] = "Unknown";   break;
//     }

//     // ── Type-specific params (for save/load round-trip) ──────────────────
//     QJsonObject typeSpecificParams;
//     switch(weaponType) {
//     case WeaponType::Missile: {
//         typeSpecificParams["guidanceType"]  = guidanceTypeToString();
//         typeSpecificParams["seekerRange"]   = seekerRange;
//         typeSpecificParams["seekerFOV"]     = seekerFOV;
//         typeSpecificParams["propulsionType"]= propulsionTypeToString();
//         typeSpecificParams["thrustMain"]    = thrustMain;
//         typeSpecificParams["thrustBooster"] = thrustBooster;
//         typeSpecificParams["burnTime"]      = burnTime;
//         typeSpecificParams["detonationType"]= detonationTypeToString();
//         typeSpecificParams["proximityRange"]= proximityRange;
//         typeSpecificParams["blastRadius"]   = blastRadius;
//         typeSpecificParams["maxRange"]      = maxRange;
//         typeSpecificParams["maxVelocity"]   = maxVelocity;
//         break;
//     }
//     case WeaponType::Bomb: {
//         typeSpecificParams["totalMass"]      = totalMass;
//         typeSpecificParams["payloadMass"]    = payloadMass;
//         typeSpecificParams["maxAltitude"]    = maxAltitude;
//         typeSpecificParams["blastRadius"]    = blastRadius;
//         typeSpecificParams["effectiveRadius"]= effectiveRadius;
//         typeSpecificParams["peakPressure"]   = peakPressure;
//         typeSpecificParams["warheadType"]    = QString::fromStdString(warheadType);
//         typeSpecificParams["detonationType"] = detonationTypeToString();
//         typeSpecificParams["timerDelay"]     = timerDelay;
//         break;
//     }
//     case WeaponType::Artillery: {
//         typeSpecificParams["maxRange"]       = maxRange;
//         typeSpecificParams["maxVelocity"]    = maxVelocity;
//         typeSpecificParams["maximumG"]       = maximumG;
//         typeSpecificParams["blastRadius"]    = blastRadius;
//         typeSpecificParams["peakPressure"]   = peakPressure;
//         typeSpecificParams["detonationType"] = detonationTypeToString();
//         typeSpecificParams["rearmTime"]      = rearmTime;
//         break;
//     }
//     case WeaponType::Rocket: {
//         typeSpecificParams["propulsionType"] = propulsionTypeToString();
//         typeSpecificParams["thrustMain"]     = thrustMain;
//         typeSpecificParams["thrustBooster"]  = thrustBooster;
//         typeSpecificParams["burnTime"]       = burnTime;
//         typeSpecificParams["maxRange"]       = maxRange;
//         typeSpecificParams["blastRadius"]    = blastRadius;
//         typeSpecificParams["detonationType"] = detonationTypeToString();
//         break;
//     }
//     case WeaponType::Torpedo: {
//         typeSpecificParams["guidanceType"]  = guidanceTypeToString();
//         typeSpecificParams["maxVelocity"]   = maxVelocity;
//         typeSpecificParams["maxRange"]      = maxRange;
//         typeSpecificParams["maxAltitude"]   = maxAltitude;
//         typeSpecificParams["blastRadius"]   = blastRadius;
//         typeSpecificParams["payloadMass"]   = payloadMass;
//         break;
//     }
//     case WeaponType::Flare: {
//         typeSpecificParams["maxRange"]       = maxRange;
//         typeSpecificParams["maxAltitude"]    = maxAltitude;
//         typeSpecificParams["diameter"]       = diameter;
//         typeSpecificParams["totalMass"]      = totalMass;
//         typeSpecificParams["effectiveRadius"]= effectiveRadius;
//         typeSpecificParams["burnTime"]       = burnTime;
//         break;
//     }
//     case WeaponType::Chaff: {
//         typeSpecificParams["maxRange"]       = maxRange;
//         typeSpecificParams["maxAltitude"]    = maxAltitude;
//         typeSpecificParams["diameter"]       = diameter;
//         typeSpecificParams["totalMass"]      = totalMass;
//         typeSpecificParams["effectiveRadius"]= effectiveRadius;
//         break;
//     }
//     default: break;
//     }
//     obj["typeSpecificParams"] = typeSpecificParams;

//     // ── Parameters map ───────────────────────────────────────────────────
//     QJsonObject paramMap;
//     for (const auto& [key, param] : parameters) {
//         if (param) paramMap[QString::fromStdString(key)] = param->toJson();
//     }
//     QJsonObject parObj;
//     parObj["type"]  = "parameter";
//     parObj["value"] = paramMap;
//     obj["parameters"] = parObj;

//     // ── Specifications section (type-aware) ──────────────────────────────
//     QJsonObject specifications;
//     specifications["type"] = "Section";
//     switch (weaponType) {
//     case WeaponType::Missile:
//     case WeaponType::Rocket:
//         specifications["length"]      = toParm(length,      "m",  0, 100);
//         specifications["diameter"]    = toParm(diameter,    "m",  0, 2);
//         specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
//         specifications["payloadMass"] = toParm(payloadMass, "kg", 0, 5000);
//         specifications["fuelMass"]    = toParm(fuelMass,    "kg", 0, 5000);
//         break;
//     case WeaponType::Bomb:
//         specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
//         specifications["payloadMass"] = toParm(payloadMass, "kg", 0, 5000);
//         specifications["diameter"]    = toParm(diameter,    "m",  0, 2);
//         break;
//     case WeaponType::Torpedo:
//         specifications["length"]      = toParm(length,      "m",  0, 100);
//         specifications["diameter"]    = toParm(diameter,    "m",  0, 2);
//         specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
//         specifications["payloadMass"] = toParm(payloadMass, "kg", 0, 5000);
//         break;
//     case WeaponType::Artillery:
//         specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
//         specifications["diameter"]    = toParm(diameter,    "m",  0, 2);
//         specifications["rearmTime"]   = toParm((float)rearmTime, "s", 0, 3600);
//         break;
//     case WeaponType::Flare:
//     case WeaponType::Chaff:
//         specifications["diameter"]    = toParm(diameter,    "m",  0, 2);
//         specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
//         break;
//     default:
//         specifications["length"]      = toParm(length,      "m",  0, 100);
//         specifications["diameter"]    = toParm(diameter,    "m",  0, 2);
//         specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
//         specifications["payloadMass"] = toParm(payloadMass, "kg", 0, 5000);
//         specifications["fuelMass"]    = toParm(fuelMass,    "kg", 0, 5000);
//         break;
//     }
//     obj["Specifications"] = specifications;

//     // ── Performance section (type-aware) ─────────────────────────────────
//     QJsonObject performance;
//     performance["type"]     = "Section";
//     performance["maxRange"] = toParm(maxRange,    "m",   0, 1000000);
//     performance["maxAltitude"] = toParm(maxAltitude, "m", 0, 50000);
//     switch (weaponType) {
//     case WeaponType::Missile:
//     case WeaponType::Rocket:
//     case WeaponType::Torpedo:
//         performance["maxVelocity"] = toParm(maxVelocity, "m/s", 0, 10000);
//         performance["minVelocity"] = toParm(minVelocity, "m/s", 0, 1000);
//         performance["minRange"]    = toParm(minRange,    "m",   0, 100000);
//         performance["minAltitude"] = toParm(minAltitude, "m",   0, 50000);
//         performance["maximumG"]    = toParm(maximumG,    "G",   0, 100);
//         performance["flightTimeMax"] = toParm(flightTimeMax, "s", 0, 3600);
//         break;
//     case WeaponType::Bomb:
//         // Gravity-guided — no thrust/velocity performance fields
//         break;
//     case WeaponType::Artillery:
//         performance["maxVelocity"] = toParm(maxVelocity, "m/s", 0, 10000);
//         performance["maximumG"]    = toParm(maximumG,    "G",   0, 100);
//         performance["rearmTime"]   = toParm((float)rearmTime, "s", 0, 3600);
//         break;
//     case WeaponType::Flare:
//     case WeaponType::Chaff:
//         performance["effectiveRadius"] = toParm(effectiveRadius, "m", 0, 10000);
//         break;
//     default:
//         performance["maxVelocity"] = toParm(maxVelocity, "m/s", 0, 10000);
//         performance["minRange"]    = toParm(minRange,    "m",   0, 100000);
//         performance["minAltitude"] = toParm(minAltitude, "m",   0, 50000);
//         break;
//     }
//     obj["Performance"] = performance;

//     // ── Guidance section (type-aware) ────────────────────────────────────
//     QJsonObject guidance;
//     guidance["type"] = "Section";
//     switch (weaponType) {
//     case WeaponType::Missile:
//     case WeaponType::Torpedo:
//         guidance["guidanceType"] = guidanceTypeToString();
//         guidance["seekerRange"]  = toParm(seekerRange, "m",   0, 500000);
//         guidance["seekerFOV"]    = toParm(seekerFOV,   "deg", 0, 180);
//         guidance["lockOnRange"]  = toParm(lockOnRange, "m",   0, 500000);
//         guidance["isLocked"]     = isLocked;
//         guidance["seekerTrackingRate"] = toParm(seekerTrackingRate, "deg/s", 0, 180);
//         guidance["seekerLockAccuracy"] = toParm(seekerLockAccuracy, "deg",   0, 10);
//         break;
//     case WeaponType::Bomb:
//     case WeaponType::Artillery:
//     case WeaponType::Rocket:
//     case WeaponType::Flare:
//     case WeaponType::Chaff:
//         // Unguided weapons — show only guidance type label
//         guidance["guidanceType"] = QString("Unguided");
//         break;
//     default:
//         guidance["guidanceType"] = guidanceTypeToString();
//         guidance["seekerRange"]  = toParm(seekerRange, "m",   0, 500000);
//         guidance["seekerFOV"]    = toParm(seekerFOV,   "deg", 0, 180);
//         guidance["isLocked"]     = isLocked;
//         break;
//     }
//     obj["Guidance"] = guidance;

//     // ── Warhead section (type-aware) ──────────────────────────────────────
//     QJsonObject warhead;
//     warhead["type"]          = "Section";
//     warhead["blastRadius"]   = toParm(blastRadius,     "m",   0, 5000);
//     warhead["effectiveRadius"]= toParm(effectiveRadius, "m",  0, 10000);
//     warhead["peakPressure"]  = toParm(peakPressure,    "kPa", 0, 10000);
//     warhead["detonationType"]= detonationTypeToString();
//     switch (weaponType) {
//     case WeaponType::Missile:
//     case WeaponType::Bomb:
//     case WeaponType::Torpedo:
//         warhead["warheadType"] = QString::fromStdString(warheadType);
//         break;
//     default:
//         break;
//     }
//     if (detonationType == DetonationType::Proximity) {
//         warhead["proximityRange"] = toParm(proximityRange, "m", 0, 1000);
//     }
//     if (detonationType == DetonationType::Timed) {
//         warhead["timerDelay"] = toParm(timerDelay, "s", 0, 60);
//     }
//     obj["Warhead"] = warhead;

//     // ── Propulsion section (type-aware) ───────────────────────────────────
//     QJsonObject propulsion;
//     propulsion["type"] = "Section";
//     switch (weaponType) {
//     case WeaponType::Missile:
//     case WeaponType::Rocket:
//         propulsion["propulsionType"]  = propulsionTypeToString();
//         propulsion["thrustMain"]      = toParm(thrustMain,     "N", 0, 5000000);
//         propulsion["thrustBooster"]   = toParm(thrustBooster,  "N", 0, 1000000);
//         propulsion["burnTime"]        = toParm(burnTime,       "s", 0, 300);
//         propulsion["specificImpulse"] = toParm(specificImpulse,"s", 0, 500);
//         break;
//     case WeaponType::Torpedo:
//         propulsion["propulsionType"]  = propulsionTypeToString();
//         propulsion["thrustMain"]      = toParm(thrustMain,    "N", 0, 5000000);
//         propulsion["burnTime"]        = toParm(burnTime,      "s", 0, 300);
//         propulsion["specificImpulse"] = toParm(specificImpulse,"s", 0, 500);
//         break;
//     case WeaponType::Bomb:
//         // Gravity-dropped — no propulsion system
//         propulsion["propulsionType"] = QString("Gravity");
//         break;
//     case WeaponType::Artillery:
//         propulsion["propulsionType"] = QString("Gravity");
//         propulsion["preflightCheck"] = toParm(preflightCheckTime, "s", 0, 300);
//         break;
//     case WeaponType::Flare:
//     case WeaponType::Chaff:
//         propulsion["propulsionType"] = propulsionTypeToString();
//         propulsion["burnTime"]       = toParm(burnTime, "s", 0, 300);
//         break;
//     default:
//         propulsion["propulsionType"]  = propulsionTypeToString();
//         propulsion["thrustMain"]      = toParm(thrustMain,    "N", 0, 5000000);
//         propulsion["burnTime"]        = toParm(burnTime,      "s", 0, 300);
//         propulsion["specificImpulse"] = toParm(specificImpulse,"s", 0, 500);
//         break;
//     }
//     obj["Propulsion"] = propulsion;

//     // ── Launch section (type-aware) ───────────────────────────────────────
//     QJsonObject launch;
//     launch["type"]           = "Section";
//     launch["armed"]          = armed;
//     launch["safed"]          = safed;
//     launch["preflightCheck"] = toParm(preflightCheckTime, "s", 0, 300);
//     switch (weaponType) {
//     case WeaponType::Missile:
//     case WeaponType::Rocket:
//     case WeaponType::Torpedo:
//         launch["launchG"]   = toParm(launchG,  "G", 0, 100);
//         break;
//     case WeaponType::Artillery:
//     case WeaponType::Bomb:
//         launch["rearmTime"] = toParm((float)rearmTime, "s", 0, 3600);
//         break;
//     default:
//         break;
//     }
//     obj["Launch"] = launch;

//     return obj;
// }

// // ============================================================================
// // DESERIALIZATION: fromJson() with TYPE-SPECIFIC PARAMETERS
// // ============================================================================

// void Weapon::fromJson(const QJsonObject& obj) {
//     if (obj.contains("active"))
//         Active = obj["active"].toBool();
//     if (obj.contains("name"))
//         Name = obj["name"].toString().toStdString();
//     if (obj.contains("id"))
//         ID = obj["id"].toString().toStdString();
//     if (obj.contains("parent_id"))
//         parentID = obj["parent_id"].toString().toStdString();
//     if (obj.contains("designation"))
//         designation = obj["designation"].toString().toStdString();
//     if (obj.contains("armed"))
//         armed = obj["armed"].toBool();
//     if (obj.contains("safed"))
//         safed = obj["safed"].toBool();
//     if (obj.contains("isActive"))
//         isActive = obj["isActive"].toBool();

//     // ── Load weapon type first (determines which params to read) ──────────
//     if (obj.contains("weaponType")) {
//         QString typeStr = obj["weaponType"].toString();
//         if      (typeStr == "Missile")   weaponType = WeaponType::Missile;
//         else if (typeStr == "Bomb")      weaponType = WeaponType::Bomb;
//         else if (typeStr == "Torpedo")   weaponType = WeaponType::Torpedo;
//         else if (typeStr == "Artillery") weaponType = WeaponType::Artillery;
//         else if (typeStr == "Rocket")    weaponType = WeaponType::Rocket;
//         else if (typeStr == "Flare")     weaponType = WeaponType::Flare;
//         else if (typeStr == "Chaff")     weaponType = WeaponType::Chaff;
//         else                             weaponType = WeaponType::Missile;
//     }

//     // ── Load type-specific parameters ────────────────────────────────────
//     if (obj.contains("typeSpecificParams") && obj["typeSpecificParams"].isObject()) {
//         QJsonObject typeParams = obj["typeSpecificParams"].toObject();

//         switch(weaponType) {
//         case WeaponType::Missile: {
//             if (typeParams.contains("guidanceType"))
//                 setGuidanceTypeFromString(typeParams["guidanceType"].toString("FullyActive"));
//             if (typeParams.contains("seekerRange"))
//                 seekerRange = typeParams["seekerRange"].toDouble(seekerRange);
//             if (typeParams.contains("seekerFOV"))
//                 seekerFOV = typeParams["seekerFOV"].toDouble(seekerFOV);
//             if (typeParams.contains("propulsionType"))
//                 setPropulsionTypeFromString(typeParams["propulsionType"].toString("SolidRocket"));
//             if (typeParams.contains("thrustMain"))
//                 thrustMain = typeParams["thrustMain"].toDouble(thrustMain);
//             if (typeParams.contains("thrustBooster"))
//                 thrustBooster = typeParams["thrustBooster"].toDouble(thrustBooster);
//             if (typeParams.contains("burnTime"))
//                 burnTime = typeParams["burnTime"].toDouble(burnTime);
//             if (typeParams.contains("detonationType"))
//                 setDetonationTypeFromString(typeParams["detonationType"].toString("Proximity"));
//             if (typeParams.contains("proximityRange"))
//                 proximityRange = typeParams["proximityRange"].toDouble(proximityRange);
//             if (typeParams.contains("blastRadius"))
//                 blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
//             if (typeParams.contains("maxRange"))
//                 maxRange = typeParams["maxRange"].toDouble(maxRange);
//             if (typeParams.contains("maxVelocity"))
//                 maxVelocity = typeParams["maxVelocity"].toDouble(maxVelocity);
//             break;
//         }
//         case WeaponType::Bomb: {
//             if (typeParams.contains("totalMass"))
//                 totalMass = typeParams["totalMass"].toDouble(totalMass);
//             if (typeParams.contains("payloadMass"))
//                 payloadMass = typeParams["payloadMass"].toDouble(payloadMass);
//             if (typeParams.contains("maxAltitude"))
//                 maxAltitude = typeParams["maxAltitude"].toDouble(maxAltitude);
//             if (typeParams.contains("blastRadius"))
//                 blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
//             if (typeParams.contains("effectiveRadius"))
//                 effectiveRadius = typeParams["effectiveRadius"].toDouble(effectiveRadius);
//             if (typeParams.contains("peakPressure"))
//                 peakPressure = typeParams["peakPressure"].toDouble(peakPressure);
//             if (typeParams.contains("warheadType"))
//                 warheadType = typeParams["warheadType"].toString().toStdString();
//             if (typeParams.contains("detonationType"))
//                 setDetonationTypeFromString(typeParams["detonationType"].toString("Impact"));
//             if (typeParams.contains("timerDelay"))
//                 timerDelay = typeParams["timerDelay"].toDouble(timerDelay);
//             // Bombs are unguided
//             guidanceType   = GuidanceType::Unguided;
//             propulsionType = PropulsionType::Gravity;
//             break;
//         }
//         case WeaponType::Artillery: {
//             if (typeParams.contains("maxRange"))
//                 maxRange = typeParams["maxRange"].toDouble(maxRange);
//             if (typeParams.contains("maxVelocity"))
//                 maxVelocity = typeParams["maxVelocity"].toDouble(maxVelocity);
//             if (typeParams.contains("maximumG"))
//                 maximumG = typeParams["maximumG"].toDouble(maximumG);
//             if (typeParams.contains("blastRadius"))
//                 blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
//             if (typeParams.contains("peakPressure"))
//                 peakPressure = typeParams["peakPressure"].toDouble(peakPressure);
//             if (typeParams.contains("detonationType"))
//                 setDetonationTypeFromString(typeParams["detonationType"].toString("Impact"));
//             if (typeParams.contains("rearmTime"))
//                 rearmTime = typeParams["rearmTime"].toInt(rearmTime);
//             guidanceType   = GuidanceType::Unguided;
//             propulsionType = PropulsionType::Gravity;
//             break;
//         }
//         case WeaponType::Rocket: {
//             if (typeParams.contains("propulsionType"))
//                 setPropulsionTypeFromString(typeParams["propulsionType"].toString("SolidRocket"));
//             if (typeParams.contains("thrustMain"))
//                 thrustMain = typeParams["thrustMain"].toDouble(thrustMain);
//             if (typeParams.contains("thrustBooster"))
//                 thrustBooster = typeParams["thrustBooster"].toDouble(thrustBooster);
//             if (typeParams.contains("burnTime"))
//                 burnTime = typeParams["burnTime"].toDouble(burnTime);
//             if (typeParams.contains("maxRange"))
//                 maxRange = typeParams["maxRange"].toDouble(maxRange);
//             if (typeParams.contains("blastRadius"))
//                 blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
//             if (typeParams.contains("detonationType"))
//                 setDetonationTypeFromString(typeParams["detonationType"].toString("Proximity"));
//             guidanceType = GuidanceType::Unguided;
//             break;
//         }
//         case WeaponType::Torpedo: {
//             if (typeParams.contains("guidanceType"))
//                 setGuidanceTypeFromString(typeParams["guidanceType"].toString("FullyActive"));
//             if (typeParams.contains("maxVelocity"))
//                 maxVelocity = typeParams["maxVelocity"].toDouble(maxVelocity);
//             if (typeParams.contains("maxRange"))
//                 maxRange = typeParams["maxRange"].toDouble(maxRange);
//             if (typeParams.contains("maxAltitude"))
//                 maxAltitude = typeParams["maxAltitude"].toDouble(maxAltitude);
//             if (typeParams.contains("blastRadius"))
//                 blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
//             if (typeParams.contains("payloadMass"))
//                 payloadMass = typeParams["payloadMass"].toDouble(payloadMass);
//             break;
//         }
//         case WeaponType::Flare: {
//             if (typeParams.contains("maxRange"))
//                 maxRange = typeParams["maxRange"].toDouble(maxRange);
//             if (typeParams.contains("maxAltitude"))
//                 maxAltitude = typeParams["maxAltitude"].toDouble(maxAltitude);
//             if (typeParams.contains("diameter"))
//                 diameter = typeParams["diameter"].toDouble(diameter);
//             if (typeParams.contains("totalMass"))
//                 totalMass = typeParams["totalMass"].toDouble(totalMass);
//             if (typeParams.contains("effectiveRadius"))
//                 effectiveRadius = typeParams["effectiveRadius"].toDouble(effectiveRadius);
//             if (typeParams.contains("burnTime"))
//                 burnTime = typeParams["burnTime"].toDouble(burnTime);
//             guidanceType = GuidanceType::Unguided;
//             break;
//         }
//         case WeaponType::Chaff: {
//             if (typeParams.contains("maxRange"))
//                 maxRange = typeParams["maxRange"].toDouble(maxRange);
//             if (typeParams.contains("maxAltitude"))
//                 maxAltitude = typeParams["maxAltitude"].toDouble(maxAltitude);
//             if (typeParams.contains("diameter"))
//                 diameter = typeParams["diameter"].toDouble(diameter);
//             if (typeParams.contains("totalMass"))
//                 totalMass = typeParams["totalMass"].toDouble(totalMass);
//             if (typeParams.contains("effectiveRadius"))
//                 effectiveRadius = typeParams["effectiveRadius"].toDouble(effectiveRadius);
//             guidanceType = GuidanceType::Unguided;
//             break;
//         }
//         default: break;
//         }
//     }

//     // ── Parse shared sections (used as fallback / for older files) ────────
//     if (obj.contains("Specifications") && obj["Specifications"].isObject()) {
//         QJsonObject specs = obj["Specifications"].toObject();
//         if (specs.contains("length"))      length      = valueFromParm(specs["length"].toObject());
//         if (specs.contains("diameter"))    diameter    = valueFromParm(specs["diameter"].toObject());
//         if (specs.contains("totalMass"))   totalMass   = valueFromParm(specs["totalMass"].toObject());
//         if (specs.contains("payloadMass")) payloadMass = valueFromParm(specs["payloadMass"].toObject());
//         if (specs.contains("fuelMass"))    fuelMass    = valueFromParm(specs["fuelMass"].toObject());
//     }

//     if (obj.contains("Performance") && obj["Performance"].isObject()) {
//         QJsonObject perf = obj["Performance"].toObject();
//         if (perf.contains("maxVelocity")) maxVelocity = valueFromParm(perf["maxVelocity"].toObject());
//         if (perf.contains("minVelocity")) minVelocity = valueFromParm(perf["minVelocity"].toObject());
//         if (perf.contains("maxRange"))    maxRange    = valueFromParm(perf["maxRange"].toObject());
//         if (perf.contains("minRange"))    minRange    = valueFromParm(perf["minRange"].toObject());
//         if (perf.contains("maxAltitude")) maxAltitude = valueFromParm(perf["maxAltitude"].toObject());
//         if (perf.contains("minAltitude")) minAltitude = valueFromParm(perf["minAltitude"].toObject());
//         if (perf.contains("maximumG"))    maximumG    = valueFromParm(perf["maximumG"].toObject());
//         if (perf.contains("rearmTime"))   rearmTime   = (int)valueFromParm(perf["rearmTime"].toObject());
//     }

//     if (obj.contains("Guidance") && obj["Guidance"].isObject()) {
//         QJsonObject guid = obj["Guidance"].toObject();
//         if (guid.contains("guidanceType"))
//             setGuidanceTypeFromString(guid["guidanceType"].toString());
//         if (guid.contains("seekerRange")) seekerRange = valueFromParm(guid["seekerRange"].toObject());
//         if (guid.contains("seekerFOV"))   seekerFOV   = valueFromParm(guid["seekerFOV"].toObject());
//         if (guid.contains("lockOnRange")) lockOnRange = valueFromParm(guid["lockOnRange"].toObject());
//         if (guid.contains("isLocked"))    isLocked    = guid["isLocked"].toBool();
//     }

//     if (obj.contains("Warhead") && obj["Warhead"].isObject()) {
//         QJsonObject wh = obj["Warhead"].toObject();
//         if (wh.contains("blastRadius"))    blastRadius    = valueFromParm(wh["blastRadius"].toObject());
//         if (wh.contains("effectiveRadius"))effectiveRadius= valueFromParm(wh["effectiveRadius"].toObject());
//         if (wh.contains("peakPressure"))   peakPressure   = valueFromParm(wh["peakPressure"].toObject());
//         if (wh.contains("warheadType"))    warheadType    = wh["warheadType"].toString().toStdString();
//         if (wh.contains("detonationType"))
//             setDetonationTypeFromString(wh["detonationType"].toString());
//         if (wh.contains("proximityRange")) proximityRange = valueFromParm(wh["proximityRange"].toObject());
//         if (wh.contains("timerDelay"))     timerDelay     = valueFromParm(wh["timerDelay"].toObject());
//     }

//     if (obj.contains("Propulsion") && obj["Propulsion"].isObject()) {
//         QJsonObject prop = obj["Propulsion"].toObject();
//         if (prop.contains("propulsionType"))
//             setPropulsionTypeFromString(prop["propulsionType"].toString());
//         if (prop.contains("thrustMain"))      thrustMain      = valueFromParm(prop["thrustMain"].toObject());
//         if (prop.contains("thrustBooster"))   thrustBooster   = valueFromParm(prop["thrustBooster"].toObject());
//         if (prop.contains("burnTime"))        burnTime        = valueFromParm(prop["burnTime"].toObject());
//         if (prop.contains("specificImpulse")) specificImpulse = valueFromParm(prop["specificImpulse"].toObject());
//     }

//     if (obj.contains("Launch") && obj["Launch"].isObject()) {
//         QJsonObject lnch = obj["Launch"].toObject();
//         if (lnch.contains("armed"))          armed              = lnch["armed"].toBool();
//         if (lnch.contains("safed"))          safed              = lnch["safed"].toBool();
//         if (lnch.contains("preflightCheck")) preflightCheckTime = valueFromParm(lnch["preflightCheck"].toObject());
//         if (lnch.contains("launchG"))        launchG            = valueFromParm(lnch["launchG"].toObject());
//         if (lnch.contains("rearmTime"))      rearmTime          = (int)valueFromParm(lnch["rearmTime"].toObject());
//     }

//     // ── Parameters map ────────────────────────────────────────────────────
//     if (obj.contains("parameters")) {
//         QJsonObject parObj = obj["parameters"].toObject();
//         if (parObj.contains("value")) {
//             QJsonObject paramMap = parObj["value"].toObject();
//             for (const QString& key : paramMap.keys()) {
//                 QJsonObject paramObj = paramMap[key].toObject();
//                 std::shared_ptr<Parameter> param = std::make_shared<Parameter>();
//                 param->fromJson(paramObj);
//                 parameters[key.toStdString()] = param;
//             }
//         }
//     }
// }
#include "weapon.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>
#include "core/Hierarchy/Utils/entityutils.h"
#include <core/GlobalRegistry.h>
#include <cmath>
#include <QtMath>
#include <QVector2D>
#include <QVector3D>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float RAD2DEG = 180.0f / M_PI;
const float DEG2RAD = M_PI / 180.0f;

Weapon::Weapon(Hierarchy* h) : Entity(h) {
    type = Constants::EntityType::Weapon;

    // Initialize default parameter (same pattern as Radio)
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "weapon_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["weapon_param"] = par;

    // Initialize with safe state
    safed = true;
    armed = false;
    isActive = false;
}

void Weapon::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

std::vector<std::string> Weapon::getSupportedComponents() {
    return std::vector<std::string>{};
}

void Weapon::addComponent(std::string name) {
    Console::error("Weapon does not support components: " + name);
}

void Weapon::removeComponent(std::string name) {
    Console::error("Weapon does not support components: " + name);
}

// ✅ FIX #1: Return toJson() for "_self" so Inspector displays correct per-weapon data
QJsonObject Weapon::getComponent(std::string name) {
    if (name == "_self") {
        return toJson();  // ✅ FIXED: Return weapon data for Inspector display
    }
    Console::error("Weapon does not support components: " + name);
    return QJsonObject();
}

void Weapon::updateComponent(QString name, const QJsonObject& /*obj*/) {
    Console::error(name.toStdString() + ": Weapon does not support components");
}

// Weapon lifecycle methods
void Weapon::launch() {
    if (safed) {
        Console::error("Cannot launch safed weapon: " + Name);
        return;
    }
    if (!armed) {
        Console::error("Weapon not armed: " + Name);
        return;
    }
    isActive = true;
    Console::log("Weapon launched: " + Name);
}

void Weapon::disarmWeapon() {
    safed = true;
    armed = false;
    isActive = false;
    Console::log("Weapon disarmed: " + Name);
}

void Weapon::rearmWeapon() {
    safed = false;
    armed = true;
    Console::log("Weapon armed: " + Name);
}

// Flight and guidance methods
void Weapon::flyToTarget() {
    if (!isActive || !targetEntity) return;

    if (!parentEntity) return;

    // Safely get parent platform
    auto it = root->Platforms->find(parentEntity->ID);
    if (it == root->Platforms->end()) return;

    Platform* platform = it->second;
    if (!platform || !platform->transform) return;

    // Calculate vector to target using translation() method
    QVector3D platformPos = platform->transform->translation();
    QVector3D targetPos = targetEntity->transform->translation();
    QVector3D direction = (targetPos - platformPos).normalized();

    // Move towards target
    float distance = platformPos.distanceToPoint(targetPos);
    if (distance > 0) {
        float timeStep = 0.016f;  // 16ms time step
        float newX = platformPos.x() + direction.x() * maxVelocity * timeStep;
        float newY = platformPos.y() + direction.y() * maxVelocity * timeStep;
        float newZ = platformPos.z() + direction.z() * maxVelocity * timeStep;
    }
}

void Weapon::updateGuidance() {
    if (!isActive || !targetEntity) return;
    if (!isLocked) return;

    // Update tracking
    seekerTrackingRate = 30.0f;
}

void Weapon::checkDetonation() {
    if (!isActive || !targetEntity) return;

    if (!parentEntity) return;

    auto it = root->Platforms->find(parentEntity->ID);
    if (it == root->Platforms->end()) return;

    Platform* platform = it->second;
    if (!platform || !platform->transform) return;

    QVector3D platformPos = platform->transform->translation();
    QVector3D targetPos = targetEntity->transform->translation();
    float distance = platformPos.distanceToPoint(targetPos);

    if (distance < proximityRange) {
        isActive = false;
        Console::log("Weapon detonated: " + Name);
    }
}

void Weapon::scan() {
}

bool Weapon::canEngage(Platform* target) {
    if (!target) return false;
    if (safed || !armed) return false;

    if (!parentEntity) return false;

    auto it = root->Platforms->find(parentEntity->ID);
    if (it == root->Platforms->end()) return false;

    Platform* platform = it->second;
    if (!platform || !platform->transform) return false;

    QVector3D platformPos = platform->transform->translation();
    QVector3D targetPos = target->transform->translation();
    float distance = platformPos.distanceToPoint(targetPos);

    return distance <= maxRange && distance >= minRange;
}

float Weapon::calculateImpactTime(Platform* target) {
    if (!target || maxVelocity == 0) return -1.0f;

    if (!parentEntity) return -1.0f;

    auto it = root->Platforms->find(parentEntity->ID);
    if (it == root->Platforms->end()) return -1.0f;

    Platform* platform = it->second;
    if (!platform || !platform->transform) return -1.0f;

    QVector3D platformPos = platform->transform->translation();
    QVector3D targetPos = target->transform->translation();
    float distance = platformPos.distanceToPoint(targetPos);

    return distance / maxVelocity;
}

float Weapon::calculateLaunchPoint() {
    return maxRange * 0.5f;
}

// ============================================================================
// ✅ FIX #3: TYPE-SPECIFIC PARAMETER HELPERS - ENUM CONVERSIONS
// ============================================================================

QString Weapon::guidanceTypeToString() const {
    switch(guidanceType) {
    case GuidanceType::Unguided:            return "Unguided";
    case GuidanceType::SemiActive:          return "SemiActive";
    case GuidanceType::FullyActive:         return "FullyActive";
    case GuidanceType::PassiveInfrared:     return "PassiveInfrared";
    case GuidanceType::CommandGuided:       return "CommandGuided";
    case GuidanceType::InertialGuidance:    return "InertialGuidance";
    default:                                 return "Unknown";
    }
}

QString Weapon::propulsionTypeToString() const {
    switch(propulsionType) {
    case PropulsionType::SolidRocket:   return "SolidRocket";
    case PropulsionType::LiquidRocket:  return "LiquidRocket";
    case PropulsionType::Turbofan:      return "Turbofan";
    case PropulsionType::Ramjet:        return "Ramjet";
    case PropulsionType::Turboprop:     return "Turboprop";
    case PropulsionType::Gravity:       return "Gravity";
    default:                             return "Unknown";
    }
}

QString Weapon::detonationTypeToString() const {
    switch(detonationType) {
    case DetonationType::Impact:    return "Impact";
    case DetonationType::Proximity: return "Proximity";
    case DetonationType::Timed:     return "Timed";
    case DetonationType::Command:   return "Command";
    default:                        return "Unknown";
    }
}

void Weapon::setGuidanceTypeFromString(const QString& str) {
    if (str == "Unguided")              guidanceType = GuidanceType::Unguided;
    else if (str == "SemiActive")       guidanceType = GuidanceType::SemiActive;
    else if (str == "FullyActive")      guidanceType = GuidanceType::FullyActive;
    else if (str == "PassiveInfrared")  guidanceType = GuidanceType::PassiveInfrared;
    else if (str == "CommandGuided")    guidanceType = GuidanceType::CommandGuided;
    else if (str == "InertialGuidance") guidanceType = GuidanceType::InertialGuidance;
    else                                 guidanceType = GuidanceType::FullyActive;
}

void Weapon::setPropulsionTypeFromString(const QString& str) {
    if (str == "SolidRocket")   propulsionType = PropulsionType::SolidRocket;
    else if (str == "LiquidRocket") propulsionType = PropulsionType::LiquidRocket;
    else if (str == "Turbofan") propulsionType = PropulsionType::Turbofan;
    else if (str == "Ramjet")   propulsionType = PropulsionType::Ramjet;
    else if (str == "Turboprop") propulsionType = PropulsionType::Turboprop;
    else if (str == "Gravity")  propulsionType = PropulsionType::Gravity;
    else                        propulsionType = PropulsionType::SolidRocket;
}

void Weapon::setDetonationTypeFromString(const QString& str) {
    if (str == "Impact")       detonationType = DetonationType::Impact;
    else if (str == "Proximity") detonationType = DetonationType::Proximity;
    else if (str == "Timed")   detonationType = DetonationType::Timed;
    else if (str == "Command") detonationType = DetonationType::Command;
    else                        detonationType = DetonationType::Proximity;
}

// ============================================================================
// SERIALIZATION: toJson() with TYPE-AWARE DISPLAY SECTIONS
// ============================================================================

QJsonObject Weapon::toJson() const {
    QJsonObject obj;
    obj["name"]       = QString::fromStdString(Name);
    obj["branch"]     = QString("Entity");
    obj["id"]         = QString::fromStdString(ID);
    obj["parent_id"]  = QString::fromStdString(parentID);
    obj["active"]     = Active;
    obj["designation"]= QString::fromStdString(designation);
    obj["armed"]      = armed;
    obj["safed"]      = safed;
    obj["isActive"]   = isActive;

    // ── Weapon type ──────────────────────────────────────────────────────
    switch(weaponType) {
    case WeaponType::Missile:   obj["weaponType"] = "Missile";   break;
    case WeaponType::Bomb:      obj["weaponType"] = "Bomb";      break;
    case WeaponType::Torpedo:   obj["weaponType"] = "Torpedo";   break;
    case WeaponType::Artillery: obj["weaponType"] = "Artillery"; break;
    case WeaponType::Rocket:    obj["weaponType"] = "Rocket";    break;
    case WeaponType::Flare:     obj["weaponType"] = "Flare";     break;
    case WeaponType::Chaff:     obj["weaponType"] = "Chaff";     break;
    default:                    obj["weaponType"] = "Unknown";   break;
    }

    // ── Type-specific params (for save/load round-trip) ──────────────────
    QJsonObject typeSpecificParams;
    switch(weaponType) {
    case WeaponType::Missile: {
        typeSpecificParams["guidanceType"]  = guidanceTypeToString();
        typeSpecificParams["seekerRange"]   = seekerRange;
        typeSpecificParams["seekerFOV"]     = seekerFOV;
        typeSpecificParams["propulsionType"]= propulsionTypeToString();
        typeSpecificParams["thrustMain"]    = thrustMain;
        typeSpecificParams["thrustBooster"] = thrustBooster;
        typeSpecificParams["burnTime"]      = burnTime;
        typeSpecificParams["detonationType"]= detonationTypeToString();
        typeSpecificParams["proximityRange"]= proximityRange;
        typeSpecificParams["blastRadius"]   = blastRadius;
        typeSpecificParams["maxRange"]      = maxRange;
        typeSpecificParams["maxVelocity"]   = maxVelocity;
        break;
    }
    case WeaponType::Bomb: {
        typeSpecificParams["totalMass"]      = totalMass;
        typeSpecificParams["payloadMass"]    = payloadMass;
        typeSpecificParams["maxAltitude"]    = maxAltitude;
        typeSpecificParams["blastRadius"]    = blastRadius;
        typeSpecificParams["effectiveRadius"]= effectiveRadius;
        typeSpecificParams["peakPressure"]   = peakPressure;
        typeSpecificParams["warheadType"]    = QString::fromStdString(warheadType);
        typeSpecificParams["detonationType"] = detonationTypeToString();
        typeSpecificParams["timerDelay"]     = timerDelay;
        break;
    }
    case WeaponType::Artillery: {
        typeSpecificParams["maxRange"]       = maxRange;
        typeSpecificParams["maxVelocity"]    = maxVelocity;
        typeSpecificParams["maximumG"]       = maximumG;
        typeSpecificParams["blastRadius"]    = blastRadius;
        typeSpecificParams["peakPressure"]   = peakPressure;
        typeSpecificParams["detonationType"] = detonationTypeToString();
        typeSpecificParams["rearmTime"]      = rearmTime;
        break;
    }
    case WeaponType::Rocket: {
        typeSpecificParams["propulsionType"] = propulsionTypeToString();
        typeSpecificParams["thrustMain"]     = thrustMain;
        typeSpecificParams["thrustBooster"]  = thrustBooster;
        typeSpecificParams["burnTime"]       = burnTime;
        typeSpecificParams["maxRange"]       = maxRange;
        typeSpecificParams["blastRadius"]    = blastRadius;
        typeSpecificParams["detonationType"] = detonationTypeToString();
        break;
    }
    case WeaponType::Torpedo: {
        typeSpecificParams["guidanceType"]  = guidanceTypeToString();
        typeSpecificParams["maxVelocity"]   = maxVelocity;
        typeSpecificParams["maxRange"]      = maxRange;
        typeSpecificParams["maxAltitude"]   = maxAltitude;
        typeSpecificParams["blastRadius"]   = blastRadius;
        typeSpecificParams["payloadMass"]   = payloadMass;
        break;
    }
    case WeaponType::Flare: {
        typeSpecificParams["maxRange"]       = maxRange;
        typeSpecificParams["maxAltitude"]    = maxAltitude;
        typeSpecificParams["diameter"]       = diameter;
        typeSpecificParams["totalMass"]      = totalMass;
        typeSpecificParams["effectiveRadius"]= effectiveRadius;
        typeSpecificParams["burnTime"]       = burnTime;
        break;
    }
    case WeaponType::Chaff: {
        typeSpecificParams["maxRange"]       = maxRange;
        typeSpecificParams["maxAltitude"]    = maxAltitude;
        typeSpecificParams["diameter"]       = diameter;
        typeSpecificParams["totalMass"]      = totalMass;
        typeSpecificParams["effectiveRadius"]= effectiveRadius;
        break;
    }
    default: break;
    }
    obj["typeSpecificParams"] = typeSpecificParams;

    // ── Parameters map ───────────────────────────────────────────────────
    QJsonObject paramMap;
    for (const auto& [key, param] : parameters) {
        if (param) paramMap[QString::fromStdString(key)] = param->toJson();
    }
    QJsonObject parObj;
    parObj["type"]  = "parameter";
    parObj["value"] = paramMap;
    obj["parameters"] = parObj;

    // ── Specifications section (type-aware) ──────────────────────────────
    QJsonObject specifications;
    specifications["type"] = "Section";
    switch (weaponType) {
    case WeaponType::Missile:
    case WeaponType::Rocket:
        specifications["length"]      = toParm(length,      "m",  0, 100);
        specifications["diameter"]    = toParm(diameter,    "m",  0, 2);
        specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
        specifications["payloadMass"] = toParm(payloadMass, "kg", 0, 5000);
        specifications["fuelMass"]    = toParm(fuelMass,    "kg", 0, 5000);
        break;
    case WeaponType::Bomb:
        specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
        specifications["payloadMass"] = toParm(payloadMass, "kg", 0, 5000);
        specifications["diameter"]    = toParm(diameter,    "m",  0, 2);
        break;
    case WeaponType::Torpedo:
        specifications["length"]      = toParm(length,      "m",  0, 100);
        specifications["diameter"]    = toParm(diameter,    "m",  0, 2);
        specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
        specifications["payloadMass"] = toParm(payloadMass, "kg", 0, 5000);
        break;
    case WeaponType::Artillery:
        specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 10000);
        break;
    case WeaponType::Flare:
    case WeaponType::Chaff:
        specifications["diameter"]    = toParm(diameter,    "m", 0, 2);
        specifications["totalMass"]   = toParm(totalMass,   "kg", 0, 1000);
        break;
    default: break;
    }
    obj["Specifications"] = specifications;

    // ── Performance section ──────────────────────────────────────────────
    QJsonObject performance;
    performance["type"] = "Section";
    performance["maxVelocity"] = toParm(maxVelocity, "m/s", 0, 10000);
    performance["minVelocity"] = toParm(minVelocity, "m/s", 0, 5000);
    performance["maxRange"]    = toParm(maxRange,    "m",   0, 500000);
    performance["minRange"]    = toParm(minRange,    "m",   0, 50000);
    performance["maxAltitude"] = toParm(maxAltitude, "m",   0, 50000);
    performance["minAltitude"] = toParm(minAltitude, "m",   0, 30000);
    performance["maximumG"]    = toParm(maximumG,    "G",   0, 50);
    obj["Performance"] = performance;

    // ── Guidance section ─────────────────────────────────────────────────
    QJsonObject guidance;
    guidance["type"]        = "Section";
    guidance["guidanceType"]= guidanceTypeToString();
    guidance["seekerRange"] = toParm(seekerRange, "m", 0, 500000);
    guidance["seekerFOV"]   = toParm(seekerFOV,   "deg", 0, 180);
    guidance["lockOnRange"] = toParm(lockOnRange, "m", 0, 500000);
    guidance["isLocked"]    = isLocked;
    obj["Guidance"] = guidance;

    // ── Warhead section ──────────────────────────────────────────────────
    QJsonObject warhead;
    warhead["type"]           = "Section";
    warhead["blastRadius"]    = toParm(blastRadius,     "m",  0, 5000);
    warhead["effectiveRadius"]= toParm(effectiveRadius, "m",  0, 5000);
    warhead["peakPressure"]   = toParm(peakPressure,    "kPa", 0, 10000);
    warhead["warheadType"]    = QString::fromStdString(warheadType);
    warhead["detonationType"] = detonationTypeToString();
    warhead["proximityRange"] = toParm(proximityRange, "m", 0, 1000);
    warhead["timerDelay"]     = toParm(timerDelay, "s", 0, 3600);
    obj["Warhead"] = warhead;

    // ── Propulsion section ───────────────────────────────────────────────
    QJsonObject propulsion;
    propulsion["type"]          = "Section";
    propulsion["propulsionType"]= propulsionTypeToString();
    propulsion["thrustMain"]    = toParm(thrustMain,      "N", 0, 1000000);
    propulsion["thrustBooster"] = toParm(thrustBooster,    "N", 0, 500000);
    propulsion["burnTime"]      = toParm(burnTime,         "s", 0, 3600);
    propulsion["specificImpulse"]= toParm(specificImpulse, "s", 0, 500);
    obj["Propulsion"] = propulsion;

    // ── Launch section ───────────────────────────────────────────────────
    QJsonObject launch;
    launch["type"]            = "Section";
    launch["armed"]           = armed;
    launch["safed"]           = safed;
    launch["preflightCheck"]  = toParm(preflightCheckTime, "s", 0, 300);
    launch["launchG"]         = toParm(launchG,    "G", 0, 50);
    launch["rearmTime"]       = rearmTime;
    obj["Launch"] = launch;

    return obj;
}

void Weapon::fromJson(const QJsonObject& obj) {
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();
    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("designation"))
        designation = obj["designation"].toString().toStdString();
    if (obj.contains("armed"))
        armed = obj["armed"].toBool();
    if (obj.contains("safed"))
        safed = obj["safed"].toBool();
    if (obj.contains("isActive"))
        isActive = obj["isActive"].toBool();

    // ── Parse weapon type ────────────────────────────────────────────────
    if (obj.contains("weaponType")) {
        QString typeStr = obj["weaponType"].toString();
        if (typeStr == "Missile")       weaponType = WeaponType::Missile;
        else if (typeStr == "Bomb")     weaponType = WeaponType::Bomb;
        else if (typeStr == "Torpedo")  weaponType = WeaponType::Torpedo;
        else if (typeStr == "Artillery")weaponType = WeaponType::Artillery;
        else if (typeStr == "Rocket")   weaponType = WeaponType::Rocket;
        else if (typeStr == "Flare")    weaponType = WeaponType::Flare;
        else if (typeStr == "Chaff")    weaponType = WeaponType::Chaff;
        else                            weaponType = WeaponType::Missile;
    }

    // ── Parse type-specific params ───────────────────────────────────────
    if (obj.contains("typeSpecificParams")) {
        QJsonObject typeParams = obj["typeSpecificParams"].toObject();

        switch (weaponType) {
        case WeaponType::Missile: {
            if (typeParams.contains("guidanceType"))
                setGuidanceTypeFromString(typeParams["guidanceType"].toString("FullyActive"));
            if (typeParams.contains("seekerRange"))
                seekerRange = typeParams["seekerRange"].toDouble(seekerRange);
            if (typeParams.contains("seekerFOV"))
                seekerFOV = typeParams["seekerFOV"].toDouble(seekerFOV);
            if (typeParams.contains("propulsionType"))
                setPropulsionTypeFromString(typeParams["propulsionType"].toString("SolidRocket"));
            if (typeParams.contains("thrustMain"))
                thrustMain = typeParams["thrustMain"].toDouble(thrustMain);
            if (typeParams.contains("thrustBooster"))
                thrustBooster = typeParams["thrustBooster"].toDouble(thrustBooster);
            if (typeParams.contains("burnTime"))
                burnTime = typeParams["burnTime"].toDouble(burnTime);
            if (typeParams.contains("detonationType"))
                setDetonationTypeFromString(typeParams["detonationType"].toString("Proximity"));
            if (typeParams.contains("proximityRange"))
                proximityRange = typeParams["proximityRange"].toDouble(proximityRange);
            if (typeParams.contains("blastRadius"))
                blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
            if (typeParams.contains("maxRange"))
                maxRange = typeParams["maxRange"].toDouble(maxRange);
            if (typeParams.contains("maxVelocity"))
                maxVelocity = typeParams["maxVelocity"].toDouble(maxVelocity);
            break;
        }
        case WeaponType::Bomb: {
            if (typeParams.contains("totalMass"))
                totalMass = typeParams["totalMass"].toDouble(totalMass);
            if (typeParams.contains("payloadMass"))
                payloadMass = typeParams["payloadMass"].toDouble(payloadMass);
            if (typeParams.contains("maxAltitude"))
                maxAltitude = typeParams["maxAltitude"].toDouble(maxAltitude);
            if (typeParams.contains("blastRadius"))
                blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
            if (typeParams.contains("effectiveRadius"))
                effectiveRadius = typeParams["effectiveRadius"].toDouble(effectiveRadius);
            if (typeParams.contains("peakPressure"))
                peakPressure = typeParams["peakPressure"].toDouble(peakPressure);
            if (typeParams.contains("warheadType"))
                warheadType = typeParams["warheadType"].toString().toStdString();
            if (typeParams.contains("detonationType"))
                setDetonationTypeFromString(typeParams["detonationType"].toString("Impact"));
            if (typeParams.contains("timerDelay"))
                timerDelay = typeParams["timerDelay"].toDouble(timerDelay);
            // Bombs are unguided
            guidanceType   = GuidanceType::Unguided;
            propulsionType = PropulsionType::Gravity;
            break;
        }
        case WeaponType::Artillery: {
            if (typeParams.contains("maxRange"))
                maxRange = typeParams["maxRange"].toDouble(maxRange);
            if (typeParams.contains("maxVelocity"))
                maxVelocity = typeParams["maxVelocity"].toDouble(maxVelocity);
            if (typeParams.contains("maximumG"))
                maximumG = typeParams["maximumG"].toDouble(maximumG);
            if (typeParams.contains("blastRadius"))
                blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
            if (typeParams.contains("peakPressure"))
                peakPressure = typeParams["peakPressure"].toDouble(peakPressure);
            if (typeParams.contains("detonationType"))
                setDetonationTypeFromString(typeParams["detonationType"].toString("Impact"));
            if (typeParams.contains("rearmTime"))
                rearmTime = typeParams["rearmTime"].toInt(rearmTime);
            guidanceType   = GuidanceType::Unguided;
            propulsionType = PropulsionType::Gravity;
            break;
        }
        case WeaponType::Rocket: {
            if (typeParams.contains("propulsionType"))
                setPropulsionTypeFromString(typeParams["propulsionType"].toString("SolidRocket"));
            if (typeParams.contains("thrustMain"))
                thrustMain = typeParams["thrustMain"].toDouble(thrustMain);
            if (typeParams.contains("thrustBooster"))
                thrustBooster = typeParams["thrustBooster"].toDouble(thrustBooster);
            if (typeParams.contains("burnTime"))
                burnTime = typeParams["burnTime"].toDouble(burnTime);
            if (typeParams.contains("maxRange"))
                maxRange = typeParams["maxRange"].toDouble(maxRange);
            if (typeParams.contains("blastRadius"))
                blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
            if (typeParams.contains("detonationType"))
                setDetonationTypeFromString(typeParams["detonationType"].toString("Proximity"));
            guidanceType = GuidanceType::Unguided;
            break;
        }
        case WeaponType::Torpedo: {
            if (typeParams.contains("guidanceType"))
                setGuidanceTypeFromString(typeParams["guidanceType"].toString("FullyActive"));
            if (typeParams.contains("maxVelocity"))
                maxVelocity = typeParams["maxVelocity"].toDouble(maxVelocity);
            if (typeParams.contains("maxRange"))
                maxRange = typeParams["maxRange"].toDouble(maxRange);
            if (typeParams.contains("maxAltitude"))
                maxAltitude = typeParams["maxAltitude"].toDouble(maxAltitude);
            if (typeParams.contains("blastRadius"))
                blastRadius = typeParams["blastRadius"].toDouble(blastRadius);
            if (typeParams.contains("payloadMass"))
                payloadMass = typeParams["payloadMass"].toDouble(payloadMass);
            break;
        }
        case WeaponType::Flare: {
            if (typeParams.contains("maxRange"))
                maxRange = typeParams["maxRange"].toDouble(maxRange);
            if (typeParams.contains("maxAltitude"))
                maxAltitude = typeParams["maxAltitude"].toDouble(maxAltitude);
            if (typeParams.contains("diameter"))
                diameter = typeParams["diameter"].toDouble(diameter);
            if (typeParams.contains("totalMass"))
                totalMass = typeParams["totalMass"].toDouble(totalMass);
            if (typeParams.contains("effectiveRadius"))
                effectiveRadius = typeParams["effectiveRadius"].toDouble(effectiveRadius);
            if (typeParams.contains("burnTime"))
                burnTime = typeParams["burnTime"].toDouble(burnTime);
            guidanceType = GuidanceType::Unguided;
            break;
        }
        case WeaponType::Chaff: {
            if (typeParams.contains("maxRange"))
                maxRange = typeParams["maxRange"].toDouble(maxRange);
            if (typeParams.contains("maxAltitude"))
                maxAltitude = typeParams["maxAltitude"].toDouble(maxAltitude);
            if (typeParams.contains("diameter"))
                diameter = typeParams["diameter"].toDouble(diameter);
            if (typeParams.contains("totalMass"))
                totalMass = typeParams["totalMass"].toDouble(totalMass);
            if (typeParams.contains("effectiveRadius"))
                effectiveRadius = typeParams["effectiveRadius"].toDouble(effectiveRadius);
            guidanceType = GuidanceType::Unguided;
            break;
        }
        default: break;
        }
    }

    // ── Parse shared sections (used as fallback / for older files) ────────
    if (obj.contains("Specifications") && obj["Specifications"].isObject()) {
        QJsonObject specs = obj["Specifications"].toObject();
        if (specs.contains("length"))      length      = valueFromParm(specs["length"].toObject());
        if (specs.contains("diameter"))    diameter    = valueFromParm(specs["diameter"].toObject());
        if (specs.contains("totalMass"))   totalMass   = valueFromParm(specs["totalMass"].toObject());
        if (specs.contains("payloadMass")) payloadMass = valueFromParm(specs["payloadMass"].toObject());
        if (specs.contains("fuelMass"))    fuelMass    = valueFromParm(specs["fuelMass"].toObject());
    }

    if (obj.contains("Performance") && obj["Performance"].isObject()) {
        QJsonObject perf = obj["Performance"].toObject();
        if (perf.contains("maxVelocity")) maxVelocity = valueFromParm(perf["maxVelocity"].toObject());
        if (perf.contains("minVelocity")) minVelocity = valueFromParm(perf["minVelocity"].toObject());
        if (perf.contains("maxRange"))    maxRange    = valueFromParm(perf["maxRange"].toObject());
        if (perf.contains("minRange"))    minRange    = valueFromParm(perf["minRange"].toObject());
        if (perf.contains("maxAltitude")) maxAltitude = valueFromParm(perf["maxAltitude"].toObject());
        if (perf.contains("minAltitude")) minAltitude = valueFromParm(perf["minAltitude"].toObject());
        if (perf.contains("maximumG"))    maximumG    = valueFromParm(perf["maximumG"].toObject());
        if (perf.contains("rearmTime"))   rearmTime   = (int)valueFromParm(perf["rearmTime"].toObject());
    }

    if (obj.contains("Guidance") && obj["Guidance"].isObject()) {
        QJsonObject guid = obj["Guidance"].toObject();
        if (guid.contains("guidanceType"))
            setGuidanceTypeFromString(guid["guidanceType"].toString());
        if (guid.contains("seekerRange")) seekerRange = valueFromParm(guid["seekerRange"].toObject());
        if (guid.contains("seekerFOV"))   seekerFOV   = valueFromParm(guid["seekerFOV"].toObject());
        if (guid.contains("lockOnRange")) lockOnRange = valueFromParm(guid["lockOnRange"].toObject());
        if (guid.contains("isLocked"))    isLocked    = guid["isLocked"].toBool();
    }

    if (obj.contains("Warhead") && obj["Warhead"].isObject()) {
        QJsonObject wh = obj["Warhead"].toObject();
        if (wh.contains("blastRadius"))    blastRadius    = valueFromParm(wh["blastRadius"].toObject());
        if (wh.contains("effectiveRadius"))effectiveRadius= valueFromParm(wh["effectiveRadius"].toObject());
        if (wh.contains("peakPressure"))   peakPressure   = valueFromParm(wh["peakPressure"].toObject());
        if (wh.contains("warheadType"))    warheadType    = wh["warheadType"].toString().toStdString();
        if (wh.contains("detonationType"))
            setDetonationTypeFromString(wh["detonationType"].toString());
        if (wh.contains("proximityRange")) proximityRange = valueFromParm(wh["proximityRange"].toObject());
        if (wh.contains("timerDelay"))     timerDelay     = valueFromParm(wh["timerDelay"].toObject());
    }

    if (obj.contains("Propulsion") && obj["Propulsion"].isObject()) {
        QJsonObject prop = obj["Propulsion"].toObject();
        if (prop.contains("propulsionType"))
            setPropulsionTypeFromString(prop["propulsionType"].toString());
        if (prop.contains("thrustMain"))      thrustMain      = valueFromParm(prop["thrustMain"].toObject());
        if (prop.contains("thrustBooster"))   thrustBooster   = valueFromParm(prop["thrustBooster"].toObject());
        if (prop.contains("burnTime"))        burnTime        = valueFromParm(prop["burnTime"].toObject());
        if (prop.contains("specificImpulse")) specificImpulse = valueFromParm(prop["specificImpulse"].toObject());
    }

    if (obj.contains("Launch") && obj["Launch"].isObject()) {
        QJsonObject lnch = obj["Launch"].toObject();
        if (lnch.contains("armed"))          armed              = lnch["armed"].toBool();
        if (lnch.contains("safed"))          safed              = lnch["safed"].toBool();
        if (lnch.contains("preflightCheck")) preflightCheckTime = valueFromParm(lnch["preflightCheck"].toObject());
        if (lnch.contains("launchG"))        launchG            = valueFromParm(lnch["launchG"].toObject());
        if (lnch.contains("rearmTime"))      rearmTime          = (int)valueFromParm(lnch["rearmTime"].toObject());
    }

    // ── Parameters map ────────────────────────────────────────────────────
    if (obj.contains("parameters")) {
        QJsonObject parObj = obj["parameters"].toObject();
        if (parObj.contains("value")) {
            QJsonObject paramMap = parObj["value"].toObject();
            for (const QString& key : paramMap.keys()) {
                QJsonObject paramObj = paramMap[key].toObject();
                std::shared_ptr<Parameter> param = std::make_shared<Parameter>();
                param->fromJson(paramObj);
                parameters[key.toStdString()] = param;
            }
        }
    }
}
