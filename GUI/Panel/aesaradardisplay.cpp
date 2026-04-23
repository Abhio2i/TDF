// =============================================================================
// FILE:         aesaradardisplay.cpp
// MODULE:       AESA Radar Display — Qt Widget
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements AESARadarDisplay. All rendering, input handling,
//               mode button management, entity binding, and signal forwarding
//               live here. No radar physics or tracking computations reside
//               in this file — all physics are delegated to RadarModel_AESA
//               via the AESARadar bridge.
//
//               Rendering pipeline per paintEvent:
//                 AIR mode:     background → rings → sector → centre → targets
//                 SURFACE mode: background → rings → centre → targets
//                 Overlays:     lock reticle → duty cycle bar → DRFM warning
//                               → HUD
//
// REQUIREMENTS: REQ-AESA-003  Mode control
//               REQ-AESA-004  Output display
//               REQ-AESA-020  Duty cycle display
//               REQ-AESA-050  IFF colouring
//               REQ-AESA-060  DRFM warning banner
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-DISPLAY-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Basic air / surface modes.
//   Rev 2  15 Feb 2026  IFF colouring, DRFM banner, duty cycle bar added.
//                       Lock reticle and mouse-click lock-on added.
//                       Track quality bar in label added.
//   Rev 3  01 Apr 2026  TWS coasted track sector filtering fixed. FNV-1a hash
//                       for target ID matching aesaradar.cpp.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Named constants replace all magic literals.
//                       Commented-out code removed per NS-05.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#include "aesaradardisplay.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "qmath.h"
#include <cmath>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QTimer>

// =============================================================================
// NAMED LAYOUT CONSTANTS
// All geometry values are declared here to satisfy VI-08 (no magic numbers)
// and to provide a single point of change if the display layout is revised.
// Identical to the corresponding constants in RadarDisplay. REQ-AESA-004.
// =============================================================================

// Widget aspect ratio — 16:9 wide-format scope. REQ-AESA-004.
constexpr double AESA_ASPECT  = 16.0 / 9.0;

// Horizontal margin (pixels) between widget edge and display area boundary.
constexpr int    AESA_MARGIN  = 30;

// Height (pixels) of the top mode button bar. REQ-AESA-004.
constexpr int    AESA_MODEBAR = 28;

// Radius (pixels) of target circle / half-width of target square symbol.
constexpr int    AESA_SYM     = 7;

// Hit-test radius squared threshold (pixels²) for mouse click target selection.
// AESA_HIT = 12 px → AESA_HIT² = 144 px². REQ-AESA-003.
constexpr int    AESA_HIT     = 12;

// Unit conversion: metres per second → knots. Used in label speed display.
constexpr float  KNOTS_MS     = 1.94384f;

// =============================================================================
// NAMED COLOUR CONSTANTS
// Green phosphor palette identical to RadarDisplay. All colours declared here
// for single-point maintenance. REQ-AESA-004.
// =============================================================================

static const QColor C_BG        {   0,   0,   0 };   // Display background black
static const QColor C_BORDER    {   0, 180,   0 };   // Display border green
static const QColor C_RING      {   0,  70,   0 };   // Range ring dim green
static const QColor C_RING_BRT  {   0, 130,   0 };   // Outer ring bright green
static const QColor C_RING_LBL  {   0, 170,   0 };   // Ring km label green
static const QColor C_SECTOR    {   0, 120,   0 };   // Scan sector line green
static const QColor C_HUD       {   0, 210,  70 };   // HUD primary text green
static const QColor C_HUD_DIM   {   0, 100,  35 };   // HUD secondary dim green
static const QColor C_CENTER    { 200, 200, 200 };   // Origin marker grey

// Target symbol colours
static const QColor C_AIR       {   0, 220,  80 };   // Air target — bright green
static const QColor C_SURFACE   {  80, 180, 255 };   // Surface target — blue
static const QColor C_LOCKED    { 255,  60,  60 };   // Locked target — red
static const QColor C_LABEL     { 160, 240, 160 };   // Label text — light green
static const QColor C_DRFM      { 255, 180,   0 };   // DRFM ghost — amber

// IFF-specific symbol colours
static const QColor C_FRIENDLY  {   0, 200, 255 };   // IFF FRIENDLY — cyan
static const QColor C_HOSTILE   { 255,  40,  40 };   // IFF HOSTILE  — red
static const QColor C_UNKNOWN   { 255, 200,   0 };   // IFF UNKNOWN  — yellow

// Duty cycle bar fill colours — thresholds: < 60% green, < 85% amber, else red
static const QColor C_DUTY_LOW  {   0, 200,  80 };
static const QColor C_DUTY_MED  { 255, 200,   0 };
static const QColor C_DUTY_HIGH { 255,  60,  40 };

// =============================================================================
// BUTTON STYLE SHEETS
// Identical to RadarDisplay button styles. All styles defined here for
// single-point maintenance. REQ-AESA-003.
// =============================================================================

// Active mode button — bright green text on dark green background.
static const QString S_ACTIVE =
    "QPushButton { background:#003d18; color:#00ff50; border:1px solid #00cc44; "
    "font:bold 8px 'Courier'; padding:2px 8px; }";

// Idle mode button — dim green text on near-black background.
static const QString S_IDLE =
    "QPushButton { background:#000a05; color:#007730; border:1px solid #004420; "
    "font:bold 8px 'Courier'; padding:2px 8px; }";

// Lock-on active button — red text on dark red background.
static const QString S_LOCK_ON =
    "QPushButton { background:#3d0000; color:#ff4040; border:1px solid #cc0000; "
    "font:bold 8px 'Courier'; padding:2px 8px; }";

// Display mode button (AIR/SURF) — medium green with hover highlight.
static const QString S_DISP =
    "QPushButton { background:#001a10; color:#00bb44; border:1px solid #005522; "
    "font:bold 8px 'Courier'; padding:2px 6px; }"
    "QPushButton:hover { background:#002a18; }";

