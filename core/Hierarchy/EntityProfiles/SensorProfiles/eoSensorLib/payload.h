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
    double k_atm      = 1.0;  // atmospheric attenuation
    double k_rain     = 1.0;  // rain/fog attenuation %
    double k_fog      = 1.0;  // Fog     %
    double k_humidity = 1.0;  // Humidity %
};
struct Coordinate {
    double latitude = NULL;
    double longitude= NULL;
    double altitude = NULL;
};

struct Axis {
    double x = NULL, y = NULL, z = NULL;
    double magnitude() const { return std::sqrt(x*x + y*y + z*z); }
};
struct Vec2 {
    double x, y;

    Vec2(double x_=0, double y_=0)
        : x(x_), y(y_) {}
};

struct Vec3 {
    double x, y, z;
    Vec3(double x_=0, double y_=0, double z_=0)
        : x(x_), y(y_), z(z_) {}
    // Normalize vector
    Vec3 normalized() const {
        double mag = std::sqrt(x*x + y*y + z*z);
        if (mag == 0) return Vec3(0,0,0);
        return Vec3(x/mag, y/mag, z/mag);
    }
    // Vector subtraction
    Vec3 operator-(const Vec3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    // Scalar division
    Vec3 operator/(double s) const {
        return {x / s, y / s, z / s};
    }

    // Scalar multiplication
    Vec3 operator*(double s) const {
        return {x * s, y * s, z * s};
    }

    // Length
    double length() const {
        return sqrt(x*x + y*y + z*z);
    }
};
struct EntityDimension {
    double length = 1.0;
    double width  = 1.0;
    double heigth = 1.0;
};

struct CustomEntity {
    Coordinate coordinate;
    Axis       axis;
    double heading = NULL;
    double pitch   = NULL;
    double illumination  = 1.0;
    double glintFactor   = 1.0;  // reflection boost
    double projectedLength = NULL;
    double projectedWidth  = NULL;
    std::string color           = "";
    std::string backgroundcolor = "";
    double frontalSurfaceArea = NULL;
    double temperature =  NULL;
};

struct Lens{
    double focalLength = NULL;
    double distortion  = NULL;
};
struct CustomSensor{
    double radius     = NULL;

    double detectionThreshold  = 0.000000008;
    double sensorGain = 1.0;
    double maxRange   = 50000;
    double fov        = 50;

    // double gridLength = NULL;
    // double gridWidth  = NULL;
    // double wavelengthMin = NULL;
    // double wavelengthMax = NULL;
};
struct EOParameters{
    Lens lens;
    CustomSensor sensor;
};
struct IRParameters{
    Lens lens;
    CustomSensor sensor;
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
    double distanceBtwUser = NULL;
    double angleBtwUser    = NULL;
    double frontalSurfaceArea = NULL;
    double visibility  =  NULL;//Not Useded
    double reflection  =  NULL;//Not Useded
    // double projectedLength = NULL;
    // double projectedWidth  = NULL;
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
    double distanceBtwUser = NULL;
    double angleBtwUser    = NULL;
    double frontalSurfaceArea = NULL;
    // double projectedLength = NULL;
    // double projectedWidth  = NULL;
    // double visibility  =  NULL;
    // double reflection  =  NULL;
};

using PostProcessEntityList = std::unordered_map<std::string,PostProcessEntity>;
#endif // PAYLOAD_H
