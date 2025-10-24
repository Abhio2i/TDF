#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <QObject>
#include "./component.h"
#include "core/Hierarchy/Struct/geocords.h"
#include <QJsonObject>
#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>
#include <QVariant>
#include <Qt3DCore/QTransform>
class Transform : public QObject, public Component
{
    Q_OBJECT
public:
    Transform();
    ComponentType Typo() const override { return ComponentType::Transform; }
    bool Active;
    std::string ID;

    Geocords* geocord;
    Qt3DCore::QTransform* matrix;
    // Use QVector3D and QQuaternion directly

    QVector3D toEulerAngles() const;
    void setFromEulerAngles(const QVector3D& eulerAngles);

    QJsonObject customParameters;


    void setTranslation(const QVector3D& vector);
    void addTranslation(const QVector3D& vector);
    QVector3D translation();
    void setRotation(const QQuaternion& quat);
    QQuaternion rotation();
    void setScale3D(const QVector3D& vector);
    QVector3D scale3D();

    // Directional methods using Quaternion math
    QVector3D forward();
    QVector3D right();
    QVector3D up();
    QVector3D back();
    QVector3D left();
    QVector3D down();

    QVector3D inverseTransformDirection(const QVector3D& worldDir);
    QVector3D TransformDirection(const QVector3D& localDir);

    QVector3D inverseTransformVector(const QVector3D& worldVec);
    QVector3D inverseTransformPoint(const QVector3D& worldPos);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // TRANSFORM_H
