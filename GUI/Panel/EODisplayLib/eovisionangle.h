#ifndef EOVISIONANGLE_H
#define EOVISIONANGLE_H

#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>

class EOVisionAngle
{
public:
    EOVisionAngle();
    ~EOVisionAngle() = default;

    void setPosition(const QVector3D &position);
    void setRotation(const QQuaternion &rotation);

    QMatrix4x4 getViewMatrix() const;

private:
    QVector3D m_position;
    QQuaternion m_rotation; // Replaces m_pitch, m_yaw, m_roll
};

#endif // EOVISIONANGLE_H
