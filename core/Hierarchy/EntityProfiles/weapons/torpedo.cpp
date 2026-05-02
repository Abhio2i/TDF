#include "torpedo.h"

Torpedo::Torpedo(Hierarchy* h) : Weapon(h)
{
    weaponType     = WeaponType::Torpedo;
    designation    = "HWT";
    length         = 6.1f;
    diameter       = 0.533f;
    totalMass      = 1400.0f;
    payloadMass    = 295.0f;
    fuelMass       = 200.0f;
    maxVelocity    = 28.0f;
    maxRange       = 50000.0f;
    detonationType = DetonationType::Proximity;
}
void Torpedo::Update(){

}
void Torpedo::launch()         { startFlightMonitor(); }
void Torpedo::updateGuidance() { /* TODO: acoustic proportional nav */ }
void Torpedo::scan()           { /* TODO: active acoustic ping */ }
bool Torpedo::canEngage(Platform* target) { return target != nullptr; }

QString Torpedo::homingModeToString() const {
    switch (homingMode) {
    case HomingMode::Passive:    return "Passive";
    case HomingMode::Active:     return "Active";
    case HomingMode::WireGuided: return "WireGuided";
    case HomingMode::WakeHoming: return "WakeHoming";
    default:                     return "Active";
    }
}
void Torpedo::setHomingModeFromString(const QString& s) {
    if      (s == "Passive")    homingMode = HomingMode::Passive;
    else if (s == "WireGuided") homingMode = HomingMode::WireGuided;
    else if (s == "WakeHoming") homingMode = HomingMode::WakeHoming;
    else                         homingMode = HomingMode::Active;
}
QString Torpedo::targetModeToString() const {
    switch (targetMode) {
    case TargetMode::AntiSurface:   return "AntiSurface";
    case TargetMode::AntiSubmarine: return "AntiSubmarine";
    default:                        return "DualMode";
    }
}
void Torpedo::setTargetModeFromString(const QString& s) {
    if      (s == "AntiSurface")   targetMode = TargetMode::AntiSurface;
    else if (s == "AntiSubmarine") targetMode = TargetMode::AntiSubmarine;
    else                            targetMode = TargetMode::DualMode;
}

QJsonObject Torpedo::toJson() const
{
    QJsonObject obj = Weapon::toJson();
    obj["weaponTypeName"]      = "Torpedo";
    obj["homingMode"]          = homingModeToString();
    obj["isWireGuided"]        = isWireGuided;
    obj["wireLength"]          = wireLength;
    obj["thrustMain"]          = thrustMain;
    obj["burnTime"]            = burnTime;
    obj["specificImpulse"]     = specificImpulse;
    obj["runDepth"]            = runDepth;
    obj["maxDepth"]            = maxDepth;
    obj["searchPatternRadius"] = searchPatternRadius;
    obj["targetMode"]          = targetModeToString();
    obj["seekerRange"]         = seekerRange;
    obj["seekerFOV"]           = seekerFOV;
    obj["isLocked"]            = isLocked;
    return obj;
}

void Torpedo::fromJson(const QJsonObject& obj)
{
    Weapon::fromJson(obj);
    setHomingModeFromString(obj.value("homingMode").toString());
    isWireGuided        = obj.value("isWireGuided").toBool(isWireGuided);
    wireLength          = obj.value("wireLength").toDouble(wireLength);
    thrustMain          = obj.value("thrustMain").toDouble(thrustMain);
    burnTime            = obj.value("burnTime").toDouble(burnTime);
    specificImpulse     = obj.value("specificImpulse").toDouble(specificImpulse);
    runDepth            = obj.value("runDepth").toDouble(runDepth);
    maxDepth            = obj.value("maxDepth").toDouble(maxDepth);
    searchPatternRadius = obj.value("searchPatternRadius").toDouble(searchPatternRadius);
    setTargetModeFromString(obj.value("targetMode").toString());
    seekerRange         = obj.value("seekerRange").toDouble(seekerRange);
    seekerFOV           = obj.value("seekerFOV").toDouble(seekerFOV);
    isLocked            = obj.value("isLocked").toBool(isLocked);
}
