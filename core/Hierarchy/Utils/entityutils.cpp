#include "entityutils.h"
#include "qmetaobject.h"

QString entityTypeToString(Constants::EntityType type) {
    switch (type) {
    case Constants::EntityType::Platform: return "Platform";
    case Constants::EntityType::Radio: return "Radio";
    case Constants::EntityType::Sensor: return "Sensor";
    case Constants::EntityType::SpecialZone: return "SpecialZone";
    case Constants::EntityType::Weapon: return "Weapon";
    case Constants::EntityType::IFF: return "IFF";
    case Constants::EntityType::Supply: return "Supply";
    default: return "Unknown";
    }
}

Constants::EntityType stringToEntityType(const QString& str) {
    if (str == "Platform") return Constants::EntityType::Platform;
    if (str == "Radio") return Constants::EntityType::Radio;
    if (str == "Sensor") return Constants::EntityType::Sensor;
    if (str == "SpecialZone") return Constants::EntityType::SpecialZone;
    if (str == "Weapon") return Constants::EntityType::Weapon;
    if (str == "IFF") return Constants::EntityType::IFF;
    if (str == "Supply") return Constants::EntityType::Supply;
    return Constants::EntityType::Platform; // default fallback
}

QStringList entityTypeOptions() {
    QStringList list;
    int index = Constants::staticMetaObject.indexOfEnumerator("EntityType");
    QMetaEnum metaEnum = Constants::staticMetaObject.enumerator(index);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        list << QString::fromLatin1(metaEnum.key(i));
    }
    return list;
}
