
/* ========================================================================= */
/* File: aesaradardisplay.h                                                  */
/* Purpose: AESA radar display — mirrors RadarDisplay structure exactly,     */
/*          adds IFF colouring, DRFM warnings, duty cycle, track quality.   */
/* ========================================================================= */
#ifndef AESARADARDISPLAY_H
#define AESARADARDISPLAY_H

#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

#include <QWidget>
#include <QPainter>
#include <QJsonObject>
#include <QVector>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>

class AESARadarDisplay : public QWidget
{
    Q_OBJECT

public:
    enum class DisplayMode { AIR, SURFACE };

    explicit AESARadarDisplay(QWidget* parent = nullptr);

    void setHierarchy(Hierarchy* h) { hierarchy = h; }

    void selectEntity(Entity* entity);
    void RemoveEntity(QString ID);
    void updateRadar();
    void setDisplayMode(DisplayMode m);

    QSize sizeHint()            const override;
    QSize minimumSize()         const;
    int   heightForWidth(int w) const override;

    Sensor*   sensor = nullptr;
    Platform* entity = nullptr;

protected:
    void paintEvent     (QPaintEvent*  event) override;
    void resizeEvent    (QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent*  event) override;

private slots:
    void toggleDisplayMode();
    void onModeButtonClicked(int modeIndex);

private:
    // ── Mode buttons ────────────────────────────────────────────────────────
    QPushButton* btnSurv     = nullptr;
    QPushButton* btnTWS      = nullptr;
    QPushButton* btnLock     = nullptr;
    QPushButton* btnDispMode = nullptr;

    // ── Display state ────────────────────────────────────────────────────────
    DisplayMode displayMode = DisplayMode::AIR;
    float scanMinAz = -60.0f;
    float scanMaxAz =  60.0f;

    // ── Lock state ───────────────────────────────────────────────────────────
    uint32_t lockedTargetID  = 0;
    QPointF  lockedTargetPos = {};

    // ── AESA status (populated from signals each frame) ──────────────────────
    float    currentDutyCycle_   = 0.0f;
    QString  lastDRFMWarning_    = "";
    uint32_t lastDRFMTargetID_   = 0;
    int      drfmWarnFrames_     = 0;      // countdown for warning display

    struct IFFEntry {
        int      responseCode = 0;   // maps to aesa::IFFResponseCode
        uint32_t squawk       = 0;
        float    confidence   = 0.0f;
    };
    QMap<uint32_t, IFFEntry> iffMap_;      // trackID → latest IFF result

    struct ScreenTarget {
        QPointF  pos;
        uint32_t id;
        Target   data;
        int      iffCode    = 0;
        float    iffConf    = 0.0f;
        bool     isDRFM     = false;
        double   trackQual  = 0.0;
    };
    QVector<ScreenTarget> screenTargets;

    QString    id        = "";
    Hierarchy* hierarchy = nullptr;
    QVector<Target> targets;

    // ── Helpers ──────────────────────────────────────────────────────────────
    AESARadar* asAESA() const {
        return sensor ? dynamic_cast<AESARadar*>(sensor) : nullptr;
    }

    void applyAESAMode(Sensor::Mode m);
    void updateModeButtonStyles();
    void repositionButtons();

    // ── Drawing ──────────────────────────────────────────────────────────────
    void drawBackground  (QPainter& p);
    void drawCenterMark  (QPainter& p, int cx, int cy);
    void drawHUD         (QPainter& p);
    void drawLockReticle (QPainter& p);
    void drawDRFMWarning (QPainter& p);
    void drawDutyCycleBar(QPainter& p);

    void drawAirRings    (QPainter& p, int cx, int cy);
    void drawAirSector   (QPainter& p, int cx, int cy);
    void drawAirTargets  (QPainter& p, int cx, int cy);

    void drawSurfaceRings  (QPainter& p, int cx, int cy, int radius);
    void drawSurfaceTargets(QPainter& p, int cx, int cy, int radius);

    void drawCircle  (QPainter& p, int x, int y, int sz, QColor col);
    void drawSquare  (QPainter& p, int x, int y, int sz, QColor col);
    void drawLabel   (QPainter& p, int x, int y,
                   const ScreenTarget& st, bool isLocked);
    QColor iffColour(int responseCode) const;
};

#endif // AESARADARDISPLAY_H
