/* ========================================================================= */
/* File: radardisplay.cpp                                                    */
/* Purpose: Radar display — mode selector, click-to-lock, air/surface view   */
//               Written by Arti Rajpoot
/* ========================================================================= */


#include "radardisplay.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "qmath.h"
#include <cmath>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QSignalMapper>
#include <core/Debug/console.h>
#include <QTimer>

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
constexpr double ASPECT_RATIO  = 16.0 / 9.0;
constexpr int    MARGIN        = 30;
constexpr int    MODEBAR_H     = 28;   // height of mode selector bar
constexpr int    SYMBOL_SIZE   = 7;
constexpr int    HIT_RADIUS    = 12;   // px — click detection radius on target

// ---------------------------------------------------------------------------
// Unit conversions
// ---------------------------------------------------------------------------
constexpr float KNOTS_PER_MS = 1.94384f;

// ---------------------------------------------------------------------------
// Colours
// ---------------------------------------------------------------------------
static const QColor COL_BG          { 0,   0,   0   };
static const QColor COL_BORDER      { 0,   180, 0   };
static const QColor COL_RING        { 0,   70,  0   };
static const QColor COL_RING_BRIGHT { 0,   130, 0   };
static const QColor COL_RING_LABEL  { 0,   170, 0   };
static const QColor COL_SECTOR_LINE { 0,   120, 0   };
static const QColor COL_SWEEP       { 0,   255, 80  };
static const QColor COL_SWEEP_CONE  { 0,   255, 80,  45 };
static const QColor COL_HUD         { 0,   210, 70  };
static const QColor COL_HUD_DIM     { 0,   100, 35  };
static const QColor COL_CENTER      { 200, 200, 200 };

// Symbol colours
static const QColor COL_AIR         { 0,   220, 80  };
static const QColor COL_ENEMY       { 255, 60,  60  };
static const QColor COL_SURFACE     { 80,  180, 255 };
static const QColor COL_LOCKED_COL  { 255, 60,  60  };   // red reticle
static const QColor COL_LABEL       { 160, 240, 160 };

// Mode bar colours
static const QColor COL_BTN_ACTIVE  { 0,   180, 60  };
static const QColor COL_BTN_IDLE    { 0,   40,  15  };
static const QColor COL_BTN_LOCK    { 200, 0,   0   };   // red when lock active

// Button style sheets
static const QString STYLE_BTN_ACTIVE =
    "QPushButton { background:#003d18; color:#00ff50; border:1px solid #00cc44; "
    "font:bold 8px 'Courier'; padding:2px 8px; }";

static const QString STYLE_BTN_IDLE =
    "QPushButton { background:#000a05; color:#007730; border:1px solid #004420; "
    "font:bold 8px 'Courier'; padding:2px 8px; }";

static const QString STYLE_BTN_LOCK_ACTIVE =
    "QPushButton { background:#3d0000; color:#ff4040; border:1px solid #cc0000; "
    "font:bold 8px 'Courier'; padding:2px 8px; }";

static const QString STYLE_DISP_BTN =
    "QPushButton { background:#001a10; color:#00bb44; border:1px solid #005522; "
    "font:bold 8px 'Courier'; padding:2px 6px; }"
    "QPushButton:hover { background:#002a18; }";

// ===========================================================================
// Coordinate helpers
// ===========================================================================
static QPointF polarToScreen(float rangeKm, float bearingDeg,
                             int cx, int cy,
                             float displayRangeKm, float pixPerKm)
{
    float  pix   = rangeKm * pixPerKm;
    double theta = qDegreesToRadians(static_cast<double>(90.0f - bearingDeg));
    return { cx + pix * std::cos(theta),
            cy - pix * std::sin(theta) };
}

static float normBearing(float b)
{
    while (b >  180.0f) b -= 360.0f;
    while (b < -180.0f) b += 360.0f;
    return b;
}

// ===========================================================================
// Size management
// ===========================================================================
int   RadarDisplay::heightForWidth(int w) const { return qRound(w * ASPECT_RATIO); }
QSize RadarDisplay::sizeHint()      const       { return { 520, heightForWidth(520) }; }
QSize RadarDisplay::minimumSize()   const       { return { 200, heightForWidth(200) }; }

// ===========================================================================
// Constructor
// ===========================================================================
RadarDisplay::RadarDisplay(QWidget* parent) : QWidget(parent)
{
    setStyleSheet("background-color: black;");
    setWindowTitle("Radar Display");
    setMouseTracking(true);

    QSizePolicy pol(QSizePolicy::Preferred, QSizePolicy::Preferred);
    pol.setHeightForWidth(true);
    setSizePolicy(pol);

    // -----------------------------------------------------------------------
    // Mode selector bar — SURV | TWS | LOCK
    // -----------------------------------------------------------------------
    btnSurv = new QPushButton("SURV", this);
    btnTWS  = new QPushButton("TWS",  this);
    btnLock = new QPushButton("LOCK", this);

    connect(btnSurv, &QPushButton::clicked, this, [this]{ onModeButtonClicked(0); });
    connect(btnTWS,  &QPushButton::clicked, this, [this]{ onModeButtonClicked(1); });
    connect(btnLock, &QPushButton::clicked, this, [this]{ onModeButtonClicked(2); });

    // -----------------------------------------------------------------------
    // AIR/SURFACE display mode toggle
    // -----------------------------------------------------------------------
    btnDispMode = new QPushButton("AIR", this);
    btnDispMode->setStyleSheet(STYLE_DISP_BTN);
    connect(btnDispMode, &QPushButton::clicked, this, &RadarDisplay::toggleDisplayMode);

    updateModeButtonStyles();
}

// ===========================================================================
// Button layout — called on resize
// ===========================================================================
void RadarDisplay::repositionButtons()
{
    int w  = width();
    int bh = MODEBAR_H - 4;
    int bw = (w - 2 * MARGIN) / 4;   // 3 mode buttons + 1 display toggle

    btnSurv->setGeometry(MARGIN,              4, bw, bh);
    btnTWS ->setGeometry(MARGIN + bw,         4, bw, bh);
    btnLock->setGeometry(MARGIN + bw * 2,     4, bw, bh);
    btnDispMode->setGeometry(MARGIN + bw * 3, 4, bw, bh);
}

void RadarDisplay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    repositionButtons();
}

// ===========================================================================
// Mode button styles — highlight active mode
// ===========================================================================
void RadarDisplay::updateModeButtonStyles()
{
    if (!sensor) {
        btnSurv->setStyleSheet(STYLE_BTN_IDLE);
        btnTWS ->setStyleSheet(STYLE_BTN_IDLE);
        btnLock->setStyleSheet(STYLE_BTN_IDLE);
        return;
    }

    btnSurv->setStyleSheet(sensor->mode == Sensor::Mode::Search
                               ? STYLE_BTN_ACTIVE : STYLE_BTN_IDLE);
    btnTWS ->setStyleSheet(sensor->mode == Sensor::Mode::TrackWhileScan
                              ? STYLE_BTN_ACTIVE : STYLE_BTN_IDLE);
    btnLock->setStyleSheet(sensor->mode == Sensor::Mode::FireControl
                               ? STYLE_BTN_LOCK_ACTIVE : STYLE_BTN_IDLE);
}

// ===========================================================================
// Mode button click handler
// ===========================================================================
void RadarDisplay::onModeButtonClicked(int modeIndex)
{
    Radar* radar = asRadar();
    if (!radar) return;

    switch (modeIndex)
    {
    case 0:   // SURVEILLANCE
    {
        if (lockedTargetID) radar->breakLock();
        lockedTargetID = 0;
        applyRadarMode(Sensor::Mode::Search);
        break;
    }
    case 1:   // TWS
    {
        if (lockedTargetID) radar->breakLock();
        lockedTargetID = 0;
        applyRadarMode(Sensor::Mode::TrackWhileScan);
        break;
    }
    case 2:   // LOCK — only activates if a target is already selected
    {
        if (lockedTargetID == 0) {
            // No target selected yet — flash LOCK button to indicate
            // operator must click a target first
            btnLock->setText("LOCK?");
            QTimer::singleShot(800, this, [this]{
                btnLock->setText("LOCK");
                updateModeButtonStyles();
            });
            return;
        }
        radar->lockOn(lockedTargetID);
        applyRadarMode(Sensor::Mode::FireControl);
        break;
    }
    }

    updateModeButtonStyles();
    update();
}

// ---------------------------------------------------------------------------
// applyRadarMode
//
// ALIGNMENT CHANGE (radarmodel refactor only):
//   Old: RadarAttributes cfg = radar->getRadarConfig();
//   New: RadarConfig      cfg = radar->getRadarConfig();
//
// All field names used here (mode) exist identically in RadarConfig.
// No functional change whatsoever.
// ---------------------------------------------------------------------------
void RadarDisplay::applyRadarMode(Sensor::Mode m)
{
    Radar* radar = asRadar();
    if (!radar) return;

    // RadarConfig replaces RadarAttributes — field names are identical
    RadarConfig cfg = radar->getRadarConfig();
    switch (m) {
    case Sensor::Mode::Search:         cfg.mode = RadarMode::SURVEILLANCE; break;
    case Sensor::Mode::TrackWhileScan: cfg.mode = RadarMode::TWS;          break;
    case Sensor::Mode::FireControl:    cfg.mode = RadarMode::LOCK_ON;      break;
    default: break;
    }
    radar->setRadarConfig(cfg);
    if (sensor) sensor->mode = m;
    radar->markDisplayRangeDirty();
}

// ===========================================================================
// Display mode toggle (AIR / SURFACE)
// ===========================================================================
void RadarDisplay::toggleDisplayMode()
{
    displayMode = (displayMode == DisplayMode::AIR)
    ? DisplayMode::SURFACE : DisplayMode::AIR;
    btnDispMode->setText(displayMode == DisplayMode::AIR ? "AIR" : "SURF");
    update();
}

void RadarDisplay::setDisplayMode(DisplayMode m)
{
    displayMode = m;
    btnDispMode->setText(displayMode == DisplayMode::AIR ? "AIR" : "SURF");
    update();
}

