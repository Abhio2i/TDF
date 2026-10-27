#include "iropengldisplay.h"
#include <QtMath>
#include <QFont>
#include <QFontInfo>
#include <algorithm>

constexpr float EARTH_RADIUS_F = 6378137.0f;
constexpr float PI_F           = 3.141592653589793f;
constexpr float DEG_TO_RAD_F   = PI_F / 180.0f;

float IROpenGLDisplay::normalizeAngle180(float deg)
{
    deg = std::fmod(deg + 180.0f, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    return deg - 180.0f;
}

static float wrap360(float deg)
{
    deg = std::fmod(deg, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    return deg;
}

static QString compassTapeString(float uiAz360)
{
    const float az = wrap360(uiAz360);
    auto slot = [](float d) -> QString {
        d = wrap360(d);
        const int id = qRound(d);
        if (id <= 22 || id >= 338) return QStringLiteral("N  000");
        if (id >= 23  && id <= 67)  return QStringLiteral("NE 045");
        if (id >= 68  && id <= 112) return QStringLiteral("E  090");
        if (id >= 113 && id <= 157) return QStringLiteral("SE 135");
        if (id >= 158 && id <= 202) return QStringLiteral("S  180");
        if (id >= 203 && id <= 247) return QStringLiteral("SW 225");
        if (id >= 248 && id <= 292) return QStringLiteral("W  270");
        return QStringLiteral("NW 315");
    };
    return QStringLiteral("%1    ▲ %2    %3")
        .arg(slot(az - 45.f)).arg(slot(az)).arg(slot(az + 45.f));
}

static QString lookHint(float uiAz180, float el)
{
    const float a = std::fabs(uiAz180);
    const QString h = (a < 30.f) ? QStringLiteral("FWD")
                      : (a < 150.f) ? ((uiAz180 > 0.f) ? QStringLiteral("RIGHT")
                                                       : QStringLiteral("LEFT"))
                                    : QStringLiteral("AFT");
    const QString v = (el > 5.f) ? QStringLiteral("UP")
                      : (el < -15.f) ? QStringLiteral("DOWN")
                                     : QStringLiteral("LEVEL");
    return h + QStringLiteral(" · ") + v;
}

IROpenGLDisplay::IROpenGLDisplay(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    slewTimer = new QTimer(this);
    slewTimer->setSingleShot(false);
    connect(slewTimer, &QTimer::timeout, this, &IROpenGLDisplay::onSlewTick);
    setupHud();
}

IROpenGLDisplay::~IROpenGLDisplay()
{
    if (slewTimer) {
        slewTimer->stop();
    }
}

void IROpenGLDisplay::setSensorParameter(
    const PoseGeo& _sensorPoseGeo,
    float _horizontalCameraAngle,
    const std::unordered_map<std::string,
                             std::pair<bool, CartesianCoord>>& _IRDetectionCoord,
    float _maxRangeMeters)
{
    sensorPoseGeo = _sensorPoseGeo;
    horizontalCameraAngle = _horizontalCameraAngle;
    IRDetectionCoord = _IRDetectionCoord;
    maxRangeMeters = (_maxRangeMeters > 1000.0f) ? _maxRangeMeters : 50000.0f;
}

/* ===================  GIMBAL / SLEW / ZOOM  =================== */

void IROpenGLDisplay::setCommandedLOS(float azimuthDeg, float elevationDeg)
{
    commandedAzimuth   = normalizeAngle180(azimuthDeg);
    commandedElevation = clampElevation(elevationDeg);

    if (slewTimer && !slewTimer->isActive()) {
        slewTimer->start(SLEW_TIMER_INTERVAL_MS);
    }
    update();
}

void IROpenGLDisplay::setZoom(float zoom)
{
    if (zoom < 1.0f)  zoom = 1.0f;
    if (zoom > 50.0f) zoom = 50.0f;
    zoomLevel = zoom;
    update();
}

void IROpenGLDisplay::setSlewRate(float degPerSec)
{
    if (degPerSec < 1.0f)   degPerSec = 1.0f;
    if (degPerSec > 300.0f) degPerSec = 300.0f;
    slewRateDegPerSec = degPerSec;
}

// float IROpenGLDisplay::normalizeAngle180(float deg)
// {
//     deg = std::fmod(deg + 180.0f, 360.0f);
//     if (deg < 0.0f) deg += 360.0f;
//     return deg - 180.0f;
// }


float IROpenGLDisplay::clampElevation(float deg)
{
    if (deg < ELEVATION_MIN_DEG) return ELEVATION_MIN_DEG;
    if (deg > ELEVATION_MAX_DEG) return ELEVATION_MAX_DEG;
    return deg;
}

bool IROpenGLDisplay::stepSlew(float dt)
{
    if (dt < 0.0f)  dt = 0.0f;
    if (dt > 0.1f)  dt = 0.1f;

    const float maxStep = slewRateDegPerSec * dt;

    float azDiff = normalizeAngle180(commandedAzimuth - currentAzimuth);
    if (std::fabs(azDiff) <= maxStep) {
        currentAzimuth = commandedAzimuth;
    } else {
        currentAzimuth += (azDiff > 0.0f ? maxStep : -maxStep);
    }
    currentAzimuth = normalizeAngle180(currentAzimuth);

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

void IROpenGLDisplay::onSlewTick()
{
    update();
}

/* =========== OPERATOR INPUT (click-drag slew + wheel zoom) === */

void IROpenGLDisplay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressScreenPos = event->pos();
    }

    if (!m_trackModeActive && event->button() == Qt::LeftButton) {
        m_dragging           = true;
        m_dragStartPos       = event->pos();
        m_dragStartAzimuth   = commandedAzimuth;
        m_dragStartElevation = commandedElevation;
        setCursor(Qt::ClosedHandCursor);
    }
    QOpenGLWidget::mousePressEvent(event);
}

void IROpenGLDisplay::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && !m_trackModeActive) {
        const QPoint delta = event->pos() - m_dragStartPos;

        const int w = width();
        float degPerPixel = (w > 0) ? (m_lastHFovDeg / static_cast<float>(w))
                                    : DRAG_DEG_PER_PIXEL_MIN;
        if (degPerPixel < DRAG_DEG_PER_PIXEL_MIN) degPerPixel = DRAG_DEG_PER_PIXEL_MIN;
        if (degPerPixel > DRAG_DEG_PER_PIXEL_MAX) degPerPixel = DRAG_DEG_PER_PIXEL_MAX;

        const float newAz = m_dragStartAzimuth   + delta.x() * degPerPixel;
        const float newEl = m_dragStartElevation - delta.y() * degPerPixel;

        setCommandedLOS(newAz, newEl);
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void IROpenGLDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);

        const QPoint moved = event->pos() - m_mousePressScreenPos;
        if (moved.manhattanLength() <= CLICK_MAX_MOVE_PX) {
            designateTargetAtScreenPos(event->pos());
        }
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void IROpenGLDisplay::wheelEvent(QWheelEvent *event)
{
    if (!m_trackModeActive) {
        const float notches = event->angleDelta().y() / 120.0f;
        if (notches != 0.0f) {
            const float factor = std::pow(WHEEL_ZOOM_FACTOR_PER_NOTCH, notches);
            setZoom(zoomLevel * factor);
        }
    }
    event->accept();
}