// =============================================================================
// FILE-SCOPE STATIC HELPER FUNCTIONS
// Pure utility functions with no access to object state.
// =============================================================================

// =============================================================================
// FUNCTION:    polarToScreen
//
// DESCRIPTION: Converts a polar (range, bearing) position to widget-space
//              Cartesian coordinates. Bearing follows the radar convention:
//              0° = up (north / boresight), increasing clockwise.
//              The transformation uses: x = cx + r*cos(90-bearing),
//              y = cy - r*sin(90-bearing). REQ-AESA-004.
//
// PARAMETERS:
//   rangeKm    [in]  Slant range to target (km).
//   bearingDeg [in]  Bearing from boresight (degrees, clockwise positive).
//   cx, cy     [in]  Display origin in widget coordinates (pixels).
//   dispRangeKm[in]  Full-scale display range (km). Unused in calculation
//                    (ppk already applied) — retained for API consistency.
//   pxKm       [in]  Pixels-per-km scale factor for the current display size.
//
// RETURNS:    QPointF screen position (pixels).
// =============================================================================
static QPointF polarToScreen(float rangeKm, float bearingDeg,
                             int cx, int cy, float dispRangeKm, float pxKm)
{
    float  pix   = rangeKm * pxKm;
    double theta = qDegreesToRadians(static_cast<double>(90.0f - bearingDeg));
    return { cx + pix * std::cos(theta), cy - pix * std::sin(theta) };
}

// =============================================================================
// FUNCTION:    normBearing
//
// DESCRIPTION: Normalises a bearing value to the range [-180, +180] degrees.
//              Used before sector gate comparisons to handle targets whose
//              azimuth may arrive slightly outside this range. REQ-AESA-004.
// =============================================================================
static float normBearing(float b)
{
    while (b >  180.0f) b -= 360.0f;
    while (b < -180.0f) b += 360.0f;
    return b;
}

// =============================================================================
// SIZE POLICY OVERRIDES
// =============================================================================

int   AESARadarDisplay::heightForWidth(int w) const { return qRound(w * AESA_ASPECT); }
QSize AESARadarDisplay::sizeHint()      const       { return { 520, heightForWidth(520) }; }
QSize AESARadarDisplay::minimumSize()   const       { return { 200, heightForWidth(200) }; }

// =============================================================================
// CONSTRUCTOR
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::AESARadarDisplay
// (Full description in header)
// =============================================================================
AESARadarDisplay::AESARadarDisplay(QWidget* parent) : QWidget(parent)
{
    setStyleSheet("background-color: black;");
    setWindowTitle("AESA Radar Display");
    setMouseTracking(true);

    // Apply heightForWidth size policy so the widget maintains the 16:9 aspect
    // ratio when resized by the Qt layout engine. REQ-AESA-004.
    QSizePolicy pol(QSizePolicy::Preferred, QSizePolicy::Preferred);
    pol.setHeightForWidth(true);
    setSizePolicy(pol);

    // Create mode buttons and sensor selector dropdown — Qt-parented, lifetime
    // tied to this widget. REQ-AESA-003, REQ-AESA-004.
    btnSurv     = new QPushButton("SURV", this);
    btnTWS      = new QPushButton("TWS",  this);
    btnLock     = new QPushButton("LOCK", this);
    btnDispMode = new QPushButton("AIR",  this);
    btnDispMode->setStyleSheet(S_DISP);

    // Connect button signals. Lambda captures ensure correct idx dispatch.
    // REQ-AESA-003.
    connect(btnSurv,     &QPushButton::clicked, this, [this]{ onModeButtonClicked(0); });
    connect(btnTWS,      &QPushButton::clicked, this, [this]{ onModeButtonClicked(1); });
    connect(btnLock,     &QPushButton::clicked, this, [this]{ onModeButtonClicked(2); });
    connect(btnDispMode, &QPushButton::clicked, this, &AESARadarDisplay::toggleDisplayMode);

    updateModeButtonStyles();

    // ADD THIS BLOCK
    // Create sensor selector dropdown — hidden by default, shown only when the
    // bound platform has two or more AESA sensors. Positioned by
    // repositionButtons() in the fifth mode bar slot. REQ-AESA-004.
    sensorDropdown = new QComboBox(this);
    sensorDropdown->setStyleSheet(
        "QComboBox { background-color: #001a00; color: #00ff00; "
        "border: 1px solid #00ff00; font-size: 10px; padding: 2px; }"
        "QComboBox QAbstractItemView { background-color: #001a00; "
        "color: #00ff00; selection-background-color: #003300; }"
        );
    sensorDropdown->hide();
    connect(sensorDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AESARadarDisplay::onSensorSelected);
}

// =============================================================================
// BUTTON LAYOUT
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::repositionButtons
// (Full description in header)
// =============================================================================
void AESARadarDisplay::repositionButtons()
{
    int w  = width();
    int bh = AESA_MODEBAR - 4;
int bw = (w - 2 * AESA_MARGIN) / 5;   // equal width: SURV TWS LOCK AIR/SURF SENSOR
    btnSurv    ->setGeometry(AESA_MARGIN,          4, bw, bh);
    btnTWS     ->setGeometry(AESA_MARGIN + bw,     4, bw, bh);
    btnLock    ->setGeometry(AESA_MARGIN + bw * 2, 4, bw, bh);
    btnDispMode->setGeometry(AESA_MARGIN + bw * 3, 4, bw, bh);

    if (sensorDropdown)
        sensorDropdown->setGeometry(AESA_MARGIN + bw * 4, 4, bw, bh);
}

