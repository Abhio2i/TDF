#ifndef EOLAND_H
#define EOLAND_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector>
#include <QVector3D>

class EOLand : protected QOpenGLFunctions
{
public:
    EOLand();
    ~EOLand();

    void initialize();
    void render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix);

private:
    QOpenGLShaderProgram groundProgram;
    QOpenGLVertexArrayObject groundVao;
    QOpenGLBuffer groundVbo;

    int vertexCount; // Tracks the size of our generated 3D mesh
};

#endif // EOLAND_H
