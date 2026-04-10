/* ========================================================================= */
/* File: aesaradardisplay.cpp                                                */
/* Purpose: AESA radar display                                               */
/* ========================================================================= */

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

// ── Layout (identical to RadarDisplay) ──────────────────────────────────────
constexpr double AESA_ASPECT   = 16.0 / 9.0;
constexpr int    AESA_MARGIN   = 30;
constexpr int    AESA_MODEBAR  = 28;
constexpr int    AESA_SYM      = 7;
constexpr int    AESA_HIT      = 12;

constexpr float  KNOTS_MS      = 1.94384f;

// ── Colours — same green palette as RadarDisplay ────────────────────────────
static const QColor C_BG         {   0,   0,   0 };
static const QColor C_BORDER     {   0, 180,   0 };
static const QColor C_RING       {   0,  70,   0 };
static const QColor C_RING_BRT   {   0, 130,   0 };
static const QColor C_RING_LBL   {   0, 170,   0 };
static const QColor C_SECTOR     {   0, 120,   0 };
static const QColor C_HUD        {   0, 210,  70 };
static const QColor C_HUD_DIM    {   0, 100,  35 };
static const QColor C_CENTER     { 200, 200, 200 };

// Target colours
static const QColor C_AIR        {   0, 220,  80 };
static const QColor C_SURFACE    {  80, 180, 255 };
static const QColor C_LOCKED     { 255,  60,  60 };
static const QColor C_LABEL      { 160, 240, 160 };
static const QColor C_DRFM       { 255, 180,   0 };   // amber — ghost warning

// IFF colours
static const QColor C_FRIENDLY   {   0, 200, 255 };   // cyan
static const QColor C_HOSTILE    { 255,  40,  40 };   // red
static const QColor C_UNKNOWN    { 255, 200,   0 };   // yellow

// Duty cycle bar
static const QColor C_DUTY_LOW   {   0, 200,  80 };
static const QColor C_DUTY_MED   { 255, 200,   0 };
static const QColor C_DUTY_HIGH  { 255,  60,  40 };

// ── Button styles (identical to RadarDisplay) ────────────────────────────────
static const QString S_ACTIVE =
    "QPushButton { background:#003d18; color:#00ff50; border:1px solid #00cc44; "
    "font:bold 8px 'Courier'; padding:2px 8px; }";
static const QString S_IDLE =
    "QPushButton { background:#000a05; color:#007730; border:1px solid #004420; "
    "font:bold 8px 'Courier'; padding:2px 8px; }";
static const QString S_LOCK_ON =
    "QPushButton { background:#3d0000; color:#ff4040; border:1px solid #cc0000; "
    "font:bold 8px 'Courier'; padding:2px 8px; }";
static const QString S_DISP =
    "QPushButton { background:#001a10; color:#00bb44; border:1px solid #005522; "
    "font:bold 8px 'Courier'; padding:2px 6px; }"
    "QPushButton:hover { background:#002a18; }";

// ── Coordinate helper ────────────────────────────────────────────────────────
static QPointF polarToScreen(float rangeKm, float bearingDeg,
                             int cx, int cy, float dispRangeKm, float pxKm)
{
    float  pix   = rangeKm * pxKm;
    double theta = qDegreesToRadians(static_cast<double>(90.0f - bearingDeg));
    return { cx + pix * std::cos(theta), cy - pix * std::sin(theta) };
}

static float normBearing(float b)
{
    while (b >  180.0f) b -= 360.0f;
    while (b < -180.0f) b += 360.0f;
    return b;
}

// ============================================================================
// Size
// ============================================================================
int   AESARadarDisplay::heightForWidth(int w) const { return qRound(w * AESA_ASPECT); }
QSize AESARadarDisplay::sizeHint()      const       { return { 520, heightForWidth(520) }; }
QSize AESARadarDisplay::minimumSize()   const       { return { 200, heightForWidth(200) }; }

