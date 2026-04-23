/**
 * @file entityutils.cpp
 * @brief Utility functions for entity management, coordinate conversion, and parameter handling.
 */

#include "entityutils.h"
#include "qjsonobject.h"
#include "qmetaobject.h"

/**
 * @brief Creates a JSON parameter object with value, unit, range, and description.
 * @param value The numeric value.
 * @param unit Unit string (e.g., "km/h", "m").
 * @param min Minimum allowed value.
 * @param max Maximum allowed value.
 * @param description Optional tooltip/description.
 * @return QJsonObject with type "unitParam" and metadata.
 */
QJsonObject toParm(float value, QString unit, float min, float max, QString description) {
    QJsonObject parm;
    parm["type"] = "unitParam";
    parm["min"] = min;
    parm["max"] = max;
    parm["value"] = value;
    parm["unit"] = unit;
    if ((min - max) != 0) {
        parm["value"] = value < min ? min : (value > max ? max : value);
    }
    parm["description"] = description;
    return parm;
}

/**
 * @brief Extracts a float value from a JSON parameter object created by toParm().
 * @param parm The JSON object.
 * @return The value field, or 0.0f if missing.
 */
float valueFromParm(const QJsonObject& parm) {
    if (parm.contains("value")) {
        return parm["value"].toVariant().toDouble();
    }
    return 0.0f;
}

/**
 * @brief Converts degrees to radians.
 * @param degree Angle in degrees.
 * @return Angle in radians.
 */
double toRadians(double degree) {
    return degree * M_PI / 180.0;
}

/**
 * @brief Converts EntityType enum to human-readable string.
 * @param type Entity type.
 * @return String representation.
 */
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

/**
 * @brief Converts a string to EntityType enum.
 * @param str String representation.
 * @return Corresponding EntityType, defaults to Platform.
 */
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
    return Constants::EntityType::Platform;
}

/**
 * @brief Returns a list of all EntityType option strings for UI dropdowns.
 * @return QStringList of enum keys.
 */
QStringList entityTypeOptions() {
    QStringList list;
    int index = Constants::staticMetaObject.indexOfEnumerator("EntityType");
    QMetaEnum metaEnum = Constants::staticMetaObject.enumerator(index);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        list << QString::fromLatin1(metaEnum.key(i));
    }
    return list;
}

/**
 * @brief Converts FormationType enum to string.
 * @param type Formation type.
 * @return String representation.
 */
QString formationTypeToString(Constants::FormationType type) {
    switch (type) {
    case Constants::FormationType::Line: return "Line";
    case Constants::FormationType::V: return "V";
    case Constants::FormationType::Diamond: return "Diamond";
    default: return "Unknown";
    }
}

/**
 * @brief Converts string to FormationType enum.
 * @param str String representation.
 * @return Corresponding FormationType, defaults to Line.
 */
Constants::FormationType stringToFormationType(const QString& str) {
    if (str == "Line") return Constants::FormationType::Line;
    if (str == "V") return Constants::FormationType::V;
    if (str == "Diamond") return Constants::FormationType::Diamond;
    return Constants::FormationType::Line;
}

/**
 * @brief Returns a list of FormationType option strings for UI.
 * @return QStringList of enum keys.
 */
QStringList formationTypeOptions() {
    QStringList list;
    int index = Constants::staticMetaObject.indexOfEnumerator("FormationType");
    QMetaEnum metaEnum = Constants::staticMetaObject.enumerator(index);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        list << QString::fromLatin1(metaEnum.key(i));
    }
    return list;
}

/**
 * @brief Calculates the great-circle distance between two geographic points.
 * @param lat1 Latitude of first point (degrees).
 * @param lon1 Longitude of first point (degrees).
 * @param lat2 Latitude of second point (degrees).
 * @param lon2 Longitude of second point (degrees).
 * @return Distance in meters.
 */
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

/**
 * @brief Computes a new latitude/longitude from a starting point, heading, and distance.
 * @param lat1 Starting latitude (degrees).
 * @param lon1 Starting longitude (degrees).
 * @param heading Bearing in degrees (clockwise from north).
 * @param DISTANCE_KM Distance in kilometers.
 * @return Tuple (new_lat, new_lon) in degrees.
 */
