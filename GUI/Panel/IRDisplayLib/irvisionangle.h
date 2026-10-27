#ifndef IRVISIONANGLE_H
#define IRVISIONANGLE_H

#include <QMatrix4x4>
#include <QVector3D>

class IRVisionAngle
{
public:
    IRVisionAngle();
    ~IRVisionAngle() = default;

    void setPosition(const QVector3D &position);
    void setRotation(const QQuaternion &rotation);

    QMatrix4x4 getViewMatrix() const;

private:
    QVector3D m_position;
    QQuaternion m_rotation; // Replaces m_pitch, m_yaw, m_roll
};

#endif // IRVISIONANGLE_H
