#include "eo_ir.h"

EO_IR::EO_IR(Hierarchy *hierarchy): m_hierarchy(hierarchy)
{
    //std::cout<<"EO/IR Module is Inilisalized"<<std::endl;

    //ir_payload.surrounding = m_payload.surrounding;
    //eo = new EO(m_payload);
    //ir = new IR(m_payload);
    isActive = true;
}

EO_IR::~EO_IR()
{
    if(eo != nullptr){
        delete eo;
    }
    //std::cout<<"EO/IR Module is Deinilisalized"<<std::endl;
}

EO *EO_IR::initEO(CustomSensor sensor,
    Surrounding surrounding)
{
    eo = new EO(sensor, surrounding);
    return eo;
}

constexpr double R = 6378137.0; // Earth radius (meters)

Vec3 EO_IR::toECEF(double latDeg, double lonDeg, double alt)
{
    double lat = latDeg * M_PI / 180.0;
    double lon = lonDeg * M_PI / 180.0;

    double r = R + alt;

    double x = r * cos(lat) * cos(lon);
    double y = r * cos(lat) * sin(lon);
    double z = r * sin(lat);

    return Vec3(x, y, z);
}

Vec3 EO_IR::getViewDir(Coordinate a, Coordinate b)
{
    Vec3 obs = toECEF(a.latitude, a.longitude, a.altitude);
    Vec3 obj = toECEF(b.latitude, b.longitude, b.altitude);
    Vec3 dirECEF = Vec3(
        obj.x - obs.x,
        obj.y - obs.y,
        obj.z - obs.z
    );
    Vec3 dirENU = ecefToENU(dirECEF, a.latitude, a.longitude);
    Vec3 viewDir = dirENU.normalized();
    return viewDir;
}

Vec3 EO_IR::ecefToENU(const Vec3 &d, double latDeg, double lonDeg)
{
    double lat = latDeg * M_PI / 180.0;
    double lon = lonDeg * M_PI / 180.0;

    double sinLat = sin(lat), cosLat = cos(lat);
    double sinLon = sin(lon), cosLon = cos(lon);

    double east  = -sinLon * d.x + cosLon * d.y;
    double north = -sinLat*cosLon * d.x - sinLat*sinLon * d.y + cosLat * d.z;
    double up    =  cosLat*cosLon * d.x + cosLat*sinLon * d.y + sinLat * d.z;

    return Vec3(east, north, up);
}

double EO_IR::getProjectedArea(
    Vec3 viewDir, EntityDimension entityDimension)
{
    Vec3 v = viewDir.normalized();
    double vx = std::abs(v.x);
    double vy = std::abs(v.y);
    double vz = std::abs(v.z);

    return (vx * (entityDimension.width *      entityDimension.heigth)) +
           (vy * (entityDimension.length * entityDimension.heigth)) +
           (vz * (entityDimension.length * entityDimension.width));
}

void EO_IR::intro()
{
    //std::cout<<"Welcome to EO/IR Module"<<std::endl;
}

void EO_IR::preprocessing()
{
    CustomEntity user = m_payload.user;
    ppel = new PreProcessEntityList();
    m_platform = m_hierarchy->Platforms;
    // Iterate entities
    for(auto entity = m_payload.entityList.begin();
         entity != m_payload.entityList.end(); ++entity){
        PreProcessEntity ppe;
        ppe.frontalSurfaceArea = entity->second.frontalSurfaceArea;
        ppe.distanceBtwUser = distanceBtw
            (user.coordinate,entity->second.coordinate);
        ppe.angleBtwUser    = calculateAngle
            (user.heading, user.pitch,
             user.coordinate,entity->second.coordinate);
        ppel->insert({entity->first, ppe});
    }
    eo_payload.surrounding  = m_payload.surrounding;
    eo_payload.eoParameters = m_payload.eoParameters;
    eo_payload.entityList   = m_payload.entityList;
    eo_payload.preProcessEntityList = *ppel;
    //eo = new EO(eo_payload);
}

