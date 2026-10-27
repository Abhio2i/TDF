#ifndef IRSKY_H
#define IRSKY_H

#include <QOpenGLFunctions>

class IRSky : protected QOpenGLFunctions
{
public:
    IRSky();
    ~IRSky() = default;

    // Call this inside initializeGL()
    void initialize();

    // Call this inside paintGL() if you ever add a 3D Skybox
    void render();
};

#endif // IRSKY_H
