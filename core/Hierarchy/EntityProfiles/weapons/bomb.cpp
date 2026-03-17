#include "bomb.h"

Bomb::Bomb(Hierarchy* h) : Weapon(h)
{
    weaponType     = WeaponType::Bomb;
    designation    = "GP-Bomb";
    length         = 2.21f;
    diameter       = 0.273f;
    totalMass      = 227.0f;
    payloadMass    = 87.0f;
    fuelMass       = 0.0f;
    maxVelocity    = 350.0f;
    maxRange       = 15000.0f;
    detonationType = DetonationType::Impact;
}

void Bomb::launch()         { /* release from platform */ }
void Bomb::flyToTarget()    { /* TODO: ballistic arc — gravity + drag */ }
void Bomb::checkDetonation(){ /* TODO: impact / delay fuze */ }

QString Bomb::releaseModeToString() const {
    switch (releaseMode) {
    case ReleaseMode::CCIP:    return "CCIP";
    case ReleaseMode::CCRP:    return "CCRP";
    case ReleaseMode::Manual:  return "Manual";
    case ReleaseMode::Lofting: return "Lofting";
    default:                   return "CCIP";
    }
}
void Bomb::setReleaseModeFromString(const QString& s) {
    if      (s == "CCRP")    releaseMode = ReleaseMode::CCRP;
    else if (s == "Manual")  releaseMode = ReleaseMode::Manual;
    else if (s == "Lofting") releaseMode = ReleaseMode::Lofting;
    else                      releaseMode = ReleaseMode::CCIP;
}
QString Bomb::guidanceTypeToString() const {
    return (guidanceType == GuidanceType::InertialGuidance) ? "InertialGuidance" : "Unguided";
}
void Bomb::setGuidanceTypeFromString(const QString& s) {
    guidanceType = (s == "InertialGuidance") ? GuidanceType::InertialGuidance
                                             : GuidanceType::Unguided;
}

QJsonObject Bomb::toJson() const
{
    QJsonObject obj = Weapon::toJson();
    obj["weaponTypeName"]   = "Bomb";
    obj["guidanceType"]     = guidanceTypeToString();
    obj["hasPrecisionKit"]  = hasPrecisionKit;
    obj["cep"]              = cep;
    obj["releaseMode"]      = releaseModeToString();
    obj["dragCoefficient"]  = dragCoefficient;
    obj["terminalVelocity"] = terminalVelocity;
    obj["releaseAltitude"]  = releaseAltitude;
    obj["hasDelayFuze"]     = hasDelayFuze;
    obj["fuzeDelaySeconds"] = fuzeDelaySeconds;
    return obj;
}

void Bomb::fromJson(const QJsonObject& obj)
{
    Weapon::fromJson(obj);
    setGuidanceTypeFromString(obj.value("guidanceType").toString());
    hasPrecisionKit  = obj.value("hasPrecisionKit").toBool(hasPrecisionKit);
    cep              = obj.value("cep").toDouble(cep);
    setReleaseModeFromString(obj.value("releaseMode").toString());
    dragCoefficient  = obj.value("dragCoefficient").toDouble(dragCoefficient);
    terminalVelocity = obj.value("terminalVelocity").toDouble(terminalVelocity);
    releaseAltitude  = obj.value("releaseAltitude").toDouble(releaseAltitude);
    hasDelayFuze     = obj.value("hasDelayFuze").toBool(hasDelayFuze);
    fuzeDelaySeconds = obj.value("fuzeDelaySeconds").toDouble(fuzeDelaySeconds);
}
