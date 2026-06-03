
#ifndef IROPENGLDISPLAY_H
#define IROPENGLDISPLAY_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QTimer>
#include <QDebug>
// --- Library Includes ---
#include "IRDisplayLib/irsky.h"
#include "IRDisplayLib/irland.h"
#include "IRDisplayLib/irtargets.h"
#include "IRDisplayLib/irvisionangle.h"
#include "../core/Hierarchy/EntityProfiles/sensor.h"

class IROpenGLDisplay : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    IROpenGLDisplay(QWidget *parent = nullptr);
    ~IROpenGLDisplay();

    void setSensorParameter(
        const PoseGeo &_sensorPoseGeo,
        const std::unordered_map<std::string,std::pair<bool,PoseOpenGL>> &_EODetectionCood);
    void setSensorParameter(
        const PoseGeo &_sensorPoseGeo,
        const std::unordered_map<std::string,std::pair<bool,CartesianCoord>> &_EODetectionCoord);
private:
    PoseGeo    sensorPoseGeo;
    std::unordered_map<std::string,std::pair<bool,PoseOpenGL>> EODetectionCood;
    std::unordered_map<std::string,std::pair<bool,CartesianCoord>> EODetectionCoord;
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    QMatrix4x4 projection;

    // --- Scene Objects ---
    IRVisionAngle camera; // <-- NEW
    IRSky sky;
    IRLand land;
    IRTargets targets;

private:
    struct EntityDimension {
        float length = 1.00;
        float width  = 1.00;
        float height = 1.00;
        EntityDimension(
            float _length ,
            float _width  ,
            float _height ):
            length(_length ),
            width (_width  ),
            height(_height )
        {};
    };
    EntityDimension Boeing_777_200ER{63.73, 60.93, 18.5};
};

#endif // IROPENGLDISPLAY_H
