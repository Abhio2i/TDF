#include "rocket.h"

Rocket::Rocket(Hierarchy* h) : Weapon(h)
{
    weaponType     = WeaponType::Rocket;
    designation    = "Hydra-70";
    length         = 1.07f;
    diameter       = 0.07f;
    totalMass      = 6.8f;
    payloadMass    = 1.8f;
    fuelMass       = 1.5f;
    maxVelocity    = 700.0f;
    maxRange       = 8000.0f;
    detonationType = DetonationType::Impact;
}

void Rocket::launch()      { /* fire salvoCount rounds */ }
void Rocket::flyToTarget() { /* TODO: fin-stabilised unguided arc */ }
void Rocket::Update(){

}

QString Rocket::warheadTypeToString() const {
    switch (warheadVariant) {
    case WarheadType::HE:           return "HE";
    case WarheadType::HEAT:         return "HEAT";
    case WarheadType::Flechette:    return "Flechette";
    case WarheadType::Illumination: return "Illumination";
    case WarheadType::Smoke:        return "Smoke";
    case WarheadType::WP:           return "WP";
    default:                        return "HE";
    }
}
void Rocket::setWarheadTypeFromString(const QString& s) {
    if      (s == "HEAT")         warheadVariant = WarheadType::HEAT;
    else if (s == "Flechette")    warheadVariant = WarheadType::Flechette;
    else if (s == "Illumination") warheadVariant = WarheadType::Illumination;
    else if (s == "Smoke")        warheadVariant = WarheadType::Smoke;
    else if (s == "WP")           warheadVariant = WarheadType::WP;
    else                           warheadVariant = WarheadType::HE;
}

QJsonObject Rocket::toJson() const
{
    QJsonObject obj = Weapon::toJson();
    obj["weaponTypeName"]          = "Rocket";
    obj["thrustMain"]              = thrustMain;
    obj["burnTime"]                = burnTime;
    obj["specificImpulse"]         = specificImpulse;
    obj["calibre"]                 = calibre;
    obj["finStabilised"]           = finStabilised;
    obj["dragCoefficient"]         = dragCoefficient;
    obj["spinRateRPM"]             = spinRateRPM;
    obj["hasAdvancedPrecisionKit"] = hasAdvancedPrecisionKit;
    obj["cep"]                     = cep;
    obj["warheadType"]             = warheadTypeToString();
    obj["salvoCount"]              = salvoCount;
    obj["salvoInterval"]           = salvoInterval;
    obj["podCapacity"]             = podCapacity;
    return obj;
}

void Rocket::fromJson(const QJsonObject& obj)
{
    Weapon::fromJson(obj);
    thrustMain      = obj.value("thrustMain").toDouble(thrustMain);
    burnTime        = obj.value("burnTime").toDouble(burnTime);
    specificImpulse = obj.value("specificImpulse").toDouble(specificImpulse);
    calibre         = obj.value("calibre").toDouble(calibre);
    finStabilised   = obj.value("finStabilised").toBool(finStabilised);
    dragCoefficient = obj.value("dragCoefficient").toDouble(dragCoefficient);
    spinRateRPM     = obj.value("spinRateRPM").toDouble(spinRateRPM);
    hasAdvancedPrecisionKit = obj.value("hasAdvancedPrecisionKit").toBool(hasAdvancedPrecisionKit);
    cep             = obj.value("cep").toDouble(cep);
    setWarheadTypeFromString(obj.value("warheadType").toString());
    salvoCount      = obj.value("salvoCount").toInt(salvoCount);
    salvoInterval   = obj.value("salvoInterval").toDouble(salvoInterval);
    podCapacity     = obj.value("podCapacity").toInt(podCapacity);
}
