#include "vector.h"
#include <cmath>


// Constructor Definition
Vector::Vector(float x, float y, float z/*, QObject* parent*/) :
    /*QObject(parent),*/ x(x), y(y), z(z)
{

}

// Magnitude and Normalize
float Vector::magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
}

float Vector::sqrMagnitude() const {
    return x * x + y * y + z * z;
}

Vector Vector::normalized() const {
    float mag = magnitude();
    return (mag == 0) ? Vector(0, 0, 0) : Vector(x / mag, y / mag, z / mag);
}

void Vector::normalize() {
    float mag = magnitude();
    if (mag != 0) {
        x /= mag;
        y /= mag;
        z /= mag;
    }
}

// Math
float Vector::Dot(const Vector& a, const Vector& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector Vector::Cross(const Vector& a, const Vector& b) {
    return Vector(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
        );
}

float Vector::Distance(const Vector& a, const Vector& b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) +
                     (a.y - b.y) * (a.y - b.y) +
                     (a.z - b.z) * (a.z - b.z));
}

Vector Vector::Lerp(const Vector& a, const Vector& b, float t) {
    t = std::fmax(0.0f, std::fmin(1.0f, t));
    return Vector(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
        );
}

float Vector::Angle(const Vector& a, const Vector& b) {
    float dot = Dot(a, b);
    float mags = a.magnitude() * b.magnitude();
    return std::acos(dot / mags) * (180.0f / M_PI);
}

Vector Vector::ClampMagnitude(const Vector& v, float maxLength) {
    float mag = v.magnitude();
    if (mag > maxLength)
        return v.normalized() * maxLength;
    return v;
}

Vector Vector::Reflect(const Vector& inDir, const Vector& normal) {
    float dot = Dot(normal, inDir);
    return inDir - normal * (2.0f * dot);
}

Vector Vector::Project(const Vector& a, const Vector& b) {
    float dotAB = Dot(a, b);
    float dotBB = Dot(b, b);
    if (dotBB == 0.0f) return Vector(0, 0, 0);
    return b * (dotAB / dotBB);
}

// Operators
Vector Vector::operator+(const Vector& other) const {
    return Vector(x + other.x, y + other.y, z + other.z);
}

Vector Vector::operator-(const Vector& other) const {
    return Vector(x - other.x, y - other.y, z - other.z);
}

Vector Vector::operator*(float scalar) const {
    return Vector(x * scalar, y * scalar, z * scalar);
}

Vector Vector::operator/(float scalar) const {
    return (scalar == 0.0f) ? Vector(0, 0, 0) : Vector(x / scalar, y / scalar, z / scalar);
}

Vector Vector::operator-() const {
    return Vector(-x, -y, -z);
}

// Yeh function tumhare physics simulation ke dot products ke liye use hoga
float Vector::dot(const Vector& other) const {
    return x * other.x + y * other.y + z * other.z;
}

// Squared magnitude for optimisation (tumne sqrMagnitude banaya hai, bas naam match karna zaroori hai)
float Vector::magnitudeSq() const {
    return x * x + y * y + z * z;
}

// Normalised vector return karta hai (yeh already hai as 'normalized', just ensure naming consistency)
Vector Vector::normalised() const {
    float mag = magnitude();
    return (mag == 0.0f) ? Vector(0, 0, 0) : Vector(x / mag, y / mag, z / mag);
}

// Compound assignment (+=) for velocity updates
Vector& Vector::operator+=(const Vector& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vector& Vector::operator-=(const Vector& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vector& Vector::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

QJsonObject Vector::toJson(){
    QJsonObject obj;
    obj["type"] = "vector";
    obj["x"] = x;
    obj["y"] = y;
    obj["z"] = z;
    return obj;
}

void Vector::fromJson(const QJsonObject &obj)
{
    if (obj.contains("x"))
        x = obj["x"].toVariant().toDouble();
    if (obj.contains("y"))
        y = obj["y"].toVariant().toDouble();
    if (obj.contains("z"))
        z = obj["z"].toVariant().toDouble();
}
