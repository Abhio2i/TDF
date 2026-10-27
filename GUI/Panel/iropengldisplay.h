#ifndef IROPENGLDISPLAY_H
#define IROPENGLDISPLAY_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <cmath>

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

    // maxRangeMeters: sensor's own configured detection range (IRSensor::range * 1000,
    // range is stored in km). Sizes the far-clipping plane so long-range targets
    // aren't invisibly clipped — mirrors EOOpenGLDisplay's fix.
    void setSensorParameter(
        const PoseGeo& _sensorPoseGeo,
        float _horizontalCameraAngle,
        const std::unordered_map<std::string,
                                 std::pair<bool, CartesianCoord>>& _IRDetectionCoord,
        float _maxRangeMeters = 50000.0f);

    /* ===================  GIMBAL / SLEW / ZOOM  =================== */
    void setCommandedLOS(float azimuthDeg, float elevationDeg);
    void setZoom(float zoom);
    void setSlewRate(float degPerSec);

    float getCurrentAzimuth()   const { return currentAzimuth; }
    float getCurrentElevation() const { return currentElevation; }

    float getZoom()             const { return zoomLevel; }

private:
    PoseGeo sensorPoseGeo;
    float horizontalCameraAngle = 0;

    float maxRangeMeters = 50000.0f;

    std::unordered_map<std::string,
                       std::pair<bool, CartesianCoord>> IRDetectionCoord;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QMatrix4x4 projection;
    QMatrix4x4 m_lastViewMatrix;

    // --- Scene Objects ---
    IRVisionAngle camera;
    IRSky sky;
    IRLand land;
    IRTargets targets;

    /* ===================  GIMBAL / SLEW / ZOOM  =================== */
private:
    float currentAzimuth     = 0.0f;
    float currentElevation   = 0.0f;
    float commandedAzimuth   = 0.0f;
    float commandedElevation = 0.0f;

    float slewRateDegPerSec  = 60.0f;
    float zoomLevel          = 1.0f;

    QElapsedTimer frameClock;
    bool frameClockStarted = false;

    QTimer *slewTimer = nullptr;
    static constexpr int SLEW_TIMER_INTERVAL_MS = 33;
    static constexpr float SLEW_CONVERGE_EPS_DEG = 0.05f;

    // Belly-mounted turret mechanical limits — same as EO.
    static constexpr float ELEVATION_MIN_DEG = -90.0f;
    static constexpr float ELEVATION_MAX_DEG =  15.0f;

    static float clampElevation(float deg);
    static float normalizeAngle180(float deg);
    bool stepSlew(float dt);
    void onSlewTick();

    /* =========== OPERATOR INPUT state =========== */
private:
    bool  m_dragging           = false;
    QPoint m_dragStartPos;
    float m_dragStartAzimuth   = 0.0f;
    float m_dragStartElevation = 0.0f;

    float m_lastHFovDeg = 45.0f;

    static constexpr float DRAG_DEG_PER_PIXEL_MIN = 0.01f;
    static constexpr float DRAG_DEG_PER_PIXEL_MAX = 1.0f;

    static constexpr float WHEEL_ZOOM_FACTOR_PER_NOTCH = 1.15f;
    static constexpr float KEY_NUDGE_DEG_PER_PRESS = 2.0f;

    static constexpr int CLICK_MAX_MOVE_PX = 6;
    QPoint m_mousePressScreenPos;

    bool designateTargetAtScreenPos(const QPoint &screenPos);

    /* ===================  TRACK MODE  =================== */
private:
    bool m_trackModeActive = false;
    std::string m_lockedTargetId;

    static constexpr float TRACK_ASSUMED_TARGET_WIDTH_M = 15.0f;
    static constexpr float TRACK_FRAMING_MULTIPLIER = 4.0f;
    static constexpr float TRACK_MIN_RANGE_M = 10.0f;

    float m_lastTrackRangeM = 0.0f;

    void updateTrackedTarget(float currentAltitude, float baseFovDeg);

    CartesianCoord m_lastLockedCoord{};
    bool m_hasLastLockedCoord = false;

    /* ===================  HUD OVERLAY  =================== */
private:
    QLabel *m_hudMode      = nullptr;
    QLabel *m_hudCrosshair = nullptr;
    QLabel *m_hudAltRange  = nullptr;
    QLabel *m_hudTargetAz  = nullptr;
    QLabel *m_hudFov       = nullptr;
    QLabel *m_hudZoom      = nullptr;
    QLabel *m_hudTrackBox  = nullptr;

    QLabel *m_hudLosAzEl   = nullptr;  //  top AZ / EL
    QLabel *m_hudCompass   = nullptr;  //  bottom compass tape

    QVector<QLabel*> m_hudCandidateBoxes;
    void ensureCandidateBoxCount(int n);

    QPushButton *m_hudUntrackButton = nullptr;

    void setupHud();
    void layoutHud();
    void updateHudText(float hFovDeg);

    bool projectToScreen(const QVector3D &worldPos, QPoint &screenPosOut) const;

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