// ============================================================================
// Constructor
// ============================================================================
AESARadarDisplay::AESARadarDisplay(QWidget* parent) : QWidget(parent)
{
    setStyleSheet("background-color: black;");
    setWindowTitle("AESA Radar Display");
    setMouseTracking(true);

    QSizePolicy pol(QSizePolicy::Preferred, QSizePolicy::Preferred);
    pol.setHeightForWidth(true);
    setSizePolicy(pol);

    btnSurv     = new QPushButton("SURV", this);
    btnTWS      = new QPushButton("TWS",  this);
    btnLock     = new QPushButton("LOCK", this);
    btnDispMode = new QPushButton("AIR",  this);
    btnDispMode->setStyleSheet(S_DISP);

    connect(btnSurv,     &QPushButton::clicked, this, [this]{ onModeButtonClicked(0); });
    connect(btnTWS,      &QPushButton::clicked, this, [this]{ onModeButtonClicked(1); });
    connect(btnLock,     &QPushButton::clicked, this, [this]{ onModeButtonClicked(2); });
    connect(btnDispMode, &QPushButton::clicked, this, &AESARadarDisplay::toggleDisplayMode);

    updateModeButtonStyles();
}

// ============================================================================
// Button layout
// ============================================================================
void AESARadarDisplay::repositionButtons()
{
    int w  = width();
    int bh = AESA_MODEBAR - 4;
    int bw = (w - 2 * AESA_MARGIN) / 4;

    btnSurv    ->setGeometry(AESA_MARGIN,              4, bw, bh);
    btnTWS     ->setGeometry(AESA_MARGIN + bw,         4, bw, bh);
    btnLock    ->setGeometry(AESA_MARGIN + bw * 2,     4, bw, bh);
    btnDispMode->setGeometry(AESA_MARGIN + bw * 3,     4, bw, bh);
}

void AESARadarDisplay::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    repositionButtons();
}

// ============================================================================
// Mode button styles
// ============================================================================
void AESARadarDisplay::updateModeButtonStyles()
{
    if (!sensor) {
        btnSurv->setStyleSheet(S_IDLE);
        btnTWS ->setStyleSheet(S_IDLE);
        btnLock->setStyleSheet(S_IDLE);
        return;
    }
    btnSurv->setStyleSheet(sensor->mode == Sensor::Mode::Search         ? S_ACTIVE  : S_IDLE);
    btnTWS ->setStyleSheet(sensor->mode == Sensor::Mode::TrackWhileScan ? S_ACTIVE  : S_IDLE);
    btnLock->setStyleSheet(sensor->mode == Sensor::Mode::FireControl    ? S_LOCK_ON : S_IDLE);
}

// ============================================================================
// Mode button click
// ============================================================================
void AESARadarDisplay::onModeButtonClicked(int idx)
{
    AESARadar* aesa = asAESA();
    if (!aesa) return;

    switch (idx)
    {
    case 0:
        if (lockedTargetID) aesa->breakLock();
        lockedTargetID = 0;
        applyAESAMode(Sensor::Mode::Search);
        break;
    case 1:
        if (lockedTargetID) aesa->breakLock();
        lockedTargetID = 0;
        applyAESAMode(Sensor::Mode::TrackWhileScan);
        break;
    case 2:
        if (lockedTargetID == 0) {
            btnLock->setText("LOCK?");
            QTimer::singleShot(800, this, [this]{
                btnLock->setText("LOCK");
                updateModeButtonStyles();
            });
            return;
        }
        aesa->lockOn(lockedTargetID);
        applyAESAMode(Sensor::Mode::FireControl);
        break;
    }
    updateModeButtonStyles();
    update();
}

void AESARadarDisplay::applyAESAMode(Sensor::Mode m)
{
    AESARadar* aesa = asAESA();
    if (!aesa) return;

    aesa::RadarConfig cfg = aesa->getRadarConfig();
    switch (m) {
    case Sensor::Mode::Search:         cfg.mode = aesa::RadarMode::SURVEILLANCE; break;
    case Sensor::Mode::TrackWhileScan: cfg.mode = aesa::RadarMode::TWS;          break;
    case Sensor::Mode::FireControl:    cfg.mode = aesa::RadarMode::LOCK_ON;      break;
    default: break;
    }
    aesa->setRadarConfig(cfg);
    if (sensor) sensor->mode = m;
    aesa->markDisplayRangeDirty();
}

