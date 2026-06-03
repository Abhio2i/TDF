#include "eoland.h"

EOLand::EOLand()
    : groundVbo(QOpenGLBuffer::VertexBuffer)
{
}

EOLand::~EOLand()
{
    groundVao.destroy();
    groundVbo.destroy();
}

void EOLand::initialize()
{
    initializeOpenGLFunctions();

    // 1. Create Ground Shader
    const char *gVsrc =
        "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "out vec3 vWorldPos;\n"
        "uniform mat4 mvpMatrix;\n"
        "void main() {\n"
        "   vWorldPos = position;\n"
        "   gl_Position = mvpMatrix * vec4(position, 1.0);\n"
        "}\n";

    const char *gFsrc =
        "#version 330 core\n"
        "in vec3 vWorldPos;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "   float gridSize = 20.0;\n"
        "   float gridX = mod(floor(vWorldPos.x / gridSize), 2.0);\n"
        "   float gridZ = mod(floor(vWorldPos.z / gridSize), 2.0);\n"
        "   bool isDark = (gridX == gridZ);\n"
        "   vec3 color = isDark ? vec3(0.15, 0.4, 0.15) : vec3(0.2, 0.5, 0.2);\n"
        "   fragColor = vec4(color, 1.0);\n"
        "}\n";

    groundProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, gVsrc);
    groundProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, gFsrc);
    groundProgram.link();

    // 2. Define a giant flat quad
    float s = 2000.0f;
    float groundVertices[] = {
        // X,    Y,     Z
        -s,    0.0f,  -s,
        s,    0.0f,  -s,
        s,    0.0f,   s,
        s,    0.0f,   s,
        -s,    0.0f,   s,
        -s,    0.0f,  -s
    };

    // 3. Load into VAO/VBO
    groundVao.create();
    groundVao.bind();

    groundVbo.create();
    groundVbo.bind();
    groundVbo.allocate(groundVertices, sizeof(groundVertices));

    int posLoc = groundProgram.attributeLocation("position");
    groundProgram.enableAttributeArray(posLoc);
    groundProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 0);

    groundVao.release();
    groundVbo.release();
}

// ... (Constructor and initialize() stay exactly the same) ...

// --> NEW: Using the altitude variable
// void EOLand::render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix, float altitude)
// {
//     // Apply camera transformations to the ground, and push it down by the altitude.
//     QMatrix4x4 groundMatrix = cameraMatrix;

//     // Move the ground down based on the sensor's altitude.
//     // If altitude is 5000 meters, the ground gets pushed down 5000 units on the Y-axis.
//     groundMatrix.translate(0.0f, -altitude, 0.0f);

//     groundProgram.bind();
//     groundProgram.setUniformValue("mvpMatrix", projection * groundMatrix);

//     groundVao.bind();
//     glDrawArrays(GL_TRIANGLES, 0, 6);
//     groundVao.release();

//     groundProgram.release();
// }

void EOLand::render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix,
                    float altitude, float rx, float ry, float rz)
{
    QMatrix4x4 groundMatrix = cameraMatrix;

    // 1. Apply INVERSE sensor rotations to the world.
    // We use negative angles because if the camera looks up (+rx), the ground must move down (-rx).
    groundMatrix.rotate(-rz, 0.0f, 0.0f, 1.0f); // Roll
    groundMatrix.rotate(-rx, 1.0f, 0.0f, 0.0f); // Pitch
    groundMatrix.rotate(-ry, 0.0f, 1.0f, 0.0f); // Yaw

    // 2. Push the ground down by altitude AFTER applying the rotation.
    // This perfectly anchors the camera at (0,0,0) while the ground drops away.
    groundMatrix.translate(0.0f, -altitude, 0.0f);

    groundProgram.bind();
    groundProgram.setUniformValue("mvpMatrix", projection * groundMatrix);

    groundVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    groundVao.release();

    groundProgram.release();
}