#define EARTH_RADIUS 6371000.0 // meters

double EO_IR::distanceBtw(Coordinate p1, Coordinate p2) {
    // Convert to radians
    p1.latitude  = toRadians(p1.latitude );
    p1.longitude = toRadians(p1.longitude);
    p2.latitude  = toRadians(p2.latitude );
    p2.longitude = toRadians(p2.longitude);

    // Haversine formula
    double dLat = p2.latitude  - p1.latitude ;
    double dLon = p2.longitude - p1.longitude;

    double a = sin(dLat/2) * sin(dLat/2) +
               cos(p1.latitude) * cos(p2.latitude) *
                   sin(dLon/2) * sin(dLon/2);

    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    double surfaceDistance = EARTH_RADIUS * c;

    // Add altitude difference
    double height = p2.altitude - p1.altitude;

    // Final 3D distance
    return sqrt(surfaceDistance * surfaceDistance + height * height);
}



double EO_IR::toRadians(double degree) {
    return degree * M_PI / 180.0;
}

double EO_IR::distanceBtw(
    double lat1, double lon1, double alt1,
    double lat2, double lon2, double alt2)
{
    // Convert to radians
    lat1 = toRadians(lat1);
    lon1 = toRadians(lon1);
    lat2 = toRadians(lat2);
    lon2 = toRadians(lon2);

    // Haversine formula
    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;

    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1) * cos(lat2) *
                   sin(dLon/2) * sin(dLon/2);

    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    double surfaceDistance = EARTH_RADIUS * c;

    // Add altitude difference
    double height = alt2 - alt1;

    // Final 3D distance
    return sqrt(surfaceDistance * surfaceDistance + height * height);
}
// double EO_IR::distanceBtw(
//     double p1latitude, double p1longitude,
//     double p1altitude, double p2latitude,
//     double p2longitude, double p2altitude)
// {
//     return sqrt(pow(p2latitude  - p1latitude , 2) +
//                 pow(p2longitude - p1longitude, 2) +
//                 pow(p2altitude  - p1altitude , 2));
// }

// Function to convert Lat/Lon/Alt to Cartesian coordinates
Coordinate EO_IR::toCartesian(double lat, double lon, double alt) {
    double R = 6371000.0; // Earth's mean radius in meters
    double latRad = lat * M_PI / 180.0;
    double lonRad = lon * M_PI / 180.0;

    Coordinate p;
    p.latitude  = (R + alt) * cos(latRad) * cos(lonRad);
    p.longitude = (R + alt) * cos(latRad) * sin(lonRad);
    p.altitude  = (R + alt) * sin(latRad);
    return p;
}