// ============================================================================
// Display mode toggle
// ============================================================================
void AESARadarDisplay::toggleDisplayMode()
{
    displayMode = (displayMode == DisplayMode::AIR) ? DisplayMode::SURFACE : DisplayMode::AIR;
    btnDispMode->setText(displayMode == DisplayMode::AIR ? "AIR" : "SURF");
    update();
}

void AESARadarDisplay::setDisplayMode(DisplayMode m)
{
    displayMode = m;
    btnDispMode->setText(displayMode == DisplayMode::AIR ? "AIR" : "SURF");
    update();
}

// ============================================================================
// Mouse click — lock-on target selection
// ============================================================================
void AESARadarDisplay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    QPointF click = event->pos();

    float best = AESA_HIT * AESA_HIT;
    int   idx  = -1;
    for (int i = 0; i < screenTargets.size(); ++i) {
        float dx = static_cast<float>(click.x() - screenTargets[i].pos.x());
        float dy = static_cast<float>(click.y() - screenTargets[i].pos.y());
        float d2 = dx*dx + dy*dy;
        if (d2 < best) { best = d2; idx = i; }
    }

    if (idx < 0) {
        AESARadar* aesa = asAESA();
        if (sensor && sensor->mode == Sensor::Mode::FireControl && aesa) aesa->breakLock();
        applyAESAMode(Sensor::Mode::Search);
        lockedTargetID = 0; lockedTargetPos = {};
        updateModeButtonStyles(); update(); return;
    }

    const ScreenTarget& st = screenTargets[idx];
    lockedTargetID  = st.id;

    lockedTargetPos = st.pos;

    AESARadar* aesa = asAESA();
    if (aesa) aesa->lockOn(lockedTargetID);
    applyAESAMode(Sensor::Mode::FireControl);
    updateModeButtonStyles(); update();
}

// ============================================================================
// Entity management
// ============================================================================
void AESARadarDisplay::selectEntity(Entity* ent)
{
    Platform* platform = dynamic_cast<Platform*>(ent);
    if (!platform) { update(); return; }

    id = QString::fromStdString(platform->ID);
    entity = platform;
    sensor = nullptr;
    lockedTargetID = 0;
    iffMap_.clear();

    for (auto const& pair : *entity->sensors->sensors) {
        Sensor* s = pair.second;
        //if (s && s->subType == Sensor::SubType::Generic) {
        if (s && s->subType == Sensor::SubType::AESA) {

            sensor = s;
            AESARadar* aesa = dynamic_cast<AESARadar*>(s);
            if (aesa) {
                aesa::RadarConfig cfg = aesa->getRadarConfig();
                scanMinAz = cfg.minAzimuth;
                scanMaxAz = cfg.maxAzimuth;

                // Connect AESA-specific signals
                connect(aesa, &AESARadar::iffResult, this,
                        [this](uint32_t tid, int code, uint32_t sq, float conf) {
                            iffMap_[tid] = { code, sq, conf };
                        }, Qt::UniqueConnection);

                connect(aesa, &AESARadar::drfmGhostDetected, this,
                        [this](uint32_t tid, float, float, float) {
                            lastDRFMTargetID_ = tid;
                            lastDRFMWarning_  = QString("DRFM GHOST  ID:%1").arg(tid);
                            drfmWarnFrames_   = 60;   // show for ~60 frames
                        }, Qt::UniqueConnection);

                connect(aesa, &AESARadar::schedulerDutyCycle, this,
                        [this](float dc) { currentDutyCycle_ = dc; },
                        Qt::UniqueConnection);
            } else {
                scanMinAz = -sensor->maxDetectionAngle;
                scanMaxAz =  sensor->maxDetectionAngle;
            }
            setDisplayMode(scanMaxAz >= 180.0f ? DisplayMode::SURFACE : DisplayMode::AIR);
            setWindowTitle("AESA Radar — " + QString::fromStdString(entity->Name));
            break;
        }
    }
    updateModeButtonStyles();
    update();
}