void IROpenGLDisplay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_T) {
        m_trackModeActive = !m_trackModeActive;
        if (!m_trackModeActive) {
            m_lockedTargetId.clear();
            m_lastTrackRangeM = 0.0f;
        }
        update();
        event->accept();
        return;
    }

    if (m_trackModeActive) {
        QOpenGLWidget::keyPressEvent(event);
        return;
    }

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

void IROpenGLDisplay::initializeGL()
{
    initializeOpenGLFunctions();

    sky.initialize();
    land.initialize();
    targets.initialize();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
}

void IROpenGLDisplay::resizeGL(int w, int h)
{
    float aspect = qreal(w) / qreal(h ? h : 1);

    const float zNear = 5.0f;
    const float zFar = 500000.0f;
    const float fov = 45.0f;

    projection.setToIdentity();
    projection.perspective(fov, aspect, zNear, zFar);

    layoutHud();
}

void IROpenGLDisplay::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- 1. EXTRACT SENSOR DATA ---
    float currentAltitude = sensorPoseGeo.altitude;
    float baseFovDeg = (horizontalCameraAngle > 0.0f) ? horizontalCameraAngle : 45.0f;

    // --- 0. TRACK MODE ---
    updateTrackedTarget(currentAltitude, baseFovDeg);

    // --- 0b. GIMBAL SLEW STEP ---
    float dt = 0.0f;
    if (frameClockStarted) {
        dt = frameClock.restart() / 1000.0f;
    } else {
        frameClock.start();
        frameClockStarted = true;
    }

    const bool stillSlewing = stepSlew(dt);

    if (stillSlewing || (m_trackModeActive && !m_lockedTargetId.empty())) {
        if (slewTimer && !slewTimer->isActive()) {
            slewTimer->start(SLEW_TIMER_INTERVAL_MS);
        }
    } else if (slewTimer && slewTimer->isActive()) {
        slewTimer->stop();
    }

    // --- 2. UPDATE CAMERA ROTATION ---
    // target.x/y/z (from IRSensor::scan()) are already relative to the
    // sensor's boresight — same reasoning as EOOpenGLDisplay. Camera now
    // rotates by currentAzimuth/currentElevation instead of hard identity.
    QQuaternion losRotation =
        QQuaternion::fromEulerAngles(currentElevation, currentAzimuth, 0.0f);
    camera.setRotation(losRotation);
    camera.setPosition(QVector3D(0.0f, currentAltitude, 0.0f));
    QMatrix4x4 cameraMatrix = camera.getViewMatrix();
    m_lastViewMatrix = cameraMatrix;

    // --- 3. UPDATE CAMERA LENS ---
    float aspect = qreal(width()) / qreal(height() ? height() : 1);

    float hFovDeg = baseFovDeg / zoomLevel;
    if (hFovDeg < 0.3f) hFovDeg = 0.3f;
    m_lastHFovDeg = hFovDeg;

    float hFovRad = qDegreesToRadians(hFovDeg);
    float vFovRad = 2.0f * std::atan(std::tan(hFovRad / 2.0f) / aspect);
    float vFovDeg = qRadiansToDegrees(vFovRad);

    projection.setToIdentity();
    const float zFar = std::max(50000.0f, maxRangeMeters);
    projection.perspective(vFovDeg, aspect, 5.0f, zFar);

    // --- 4. RENDER ENVIRONMENT ---
    sky.render();
    land.render(projection, cameraMatrix);

    // --- 5. RENDER TARGETS ---
    for (const auto& pair : IRDetectionCoord) {

        const bool isLocked =
            m_trackModeActive && !m_lockedTargetId.empty()
            && pair.first == m_lockedTargetId;

        if (!pair.second.first && !isLocked)
            continue;

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
    updateHudText(hFovDeg);
}

