#include "irland.h"

IRLand::IRLand()
    : groundVbo(QOpenGLBuffer::VertexBuffer), vertexCount(0)
{
}

IRLand::~IRLand()
{
    groundVao.destroy();
    groundVbo.destroy();
}

void IRLand::initialize()
{
    initializeOpenGLFunctions();

    // 1. VERTEX SHADER (Procedural 3D Terrain)
    // We use a mathematical noise algorithm to displace the Y (height) of the flat grid.
    const char *gVsrc =
        "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "out vec3 vWorldPos;\n"
        "uniform mat4 mvpMatrix;\n"

        "// Pseudo-random hash function\n"
        "float hash(vec2 p) {\n"
        "   return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);\n"
        "}\n"

        "// Bilinear noise function\n"
        "float noise(vec2 p) {\n"
        "   vec2 i = floor(p);\n"
        "   vec2 f = fract(p);\n"
        "   vec2 u = f * f * (3.0 - 2.0 * f);\n"
        "   return mix(mix(hash(i + vec2(0.0,0.0)), hash(i + vec2(1.0,0.0)), u.x),\n"
        "              mix(hash(i + vec2(0.0,1.0)), hash(i + vec2(1.0,1.0)), u.x), u.y);\n"
        "}\n"

        "// Fractal Brownian Motion (Generates mountainous shapes)\n"
        "float fbm(vec2 p) {\n"
        "   float v = 0.0;\n"
        "   float a = 0.5;\n"
        "   mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));\n"
        "   for (int i = 0; i < 5; ++i) {\n"
        "       v += a * noise(p);\n"
        "       p = rot * p * 2.0 + vec2(100.0);\n"
        "       a *= 0.5;\n"
        "   }\n"
        "   return v;\n"
        "}\n"

        "void main() {\n"
        "   vec3 pos = position;\n"
        "   // Sample the noise based on X and Z world coordinates.\n"
        "   // Multiply by 0.0002 to stretch the mountains out wide.\n"
        "   // Multiply the result by 800.0 to make the mountains 800 meters tall.\n"
        "   float elevation = fbm(pos.xz * 0.0002) * 800.0;\n"
        "   pos.y += elevation;\n"

        "   vWorldPos = pos;\n"
        "   gl_Position = mvpMatrix * vec4(pos, 1.0);\n"
        "}\n";

    // 2. FRAGMENT SHADER (Thermal Elevation Mapping + Distance Fog)
    // We color the mountains based on their height. Valleys are warmer
    // (lighter gray), peaks are colder (darker gray). Distant terrain fades
    // into the sky's cold color instead of hard-clipping at the far plane.
    const char *gFsrc =
        "#version 330 core\n"
        "in vec3 vWorldPos;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "   // Map the 0-800 meter elevation to a 0.0-1.0 gradient\n"
        "   float heightPercent = clamp(vWorldPos.y / 800.0, 0.0, 1.0);\n"

        "   // Add the topological grid back in so you can gauge distance/speed\n"
        "   float gridSize = 100.0;\n"
        "   float gridX = mod(floor(vWorldPos.x / gridSize), 2.0);\n"
        "   float gridZ = mod(floor(vWorldPos.z / gridSize), 2.0);\n"
        "   bool isGridLine = (gridX == gridZ);\n"

        "   // Calculate Thermal Color (Invert height so low=hot/bright, high=cold/dark)\n"
        "   float baseThermal = mix(0.4, 0.1, heightPercent);\n"
        "   if (isGridLine) baseThermal -= 0.05; // Slightly darken grid lines\n"

        "   vec3 terrainColor = vec3(baseThermal);\n"

        "   // Distance fog: fade distant ground into the sky's cold thermal\n"
        "   // color (IRSky clears to 0.05,0.05,0.05) instead of hard-clipping.\n"
        "   // Camera is always at local (X=0, Z=0), so horizontal distance from\n"
        "   // it is just the length of the world XZ position.\n"
        "   float camDist = length(vWorldPos.xz);\n"
        "   float fogNear = 20000.0;\n"
        "   float fogFar  = 480000.0;\n"
        "   float fog = clamp((camDist - fogNear) / (fogFar - fogNear), 0.0, 1.0);\n"
        "   vec3 skyColor = vec3(0.05, 0.05, 0.05);\n" // must match IRSky::initialize() clear color
        "   terrainColor = mix(terrainColor, skyColor, fog);\n"

        "   fragColor = vec4(terrainColor, 1.0);\n"
        "}\n";

    groundProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, gVsrc);
    groundProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, gFsrc);
    groundProgram.link();

    // 3. GENERATE A DENSE 3D MESH (CPU Side)
    // 500km grid divided into 200x200 segments — enlarged from 50km so the
    // ground still extends to the horizon at high altitude instead of running
    // out before the (also enlarged) far clipping plane.
    QVector<QVector3D> vertices;
    int segments = 200;
    float size = 500000.0f;
    float halfSize = size / 2.0f;
    float step = size / segments;

    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            float x1 = -halfSize + j * step;
            float z1 = -halfSize + i * step;
            float x2 = x1 + step;
            float z2 = z1 + step;

            // Triangle 1
            vertices.append(QVector3D(x1, 0.0f, z1));
            vertices.append(QVector3D(x2, 0.0f, z1));
            vertices.append(QVector3D(x1, 0.0f, z2));

            // Triangle 2
            vertices.append(QVector3D(x2, 0.0f, z1));
            vertices.append(QVector3D(x2, 0.0f, z2));
            vertices.append(QVector3D(x1, 0.0f, z2));
        }
    }

    vertexCount = vertices.size();

    // 4. PUSH TO GPU
    groundVao.create();
    groundVao.bind();

    groundVbo.create();
    groundVbo.bind();
    groundVbo.allocate(vertices.constData(), vertexCount * sizeof(QVector3D));

    int posLoc = groundProgram.attributeLocation("position");
    groundProgram.enableAttributeArray(posLoc);
    groundProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 0);

    groundVao.release();
    groundVbo.release();
}

void IRLand::render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix)
{
    // 1. Create a local copy of the camera matrix just for the ground
    QMatrix4x4 groundMatrix = cameraMatrix;

    // 2. Push the ground 150 meters DOWN on the Y-axis.
    groundMatrix.translate(0.0f, -151.0f, 0.0f);

    groundProgram.bind();

    // 3. Pass our new, offset groundMatrix to the shader
    groundProgram.setUniformValue("mvpMatrix", projection * groundMatrix);

    groundVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    groundVao.release();

    groundProgram.release();
}
