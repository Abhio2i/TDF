#include "eotargets.h"

EOTargets::EOTargets()
{
}

EOTargets::~EOTargets()
{
    // Clean up all cached GPU assets
    for (auto& asset : assetCache) {
        if (asset.texture) delete asset.texture;
        if (asset.vao) { asset.vao->destroy(); delete asset.vao; }
        if (asset.vbo) { asset.vbo->destroy(); delete asset.vbo; }
    }
}

void EOTargets::initialize()
{
    initializeOpenGLFunctions();
    initShaders();

    // Pre-load the default model so there is no stutter when the first frame renders
    getAsset(defaultModel3d, defaultTexture);
}

// --- ASSET MANAGER (The Caching Engine) ---
EOTargets::TargetAsset* EOTargets::getAsset(const QString& objPath, const QString& texPath)
{
    QString key = objPath + "|" + texPath;

    // 1. If it's already in the GPU, return it instantly!
    if (assetCache.contains(key)) {
        return &assetCache[key];
    }

    // 2. Otherwise, we need to load it.
    qDebug() << "Loading new asset into GPU:" << objPath;
    TargetAsset asset;
    asset.vao = new QOpenGLVertexArrayObject();
    asset.vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);

    // Load Texture safely
    QImage img(texPath);
    if (!img.isNull()) {
        asset.texture = new QOpenGLTexture(img.mirrored(false, true));
        asset.texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        asset.texture->setMagnificationFilter(QOpenGLTexture::Linear);
    } else {
        qDebug() << "Warning: Failed to load texture:" << texPath;
        asset.texture = nullptr;
    }

    // Load OBJ
    QVector<VertexData> loaded_vertices;
    loadOBJ(objPath, loaded_vertices);
    asset.vertexCount = loaded_vertices.size();

    // Push geometry to GPU
    if (asset.vertexCount > 0) {
        asset.vao->create();
        asset.vao->bind();

        asset.vbo->create();
        asset.vbo->bind();
        asset.vbo->allocate(loaded_vertices.constData(), loaded_vertices.size() * sizeof(VertexData));

        program.bind();
        int vertexLocation = program.attributeLocation("position");
        program.enableAttributeArray(vertexLocation);
        program.setAttributeBuffer(vertexLocation, GL_FLOAT, offsetof(VertexData, position), 3, sizeof(VertexData));

        int texCoordLocation = program.attributeLocation("texCoord");
        program.enableAttributeArray(texCoordLocation);
        program.setAttributeBuffer(texCoordLocation, GL_FLOAT, offsetof(VertexData, texCoord), 2, sizeof(VertexData));

        asset.vao->release();
        asset.vbo->release();
        program.release();
    }

    // Store in cache and return
    assetCache.insert(key, asset);
    return &assetCache[key];
}

void EOTargets::render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix,
                       float tx, float ty, float tz,
                       const QQuaternion &rotation,
                       const QString &modelPath, const QString &texturePath)
{
    // Check if telemetry paths are empty. If so, substitute defaults natively.
    QString finalModel = modelPath.isEmpty() ? defaultModel3d : modelPath;
    QString finalTex = texturePath.isEmpty() ? defaultTexture : texturePath;

    // Grab asset from cache
    TargetAsset* asset = getAsset(finalModel, finalTex);
    if (!asset || asset->vertexCount == 0) return; // Skip if loading failed entirely

    QMatrix4x4 airplaneMatrix = cameraMatrix;

    airplaneMatrix.translate(tx, ty, tz);
    airplaneMatrix.rotate(rotation);

    // Model orientation fixes
    airplaneMatrix.rotate(90.0f, 0.0f, 1.0f, 0.0f);
    airplaneMatrix.rotate(-90.0f, 1.0f, 0.0f, 0.0f);
    airplaneMatrix.scale(0.02f);

    // --- Draw ---
    program.bind();
    program.setUniformValue("mvpMatrix", projection * airplaneMatrix);

    glActiveTexture(GL_TEXTURE0);
    if (asset->texture) {
        asset->texture->bind();
    }
    program.setUniformValue("textureSampler", 0);

    asset->vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, asset->vertexCount);
    asset->vao->release();

    if (asset->texture) {
        asset->texture->release();
    }
    program.release();
}


void EOTargets::initShaders()
{
    const char *vsrc =
        "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "layout(location = 1) in vec2 texCoord;\n"
        "out vec2 vTexCoord;\n"
        "uniform mat4 mvpMatrix;\n"
        "void main() {\n"
        "   vTexCoord = texCoord;\n"
        "   gl_Position = mvpMatrix * vec4(position, 1.0);\n"
        "}\n";

    const char *fsrc =
        "#version 330 core\n"
        "in vec2 vTexCoord;\n"
        "out vec4 fragColor;\n"
        "uniform sampler2D textureSampler;\n"
        "void main() {\n"
        "   fragColor = texture(textureSampler, vTexCoord);\n"
        "}\n";

    program.addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);
    program.addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);
    program.link();
}

// Slightly modified to push data into 'outVertices' instead of a class member
bool EOTargets::loadOBJ(const QString &filePath, QVector<VertexData> &outVertices)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open OBJ file:" << filePath;
        return false;
    }

    QVector<QVector3D> temp_positions;
    QVector<QVector2D> temp_texCoords;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QStringList tokens = line.split(' ', Qt::SkipEmptyParts);
        if (tokens.isEmpty()) continue;

        if (tokens[0] == "v") {
            temp_positions.append(QVector3D(tokens[1].toFloat(), tokens[2].toFloat(), tokens[3].toFloat()));
        }
        else if (tokens[0] == "vt") {
            temp_texCoords.append(QVector2D(tokens[1].toFloat(), tokens[2].toFloat()));
        }
        else if (tokens[0] == "f") {
            if (tokens.size() >= 4) {
                for (int i = 2; i < tokens.size() - 1; ++i) {
                    QStringList indices[3];
                    indices[0] = tokens[1].split('/');
                    indices[1] = tokens[i].split('/');
                    indices[2] = tokens[i + 1].split('/');

                    for (int j = 0; j < 3; ++j) {
                        QStringList &vertexParts = indices[j];
                        int vIndex = vertexParts[0].toInt() - 1;
                        int vtIndex = (vertexParts.size() > 1 && !vertexParts[1].isEmpty()) ? vertexParts[1].toInt() - 1 : -1;

                        if (vIndex >= 0 && vIndex < temp_positions.size()) {
                            VertexData vData;
                            vData.position = temp_positions[vIndex];
                            vData.texCoord = (vtIndex >= 0 && vtIndex < temp_texCoords.size()) ? temp_texCoords[vtIndex] : QVector2D(0.0f, 0.0f);

                            // Load into the parameter vector!
                            outVertices.append(vData);
                        }
                    }
                }
            }
        }
    }
    return true;
}
