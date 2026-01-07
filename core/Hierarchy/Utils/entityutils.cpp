#include "entityutils.h"
#include "qjsonobject.h"
#include "qmetaobject.h"




QJsonObject toParm(float value,QString unit){
    QJsonObject parm;
    parm["type"] = "unitParam";
    parm["value"] = value;
    parm["unit"] = unit;
    return parm;
}

float valueFromParm(const QJsonObject& parm) {
    if (parm.contains("value") ) {
        return parm["value"].toVariant().toDouble();
    }
    return 0.0f; // Default value if key is missing or not a double
}


double toRadians(double degree) {
    return degree * M_PI / 180.0;
}

QString entityTypeToString(Constants::EntityType type) {
    switch (type) {
    case Constants::EntityType::Platform: return "Platform";
    case Constants::EntityType::Radio: return "Radio";
    case Constants::EntityType::Sensor: return "Sensor";
    case Constants::EntityType::SpecialZone: return "SpecialZone";
    case Constants::EntityType::Weapon: return "Weapon";
    case Constants::EntityType::IFF: return "IFF";
    case Constants::EntityType::FixedPoint: return "FixedPoint";
    case Constants::EntityType::Formation: return "Formation";
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
    if (str == "FixedPoint") return Constants::EntityType::FixedPoint;
    if (str == "Formation") return Constants::EntityType::Formation;
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

QString formationTypeToString(Constants::FormationType type) {
    switch (type) {
    case Constants::FormationType::Line: return "Line";
    case Constants::FormationType::V: return "V";
    case Constants::FormationType::Diamond: return "Diamond";

    default: return "Unknown";
    }
}

Constants::FormationType stringToFormationType(const QString& str) {
    if (str == "Line") return Constants::FormationType::Line;
    if (str == "V") return Constants::FormationType::V;
    if (str == "Diamond") return Constants::FormationType::Diamond;
    return Constants::FormationType::Line; // default fallback
}

QStringList formationTypeOptions() {
    QStringList list;
    int index = Constants::staticMetaObject.indexOfEnumerator("FormationType");
    QMetaEnum metaEnum = Constants::staticMetaObject.enumerator(index);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        list << QString::fromLatin1(metaEnum.key(i));
    }
    return list;
}

double distanceBetween(double lat1, double lon1, double lat2, double lon2) {
    double dLat = toRadians(lat2 - lat1);
    double dLon = toRadians(lon2 - lon1);

    lat1 = toRadians(lat1);
    lat2 = toRadians(lat2);

    double a = pow(sin(dLat / 2), 2) +
               pow(sin(dLon / 2), 2) * cos(lat1) * cos(lat2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS * c; // Distance in meters
}

std::tuple<double, double> calculateNewLatLong(double lat1, double lon1, double heading, double DISTANCE_KM)
{

    // 1. सभी इनपुट को रेडियन में बदलें
    double phi1 = lat1 * (M_PI / 180.0);
    double lambda1 = lon1 * (M_PI / 180.0);
    double theta = heading * (M_PI / 180.0);

    // 2. कोणीय दूरी (Angular Distance) की गणना
    double angular_distance = DISTANCE_KM / 6371.0; // डेल्टा (delta)

    // 3. नए अक्षांश (phi2) की गणना
    double phi2 = asin(sin(phi1) * cos(angular_distance) +
                       cos(phi1) * sin(angular_distance) * cos(theta));

    // 4. नए देशांतर (lambda2) की गणना
    double lambda2 = lambda1 + atan2(sin(theta) * sin(angular_distance) * cos(phi1),
                                     cos(angular_distance) - sin(phi1) * sin(phi2));

    // 5. परिणाम को वापस डिग्री में बदलें
    double new_lat_deg = phi2* (180.0 / M_PI);
    double new_lon_deg = lambda2* (180.0 / M_PI);

    // 6. परिणाम लौटाएँ
    return {new_lat_deg, new_lon_deg};
}







FlatXYZ geoToFlatXYZ(double lat, double lon, double alt) {
    const double METERS_PER_DEGREE = 111.31949; // Equator par 1 degree kitne meter hai
    lat = lat - 21.1458;
    lon = lon - 79.0882;
    double latRad = lat * M_PI / 180.0;

    FlatXYZ res;
    // X axis: Longitude distance poles par kam hoti jati hai (Cosine Factor)
    res.x = lon * METERS_PER_DEGREE * std::cos(latRad);

    // Y axis: Latitude distance hamesha constant rehti hai
    res.z = lat * METERS_PER_DEGREE;

    // Z axis: Altitude
    res.y = alt;

    return res;
}



GeoPos flatXYZToGeo(double x, double y, double z) {
    const double METERS_PER_DEGREE = 111.31949;

    GeoPos res;

    // 1. Pehle Latitude nikalein kyunki ye independent hai
    res.lat = z / METERS_PER_DEGREE;

    // 2. Latitude ka use karke Longitude nikalein
    double latRad = res.lat * M_PI / 180.0;
    double cosLat = std::cos(latRad);

    // Division by zero check (Poles par)
    if (std::abs(cosLat) < 0.000001) {
        res.lon = 0; // Pole point par longitude ek single point hota hai
    } else {
        res.lon = x / (METERS_PER_DEGREE * cosLat);
    }
    res.lat = 21.1458 + res.lat;
    res.lon = 79.0882 + res.lon;
    res.alt = y;
    return res;
}

ECEF geoToXYZ(double lat, double lon, double alt) {
    double a = 6378137.0;              // Equatorial radius
    double e2 = 0.00669437999014;      // Square of eccentricity

    double latRad = lat * M_PI / 180.0;
    double lonRad = lon * M_PI / 180.0;

    // N = Radius of curvature in the prime vertical
    double N = a / std::sqrt(1 - e2 * std::sin(latRad) * std::sin(latRad));

    ECEF res;
    res.x = (N + alt) * std::cos(latRad) * std::cos(lonRad);
    res.y = (N + alt) * std::cos(latRad) * std::sin(lonRad);
    res.z = ((1 - e2) * N + alt) * std::sin(latRad);

    return res;
}

GeoPos xyzToGeo(double x, double y, double z) {
    double a = 6378137.0;
    double b = 6356752.3142;           // Polar radius
    double e2 = 0.00669437999014;
    double ep2 = 0.00673949674228;     // Second eccentricity squared

    double p = std::sqrt(x*x + y*y);
    double theta = std::atan2(z * a, p * b);

    GeoPos res;
    // Longitude is simple
    res.lon = std::atan2(y, x) * 180.0 / M_PI;

    // Latitude (Bowring's formula)
    double latRad = std::atan2(z + ep2 * b * std::pow(std::sin(theta), 3),
                               p - e2 * a * std::pow(std::cos(theta), 3));
    res.lat = latRad * 180.0 / M_PI;

    // Altitude
    double N = a / std::sqrt(1 - e2 * std::sin(latRad) * std::sin(latRad));
    res.alt = (p / std::cos(latRad)) - N;

    return res;
}


double convertToClockwise360(double lon180) {
    // 1. Pehle standard 0-360 range mein layein
    double lon360 = (lon180 < 0) ? (lon180 + 360.0) : lon180;

    // 2. Clockwise karne ke liye 360 se subtract karein
    // Isse 90 (East) ban jayega 270, aur -90 (West) ban jayega 90.
    double clockwise =lon360;

    // Agar result 360 hai toh use 0 kar dein
    if (clockwise >= 360.0) clockwise = 0.0;

    return clockwise;
}
