#include "irvisionangle.h"

IRVisionAngle::IRVisionAngle()
    : m_position(0.0f, 0.0f, -50.0f)
{
}

void IRVisionAngle::setPosition(const QVector3D &position)
{
    m_position = position;
}

void IRVisionAngle::setRotation(const QQuaternion &rotation)
{
    m_rotation = rotation;
}

QMatrix4x4 IRVisionAngle::getViewMatrix() const
{
    QMatrix4x4 viewMatrix;

    // Apply the exact opposite rotation of the camera to the world
    viewMatrix.rotate(m_rotation.conjugated());

    // Apply inverse translation (push world in the opposite direction)
    viewMatrix.translate(-m_position);

    return viewMatrix;
}
