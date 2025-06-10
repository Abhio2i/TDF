#ifndef VECTOR_H
#define VECTOR_H
#include <QObject>
#include <QJsonObject>
class Vector/*: public QObject*/
{
    // Q_OBJECT
public:
    explicit Vector(float x = 0.0f, float y = 0.0f, float z = 0.0f/*, QObject* parent = nullptr*/);

    float x;
    float y;
    float z;

public:
    // Unity-style Methods
    float magnitude() const;
    float sqrMagnitude() const;
    Vector normalized() const;
    float magnitudeSq() const;
    Vector normalised() const;
    float dot(const Vector& other) const;

    void normalize();

    static float Dot(const Vector& a, const Vector& b);
    static Vector Cross(const Vector& a, const Vector& b);
    static float Distance(const Vector& a, const Vector& b);
    static Vector Lerp(const Vector& a, const Vector& b, float t);
    static float Angle(const Vector& a, const Vector& b);
    static Vector ClampMagnitude(const Vector& vector, float maxLength);
    static Vector Reflect(const Vector& inDirection, const Vector& inNormal);
    static Vector Project(const Vector& a, const Vector& b);


    Vector& operator-=(const Vector& other);
    Vector& operator*=(float scalar);
    Vector& operator+=(const Vector& other);




    // Operators
    Vector operator+(const Vector& other) const;
    Vector operator-(const Vector& other) const;
    Vector operator*(float scalar) const;
    Vector operator/(float scalar) const;
    Vector operator-() const;

    QJsonObject toJson();
    void fromJson(const QJsonObject &obj);

};

#endif // VECTOR_H