void AESARadarDisplay::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    repositionButtons();
}
// =============================================================================
// MODE BUTTON STYLES
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::updateModeButtonStyles
// (Full description in header)
// =============================================================================
void AESARadarDisplay::updateModeButtonStyles()
{
    if (!sensor)
    {
        // No sensor bound — all buttons idle. REQ-AESA-003.
        btnSurv->setStyleSheet(S_IDLE);
        btnTWS ->setStyleSheet(S_IDLE);
        btnLock->setStyleSheet(S_IDLE);
        return;
    }

    // Highlight the button corresponding to the current sensor mode.
    btnSurv->setStyleSheet(sensor->mode == Sensor::Mode::Search         ? S_ACTIVE  : S_IDLE);
    btnTWS ->setStyleSheet(sensor->mode == Sensor::Mode::TrackWhileScan ? S_ACTIVE  : S_IDLE);
    btnLock->setStyleSheet(sensor->mode == Sensor::Mode::FireControl    ? S_LOCK_ON : S_IDLE);
}

// =============================================================================
// MODE BUTTON CLICK
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::onModeButtonClicked
// (Full description in header)
// =============================================================================
void AESARadarDisplay::onModeButtonClicked(int idx)
{
    AESARadar* aesa = asAESA();
    if (!aesa) return;

    switch (idx)
    {
    case 0:
        // SURV: break any active lock, return to wide-area surveillance.
        // REQ-AESA-003.
        if (lockedTargetID) aesa->breakLock();
        lockedTargetID = 0;
        applyAESAMode(Sensor::Mode::Search);
        break;

    case 1:
        // TWS: break any active lock, enter track-while-scan mode.
        // REQ-AESA-003.
        if (lockedTargetID) aesa->breakLock();
        lockedTargetID = 0;
        applyAESAMode(Sensor::Mode::TrackWhileScan);
        break;

    case 2:
        // LOCK: requires a target to be pre-selected via mouse click.
        // If none selected, flash "LOCK?" for 800 ms and abort. REQ-AESA-003.
        if (lockedTargetID == 0)
        {
            btnLock->setText("LOCK?");
            QTimer::singleShot(800, this, [this]
                               {
                                   btnLock->setText("LOCK");
                                   updateModeButtonStyles();
                               });
            return;
        }
        // Target selected — commit fire-control lock. REQ-AESA-003.
        aesa->lockOn(lockedTargetID);
        applyAESAMode(Sensor::Mode::FireControl);
        break;
    }

    updateModeButtonStyles();
    update();
}

// =============================================================================
// FUNCTION:    AESARadarDisplay::applyAESAMode
// (Full description in header)
// =============================================================================
void AESARadarDisplay::applyAESAMode(Sensor::Mode m)
{
    AESARadar* aesa = asAESA();
    if (!aesa) return;

    // Read current config, set the corresponding radar mode, write back.
    // REQ-AESA-003.
    aesa::RadarConfig cfg = aesa->getRadarConfig();
    switch (m)
    {
    case Sensor::Mode::Search:         cfg.mode = aesa::RadarMode::SURVEILLANCE; break;
    case Sensor::Mode::TrackWhileScan: cfg.mode = aesa::RadarMode::TWS;          break;
    case Sensor::Mode::FireControl:    cfg.mode = aesa::RadarMode::LOCK_ON;      break;
    default: break;
    }
    aesa->setRadarConfig(cfg);

    // Sync the Sensor base class mode so other display consumers stay consistent.
    if (sensor) sensor->mode = m;

    // Mark display range dirty to force recalculation on the next scan tick.
    aesa->markDisplayRangeDirty();
}

// =============================================================================
// DISPLAY MODE TOGGLE
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::toggleDisplayMode
// (Full description in header)
// =============================================================================
void AESARadarDisplay::toggleDisplayMode()
{
    // Cycle between AIR and SURFACE display geometries. REQ-AESA-004.
    displayMode = (displayMode == DisplayMode::AIR)
                      ? DisplayMode::SURFACE
                      : DisplayMode::AIR;
    btnDispMode->setText(displayMode == DisplayMode::AIR ? "AIR" : "SURF");
    update();
}

// =============================================================================
// FUNCTION:    AESARadarDisplay::setDisplayMode
// (Full description in header)
// =============================================================================
void AESARadarDisplay::setDisplayMode(DisplayMode m)
{
    displayMode = m;
    btnDispMode->setText(displayMode == DisplayMode::AIR ? "AIR" : "SURF");
    update();
}

// =============================================================================
// MOUSE CLICK — LOCK-ON TARGET SELECTION
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::mousePressEvent
// (Full description in header)
// =============================================================================
void AESARadarDisplay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    QPointF click = event->pos();

    // Hit-test: find the screenTarget nearest to the click within AESA_HIT px.
    float best = AESA_HIT * AESA_HIT;
    int   idx  = -1;
    for (int i = 0; i < screenTargets.size(); ++i)
    {
        float dx = static_cast<float>(click.x() - screenTargets[i].pos.x());
        float dy = static_cast<float>(click.y() - screenTargets[i].pos.y());
        float d2 = dx * dx + dy * dy;
        if (d2 < best) { best = d2; idx = i; }
    }

    if (idx < 0)
    {
        // Click on empty space — break any active lock, return to SURV.
        // REQ-AESA-003.
        AESARadar* aesa = asAESA();
        if (sensor && sensor->mode == Sensor::Mode::FireControl && aesa)
            aesa->breakLock();
        applyAESAMode(Sensor::Mode::Search);
        lockedTargetID  = 0;
        lockedTargetPos = {};
        updateModeButtonStyles();
        update();
        return;
    }

    // Target found — commit lock-on. REQ-AESA-003.
    const ScreenTarget& st = screenTargets[idx];
    lockedTargetID  = st.id;
    lockedTargetPos = st.pos;

    AESARadar* aesa = asAESA();
    if (aesa) aesa->lockOn(lockedTargetID);
    applyAESAMode(Sensor::Mode::FireControl);
    updateModeButtonStyles();
    update();
}
// =============================================================================
// FUNCTION:    AESARadarDisplay::updateDropdown
// (Full description in header)
// =============================================================================