// ===========================================================================
// Mouse click — target selection for lock-on
// ===========================================================================
void RadarDisplay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    QPointF click = event->pos();

    // Find nearest screen target within HIT_RADIUS
    float bestDist = HIT_RADIUS * HIT_RADIUS;
    int   bestIdx  = -1;

    for (int i = 0; i < screenTargets.size(); ++i) {
        float dx = static_cast<float>(click.x() - screenTargets[i].pos.x());
        float dy = static_cast<float>(click.y() - screenTargets[i].pos.y());
        float d2 = dx*dx + dy*dy;
        if (d2 < bestDist) {
            bestDist = d2;
            bestIdx  = i;
        }
    }

    if (bestIdx < 0) {
        // Click on empty space — deselect
        if (sensor && sensor->mode == Sensor::Mode::FireControl) {
            Radar* radar = asRadar();
            if (radar) radar->breakLock();
            applyRadarMode(Sensor::Mode::Search);
        }
        lockedTargetID  = 0;
        lockedTargetPos = {};
        updateModeButtonStyles();
        update();
        return;
    }

    // Target clicked — select it and switch to LOCK
    const ScreenTarget& st = screenTargets[bestIdx];
    lockedTargetID  = st.id;
    lockedTargetPos = st.pos;

    Radar* radar = asRadar();
    if (radar) radar->lockOn(lockedTargetID);
    applyRadarMode(Sensor::Mode::FireControl);
    updateModeButtonStyles();
    update();

    qDebug().noquote()
        << QString("[ RadarDisplay ]  LOCK-ON: target id=%1  range=%2km  az=%3°")
               .arg(lockedTargetID)
               .arg(st.data.radius, 0, 'f', 1)
               .arg(st.data.angle,  0, 'f', 1);
}

// ===========================================================================
// Entity management
// ===========================================================================
void RadarDisplay::selectEntity(Entity* entit)
{
    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        update();
        return;
    }

    id     = QString::fromStdString(platform->ID);
    entity = platform;
    sensor = nullptr;
    lockedTargetID = 0;

    for (auto const& pair : *entity->sensors->sensors) {
        Sensor* s = pair.second;
        if (s && s->subType == Sensor::SubType::Generic) {
            sensor = s;

            // ALIGNMENT: getRadarConfig() now returns RadarConfig.
            // Fields minAzimuth / maxAzimuth are identical in RadarConfig.
            Radar* r = dynamic_cast<Radar*>(s);
            if (r) {
                scanMinAz = r->getRadarConfig().minAzimuth;
                scanMaxAz = r->getRadarConfig().maxAzimuth;
            } else {
                scanMinAz = -sensor->maxDetectionAngle;
                scanMaxAz =  sensor->maxDetectionAngle;
            }

            bool is360 = (scanMaxAz >= 180.0f);
            setDisplayMode(is360 ? DisplayMode::SURFACE : DisplayMode::AIR);
            setWindowTitle("Radar Display — " +
                           QString::fromStdString(entity->Name));
            break;
        }
    }
    updateModeButtonStyles();
    update();
}

void RadarDisplay::updateRadar()
{
    if (!entity || !sensor) return;

    // ALIGNMENT: getRadarConfig() returns RadarConfig — same field names used.
    Radar* r = dynamic_cast<Radar*>(sensor);
    if (r) {
        scanMinAz = r->getRadarConfig().minAzimuth;
        scanMaxAz = r->getRadarConfig().maxAzimuth;
    } else {
        scanMinAz = -sensor->maxDetectionAngle;
        scanMaxAz =  sensor->maxDetectionAngle;
    }

    updateModeButtonStyles();
    update();
}

void RadarDisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        entity = nullptr;
        sensor = nullptr;
        targets.clear();
        screenTargets.clear();
        lockedTargetID = 0;
        setWindowTitle("Radar Display");
        updateModeButtonStyles();
        update();
    }
}

void RadarDisplay::updateFromJson(const QJsonObject& json)
{
    if (json.contains("range") && json["range"].isDouble())
        if (sensor) sensor->range = static_cast<float>(json["range"].toDouble());

    // Mark dirty so display range recalculates on next scan()
    Radar* radar = asRadar();
    if (radar) radar->markDisplayRangeDirty();

    if (json.contains("azimuth") && json["azimuth"].isDouble())
        if (sensor) sensor->azimuth = static_cast<float>(json["azimuth"].toDouble());
    if (json.contains("current_speed") && json["current_speed"].isDouble())
        current_speed = json["current_speed"].toInt();
    if (json.contains("max_speed") && json["max_speed"].isDouble())
        max_speed = json["max_speed"].toInt();
    if (json.contains("height") && json["height"].isDouble())
        radar_height = json["height"].toInt();

    if (json.contains("targets") && json["targets"].isArray()) {
        targets.clear();
        for (const QJsonValue& val : json["targets"].toArray()) {
            QJsonObject o = val.toObject();
            if (o.contains("angle") && o.contains("radius")) {
                Target t;
                t.entity    = nullptr;
                t.angle     = static_cast<float>(o["angle"].toDouble());
                t.radius    = static_cast<float>(o["radius"].toDouble());
                t.speed     = static_cast<float>(o["speed"].toDouble(0));
                t.direction = static_cast<float>(o["direction"].toDouble(0));
                t.altitude  = static_cast<float>(o["altitude"].toDouble(0));
                targets.append(t);
            }
        }
    }
    update();
}

// ===========================================================================
// Symbols
// ===========================================================================
void RadarDisplay::drawCircle(QPainter& p, int x, int y, int sz, QColor col)
{
    p.setPen(QPen(col, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(x, y), static_cast<qreal>(sz), static_cast<qreal>(sz));
    p.setBrush(col);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(x, y), 2.0, 2.0);
}

void RadarDisplay::drawTriangle(QPainter& p, int x, int y, int sz, QColor col)
{
    QPolygon tri;
    tri << QPoint(x,      y - sz)
        << QPoint(x - sz, y + sz)
        << QPoint(x + sz, y + sz);
    p.setPen(QPen(col, 1.5));
    p.setBrush(QColor(col.red(), col.green(), col.blue(), 40));
    p.drawPolygon(tri);
}

void RadarDisplay::drawSquare(QPainter& p, int x, int y, int sz, QColor col)
{
    p.setPen(QPen(col, 1.5));
    p.setBrush(QColor(col.red(), col.green(), col.blue(), 40));
    p.drawRect(x - sz, y - sz, sz * 2, sz * 2);
}

// ===========================================================================
// Label
// ===========================================================================
void RadarDisplay::drawLabel(QPainter& p, int x, int y,
                             const Target& t, bool isLocked)
{
    p.setFont(QFont("Courier", 7));
    p.setPen(isLocked ? COL_LOCKED_COL : COL_LABEL);

    QString spd = QString("S:%1kt").arg(static_cast<int>(t.speed * KNOTS_PER_MS));
    QString alt = QString("A:%1m") .arg(static_cast<int>(t.altitude));
    QString hdg = QString("H:%1°") .arg(static_cast<int>(t.direction), 3, 10, QChar('0'));
    QString rv  = QString("RV:%1%2m/s")
                     .arg(t.radialVelocity >= 0 ? "+" : "")
                     .arg(t.radialVelocity, 0, 'f', 1);

    int lx = x + SYMBOL_SIZE + 4;
    int ly = y - 14;
    p.drawText(lx, ly,      spd + "  " + alt);
    p.drawText(lx, ly + 11, hdg + "  " + rv);

    // Heading vector
    if (t.speed > 0.5f) {
        double theta = qDegreesToRadians(static_cast<double>(90.0f - t.direction));
        float  vlen  = 14.0f + std::min(t.speed / 30.0f, 1.0f) * 14.0f;
        int    vx    = x + static_cast<int>(vlen * std::cos(theta));
        int    vy    = y - static_cast<int>(vlen * std::sin(theta));
        p.setPen(QPen(isLocked ? COL_LOCKED_COL : COL_LABEL, 1, Qt::DotLine));
        p.drawLine(x, y, vx, vy);
    }
}

// ===========================================================================
// Lock reticle — drawn over the locked target
// ===========================================================================
void RadarDisplay::drawLockReticle(QPainter& p)
{
    if (lockedTargetID == 0 || lockedTargetPos.isNull()) return;

    int x = static_cast<int>(lockedTargetPos.x());
    int y = static_cast<int>(lockedTargetPos.y());
    int r = 14;

    p.setPen(QPen(COL_LOCKED_COL, 1.5));
    p.setBrush(Qt::NoBrush);

    // Corner brackets
    int b = 5;
    // Top-left
    p.drawLine(x - r, y - r, x - r + b, y - r);
    p.drawLine(x - r, y - r, x - r,     y - r + b);
    // Top-right
    p.drawLine(x + r, y - r, x + r - b, y - r);
    p.drawLine(x + r, y - r, x + r,     y - r + b);
    // Bottom-left
    p.drawLine(x - r, y + r, x - r + b, y + r);
    p.drawLine(x - r, y + r, x - r,     y + r - b);
    // Bottom-right
    p.drawLine(x + r, y + r, x + r - b, y + r);
    p.drawLine(x + r, y + r, x + r,     y + r - b);

    // Cross-hairs
    p.setPen(QPen(COL_LOCKED_COL, 1, Qt::DotLine));
    p.drawLine(x - r + b, y, x + r - b, y);
    p.drawLine(x, y - r + b, x, y + r - b);
}

// ===========================================================================
// drawBackground
// ===========================================================================
void RadarDisplay::drawBackground(QPainter& p)
{
    p.setBrush(COL_BG);
    p.setPen(Qt::NoPen);
    p.drawRect(rect());

    // Mode bar background
    p.setBrush(QColor(0, 15, 5));
    p.setPen(QPen(COL_BORDER, 1));
    p.drawRect(MARGIN, 0, width() - 2 * MARGIN, MODEBAR_H);

    // Radar panel border
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(COL_BORDER, 1));
    p.drawRect(MARGIN, MODEBAR_H,
               width()  - 2 * MARGIN,
               height() - MODEBAR_H - MARGIN);
}

void RadarDisplay::drawCenterMark(QPainter& p, int cx, int cy)
{
    p.setBrush(COL_CENTER);
    p.setPen(Qt::NoPen);
    p.drawRect(cx - 3, cy - 3, 6, 6);
}

