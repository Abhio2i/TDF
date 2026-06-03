#ifndef EOSKY_H
#define EOSKY_H

#include <QOpenGLFunctions>

class EOSky : protected QOpenGLFunctions
{
public:
    EOSky();
    ~EOSky() = default;

    // Call this inside initializeGL()
    void initialize();

    // Call this inside paintGL() if you ever add a 3D Skybox
    void render();
};

#endif // EOSKY_H