void AESARadarDisplay::updateDropdown()
{
    if (!sensorDropdown) return;

    QSignalBlocker blocker(sensorDropdown);
    sensorDropdown->clear();

    if (sensorlist.isEmpty()) {
        sensorDropdown->hide();
        return;
    }

    for (int i = 0; i < sensorlist.size(); ++i) {
        Sensor* s = sensorlist[i];
        QString name = s ? QString::fromStdString(s->Name) : QString("AESA %1").arg(i + 1);
        if (name.trimmed().isEmpty())
            name = QString("AESA %1").arg(i + 1);
        sensorDropdown->addItem(name);
    }

    int currentIdx = sensorlist.indexOf(sensor);
    if (currentIdx >= 0)
        sensorDropdown->setCurrentIndex(currentIdx);

    // Only show dropdown if more than one sensor
    if (sensorlist.size() > 1)
        sensorDropdown->show();
    else
        sensorDropdown->hide();

    // Reposition
}
// =============================================================================
// FUNCTION:    AESARadarDisplay::onSensorSelected
// (Full description in header)
// =============================================================================

void AESARadarDisplay::onSensorSelected(int index)
{
    if (index < 0 || index >= sensorlist.size()) return;
    sensor         = sensorlist[index];
    lockedTargetID = 0;
    lockedTargetPos = {};
    iffMap_.clear();
    drfmWarnFrames_ = 0;

    AESARadar* aesa = dynamic_cast<AESARadar*>(sensor);
    if (aesa) {
        aesa::RadarConfig cfg = aesa->getRadarConfig();
        scanMinAz = cfg.minAzimuth;
        scanMaxAz = cfg.maxAzimuth;

        connect(aesa, &AESARadar::iffResult, this,
                [this](uint32_t tid, int code, uint32_t sq, float conf)
                { iffMap_[tid] = { code, sq, conf }; },
                Qt::UniqueConnection);

        connect(aesa, &AESARadar::drfmGhostDetected, this,
                [this](uint32_t tid, float, float, float)
                {
                    lastDRFMTargetID_ = tid;
                    lastDRFMWarning_  = QString("DRFM GHOST  ID:%1").arg(tid);
                    drfmWarnFrames_   = 60;
                }, Qt::UniqueConnection);

        connect(aesa, &AESARadar::schedulerDutyCycle, this,
                [this](float dc) { currentDutyCycle_ = dc; },
                Qt::UniqueConnection);

        setDisplayMode(scanMaxAz >= 180.0f ? DisplayMode::SURFACE : DisplayMode::AIR);
    }

    updateModeButtonStyles();
    update();
}
// =============================================================================
// ENTITY MANAGEMENT
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::selectEntity
// (Full description in header)
// =============================================================================
void AESARadarDisplay::selectEntity(Entity* ent)
{
    Platform* platform = dynamic_cast<Platform*>(ent);
    if (!platform) { update(); return; }

    // Bind to the new entity — clear all prior state. REQ-AESA-004.
    id             = QString::fromStdString(platform->ID);
    entity         = platform;
    sensor         = nullptr;
    lockedTargetID = 0;
    iffMap_.clear();

    // Search entity's sensor list for an AESA radar. REQ-AESA-004.

    sensorlist.clear();   // clear old list

    for (auto const& pair : *entity->sensors->sensors)
    {
        Sensor* s = pair.second;
        if (s && s->subType == Sensor::SubType::AESA)
            sensorlist.append(s);
    }

    // Pick first sensor by default
    if (!sensorlist.isEmpty())
    {
        sensor = sensorlist.first();
        AESARadar* aesa = dynamic_cast<AESARadar*>(sensor);
        if (aesa)
        {
            aesa::RadarConfig cfg = aesa->getRadarConfig();
            scanMinAz = cfg.minAzimuth;
            scanMaxAz = cfg.maxAzimuth;

            connect(aesa, &AESARadar::iffResult, this,
                    [this](uint32_t tid, int code, uint32_t sq, float conf)
                    { iffMap_[tid] = { code, sq, conf }; },
                    Qt::UniqueConnection);

            connect(aesa, &AESARadar::drfmGhostDetected, this,
                    [this](uint32_t tid, float, float, float)
                    {
                        lastDRFMTargetID_ = tid;
                        lastDRFMWarning_  = QString("DRFM GHOST  ID:%1").arg(tid);
                        drfmWarnFrames_   = 60;
                    }, Qt::UniqueConnection);

            connect(aesa, &AESARadar::schedulerDutyCycle, this,
                    [this](float dc) { currentDutyCycle_ = dc; },
                    Qt::UniqueConnection);
        }
        else
        {
            scanMinAz = -sensor->maxDetectionAngle;
            scanMaxAz =  sensor->maxDetectionAngle;
        }

        setDisplayMode(scanMaxAz >= 180.0f ? DisplayMode::SURFACE : DisplayMode::AIR);
        setWindowTitle("AESA Radar — " + QString::fromStdString(entity->Name));
    }

    updateDropdown();   // ADD THIS CALL

    updateModeButtonStyles();
    update();
}

// =============================================================================
// FUNCTION:    AESARadarDisplay::updateRadar
// (Full description in header)
// =============================================================================
void AESARadarDisplay::updateRadar()
{
    if (!entity || !sensor) return;

    // Re-read FoV limits — they may change if config is updated at runtime.
    // REQ-AESA-004.
    AESARadar* aesa = dynamic_cast<AESARadar*>(sensor);
    if (aesa)
    {
        aesa::RadarConfig cfg = aesa->getRadarConfig();
        scanMinAz = cfg.minAzimuth;
        scanMaxAz = cfg.maxAzimuth;
    }

    // Decrement DRFM warning frame countdown. REQ-AESA-060.
    if (drfmWarnFrames_ > 0) --drfmWarnFrames_;

    updateModeButtonStyles();
    update();
}

