#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector>
#include <QLabel>
#include <QWidget>

class AttitudeIndicator : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    // Flight data (degrees / knots / feet) - plugin.cpp inhe set karta hai
    float pitch;
    float roll;
    float speed;
    float altitude;
    float heading;

    explicit AttitudeIndicator(QWidget *parent = nullptr);
    ~AttitudeIndicator() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    struct Face {
        QVector<QVector3D> pts;
        QVector3D col;
    };

    void buildAircraft();
    void updatePanel();
    static void addTri(QVector<float> &v, const QVector3D &a, const QVector3D &b,
                       const QVector3D &c, const QVector3D &col);

    QVector<Face> m_faces;
    QOpenGLShaderProgram *m_prog = nullptr;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};

    QWidget *m_panel = nullptr;
    QLabel *m_pitchLbl = nullptr;
    QLabel *m_rollLbl = nullptr;
    QLabel *m_headLbl = nullptr;
    QLabel *m_speedLbl = nullptr;
    QLabel *m_altLbl = nullptr;
    QLabel *m_vsLbl = nullptr;
};
