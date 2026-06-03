#include "irsky.h"

IRSky::IRSky()
{
}

void IRSky::initialize()
{
    // Initialize OpenGL function pointers for this class
    initializeOpenGLFunctions();

    // 1. SKY: Set the clear color to Sky Blue
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
}

void IRSky::render()
{
    // Right now, the sky is handled by glClear() using the color set above.
    // If you add a 3D skybox texture later, you will bind and draw it here.
}
