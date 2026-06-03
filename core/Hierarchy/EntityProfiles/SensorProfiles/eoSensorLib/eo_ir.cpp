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

EO *EO_IR::initEO(eoSensorPayload sensor,
    Surrounding surrounding)
{
    eo = new EO(sensor, surrounding);
    return eo;
}

constexpr float R = 6378137.0; // Earth radius (meters)

Vec3 EO_IR::toECEF(float latDeg, float lonDeg, float alt)
{
    float lat = latDeg * M_PI / 180.0;
    float lon = lonDeg * M_PI / 180.0;

    float r = R + alt;

    float x = r * cos(lat) * cos(lon);
    float y = r * cos(lat) * sin(lon);
    float z = r * sin(lat);

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

Vec3 EO_IR::ecefToENU(const Vec3 &d, float latDeg, float lonDeg)
{
    float lat = latDeg * M_PI / 180.0;
    float lon = lonDeg * M_PI / 180.0;

    float sinLat = sin(lat), cosLat = cos(lat);
    float sinLon = sin(lon), cosLon = cos(lon);

    float east  = -sinLon * d.x + cosLon * d.y;
    float north = -sinLat*cosLon * d.x - sinLat*sinLon * d.y + cosLat * d.z;
    float up    =  cosLat*cosLon * d.x + cosLat*sinLon * d.y + sinLat * d.z;

    return Vec3(east, north, up);
}

float EO_IR::getProjectedArea(
    Vec3 viewDir, EntityDimension entityDimension)
{
    Vec3 v = viewDir.normalized();
    float vx = std::abs(v.x);
    float vy = std::abs(v.y);
    float vz = std::abs(v.z);

    return (vx * (entityDimension.width *      entityDimension.heigth)) +
           (vy * (entityDimension.length * entityDimension.heigth)) +
           (vz * (entityDimension.length * entityDimension.width));
}

ProjectedExtents EO_IR::getProjectedExtents(Vec3 viewDir, EntityDimension entityDimension)
{
    // 1. Establish the 2D coordinate system of the view plane.
    // 'v' is the vector pointing from the object to the camera (view direction).
    Vec3 v = viewDir.normalized();

    // 'right' vector (horizontal axis in 2D view)
    // We use (0,0,1) as a reference up vector to find 'right' via cross product.
    // If viewing exactly from top/bottom, we switch reference to (0,1,0).
    Vec3 upRef(0, 0, 1);
    if (std::abs(v.z) > 0.99f) upRef = Vec3(0, 1, 0);

    // Manual Cross Product: Right = v x upRef
    Vec3 right(
        v.y * upRef.z - v.z * upRef.y,
        v.z * upRef.x - v.x * upRef.z,
        v.x * upRef.y - v.y * upRef.x
        );
    right = right.normalized();

    // 'up' vector (vertical axis in 2D view): up = right x v
    Vec3 up(
        right.y * v.z - right.z * v.y,
        right.z * v.x - right.x * v.z,
        right.x * v.y - right.y * v.x
        );

    // 2. Project the three principal axes of the box (Width, Length, Height)
    // onto the 'right' and 'up' vectors.
    // The max extent is the sum of the absolute projections of these axes.

    // Projections onto the 'right' vector (Horizontal Length)
    float h = std::abs(right.x * entityDimension.width) +
              std::abs(right.y * entityDimension.length) +
              std::abs(right.z * entityDimension.heigth);

    // Projections onto the 'up' vector (Vertical Length)
    float w = std::abs(up.x * entityDimension.width) +
              std::abs(up.y * entityDimension.length) +
              std::abs(up.z * entityDimension.heigth);

    return { h, w };
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

float EO_IR::distanceBtw(Coordinate p1, Coordinate p2) {
    // Convert to radians
    p1.latitude  = toRadians(p1.latitude );
    p1.longitude = toRadians(p1.longitude);
    p2.latitude  = toRadians(p2.latitude );
    p2.longitude = toRadians(p2.longitude);

    // Haversine formula
    float dLat = p2.latitude  - p1.latitude ;
    float dLon = p2.longitude - p1.longitude;

    float a = sin(dLat/2) * sin(dLat/2) +
               cos(p1.latitude) * cos(p2.latitude) *
                   sin(dLon/2) * sin(dLon/2);

    float c = 2 * atan2(sqrt(a), sqrt(1-a));

    float surfaceDistance = EARTH_RADIUS * c;

    // Add altitude difference
    float height = p2.altitude - p1.altitude;

    // Final 3D distance
    return sqrt(surfaceDistance * surfaceDistance + height * height);
}



float EO_IR::toRadians(float degree) {
    return degree * M_PI / 180.0;
}

float EO_IR::distanceBtw(
    float lat1, float lon1, float alt1,
    float lat2, float lon2, float alt2)
{
    // Convert to radians
    lat1 = toRadians(lat1);
    lon1 = toRadians(lon1);
    lat2 = toRadians(lat2);
    lon2 = toRadians(lon2);

    // Haversine formula
    float dLat = lat2 - lat1;
    float dLon = lon2 - lon1;

    float a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1) * cos(lat2) *
                   sin(dLon/2) * sin(dLon/2);

    float c = 2 * atan2(sqrt(a), sqrt(1-a));

    float surfaceDistance = EARTH_RADIUS * c;

    // Add altitude difference
    float height = alt2 - alt1;

    // Final 3D distance
    return sqrt(surfaceDistance * surfaceDistance + height * height);
}
// float EO_IR::distanceBtw(
//     float p1latitude, float p1longitude,
//     float p1altitude, float p2latitude,
//     float p2longitude, float p2altitude)
// {
//     return sqrt(pow(p2latitude  - p1latitude , 2) +
//                 pow(p2longitude - p1longitude, 2) +
//                 pow(p2altitude  - p1altitude , 2));
// }

// Function to convert Lat/Lon/Alt to Cartesian coordinates
Coordinate EO_IR::toCartesian(float lat, float lon, float alt) {
    float R = 6371000.0; // Earth's mean radius in meters
    float latRad = lat * M_PI / 180.0;
    float lonRad = lon * M_PI / 180.0;

    Coordinate p;
    p.latitude  = (R + alt) * cos(latRad) * cos(lonRad);
    p.longitude = (R + alt) * cos(latRad) * sin(lonRad);
    p.altitude  = (R + alt) * sin(latRad);
    return p;
}

float EO_IR::calculateAngle(float headingDeg,
                             float pitchDeg,
                             Coordinate entityA,
                             Coordinate entityB)
{
    Axis A_pos = toVector(entityA.latitude, entityA.longitude, entityA.altitude);
    Axis B_pos = toVector(entityB.latitude, entityB.longitude, entityB.altitude);

    // Convert AB → ENU frame
    Axis AB = ecefToENU(A_pos, B_pos, entityA.latitude, entityA.longitude);

    // Direction from heading/pitch (ENU frame)
    float heading = headingDeg * M_PI / 180.0;
    float pitch   = pitchDeg   * M_PI / 180.0;

    Axis dirA = {
        cosf(pitch) * sinf(heading), // East
        cosf(pitch) * cosf(heading), // North
        sinf(pitch)                 // Up
    };

    float magDir = dirA.magnitude();
    float magAB  = AB.magnitude();

    if (magDir == 0 || magAB == 0) return -1;

    float dot = dirA.x * AB.x + dirA.y * AB.y + dirA.z * AB.z;

    // float cosTheta = dot / (magDir * magAB);
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

    float angle = atan2(cross.magnitude(), dot);

    // Sign using Up axis (Z)
    float sign = (cross.z >= 0) ? -1.0 : 1.0;

    return sign * angle * 180.0 / M_PI;
}
float EO_IR::calculateAngle(
    float headingDeg, float pitchDeg,
    float p1latitude, float p1longitude,
    float p1altitude, float p2latitude,
    float p2longitude, float p2altitude)
{
    Axis A_pos = toVector(p1latitude, p1longitude, p1altitude);
    Axis B_pos = toVector(p2latitude, p2longitude, p2altitude);

    Axis AB = ecefToENU(A_pos, B_pos, p1latitude, p1longitude);

    float heading = headingDeg * M_PI / 180.0;
    float pitch   = pitchDeg   * M_PI / 180.0;

    Axis dirA = {
        cosf(pitch) * sinf(heading), // East
        cosf(pitch) * cosf(heading), // North
        sinf(pitch)                 // Up
    };

    float magDir = dirA.magnitude();
    float magAB  = AB.magnitude();

    if (magDir == 0 || magAB == 0) return -1;

    float dot = dirA.x * AB.x + dirA.y * AB.y + dirA.z * AB.z;

    Axis cross = {
        dirA.y * AB.z - dirA.z * AB.y,
        dirA.z * AB.x - dirA.x * AB.z,
        dirA.x * AB.y - dirA.y * AB.x
    };

    float angle = atan2(cross.magnitude(), dot);

    // Sign using Up axis (Z)
    float sign = (cross.z >= 0) ? -1.0 : 1.0;

    return sign * angle * 180.0 / M_PI;
}

float EO_IR::relativeAngle(
    float sensorAngle, float targetAngle)
{
    float diff = targetAngle - sensorAngle;
    if(diff > 180){
        return diff -360;
    }else if(diff < -180){
        return diff + 360;
    }else{
        return diff;
    }
}

float EO_IR::viewAngle(float sensorAngle, float targetAngle)
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

bool EO_IR::scanVeticalHorizonatalAngles(
    float veticalAzimuth, float horizonatalAzimuth,
    float veticalAngle, float horizonatalAngle)
{
    if(!((veticalAzimuth/2) > veticalAngle && (-1*veticalAzimuth/2) < veticalAngle))
        return  false;
    if(!((horizonatalAzimuth/2) > horizonatalAngle && (-1*horizonatalAzimuth/2) < horizonatalAngle))
        return  false;
    return true;
}

float EO_IR::getHorizontalTargetAngle(
    Coordinate sensor, Coordinate target,
    float sensorHeadingDeg)
{
    // 1. Convert all degrees to radians (using float literals)
    float lat1 = sensor.latitude * (M_PI / 180.0f);
    float lon1 = sensor.longitude * (M_PI / 180.0f);
    float lat2 = target.latitude * (M_PI / 180.0f);
    float lon2 = target.longitude * (M_PI / 180.0f);

    float dLon = lon2 - lon1;

    // 2. Calculate absolute bearing
    // Using 'f' suffix versions for single-precision math
    float y = sinf(dLon) * cosf(lat2);
    float x = cosf(lat1) * sinf(lat2) -
              sinf(lat1) * cosf(lat2) * cosf(dLon);

    float absoluteBearingRad = atan2f(y, x);
    float absoluteBearingDeg = absoluteBearingRad * (180.0f / M_PI);

    // Normalize absolute bearing to [0, 360)
    absoluteBearingDeg = fmodf((absoluteBearingDeg + 360.0f), 360.0f);

    // 3. Calculate the relative angle
    float relativeAngle = absoluteBearingDeg - sensorHeadingDeg;

    // 4. Normalize to [-180, 180] for sensor boresight logic
    if (relativeAngle > 180.0f) {
        relativeAngle -= 360.0f;
    } else if (relativeAngle <= -180.0f) {
        relativeAngle += 360.0f;
    }
    return relativeAngle;
}

float EO_IR::getVerticalTargetAngle(float sensorAlt, float targetAlt, float horizontalDistance, float sensorPitchDeg)
{
    // Prevent division by zero or domain errors if they occupy the exact same X/Y space
    if (horizontalDistance <= 0.001f) {
        if (targetAlt > sensorAlt) return 90.0f - sensorPitchDeg;
        if (targetAlt < sensorAlt) return -90.0f - sensorPitchDeg;
        return 0.0f;
    }

    // 1. Calculate the altitude difference (Delta Y)
    float deltaAlt = targetAlt - sensorAlt;

    // 2. Calculate the absolute elevation angle to the target
    // atan2f safely handles the quadrant logic based on height difference and distance
    float absoluteElevationRad = atan2f(deltaAlt, horizontalDistance);
    float absoluteElevationDeg = absoluteElevationRad * (180.0f / M_PI);

    // 3. Calculate the relative vertical angle (off-boresight)
    float relativeAngle = absoluteElevationDeg - sensorPitchDeg;

    // 4. Normalize to [-180, 180]
    if (relativeAngle > 180.0f) {
        relativeAngle -= 360.0f;
    } else if (relativeAngle <= -180.0f) {
        relativeAngle += 360.0f;
    }

    return relativeAngle;
}

float EO_IR::getHorizontalDistance(Coordinate point1, Coordinate point2)
{
    // 1. Convert degrees to radians
    float lat1 = point1.latitude * (M_PI / 180.0f);
    float lon1 = point1.longitude * (M_PI / 180.0f);
    float lat2 = point2.latitude * (M_PI / 180.0f);
    float lon2 = point2.longitude * (M_PI / 180.0f);

    // 2. Calculate differences
    float dLat = lat2 - lat1;
    float dLon = lon2 - lon1;

    // 3. Haversine formula core
    // Using sinf and cosf for float performance
    float halfLatSin = sinf(dLat / 2.0f);
    float halfLonSin = sinf(dLon / 2.0f);

    float a = (halfLatSin * halfLatSin) +
              cosf(lat1) * cosf(lat2) *
                  (halfLonSin * halfLonSin);

    // Ensure 'a' doesn't slightly exceed 1.0 due to floating-point errors
    if (a > 1.0f) a = 1.0f;

    // 4. Calculate angular distance
    float c = 2.0f * atan2f(std::sqrt(a), std::sqrt(1.0f - a));

    // 5. Convert to physical distance
    return EARTH_RADIUS_METERS * c;
}

Axis EO_IR::toVector(float lat, float lon, float alt) {
    float R = 6371000.0; // Earth radius in meters
    float phi = lat * M_PI / 180.0;
    float lam = lon * M_PI / 180.0;

    return {
        (R + alt) * std::cos(phi) * std::cos(lam),
        (R + alt) * std::cos(phi) * std::sin(lam),
        (R + alt) * std::sin(phi)
    };
}
Axis EO_IR::ecefToENU(const Axis& ref, const Axis& target, float latDeg, float lonDeg) {
    float lat = latDeg * M_PI / 180.0;
    float lon = lonDeg * M_PI / 180.0;

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
