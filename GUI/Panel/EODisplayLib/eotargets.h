#ifndef EOTARGETS_H
#define EOTARGETS_H

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
#include <QMap>

struct VertexData {
    QVector3D position;
    QVector2D texCoord;
};

class EOTargets : protected QOpenGLFunctions
{
public:
    EOTargets();
    ~EOTargets();

    void initialize();

    // --> NEW: Accepts the model and texture paths from PoseGeo
    void render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix,
                float tx, float ty, float tz,
                const QQuaternion &rotation,
                const QString &modelPath, const QString &texturePath);

private:
    void initShaders();

    // Updated to output into a vector instead of a class member
    bool loadOBJ(const QString &filePath, QVector<VertexData> &outVertices);

    // --- Asset Management ---
    struct TargetAsset {
        QOpenGLVertexArrayObject* vao = nullptr;
        QOpenGLBuffer* vbo = nullptr;
        QOpenGLTexture* texture = nullptr;
        int vertexCount = 0;
    };

    // Cache storing GPU data keyed by the file paths
    QMap<QString, TargetAsset> assetCache;
    TargetAsset* getAsset(const QString& objPath, const QString& texPath);

    QOpenGLShaderProgram program;

    // Fallback defaults
    QString defaultModel3d = ":/model/airplane/Model/Airplane/11803_Airplane_v1_l1.obj";
    QString defaultTexture = ":/model/airplane/Model/Airplane/11803_Airplane_body_diff.jpg";
};

#endif // EOTARGETS_H
