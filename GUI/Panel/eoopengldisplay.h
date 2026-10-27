#ifndef EOOPENGLDISPLAY_H
#define EOOPENGLDISPLAY_H

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
#include "EODisplayLib/eosky.h"
#include "EODisplayLib/eoland.h"
#include "EODisplayLib/eotargets.h"
#include "EODisplayLib/eovisionangle.h"
#include "../core/Hierarchy/EntityProfiles/sensor.h"

// Forward declaration only — full definition is included in the .cpp.
// A pointer is all this header needs; keeps the dependency light and
// avoids any risk of a circular include with eosensor.h.
class EOSensor;

// Struct to hold our calculated OpenGL Cartesian coordinates

class EOOpenGLDisplay : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    EOOpenGLDisplay(QWidget *parent = nullptr);
    ~EOOpenGLDisplay();

    // Changed to take PoseGeo for the targets as well, so we can calculate
    // the relative distances natively inside this display class.
    // maxRangeMeters: the sensor's own configured detection range (EOSensor
    // ::range * 1000, since range is stored in km — confirmed via
    // eosensor.cpp's "range*1000" conversions and the toJson() "km" label).
    // Used to size the far-clipping plane so targets the sensor legitimately
    // detects at long range aren't invisibly clipped by a fixed render
    // distance — see the zFar calculation in paintGL().
    void setSensorParameter(
        PoseGeo _sensorPoseGeo,
        float _horizontalCameraAngle,
        std::unordered_map<std::string,
                           std::pair<bool, CartesianCoord>> _EODetectionCoord,
        float _maxRangeMeters = 50000.0f);

    /* ===================  PHASE 1: GIMBAL / SLEW / ZOOM  =================== *
     * Public control surface for future Manual/Cue/Track modes. Nothing else
     * in the codebase calls these yet — safe no-op additions.             */

    // Command the gimbal to point at a new LOS (degrees). The camera will
    // slew towards this smoothly at slewRateDegPerSec instead of snapping.
    void setCommandedLOS(float azimuthDeg, float elevationDeg);

    // Optical zoom factor. 1.0 = base FOV (whatever the sensor reports).
    // Internally clamped to >= 1.0 so FOV can never invert or divide by zero.
    void setZoom(float zoom);

    // Gimbal slew speed in degrees/second. Clamped to a sane positive range.
    void setSlewRate(float degPerSec);

    // Current (already-slewed, not commanded-target) LOS, in degrees.
    // Read by EODisplay so it can push the live gimbal state into the
    // EOSensor for gimbal-aware detection — see eosensor.cpp's scan().
    float getCurrentAzimuth()   const { return currentAzimuth; }
    float getCurrentElevation() const { return currentElevation; }
    float getZoom()             const { return zoomLevel; }

    // Direct link to the EOSensor currently paired with this display.
    // Set once by EODisplay when an entity/sensor is selected (and cleared
    // to nullptr when deselected/removed, so this never dangles). Every
    // paintGL() frame, this display pushes its live currentAzimuth/
    // currentElevation straight into the sensor's gimbalAzimuthOffset/
    // gimbalElevationOffset — see paintGL(). This replaces relying on
    // EODisplay::updateRadar()'s own (slower, decoupled) cadence, which
    // was letting the sensor's detection-gate see a STALE gimbal position
    // while a fast-moving TRACK-mode target's true bearing kept changing,
    // causing the lock to silently drift out of the FOV cone over time.
    void setPairedSensor(EOSensor *sensor) { m_pairedSensor = sensor; }

private:
    PoseGeo sensorPoseGeo;
    // Map now stores the target's absolute geographic location (PoseGeo)
    // std::unordered_map<std::string, std::pair<bool, PoseGeo>> EODetectionCoord;
    float horizontalCameraAngle = 0;

    // Sensor's own configured detection range, in meters. Drives the
    // far-clipping plane in paintGL() so long-range detections are actually
    // rendered instead of being clipped by a fixed distance. Defaults to
    // 50km (the old hardcoded value) until the first real setSensorParameter().
    float maxRangeMeters = 50000.0f;

    // See setPairedSensor() above. nullptr until EODisplay pairs an entity.
    EOSensor *m_pairedSensor = nullptr;

    // Relative Coordinate Calculator
    //  CartesianCoord getRelativeCoordinatesGL(const PoseGeo& sensor, const PoseGeo& target);

    std::unordered_map<std::string,
                       std::pair<bool, CartesianCoord>> EODetectionCoord;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override; // also repositions the HUD overlay (see layoutHud())
    void paintGL() override;

    /* =========== PHASE 1.5: OPERATOR INPUT (click-drag slew + wheel zoom) === *
     * Mirrors a real EO/IR hand-controller: drag the sensor view to slew the
     * LOS, scroll to zoom. Sensitivity scales with current FOV so slewing
     * feels the same "amount of screen" at any zoom level, exactly like a
     * real turret's joystick gain is tied to optical zoom.                  */
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    // Keyboard: arrow keys for manual az/el nudge (number keys and +/- are
    // reserved elsewhere in the app, so this scheme avoids them entirely),
    // and 'T' to toggle TRACK mode (real hand-controller convention: lock
    // onto the nearest detected target, auto-slew + auto-zoom to follow it,
    // and hand gimbal control over from the operator until untracked).
    void keyPressEvent(QKeyEvent *event) override;

