#include "eoopengldisplay.h"
#include <QtMath> // Required for qDegreesToRadians and qRadiansToDegrees
#include <QFont>
#include <QFontInfo>
#include <algorithm>

// Compile-time constants for the coordinate conversion
constexpr float EARTH_RADIUS_F = 6378137.0f; // WGS84 equatorial radius in meters
constexpr float PI_F           = 3.141592653589793f;
constexpr float DEG_TO_RAD_F   = PI_F / 180.0f;

EOOpenGLDisplay::EOOpenGLDisplay(QWidget *parent)
    : QOpenGLWidget(parent)
{
    // Needed so this widget actually receives key events for the arrow-key
    // control scheme below — without this, keyPressEvent() never fires.
    setFocusPolicy(Qt::StrongFocus);

    // Phase 1: on-demand repaint pump for smooth gimbal slew. Created here
    // but deliberately NOT started — it only runs while a slew is actually
    // converging (see stepSlew()/paintGL()), so at rest this widget costs
    // exactly the same CPU/RAM/GPU as before this feature existed.
    slewTimer = new QTimer(this);
    slewTimer->setSingleShot(false);
    connect(slewTimer, &QTimer::timeout, this, &EOOpenGLDisplay::onSlewTick);

    setupHud();
}

static float wrap360(float deg)
{
    deg = std::fmod(deg, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    return deg;
}

static QString compassTapeString(float uiAzDeg)
{
    const float az = wrap360(uiAzDeg);
    auto slot = [](float d) -> QString {
        d = wrap360(d);
        const int id = qRound(d);
        if (id <= 22 || id >= 338) return QString("N  000");
        if (id >= 23  && id <= 67)  return QString("NE 045");
        if (id >= 68  && id <= 112) return QString("E  090");
        if (id >= 113 && id <= 157) return QString("SE 135");
        if (id >= 158 && id <= 202) return QString("S  180");
        if (id >= 203 && id <= 247) return QString("SW 225");
        if (id >= 248 && id <= 292) return QString("W  270");
        return QString("NW 315");
    };
    return QString("%1    ▲ %2    %3")
        .arg(slot(az - 45.f))
        .arg(slot(az))
        .arg(slot(az + 45.f));
}

static QString lookHint(float uiAz, float el)
{
    const float a = std::fabs(uiAz);
    QString h = (a < 30.f) ? "FWD" :
                    (a < 150.f) ? ((uiAz > 0.f) ? "RIGHT" : "LEFT") : "AFT";
    QString v = (el > 5.f) ? "UP" : (el < -15.f) ? "DOWN" : "LEVEL";
    return h + " · " + v;
}

EOOpenGLDisplay::~EOOpenGLDisplay()
{
    // slewTimer is parented to 'this' so Qt would clean it up anyway, but
    // stop it explicitly first so no queued timeout can fire mid-teardown.
    if (slewTimer) {
        slewTimer->stop();
    }
}

void EOOpenGLDisplay::setSensorParameter(
    PoseGeo _sensorPoseGeo,
    float _horizontalCameraAngle,
    std::unordered_map<std::string,
                       std::pair<bool, CartesianCoord>> _EODetectionCoord,
    float _maxRangeMeters)
{
    sensorPoseGeo = _sensorPoseGeo;
    horizontalCameraAngle = _horizontalCameraAngle;
    EODetectionCoord = _EODetectionCoord;
    // Guard against 0/negative (e.g. sensor not configured yet) so the
    // far-clipping plane never collapses to something degenerate.
    maxRangeMeters = (_maxRangeMeters > 1000.0f) ? _maxRangeMeters : 50000.0f;
}


/* ===================  PHASE 1: GIMBAL / SLEW / ZOOM  =================== */

void EOOpenGLDisplay::setCommandedLOS(float azimuthDeg, float elevationDeg)
{
    commandedAzimuth   = normalizeAngle180(azimuthDeg);
    // Realistic mechanical limit: a belly-mounted turret can't look through
    // the aircraft's own fuselage, so elevation is clamped here — every
    // caller (mouse drag, arrow keys, future Cue/Track modes) goes through
    // this one function, so the limit applies uniformly everywhere.
    commandedElevation = clampElevation(elevationDeg);

    // Wake the repaint pump only if it isn't already running — cheap and
    // idempotent, avoids restarting an already-active timer.
    if (slewTimer && !slewTimer->isActive()) {
        slewTimer->start(SLEW_TIMER_INTERVAL_MS);
    }

    // Kick one immediate frame so a command issued while fully idle (timer
    // stopped) doesn't wait for the next external repaint to start moving.
    update();
}

void EOOpenGLDisplay::setZoom(float zoom)
{
    // Clamp defensively: zoom must never be <= 0 (would divide FOV by zero
    // or invert it) and is bounded above at a sane value until a real lens
    // spec says otherwise.
    if (zoom < 1.0f)  zoom = 1.0f;
    // if (zoom > 20.0f) zoom = 50.0f;
    if (zoom > 50.0f)  zoom = 50.0f;
    zoomLevel = zoom;
    update();
}

void EOOpenGLDisplay::setSlewRate(float degPerSec)
{
    // Clamp to a plausible mechanical gimbal range so a bad caller can't
    // set 0 (gimbal never reaches target) or an absurdly large value.
    if (degPerSec < 1.0f)   degPerSec = 1.0f;
    if (degPerSec > 300.0f) degPerSec = 300.0f;
    slewRateDegPerSec = degPerSec;
}

float EOOpenGLDisplay::normalizeAngle180(float deg)
{
    // Wrap into (-180, 180]. fmod alone isn't enough since deg can be
    // negative or far outside +-360 if a caller passes an unwrapped value.
    deg = std::fmod(deg + 180.0f, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    return deg - 180.0f;
}

float EOOpenGLDisplay::clampElevation(float deg)
{
    if (deg < ELEVATION_MIN_DEG) return ELEVATION_MIN_DEG;
    if (deg > ELEVATION_MAX_DEG) return ELEVATION_MAX_DEG;
    return deg;
}

bool EOOpenGLDisplay::stepSlew(float dt)
{
    // Defensive clamp on dt: never negative, and capped so a long stall
    // (e.g. window minimized, breakpoint hit, first-ever frame) can't make
    // the gimbal jump in one giant step — it just takes a couple of extra
    // ticks to catch up instead.
    if (dt < 0.0f)  dt = 0.0f;
    if (dt > 0.1f)  dt = 0.1f;

    const float maxStep = slewRateDegPerSec * dt;

    // --- Azimuth: shortest angular path, wraparound-aware ---
    float azDiff = normalizeAngle180(commandedAzimuth - currentAzimuth);
    if (std::fabs(azDiff) <= maxStep) {
        currentAzimuth = commandedAzimuth;
    } else {
        currentAzimuth += (azDiff > 0.0f ? maxStep : -maxStep);
    }
    currentAzimuth = normalizeAngle180(currentAzimuth);

    // --- Elevation: linear slew (no wraparound; gimbal limits are Phase 2) ---
    float elDiff = commandedElevation - currentElevation;
    if (std::fabs(elDiff) <= maxStep) {
        currentElevation = commandedElevation;
    } else {
        currentElevation += (elDiff > 0.0f ? maxStep : -maxStep);
    }

    const bool converging =
        std::fabs(normalizeAngle180(commandedAzimuth - currentAzimuth)) > SLEW_CONVERGE_EPS_DEG ||
        std::fabs(commandedElevation - currentElevation) > SLEW_CONVERGE_EPS_DEG;

    return converging;
}

void EOOpenGLDisplay::onSlewTick()
{
    // Just requests a repaint; paintGL() does the actual integration step
    // and is also the single place that decides whether to keep the timer
    // alive (still converging) or stop it (arrived — save CPU).
    update();
}

/* =========== PHASE 1.5: OPERATOR INPUT (click-drag slew + wheel zoom) === */

void EOOpenGLDisplay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Recorded unconditionally (even while TRACK is active) so a plain
        // click can still re-designate a different target mid-track — see
        // mouseReleaseEvent()/designateTargetAtScreenPos().
        m_mousePressScreenPos = event->pos();
    }

    // Manual slew-drag is suspended while TRACK mode has the gimbal — real
    // hand-controllers do the same ("halt platform movement upon
    // detection/lock"). Click-to-designate (below) still works regardless.
    if (!m_trackModeActive && event->button() == Qt::LeftButton) {
        m_dragging           = true;
        m_dragStartPos       = event->pos();
        // Drag from the COMMANDED angle, not the current (still-slewing)
        // angle, so starting a new drag mid-slew doesn't cause a jump —
        // the operator's reference point is "where I last told it to go".
        m_dragStartAzimuth   = commandedAzimuth;
        m_dragStartElevation = commandedElevation;
        setCursor(Qt::ClosedHandCursor);
    }
    QOpenGLWidget::mousePressEvent(event);
}