void AESARadarDisplay::updateRadar()
{
    if (!entity || !sensor) return;
    AESARadar* aesa = dynamic_cast<AESARadar*>(sensor);
    if (aesa) {
        aesa::RadarConfig cfg = aesa->getRadarConfig();
        scanMinAz = cfg.minAzimuth;
        scanMaxAz = cfg.maxAzimuth;
    }
    if (drfmWarnFrames_ > 0) --drfmWarnFrames_;
    updateModeButtonStyles();
    update();
}

void AESARadarDisplay::RemoveEntity(QString eid)
{
    if (id == eid) {
        entity = nullptr; sensor = nullptr;
        targets.clear(); screenTargets.clear();
        lockedTargetID = 0; iffMap_.clear();
        setWindowTitle("AESA Radar Display");
        updateModeButtonStyles(); update();
    }
}

// ============================================================================
// IFF colour helper
// ============================================================================
QColor AESARadarDisplay::iffColour(int code) const
{
    // 0=NO_REPLY  1=FRIENDLY  2=UNKNOWN  3=HOSTILE  4=CORRUPTED
    switch (code) {
    case 1:  return C_FRIENDLY;
    case 3:  return C_HOSTILE;
    case 2:  return C_UNKNOWN;
    default: return C_AIR;
    }
}

// ============================================================================
// Symbols
// ============================================================================
void AESARadarDisplay::drawCircle(QPainter& p, int x, int y, int sz, QColor col)
{
    p.setPen(QPen(col, 1.5)); p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(x, y), sz, sz);
    p.setBrush(col); p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(x, y), 2.0, 2.0);
}

void AESARadarDisplay::drawSquare(QPainter& p, int x, int y, int sz, QColor col)
{
    p.setPen(QPen(col, 1.5));
    p.setBrush(QColor(col.red(), col.green(), col.blue(), 40));
    p.drawRect(x - sz, y - sz, sz * 2, sz * 2);
}

// ============================================================================
// Label — adds IFF badge and track quality
// ============================================================================
void AESARadarDisplay::drawLabel(QPainter& p, int x, int y,
                                 const ScreenTarget& st, bool isLocked)
{
    const Target& t = st.data;
    p.setFont(QFont("Courier", 7));
    p.setPen(isLocked ? C_LOCKED : C_LABEL);

    QString spd = QString("S:%1kt").arg(static_cast<int>(t.speed * KNOTS_MS));
    QString alt = QString("A:%1m") .arg(static_cast<int>(t.altitude));
    QString hdg = QString("H:%1°") .arg(static_cast<int>(t.direction), 3, 10, QChar('0'));
    QString rv  = QString("RV:%1%2")
                     .arg(t.radialVelocity >= 0 ? "+" : "")
                     .arg(t.radialVelocity, 0, 'f', 1);

    // IFF tag
    QString iffTag;
    if (st.iffCode == 1) iffTag = " [FRD]";
    else if (st.iffCode == 3) iffTag = " [HOS]";
    else if (st.iffCode == 2) iffTag = " [UNK]";

    // DRFM tag
    QString drfmTag = st.isDRFM ? " [GHOST]" : "";

    // Track quality bar (5 chars)
    int qBars = static_cast<int>(st.trackQual * 5.0);
    QString qStr = "|" + QString(qBars, '#') + QString(5 - qBars, '.') + "|";

    int lx = x + AESA_SYM + 4;
    int ly = y - 18;

    // IFF line — coloured
    if (!iffTag.isEmpty()) {
        p.setPen(iffColour(st.iffCode));
        p.drawText(lx, ly, iffTag + drfmTag);
        ly += 11;
    }

    p.setPen(isLocked ? C_LOCKED : C_LABEL);
    p.drawText(lx, ly,      spd + "  " + alt);
    p.drawText(lx, ly + 11, hdg + "  " + rv);
    p.drawText(lx, ly + 22, qStr);

    // Heading vector
    if (t.speed > 0.5f) {
        double theta = qDegreesToRadians(static_cast<double>(90.0f - t.direction));
        float  vlen  = 14.0f + std::min(t.speed / 30.0f, 1.0f) * 14.0f;
        int    vx    = x + static_cast<int>(vlen * std::cos(theta));
        int    vy    = y - static_cast<int>(vlen * std::sin(theta));
        p.setPen(QPen(isLocked ? C_LOCKED : C_LABEL, 1, Qt::DotLine));
        p.drawLine(x, y, vx, vy);
    }
}