private:
    QMatrix4x4 projection;
    QMatrix4x4 m_lastViewMatrix; // cached each paintGL() frame for projectToScreen()

    // --- Scene Objects ---
    EOVisionAngle camera;
    EOSky sky;
    EOLand land;
    EOTargets targets;

    /* ===================  PHASE 1: GIMBAL / SLEW / ZOOM  =================== */
private:
    // LOS state (degrees). Default 0,0 means "no gimbal command yet" and
    // reproduces the exact identity-rotation behaviour the display already
    // had before this change.
    float currentAzimuth     = 0.0f;
    float currentElevation   = 0.0f;
    float commandedAzimuth   = 0.0f;
    float commandedElevation = 0.0f;

    float slewRateDegPerSec  = 60.0f; // realistic mid-range gimbal speed
    float zoomLevel          = 1.0f;  // 1x = base FOV, never <= 0

    // Delta-time source for the slew integration inside paintGL().
    QElapsedTimer frameClock;
    bool frameClockStarted = false;

    // Repaint pump: only alive while a slew is actually in progress, so at
    // rest (the common case) this adds zero extra CPU/GPU/RAM load beyond
    // whatever already drove repaints before this change.
    QTimer *slewTimer = nullptr;
    static constexpr int SLEW_TIMER_INTERVAL_MS = 33; // ~30 Hz while slewing
    static constexpr float SLEW_CONVERGE_EPS_DEG = 0.05f; // stop-jitter guard

    // Realistic mechanical elevation limits for a belly/chin-mounted EO/IR
    // turret ball: it hangs underneath the aircraft, so it can look almost
    // straight down (nadir) but the fuselage physically blocks anything
    // near straight up. Azimuth is intentionally left unlimited (0-360,
    // continuous) since the gimbal ring rotates freely underneath — this
    // matches real turrets like MTS-B/Wescam MX-series.
    static constexpr float ELEVATION_MIN_DEG = -90.0f; // straight down
    static constexpr float ELEVATION_MAX_DEG =  15.0f; // slightly above horizon

    // Clamps an elevation command/value to the mechanical limits above.
    static float clampElevation(float deg);

    // Wraps an angle into (-180, 180].
    static float normalizeAngle180(float deg);

    // Advances currentAzimuth/currentElevation towards the commanded values
    // by at most slewRateDegPerSec * dt, taking the shortest angular path
    // for azimuth (handles the 350deg -> 10deg wraparound case).
    // Returns true if the LOS is still converging (i.e. timer should keep running).
    bool stepSlew(float dt);

    void onSlewTick();

    /* =========== PHASE 1.5: OPERATOR INPUT state =========== */
private:
    bool  m_dragging           = false;
    QPoint m_dragStartPos;
    float m_dragStartAzimuth   = 0.0f;
    float m_dragStartElevation = 0.0f;

    // Cached each paintGL() frame so the input handlers can convert screen
    // pixels to degrees using the ACTUAL current horizontal FOV (which
    // depends on zoom) without recomputing the whole projection here.
    float m_lastHFovDeg = 45.0f;

    // Degrees-per-pixel drag sensitivity, min/max clamps so it never goes
    // to zero (frozen control) or absurdly high (twitchy) even at extreme
    // zoom or an unusually small/large widget.
    static constexpr float DRAG_DEG_PER_PIXEL_MIN = 0.01f;
    static constexpr float DRAG_DEG_PER_PIXEL_MAX = 1.0f;

    // Multiplicative zoom step per wheel "notch" (120 units of angleDelta).
    static constexpr float WHEEL_ZOOM_FACTOR_PER_NOTCH = 1.15f;

    // Degrees nudged per arrow-key press (keyboard alternative to drag).
    static constexpr float KEY_NUDGE_DEG_PER_PRESS = 2.0f;

    // If the mouse moves less than this many pixels between press and
    // release, it's treated as a CLICK (target designation) rather than a
    // drag (manual slew) — see mouseReleaseEvent()/designateTargetAtScreenPos().
    static constexpr int CLICK_MAX_MOVE_PX = 6;
    QPoint m_mousePressScreenPos;

    // Finds the detected target whose projected screen position is nearest
    // the given point (within a reasonable pixel radius) and, if found,
    // locks TRACK mode onto it directly — "click on the aircraft you want"
    // target designation. Returns false if nothing detected is close enough.
    bool designateTargetAtScreenPos(const QPoint &screenPos);

    /* ===================  TRACK MODE (auto-lock / auto-zoom / follow) === *
     * Real EO/IR hand-controllers support an operator-triggered TRACK mode:
     * press a button, the gimbal locks onto the nearest currently-detected
     * target, auto-slews + auto-zooms to keep it framed, and keeps following
     * it as it moves — manual joystick input is suspended while locked
     * ("halt platform movement upon detection", per real gimbal specs).
     * Losing the target re-searches for the nearest one automatically
     * (matches "memory tracking with rapid target reacquisition").         */