double EO_IR::calculateAngle(double headingDeg,
                             double pitchDeg,
                             Coordinate entityA,
                             Coordinate entityB)
{
    Axis A_pos = toVector(entityA.latitude, entityA.longitude, entityA.altitude);
    Axis B_pos = toVector(entityB.latitude, entityB.longitude, entityB.altitude);

    // Convert AB → ENU frame
    Axis AB = ecefToENU(A_pos, B_pos, entityA.latitude, entityA.longitude);

    // Direction from heading/pitch (ENU frame)
    double heading = headingDeg * M_PI / 180.0;
    double pitch   = pitchDeg   * M_PI / 180.0;

    Axis dirA = {
        cos(pitch) * sin(heading), // East
        cos(pitch) * cos(heading), // North
        sin(pitch)                 // Up
    };

    double magDir = dirA.magnitude();
    double magAB  = AB.magnitude();

    if (magDir == 0 || magAB == 0) return -1;

    double dot = dirA.x * AB.x + dirA.y * AB.y + dirA.z * AB.z;

    // double cosTheta = dot / (magDir * magAB);
    // cosTheta = std::max(-1.0, std::min(1.0, cosTheta));
    // // std::cout << "AB (ENU): " << AB.x << ", " << AB.y << ", " << AB.z << std::endl;
    // // std::cout << "dirA: " << dirA.x << ", " << dirA.y << ", " << dirA.z << std::endl;
    // // std::cout << "cosTheta: " << std::acos(cosTheta) * 180.0 / M_PI<< std::endl;
    // return std::acos(cosTheta) * 180.0 / M_PI;
    Axis cross = {
        dirA.y * AB.z - dirA.z * AB.y,
        dirA.z * AB.x - dirA.x * AB.z,
        dirA.x * AB.y - dirA.y * AB.x
    };

    double angle = atan2(cross.magnitude(), dot);

    // Sign using Up axis (Z)
    double sign = (cross.z >= 0) ? -1.0 : 1.0;

    return sign * angle * 180.0 / M_PI;
}
double EO_IR::calculateAngle(
    double headingDeg, double pitchDeg,
    double p1latitude, double p1longitude,
    double p1altitude, double p2latitude,
    double p2longitude, double p2altitude)
{
    Axis A_pos = toVector(p1latitude, p1longitude, p1altitude);
    Axis B_pos = toVector(p2latitude, p2longitude, p2altitude);

    Axis AB = ecefToENU(A_pos, B_pos, p1latitude, p1longitude);

    double heading = headingDeg * M_PI / 180.0;
    double pitch   = pitchDeg   * M_PI / 180.0;

    Axis dirA = {
        cos(pitch) * sin(heading), // East
        cos(pitch) * cos(heading), // North
        sin(pitch)                 // Up
    };

    double magDir = dirA.magnitude();
    double magAB  = AB.magnitude();

    if (magDir == 0 || magAB == 0) return -1;

    double dot = dirA.x * AB.x + dirA.y * AB.y + dirA.z * AB.z;

    Axis cross = {
        dirA.y * AB.z - dirA.z * AB.y,
        dirA.z * AB.x - dirA.x * AB.z,
        dirA.x * AB.y - dirA.y * AB.x
    };

    double angle = atan2(cross.magnitude(), dot);

    // Sign using Up axis (Z)
    double sign = (cross.z >= 0) ? -1.0 : 1.0;

    return sign * angle * 180.0 / M_PI;
}

double EO_IR::relativeAngle(
    double sensorAngle, double targetAngle)
{
    double diff = targetAngle - sensorAngle;
    if(diff > 180){
        return diff -360;
    }else if(diff < -180){
        return diff + 360;
    }else{
        return diff;
    }
}

double EO_IR::viewAngle(double sensorAngle, double targetAngle)
{
    if(sensorAngle > 0){
        if(targetAngle > 0){
            return (-180 + sensorAngle -(targetAngle-180));
        }else {
            return (-180 + sensorAngle - targetAngle);
        }
    }else{
        if(targetAngle > 0){
            return 180 + (sensorAngle - targetAngle);
        }else {
            return -180 + (sensorAngle + targetAngle);
        }
    }
}

Axis EO_IR::toVector(double lat, double lon, double alt) {
    double R = 6371000.0; // Earth radius in meters
    double phi = lat * M_PI / 180.0;
    double lam = lon * M_PI / 180.0;

    return {
        (R + alt) * std::cos(phi) * std::cos(lam),
        (R + alt) * std::cos(phi) * std::sin(lam),
        (R + alt) * std::sin(phi)
    };
}
Axis EO_IR::ecefToENU(const Axis& ref, const Axis& target, double latDeg, double lonDeg) {
    double lat = latDeg * M_PI / 180.0;
    double lon = lonDeg * M_PI / 180.0;

    // Difference vector
    Axis d = {
        target.x - ref.x,
        target.y - ref.y,
        target.z - ref.z
    };

    Axis enu;

    enu.x = -sin(lon) * d.x + cos(lon) * d.y; // East
    enu.y = -sin(lat)*cos(lon)*d.x - sin(lat)*sin(lon)*d.y + cos(lat)*d.z; // North
    enu.z =  cos(lat)*cos(lon)*d.x + cos(lat)*sin(lon)*d.y + sin(lat)*d.z; // Up

    return enu;
}