// =============================================================================
// FUNCTION:    AESARadarDisplay::RemoveEntity
// (Full description in header)
// =============================================================================
void AESARadarDisplay::RemoveEntity(QString eid)
{
    if (id != eid) return;

    // Bound entity removed from scene — clear all display state. REQ-AESA-004.
    entity  = nullptr;
    sensor  = nullptr;
    targets.clear();
    screenTargets.clear();
    lockedTargetID = 0;
    iffMap_.clear();
    sensorlist.clear();
    if (sensorDropdown) sensorDropdown->hide();
    setWindowTitle("AESA Radar Display");
    updateModeButtonStyles();
    update();
}

// =============================================================================
// IFF COLOUR HELPER
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::iffColour
// (Full description in header)
// =============================================================================
QColor AESARadarDisplay::iffColour(int code) const
{
    // Maps aesa::IFFResponseCode (as int) to display colour. REQ-AESA-050.
    // 0 = NO_REPLY, 1 = FRIENDLY, 2 = UNKNOWN, 3 = HOSTILE, 4 = CORRUPTED.
    switch (code)
    {
    case 1:  return C_FRIENDLY;   // cyan
    case 3:  return C_HOSTILE;    // red
    case 2:  return C_UNKNOWN;    // yellow
    default: return C_AIR;        // green (no reply or corrupted)
    }
}

// =============================================================================
// SYMBOL DRAWING
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawCircle
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawCircle(QPainter& p, int x, int y, int sz, QColor col)
{
    // Outer ring — unfilled circle at symbol radius. REQ-AESA-004.
    p.setPen(QPen(col, 1.5)); p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(x, y), sz, sz);

    // Inner dot — filled 2 px circle marking exact position. REQ-AESA-004.
    p.setBrush(col); p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(x, y), 2.0, 2.0);
}

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawSquare
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawSquare(QPainter& p, int x, int y, int sz, QColor col)
{
    // Semi-transparent filled square for surface target symbol. REQ-AESA-004.
    p.setPen(QPen(col, 1.5));
    p.setBrush(QColor(col.red(), col.green(), col.blue(), 40));
    p.drawRect(x - sz, y - sz, sz * 2, sz * 2);
}

// =============================================================================
// LABEL DRAWING
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawLabel
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawLabel(QPainter& p, int x, int y,
                                 const ScreenTarget& st, bool isLocked)
{
    const Target& t = st.data;
    p.setFont(QFont("Courier", 7));
    p.setPen(isLocked ? C_LOCKED : C_LABEL);

    // Format data strings for the label. Speed converted to knots. REQ-AESA-004.
    QString spd = QString("S:%1kt").arg(static_cast<int>(t.speed * KNOTS_MS));
    QString alt = QString("A:%1m") .arg(static_cast<int>(t.altitude));
    QString hdg = QString("H:%1°") .arg(static_cast<int>(t.direction), 3, 10, QChar('0'));
    QString rv  = QString("RV:%1%2")
                     .arg(t.radialVelocity >= 0 ? "+" : "")
                     .arg(t.radialVelocity, 0, 'f', 1);

    // IFF classification tag — colour-matched in the IFF-coloured line below.
    QString iffTag;
    if      (st.iffCode == 1) iffTag = " [FRD]";
    else if (st.iffCode == 3) iffTag = " [HOS]";
    else if (st.iffCode == 2) iffTag = " [UNK]";

    // DRFM ghost tag — appended to IFF line if target is flagged. REQ-AESA-060.
    QString drfmTag = st.isDRFM ? " [GHOST]" : "";

    // Track quality bar — 5-character progress indicator. REQ-AESA-004.
    int qBars = static_cast<int>(st.trackQual * 5.0);
    QString qStr = "|" + QString(qBars, '#') + QString(5 - qBars, '.') + "|";

    // Label origin: right of symbol, slightly above centre. REQ-AESA-004.
    int lx = x + AESA_SYM + 4;
    int ly = y - 18;

    // IFF line — rendered in IFF colour if an IFF code is present. REQ-AESA-050.
    if (!iffTag.isEmpty())
    {
        p.setPen(iffColour(st.iffCode));
        p.drawText(lx, ly, iffTag + drfmTag);
        ly += 11;
    }

    // Data lines — speed/altitude, heading/radial velocity, quality bar.
    p.setPen(isLocked ? C_LOCKED : C_LABEL);
    p.drawText(lx, ly,      spd + "  " + alt);
    p.drawText(lx, ly + 11, hdg + "  " + rv);
    p.drawText(lx, ly + 22, qStr);

    // Heading vector line — proportional to speed, dotted for non-locked targets.
    // REQ-AESA-004.
    if (t.speed > 0.5f)
    {
        double theta = qDegreesToRadians(static_cast<double>(90.0f - t.direction));
        float  vlen  = 14.0f + std::min(t.speed / 30.0f, 1.0f) * 14.0f;
        int    vx    = x + static_cast<int>(vlen * std::cos(theta));
        int    vy    = y - static_cast<int>(vlen * std::sin(theta));
        p.setPen(QPen(isLocked ? C_LOCKED : C_LABEL, 1, Qt::DotLine));
        p.drawLine(x, y, vx, vy);
    }
}

// =============================================================================
// LOCK RETICLE
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawLockReticle
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawLockReticle(QPainter& p)
{
    if (lockedTargetID == 0 || lockedTargetPos.isNull()) return;

    int x = static_cast<int>(lockedTargetPos.x());
    int y = static_cast<int>(lockedTargetPos.y());
    int r = 14;   // reticle corner radius (pixels)
    int b = 5;    // bracket arm length (pixels)

    p.setPen(QPen(C_LOCKED, 1.5)); p.setBrush(Qt::NoBrush);

    // Four corner brackets — each two lines (horizontal + vertical arm).
    // REQ-AESA-003.
    p.drawLine(x-r, y-r, x-r+b, y-r); p.drawLine(x-r, y-r, x-r, y-r+b);
    p.drawLine(x+r, y-r, x+r-b, y-r); p.drawLine(x+r, y-r, x+r, y-r+b);
    p.drawLine(x-r, y+r, x-r+b, y+r); p.drawLine(x-r, y+r, x-r, y+r-b);
    p.drawLine(x+r, y+r, x+r-b, y+r); p.drawLine(x+r, y+r, x+r, y+r-b);

    // Dotted crosshair lines within the reticle. REQ-AESA-003.
    p.setPen(QPen(C_LOCKED, 1, Qt::DotLine));
    p.drawLine(x-r+b, y, x+r-b, y);
    p.drawLine(x, y-r+b, x, y+r-b);
}

