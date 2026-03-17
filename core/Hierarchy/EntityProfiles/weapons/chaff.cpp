#include "chaff.h"

Chaff::Chaff(Hierarchy* h) : Weapon(h)
{
    weaponType     = WeaponType::Chaff;
    designation    = "RR-170";
    length         = 0.146f;
    diameter       = 0.035f;
    totalMass      = 0.18f;
    payloadMass    = 0.16f;
    fuelMass       = 0.0f;
    maxRange       = 500.0f;
    detonationType = DetonationType::Timed;
}

void Chaff::launch()                        { /* eject + bloom */ }
bool Chaff::canEngage(Platform* /*target*/) { return false; /* defensive only */ }

QString Chaff::chaffModeToString() const {
    return (chaffMode == ChaffMode::Saturation) ? "Saturation" : "Seduction";
}
void Chaff::setChaffModeFromString(const QString& s) {
    chaffMode = (s == "Saturation") ? ChaffMode::Saturation : ChaffMode::Seduction;
}
QString Chaff::dispensingModeToString() const {
    switch (dispensingMode) {
    case DispensingMode::Single: return "Single";
    case DispensingMode::Burst:  return "Burst";
    case DispensingMode::Salvo:  return "Salvo";
    case DispensingMode::Auto:   return "Auto";
    default:                     return "Burst";
    }
}
void Chaff::setDispensingModeFromString(const QString& s) {
    if      (s == "Single") dispensingMode = DispensingMode::Single;
    else if (s == "Salvo")  dispensingMode = DispensingMode::Salvo;
    else if (s == "Auto")   dispensingMode = DispensingMode::Auto;
    else                     dispensingMode = DispensingMode::Burst;
}

QJsonObject Chaff::toJson() const
{
    QJsonObject obj = Weapon::toJson();
    obj["weaponTypeName"]       = "Chaff";
    obj["dipoleCount"]          = dipoleCount;
    obj["radarFrequencyHz"]     = radarFrequencyHz;
    obj["bloomRCS"]             = bloomRCS;
    obj["bloomRadius"]          = bloomRadius;
    obj["bloomGrowthRate"]      = bloomGrowthRate;
    obj["bloomDecayTime"]       = bloomDecayTime;
    obj["chaffMode"]            = chaffModeToString();
    obj["breakLockProbability"] = breakLockProbability;
    obj["saturationClutterDb"]  = saturationClutterDb;
    obj["dispensingMode"]       = dispensingModeToString();
    obj["burstCount"]           = burstCount;
    obj["burstInterval"]        = burstInterval;
    obj["cartridgesRemaining"]  = cartridgesRemaining;
    obj["ejectionVelocity"]     = ejectionVelocity;
    obj["ejectionAngle"]        = ejectionAngle;
    obj["ejectionSpread"]       = ejectionSpread;
    return obj;
}

void Chaff::fromJson(const QJsonObject& obj)
{
    Weapon::fromJson(obj);
    dipoleCount          = obj.value("dipoleCount").toDouble(dipoleCount);
    radarFrequencyHz     = obj.value("radarFrequencyHz").toDouble(radarFrequencyHz);
    bloomRCS             = obj.value("bloomRCS").toDouble(bloomRCS);
    bloomRadius          = obj.value("bloomRadius").toDouble(bloomRadius);
    bloomGrowthRate      = obj.value("bloomGrowthRate").toDouble(bloomGrowthRate);
    bloomDecayTime       = obj.value("bloomDecayTime").toDouble(bloomDecayTime);
    setChaffModeFromString(obj.value("chaffMode").toString());
    breakLockProbability = obj.value("breakLockProbability").toDouble(breakLockProbability);
    saturationClutterDb  = obj.value("saturationClutterDb").toDouble(saturationClutterDb);
    setDispensingModeFromString(obj.value("dispensingMode").toString());
    burstCount           = obj.value("burstCount").toInt(burstCount);
    burstInterval        = obj.value("burstInterval").toDouble(burstInterval);
    cartridgesRemaining  = obj.value("cartridgesRemaining").toInt(cartridgesRemaining);
    ejectionVelocity     = obj.value("ejectionVelocity").toDouble(ejectionVelocity);
    ejectionAngle        = obj.value("ejectionAngle").toDouble(ejectionAngle);
    ejectionSpread       = obj.value("ejectionSpread").toDouble(ejectionSpread);
}
