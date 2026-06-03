#ifndef EOLAND_H
#define EOLAND_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>

class EOLand : protected QOpenGLFunctions
{
public:
    EOLand();
    ~EOLand();

    void initialize();

    // --> NEW: Added rx, ry, rz parameters
    void render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix,
                float altitude, float rx, float ry, float rz);

private:
    QOpenGLShaderProgram groundProgram;
    QOpenGLVertexArrayObject groundVao;
    QOpenGLBuffer groundVbo;
};

#endif // EOLAND_H
