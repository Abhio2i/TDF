#ifndef IRVISIONANGLE_H
#define IRVISIONANGLE_H

#include <QMatrix4x4>
#include <QVector3D>

class IRVisionAngle
{
public:
    IRVisionAngle();
    ~IRVisionAngle() = default;

    // Setters for direct control
    void setPosition(const QVector3D &position);
    void setRotation(float pitch, float yaw, float roll);

    // Modifiers (useful if you want to add mouse/keyboard controls later)
    void translate(const QVector3D &offset);
    void rotate(float pitchOffset, float yawOffset, float rollOffset);

    // Generates the final View Matrix for the shaders
    QMatrix4x4 getViewMatrix() const;

private:
    QVector3D m_position;
    float m_pitch;
    float m_yaw;
    float m_roll;
};

#endif // IRVISIONANGLE_H
