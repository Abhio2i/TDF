#include "irsky.h"

IRSky::IRSky()
{
}

void IRSky::initialize()
{
    initializeOpenGLFunctions();

    // 1. SKY (IR View): The sky is generally very cold in thermal imaging.
    // Set to a very dark gray/almost black.
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
}

void IRSky::render()
{
    // Sky is handled by glClear() using the cold color set above.
}
