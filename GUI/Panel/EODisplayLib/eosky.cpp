#include "eosky.h"

EOSky::EOSky() : skyVbo(QOpenGLBuffer::VertexBuffer)
{
}

EOSky::~EOSky()
{
    if (skyTexture) delete skyTexture;
    skyVao.destroy();
    skyVbo.destroy();
}

void EOSky::initialize(const QString& panoramaPath)
{
    initializeOpenGLFunctions();

    // NOTE: panoramaPath is intentionally unused now. The old texture-based
    // sky depended on ":/model/airplane/Model/Airplane/Environment.png" being
    // correctly registered in the .qrc file — if it wasn't, img.isNull() was
    // true, skyTexture never got created, and the whole sky rendered solid
    // black (which is what was happening). A procedural gradient sky has no
    // external resource dependency, so it can never fail to load, and it
    // looks correct from any viewing angle / altitude without needing an
    // artist-authored panorama.

    // 1. VERTEX SHADER
    // Pushes the cube vertices to the absolute maximum depth (Z = 1.0)
    const char *vsrc =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "out vec3 localPos;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "   localPos = aPos;\n"
        "   vec4 pos = projection * view * vec4(aPos, 1.0);\n"
        "   gl_Position = pos.xyww;\n"
        "}\n";

    // 2. FRAGMENT SHADER (Procedural Atmospheric Gradient)
    // Colors the sky based purely on the vertical component of the view
    // direction — deep blue overhead, lightening toward a pale horizon band,
    // with a soft sun-glow bias. No texture sampling required.
    const char *fsrc =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 localPos;\n"
        "void main() {\n"
        "   vec3 dir = normalize(localPos);\n"
        "   float h = clamp(dir.y, -1.0, 1.0);\n"

        "   vec3 zenithColor  = vec3(0.05, 0.20, 0.55);\n" // deep blue overhead
        "   vec3 horizonColor = vec3(0.65, 0.78, 0.92);\n" // pale hazy horizon
        "   vec3 groundGlow   = vec3(0.55, 0.50, 0.45);\n" // warm haze looking down

        "   vec3 color;\n"
        "   if (h >= 0.0) {\n"
        "       float t = pow(h, 0.45);\n" // faster falloff near horizon
        "       color = mix(horizonColor, zenithColor, t);\n"
        "   } else {\n"
        "       float t = pow(-h, 0.5);\n"
        "       color = mix(horizonColor, groundGlow, t);\n"
        "   }\n"

        "   FragColor = vec4(color, 1.0);\n"
        "}\n";

    skyProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);
    skyProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);
    skyProgram.link();

    // 3. BUILD THE CUBE (36 vertices)
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };

    skyVao.create();
    skyVao.bind();
    skyVbo.create();
    skyVbo.bind();
    skyVbo.allocate(skyboxVertices, sizeof(skyboxVertices));
    skyProgram.enableAttributeArray(0);
    skyProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);
    skyVao.release();

    // Texture loading removed — no longer used. skyTexture stays nullptr,
    // which is safe: nothing in render() requires it to be valid anymore.
}

void EOSky::render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix)
{
    // Change depth function so the sky (Z=1.0) renders behind everything else
    glDepthFunc(GL_LEQUAL);

    skyProgram.bind();

    // CRITICAL: Strip the translation (altitude/X/Z movement) from the camera matrix!
    // We only want the rotation (Pitch/Yaw/Roll).
    QMatrix4x4 skyView = cameraMatrix;
    skyView.setColumn(3, QVector4D(0.0f, 0.0f, 0.0f, 1.0f));

    skyProgram.setUniformValue("projection", projection);
    skyProgram.setUniformValue("view", skyView);

    skyVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    skyVao.release();

    skyProgram.release();

    // Return depth function back to normal for the rest of the scene
    glDepthFunc(GL_LESS);
}