// =============================================================================
// DRFM WARNING BANNER
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawDRFMWarning
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawDRFMWarning(QPainter& p)
{
    if (drfmWarnFrames_ <= 0) return;

    // Alpha fades proportionally as the frame counter counts down to 0.
    // At 60 frames: alpha = 255. At 1 frame: alpha = 5. REQ-AESA-060.
    int alpha = std::min(255, drfmWarnFrames_ * 5);
    p.setFont(QFont("Courier", 9, QFont::Bold));
    p.setPen(QColor(255, 180, 0, alpha));
    p.drawText(AESA_MARGIN + 4, height() - AESA_MARGIN - 22, lastDRFMWarning_);
}

// =============================================================================
// DUTY CYCLE BAR
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawDutyCycleBar
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawDutyCycleBar(QPainter& p)
{
    // Bar geometry — bottom-right corner inside the display border. REQ-AESA-020.
    int barW = 60, barH = 8;
    int bx   = width()  - AESA_MARGIN - barW - 4;
    int by   = height() - AESA_MARGIN - barH - 4;

    // Background track. REQ-AESA-020.
    p.setPen(QPen(C_HUD_DIM, 1));
    p.setBrush(QColor(0, 20, 10));
    p.drawRect(bx, by, barW, barH);

    // Colour-coded fill — green < 60%, amber < 85%, red >= 85%. REQ-AESA-020.
    float clamp = std::clamp(currentDutyCycle_, 0.0f, 1.0f);
    QColor fill = clamp < 0.6f ? C_DUTY_LOW
                  : clamp < 0.85f ? C_DUTY_MED
                                  : C_DUTY_HIGH;
    p.setBrush(fill); p.setPen(Qt::NoPen);
    p.drawRect(bx, by, static_cast<int>(barW * clamp), barH);

    // "DUTY xx%" label above the bar. REQ-AESA-020.
    p.setFont(QFont("Courier", 7));
    p.setPen(C_HUD_DIM);
    p.drawText(bx, by - 2, QString("DUTY %1%").arg(static_cast<int>(clamp * 100)));
}

// =============================================================================
// BACKGROUND
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawBackground
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawBackground(QPainter& p)
{
    // Full-widget black fill. REQ-AESA-004.
    p.setBrush(C_BG); p.setPen(Qt::NoPen); p.drawRect(rect());

    // Top mode bar — dark green background with green border. REQ-AESA-004.
    p.setBrush(QColor(0, 15, 5));
    p.setPen(QPen(C_BORDER, 1));
    p.drawRect(AESA_MARGIN, 0, width() - 2 * AESA_MARGIN, AESA_MODEBAR);

    // Main display area — transparent fill (shows black BG), green border.
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(C_BORDER, 1));
    p.drawRect(AESA_MARGIN, AESA_MODEBAR,
               width()  - 2 * AESA_MARGIN,
               height() - AESA_MODEBAR - AESA_MARGIN);
}

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawCenterMark
// =============================================================================
void AESARadarDisplay::drawCenterMark(QPainter& p, int cx, int cy)
{
    // 6×6 grey filled square marks the radar platform origin. REQ-AESA-004.
    p.setBrush(C_CENTER); p.setPen(Qt::NoPen);
    p.drawRect(cx - 3, cy - 3, 6, 6);
}

