#include "flare.h"

Flare::Flare(Hierarchy* h) : Weapon(h)
{
    weaponType     = WeaponType::Flare;
    designation    = "MJU-7";
    length         = 0.146f;
    diameter       = 0.035f;
    totalMass      = 0.203f;
    payloadMass    = 0.18f;
    fuelMass       = 0.0f;
    maxRange       = 1000.0f;
    detonationType = DetonationType::Timed;
}

void Flare::launch()                        { /* eject + ignite cartridge */ }
bool Flare::canEngage(Platform* /*target*/) { return false; /* defensive only */ }

QString Flare::dispensingModeToString() const {
    switch (dispensingMode) {
    case DispensingMode::Single: return "Single";
    case DispensingMode::Burst:  return "Burst";
    case DispensingMode::Salvo:  return "Salvo";
    case DispensingMode::Auto:   return "Auto";
    default:                     return "Burst";
    }
}
void Flare::setDispensingModeFromString(const QString& s) {
    if      (s == "Single") dispensingMode = DispensingMode::Single;
    else if (s == "Salvo")  dispensingMode = DispensingMode::Salvo;
    else if (s == "Auto")   dispensingMode = DispensingMode::Auto;
    else                     dispensingMode = DispensingMode::Burst;
}

QJsonObject Flare::toJson() const
{
    QJsonObject obj = Weapon::toJson();
    obj["weaponTypeName"]       = "Flare";
    obj["burnTemperature"]      = burnTemperature;
    obj["burnDuration"]         = burnDuration;
    obj["peakIrradiance"]       = peakIrradiance;
    obj["spectralMatch"]        = spectralMatch;
    obj["ejectionVelocity"]     = ejectionVelocity;
    obj["ejectionAngle"]        = ejectionAngle;
    obj["ejectionSpread"]       = ejectionSpread;
    obj["dispensingMode"]       = dispensingModeToString();
    obj["burstCount"]           = burstCount;
    obj["burstInterval"]        = burstInterval;
    obj["cartridgesRemaining"]  = cartridgesRemaining;
    obj["breakLockProbability"] = breakLockProbability;
    return obj;
}

void Flare::fromJson(const QJsonObject& obj)
{
    Weapon::fromJson(obj);
    burnTemperature     = obj.value("burnTemperature").toDouble(burnTemperature);
    burnDuration        = obj.value("burnDuration").toDouble(burnDuration);
    peakIrradiance      = obj.value("peakIrradiance").toDouble(peakIrradiance);
    spectralMatch       = obj.value("spectralMatch").toDouble(spectralMatch);
    ejectionVelocity    = obj.value("ejectionVelocity").toDouble(ejectionVelocity);
    ejectionAngle       = obj.value("ejectionAngle").toDouble(ejectionAngle);
    ejectionSpread      = obj.value("ejectionSpread").toDouble(ejectionSpread);
    setDispensingModeFromString(obj.value("dispensingMode").toString());
    burstCount          = obj.value("burstCount").toInt(burstCount);
    burstInterval       = obj.value("burstInterval").toDouble(burstInterval);
    cartridgesRemaining = obj.value("cartridgesRemaining").toInt(cartridgesRemaining);
    breakLockProbability = obj.value("breakLockProbability").toDouble(breakLockProbability);
}