// ============================================================================
// Lock reticle — identical to RadarDisplay
// ============================================================================
void AESARadarDisplay::drawLockReticle(QPainter& p)
{
    if (lockedTargetID == 0 || lockedTargetPos.isNull()) return;
    int x = static_cast<int>(lockedTargetPos.x());
    int y = static_cast<int>(lockedTargetPos.y());
    int r = 14, b = 5;
    p.setPen(QPen(C_LOCKED, 1.5)); p.setBrush(Qt::NoBrush);
    p.drawLine(x-r, y-r, x-r+b, y-r); p.drawLine(x-r, y-r, x-r, y-r+b);
    p.drawLine(x+r, y-r, x+r-b, y-r); p.drawLine(x+r, y-r, x+r, y-r+b);
    p.drawLine(x-r, y+r, x-r+b, y+r); p.drawLine(x-r, y+r, x-r, y+r-b);
    p.drawLine(x+r, y+r, x+r-b, y+r); p.drawLine(x+r, y+r, x+r, y+r-b);
    p.setPen(QPen(C_LOCKED, 1, Qt::DotLine));
    p.drawLine(x-r+b, y, x+r-b, y);
    p.drawLine(x, y-r+b, x, y+r-b);
}

// ============================================================================
// DRFM warning banner
// ============================================================================
void AESARadarDisplay::drawDRFMWarning(QPainter& p)
{
    if (drfmWarnFrames_ <= 0) return;
    int alpha = std::min(255, drfmWarnFrames_ * 5);
    p.setFont(QFont("Courier", 9, QFont::Bold));
    p.setPen(QColor(255, 180, 0, alpha));
    p.drawText(AESA_MARGIN + 4, height() - AESA_MARGIN - 22, lastDRFMWarning_);
}

// ============================================================================
// Duty cycle bar — bottom-right corner
// ============================================================================
void AESARadarDisplay::drawDutyCycleBar(QPainter& p)
{
    int barW = 60, barH = 8;
    int bx   = width()  - AESA_MARGIN - barW - 4;
    int by   = height() - AESA_MARGIN - barH - 4;

    // Background
    p.setPen(QPen(C_HUD_DIM, 1));
    p.setBrush(QColor(0, 20, 10));
    p.drawRect(bx, by, barW, barH);

    // Fill
    float clamp = std::clamp(currentDutyCycle_, 0.0f, 1.0f);
    QColor fill = clamp < 0.6f ? C_DUTY_LOW : (clamp < 0.85f ? C_DUTY_MED : C_DUTY_HIGH);
    p.setBrush(fill); p.setPen(Qt::NoPen);
    p.drawRect(bx, by, static_cast<int>(barW * clamp), barH);

    // Label
    p.setFont(QFont("Courier", 7));
    p.setPen(C_HUD_DIM);
    p.drawText(bx, by - 2, QString("DUTY %1%").arg(static_cast<int>(clamp * 100)));
}

// ============================================================================
// Background
// ============================================================================
void AESARadarDisplay::drawBackground(QPainter& p)
{
    p.setBrush(C_BG); p.setPen(Qt::NoPen); p.drawRect(rect());
    p.setBrush(QColor(0, 15, 5));
    p.setPen(QPen(C_BORDER, 1));
    p.drawRect(AESA_MARGIN, 0, width() - 2*AESA_MARGIN, AESA_MODEBAR);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(C_BORDER, 1));
    p.drawRect(AESA_MARGIN, AESA_MODEBAR,
               width()  - 2*AESA_MARGIN,
               height() - AESA_MODEBAR - AESA_MARGIN);
}

void AESARadarDisplay::drawCenterMark(QPainter& p, int cx, int cy)
{
    p.setBrush(C_CENTER); p.setPen(Qt::NoPen);
    p.drawRect(cx - 3, cy - 3, 6, 6);
}

