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

class Transform : public QObject, public Component
{
    Q_OBJECT
public:
    Transform();
    ComponentType Typo() const override { return ComponentType::Transform; }
    bool Active;

    Geocords* geocord;
    Qt3DCore::QTransform* matrix;
    // Use QVector3D and QQuaternion directly

    QVector3D toEulerAngles() const;
    void setFromEulerAngles(const QVector3D& eulerAngles);

    QJsonObject customParameters;
    std::vector<QVector3D> trailData;

    void setGeoCord(float lat, float lon);
    void setGeoCord(float lat, float lon, float alt);
    void setGeoCord(float lat, float lon, float alt, float heading);

    void setLatitude(float lat);
    float getLatitude();

    void setLongitude(float lon);
    float getLongitude();

    void setAltitude(float alt);
    float getAltitude();

    void setHeading(float heading);
    float getHeading();


    void setTranslation(const QVector3D& vector);
    void addTranslation(const QVector3D& vector);
    QVector3D translation() const;
    void setRotation(const QQuaternion& quat);
    QQuaternion rotation();
    void setScale3D(const QVector3D& vector);
    QVector3D scale3D();

    // Directional methods using Quaternion math
    float pitch();
    float roll();
    float yaw();
    QVector3D forward();
    QVector3D right();
    QVector3D up();
    QVector3D back();
    QVector3D left();
    QVector3D down();

    void lookAt(const QVector3D& targetWorldPos);
    void lookAt3D(const QVector3D& targetWorldPos);

    QVector3D inverseTransformDirection(const QVector3D& worldDir);
    QVector3D TransformDirection(const QVector3D& localDir);

    QVector3D inverseTransformVector(const QVector3D& worldVec);
    QVector3D inverseTransformPoint(const QVector3D& worldPos);

    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QString data3 = "") override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
    // --- Conversion for Network (TransformPDU) ---
    void toPDU(TransformPDU& pdu, const std::string& entityID, const std::string& parentID) const;
    void fromPDU(const TransformPDU& pdu);

public slots:
    void sync();
    void invokesync();

private:
    QVector3D positionbuffer;
    QQuaternion rotationbuffer;
    bool PosUpdate = false;
    bool RotUpdate = false;
    void VectorChanged(QVector3D v);
    void rotationChanged(QQuaternion r);
};

#endif // TRANSFORM_H