void EOOpenGLDisplay::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && !m_trackModeActive) {
        const QPoint delta = event->pos() - m_dragStartPos;

        // Degrees-per-pixel derived from the ACTUAL current horizontal FOV,
        // so a given screen-space drag always covers the same fraction of
        // the visible frame regardless of zoom — the real-world behaviour
        // of slewing while looking through a narrow vs wide optical FOV.
        const int w = width();
        float degPerPixel = (w > 0) ? (m_lastHFovDeg / static_cast<float>(w))
                                    : DRAG_DEG_PER_PIXEL_MIN;
        if (degPerPixel < DRAG_DEG_PER_PIXEL_MIN) degPerPixel = DRAG_DEG_PER_PIXEL_MIN;
        if (degPerPixel > DRAG_DEG_PER_PIXEL_MAX) degPerPixel = DRAG_DEG_PER_PIXEL_MAX;

        // Drag right -> LOS moves right (azimuth increases).
        // Drag up (negative pixel y) -> LOS moves up (elevation increases).
        const float newAz = m_dragStartAzimuth   + delta.x() * degPerPixel;
        const float newEl = m_dragStartElevation - delta.y() * degPerPixel;

        setCommandedLOS(newAz, newEl);
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void EOOpenGLDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);

        // A plain click (negligible movement since press) designates
        // whichever detected target is under the cursor — regardless of
        // whether TRACK mode was already active, so the operator can
        // re-designate a different target at any time. A real drag
        // (manual slew) does NOT also trigger a designation.
        const QPoint moved = event->pos() - m_mousePressScreenPos;
        if (moved.manhattanLength() <= CLICK_MAX_MOVE_PX) {
            designateTargetAtScreenPos(event->pos());
        }
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void EOOpenGLDisplay::wheelEvent(QWheelEvent *event)
{
    // Manual zoom is suspended while TRACK mode is auto-framing the
    // locked target — matches real systems where the tracker owns zoom.
    if (!m_trackModeActive) {
        // angleDelta().y() is in eighths of a degree; a standard wheel notch
        // is 120 units (15 degrees), so this normalizes to "notches"
        // regardless of OS/mouse wheel resolution, including trackpads.
        const float notches = event->angleDelta().y() / 120.0f;
        if (notches != 0.0f) {
            const float factor = std::pow(WHEEL_ZOOM_FACTOR_PER_NOTCH, notches);
            setZoom(zoomLevel * factor); // setZoom() clamps to [1, 50] internally
        }
    }
    event->accept();
}

