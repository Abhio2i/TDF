#include "irvisionangle.h"

IRVisionAngle::IRVisionAngle()
    : m_position(0.0f, 0.0f, -50.0f), // Default: pushed back 50 units
    m_pitch(0.0f),
    m_yaw(0.0f),
    m_roll(0.0f)
{
}

void IRVisionAngle::setPosition(const QVector3D &position)
{
    m_position = position;
}

void IRVisionAngle::setRotation(float pitch, float yaw, float roll)
{
    m_pitch = pitch;
    m_yaw = yaw;
    m_roll = roll;
}

void IRVisionAngle::translate(const QVector3D &offset)
{
    m_position += offset;
}

void IRVisionAngle::rotate(float pitchOffset, float yawOffset, float rollOffset)
{
    m_pitch += pitchOffset;
    m_yaw += yawOffset;
    m_roll += rollOffset;
}

QMatrix4x4 IRVisionAngle::getViewMatrix() const
{
    QMatrix4x4 viewMatrix;

    // Apply camera rotations
    viewMatrix.rotate(m_pitch, 1.0f, 0.0f, 0.0f);
    viewMatrix.rotate(m_yaw,   0.0f, 1.0f, 0.0f);
    viewMatrix.rotate(m_roll,  0.0f, 0.0f, 1.0f);

    // Apply camera translation
    viewMatrix.translate(m_position);

    return viewMatrix;
}