private:
    bool m_trackModeActive = false;
    std::string m_lockedTargetId; // empty = no target currently locked

    // Last known Cartesian position of the locked target, cached every
    // frame it's actually visible — used to "coast" (keep aiming at the
    // last-seen position) for a moment if the target briefly drops out of
    // detection, rather than snapping/freezing instantly.
    CartesianCoord m_lastLockedCoord{};
    bool m_hasLastLockedCoord = false;

    // Assumed physical width (m) used to size the auto-zoom framing — 15m
    // matches the "baseSize" constant already used elsewhere in the sensor
    // code for a generic target. Not target-specific (we don't have real
    // target dimensions here), but gives a believable "keep it framed"
    // auto-zoom law: FOV narrows as range increases, widens as it closes.
    static constexpr float TRACK_ASSUMED_TARGET_WIDTH_M = 15.0f;
    // How many target-widths should span the frame when framed — smaller
    // = tighter/more zoomed-in framing.
    static constexpr float TRACK_FRAMING_MULTIPLIER = 6.0f;
    // Floor on range used in the zoom calc, so a target at point-blank
    // range can't demand a divide-by-zero / absurdly extreme zoom.
    static constexpr float TRACK_MIN_RANGE_M = 10.0f;

    // Range (m) to the currently locked target, for the HUD. 0 if untracked.
    float m_lastTrackRangeM = 0.0f;

    // Called once per paintGL() frame, BEFORE stepSlew(), so any target
    // lock this frame feeds straight into the normal smooth-slew machinery
    // (setCommandedLOS()) rather than snapping the camera.
    void updateTrackedTarget(float currentAltitude, float baseFovDeg);

    /* ===================  HUD OVERLAY  =================== *
     * Plain child QLabels drawn on top of the GL surface — the standard,
     * low-risk way to mix 2D text with a QOpenGLWidget (no custom
     * QPainter/paintGL mixing, which is easy to get subtly wrong). They
     * are mouse-transparent so drag/click-to-slew still works underneath.
     * Layout matches a real EO/IR targeting-pod display: green monochrome
     * text, mode label top-center, crosshair center, altitude+range left,
     * target aspect right, FOV/zoom bottom corners, tracking bracket box
     * drawn around the locked target's live screen position.             */
private:
    QLabel *m_hudMode      = nullptr; // top-center: "POINT TRACK" / "MANUAL"
    QLabel *m_hudCrosshair = nullptr; // dead-center: "+" boresight reticle
    QLabel *m_hudAltRange  = nullptr; // left-mid: "ALTITUDE ... / SLANT RANGE ..."
    QLabel *m_hudTargetAz  = nullptr; // right-mid: "TARGET ASPECT nnn°"
    QLabel *m_hudFov       = nullptr; // bottom-left: "FOV: n.n°"
    QLabel *m_hudZoom      = nullptr; // bottom-right: "ZOOM: n.nx"
    QLabel *m_hudTrackBox  = nullptr; // bracket box, repositioned onto the

    QLabel *m_hudLosAzEl   = nullptr;  //  top AZ / EL
    QLabel *m_hudCompass   = nullptr;  //  bottom compass tape

    // locked target's projected screen
    // position each frame; hidden otherwise

    // One box per currently-detected, NOT-locked target — drawn wherever
    // that target currently projects on screen, every frame. Pooled and
    // reused (grown/shrunk to match the live detected-target count) rather
    // than created/destroyed every frame, to avoid widget-churn overhead.
    // Mouse-transparent, same as the other HUD elements: clicking one is
    // handled computationally in mouseReleaseEvent()/designateTargetAtScreenPos()
    // by finding the nearest detected target to the click point — the box
    // is a visual indicator only, not a separate clickable widget.
    QVector<QLabel*> m_hudCandidateBoxes;
    void ensureCandidateBoxCount(int n); // grows the pool if needed

    // Real, clickable UI button (not just the 'T' key) to drop TRACK mode —
    // shown only while a lock is active.
    QPushButton *m_hudUntrackButton = nullptr;

    void setupHud();                   // creates + styles the labels (constructor)
    void layoutHud();                  // positions the fixed labels (resizeGL)
    void updateHudText(float hFovDeg); // refreshes text + boxes (paintGL, every frame)

    // Projects a world-space point through the current projection*view
    // matrices to a widget-space (pixel) position, for the tracking box.
    // Returns false (and leaves screenPos untouched) if the point is
    // behind the camera / not projectable this frame.
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

#endif // EOOPENGLDISPLAY_H
