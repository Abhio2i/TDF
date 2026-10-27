#ifndef IRLAND_H
#define IRLAND_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector>
#include <QVector3D>

class IRLand : protected QOpenGLFunctions
{
public:
    IRLand();
    ~IRLand();

    void initialize();

    void render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix);

private:
    QOpenGLShaderProgram groundProgram;
    QOpenGLVertexArrayObject groundVao;
    QOpenGLBuffer groundVbo;

    int vertexCount; // --> NEW: Tracks the size of our generated mesh
};

#endif // IRLAND_H