void EOOpenGLDisplay::keyPressEvent(QKeyEvent *event)
{
    // 'T' always works — toggles TRACK mode itself, regardless of current
    // state, so the operator can always get manual control back.
    if (event->key() == Qt::Key_T) {
        m_trackModeActive = !m_trackModeActive;
        if (!m_trackModeActive) {
            // Untracking: drop the lock, hold the current LOS/zoom right
            // where they are rather than snapping anywhere.
            m_lockedTargetId.clear();
            m_lastTrackRangeM = 0.0f;
        }
        update(); // reflect the mode change (HUD text) immediately
        event->accept();
        return;
    }

    // Manual arrow-key nudge is suspended while TRACK mode has the gimbal —
    // same reasoning as the mouse-drag suspension above.
    if (m_trackModeActive) {
        QOpenGLWidget::keyPressEvent(event);
        return;
    }

    // Deliberately only Left/Right/Up/Down — number keys and +/- are
    // reserved elsewhere in the app, so this control scheme avoids them
    // entirely. Nudges are relative to the COMMANDED angle (consistent with
    // the mouse-drag handler), not the currently-slewing angle, so a key
    // press mid-slew adjusts the target smoothly instead of jumping.
    switch (event->key()) {
    case Qt::Key_Left:
        setCommandedLOS(commandedAzimuth - KEY_NUDGE_DEG_PER_PRESS, commandedElevation);
        break;
    case Qt::Key_Right:
        setCommandedLOS(commandedAzimuth + KEY_NUDGE_DEG_PER_PRESS, commandedElevation);
        break;
    case Qt::Key_Up:
        setCommandedLOS(commandedAzimuth, commandedElevation + KEY_NUDGE_DEG_PER_PRESS);
        break;
    case Qt::Key_Down:
        setCommandedLOS(commandedAzimuth, commandedElevation - KEY_NUDGE_DEG_PER_PRESS);
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
        return;
    }
    event->accept();
}