std::tuple<double, double> calculateNewLatLong(double lat1, double lon1, double heading, double DISTANCE_KM) {
    // Convert all inputs to radians
    double phi1 = lat1 * (M_PI / 180.0);
    double lambda1 = lon1 * (M_PI / 180.0);
    double theta = heading * (M_PI / 180.0);

    // Angular distance
    double angular_distance = DISTANCE_KM / 6371.0;

    // New latitude (phi2)
    double phi2 = asin(sin(phi1) * cos(angular_distance) +
                       cos(phi1) * sin(angular_distance) * cos(theta));

    // New longitude (lambda2)
    double lambda2 = lambda1 + atan2(sin(theta) * sin(angular_distance) * cos(phi1),
                                     cos(angular_distance) - sin(phi1) * sin(phi2));

    // Convert back to degrees
    double new_lat_deg = phi2 * (180.0 / M_PI);
    double new_lon_deg = lambda2 * (180.0 / M_PI);

    return {new_lat_deg, new_lon_deg};
}

/**
 * @brief Converts geographic coordinates (lat, lon, alt) to a local flat-Earth Cartesian (XYZ) system.
 * @param lat Latitude (degrees).
 * @param lon Longitude (degrees).
 * @param alt Altitude (km).
 * @return FlatXYZ structure with x (east), z (north), y (up).
 */
FlatXYZ geoToFlatXYZ(double lat, double lon, double alt) {
    const double METERS_PER_DEGREE = 111.31949; // Meters per degree at equator
    lat = lat - 21.1458;
    lon = lon - 79.0882;
    double latRad = lat * M_PI / 180.0;

    FlatXYZ res;
    // X axis: Longitude distance decreases toward poles (cosine factor)
    res.x = lon * METERS_PER_DEGREE * std::cos(latRad);

    // Z axis: Latitude distance is constant
    res.z = lat * METERS_PER_DEGREE;

    // Y axis: Altitude
    res.y = alt;

    return res;
}

/**
 * @brief Converts local flat-Earth Cartesian (XYZ) back to geographic coordinates.
 * @param x Easting (meters).
 * @param y Altitude (km).
 * @param z Northing (meters).
 * @return GeoPos structure with lat, lon, alt.
 */
GeoPos flatXYZToGeo(double x, double y, double z) {
    const double METERS_PER_DEGREE = 111.31949;

    GeoPos res;

    // First compute latitude (independent)
    res.lat = z / METERS_PER_DEGREE;

    // Then compute longitude using latitude
    double latRad = res.lat * M_PI / 180.0;
    double cosLat = std::cos(latRad);

    // Division by zero check (at poles)
    if (std::abs(cosLat) < 0.000001) {
        res.lon = 0; // At a pole, longitude is a single point
    } else {
        res.lon = x / (METERS_PER_DEGREE * cosLat);
    }
    res.lat = 21.1458 + res.lat;
    res.lon = 79.0882 + res.lon;
    res.alt = y;
    return res;
}

/**
 * @brief Converts geographic coordinates to Earth-Centered Earth-Fixed (ECEF) Cartesian.
 * @param lat Latitude (degrees).
 * @param lon Longitude (degrees).
 * @param alt Altitude (meters).
 * @return ECEF structure (x, y, z) in meters.
 */
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

/**
 * @brief Converts ECEF Cartesian coordinates to geographic coordinates.
 * @param x ECEF X (meters).
 * @param y ECEF Y (meters).
 * @param z ECEF Z (meters).
 * @return GeoPos with lat, lon, alt.
 */
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

/**
 * @brief Converts a longitude from -180..180 to 0..360 clockwise (0 = north, 90 = east, etc.).
 * @param lon180 Longitude in degrees (-180 to 180).
 * @return Longitude in degrees (0 to 360) where 0 = north, 90 = east.
 */
double convertToClockwise360(double lon180) {
    // First bring into standard 0-360 range
    double lon360 = (lon180 < 0) ? (lon180 + 360.0) : lon180;

    // Clockwise conversion: subtract from 360
    double clockwise = lon360;

    // If result is 360, set to 0
    if (clockwise >= 360.0) clockwise = 0.0;

    return clockwise;
}
