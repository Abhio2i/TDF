#include "eoland.h"

EOLand::EOLand()
    : groundVbo(QOpenGLBuffer::VertexBuffer), vertexCount(0)
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

    // 1. VERTEX SHADER (Procedural 3D Terrain)
    // Identical to IR: Uses noise to push the flat grid up into mountains.
    const char *gVsrc =
        "#version 330 core\n"
        "layout(location = 0) in vec3 position;\n"
        "out vec3 vWorldPos;\n"
        "uniform mat4 mvpMatrix;\n"

        "float hash(vec2 p) {\n"
        "   return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);\n"
        "}\n"

        "float noise(vec2 p) {\n"
        "   vec2 i = floor(p);\n"
        "   vec2 f = fract(p);\n"
        "   vec2 u = f * f * (3.0 - 2.0 * f);\n"
        "   return mix(mix(hash(i + vec2(0.0,0.0)), hash(i + vec2(1.0,0.0)), u.x),\n"
        "              mix(hash(i + vec2(0.0,1.0)), hash(i + vec2(1.0,1.0)), u.x), u.y);\n"
        "}\n"

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
        "   float elevation = fbm(pos.xz * 0.0002) * 800.0;\n"
        "   pos.y += elevation;\n"

        "   vWorldPos = pos;\n"
        "   gl_Position = mvpMatrix * vec4(pos, 1.0);\n"
        "}\n";

    // 2. FRAGMENT SHADER (Optical Biome Mapping + Distance Fog)
    // We color the terrain based on its height to look natural, then fade
    // distant terrain into the horizon color so the ground blends into the
    // sky instead of hard-clipping at the far plane.
    const char *gFsrc =
        "#version 330 core\n"
        "in vec3 vWorldPos;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "   // Map 0-800m elevation to a 0.0-1.0 percentage\n"
        "   float heightPercent = clamp(vWorldPos.y / 800.0, 0.0, 1.0);\n"

        "   // Retain the topological grid for scale/speed reference\n"
        "   float gridSize = 100.0;\n"
        "   float gridX = mod(floor(vWorldPos.x / gridSize), 2.0);\n"
        "   float gridZ = mod(floor(vWorldPos.z / gridSize), 2.0);\n"
        "   bool isGridLine = (gridX == gridZ);\n"

        "   // Define Biome Colors\n"
        "   vec3 valleyColor = vec3(0.15, 0.45, 0.15); // Deep green grass\n"
        "   vec3 hillColor   = vec3(0.45, 0.35, 0.20); // Brown dirt/rock\n"
        "   vec3 peakColor   = vec3(0.85, 0.90, 0.95); // White snow\n"

        "   vec3 terrainColor;\n"
        "   // Blend colors based on altitude\n"
        "   if (heightPercent < 0.4) {\n"
        "       // 0% to 40% height: Blend Grass to Rock\n"
        "       terrainColor = mix(valleyColor, hillColor, heightPercent / 0.4);\n"
        "   } else {\n"
        "       // 40% to 100% height: Blend Rock to Snow\n"
        "       terrainColor = mix(hillColor, peakColor, (heightPercent - 0.4) / 0.6);\n"
        "   }\n"

        "   // Darken the procedural grid lines slightly for contrast\n"
        "   if (isGridLine) terrainColor *= 0.85;\n"

        "   // Distance fog: fade the ground into the horizon color instead of\n"
        "   // hard-clipping at the far plane. The camera is always at local\n"
        "   // (X=0, Z=0) in this coordinate scheme, so horizontal distance from\n"
        "   // the camera is just the length of the world XZ position — no extra\n"
        "   // camera-position uniform needed. This matters a lot at high\n"
        "   // altitude, where the visible horizon is tens of kilometres away.\n"
        "   float camDist = length(vWorldPos.xz);\n"
        "   float fogNear = 20000.0;\n"
        "   float fogFar  = 480000.0;\n"
        "   float fog = clamp((camDist - fogNear) / (fogFar - fogNear), 0.0, 1.0);\n"
        "   vec3 horizonColor = vec3(0.65, 0.78, 0.92);\n" // must match sky's horizon color
        "   terrainColor = mix(terrainColor, horizonColor, fog);\n"

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

void EOLand::render(const QMatrix4x4 &projection, const QMatrix4x4 &cameraMatrix)
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