void EOOpenGLDisplay::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    // Add the correct path to your downloaded panoramic image here
    sky.initialize(":/model/airplane/Model/Airplane/Environment.png");
    land.initialize();
    targets.initialize();
}

void EOOpenGLDisplay::resizeGL(int w, int h)
{
    float aspect = qreal(w) / qreal(h ? h : 1);

    // IMPORTANT FIX:
    // Push zNear out to 1.0f or 5.0f to prevent "Z-fighting" (flickering textures).
    // Push zFar out to 50000.0f (50 kilometers) so you can see far away targets.
    const float zNear = 5.0f;
    const float zFar = 500000.0f;
    const float fov = 45.0f;

    projection.setToIdentity();
    projection.perspective(fov, aspect, zNear, zFar);

    layoutHud();
}

void EOOpenGLDisplay::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- 1. EXTRACT SENSOR DATA (moved early: TRACK mode needs both of
    // these before the slew step runs) ---
    float currentAltitude = sensorPoseGeo.altitude;
    float baseFovDeg = (horizontalCameraAngle > 0.0f) ? horizontalCameraAngle : 45.0f;

    // --- 0. TRACK MODE (auto-lock / auto-zoom / follow) ---
    // Runs BEFORE the slew step so a fresh lock (or an updated target
    // position) this frame feeds straight into the normal smooth-slew
    // machinery below, instead of snapping the camera.
    updateTrackedTarget(currentAltitude, baseFovDeg);

    // --- 0b. GIMBAL SLEW STEP (Phase 1) ---
    // Delta-time since the last *painted* frame (not wall-clock since last
    // tick), so this stays correct whether repaints come from the external
    // hierarchy update path, a resize, or our own slewTimer.
    float dt = 0.0f;
    if (frameClockStarted) {
        dt = frameClock.restart() / 1000.0f; // ms -> seconds
    } else {
        frameClock.start();
        frameClockStarted = true; // first-ever frame: dt stays 0, no snap
    }

    const bool stillSlewing = stepSlew(dt);

    // Only keep the repaint-pump timer alive while actually converging (or
    // while TRACK mode is actively following a target, which continuously
    // re-commands the LOS as it moves).
    if (stillSlewing || (m_trackModeActive && !m_lockedTargetId.empty())) {
        if (slewTimer && !slewTimer->isActive()) {
            slewTimer->start(SLEW_TIMER_INTERVAL_MS);
        }
    } else if (slewTimer && slewTimer->isActive()) {
        slewTimer->stop();
    }

    // --- 2. UPDATE CAMERA ROTATION (View Matrix) ---
    // FIX (unchanged from before): target.x/y/z (set below, from
    // EOSensor::scan()) are ALREADY computed relative to the sensor's own
    // boresight — getHorizontalTargetAngle()/getVerticalTargetAngle()
    // subtract sensorHeading/sensorPitch before this data ever reaches here.
    // Rotating the camera AGAIN by the sensor's full world orientation
    // (sensorPoseGeo.rx/ry/rz) would double-apply that rotation.
    //
    // Phase 1 addition: the camera now rotates by currentAzimuth/
    // currentElevation instead of a hardcoded identity quaternion. Since
    // both default to 0 and nothing yet drives them away from 0 (see
    // comment above), fromEulerAngles(0,0,0) IS the identity quaternion —
    // this line is a strict superset of the old behaviour, not a change to
    // it, until something starts calling setCommandedLOS().
    QQuaternion losRotation =
        QQuaternion::fromEulerAngles(currentElevation, currentAzimuth, 0.0f);
    camera.setRotation(losRotation);
    camera.setPosition(QVector3D(0.0f, currentAltitude, 0.0f));
    QMatrix4x4 cameraMatrix = camera.getViewMatrix();
    m_lastViewMatrix = cameraMatrix; // cached for projectToScreen() (track box)
    // --- 3. UPDATE CAMERA LENS (Projection Matrix) ---
    // Safely get aspect ratio to prevent divide-by-zero
    float aspect = qreal(width()) / qreal(height() ? height() : 1);
    // (baseFovDeg already extracted at the top of this function, ahead of
    // the TRACK-mode step, which needs it for the auto-zoom framing calc.)
    // --- Optical zoom (Phase 1): FOV narrows as zoomLevel increases.
    // zoomLevel is clamped >= 1.0 in setZoom(), so this can never divide by
    // zero or widen past baseFovDeg; the extra floor below guards against a
    // degenerate near-zero frustum at extreme zoom.
    float hFovDeg = baseFovDeg / zoomLevel;
    if (hFovDeg < 0.3f) hFovDeg = 0.3f;
    m_lastHFovDeg = hFovDeg; // used by mouseMoveEvent() to scale drag sensitivity
    // Convert HFOV to VFOV for OpenGL
    float hFovRad = qDegreesToRadians(hFovDeg);
    float vFovRad = 2.0f * std::atan(std::tan(hFovRad / 2.0f) / aspect);
    float vFovDeg = qRadiansToDegrees(vFovRad);
    // Rebuild projection matrix dynamically
    projection.setToIdentity();
    // FIX: zFar used to be hardcoded at 50000.0f (50km) regardless of the
    // sensor's own configured range — so a sensor set for e.g. 100km would
    // correctly mark a 70km target as "detected" (EODetectionCoord[id] =
    // {true, ...}) but OpenGL would silently clip it from the render at
    // 50km, making it invisible despite being "detected". zFar now tracks
    // maxRangeMeters (pushed in from EODisplay via setSensorParameter(),
    // sourced from EOSensor::range*1000), with a 50km floor so nothing
    // regresses for sensors configured at/under the old default.
    const float zFar = std::max(50000.0f, maxRangeMeters);
    projection.perspective(vFovDeg, aspect, 5.0f, zFar);
    // --- 4. RENDER ENVIRONMENT ---
    sky.render(projection, cameraMatrix);
    land.render(projection, cameraMatrix);
    // --- 5. RENDER TARGETS ---
    for (const auto& pair : EODetectionCoord) {

        const bool isLocked =
            m_trackModeActive && !m_lockedTargetId.empty()
            && pair.first == m_lockedTargetId;

        if (!pair.second.first && !isLocked)
            continue;

        // if (!pair.second.first)
        //     continue;

        const CartesianCoord& target = pair.second.second;
        QQuaternion targetRotation =
            QQuaternion::fromEulerAngles(
                target.rx,
                target.ry,
                target.rz
                );
        targets.render(
            projection,
            cameraMatrix,
            target.x,
            target.y,
            target.z,
            targetRotation,
            QString(),
            QString()
            );
    }

    // --- 6. HUD OVERLAY ---
    // Runs last, using the final cameraMatrix/projection/hFovDeg for this
    // frame so the tracking bracket box lines up exactly with what was
    // just rendered.
    updateHudText(hFovDeg);
}