// ===========================================================================
// AIR MODE
// ===========================================================================
void RadarDisplay::drawAirRings(QPainter& p, int cx, int cy)
{
    float displayRange = sensor ? sensor->range : 100.0f;
    if (displayRange <= 0) return;

    int   panelH = height() - MODEBAR_H - MARGIN;
    float step   = (displayRange <= 30)  ? 5.0f
                 : (displayRange <= 100) ? 10.0f : 20.0f;
    // int   nRings = static_cast<int>(displayRange / step);
    int   nRings = static_cast<int>(std::ceil(displayRange / step));
    p.setFont(QFont("Courier", 7));

    for (int i = 1; i <= nRings; ++i) {
        float rKm  = step * i;
        float pixR = panelH * (rKm / displayRange);
        bool  last = (i == nRings);

        p.setPen(QPen(last ? COL_RING_BRIGHT : COL_RING, 1,
                      last ? Qt::SolidLine : Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(cx, cy),
                      static_cast<qreal>(pixR),
                      static_cast<qreal>(pixR));

        p.setPen(COL_RING_LABEL);
        p.drawText(cx + 3,
                   static_cast<int>(cy - pixR) + 9,
                   QString("%1km").arg(static_cast<int>(rKm)));
    }
}

void RadarDisplay::drawAirSector(QPainter& p, int cx, int cy)
{
    int   panelH = height() - MODEBAR_H - MARGIN;
    float r      = static_cast<float>(panelH) * 1.02f;

    p.setFont(QFont("Courier", 7));

    for (float az : { scanMinAz, scanMaxAz }) {
        double theta = qDegreesToRadians(static_cast<double>(90.0f - az));
        int ex = cx + static_cast<int>(r * std::cos(theta));
        int ey = cy - static_cast<int>(r * std::sin(theta));
        p.setPen(QPen(COL_SECTOR_LINE, 1, Qt::DashLine));
        p.drawLine(cx, cy, ex, ey);
        p.setPen(COL_RING_LABEL);
        p.drawText(ex - 14, ey - 3,
                   QString("%1°").arg(static_cast<int>(az)));
    }
}

void RadarDisplay::drawAirSweepLine(QPainter& p, int cx, int cy)
{
#ifdef RADAR_DEBUG_SWEEP
    if (!sensor) return;
    float currentAz = sensor->azimuth;
    float bw        = (sensor->beamWidth > 0) ? sensor->beamWidth : 3.0f;
    float halfBW    = bw / 2.0f;
    int   panelH    = height() - MODEBAR_H - MARGIN;
    float r         = static_cast<float>(panelH);

    double theta = qDegreesToRadians(static_cast<double>(90.0f - currentAz));
    int ex = cx + static_cast<int>(r * std::cos(theta));
    int ey = cy - static_cast<int>(r * std::sin(theta));

    p.setPen(QPen(COL_SWEEP, 2));
    p.drawLine(cx, cy, ex, ey);

    p.setPen(QPen(COL_SWEEP_CONE, 1));
    for (float side : { -halfBW, +halfBW }) {
        double t2 = qDegreesToRadians(
            static_cast<double>(90.0f - currentAz + side));
        int bx = cx + static_cast<int>(r * std::cos(t2));
        int by = cy - static_cast<int>(r * std::sin(t2));
        p.drawLine(cx, cy, bx, by);
    }

    p.setFont(QFont("Courier", 7));
    p.setPen(COL_SWEEP);
    p.drawText(ex + 3, ey - 3,
               QString("%1°").arg(currentAz, 0, 'f', 1));
#else
    Q_UNUSED(p); Q_UNUSED(cx); Q_UNUSED(cy);
#endif
}

void RadarDisplay::drawAirTargets(QPainter& p, int cx, int cy)
{
    const QVector<Target>& list = (entity && sensor) ? sensor->targets : targets;
    if (list.isEmpty()) return;

    float displayRange = sensor ? sensor->range : 100.0f;
    if (displayRange <= 0) return;

    int   panelH   = height() - MODEBAR_H - MARGIN;
    float pixPerKm = static_cast<float>(panelH) / displayRange;

    screenTargets.clear();

    for (const Target& tgt : list) {
        if (tgt.radius > displayRange) continue;

        // Resolve ID first so we can use it in the sector filter
        uint32_t tid = 0;
        if (tgt.entity)
            tid = static_cast<uint32_t>(std::hash<std::string>{}(tgt.entity->ID));

        bool isLocked = (tid != 0 && tid == lockedTargetID);

        // Skip sector filter for locked target — it must always be visible
        if (!isLocked) {
            float bearing = normBearing(tgt.angle);
            if (bearing < scanMinAz || bearing > scanMaxAz) continue;
        }

        QPointF pos = polarToScreen(tgt.radius, tgt.angle,
                                    cx, cy, displayRange, pixPerKm);
        int tx = static_cast<int>(pos.x());
        int ty = static_cast<int>(pos.y());

        if (isLocked) lockedTargetPos = pos;

        screenTargets.append({ pos, tid, tgt });

        QColor col = isLocked ? COL_LOCKED_COL : COL_AIR;
        drawCircle(p, tx, ty, SYMBOL_SIZE, col);
        drawLabel (p, tx, ty, tgt, isLocked);
    }
}

// ===========================================================================
// SURFACE MODE
// ===========================================================================
void RadarDisplay::drawSurfaceRings(QPainter& p, int cx, int cy, int radius)
{
    float displayRange = sensor ? sensor->range : 100.0f;
    if (displayRange <= 0) return;

    float step   = (displayRange <= 30)  ? 5.0f
                 : (displayRange <= 100) ? 10.0f : 20.0f;
    // int   nRings = static_cast<int>(displayRange / step);
    int   nRings = static_cast<int>(std::ceil(static_cast<double>(displayRange) / step));

    p.setFont(QFont("Courier", 7));

    for (int i = 1; i <= nRings; ++i) {
        float rKm  = step * i;
        float pixR = radius * (rKm / displayRange);
        bool  last = (i == nRings);

        p.setPen(QPen(last ? COL_RING_BRIGHT : COL_RING, 1,
                      last ? Qt::SolidLine : Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(cx, cy),
                      static_cast<qreal>(pixR),
                      static_cast<qreal>(pixR));

        p.setPen(COL_RING_LABEL);
        p.drawText(cx + 3,
                   static_cast<int>(cy - pixR) + 9,
                   QString("%1km").arg(static_cast<int>(rKm)));
    }

    // Cardinal labels
    p.setFont(QFont("Courier", 8, QFont::Bold));
    p.setPen(COL_RING_LABEL);
    float outerR = static_cast<float>(radius);
    p.drawText(cx - 4, static_cast<int>(cy - outerR) - 4, "N");
    p.drawText(cx - 4, static_cast<int>(cy + outerR) + 12, "S");
    p.drawText(static_cast<int>(cx + outerR) + 4, cy + 4,  "E");
    p.drawText(static_cast<int>(cx - outerR) - 12, cy + 4, "W");
}

void RadarDisplay::drawSurfaceSweep(QPainter& p, int cx, int cy, int radius)
{
#ifdef RADAR_DEBUG_SWEEP
    if (!sensor) return;
    float currentAz = sensor->azimuth;
    float bw        = (sensor->beamWidth > 0) ? sensor->beamWidth : 3.0f;
    float halfBW    = bw / 2.0f;
    float r         = static_cast<float>(radius);

    double theta = qDegreesToRadians(static_cast<double>(90.0f - currentAz));
    int ex = cx + static_cast<int>(r * std::cos(theta));
    int ey = cy - static_cast<int>(r * std::sin(theta));

    p.setPen(QPen(COL_SWEEP, 2));
    p.drawLine(cx, cy, ex, ey);

    p.setPen(QPen(COL_SWEEP_CONE, 1));
    for (float side : { -halfBW, +halfBW }) {
        double t2 = qDegreesToRadians(
            static_cast<double>(90.0f - currentAz + side));
        int bx = cx + static_cast<int>(r * std::cos(t2));
        int by = cy - static_cast<int>(r * std::sin(t2));
        p.drawLine(cx, cy, bx, by);
    }

    p.setFont(QFont("Courier", 7));
    p.setPen(COL_SWEEP);
    p.drawText(ex + 3, ey - 3,
               QString("%1°").arg(currentAz, 0, 'f', 1));
#else
    Q_UNUSED(p); Q_UNUSED(cx); Q_UNUSED(cy); Q_UNUSED(radius);
#endif
}

void RadarDisplay::drawSurfaceTargets(QPainter& p, int cx, int cy, int radius)
{
    const QVector<Target>& list = (entity && sensor) ? sensor->targets : targets;
    if (list.isEmpty()) return;

    float displayRange = sensor ? sensor->range : 100.0f;
    if (displayRange <= 0) return;

    float pixPerKm = static_cast<float>(radius) / displayRange;

    screenTargets.clear();

    for (const Target& tgt : list) {
        if (tgt.radius > displayRange) continue;

        QPointF pos = polarToScreen(tgt.radius, tgt.angle,
                                    cx, cy, displayRange, pixPerKm);
        int tx = static_cast<int>(pos.x());
        int ty = static_cast<int>(pos.y());

        uint32_t tid = 0;
        if (tgt.entity)
            tid = static_cast<uint32_t>(std::hash<std::string>{}(tgt.entity->ID));

        bool isLocked = (tid != 0 && tid == lockedTargetID);
        if (isLocked) lockedTargetPos = pos;

        screenTargets.append({ pos, tid, tgt });

        QColor col = isLocked ? COL_LOCKED_COL : COL_SURFACE;
        drawSquare(p, tx, ty, SYMBOL_SIZE, col);
        drawLabel (p, tx, ty, tgt, isLocked);
    }
}

// ===========================================================================
// HUD
// ===========================================================================
void RadarDisplay::drawHUD(QPainter& p)
{
    if (!sensor) return;

    QString modeStr;
    switch (sensor->mode) {
    case Sensor::Mode::Search:         modeStr = "SURV"; break;
    case Sensor::Mode::TrackWhileScan: modeStr = "TWS";  break;
    case Sensor::Mode::FireControl:    modeStr = "LOCK"; break;
    case Sensor::Mode::Track:          modeStr = "TRK";  break;
    default:                           modeStr = "----"; break;
    }

    QString dispStr = (displayMode == DisplayMode::AIR) ? "AIR" : "SURF";

    p.setFont(QFont("Courier", 8));
    p.setPen(COL_HUD);

    int top = MODEBAR_H + 4;
    p.drawText(4, top + 14, QString("RNG  %1km").arg(static_cast<int>(sensor->range)));
    p.drawText(4, top + 26, QString("AZ   %1°") .arg(sensor->azimuth,   0, 'f', 1));
    p.drawText(4, top + 38, QString("BW   %1°") .arg(sensor->beamWidth, 0, 'f', 1));
    p.drawText(4, top + 50, QString("MODE %1")  .arg(modeStr));
    p.drawText(4, top + 62, QString("DISP %1")  .arg(dispStr));

    // Lock status
    if (lockedTargetID != 0) {
        p.setPen(COL_LOCKED_COL);
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(4, top + 76, QString("LOCKED"));
    }

    // Track count
    p.setFont(QFont("Courier", 8));
    p.setPen(COL_HUD);
    p.drawText(4, height() - MARGIN - 8,
               QString("TRK  %1").arg(sensor->targets.size()));

    // Platform name — top right
    if (entity) {
        QString pname = QString::fromStdString(entity->Name);
        int tw = p.fontMetrics().horizontalAdvance(pname);
        p.drawText(width() - MARGIN - tw - 4, MODEBAR_H + 14, pname);
    }

    // Legend — bottom right
    int lx = width()  - MARGIN - 90;
    int ly = height() - MARGIN - 52;
    p.setFont(QFont("Courier", 7));
    p.setPen(COL_HUD_DIM);
    p.drawText(lx, ly, "LEGEND");

    p.setPen(QPen(COL_AIR, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPoint(lx + 6, ly + 12), 4, 4);
    p.setPen(COL_HUD_DIM);
    p.drawText(lx + 14, ly + 16, "AIR");

    {
        QPolygon tri;
        tri << QPoint(lx + 6,  ly + 22)
            << QPoint(lx + 2,  ly + 30)
            << QPoint(lx + 10, ly + 30);
        p.setPen(QPen(COL_ENEMY, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(tri);
    }
    p.setPen(COL_HUD_DIM);
    p.drawText(lx + 14, ly + 30, "ENEMY");

    p.setPen(QPen(COL_SURFACE, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRect(lx + 2, ly + 34, 8, 8);
    p.setPen(COL_HUD_DIM);
    p.drawText(lx + 14, ly + 43, "SURFACE");
}

// ===========================================================================
// paintEvent
// ===========================================================================
void RadarDisplay::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);

    if (displayMode == DisplayMode::AIR) {
        int cx = width()  / 2;
        int cy = height() - MARGIN;

        drawAirRings    (painter, cx, cy);
        drawAirSector   (painter, cx, cy);
        drawAirSweepLine(painter, cx, cy);
        drawCenterMark  (painter, cx, cy);
        drawAirTargets  (painter, cx, cy);

    } else {
        int cx     = width()  / 2;
        int cy     = MODEBAR_H + (height() - MODEBAR_H - MARGIN) / 2;
        int radius = std::min(width() - 2 * MARGIN,
                              height() - MODEBAR_H - MARGIN) / 2;

        drawSurfaceRings  (painter, cx, cy, radius);
        drawSurfaceSweep  (painter, cx, cy, radius);
        drawCenterMark    (painter, cx, cy);
        drawSurfaceTargets(painter, cx, cy, radius);
    }

    drawLockReticle(painter);
    drawHUD        (painter);

    QWidget::paintEvent(event);
}
// /* ========================================================================= */
// /* File: radardisplay.cpp                                                 */
// /* Purpose: Implements radar display widget for visualization              */
// //               Written by Arti Rajpoot
// /* ========================================================================= */
// /* ========================================================================= */
// /* File: radardisplay.cpp                                                    */
// /* Purpose: Radar display — mode selector, click-to-lock, air/surface view   */
// /* ========================================================================= */

// #include "radardisplay.h"
// #include "core/Hierarchy/Utils/entityutils.h"
// #include "core/Hierarchy/hierarchy.h"
// #include "qmath.h"
// #include <cmath>
// #include <QDebug>
// #include <QPainter>
// #include <QPainterPath>
// #include <QResizeEvent>
// #include <QMouseEvent>
// #include <QSignalMapper>
// #include <core/Debug/console.h>
// #include <QTimer>
// // ---------------------------------------------------------------------------
// // Layout
// // ---------------------------------------------------------------------------
// constexpr double ASPECT_RATIO  = 16.0 / 9.0;
// constexpr int    MARGIN        = 30;
// constexpr int    MODEBAR_H     = 28;    // height of mode selector bar
// constexpr int    SYMBOL_SIZE   = 7;
// constexpr int    HIT_RADIUS    = 12;    // px — click detection radius on target

// // ---------------------------------------------------------------------------
// // Unit conversions
// // ---------------------------------------------------------------------------
// constexpr float KNOTS_PER_MS = 1.94384f;

// // ---------------------------------------------------------------------------
// // Colours
// // ---------------------------------------------------------------------------
// static const QColor COL_BG          { 0,   0,   0   };
// static const QColor COL_BORDER      { 0,   180, 0   };
// static const QColor COL_RING        { 0,   70,  0   };
// static const QColor COL_RING_BRIGHT { 0,   130, 0   };
// static const QColor COL_RING_LABEL  { 0,   170, 0   };
// static const QColor COL_SECTOR_LINE { 0,   120, 0   };
// static const QColor COL_SWEEP       { 0,   255, 80  };
// static const QColor COL_SWEEP_CONE  { 0,   255, 80,  45 };
// static const QColor COL_HUD         { 0,   210, 70  };
// static const QColor COL_HUD_DIM     { 0,   100, 35  };
// static const QColor COL_CENTER      { 200, 200, 200 };

// // Symbol colours
// static const QColor COL_AIR         { 0,   220, 80  };
// static const QColor COL_ENEMY       { 255, 60,  60  };
// static const QColor COL_SURFACE     { 80,  180, 255 };
// static const QColor COL_LOCKED_COL  { 255, 60,  60  };   // red reticle
// static const QColor COL_LABEL       { 160, 240, 160 };

// // Mode bar colours
// static const QColor COL_BTN_ACTIVE  { 0,   180, 60  };
// static const QColor COL_BTN_IDLE    { 0,   40,  15  };
// static const QColor COL_BTN_LOCK    { 200, 0,   0   };   // red when lock active

// // Button style sheets
// static const QString STYLE_BTN_ACTIVE =
//     "QPushButton { background:#003d18; color:#00ff50; border:1px solid #00cc44; "
//     "font:bold 8px 'Courier'; padding:2px 8px; }";

// static const QString STYLE_BTN_IDLE =
//     "QPushButton { background:#000a05; color:#007730; border:1px solid #004420; "
//     "font:bold 8px 'Courier'; padding:2px 8px; }";

// static const QString STYLE_BTN_LOCK_ACTIVE =
//     "QPushButton { background:#3d0000; color:#ff4040; border:1px solid #cc0000; "
//     "font:bold 8px 'Courier'; padding:2px 8px; }";

// static const QString STYLE_DISP_BTN =
//     "QPushButton { background:#001a10; color:#00bb44; border:1px solid #005522; "
//     "font:bold 8px 'Courier'; padding:2px 6px; }"
//     "QPushButton:hover { background:#002a18; }";

// // ===========================================================================
// // Coordinate helpers
// // ===========================================================================
// static QPointF polarToScreen(float rangeKm, float bearingDeg,
//                              int cx, int cy,
//                              float displayRangeKm, float pixPerKm)
// {
//     float  pix   = rangeKm * pixPerKm;
//     double theta = qDegreesToRadians(static_cast<double>(90.0f - bearingDeg));
//     return { cx + pix * std::cos(theta),
//             cy - pix * std::sin(theta) };
// }

// static float normBearing(float b)
// {
//     while (b >  180.0f) b -= 360.0f;
//     while (b < -180.0f) b += 360.0f;
//     return b;
// }

// // ===========================================================================
// // Size management
// // ===========================================================================
// int   RadarDisplay::heightForWidth(int w) const { return qRound(w * ASPECT_RATIO); }
// QSize RadarDisplay::sizeHint()      const       { return { 520, heightForWidth(520) }; }
// QSize RadarDisplay::minimumSize()   const       { return { 200, heightForWidth(200) }; }

// // ===========================================================================
// // Constructor
// // ===========================================================================
// RadarDisplay::RadarDisplay(QWidget* parent) : QWidget(parent)
// {
//     setStyleSheet("background-color: black;");
//     setWindowTitle("Radar Display");
//     setMouseTracking(true);

//     QSizePolicy pol(QSizePolicy::Preferred, QSizePolicy::Preferred);
//     pol.setHeightForWidth(true);
//     setSizePolicy(pol);

//     // -----------------------------------------------------------------------
//     // Mode selector bar — SURV | TWS | LOCK
//     // -----------------------------------------------------------------------
//     btnSurv = new QPushButton("SURV", this);
//     btnTWS  = new QPushButton("TWS",  this);
//     btnLock = new QPushButton("LOCK", this);

//     // Use lambdas to pass mode index
//     connect(btnSurv, &QPushButton::clicked, this, [this]{ onModeButtonClicked(0); });
//     connect(btnTWS,  &QPushButton::clicked, this, [this]{ onModeButtonClicked(1); });
//     connect(btnLock, &QPushButton::clicked, this, [this]{ onModeButtonClicked(2); });

//     // -----------------------------------------------------------------------
//     // AIR/SURFACE display mode toggle
//     // -----------------------------------------------------------------------
//     btnDispMode = new QPushButton("AIR", this);
//     btnDispMode->setStyleSheet(STYLE_DISP_BTN);
//     connect(btnDispMode, &QPushButton::clicked, this, &RadarDisplay::toggleDisplayMode);

//     updateModeButtonStyles();
// }

// // ===========================================================================
// // Button layout — called on resize
// // ===========================================================================
// void RadarDisplay::repositionButtons()
// {
//     int w  = width();
//     int bh = MODEBAR_H - 4;
//     int bw = (w - 2 * MARGIN) / 4;   // 3 mode buttons + 1 display toggle

//     btnSurv->setGeometry(MARGIN,              4, bw, bh);
//     btnTWS ->setGeometry(MARGIN + bw,         4, bw, bh);
//     btnLock->setGeometry(MARGIN + bw * 2,     4, bw, bh);
//     btnDispMode->setGeometry(MARGIN + bw * 3, 4, bw, bh);
// }

// void RadarDisplay::resizeEvent(QResizeEvent* event)
// {
//     QWidget::resizeEvent(event);
//     repositionButtons();
// }

// // ===========================================================================
// // Mode button styles — highlight active mode
// // ===========================================================================
// void RadarDisplay::updateModeButtonStyles()
// {
//     if (!sensor) {
//         btnSurv->setStyleSheet(STYLE_BTN_IDLE);
//         btnTWS ->setStyleSheet(STYLE_BTN_IDLE);
//         btnLock->setStyleSheet(STYLE_BTN_IDLE);
//         return;
//     }

//     btnSurv->setStyleSheet(sensor->mode == Sensor::Mode::Search
//                                ? STYLE_BTN_ACTIVE : STYLE_BTN_IDLE);
//     btnTWS ->setStyleSheet(sensor->mode == Sensor::Mode::TrackWhileScan
//                               ? STYLE_BTN_ACTIVE : STYLE_BTN_IDLE);
//     btnLock->setStyleSheet(sensor->mode == Sensor::Mode::FireControl
//                                ? STYLE_BTN_LOCK_ACTIVE : STYLE_BTN_IDLE);
// }

// // ===========================================================================
// // Mode button click handler
// // ===========================================================================
// void RadarDisplay::onModeButtonClicked(int modeIndex)
// {
//     Radar* radar = asRadar();
//     if (!radar) return;

//     switch (modeIndex)
//     {
//     case 0:   // SURVEILLANCE
//     {
//         if (lockedTargetID) radar->breakLock();
//         lockedTargetID = 0;
//         applyRadarMode(Sensor::Mode::Search);
//         break;
//     }
//     case 1:   // TWS
//     {
//         if (lockedTargetID) radar->breakLock();
//         lockedTargetID = 0;
//         applyRadarMode(Sensor::Mode::TrackWhileScan);
//         break;
//     }
//     case 2:   // LOCK — only activates if a target is already selected
//     {
//         if (lockedTargetID == 0) {
//             // No target selected yet — flash LOCK button to indicate
//             // operator must click a target first
//             btnLock->setText("LOCK?");
//             QTimer::singleShot(800, this, [this]{
//                 btnLock->setText("LOCK");
//                 updateModeButtonStyles();
//             });
//             return;
//         }
//         radar->lockOn(lockedTargetID);
//         applyRadarMode(Sensor::Mode::FireControl);
//         break;
//     }
//     }

//     updateModeButtonStyles();
//     update();
// }

// void RadarDisplay::applyRadarMode(Sensor::Mode m)
// {
//     Radar* radar = asRadar();
//     if (!radar) return;

//     RadarAttributes cfg = radar->getRadarConfig();
//     switch (m) {
//     case Sensor::Mode::Search:         cfg.mode = RadarMode::SURVEILLANCE; break;
//     case Sensor::Mode::TrackWhileScan: cfg.mode = RadarMode::TWS;          break;
//     case Sensor::Mode::FireControl:    cfg.mode = RadarMode::LOCK_ON;      break;
//     default: break;
//     }
//     radar->setRadarConfig(cfg);
//     if (sensor) sensor->mode = m;
//     radar->markDisplayRangeDirty();  // ← ADD THIS

// }

// // ===========================================================================
// // Display mode toggle (AIR / SURFACE)
// // ===========================================================================
// void RadarDisplay::toggleDisplayMode()
// {
//     displayMode = (displayMode == DisplayMode::AIR)
//     ? DisplayMode::SURFACE : DisplayMode::AIR;
//     btnDispMode->setText(displayMode == DisplayMode::AIR ? "AIR" : "SURF");
//     update();
// }

// void RadarDisplay::setDisplayMode(DisplayMode m)
// {
//     displayMode = m;
//     btnDispMode->setText(displayMode == DisplayMode::AIR ? "AIR" : "SURF");
//     update();
// }

// // ===========================================================================
// // Mouse click — target selection for lock-on
// // ===========================================================================
// void RadarDisplay::mousePressEvent(QMouseEvent* event)
// {
//     if (event->button() != Qt::LeftButton) return;

//     QPointF click = event->pos();

//     // Find nearest screen target within HIT_RADIUS
//     float   bestDist = HIT_RADIUS * HIT_RADIUS;
//     int     bestIdx  = -1;

//     for (int i = 0; i < screenTargets.size(); ++i) {
//         float dx = static_cast<float>(click.x() - screenTargets[i].pos.x());
//         float dy = static_cast<float>(click.y() - screenTargets[i].pos.y());
//         float d2 = dx*dx + dy*dy;
//         if (d2 < bestDist) {
//             bestDist = d2;
//             bestIdx  = i;
//         }
//     }

//     if (bestIdx < 0) {
//         // Click on empty space — deselect
//         if (sensor && sensor->mode == Sensor::Mode::FireControl) {
//             Radar* radar = asRadar();
//             if (radar) radar->breakLock();
//             applyRadarMode(Sensor::Mode::Search);
//         }
//         lockedTargetID  = 0;
//         lockedTargetPos = {};
//         updateModeButtonStyles();
//         update();
//         return;
//     }

//     // Target clicked — select it and switch to LOCK
//     const ScreenTarget& st = screenTargets[bestIdx];
//     lockedTargetID  = st.id;
//     lockedTargetPos = st.pos;

//     Radar* radar = asRadar();
//     if (radar) radar->lockOn(lockedTargetID);
//     applyRadarMode(Sensor::Mode::FireControl);
//     updateModeButtonStyles();
//     update();

//     qDebug().noquote()
//         << QString("[ RadarDisplay ]  LOCK-ON: target id=%1  range=%2km  az=%3°")
//                .arg(lockedTargetID)
//                .arg(st.data.radius, 0, 'f', 1)
//                .arg(st.data.angle,  0, 'f', 1);
// }

// // ===========================================================================
// // Entity management
// // ===========================================================================
// void RadarDisplay::selectEntity(Entity* entit)
// {
//     Platform* platform = dynamic_cast<Platform*>(entit);
//     if (!platform) {
//         Console::error("Entity is not a Platform");
//         update();
//         return;
//     }

//     id     = QString::fromStdString(platform->ID);
//     entity = platform;
//     sensor = nullptr;
//     lockedTargetID = 0;

//     for (auto const& pair : *entity->sensors->sensors) {
//         Sensor* s = pair.second;
//         if (s && s->subType == Sensor::SubType::Generic) {
//             sensor = s;

//             Radar* r = dynamic_cast<Radar*>(s);
//             if (r) {
//                 scanMinAz = r->getRadarConfig().minAzimuth;
//                 scanMaxAz = r->getRadarConfig().maxAzimuth;
//             } else {
//                 scanMinAz = -sensor->maxDetectionAngle;
//                 scanMaxAz =  sensor->maxDetectionAngle;
//             }

//             bool is360 = (scanMaxAz >= 180.0f);
//             setDisplayMode(is360 ? DisplayMode::SURFACE : DisplayMode::AIR);
//             setWindowTitle("Radar Display — " +
//                            QString::fromStdString(entity->Name));
//             break;
//         }
//     }
//     updateModeButtonStyles();
//     update();
// }

// void RadarDisplay::updateRadar()
// {
//     if (!entity || !sensor) return;

//     Radar* r = dynamic_cast<Radar*>(sensor);
//     if (r) {
//         scanMinAz = r->getRadarConfig().minAzimuth;
//         scanMaxAz = r->getRadarConfig().maxAzimuth;
//     } else {
//         scanMinAz = -sensor->maxDetectionAngle;
//         scanMaxAz =  sensor->maxDetectionAngle;
//     }



//     updateModeButtonStyles();
//     update();
// }

// void RadarDisplay::RemoveEntity(QString ID)
// {
//     if (id == ID) {
//         entity = nullptr;
//         sensor = nullptr;
//         targets.clear();
//         screenTargets.clear();
//         lockedTargetID = 0;
//         setWindowTitle("Radar Display");
//         updateModeButtonStyles();
//         update();
//     }
// }



// void RadarDisplay::updateFromJson(const QJsonObject& json)
// {
//     if (json.contains("range")   && json["range"].isDouble())
//         if (sensor) sensor->range = static_cast<float>(json["range"].toDouble());

//     // Mark dirty so display range recalculates on next scan()
//     Radar* radar = asRadar();
//     if (radar) radar->markDisplayRangeDirty();
//     if (json.contains("azimuth") && json["azimuth"].isDouble())
//         if (sensor) sensor->azimuth = static_cast<float>(json["azimuth"].toDouble());
//     if (json.contains("current_speed") && json["current_speed"].isDouble())
//         current_speed = json["current_speed"].toInt();
//     if (json.contains("max_speed") && json["max_speed"].isDouble())
//         max_speed = json["max_speed"].toInt();
//     if (json.contains("height") && json["height"].isDouble())
//         radar_height = json["height"].toInt();

//     if (json.contains("targets") && json["targets"].isArray()) {
//         targets.clear();
//         for (const QJsonValue& val : json["targets"].toArray()) {
//             QJsonObject o = val.toObject();
//             if (o.contains("angle") && o.contains("radius")) {
//                 Target t;
//                 t.entity    = nullptr;
//                 t.angle     = static_cast<float>(o["angle"].toDouble());
//                 t.radius    = static_cast<float>(o["radius"].toDouble());
//                 t.speed     = static_cast<float>(o["speed"].toDouble(0));
//                 t.direction = static_cast<float>(o["direction"].toDouble(0));
//                 t.altitude  = static_cast<float>(o["altitude"].toDouble(0));
//                 targets.append(t);
//             }
//         }
//     }
//     update();
// }

// // ===========================================================================
// // Symbols
// // ===========================================================================
// void RadarDisplay::drawCircle(QPainter& p, int x, int y, int sz, QColor col)
// {
//     p.setPen(QPen(col, 1.5));
//     p.setBrush(Qt::NoBrush);
//     p.drawEllipse(QPointF(x, y), static_cast<qreal>(sz), static_cast<qreal>(sz));
//     p.setBrush(col);
//     p.setPen(Qt::NoPen);
//     p.drawEllipse(QPointF(x, y), 2.0, 2.0);
// }

// void RadarDisplay::drawTriangle(QPainter& p, int x, int y, int sz, QColor col)
// {
//     QPolygon tri;
//     tri << QPoint(x,      y - sz)
//         << QPoint(x - sz, y + sz)
//         << QPoint(x + sz, y + sz);
//     p.setPen(QPen(col, 1.5));
//     p.setBrush(QColor(col.red(), col.green(), col.blue(), 40));
//     p.drawPolygon(tri);
// }

// void RadarDisplay::drawSquare(QPainter& p, int x, int y, int sz, QColor col)
// {
//     p.setPen(QPen(col, 1.5));
//     p.setBrush(QColor(col.red(), col.green(), col.blue(), 40));
//     p.drawRect(x - sz, y - sz, sz * 2, sz * 2);
// }

// // ===========================================================================
// // Label
// // ===========================================================================
// void RadarDisplay::drawLabel(QPainter& p, int x, int y,
//                              const Target& t, bool isLocked)
// {
//     p.setFont(QFont("Courier", 7));
//     p.setPen(isLocked ? COL_LOCKED_COL : COL_LABEL);

//     QString spd = QString("S:%1kt").arg(static_cast<int>(t.speed * KNOTS_PER_MS));
//     QString alt = QString("A:%1m") .arg(static_cast<int>(t.altitude));
//     QString hdg = QString("H:%1°") .arg(static_cast<int>(t.direction), 3, 10, QChar('0'));
//     QString rv  = QString("RV:%1%2m/s")
//                      .arg(t.radialVelocity >= 0 ? "+" : "")
//                      .arg(t.radialVelocity, 0, 'f', 1);

//     int lx = x + SYMBOL_SIZE + 4;
//     int ly = y - 14;
//     p.drawText(lx, ly,      spd + "  " + alt);
//     p.drawText(lx, ly + 11, hdg + "  " + rv);

//     // Heading vector
//     if (t.speed > 0.5f) {
//         double theta = qDegreesToRadians(static_cast<double>(90.0f - t.direction));
//         float  vlen  = 14.0f + std::min(t.speed / 30.0f, 1.0f) * 14.0f;
//         int    vx    = x + static_cast<int>(vlen * std::cos(theta));
//         int    vy    = y - static_cast<int>(vlen * std::sin(theta));
//         p.setPen(QPen(isLocked ? COL_LOCKED_COL : COL_LABEL, 1, Qt::DotLine));
//         p.drawLine(x, y, vx, vy);
//     }
// }

// // ===========================================================================
// // Lock reticle — drawn over the locked target
// // ===========================================================================
// void RadarDisplay::drawLockReticle(QPainter& p)
// {
//     if (lockedTargetID == 0 || lockedTargetPos.isNull()) return;

//     int x = static_cast<int>(lockedTargetPos.x());
//     int y = static_cast<int>(lockedTargetPos.y());
//     int r = 14;

//     p.setPen(QPen(COL_LOCKED_COL, 1.5));
//     p.setBrush(Qt::NoBrush);

//     // Corner brackets
//     int b = 5;
//     // Top-left
//     p.drawLine(x - r, y - r, x - r + b, y - r);
//     p.drawLine(x - r, y - r, x - r,     y - r + b);
//     // Top-right
//     p.drawLine(x + r, y - r, x + r - b, y - r);
//     p.drawLine(x + r, y - r, x + r,     y - r + b);
//     // Bottom-left
//     p.drawLine(x - r, y + r, x - r + b, y + r);
//     p.drawLine(x - r, y + r, x - r,     y + r - b);
//     // Bottom-right
//     p.drawLine(x + r, y + r, x + r - b, y + r);
//     p.drawLine(x + r, y + r, x + r,     y + r - b);

//     // Cross-hairs
//     p.setPen(QPen(COL_LOCKED_COL, 1, Qt::DotLine));
//     p.drawLine(x - r + b, y, x + r - b, y);
//     p.drawLine(x, y - r + b, x, y + r - b);
// }

// // ===========================================================================
// // drawBackground
// // ===========================================================================
// void RadarDisplay::drawBackground(QPainter& p)
// {
//     p.setBrush(COL_BG);
//     p.setPen(Qt::NoPen);
//     p.drawRect(rect());

//     // Mode bar background
//     p.setBrush(QColor(0, 15, 5));
//     p.setPen(QPen(COL_BORDER, 1));
//     p.drawRect(MARGIN, 0, width() - 2 * MARGIN, MODEBAR_H);

//     // Radar panel border
//     p.setBrush(Qt::NoBrush);
//     p.setPen(QPen(COL_BORDER, 1));
//     p.drawRect(MARGIN, MODEBAR_H,
//                width()  - 2 * MARGIN,
//                height() - MODEBAR_H - MARGIN);
// }

// void RadarDisplay::drawCenterMark(QPainter& p, int cx, int cy)
// {
//     p.setBrush(COL_CENTER);
//     p.setPen(Qt::NoPen);
//     p.drawRect(cx - 3, cy - 3, 6, 6);
// }

// // ===========================================================================
// // AIR MODE
// // ===========================================================================
// void RadarDisplay::drawAirRings(QPainter& p, int cx, int cy)
// {
//     float displayRange = sensor ? sensor->range : 100.0f;
//     if (displayRange <= 0) return;

//     int   panelH = height() - MODEBAR_H - MARGIN;
//     float step   = (displayRange <= 30)  ? 5.0f
//                  : (displayRange <= 100) ? 10.0f : 20.0f;
//     int   nRings = static_cast<int>(displayRange / step);

//     p.setFont(QFont("Courier", 7));

//     for (int i = 1; i <= nRings; ++i) {
//         float rKm  = step * i;
//         float pixR = panelH * (rKm / displayRange);
//         bool  last = (i == nRings);

//         p.setPen(QPen(last ? COL_RING_BRIGHT : COL_RING, 1,
//                       last ? Qt::SolidLine : Qt::DashLine));
//         p.setBrush(Qt::NoBrush);
//         p.drawEllipse(QPointF(cx, cy),
//                       static_cast<qreal>(pixR),
//                       static_cast<qreal>(pixR));

//         p.setPen(COL_RING_LABEL);
//         p.drawText(cx + 3,
//                    static_cast<int>(cy - pixR) + 9,
//                    QString("%1km").arg(static_cast<int>(rKm)));
//     }
// }

// void RadarDisplay::drawAirSector(QPainter& p, int cx, int cy)
// {
//     int   panelH = height() - MODEBAR_H - MARGIN;
//     float r      = static_cast<float>(panelH) * 1.02f;

//     p.setFont(QFont("Courier", 7));

//     for (float az : { scanMinAz, scanMaxAz }) {
//         double theta = qDegreesToRadians(static_cast<double>(90.0f - az));
//         int ex = cx + static_cast<int>(r * std::cos(theta));
//         int ey = cy - static_cast<int>(r * std::sin(theta));
//         p.setPen(QPen(COL_SECTOR_LINE, 1, Qt::DashLine));
//         p.drawLine(cx, cy, ex, ey);
//         p.setPen(COL_RING_LABEL);
//         p.drawText(ex - 14, ey - 3,
//                    QString("%1°").arg(static_cast<int>(az)));
//     }
// }

// void RadarDisplay::drawAirSweepLine(QPainter& p, int cx, int cy)
// {
// #ifdef RADAR_DEBUG_SWEEP
//     if (!sensor) return;
//     float currentAz = sensor->azimuth;
//     float bw        = (sensor->beamWidth > 0) ? sensor->beamWidth : 3.0f;
//     float halfBW    = bw / 2.0f;
//     int   panelH    = height() - MODEBAR_H - MARGIN;
//     float r         = static_cast<float>(panelH);

//     double theta = qDegreesToRadians(static_cast<double>(90.0f - currentAz));
//     int ex = cx + static_cast<int>(r * std::cos(theta));
//     int ey = cy - static_cast<int>(r * std::sin(theta));

//     p.setPen(QPen(COL_SWEEP, 2));
//     p.drawLine(cx, cy, ex, ey);

//     p.setPen(QPen(COL_SWEEP_CONE, 1));
//     for (float side : { -halfBW, +halfBW }) {
//         double t2 = qDegreesToRadians(
//             static_cast<double>(90.0f - currentAz + side));
//         int bx = cx + static_cast<int>(r * std::cos(t2));
//         int by = cy - static_cast<int>(r * std::sin(t2));
//         p.drawLine(cx, cy, bx, by);
//     }

//     p.setFont(QFont("Courier", 7));
//     p.setPen(COL_SWEEP);
//     p.drawText(ex + 3, ey - 3,
//                QString("%1°").arg(currentAz, 0, 'f', 1));
// #else
//     Q_UNUSED(p); Q_UNUSED(cx); Q_UNUSED(cy);
// #endif
// }

// void RadarDisplay::drawAirTargets(QPainter& p, int cx, int cy)
// {
//     const QVector<Target>& list = (entity && sensor) ? sensor->targets : targets;
//     if (list.isEmpty()) return;

//     float displayRange = sensor ? sensor->range : 100.0f;
//     if (displayRange <= 0) return;

//     int   panelH   = height() - MODEBAR_H - MARGIN;
//     float pixPerKm = static_cast<float>(panelH) / displayRange;

//     screenTargets.clear();

//     for (const Target& tgt : list) {
//         if (tgt.radius > displayRange) continue;

//         // Resolve ID first so we can use it in the sector filter
//         uint32_t tid = 0;
//         if (tgt.entity)
//             tid = static_cast<uint32_t>(std::hash<std::string>{}(tgt.entity->ID));

//         bool isLocked = (tid != 0 && tid == lockedTargetID);

//         // Skip sector filter for locked target — it must always be visible
//         if (!isLocked) {
//             float bearing = normBearing(tgt.angle);
//             if (bearing < scanMinAz || bearing > scanMaxAz) continue;
//         }

//         QPointF pos = polarToScreen(tgt.radius, tgt.angle,
//                                     cx, cy, displayRange, pixPerKm);
//         int tx = static_cast<int>(pos.x());
//         int ty = static_cast<int>(pos.y());

//         if (isLocked) lockedTargetPos = pos;

//         screenTargets.append({ pos, tid, tgt });

//         QColor col = isLocked ? COL_LOCKED_COL : COL_AIR;
//         drawCircle(p, tx, ty, SYMBOL_SIZE, col);
//         drawLabel (p, tx, ty, tgt, isLocked);
//     }
// }

// // ===========================================================================
// // SURFACE MODE
// // ===========================================================================
// void RadarDisplay::drawSurfaceRings(QPainter& p, int cx, int cy, int radius)
// {
//     float displayRange = sensor ? sensor->range : 100.0f;
//     if (displayRange <= 0) return;

//     float step  = (displayRange <= 30)  ? 5.0f
//                  : (displayRange <= 100) ? 10.0f : 20.0f;
//     int   nRings = static_cast<int>(displayRange / step);

//     p.setFont(QFont("Courier", 7));

//     for (int i = 1; i <= nRings; ++i) {
//         float rKm  = step * i;
//         float pixR = radius * (rKm / displayRange);
//         bool  last = (i == nRings);

//         p.setPen(QPen(last ? COL_RING_BRIGHT : COL_RING, 1,
//                       last ? Qt::SolidLine : Qt::DashLine));
//         p.setBrush(Qt::NoBrush);
//         p.drawEllipse(QPointF(cx, cy),
//                       static_cast<qreal>(pixR),
//                       static_cast<qreal>(pixR));

//         p.setPen(COL_RING_LABEL);
//         p.drawText(cx + 3,
//                    static_cast<int>(cy - pixR) + 9,
//                    QString("%1km").arg(static_cast<int>(rKm)));
//     }

//     // Cardinal labels
//     p.setFont(QFont("Courier", 8, QFont::Bold));
//     p.setPen(COL_RING_LABEL);
//     float outerR = static_cast<float>(radius);
//     p.drawText(cx - 4, static_cast<int>(cy - outerR) - 4, "N");
//     p.drawText(cx - 4, static_cast<int>(cy + outerR) + 12, "S");
//     p.drawText(static_cast<int>(cx + outerR) + 4, cy + 4,  "E");
//     p.drawText(static_cast<int>(cx - outerR) - 12, cy + 4, "W");
// }

// void RadarDisplay::drawSurfaceSweep(QPainter& p, int cx, int cy, int radius)
// {
// #ifdef RADAR_DEBUG_SWEEP
//     if (!sensor) return;
//     float currentAz = sensor->azimuth;
//     float bw        = (sensor->beamWidth > 0) ? sensor->beamWidth : 3.0f;
//     float halfBW    = bw / 2.0f;
//     float r         = static_cast<float>(radius);

//     double theta = qDegreesToRadians(static_cast<double>(90.0f - currentAz));
//     int ex = cx + static_cast<int>(r * std::cos(theta));
//     int ey = cy - static_cast<int>(r * std::sin(theta));

//     p.setPen(QPen(COL_SWEEP, 2));
//     p.drawLine(cx, cy, ex, ey);

//     p.setPen(QPen(COL_SWEEP_CONE, 1));
//     for (float side : { -halfBW, +halfBW }) {
//         double t2 = qDegreesToRadians(
//             static_cast<double>(90.0f - currentAz + side));
//         int bx = cx + static_cast<int>(r * std::cos(t2));
//         int by = cy - static_cast<int>(r * std::sin(t2));
//         p.drawLine(cx, cy, bx, by);
//     }

//     p.setFont(QFont("Courier", 7));
//     p.setPen(COL_SWEEP);
//     p.drawText(ex + 3, ey - 3,
//                QString("%1°").arg(currentAz, 0, 'f', 1));
// #else
//     Q_UNUSED(p); Q_UNUSED(cx); Q_UNUSED(cy); Q_UNUSED(radius);
// #endif
// }

// void RadarDisplay::drawSurfaceTargets(QPainter& p, int cx, int cy, int radius)
// {
//     const QVector<Target>& list = (entity && sensor) ? sensor->targets : targets;
//     if (list.isEmpty()) return;

//     float displayRange = sensor ? sensor->range : 100.0f;
//     if (displayRange <= 0) return;

//     float pixPerKm = static_cast<float>(radius) / displayRange;

//     screenTargets.clear();

//     for (const Target& tgt : list) {
//         if (tgt.radius > displayRange) continue;

//         QPointF pos = polarToScreen(tgt.radius, tgt.angle,
//                                     cx, cy, displayRange, pixPerKm);
//         int tx = static_cast<int>(pos.x());
//         int ty = static_cast<int>(pos.y());

//         uint32_t tid = 0;
//         if (tgt.entity)
//             tid = static_cast<uint32_t>(std::hash<std::string>{}(tgt.entity->ID));

//         bool isLocked = (tid != 0 && tid == lockedTargetID);
//         if (isLocked) lockedTargetPos = pos;

//         screenTargets.append({ pos, tid, tgt });

//         QColor col = isLocked ? COL_LOCKED_COL : COL_SURFACE;
//         drawSquare(p, tx, ty, SYMBOL_SIZE, col);
//         drawLabel (p, tx, ty, tgt, isLocked);
//     }
// }

// // ===========================================================================
// // HUD
// // ===========================================================================
// void RadarDisplay::drawHUD(QPainter& p)
// {
//     if (!sensor) return;

//     QString modeStr;
//     switch (sensor->mode) {
//     case Sensor::Mode::Search:         modeStr = "SURV"; break;
//     case Sensor::Mode::TrackWhileScan: modeStr = "TWS";  break;
//     case Sensor::Mode::FireControl:    modeStr = "LOCK"; break;
//     case Sensor::Mode::Track:          modeStr = "TRK";  break;
//     default:                           modeStr = "----"; break;
//     }

//     QString dispStr = (displayMode == DisplayMode::AIR) ? "AIR" : "SURF";

//     p.setFont(QFont("Courier", 8));
//     p.setPen(COL_HUD);

//     int top = MODEBAR_H + 4;
//     p.drawText(4, top + 14, QString("RNG  %1km").arg(static_cast<int>(sensor->range)));
//     p.drawText(4, top + 26, QString("AZ   %1°") .arg(sensor->azimuth, 0, 'f', 1));
//     p.drawText(4, top + 38, QString("BW   %1°") .arg(sensor->beamWidth, 0, 'f', 1));
//     p.drawText(4, top + 50, QString("MODE %1")  .arg(modeStr));
//     p.drawText(4, top + 62, QString("DISP %1")  .arg(dispStr));

//     // Lock status
//     if (lockedTargetID != 0) {
//         p.setPen(COL_LOCKED_COL);
//         p.setFont(QFont("Courier", 8, QFont::Bold));
//         p.drawText(4, top + 76, QString("LOCKED"));
//     }

//     // Track count
//     p.setFont(QFont("Courier", 8));
//     p.setPen(COL_HUD);
//     p.drawText(4, height() - MARGIN - 8,
//                QString("TRK  %1").arg(sensor->targets.size()));

//     // Platform name — top right
//     if (entity) {
//         QString pname = QString::fromStdString(entity->Name);
//         int tw = p.fontMetrics().horizontalAdvance(pname);
//         p.drawText(width() - MARGIN - tw - 4, MODEBAR_H + 14, pname);
//     }

//     // Legend — bottom right
//     int lx = width()  - MARGIN - 90;
//     int ly = height() - MARGIN - 52;
//     p.setFont(QFont("Courier", 7));
//     p.setPen(COL_HUD_DIM);
//     p.drawText(lx, ly, "LEGEND");

//     p.setPen(QPen(COL_AIR, 1.5));
//     p.setBrush(Qt::NoBrush);
//     p.drawEllipse(QPoint(lx + 6, ly + 12), 4, 4);
//     p.setPen(COL_HUD_DIM);
//     p.drawText(lx + 14, ly + 16, "AIR");

//     {
//         QPolygon tri;
//         tri << QPoint(lx + 6,  ly + 22)
//             << QPoint(lx + 2,  ly + 30)
//             << QPoint(lx + 10, ly + 30);
//         p.setPen(QPen(COL_ENEMY, 1.5));
//         p.setBrush(Qt::NoBrush);
//         p.drawPolygon(tri);
//     }
//     p.setPen(COL_HUD_DIM);
//     p.drawText(lx + 14, ly + 30, "ENEMY");

//     p.setPen(QPen(COL_SURFACE, 1.5));
//     p.setBrush(Qt::NoBrush);
//     p.drawRect(lx + 2, ly + 34, 8, 8);
//     p.setPen(COL_HUD_DIM);
//     p.drawText(lx + 14, ly + 43, "SURFACE");
// }

// // ===========================================================================
// // paintEvent
// // ===========================================================================
// void RadarDisplay::paintEvent(QPaintEvent* event)
// {
//     QPainter painter(this);
//     painter.setRenderHint(QPainter::Antialiasing);

//     drawBackground(painter);

//     if (displayMode == DisplayMode::AIR) {
//         int cx = width()  / 2;
//         int cy = height() - MARGIN;

//         drawAirRings    (painter, cx, cy);
//         drawAirSector   (painter, cx, cy);
//         drawAirSweepLine(painter, cx, cy);
//         drawCenterMark  (painter, cx, cy);
//         drawAirTargets  (painter, cx, cy);

//     } else {
//         int cx     = width()  / 2;
//         int cy     = MODEBAR_H + (height() - MODEBAR_H - MARGIN) / 2;
//         int radius = std::min(width() - 2 * MARGIN,
//                               height() - MODEBAR_H - MARGIN) / 2;

//         drawSurfaceRings  (painter, cx, cy, radius);
//         drawSurfaceSweep  (painter, cx, cy, radius);
//         drawCenterMark    (painter, cx, cy);
//         drawSurfaceTargets(painter, cx, cy, radius);
//     }

//     drawLockReticle(painter);
//     drawHUD        (painter);

//     QWidget::paintEvent(event);
// }
// #include "radardisplay.h"                          // For radar display class
// #include "core/Hierarchy/Utils/entityutils.h"
// #include "qmath.h"
// #include <cmath>                                   // For math functions
// #include <QDebug>                                  // For debug output
// #include <QJsonParseError>                         // For JSON parsing errors
// #include <QHBoxLayout>                             // For horizontal layout
// #include <QPainter>                                // For painting operations
// #include <core/Debug/console.h>                    // For console error logging

// // Define aspect ratio constant
// constexpr double ASPECT_RATIO = 16.0 / 9.0;

// // %%% Size Management %%%
// /* Calculate height based on width and aspect ratio */
// int RadarDisplay::heightForWidth(int width) const
// {
//     return qRound(width * ASPECT_RATIO);
// }

// /* Provide size hint for widget */
// QSize RadarDisplay::sizeHint() const
// {
//     int defaultWidth = 400;
//     return QSize(defaultWidth, heightForWidth(defaultWidth));
// }

// /* Provide minimum size for widget */
// QSize RadarDisplay::minimumSize() const
// {
//     int minWidth = 90;
//     return QSize(minWidth, heightForWidth(minWidth));
// }

// // %%% Constructor %%%
// /* Initialize radar display widget */
// RadarDisplay::RadarDisplay(QWidget *parent)
//     : QWidget(parent)
// {
//     // Set background color
//     setStyleSheet("background-color: black;");
//     // Set size policy with aspect ratio
//     QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//     setWindowTitle("Radar Display");
//     policy.setHeightForWidth(true);
//     setSizePolicy(policy);
//     // Initialize test targets
//     for (int i = 1; i < 10; i++) {
//         Target target;
//         target.angle = 9 * i;
//         target.radius = 10 * i;
//         targets.append(target);
//     }
// }

// // %%% Entity Management %%%
// /* Select and configure entity for display */
// void RadarDisplay::selectEntity(Entity* entit)
// {
//     // Cast entity to Platform
//     Platform* platform = dynamic_cast<Platform*>(entit);
//     if (!platform) {
//         Console::error("Entity is not a Platform");
//         update();
//         return;
//     }
//     // Set entity ID and pointer
//     id = QString::fromStdString(platform->ID);
//     entity = platform;
//     sensor = nullptr;
//     // Select first valid sensor
//     for (auto const& pair :  *entity->sensors->sensors) {
//         Sensor* s = pair.second;
//         if (s) {
//             if(s->subType == Sensor::SubType::Generic){
//                 sensor = s;
//                 // Set window title with platform name
//                 setWindowTitle("Radar Display (" + QString::fromStdString(entity->Name) + ")");
//                 break;
//             }
//         }
//     }
//     update();
// }

// /* Remove entity if ID matches */
// void RadarDisplay::RemoveEntity(QString ID)
// {
//     if (id == ID) {
//         // Clear entity and sensor
//         entity = nullptr;
//         sensor = nullptr;
//         // Reset window title
//         setWindowTitle("Radar Display");
//     }
// }

// // %%% Update Methods %%%
// /* Update radar display data */
// void RadarDisplay::updateRadar()
// {
//     if (entity && sensor) {
//         // Set radar range and azimuth
//         setRange(sensor->range);
//         setAzimuth(sensor->azimuth);
//         // Trigger repaint
//         update();
//     }
// }

// /* Update display from JSON data */
// void RadarDisplay::updateFromJson(const QJsonObject &json)
// {
//     // Update range
//     if (json.contains("range") && json["range"].isDouble()) {
//         range = json["range"].toInt();
//     }
//     // Update azimuth
//     if (json.contains("azimuth") && json["azimuth"].isDouble()) {
//         azimuth = json["azimuth"].toDouble();
//     }
//     // Update bar
//     if (json.contains("bar") && json["bar"].isDouble()) {
//         bar = json["bar"].toDouble();
//     }
//     // Update current speed
//     if (json.contains("current_speed") && json["current_speed"].isDouble()) {
//         current_speed = json["current_speed"].toInt();
//     }
//     // Update max speed
//     if (json.contains("max_speed") && json["max_speed"].isDouble()) {
//         max_speed = json["max_speed"].toInt();
//     }
//     // Update radar height
//     if (json.contains("height") && json["height"].isDouble()) {
//         radar_height = json["height"].toInt();
//     }
//     // Update targets
//     if (json.contains("targets") && json["targets"].isArray()) {
//         targets.clear();
//         QJsonArray targetArray = json["targets"].toArray();
//         for (const QJsonValue &value : targetArray) {
//             QJsonObject targetObj = value.toObject();
//             if (targetObj.contains("angle") && targetObj.contains("radius")) {
//                 Target target;
//                 target.angle = targetObj["angle"].toDouble();
//                 target.radius = targetObj["radius"].toDouble();
//                 targets.append(target);
//             }
//         }
//     }
//     // Trigger repaint
//     update();
// }

// /* Draw display background */
// void RadarDisplay::drawBackground(QPainter &painter)
// {
//     painter.setBrush(Qt::black);
//     // Fill background
//     painter.drawRect(rect());
//     painter.setPen(QPen(Qt::green, 1));
//     painter.setBrush(Qt::black);
//     // Draw border
//     painter.drawRect(30, 30, QWidget::width() - 60, QWidget::height() - 60);
// }

// /* Set azimuth value */
// void RadarDisplay::setAzimuth(float value)
// {
//     azimuth = value;
// }

// /* Set range value */
// void RadarDisplay::setRange(float value)
// {
//     range = value;
// }

// /* Draw concentric radar circles */
// void RadarDisplay::drawConcentricCircles(QPainter &painter, int centerX, int centerY, int radius)
// {
//     painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
//     painter.setBrush(Qt::transparent);
//     int panelhigh = QWidget::height() - 60;
//     int offset = 10;
//     float num = (range < offset ? offset : range) / 10;
//     // Draw circles based on range
//     for (float i = 1; i <= num; i++) {
//         float per = (i * offset) / range;
//         float radiu = panelhigh * per;
//         painter.drawEllipse(centerX - radiu, centerY - radiu, radiu * 2, radiu * 2);
//     }
// }

// /* Draw vertical center line */
// void RadarDisplay::drawVerticalLine(QPainter &painter, int centerX, int centerY, int radius)
// {
//     painter.setPen(QPen(Qt::green, 1, Qt::DashLine));
//     painter.drawLine(centerX, centerY, centerX, 30);
// }

// /* Draw center square */
// void RadarDisplay::drawCenterSquare(QPainter &painter, int centerX, int centerY)
// {
//     painter.setBrush(Qt::white);
//     painter.drawRect(centerX - 5, centerY - 5, 10, 10);
// }

// /* Draw radar annotations */
// void RadarDisplay::drawAnnotations(QPainter &painter, int centerX, int widgetHeight)
// {
//     Q_UNUSED(centerX);
//     Q_UNUSED(widgetHeight);
//     painter.setPen(QPen(Qt::green, 1));
//     painter.setFont(QFont("Arial", 9));
//     // Draw mode annotations
//     int width = QWidget::width();
//     int spacing = width / 6;
//     painter.drawText(spacing * 1 - 20, 15, "CRM");
//     painter.drawText(spacing * 2 - 20, 15, "ACM");
//     painter.drawText(spacing * 3 - 20, 15, "TWS");
//     painter.drawText(spacing * 4 - 20, 15, "ATA");
//     painter.drawText(spacing * 5 - 20, 15, "AAST");
//     // Draw range
//     painter.drawText(5, 30, QString("%1").arg(range));
//     // Draw azimuth
//     painter.drawText(5, 45, QString("%1 A").arg(azimuth));
// }

// /* Draw azimuth lines */
// void RadarDisplay::drawAzimuth(QPainter &painter, int centerX, int centerY, int radius)
// {
//     radius *= 2;
//     painter.setPen(QPen(Qt::blue, 2));
//     double azimuthRad = 90 * M_PI / 180;
//     double halfBeamWidth = azimuth / 2.0;
//     double leftAngle = azimuthRad - (halfBeamWidth * M_PI / 180);
//     double rightAngle = azimuthRad + (halfBeamWidth * M_PI / 180);
//     int leftEndX = centerX + static_cast<int>(radius * cos(leftAngle));
//     int leftEndY = centerY - static_cast<int>(radius * sin(leftAngle));
//     int rightEndX = centerX + static_cast<int>(radius * cos(rightAngle));
//     int rightEndY = centerY - static_cast<int>(radius * sin(rightAngle));
//     painter.drawLine(centerX, centerY, leftEndX, leftEndY);
//     painter.drawLine(centerX, centerY, rightEndX, rightEndY);
// }

// /* Draw targets and their paths */
// void RadarDisplay::drawTargetAndPath(QPainter &painter, int centerX, int centerY)
// {
//     if (entity && sensor) {
//         painter.setBrush(Qt::red);
//         for (const Target &target : sensor->targets) {
//             int panelhigh = QWidget::height() - 60;
//             float per = target.radius / range;
//             float radius = panelhigh * per;
//             float angle = target.angle+90;
//            // qDebug()<<target.angle;
//             if (std::abs(target.angle) > (azimuth / 2)) continue;
//             double theta = qDegreesToRadians(angle);
//             double targetRadius = radius;
//             int targetX = centerX - static_cast<int>(targetRadius * cos(theta));
//             int targetY = centerY - static_cast<int>(targetRadius * sin(theta));
//             // Draw target point
//             painter.drawEllipse(targetX - 3, targetY - 3, 6, 6);

//             if(target.speed>50){
//                 QPolygon triangle;
//                 int size = 10;

//                 triangle << QPoint(targetX, targetY - size)
//                      << QPoint(targetX - size/2, targetY + size/2)
//                      << QPoint(targetX + size/2, targetY + size/2);
//                 painter.setBrush(Qt::yellow);
//                 painter.drawPolygon(triangle);
//             }
//             // Draw path to target
//             painter.setPen(QPen(Qt::cyan, 1, Qt::DotLine));
//             QPointF pathPoints[] = {
//                 QPointF(centerX, centerY),
//                 QPointF(centerX, centerY - static_cast<int>(0.4 * radius)),
//                 QPointF(targetX, targetY)
//             };
//             painter.drawPolyline(pathPoints, 3);
//             // Draw target labels
//             painter.setPen(QPen(Qt::green, 1));
//             painter.drawText(targetX - 20, targetY - 10, QString("%1").arg(angle));
//             painter.drawText(targetX - 20, targetY + 5, QString("%1").arg(radius));
//         }
//     }
// }

// // %%% Paint Event %%%
// /* Handle painting of radar display */
// void RadarDisplay::paintEvent(QPaintEvent *event)
// {
//     QPainter painter(this);
//     //painter.setRenderHint(QPainter::Antialiasing);
//     int centerX = width() / 2;
//     int centerY = QWidget::height() - 30;
//     int radius = (QWidget::height() - 100) / 2;
//     // Draw display components
//     drawBackground(painter);
//     drawConcentricCircles(painter, centerX, centerY, radius);
//     drawVerticalLine(painter, centerX, centerY, radius);
//     drawAnnotations(painter, centerX, QWidget::height());
//     drawAzimuth(painter, centerX, centerY, width() * 2);
//     drawCenterSquare(painter, centerX, centerY);
//     drawTargetAndPath(painter, centerX, centerY);
//     QWidget::paintEvent(event);
// }
