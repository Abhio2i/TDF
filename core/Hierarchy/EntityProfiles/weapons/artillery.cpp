#include "artillery.h"

Artillery::Artillery(Hierarchy* h) : Weapon(h)
{
    weaponType     = WeaponType::Artillery;
    designation    = "155mm-HE";
    length         = 0.897f;
    diameter       = 0.155f;
    totalMass      = 43.5f;
    payloadMass    = 10.8f;
    fuelMass       = 0.0f;
    maxVelocity    = 827.0f;
    maxRange       = 30000.0f;
    detonationType = DetonationType::Impact;
}

void  Artillery::launch()                              { /* fire round */ }
void  Artillery::flyToTarget()                         { /* TODO: high-arc ballistic */ }
void  Artillery::checkDetonation()                     { /* TODO: fuze logic */ }
float Artillery::calculateImpactTime(Platform* /*t*/)  { return 0.0f; /* TODO */ }

void Artillery::Update(){

}

QString Artillery::shellTypeToString() const {
    switch (shellType) {
    case ShellType::HE:           return "HE";
    case ShellType::HEAT:         return "HEAT";
    case ShellType::HESH:         return "HESH";
    case ShellType::Illumination: return "Illumination";
    case ShellType::Smoke:        return "Smoke";
    case ShellType::Cluster:      return "Cluster";
    case ShellType::Excalibur:    return "Excalibur";
    case ShellType::DPICM:        return "DPICM";
    default:                      return "HE";
    }
}
void Artillery::setShellTypeFromString(const QString& s) {
    if      (s == "HEAT")         shellType = ShellType::HEAT;
    else if (s == "HESH")         shellType = ShellType::HESH;
    else if (s == "Illumination") shellType = ShellType::Illumination;
    else if (s == "Smoke")        shellType = ShellType::Smoke;
    else if (s == "Cluster")      shellType = ShellType::Cluster;
    else if (s == "Excalibur")    shellType = ShellType::Excalibur;
    else if (s == "DPICM")        shellType = ShellType::DPICM;
    else                           shellType = ShellType::HE;
}
QString Artillery::fuzeTypeToString() const {
    switch (fuzeType) {
    case FuzeType::Delay:              return "Delay";
    case FuzeType::ProximityAirburst:  return "ProximityAirburst";
    case FuzeType::TimedAirburst:      return "TimedAirburst";
    default:                           return "PointDetonating";
    }
}
void Artillery::setFuzeTypeFromString(const QString& s) {
    if      (s == "Delay")             fuzeType = FuzeType::Delay;
    else if (s == "ProximityAirburst") fuzeType = FuzeType::ProximityAirburst;
    else if (s == "TimedAirburst")     fuzeType = FuzeType::TimedAirburst;
    else                                fuzeType = FuzeType::PointDetonating;
}

QJsonObject Artillery::toJson() const
{
    QJsonObject obj = Weapon::toJson();
    obj["weaponTypeName"]    = "Artillery";
    obj["muzzleVelocity"]    = muzzleVelocity;
    obj["calibre"]           = calibre;
    obj["barrelLength"]      = barrelLength;
    obj["propellantCharge"]  = propellantCharge;
    obj["launchAngle"]       = launchAngle;
    obj["dragCoefficient"]   = dragCoefficient;
    obj["spinStabilised"]    = spinStabilised;
    obj["shellType"]         = shellTypeToString();
    obj["hasGpsGuidance"]    = hasGpsGuidance;
    obj["cep"]               = cep;
    obj["mrsiEnabled"]       = mrsiEnabled;
    obj["mrsiRounds"]        = mrsiRounds;
    obj["mrsiFiringInterval"] = mrsiFiringInterval;
    obj["fuzeType"]          = fuzeTypeToString();
    obj["airburstHeight"]    = airburstHeight;
    return obj;
}

void Artillery::fromJson(const QJsonObject& obj)
{
    Weapon::fromJson(obj);
    muzzleVelocity    = obj.value("muzzleVelocity").toDouble(muzzleVelocity);
    calibre           = obj.value("calibre").toDouble(calibre);
    barrelLength      = obj.value("barrelLength").toDouble(barrelLength);
    propellantCharge  = obj.value("propellantCharge").toInt(propellantCharge);
    launchAngle       = obj.value("launchAngle").toDouble(launchAngle);
    dragCoefficient   = obj.value("dragCoefficient").toDouble(dragCoefficient);
    spinStabilised    = obj.value("spinStabilised").toBool(spinStabilised);
    setShellTypeFromString(obj.value("shellType").toString());
    hasGpsGuidance    = obj.value("hasGpsGuidance").toBool(hasGpsGuidance);
    cep               = obj.value("cep").toDouble(cep);
    mrsiEnabled       = obj.value("mrsiEnabled").toBool(mrsiEnabled);
    mrsiRounds        = obj.value("mrsiRounds").toInt(mrsiRounds);
    mrsiFiringInterval = obj.value("mrsiFiringInterval").toDouble(mrsiFiringInterval);
    setFuzeTypeFromString(obj.value("fuzeType").toString());
    airburstHeight    = obj.value("airburstHeight").toDouble(airburstHeight);
}
