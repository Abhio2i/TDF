// =============================================================================
// FILE:        transform.h
// MODULE:      Spatial Transformation
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Transform class, which manages the position,
//              orientation, and scale of an entity in 3D space. Provides
//              geographic coordinate (latitude/longitude/altitude) support,
//              Euler/quaternion conversions, directional vectors, look-at
//              functionality, and network PDU conversion for DIS/HLA
//              interoperability.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added Qt3D integration and network PDU methods.
//   Rev 3  [Date]  Added trail data and custom parameters.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <QObject>
#include "./component.h"
#include "core/Hierarchy/Struct/geocords.h"
#include "qmutex.h"
#include <QJsonObject>
#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>
#include <QVariant>
#include <Qt3DCore/QTransform>
#include <core/Network/transformpdu.h>

// =============================================================================
// CLASS: Transform
//
// DESCRIPTION: Component that defines an entity's spatial properties:
//              geographic coordinates (lat/lon/alt), local translation,
//              rotation (quaternion/Euler), and scaling. Provides methods
//              for direction vectors (forward, right, up, etc.), look-at
//              targeting, and coordinate transformations between local and
//              world space. Supports network synchronisation via TransformPDU.
// =============================================================================
class Transform : public QObject, public Component
{
    Q_OBJECT
public:
    Transform();
    ComponentType Typo() const override { return ComponentType::Transform; }

    // =========================================================================
    // SECTION: Core State
    // DESCRIPTION: Activation flag and geographic/transform data.
    // =========================================================================
    bool Active;                            //!< Whether this transform is active
    Geocords* geocord;                      //!< Geographic coordinates (lat/lon/alt/heading)
    Qt3DCore::QTransform* matrix;           //!< Qt3D transform matrix (for 3D rendering)

    // =========================================================================
    // SECTION: Euler/Quaternion Conversions
    // DESCRIPTION: Methods to convert between quaternion and Euler angles.
    // =========================================================================
    QVector3D toEulerAngles() const;        //!< Converts current rotation to Euler (pitch, roll, yaw)
    void setFromEulerAngles(const QVector3D& eulerAngles); //!< Sets rotation from Euler angles

    // =========================================================================
    // SECTION: Custom Data
    // DESCRIPTION: Extensible parameters and historical trail data.
    // =========================================================================
    QJsonObject AdditionalParameters;            //!< User-defined key-value parameters
    std::vector<QVector3D> trailData;       //!< Historical position trail for path visualisation

    // =========================================================================
    // SECTION: Geographic Coordinate Setters/Getters
    // DESCRIPTION: Overloaded methods for latitude, longitude, altitude, heading.
    // =========================================================================
    void setGeoCord(float lat, float lon);                      //!< Sets lat/lon (alt=0, heading=0)
    void setGeoCord(float lat, float lon, float alt);           //!< Sets lat/lon/alt (heading=0)
    void setGeoCord(float lat, float lon, float alt, float heading); //!< Sets full geocord

    void setLatitude(float lat);            //!< Sets latitude only
    float getLatitude();                    //!< Returns latitude

    void setLongitude(float lon);           //!< Sets longitude only
    float getLongitude();                   //!< Returns longitude

    void setAltitude(float alt);            //!< Sets altitude only
    float getAltitude();                    //!< Returns altitude

    void setHeading(float heading);         //!< Sets heading (yaw) only
    float getHeading();                     //!< Returns heading

    // =========================================================================
    // SECTION: Local Transform Manipulation
    // DESCRIPTION: Methods for translation, rotation, and scaling.
    // =========================================================================
    void setTranslation(const QVector3D& vector);   //!< Sets local position
    void addTranslation(const QVector3D& vector);   //!< Adds offset to local position
    QVector3D translation() const;                  //!< Returns local position

    void setRotation(const QQuaternion& quat);      //!< Sets local rotation
    QQuaternion rotation();                         //!< Returns local rotation

    void setScale3D(const QVector3D& vector);       //!< Sets local scale
    QVector3D scale3D();                            //!< Returns local scale

    // =========================================================================
    // SECTION: Directional Vectors
    // DESCRIPTION: Returns unit vectors in world space derived from rotation.
    // =========================================================================
    float pitch();                  //!< Pitch angle (rotation around X axis) in degrees
    float roll();                   //!< Roll angle (rotation around Z axis) in degrees
    float yaw();                    //!< Yaw angle (rotation around Y axis) in degrees
    QVector3D forward();            //!< Forward direction vector
    QVector3D right();              //!< Right direction vector
    QVector3D up();                 //!< Up direction vector
    QVector3D back();               //!< Backward direction vector
    QVector3D left();               //!< Left direction vector
    QVector3D down();               //!< Down direction vector

    // =========================================================================
    // SECTION: Look-At & Direction Utilities
    // DESCRIPTION: Rotate to face a target point.
    // =========================================================================
    void lookAt(const QVector3D& targetWorldPos);   //!< Rotates to face target in world space
    void lookAt3D(const QVector3D& targetWorldPos); //!< 3D version (same as lookAt)

    // =========================================================================
    // SECTION: Coordinate Space Transformations
    // DESCRIPTION: Convert between local and world space.
    // =========================================================================
    QVector3D inverseTransformDirection(const QVector3D& worldDir); //!< World direction to local direction
    QVector3D TransformDirection(const QVector3D& localDir);        //!< Local direction to world direction

    QVector3D inverseTransformVector(const QVector3D& worldVec);    //!< World vector to local vector (preserves magnitude)
    QVector3D inverseTransformPoint(const QVector3D& worldPos);     //!< World point to local point

    QVector3D transformPoint(const QVector3D& localPos);

    // Component interface overrides
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

    // -------------------------------------------------------------------------
    // SECTION: Network PDU Conversion
    // DESCRIPTION: Converts between Transform and TransformPDU for DIS/HLA.
    // -------------------------------------------------------------------------
    void toPDU(TransformPDU& pdu, const std::string& entityID, const std::string& parentID) const;
    void fromPDU(const TransformPDU& pdu);

public slots:
    void sync();            //!< Synchronises transform state with associated components
    void invokesync();      //!< Invokes sync from another thread/context

private:
    QVector3D positionbuffer;   //!< Buffer for position updates
    QQuaternion rotationbuffer; //!< Buffer for rotation updates
    bool PosUpdate = false;     //!< Flag indicating pending position update
    bool RotUpdate = false;     //!< Flag indicating pending rotation update

    void VectorChanged(QVector3D v);    //!< Called when position changes
    void rotationChanged(QQuaternion r);//!< Called when rotation changes
};

#endif // TRANSFORM_H