/* ===================  TRACK MODE (auto-lock / auto-zoom / follow) === */

void EOOpenGLDisplay::updateTrackedTarget(float currentAltitude, float baseFovDeg)
{
    if (!m_trackModeActive) {
        m_lastTrackRangeM = 0.0f;
        m_hasLastLockedCoord = false;
        return;
    }

    // --- 1) Live locked target ---
    auto lockedIt = EODetectionCoord.end();
    if (!m_lockedTargetId.empty()) {
        auto it = EODetectionCoord.find(m_lockedTargetId);
        if (it != EODetectionCoord.end())
            lockedIt = it;
    }

    // --- 2) Lost: reacquire nearest, else coast on last coord ---
    if (lockedIt == EODetectionCoord.end()) {
        if (m_lockedTargetId.empty()) {
            m_lastTrackRangeM = 0.0f;
            return;
        }

        std::string bestId;
        float bestAngDistSq = -1.0f;
        for (const auto &pair : EODetectionCoord) {
            if (!pair.second.first) continue;
            const CartesianCoord &c = pair.second.second;
            float horizDist = std::sqrt(c.x * c.x + c.z * c.z);
            if (horizDist < TRACK_MIN_RANGE_M) horizDist = TRACK_MIN_RANGE_M;
            const float az = qRadiansToDegrees(std::atan2(c.x, -c.z));
            const float el = qRadiansToDegrees(std::atan2(c.y - currentAltitude, horizDist));
            const float dAz = normalizeAngle180(az - currentAzimuth);
            const float dEl = el - currentElevation;
            const float angDistSq = dAz * dAz + dEl * dEl;
            if (bestAngDistSq < 0.0f || angDistSq < bestAngDistSq) {
                bestAngDistSq = angDistSq;
                bestId = pair.first;
            }
        }

        if (!bestId.empty()) {
            m_lockedTargetId = bestId;
            lockedIt = EODetectionCoord.find(m_lockedTargetId);
        } else {
            // ★ Coast: last known position pe aim hold
            if (m_hasLastLockedCoord) {
                const CartesianCoord &c = m_lastLockedCoord;
                float hd = std::sqrt(c.x * c.x + c.z * c.z);
                if (hd < TRACK_MIN_RANGE_M) hd = TRACK_MIN_RANGE_M;
                setCommandedLOS(
                    -qRadiansToDegrees(std::atan2(c.x, -c.z)),
                    qRadiansToDegrees(std::atan2(c.y - currentAltitude, hd)));
            }
            m_lastTrackRangeM = 0.0f;
            return;
        }
    }

    const CartesianCoord &target = lockedIt->second.second;

    // Cache for next-frame coast
    m_lastLockedCoord = target;
    m_hasLastLockedCoord = true;

    float horizontalDistance = std::sqrt(target.x * target.x + target.z * target.z);
    if (horizontalDistance < TRACK_MIN_RANGE_M)
        horizontalDistance = TRACK_MIN_RANGE_M;

    const float azToTarget = qRadiansToDegrees(std::atan2(target.x, -target.z));
    const float elToTarget = qRadiansToDegrees(
        std::atan2(target.y - currentAltitude, horizontalDistance));

    // Sensor HTA: +az = right of nose. Camera yaw: opposite sign → center lock.
    setCommandedLOS(-azToTarget, elToTarget);

    const float desiredWidthM =
        TRACK_ASSUMED_TARGET_WIDTH_M * TRACK_FRAMING_MULTIPLIER;
    float desiredHFovDeg = qRadiansToDegrees(
        2.0f * std::atan((desiredWidthM * 0.5f) / horizontalDistance));
    if (desiredHFovDeg < 0.3f) desiredHFovDeg = 0.3f;
    if (desiredHFovDeg > baseFovDeg) desiredHFovDeg = baseFovDeg;

    setZoom(baseFovDeg / desiredHFovDeg);
    m_lastTrackRangeM = horizontalDistance;
}

