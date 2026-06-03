#include "eoopengldisplay.h"


EOOpenGLDisplay::EOOpenGLDisplay(QWidget *parent)
    : QOpenGLWidget(parent)
{
    // Timer removed since we aren't animating the orbit anymore,
    // but you can add it back here if you want to animate camera movement later.
}


EOOpenGLDisplay::~EOOpenGLDisplay()
{
    // Context teardown handled by child objects (EOLand, EOTargets)
}

void EOOpenGLDisplay::setSensorParameter(
    const PoseGeo &_sensorPoseGeo,
    const std::unordered_map<std::string, std::pair<bool, PoseOpenGL> > &_EODetectionCood)
{
    sensorPoseGeo = _sensorPoseGeo;
    EODetectionCood = _EODetectionCood;
}

void EOOpenGLDisplay::setSensorParameter(const PoseGeo &_sensorPoseGeo, const std::unordered_map<std::string, std::pair<bool, CartesianCoord> > &_EODetectionCoord)
{
    sensorPoseGeo = _sensorPoseGeo;
    EODetectionCoord = _EODetectionCoord;
}



void EOOpenGLDisplay::initializeGL()
{
    initializeOpenGLFunctions();

    // Initialize our external library components
    sky.initialize();
    land.initialize();
    targets.initialize();

    // Global rendering flags
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);

}

void EOOpenGLDisplay::resizeGL(int w, int h)
{
    float aspect = qreal(w) / qreal(h ? h : 1);
    const float zNear = 0.1f, zFar = 1000.0f, fov = 45.0f;

    projection.setToIdentity();
    projection.perspective(fov, aspect, zNear, zFar);
}

// void EOOpenGLDisplay::paintGL()
// {
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//     // --- 1. GET CAMERA MATRIX ---
//     // The camera remains completely stationary.
//     QMatrix4x4 cameraMatrix = camera.getViewMatrix();

//     // --- 2. EXTRACT SENSOR DATA SAFELY ---
//     // Always check if the pointer is null before trying to read it!
//     // If it is null (e.g., right when the app starts), default the ground to 10 meters below.
//     float currentAltitude = 10.0f;


//     if(sensorPoseGeo.altitude){
//         qDebug()<<"Alt: "<<QString::number(sensorPoseGeo.altitude);
//     }else{
//         qDebug()<<"Alt: NA";
//     }

//     currentAltitude = sensorPoseGeo.altitude;

//     // --- 3. RENDER SCENE ---
//     sky.render();

//     // Pass the extracted altitude down into the land renderer
//     land.render(projection, cameraMatrix, currentAltitude);

//     // Targets will eventually use the EODetectionCood map the exact same way
//     targets.render(projection, cameraMatrix);
// }
// void EOOpenGLDisplay::paintGL()
// {
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//     // --- 1. GET CAMERA MATRIX ---
//     QMatrix4x4 cameraMatrix = camera.getViewMatrix();

//     // --- 2. EXTRACT SENSOR DATA ---
//     float currentAltitude = sensorPoseGeo.altitude;
//     if (currentAltitude) {
//         // Optional: Keep debug if you want, but printing every frame (60FPS)
//         // will flood your console. Consider removing or throttling this later.
//         // qDebug() << "Alt: " << QString::number(currentAltitude);
//     }

//     // --- 3. RENDER ENVIRONMENT ---
//     sky.render();
//     land.render(projection, cameraMatrix, currentAltitude);

//     // --- 4. RENDER ALL DETECTED TARGETS ---
//     for (const auto& pair : EODetectionCood) {

//         bool isDetected = pair.second.first; // The boolean flag

//         // Only draw the airplane if it is actively detected by the sensor
//         if (isDetected) {
//             PoseOpenGL targetPose = pair.second.second; // The pose data

//             targets.render(projection, cameraMatrix,
//                            targetPose.x, targetPose.y, targetPose.z,
//                            targetPose.rx, targetPose.ry, targetPose.rz);
//         }
//     }
// }

void EOOpenGLDisplay::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- 1. GET STATIONARY CAMERA MATRIX ---
    QMatrix4x4 cameraMatrix = camera.getViewMatrix();

    // --- 2. EXTRACT SENSOR DATA SAFELY ---
    float currentAltitude = 10.0f;
    float pitch = 0.0f;
    float yaw   = 0.0f;
    float roll  = 0.0f;

    // Safety check to ensure we only read valid data
    if (sensorPoseGeo.altitude != 0.0f || sensorPoseGeo.rx != 0.0f) {
        currentAltitude = sensorPoseGeo.altitude;
        pitch = sensorPoseGeo.rx;
        yaw   = sensorPoseGeo.ry;
        roll  = sensorPoseGeo.rz;
    }

    // --- 3. RENDER ENVIRONMENT ---
    sky.render();

    // Pass altitude AND angles to the land renderer
    land.render(projection, cameraMatrix, currentAltitude, pitch, yaw, roll);

    // --- 4. RENDER ALL DETECTED TARGETS ---
    for (const auto& pair : EODetectionCood) {
        bool isDetected = pair.second.first;

        if (isDetected) {
            PoseOpenGL targetPose = pair.second.second;

            // Assuming your target coordinates from EODetectionCood are already calculated
            // relative to the sensor's current viewpoint by your simulation backend:
            targets.render(projection, cameraMatrix,
                           targetPose.x, targetPose.y, targetPose.z,
                           targetPose.rx, targetPose.ry, targetPose.rz);
        }
    }
}
