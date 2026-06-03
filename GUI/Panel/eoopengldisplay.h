
#ifndef EOOPENGLDISPLAY_H
#define EOOPENGLDISPLAY_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QTimer>
#include <QDebug>
// --- Library Includes ---
#include "EODisplayLib/eosky.h"
#include "EODisplayLib/eoland.h"
#include "EODisplayLib/eotargets.h"
#include "EODisplayLib/eovisionangle.h"
#include "../core/Hierarchy/EntityProfiles/sensor.h"

class EOOpenGLDisplay : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    EOOpenGLDisplay(QWidget *parent = nullptr);
    ~EOOpenGLDisplay();

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
    EOVisionAngle camera; // <-- NEW
    EOSky sky;
    EOLand land;
    EOTargets targets;

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

#endif // EOOPENGLDISPLAY_H