// =============================================================================
// AIR MODE — RINGS AND SECTOR
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawAirRings
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawAirRings(QPainter& p, int cx, int cy)
{
    float dr = sensor ? sensor->range : 100.0f;
    if (dr <= 0) return;

    // Display pixel height available for the plot area. REQ-AESA-004.
    int   ph   = height() - AESA_MODEBAR - AESA_MARGIN;

    // Adaptive ring step: 5 km for close range, 10 km medium, 20 km long.
    float step = dr <= 30 ? 5.0f : dr <= 100 ? 10.0f : 20.0f;
    int   n    = static_cast<int>(std::ceil(dr / step));

    p.setFont(QFont("Courier", 7));
    for (int i = 1; i <= n; ++i)
    {
        float rKm = step * i;
        float px  = ph * (rKm / dr);
        bool  last = (i == n);

        // Outer ring solid, inner rings dashed. REQ-AESA-004.
        p.setPen(QPen(last ? C_RING_BRT : C_RING, 1, last ? Qt::SolidLine : Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(cx, cy), static_cast<qreal>(px), static_cast<qreal>(px));

        // Range label at top of ring. REQ-AESA-004.
        p.setPen(C_RING_LBL);
        p.drawText(cx + 3, static_cast<int>(cy - px) + 9,
                   QString("%1km").arg(static_cast<int>(rKm)));
    }
}

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawAirSector
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawAirSector(QPainter& p, int cx, int cy)
{
    // Draw scan sector boundary lines extending slightly beyond the outer ring.
    int   ph = height() - AESA_MODEBAR - AESA_MARGIN;
    float r  = static_cast<float>(ph) * 1.02f;   // slightly beyond outer ring

    p.setFont(QFont("Courier", 7));
    for (float az : { scanMinAz, scanMaxAz })
    {
        double theta = qDegreesToRadians(static_cast<double>(90.0f - az));
        int ex = cx + static_cast<int>(r * std::cos(theta));
        int ey = cy - static_cast<int>(r * std::sin(theta));
        p.setPen(QPen(C_SECTOR, 1, Qt::DashLine));
        p.drawLine(cx, cy, ex, ey);
        p.setPen(C_RING_LBL);
        p.drawText(ex - 14, ey - 3, QString("%1°").arg(static_cast<int>(az)));
    }
}

// =============================================================================
// AIR MODE — TARGETS
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawAirTargets
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawAirTargets(QPainter& p, int cx, int cy)
{
    const QVector<Target>& list = (entity && sensor) ? sensor->targets : targets;
    if (list.isEmpty()) return;

    float dr = sensor ? sensor->range : 100.0f;
    if (dr <= 0) return;

    int   ph  = height() - AESA_MODEBAR - AESA_MARGIN;
    float ppk = static_cast<float>(ph) / dr;   // pixels per km
    screenTargets.clear();

    for (const Target& tgt : list)
    {
        if (tgt.radius > dr) continue;   // beyond display range — skip

        // ---- Target ID resolution -------------------------------------------
        // Use the same FNV-1a hash as platformToRadarID() in aesaradar.cpp to
        // ensure the lockedTargetID comparison is consistent. REQ-AESA-004.
        uint32_t tid = 0;
        if (tgt.entity)
        {
            const std::string& key = tgt.entity->ID;
            uint32_t hash = 2166136261u;
            for (unsigned char c : key) { hash ^= c; hash *= 16777619u; }
            tid = (hash == 0u) ? 1u : hash;
        }

        bool isLocked = (tid != 0 && tid == lockedTargetID);

        // ---- Sector gate ---------------------------------------------------
        // TWS coasted tracks may exist outside the current scan sector —
        // show them always so the operator sees the full recognised air picture.
        // Only filter raw SURVEILLANCE detections to the current scan sector.
        // REQ-AESA-004.
        if (!isLocked && sensor->mode == Sensor::Mode::Search)
        {
            float bearing = normBearing(tgt.angle);
            if (bearing < scanMinAz || bearing > scanMaxAz) continue;
        }

        QPointF pos = polarToScreen(tgt.radius, tgt.angle, cx, cy, dr, ppk);
        int tx = static_cast<int>(pos.x());
        int ty = static_cast<int>(pos.y());

        // Update locked target screen position for drawLockReticle(). REQ-AESA-003.
        if (isLocked) lockedTargetPos = pos;

        // ---- Build ScreenTarget entry ---------------------------------------
        IFFEntry iff = iffMap_.value(tid, { 0, 0, 0.0f });

        ScreenTarget st;
        st.pos      = pos;
        st.id       = tid;
        st.data     = tgt;
        st.iffCode  = iff.responseCode;
        st.iffConf  = iff.confidence;
        // DRFM flag: true only within the active warning window. REQ-AESA-060.
        st.isDRFM   = (tid != 0 && tid == lastDRFMTargetID_ && drfmWarnFrames_ > 0);
        st.trackQual = 0.0;
        screenTargets.append(st);

        // ---- Symbol colour priority: DRFM > locked > IFF > air default -----
        QColor col = st.isDRFM          ? C_DRFM
                     : isLocked           ? C_LOCKED
                     : iff.responseCode > 0 ? iffColour(iff.responseCode)
                                            : C_AIR;

        drawCircle(p, tx, ty, AESA_SYM, col);
        drawLabel (p, tx, ty, st, isLocked);
    }
}

// =============================================================================
// SURFACE MODE — RINGS AND TARGETS
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawSurfaceRings
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawSurfaceRings(QPainter& p, int cx, int cy, int radius)
{
    float dr = sensor ? sensor->range : 100.0f;
    if (dr <= 0) return;

    float step = dr <= 30 ? 5.0f : dr <= 100 ? 10.0f : 20.0f;
    int   n    = static_cast<int>(std::ceil(static_cast<double>(dr) / step));

    p.setFont(QFont("Courier", 7));
    for (int i = 1; i <= n; ++i)
    {
        float rKm = step * i;
        float px  = radius * (rKm / dr);
        bool  last = (i == n);
        p.setPen(QPen(last ? C_RING_BRT : C_RING, 1, last ? Qt::SolidLine : Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(cx, cy), static_cast<qreal>(px), static_cast<qreal>(px));
        p.setPen(C_RING_LBL);
        p.drawText(cx + 3, static_cast<int>(cy - px) + 9,
                   QString("%1km").arg(static_cast<int>(rKm)));
    }

    // North-up cardinal labels at outer ring extremes. REQ-AESA-004.
    float outerR = static_cast<float>(radius);
    p.setFont(QFont("Courier", 8, QFont::Bold)); p.setPen(C_RING_LBL);
    p.drawText(cx - 4, static_cast<int>(cy - outerR) - 4,  "N");
    p.drawText(cx - 4, static_cast<int>(cy + outerR) + 12, "S");
    p.drawText(static_cast<int>(cx + outerR) + 4,  cy + 4, "E");
    p.drawText(static_cast<int>(cx - outerR) - 12, cy + 4, "W");
}

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawSurfaceTargets
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawSurfaceTargets(QPainter& p, int cx, int cy, int radius)
{
    const QVector<Target>& list = (entity && sensor) ? sensor->targets : targets;
    if (list.isEmpty()) return;

    float dr = sensor ? sensor->range : 100.0f;
    if (dr <= 0) return;

    float ppk = static_cast<float>(radius) / dr;
    screenTargets.clear();

    for (const Target& tgt : list)
    {
        if (tgt.radius > dr) continue;

        QPointF pos = polarToScreen(tgt.radius, tgt.angle, cx, cy, dr, ppk);
        int tx = static_cast<int>(pos.x());
        int ty = static_cast<int>(pos.y());

        // Surface mode uses std::hash — separate from the AESA FNV-1a path.
        // This is the existing surface mode behaviour, unchanged. REQ-AESA-004.
        uint32_t tid = 0;
        if (tgt.entity)
            tid = static_cast<uint32_t>(std::hash<std::string>{}(tgt.entity->ID));

        bool isLocked = (tid != 0 && tid == lockedTargetID);
        if (isLocked) lockedTargetPos = pos;

        IFFEntry iff = iffMap_.value(tid, { 0, 0, 0.0f });

        ScreenTarget st;
        st.pos      = pos; st.id  = tid; st.data = tgt;
        st.iffCode  = iff.responseCode; st.iffConf = iff.confidence;
        st.isDRFM   = false;
        st.trackQual = 0.0;
        screenTargets.append(st);

        QColor col = isLocked            ? C_LOCKED
                     : iff.responseCode > 0 ? iffColour(iff.responseCode)
                                            : C_SURFACE;

        drawSquare(p, tx, ty, AESA_SYM, col);
        drawLabel (p, tx, ty, st, isLocked);
    }
}

// =============================================================================
// HUD
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::drawHUD
// (Full description in header)
// =============================================================================
void AESARadarDisplay::drawHUD(QPainter& p)
{
    if (!sensor) return;

    // Mode string for display. REQ-AESA-003.
    QString modeStr;
    switch (sensor->mode)
    {
    case Sensor::Mode::Search:         modeStr = "SURV"; break;
    case Sensor::Mode::TrackWhileScan: modeStr = "TWS";  break;
    case Sensor::Mode::FireControl:    modeStr = "LOCK"; break;
    default:                           modeStr = "----"; break;
    }

    p.setFont(QFont("Courier", 8));
    p.setPen(C_HUD);
    int top = AESA_MODEBAR + 4;

    // Primary HUD data column — left edge. REQ-AESA-004.
    p.drawText(4, top + 14, QString("RNG  %1km").arg(static_cast<int>(sensor->range)));
    p.drawText(4, top + 26, QString("AZ   %1°") .arg(sensor->azimuth,   0, 'f', 1));
    p.drawText(4, top + 38, QString("BW   %1°") .arg(sensor->beamWidth, 0, 'f', 1));
    p.drawText(4, top + 50, QString("MODE %1")  .arg(modeStr));
    p.drawText(4, top + 62, QString("DUTY %1%") .arg(static_cast<int>(currentDutyCycle_ * 100)));

    // Lock status and IFF of locked target. REQ-AESA-003, REQ-AESA-050.
    if (lockedTargetID != 0)
    {
        p.setPen(C_LOCKED);
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(4, top + 76, "LOCKED");

        if (iffMap_.contains(lockedTargetID))
        {
            const IFFEntry& e = iffMap_[lockedTargetID];
            QColor ic = iffColour(e.responseCode);
            QString iffStr;
            switch (e.responseCode)
            {
            case 1: iffStr = "IFF:FRD"; break;
            case 3: iffStr = "IFF:HOS"; break;
            case 2: iffStr = "IFF:UNK"; break;
            default: iffStr = "IFF:---"; break;
            }
            p.setPen(ic);
            p.drawText(4, top + 90, iffStr + QString(" SQ:%1").arg(e.squawk));
        }
    }

    // Track count — bottom left. REQ-AESA-004.
    p.setFont(QFont("Courier", 8));
    p.setPen(C_HUD);
    p.drawText(4, height() - AESA_MARGIN - 8,
               QString("TRK  %1").arg(sensor->targets.size()));

    // Platform name — top right of display area. REQ-AESA-004.
    if (entity)
    {
        QString pname = QString::fromStdString(entity->Name);
        int tw = p.fontMetrics().horizontalAdvance(pname);
        p.drawText(width() - AESA_MARGIN - tw - 4, AESA_MODEBAR + 14, pname);
    }

    // ---- IFF colour legend — bottom-right -----------------------------------
    // Four small circle samples with FRD / HOS / UNK / N/A labels.
    // REQ-AESA-050.
    int lx = width()  - AESA_MARGIN - 90;
    int ly = height() - AESA_MARGIN - 80;

    p.setFont(QFont("Courier", 7)); p.setPen(C_HUD_DIM);
    p.drawText(lx, ly, "IFF");

    struct { QColor col; const char* lbl; } legend[] = {
        { C_FRIENDLY, "FRD" },
        { C_HOSTILE,  "HOS" },
        { C_UNKNOWN,  "UNK" },
        { C_AIR,      "N/A" }
    };
    for (int i = 0; i < 4; ++i)
    {
        p.setPen(QPen(legend[i].col, 1.5)); p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPoint(lx + 6, ly + 12 + i * 13), 4, 4);
        p.setPen(C_HUD_DIM);
        p.drawText(lx + 14, ly + 16 + i * 13, legend[i].lbl);
    }
}

// =============================================================================
// PAINT EVENT — MAIN RENDER DISPATCH
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadarDisplay::paintEvent
// (Full description in header)
// =============================================================================
void AESARadarDisplay::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);

    if (displayMode == DisplayMode::AIR)
    {
        // AIR mode: half-plane sweep, origin at bottom-centre. REQ-AESA-004.
        int cx = width()  / 2;
        int cy = height() - AESA_MARGIN;
        drawAirRings  (painter, cx, cy);
        drawAirSector (painter, cx, cy);
        drawCenterMark(painter, cx, cy);
        drawAirTargets(painter, cx, cy);
    }
    else
    {
        // SURFACE mode: full 360° PPI, origin at centre. REQ-AESA-004.
        int cx     = width()  / 2;
        int cy     = AESA_MODEBAR + (height() - AESA_MODEBAR - AESA_MARGIN) / 2;
        int radius = std::min(width()  - 2 * AESA_MARGIN,
                              height() - AESA_MODEBAR - AESA_MARGIN) / 2;
        drawSurfaceRings  (painter, cx, cy, radius);
        drawCenterMark    (painter, cx, cy);
        drawSurfaceTargets(painter, cx, cy, radius);
    }

    // Overlays — drawn on top of both modes. REQ-AESA-003, REQ-AESA-020, REQ-AESA-060.
    drawLockReticle (painter);
    drawDutyCycleBar(painter);
    drawDRFMWarning (painter);
    drawHUD         (painter);

    QWidget::paintEvent(event);
}


