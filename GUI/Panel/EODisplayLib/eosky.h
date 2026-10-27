#ifndef EOSKY_H
#define EOSKY_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QImage>

class EOSky : protected QOpenGLFunctions
{
public:
    EOSky();
    ~EOSky();

    void initialize(const QString& panoramaPath);

    // Now requires the projection and camera matrices!
    void render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix);

private:
    QOpenGLShaderProgram skyProgram;
    QOpenGLVertexArrayObject skyVao;
    QOpenGLBuffer skyVbo;
    QOpenGLTexture *skyTexture = nullptr;
};

#endif // EOSKY_H
