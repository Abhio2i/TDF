#ifndef PAYLOAD_H
#define PAYLOAD_H

//#include <vector>

#include <unordered_map>
#include <string>
#include <cmath>
enum class InputResult{
    VALID,
    INVALID,
    INCOMPLETE,
};

// atmCoeff     // atmospheric attenuation
// rainCoeff    // rain/fog attenuation
// illumination // day/night factor
// glintFactor  // reflection boost

struct Surrounding {
    float k_atm      = 1.0;  // atmospheric attenuation
    float k_rain     = 1.0;  // rain/fog attenuation %
    float k_fog      = 1.0;  // Fog     %
    float k_humidity = 1.0;  // Humidity %
};
struct Coordinate {
    float latitude = NULL;
    float longitude= NULL;
    float altitude = NULL;
};

struct Axis {
    float x = NULL, y = NULL, z = NULL;
    float magnitude() const { return std::sqrt(x*x + y*y + z*z); }
};
struct Vec2 {
    float x, y;

    Vec2(float x_=0, float y_=0)
        : x(x_), y(y_) {}
};

struct Vec3 {
    float x, y, z;
    Vec3(float x_=0, float y_=0, float z_=0)
        : x(x_), y(y_), z(z_) {}
    // Normalize vector
    Vec3 normalized() const {
        float mag = std::sqrt(x*x + y*y + z*z);
        if (mag == 0) return Vec3(0,0,0);
        return Vec3(x/mag, y/mag, z/mag);
    }
    // Vector subtraction
    Vec3 operator-(const Vec3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    // Scalar division
    Vec3 operator/(float s) const {
        return {x / s, y / s, z / s};
    }

    // Scalar multiplication
    Vec3 operator*(float s) const {
        return {x * s, y * s, z * s};
    }

    // Length
    float length() const {
        return sqrt(x*x + y*y + z*z);
    }
};
struct EntityDimension {
    float length = 1.0;
    float width  = 1.0;
    float heigth = 1.0;
};

struct CustomEntity {
    Coordinate coordinate;
    Axis       axis;
    float heading = NULL;
    float pitch   = NULL;
    float illumination  = 1.0;
    float glintFactor   = 1.0;  // reflection boost
    float projectedLength = NULL;
    float projectedWidth  = NULL;
    std::string color           = "";
    std::string backgroundcolor = "";
    float frontalSurfaceArea = NULL;
    float temperature =  NULL;
};

struct Lens{
    float focalLength = NULL;
    float distortion  = NULL;
};
struct eoSensorPayload{
    float radius     = NULL;

    float detectionThreshold  = 0.000000008;
    float sensorGain = 1.0;
    float maxRange   = 50000;
    float fov        = 50;

    // float gridLength = NULL;
    // float gridWidth  = NULL;
    // float wavelengthMin = NULL;
    // float wavelengthMax = NULL;
};
struct EOParameters{
    Lens lens;
    eoSensorPayload sensor;
};
struct IRParameters{
    Lens lens;
    eoSensorPayload sensor;
};
using EntityList = std::unordered_map<std::string,CustomEntity>;

struct PayLoad{
    CustomEntity user;
    Surrounding surrounding;
    EntityList  entityList;
    EOParameters eoParameters;
    IRParameters irParameters;
};

struct PreProcessEntity {
    float distanceBtwUser = NULL;
    float angleBtwUser    = NULL;
    float frontalSurfaceArea = NULL;
    float visibility  =  NULL;//Not Useded
    float reflection  =  NULL;//Not Useded
    // float projectedLength = NULL;
    // float projectedWidth  = NULL;
};
using PreProcessEntityList = std::unordered_map<std::string,PreProcessEntity>;

struct EO_PayLoad{
    Surrounding surrounding;
    EntityList  entityList;
    PreProcessEntityList preProcessEntityList;
    EOParameters eoParameters;
};

struct IR_PayLoad{
    Surrounding surrounding;
    EntityList  entityList;
    PreProcessEntityList preProcessEntityList;
    IRParameters irParameters;
};

struct PostProcessEntity {
    float distanceBtwUser = NULL;
    float angleBtwUser    = NULL;
    float frontalSurfaceArea = NULL;
    // float projectedLength = NULL;
    // float projectedWidth  = NULL;
    // float visibility  =  NULL;
    // float reflection  =  NULL;
};

using PostProcessEntityList = std::unordered_map<std::string,PostProcessEntity>;


// ==========================================
// 1. Parameter Structures
// ==========================================

struct EOIR_Environment {
    float relativeHumidity;       // % (0.0 to 100.0)
    float absoluteHumidity;       // g/m^3
    float rainfallRate;           // mm/hr
    float snowfallEquivalent;     // mm/hr
    float ambientTemp;            // Celsius
    float backgroundTemp;         // Celsius
    float aerosolConcentration;   // mg/m^3
    float baseExtinctionCoeff;    // Base sigma (1/km)
    float ambientIlluminance;     // lux
    float solarIrradiance;        // W/m^2 (Crucial for Glint)
};

struct EOIR_Target {
    float surfaceTemp;            // Celsius
    float specularReflectivity;   // 0.0 to 1.0 (e.g., 0.05 for matte paint, 0.9 for glass/polished metal)
};

struct EOIR_Sensor {
    float slantRange;             // km
    float mrtd;                   // Minimum Resolvable Temp Difference (Celsius)
    float sunPhaseAngle;          // Degrees (0 = perfect alignment for direct reflection into sensor)
    float saturationLimit;        // Max apparent delta-T before the sensor blooms/blinds (Celsius)
};

// Updated struct using float for sub-pixel accuracy
struct ScreenTarget {
    float x;          // Center X coordinate
    float y;          // Center Y coordinate
    float width;      // Bounding box width in pixels
    float height;     // Bounding box height in pixels
    bool isOnScreen;  // True if at least partially visible
};

struct ProjectedExtents {
    float maxHorizontal;
    float maxVertical;
};


#endif // PAYLOAD_H