/* ===================  TRACK MODE  === */

void IROpenGLDisplay::updateTrackedTarget(float currentAltitude, float baseFovDeg)
{
    if (!m_trackModeActive) {
        m_lastTrackRangeM = 0.0f;
        m_hasLastLockedCoord = false;
        return;
    }

    auto lockedIt = IRDetectionCoord.end();
    if (!m_lockedTargetId.empty()) {
        auto it = IRDetectionCoord.find(m_lockedTargetId);
        if (it != IRDetectionCoord.end())
            lockedIt = it;
    }

    if (lockedIt == IRDetectionCoord.end()) {
        if (m_lockedTargetId.empty()) {
            m_lastTrackRangeM = 0.0f;
            return;
        }

        std::string bestId;
        float bestAngDistSq = -1.0f;
        for (const auto &pair : IRDetectionCoord) {
            if (!pair.second.first) continue;
            const CartesianCoord &c = pair.second.second;
            float horizDist = std::sqrt(c.x * c.x + c.z * c.z);
            if (horizDist < TRACK_MIN_RANGE_M) horizDist = TRACK_MIN_RANGE_M;
            const float az = -qRadiansToDegrees(std::atan2(c.x, -c.z));
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
            lockedIt = IRDetectionCoord.find(m_lockedTargetId);
        } else {
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

    m_lastLockedCoord = target;
    m_hasLastLockedCoord = true;

    float horizontalDistance = std::sqrt(target.x * target.x + target.z * target.z);
    if (horizontalDistance < TRACK_MIN_RANGE_M)
        horizontalDistance = TRACK_MIN_RANGE_M;

    const float azToTarget = qRadiansToDegrees(std::atan2(target.x, -target.z));
    const float elToTarget = qRadiansToDegrees(
        std::atan2(target.y - currentAltitude, horizontalDistance));

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

bool IROpenGLDisplay::designateTargetAtScreenPos(const QPoint &screenPos)
{
    std::string bestId;
    float bestPixelDistSq = -1.0f;
    const float maxPixelRadius = 80.0f;

    for (const auto &pair : IRDetectionCoord) {
        if (!pair.second.first) continue;
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
        return false;
    }

    m_lockedTargetId  = bestId;
    m_trackModeActive = true;
    update();
    return true;
}

bool IROpenGLDisplay::projectToScreen(const QVector3D &worldPos, QPoint &screenPosOut) const
{
    const QVector4D clip = projection * m_lastViewMatrix * QVector4D(worldPos, 1.0f);
    if (clip.w() <= 0.001f) return false;

    const float ndcX = clip.x() / clip.w();
    const float ndcY = clip.y() / clip.w();

    if (ndcX < -1.5f || ndcX > 1.5f || ndcY < -1.5f || ndcY > 1.5f) return false;

    screenPosOut = QPoint(
        static_cast<int>((ndcX * 0.5f + 0.5f) * width()),
        static_cast<int>((1.0f - (ndcY * 0.5f + 0.5f)) * height()));
    return true;
}

/* ===================  HUD OVERLAY  =================== */

void IROpenGLDisplay::setupHud()
{
    auto styleLabel = [this](QLabel *lbl, int pointSize, bool bold) {
        lbl->setParent(this);
        lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
        QFont f;
        f.setStyleHint(QFont::Monospace);
        f.setFamily("Consolas");
        if (!QFontInfo(f).fixedPitch()) f.setFamily("Monospace");
        f.setPointSize(pointSize);
        f.setBold(bold);
        lbl->setFont(f);
        // IR convention: white-hot monochrome (vs EO's green) — matches
        // real thermal imager display palettes.
        lbl->setStyleSheet("color: rgb(255,255,255); background: transparent;");
        lbl->show();
    };

    m_hudMode = new QLabel(this);
    styleLabel(m_hudMode, 12, true);
    m_hudMode->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_hudMode->setText("MANUAL");

    m_hudLosAzEl = new QLabel(this);
    styleLabel(m_hudLosAzEl, 11, true);
    m_hudLosAzEl->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_hudLosAzEl->setText(QStringLiteral("AZ 000.0°   EL  +0.0°"));

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
    m_hudCompass->setText(QStringLiteral("W  270    ▲ 000    E  090"));

    m_hudTrackBox = new QLabel(this);
    m_hudTrackBox->setParent(this);
    m_hudTrackBox->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hudTrackBox->setStyleSheet("border: 2px solid rgb(255,255,255); background: transparent;");
    m_hudTrackBox->hide();

    m_hudUntrackButton = new QPushButton("STOP TRACK", this);
    m_hudUntrackButton->setStyleSheet(
        "QPushButton {"
        "  color: rgb(255,255,255);"
        "  background: rgba(40,40,40,190);"
        "  border: 1px solid rgb(255,255,255);"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover   { background: rgba(80,80,80,220); }"
        "QPushButton:pressed { background: rgba(120,120,120,230); }");
    connect(m_hudUntrackButton, &QPushButton::clicked, this, [this]() {
        m_trackModeActive = false;
        m_lockedTargetId.clear();
        m_lastTrackRangeM = 0.0f;
        m_hasLastLockedCoord = false;
        update();
    });
    m_hudUntrackButton->hide();
}

void IROpenGLDisplay::layoutHud()
{
    if (!m_hudMode) return;

    const int w = width();
    const int h = height();
    const int margin = 12;

    m_hudMode->setGeometry(0, margin, w, 24);

    if (m_hudLosAzEl)
        m_hudLosAzEl->setGeometry(0, margin + 22, w, 22);

    const int chSize = 28;
    m_hudCrosshair->setGeometry(w / 2 - chSize / 2, h / 2 - chSize / 2, chSize, chSize);

    const int sideBoxW = 180;
    const int sideBoxH = 90;
    m_hudAltRange->setGeometry(
        margin, h / 2 - sideBoxH / 2, sideBoxW, sideBoxH);
    m_hudTargetAz->setGeometry(
        w - margin - sideBoxW, h / 2 - sideBoxH / 2, sideBoxW, sideBoxH);

    m_hudFov->setGeometry(margin, h - margin - 18, 140, 18);
    m_hudZoom->setGeometry(w - margin - 140, h - margin - 18, 140, 18);

    if (m_hudCompass)
        m_hudCompass->setGeometry(0, h - margin - 22, w, 20);

    const int btnW = 130, btnH = 28;
    m_hudUntrackButton->setGeometry(w / 2 - btnW / 2, h - margin - btnH - 26, btnW, btnH);
}

void IROpenGLDisplay::ensureCandidateBoxCount(int n)
{
    while (m_hudCandidateBoxes.size() < n) {
        QLabel *box = new QLabel(this);
        box->setAttribute(Qt::WA_TransparentForMouseEvents);
        box->setStyleSheet("border: 1px solid rgb(200,200,200); background: transparent;");
        box->hide();
        m_hudCandidateBoxes.append(box);
    }
}

void IROpenGLDisplay::updateHudText(float hFovDeg)
{
    if (!m_hudMode) return;

    const bool locked = m_trackModeActive && !m_lockedTargetId.empty();

    m_hudMode->setText(
        !m_trackModeActive ? "MANUAL" :
            locked             ? "POINT TRACK" :
            "SEARCHING...");

    // FOV sign convention EO jaisa; 0..360 display
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

    bool showLockBox = false;
    if (locked) {
        auto it = IRDetectionCoord.find(m_lockedTargetId);
        if (it != IRDetectionCoord.end()) {
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

    QVector<QPoint> candidatePositions;
    for (const auto &pair : IRDetectionCoord) {
        if (!pair.second.first) continue;
        if (locked && pair.first == m_lockedTargetId) continue;

        const CartesianCoord &c = pair.second.second;
        QPoint screenPos;
        if (!projectToScreen(QVector3D(c.x, c.y, c.z), screenPos))
            continue;
        candidatePositions.append(screenPos);
    }

    ensureCandidateBoxCount(candidatePositions.size());

    for (int i = 0; i < m_hudCandidateBoxes.size(); ++i) {
        if (i < candidatePositions.size()) {
            const int boxSize = 48;
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

    if (m_hudUntrackButton)
        m_hudUntrackButton->setVisible(locked);
}