// ============================================================================
// AIR rings + sector — identical to RadarDisplay
// ============================================================================
void AESARadarDisplay::drawAirRings(QPainter& p, int cx, int cy)
{
    float dr = sensor ? sensor->range : 100.0f;
    if (dr <= 0) return;
    int   ph   = height() - AESA_MODEBAR - AESA_MARGIN;
    float step = dr <= 30 ? 5.0f : dr <= 100 ? 10.0f : 20.0f;
    int   n    = static_cast<int>(std::ceil(dr / step));
    p.setFont(QFont("Courier", 7));
    for (int i = 1; i <= n; ++i) {
        float rKm = step * i;
        float px  = ph * (rKm / dr);
        bool  last = (i == n);
        p.setPen(QPen(last ? C_RING_BRT : C_RING, 1, last ? Qt::SolidLine : Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(cx, cy), static_cast<qreal>(px), static_cast<qreal>(px));
        p.setPen(C_RING_LBL);
        p.drawText(cx+3, static_cast<int>(cy-px)+9, QString("%1km").arg(static_cast<int>(rKm)));
    }
}

void AESARadarDisplay::drawAirSector(QPainter& p, int cx, int cy)
{
    int   ph = height() - AESA_MODEBAR - AESA_MARGIN;
    float r  = static_cast<float>(ph) * 1.02f;
    p.setFont(QFont("Courier", 7));
    for (float az : { scanMinAz, scanMaxAz }) {
        double theta = qDegreesToRadians(static_cast<double>(90.0f - az));
        int ex = cx + static_cast<int>(r * std::cos(theta));
        int ey = cy - static_cast<int>(r * std::sin(theta));
        p.setPen(QPen(C_SECTOR, 1, Qt::DashLine));
        p.drawLine(cx, cy, ex, ey);
        p.setPen(C_RING_LBL);
        p.drawText(ex-14, ey-3, QString("%1°").arg(static_cast<int>(az)));
    }
}

// ============================================================================
// AIR targets — adds IFF colour and DRFM marker
// ============================================================================
void AESARadarDisplay::drawAirTargets(QPainter& p, int cx, int cy)
{
    const QVector<Target>& list = (entity && sensor) ? sensor->targets : targets;
    if (list.isEmpty()) return;
    float dr = sensor ? sensor->range : 100.0f;
    if (dr <= 0) return;

    int   ph  = height() - AESA_MODEBAR - AESA_MARGIN;
    float ppk = static_cast<float>(ph) / dr;
    screenTargets.clear();

    for (const Target& tgt : list) {
        if (tgt.radius > dr) continue;

        // uint32_t tid = 0;
        // if (tgt.entity)
        //     tid = static_cast<uint32_t>(std::hash<std::string>{}(tgt.entity->ID));
        // FIXED — same FNV-1a hash as platformToRadarID() in aesaradar.cpp:
        uint32_t tid = 0;
        if (tgt.entity)
        {
            const std::string& key = tgt.entity->ID;
            uint32_t hash = 2166136261u;
            for (unsigned char c : key) { hash ^= c; hash *= 16777619u; }
            tid = (hash == 0) ? 1u : hash;
        }
        bool isLocked = (tid != 0 && tid == lockedTargetID);

        // if (!isLocked) {
        //     float bearing = normBearing(tgt.angle);
        //     if (bearing < scanMinAz || bearing > scanMaxAz) continue;
        // }
        // TWS tracks can coast outside scan sector — always show validated tracks
        // Only filter raw surveillance detections, not maintained Kalman tracks
        if (!isLocked && sensor->mode == Sensor::Mode::Search) {
            float bearing = normBearing(tgt.angle);
            if (bearing < scanMinAz || bearing > scanMaxAz) continue;
        }

        QPointF pos = polarToScreen(tgt.radius, tgt.angle, cx, cy, dr, ppk);
        int tx = static_cast<int>(pos.x());
        int ty = static_cast<int>(pos.y());

        if (isLocked) lockedTargetPos = pos;

        // Resolve IFF and track quality
        IFFEntry iff = iffMap_.value(tid, {0, 0, 0.0f});

        ScreenTarget st;
        st.pos       = pos;
        st.id        = tid;
        st.data      = tgt;
        st.iffCode   = iff.responseCode;
        st.iffConf   = iff.confidence;
        st.isDRFM    = (tid != 0 && tid == lastDRFMTargetID_ && drfmWarnFrames_ > 0);
        st.trackQual = 0.0;   // TrackOutput quality not in Target — shown via colour
        screenTargets.append(st);

        QColor col = st.isDRFM   ? C_DRFM
                     : isLocked    ? C_LOCKED
                     : iff.responseCode > 0 ? iffColour(iff.responseCode)
                                            : C_AIR;

        drawCircle(p, tx, ty, AESA_SYM, col);
        drawLabel (p, tx, ty, st, isLocked);
    }
}

// ============================================================================
// SURFACE rings + targets
// ============================================================================
void AESARadarDisplay::drawSurfaceRings(QPainter& p, int cx, int cy, int radius)
{
    float dr = sensor ? sensor->range : 100.0f;
    if (dr <= 0) return;
    float step = dr <= 30 ? 5.0f : dr <= 100 ? 10.0f : 20.0f;
    int   n    = static_cast<int>(std::ceil(static_cast<double>(dr) / step));
    p.setFont(QFont("Courier", 7));
    for (int i = 1; i <= n; ++i) {
        float rKm = step * i;
        float px  = radius * (rKm / dr);
        bool  last = (i == n);
        p.setPen(QPen(last ? C_RING_BRT : C_RING, 1, last ? Qt::SolidLine : Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(cx, cy), static_cast<qreal>(px), static_cast<qreal>(px));
        p.setPen(C_RING_LBL);
        p.drawText(cx+3, static_cast<int>(cy-px)+9, QString("%1km").arg(static_cast<int>(rKm)));
    }
    float outerR = static_cast<float>(radius);
    p.setFont(QFont("Courier", 8, QFont::Bold)); p.setPen(C_RING_LBL);
    p.drawText(cx-4, static_cast<int>(cy-outerR)-4, "N");
    p.drawText(cx-4, static_cast<int>(cy+outerR)+12, "S");
    p.drawText(static_cast<int>(cx+outerR)+4, cy+4, "E");
    p.drawText(static_cast<int>(cx-outerR)-12, cy+4, "W");
}

void AESARadarDisplay::drawSurfaceTargets(QPainter& p, int cx, int cy, int radius)
{
    const QVector<Target>& list = (entity && sensor) ? sensor->targets : targets;
    if (list.isEmpty()) return;
    float dr = sensor ? sensor->range : 100.0f;
    if (dr <= 0) return;
    float ppk = static_cast<float>(radius) / dr;
    screenTargets.clear();

    for (const Target& tgt : list) {
        if (tgt.radius > dr) continue;
        QPointF pos = polarToScreen(tgt.radius, tgt.angle, cx, cy, dr, ppk);
        int tx = static_cast<int>(pos.x());
        int ty = static_cast<int>(pos.y());

        uint32_t tid = 0;
        if (tgt.entity)
            tid = static_cast<uint32_t>(std::hash<std::string>{}(tgt.entity->ID));

        bool isLocked = (tid != 0 && tid == lockedTargetID);
        if (isLocked) lockedTargetPos = pos;

        IFFEntry iff = iffMap_.value(tid, {0, 0, 0.0f});
        ScreenTarget st;
        st.pos = pos; st.id = tid; st.data = tgt;
        st.iffCode = iff.responseCode; st.iffConf = iff.confidence;
        st.isDRFM = false; st.trackQual = 0.0;
        screenTargets.append(st);

        QColor col = isLocked ? C_LOCKED
                     : iff.responseCode > 0 ? iffColour(iff.responseCode)
                                            : C_SURFACE;

        drawSquare(p, tx, ty, AESA_SYM, col);
        drawLabel (p, tx, ty, st, isLocked);
    }
}

// ============================================================================
// HUD — adds duty cycle, IFF legend, DRFM status
// ============================================================================
void AESARadarDisplay::drawHUD(QPainter& p)
{
    if (!sensor) return;

    QString modeStr;
    switch (sensor->mode) {
    case Sensor::Mode::Search:         modeStr = "SURV"; break;
    case Sensor::Mode::TrackWhileScan: modeStr = "TWS";  break;
    case Sensor::Mode::FireControl:    modeStr = "LOCK"; break;
    default:                           modeStr = "----"; break;
    }

    p.setFont(QFont("Courier", 8));
    p.setPen(C_HUD);
    int top = AESA_MODEBAR + 4;
    p.drawText(4, top+14, QString("RNG  %1km") .arg(static_cast<int>(sensor->range)));
    p.drawText(4, top+26, QString("AZ   %1°")  .arg(sensor->azimuth,   0, 'f', 1));
    p.drawText(4, top+38, QString("BW   %1°")  .arg(sensor->beamWidth, 0, 'f', 1));
    p.drawText(4, top+50, QString("MODE %1")   .arg(modeStr));
    p.drawText(4, top+62, QString("DUTY %1%")  .arg(static_cast<int>(currentDutyCycle_ * 100)));

    if (lockedTargetID != 0) {
        p.setPen(C_LOCKED);
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(4, top+76, "LOCKED");

        // IFF of locked target
        if (iffMap_.contains(lockedTargetID)) {
            const IFFEntry& e = iffMap_[lockedTargetID];
            QColor ic = iffColour(e.responseCode);
            QString iffStr;
            switch (e.responseCode) {
            case 1: iffStr = "IFF:FRD"; break;
            case 3: iffStr = "IFF:HOS"; break;
            case 2: iffStr = "IFF:UNK"; break;
            default: iffStr = "IFF:---"; break;
            }
            p.setPen(ic);
            p.drawText(4, top+90, iffStr + QString(" SQ:%1").arg(e.squawk));
        }
    }

    p.setFont(QFont("Courier", 8));
    p.setPen(C_HUD);
    p.drawText(4, height()-AESA_MARGIN-8, QString("TRK  %1").arg(sensor->targets.size()));

    if (entity) {
        QString pname = QString::fromStdString(entity->Name);
        int tw = p.fontMetrics().horizontalAdvance(pname);
        p.drawText(width()-AESA_MARGIN-tw-4, AESA_MODEBAR+14, pname);
    }

    // ── IFF colour legend ────────────────────────────────────────────────────
    int lx = width()  - AESA_MARGIN - 90;
    int ly = height() - AESA_MARGIN - 80;
    p.setFont(QFont("Courier", 7)); p.setPen(C_HUD_DIM);
    p.drawText(lx, ly, "IFF");
    struct { QColor col; const char* lbl; } legend[] = {
        { C_FRIENDLY, "FRD" }, { C_HOSTILE, "HOS" },
        { C_UNKNOWN,  "UNK" }, { C_AIR,     "N/A" }
    };
    for (int i = 0; i < 4; ++i) {
        p.setPen(QPen(legend[i].col, 1.5)); p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPoint(lx+6, ly+12+i*13), 4, 4);
        p.setPen(C_HUD_DIM);
        p.drawText(lx+14, ly+16+i*13, legend[i].lbl);
    }
}

// ============================================================================
// paintEvent
// ============================================================================
void AESARadarDisplay::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);

    if (displayMode == DisplayMode::AIR) {
        int cx = width()  / 2;
        int cy = height() - AESA_MARGIN;
        drawAirRings  (painter, cx, cy);
        drawAirSector (painter, cx, cy);
        drawCenterMark(painter, cx, cy);
        drawAirTargets(painter, cx, cy);
    } else {
        int cx     = width()  / 2;
        int cy     = AESA_MODEBAR + (height() - AESA_MODEBAR - AESA_MARGIN) / 2;
        int radius = std::min(width()-2*AESA_MARGIN, height()-AESA_MODEBAR-AESA_MARGIN) / 2;
        drawSurfaceRings  (painter, cx, cy, radius);
        drawCenterMark    (painter, cx, cy);
        drawSurfaceTargets(painter, cx, cy, radius);
    }

    drawLockReticle (painter);
    drawDutyCycleBar(painter);
    drawDRFMWarning (painter);
    drawHUD         (painter);

    QWidget::paintEvent(event);
}
