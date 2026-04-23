// =============================================================================
// FILE:        vector.h
// MODULE:      3D Vector Mathematics
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Vector class, a 3D vector (x, y, z) with common
//              mathematical operations. Provides magnitude, normalisation,
//              dot/cross product, distance, linear interpolation, angle,
//              clamping, reflection, projection, and arithmetic operators.
//              Supports JSON serialisation for persistence.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef VECTOR_H
#define VECTOR_H

#include <QObject>
#include <QJsonObject>

// =============================================================================
// CLASS: Vector
//
// DESCRIPTION: Represents a 3D vector with x, y, z components. Provides a
//              comprehensive set of vector operations similar to Unity's
//              Vector3. Supports arithmetic operators and JSON serialisation.
// =============================================================================
class Vector/*: public QObject*/   // QObject inheritance commented out
{
    // Q_OBJECT                      // Qt macro disabled

public:
    explicit Vector(float x = 0.0f, float y = 0.0f, float z = 0.0f/*, QObject* parent = nullptr*/);

    // =========================================================================
    // SECTION: Vector Components
    // DESCRIPTION: Cartesian coordinates.
    // =========================================================================
    float x;    //!< X component (right/left)
    float y;    //!< Y component (up/down)
    float z;    //!< Z component (forward/back)

    // =========================================================================
    // SECTION: Vector Properties
    // DESCRIPTION: Magnitude and normalisation methods.
    // =========================================================================
    float magnitude() const;        //!< Length of the vector
    float sqrMagnitude() const;     //!< Squared length (avoids sqrt)
    Vector normalized() const;      //!< Returns unit vector (non‑modifying)
    float magnitudeSq() const;      //!< Alias for sqrMagnitude
    Vector normalised() const;      //!< Alias for normalized (alternate spelling)
    float dot(const Vector& other) const;   //!< Dot product with another vector

    void normalize();               //!< Normalises this vector in‑place

    // =========================================================================
    // SECTION: Static Utility Methods
    // DESCRIPTION: Common vector operations (Unity‑style).
    // =========================================================================
    static float Dot(const Vector& a, const Vector& b);     //!< Dot product of a and b
    static Vector Cross(const Vector& a, const Vector& b);  //!< Cross product of a and b
    static float Distance(const Vector& a, const Vector& b); //!< Euclidean distance between a and b
    static Vector Lerp(const Vector& a, const Vector& b, float t); //!< Linear interpolation
    static float Angle(const Vector& a, const Vector& b);   //!< Angle between vectors (degrees)
    static Vector ClampMagnitude(const Vector& vector, float maxLength); //!< Clamps vector length
    static Vector Reflect(const Vector& inDirection, const Vector& inNormal); //!< Reflects direction off normal
    static Vector Project(const Vector& a, const Vector& b); //!< Projects a onto b

    // =========================================================================
    // SECTION: Arithmetic Operators
    // DESCRIPTION: In‑place and binary operators.
    // =========================================================================
    Vector& operator-=(const Vector& other);
    Vector& operator*=(float scalar);
    Vector& operator+=(const Vector& other);

    Vector operator+(const Vector& other) const;
    Vector operator-(const Vector& other) const;
    Vector operator*(float scalar) const;
    Vector operator/(float scalar) const;
    Vector operator-() const;               //!< Negation

    // Serialization
    QJsonObject toJson();                   //!< Serialises vector to JSON
    void fromJson(const QJsonObject &obj);  //!< Deserialises vector from JSON
};

#endif // VECTOR_H