bool EOOpenGLDisplay::designateTargetAtScreenPos(const QPoint &screenPos)
{
    // "Click on the aircraft you want" — finds whichever DETECTED target's
    // projected screen position is nearest the click, within a pixel
    // radius (so clicking empty sky doesn't grab something far away).
    std::string bestId;
    float bestPixelDistSq = -1.0f;
    const float maxPixelRadius = 80.0f;

    for (const auto &pair : EODetectionCoord) {
        if (!pair.second.first) continue; // only actually-detected targets
        const CartesianCoord &c = pair.second.second;
        QPoint proj;
        if (!projectToScreen(QVector3D(c.x, c.y, c.z), proj)) continue;
        const QPoint d = proj - screenPos;
        const float distSq = static_cast<float>(d.x() * d.x() + d.y() * d.y());
        if (bestPixelDistSq < 0.0f || distSq < bestPixelDistSq) {
            bestPixelDistSq = distSq;
            bestId = pair.first;
        }
    }

    if (bestId.empty() || bestPixelDistSq > maxPixelRadius * maxPixelRadius) {
        return false; // nothing detected close enough to the click
    }

    m_lockedTargetId  = bestId;
    m_trackModeActive = true;
    update(); // reflect the new lock (HUD, bracket box) immediately
    return true;
}

bool EOOpenGLDisplay::projectToScreen(const QVector3D &worldPos, QPoint &screenPosOut) const
{
    const QVector4D clip = projection * m_lastViewMatrix * QVector4D(worldPos, 1.0f);
    if (clip.w() <= 0.001f) return false; // behind the camera / degenerate

    const float ndcX = clip.x() / clip.w();
    const float ndcY = clip.y() / clip.w();

    // A little outside [-1,1] is still allowed (partially-visible target
    // near the frame edge), but well outside isn't worth positioning a box
    // for.
    if (ndcX < -1.5f || ndcX > 1.5f || ndcY < -1.5f || ndcY > 1.5f) return false;

    screenPosOut = QPoint(
        static_cast<int>((ndcX * 0.5f + 0.5f) * width()),
        static_cast<int>((1.0f - (ndcY * 0.5f + 0.5f)) * height()));
    return true;
}

/* ===================  HUD OVERLAY  =================== */

