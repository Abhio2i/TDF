#include "irtargets.h"

IRTargets::IRTargets()
    : vbo(QOpenGLBuffer::VertexBuffer), vertexCount(0)
{
}

IRTargets::~IRTargets()
{
    if (texture) {
        delete texture;
    }
    vao.destroy();
    vbo.destroy();
}

void IRTargets::initialize()
{
    initializeOpenGLFunctions();

    initShaders();
    loadOBJ("/home/o2i/Documents/Airplane_v1_L1/11803_Airplane_v1_l1.obj");

    texture = new QOpenGLTexture(QImage("/home/o2i/Documents/Airplane_v1_L1/11803_Airplane_body_diff.jpg").mirrored(false, true));
    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);

    vao.create();
    vao.bind();
    program.bind();
    vbo.bind();

    int vertexLocation = program.attributeLocation("position");
    program.enableAttributeArray(vertexLocation);
    program.setAttributeBuffer(vertexLocation, GL_FLOAT, offsetof(IRVertexData, position), 3, sizeof(IRVertexData));

    int texCoordLocation = program.attributeLocation("texCoord");
    program.enableAttributeArray(texCoordLocation);
    program.setAttributeBuffer(texCoordLocation, GL_FLOAT, offsetof(IRVertexData, texCoord), 2, sizeof(IRVertexData));

    vao.release();
    vbo.release();
    program.release();
}

// ... (initialize, initShaders, loadOBJ stay the same)

void IRTargets::render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix,
                       float tx, float ty, float tz,
                       float rx, float ry, float rz)
{
    QMatrix4x4 airplaneMatrix = cameraMatrix;

    // 1. Move the target to its location relative to the sensor
    airplaneMatrix.translate(tx, ty, tz);

    // 2. Apply the target's dynamic rotation (Yaw, Pitch, Roll)
    airplaneMatrix.rotate(ry, 0.0f, 1.0f, 0.0f); // Yaw (Y-axis)
    airplaneMatrix.rotate(rx, 1.0f, 0.0f, 0.0f); // Pitch (X-axis)
    airplaneMatrix.rotate(rz, 0.0f, 0.0f, 1.0f); // Roll (Z-axis)

    // 3. Apply model-specific orientation to fix how the OBJ was exported
    airplaneMatrix.rotate(-90.0f, 1.0f, 0.0f, 0.0f); // Stand upright
    airplaneMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f); // Point nose forward

    // 4. Shrink the model
    airplaneMatrix.scale(0.02f);

    // --- Draw the Model ---
    program.bind();
    program.setUniformValue("mvpMatrix", projection * airplaneMatrix);

    glActiveTexture(GL_TEXTURE0);
    if (texture) {
        texture->bind();
    }
    program.setUniformValue("textureSampler", 0);

    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    vao.release();

    if (texture) {
        texture->release();
    }
    program.release();
}

void IRTargets::render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix)
{
    QMatrix4x4 airplaneMatrix = cameraMatrix;

    // Apply model-specific orientation and scaling
    airplaneMatrix.rotate(90.0f, 1.0f, 0.0f, 0.0f); // Stand upright
    airplaneMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f); // Point nose at camera
    airplaneMatrix.scale(0.02f);                     // Shrink

    program.bind();
    program.setUniformValue("mvpMatrix", projection * airplaneMatrix);

    glActiveTexture(GL_TEXTURE0);
    if (texture) {
        texture->bind();
    }
    program.setUniformValue("textureSampler", 0);

    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    vao.release();

    if (texture) {
        texture->release();
    }
    program.release();
}

void IRTargets::initShaders()
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

bool IRTargets::loadOBJ(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open OBJ file:" << filePath;
        return false;
    }

    QVector<QVector3D> temp_positions;
    QVector<QVector2D> temp_texCoords;
    QVector<IRVertexData> loaded_vertices;

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
                        int vtIndex = -1;

                        if (vertexParts.size() > 1 && !vertexParts[1].isEmpty()) {
                            vtIndex = vertexParts[1].toInt() - 1;
                        }

                        if (vIndex >= 0 && vIndex < temp_positions.size()) {
                            IRVertexData vData;
                            vData.position = temp_positions[vIndex];

                            if (vtIndex >= 0 && vtIndex < temp_texCoords.size()) {
                                vData.texCoord = temp_texCoords[vtIndex];
                            } else {
                                vData.texCoord = QVector2D(0.0f, 0.0f);
                            }
                            loaded_vertices.append(vData);
                        }
                    }
                }
            }
        }
    }

    vertexCount = loaded_vertices.size();

    vbo.create();
    vbo.bind();
    vbo.allocate(loaded_vertices.constData(), loaded_vertices.size() * sizeof(IRVertexData));
    return true;
}
