#ifndef IRTARGETS_H
#define IRTARGETS_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QFile>
#include <QTextStream>
#include <QVector3D>
#include <QVector2D>
#include <QDebug>

struct IRVertexData {
    QVector3D position;
    QVector2D texCoord;
};

class IRTargets : protected QOpenGLFunctions
{
public:
    IRTargets();
    ~IRTargets();

    // Call inside initializeGL()
    void initialize();

    // --> NEW: Added translation (tx, ty, tz) and rotation (rx, ry, rz)
    void render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix,
                float tx, float ty, float tz,
                float rx, float ry, float rz);

    // Call inside paintGL()
    void render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix);

private:
    void initShaders();
    bool loadOBJ(const QString &filePath);

    QOpenGLShaderProgram program;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLTexture *texture = nullptr;
    int vertexCount;
};

#endif // IRTARGETS_H