void EOOpenGLDisplay::setupHud()
{
    auto styleLabel = [this](QLabel *lbl, int pointSize, bool bold) {
        lbl->setParent(this);
        lbl->setAttribute(Qt::WA_TransparentForMouseEvents); // never blocks drag/click
        QFont f;
        f.setStyleHint(QFont::Monospace);
        f.setFamily("Consolas");
        if (!QFontInfo(f).fixedPitch()) f.setFamily("Monospace");
        f.setPointSize(pointSize);
        f.setBold(bold);
        lbl->setFont(f);
        // Classic monochrome FLIR/targeting-pod green-on-transparent look.
        lbl->setStyleSheet("color: rgb(0, 0, 139); background: transparent;");
        lbl->show();
    };

    m_hudMode = new QLabel(this);
    styleLabel(m_hudMode, 12, true);
    m_hudMode->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_hudMode->setText("MANUAL");

    m_hudLosAzEl = new QLabel(this);
    styleLabel(m_hudLosAzEl, 11, true);
    m_hudLosAzEl->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_hudLosAzEl->setText("AZ 000.0°   EL  +0.0°");

    m_hudCrosshair = new QLabel(this);
    styleLabel(m_hudCrosshair, 20, true);
    m_hudCrosshair->setAlignment(Qt::AlignCenter);
    m_hudCrosshair->setText("+");

    m_hudAltRange = new QLabel(this);
    styleLabel(m_hudAltRange, 9, false);
    m_hudAltRange->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_hudTargetAz = new QLabel(this);
    styleLabel(m_hudTargetAz, 9, false);
    m_hudTargetAz->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_hudFov = new QLabel(this);
    styleLabel(m_hudFov, 10, false);
    m_hudFov->setAlignment(Qt::AlignLeft | Qt::AlignBottom);

    m_hudZoom = new QLabel(this);
    styleLabel(m_hudZoom, 10, false);
    m_hudZoom->setAlignment(Qt::AlignRight | Qt::AlignBottom);

    m_hudCompass = new QLabel(this);
    styleLabel(m_hudCompass, 10, true);
    m_hudCompass->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    m_hudCompass->setText("W  270    ▲ 000    E  090");

    // Tracking bracket box: repositioned onto the locked target's live
    // screen coordinates each frame; just a bordered transparent square,
    // hidden whenever there's no valid on-screen lock.
    m_hudTrackBox = new QLabel(this);
    m_hudTrackBox->setParent(this);
    m_hudTrackBox->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hudTrackBox->setStyleSheet("border: 1px solid rgb(25, 118, 210); background: transparent;");
    m_hudTrackBox->hide();

    // Real, clickable UNTRACK button — the UI equivalent of pressing 'T'.
    // NOT mouse-transparent (it must actually receive the click).
    m_hudUntrackButton = new QPushButton("STOP TRACK", this);
    m_hudUntrackButton->setStyleSheet(
        "QPushButton {"
        "  color: rgb(30, 136, 229);"
        "  background: rgba(0,40,20,190);"
        "  border: 1px solid rgb(0,255,102);"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover   { background: rgba(15, 50, 100, 220); }"
        "QPushButton:pressed { background: rgba(20, 70, 140, 230); }");
    connect(m_hudUntrackButton, &QPushButton::clicked, this, [this]() {
        m_trackModeActive = false;
        m_lockedTargetId.clear();
        m_lastTrackRangeM = 0.0f;
        m_hasLastLockedCoord = false;
        update(); // reflect the mode change (HUD text) immediately
    });
    m_hudUntrackButton->hide(); // shown only while a lock is active
}

void EOOpenGLDisplay::layoutHud()
{
    if (!m_hudMode) return;

    const int w = width();
    const int h = height();
    const int margin = 12;

    m_hudMode->setGeometry(0, margin, w, 22);

    if (m_hudLosAzEl)
        m_hudLosAzEl->setGeometry(0, margin + 22, w, 22);

    const int chSize = 28;
    m_hudCrosshair->setGeometry(
        w / 2 - chSize / 2, h / 2 - chSize / 2, chSize, chSize);

    const int sideBoxW = 180;
    const int sideBoxH = 90;
    m_hudAltRange->setGeometry(
        margin, h / 2 - sideBoxH / 2, sideBoxW, sideBoxH);
    m_hudTargetAz->setGeometry(
        w - margin - sideBoxW, h / 2 - sideBoxH / 2, sideBoxW, sideBoxH);

    // FOV / ZOOM thoda upar (compass ke liye jagah)
    m_hudFov->setGeometry(margin, h - margin - 40, 140, 18);
    m_hudZoom->setGeometry(w - margin - 140, h - margin - 40, 140, 18);

    if (m_hudCompass)
        m_hudCompass->setGeometry(0, h - margin - 22, w, 20);

    const int btnW = 130, btnH = 28;
    m_hudUntrackButton->setGeometry(
        w / 2 - btnW / 2, h - margin - btnH - 48, btnW, btnH);
}

void EOOpenGLDisplay::ensureCandidateBoxCount(int n)
{
    while (m_hudCandidateBoxes.size() < n) {
        QLabel *box = new QLabel(this);
        box->setAttribute(Qt::WA_TransparentForMouseEvents); // clicks pass through to the GL widget
        // Dimmer/thinner than the locked target's bracket (m_hudTrackBox)
        // so the operator can visually tell "detected, clickable" apart
        // from "currently locked".
        box->setStyleSheet("border: 1px solid rgb(0,200,80); background: transparent;");
        box->hide();
        m_hudCandidateBoxes.append(box);
    }
}

void EOOpenGLDisplay::updateHudText(float hFovDeg)
{
    if (!m_hudMode) return;

    const bool locked = m_trackModeActive && !m_lockedTargetId.empty();

    m_hudMode->setText(
        !m_trackModeActive ? "MANUAL" :
            locked             ? "POINT TRACK" :
            "SEARCHING...");

    const float uiAz180 = normalizeAngle180(-currentAzimuth);
    float uiAz360 = uiAz180;
    if (uiAz360 < 0.0f) uiAz360 += 360.0f;
    const float uiEl = currentElevation;

    if (m_hudLosAzEl) {
        m_hudLosAzEl->setText(
            QStringLiteral("AZ %1°   EL %2%3°")
                .arg(uiAz360, 0, 'f', 1)
                .arg(uiEl >= 0.f ? QStringLiteral("+") : QString())
                .arg(uiEl, 0, 'f', 1));
    }
    if (m_hudCompass)
        m_hudCompass->setText(compassTapeString(uiAz360));

    QString altRangeText =
        QString("ALTITUDE\n%1 M").arg(qRound(sensorPoseGeo.altitude));
    if (locked && m_lastTrackRangeM > 0.0f) {
        altRangeText += QString("\nSLANT RANGE\n%1 M").arg(qRound(m_lastTrackRangeM));
    }
    m_hudAltRange->setText(altRangeText);

    QString rightText = lookHint(uiAz180, uiEl);
    if (locked)
        rightText += QString("\n\nTARGET AZ\n%1°").arg(uiAz360, 0, 'f', 1);
    m_hudTargetAz->setText(rightText);

    m_hudFov->setText(QStringLiteral("FOV: %1°").arg(hFovDeg, 0, 'f', 1));
    m_hudZoom->setText(QString("ZOOM: %1x").arg(zoomLevel, 0, 'f', 1));

    // ---------- Locked target: thick green box ----------
    bool showLockBox = false;
    if (locked) {
        auto it = EODetectionCoord.find(m_lockedTargetId);
        if (it != EODetectionCoord.end()) {
            const CartesianCoord &c = it->second.second;
            QPoint screenPos;
            if (projectToScreen(QVector3D(c.x, c.y, c.z), screenPos)) {
                const int boxSize = 60;
                m_hudTrackBox->setGeometry(
                    screenPos.x() - boxSize / 2,
                    screenPos.y() - boxSize / 2,
                    boxSize, boxSize);
                showLockBox = true;
            }
        }
    }
    m_hudTrackBox->setVisible(showLockBox);

    // ---------- Candidate boxes: EVERY detected target (not locked) ----------
    // Yeh pehle se dikhne chahiye — click se pehle
    QVector<QPoint> candidatePositions;
    for (const auto &pair : EODetectionCoord) {
        if (!pair.second.first) continue;
        if (locked && pair.first == m_lockedTargetId) continue; // lock box alag hai

        const CartesianCoord &c = pair.second.second;
        QPoint screenPos;
        if (!projectToScreen(QVector3D(c.x, c.y, c.z), screenPos))
            continue;
        candidatePositions.append(screenPos);
    }

    ensureCandidateBoxCount(candidatePositions.size());

    for (int i = 0; i < m_hudCandidateBoxes.size(); ++i) {
        if (i < candidatePositions.size()) {
            const int boxSize = 48; // thoda chhota / thinner than lock box
            const QPoint &p = candidatePositions[i];
            m_hudCandidateBoxes[i]->setGeometry(
                p.x() - boxSize / 2,
                p.y() - boxSize / 2,
                boxSize, boxSize);
            m_hudCandidateBoxes[i]->show();
        } else {
            m_hudCandidateBoxes[i]->hide();
        }
    }

    // ---------- STOP TRACK button ----------
    if (m_hudUntrackButton)
        m_hudUntrackButton->setVisible(locked);
}
