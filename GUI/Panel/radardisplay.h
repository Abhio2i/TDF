/* ========================================================================= */
/* File: radardisplay.h                                                      */
/* Purpose: Radar display — mode selector, click-to-lock, air/surface view   */
/* ========================================================================= */
#ifndef RADARDISPLAY_H
#define RADARDISPLAY_H

#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/hierarchy.h"
// radar.h now exposes RadarConfig (was RadarAttributes) — include for
// lockOn() / breakLock() / getRadarConfig() / setRadarConfig()
#include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"

#include <QWidget>
#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QPushButton>
#include <QMouseEvent>

// ---------------------------------------------------------------------------
// RadarDisplay
//
// Operator interface for a single radar sensor.
// Mode selector bar at top: SURV | TWS | LOCK  (click to switch)
// Click on a target dot in display to lock onto it.
// AIR/SURFACE view toggle button (top-right corner).
//
// API alignment note (radarmodel refactor):
//   getRadarConfig() now returns RadarConfig (was RadarAttributes).
//   All field names that this display uses are identical in RadarConfig.
//   No functional change — only the struct type name changed.
// ---------------------------------------------------------------------------
class RadarDisplay : public QWidget
{
    Q_OBJECT

public:
    enum class DisplayMode { AIR, SURFACE };

    explicit RadarDisplay(QWidget* parent = nullptr);

    void setHierarchy(Hierarchy* h) { hierarchy = h; }

    // Entity binding
    void selectEntity(Entity* entity);
    void RemoveEntity(QString ID);

    // Called each sim tick
    void updateRadar();

    // JSON update (network / replay path)
    void updateFromJson(const QJsonObject& json);

    // Set display mode externally
    void setDisplayMode(DisplayMode m);

    // Size hints
    QSize sizeHint()            const override;
    QSize minimumSize()         const;
    int   heightForWidth(int w) const override;

    // Public sensor/entity pointers (read by the engine)
    Sensor*   sensor = nullptr;
    Platform* entity = nullptr;

protected:
    void paintEvent     (QPaintEvent*  event) override;
    void resizeEvent    (QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent*  event) override;

private slots:
    void toggleDisplayMode();
    void onModeButtonClicked(int modeIndex);   // 0=SURV  1=TWS  2=LOCK

private:
    // -----------------------------------------------------------------------
    // Radar mode buttons (top bar)
    // -----------------------------------------------------------------------
    QPushButton* btnSurv     = nullptr;
    QPushButton* btnTWS      = nullptr;
    QPushButton* btnLock     = nullptr;
    QPushButton* btnDispMode = nullptr;   // AIR / SURF toggle

    // -----------------------------------------------------------------------
    // Display state
    // -----------------------------------------------------------------------
    DisplayMode displayMode = DisplayMode::AIR;

    float scanMinAz = -60.0f;
    float scanMaxAz =  60.0f;

    // Lock-on state
    uint32_t lockedTargetID  = 0;
    QPointF  lockedTargetPos = {};   // screen position of locked target

    // Per-frame screen positions of targets (for click hit-testing)
    struct ScreenTarget {
        QPointF  pos;
        uint32_t id;
        Target   data;
    };
    QVector<ScreenTarget> screenTargets;

    QString    id        = "";
    Hierarchy* hierarchy = nullptr;

    // Fallback targets (JSON / offline path)
    QVector<Target> targets;

    // Platform motion telemetry (populated via updateFromJson)
    int current_speed = 0;
    int max_speed     = 0;
    int radar_height  = 0;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    // Safe cast of sensor → Radar* (returns nullptr if not a Radar)
    Radar* asRadar() const { return sensor ? dynamic_cast<Radar*>(sensor) : nullptr; }

    // Apply a Sensor::Mode to both the Radar model and the sensor base class.
    // Internally reads/writes RadarConfig (not RadarAttributes).
    void applyRadarMode(Sensor::Mode m);

    void updateModeButtonStyles();
    void repositionButtons();

    // -----------------------------------------------------------------------
    // Drawing — shared
    // -----------------------------------------------------------------------
    void drawBackground (QPainter& p);
    void drawCenterMark (QPainter& p, int cx, int cy);
    void drawHUD        (QPainter& p);
    void drawLockReticle(QPainter& p);

    // -----------------------------------------------------------------------
    // Drawing — AIR mode
    // -----------------------------------------------------------------------
    void drawAirRings    (QPainter& p, int cx, int cy);
    void drawAirSector   (QPainter& p, int cx, int cy);
    void drawAirSweepLine(QPainter& p, int cx, int cy);
    void drawAirTargets  (QPainter& p, int cx, int cy);

    // -----------------------------------------------------------------------
    // Drawing — SURFACE mode
    // -----------------------------------------------------------------------
    void drawSurfaceRings  (QPainter& p, int cx, int cy, int radius);
    void drawSurfaceSweep  (QPainter& p, int cx, int cy, int radius);
    void drawSurfaceTargets(QPainter& p, int cx, int cy, int radius);

    // -----------------------------------------------------------------------
    // Symbols
    // -----------------------------------------------------------------------
    void drawCircle  (QPainter& p, int x, int y, int sz, QColor col);
    void drawTriangle(QPainter& p, int x, int y, int sz, QColor col);
    void drawSquare  (QPainter& p, int x, int y, int sz, QColor col);
    void drawLabel   (QPainter& p, int x, int y, const Target& t, bool isLocked);
};

#endif // RADARDISPLAY_H

// /* ========================================================================= */
// /* File: radardisplay.h                                                      */
// /* Purpose: Radar display — mode selector, click-to-lock, air/surface view   */
// /* ========================================================================= */
// #ifndef RADARDISPLAY_H
// #define RADARDISPLAY_H

// #include "core/Hierarchy/EntityProfiles/sensor.h"
// #include "core/Hierarchy/hierarchy.h"
// #include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"          // needed for lockOn() / breakLock() / setRadarConfig()

// #include <QWidget>
// #include <QPainter>
// #include <QJsonObject>
// #include <QJsonArray>
// #include <QVector>
// #include <QPushButton>
// #include <QMouseEvent>

// // ---------------------------------------------------------------------------
// // RadarDisplay
// //
// // Operator interface for a single radar sensor.
// // Mode selector bar at top: SURV | TWS | LOCK  (click to switch)
// // Click on a target dot in display to lock onto it.
// // AIR/SURFACE view toggle button (top-right corner).
// // ---------------------------------------------------------------------------
// class RadarDisplay : public QWidget
// {
//     Q_OBJECT

// public:
//     enum class DisplayMode { AIR, SURFACE };

//     explicit RadarDisplay(QWidget* parent = nullptr);

//     void setHierarchy(Hierarchy* h) { hierarchy = h; }

//     // Entity binding
//     void selectEntity(Entity* entity);
//     void RemoveEntity(QString ID);

//     // Called each sim tick
//     void updateRadar();

//     // JSON update (network / replay path)
//     void updateFromJson(const QJsonObject& json);

//     // Set display mode externally
//     void setDisplayMode(DisplayMode m);

//     // Size hints
//     QSize sizeHint()            const override;
//     QSize minimumSize()         const;
//     int   heightForWidth(int w) const override;

//     // Public
//     Sensor*   sensor = nullptr;
//     Platform* entity = nullptr;

// protected:
//     void paintEvent  (QPaintEvent*  event) override;
//     void resizeEvent (QResizeEvent* event) override;
//     void mousePressEvent(QMouseEvent* event) override;

// private slots:
//     void toggleDisplayMode();
//     void onModeButtonClicked(int modeIndex);   // 0=SURV 1=TWS 2=LOCK

// private:
//     // -----------------------------------------------------------------------
//     // Radar mode buttons (top bar)
//     // -----------------------------------------------------------------------
//     QPushButton* btnSurv = nullptr;
//     QPushButton* btnTWS  = nullptr;
//     QPushButton* btnLock = nullptr;

//     // AIR/SURFACE toggle
//     QPushButton* btnDispMode = nullptr;

//     // -----------------------------------------------------------------------
//     // State
//     // -----------------------------------------------------------------------
//     DisplayMode displayMode = DisplayMode::AIR;

//     float  scanMinAz = -60.0f;
//     float  scanMaxAz =  60.0f;

//     // Lock-on state
//     uint32_t  lockedTargetID  = 0;
//     QPointF   lockedTargetPos = {};   // screen position of locked target

//     // Per-frame screen positions of targets (for click hit-testing)
//     struct ScreenTarget {
//         QPointF   pos;
//         uint32_t  id;
//         Target    data;
//     };
//     QVector<ScreenTarget> screenTargets;

//     QString    id        = "";
//     Hierarchy* hierarchy = nullptr;

//     // Fallback targets (JSON / offline)
//     QVector<Target> targets;

//     // Platform motion for HUD
//     int current_speed = 0;
//     int max_speed     = 0;
//     int radar_height  = 0;

//     // -----------------------------------------------------------------------
//     // Helpers
//     // -----------------------------------------------------------------------
//     Radar* asRadar() const { return sensor ? dynamic_cast<Radar*>(sensor) : nullptr; }
//     void   applyRadarMode(Sensor::Mode m);
//     void   updateModeButtonStyles();
//     void   repositionButtons();

//     // -----------------------------------------------------------------------
//     // Drawing — shared
//     // -----------------------------------------------------------------------
//     void drawBackground (QPainter& p);
//     void drawCenterMark (QPainter& p, int cx, int cy);
//     void drawHUD        (QPainter& p);
//     void drawLockReticle(QPainter& p);

//     // -----------------------------------------------------------------------
//     // Drawing — AIR mode
//     // -----------------------------------------------------------------------
//     void drawAirRings      (QPainter& p, int cx, int cy);
//     void drawAirSector     (QPainter& p, int cx, int cy);
//     void drawAirSweepLine  (QPainter& p, int cx, int cy);
//     void drawAirTargets    (QPainter& p, int cx, int cy);

//     // -----------------------------------------------------------------------
//     // Drawing — SURFACE mode
//     // -----------------------------------------------------------------------
//     void drawSurfaceRings  (QPainter& p, int cx, int cy, int radius);
//     void drawSurfaceSweep  (QPainter& p, int cx, int cy, int radius);
//     void drawSurfaceTargets(QPainter& p, int cx, int cy, int radius);

//     // -----------------------------------------------------------------------
//     // Symbols
//     // -----------------------------------------------------------------------
//     void drawCircle  (QPainter& p, int x, int y, int sz, QColor col);
//     void drawTriangle(QPainter& p, int x, int y, int sz, QColor col);
//     void drawSquare  (QPainter& p, int x, int y, int sz, QColor col);
//     void drawLabel   (QPainter& p, int x, int y, const Target& t, bool isLocked);
// };

// #endif

/* ========================================================================= */
/* File: radardisplay.h                                                     */
/* Purpose: Defines widget for radar display visualization                   */
//               Written by Arti Rajpoot
/* ========================================================================= */
/*
#ifndef RADARDISPLAY_H
#define RADARDISPLAY_H

#include "core/Hierarchy/EntityProfiles/sensor.h"  // For sensor profile
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include <QWidget>                                // For widget base class
#include <QPainter>                               // For painting operations
#include <QJsonDocument>                          // For JSON document handling
#include <QJsonObject>                            // For JSON object handling
#include <QJsonArray>                             // For JSON array handling
#include <QVector>                                // For vector container

// %%% Class Definition %%%
/* Widget for radar display visualization */
// class RadarDisplay : public QWidget
// {
//     Q_OBJECT

// public:
//     // Initialize radar display
//     explicit RadarDisplay(QWidget *parent = nullptr);
//     // Set hierarchy instance
//     void setHierarchy(Hierarchy* h) { hierarchy = h; }
//     // Update display from JSON
//     void updateFromJson(const QJsonObject &json);
//     // Get size hint
//     QSize sizeHint() const override;
//     // Get minimum size
//     QSize minimumSize() const;
//     // Get height for width
//     int heightForWidth(int width) const override;
//     // Set azimuth angle
//     void setAzimuth(float value);
//     // Set radar range
//     void setRange(float value);
//     // Select entity
//     void selectEntity(Entity* entity);
//     // Remove entity by ID
//     void RemoveEntity(QString ID);
//     // Update radar display
//     void updateRadar();
//     // Sensor instance
//     Sensor* sensor = nullptr;
//     // Entity platform
//     Platform* entity = nullptr;

// protected:
//     // Handle paint events
//     void paintEvent(QPaintEvent *event) override;

// private:
//     // %%% Display Properties %%%
//     // Radar range
//     int range = 100;
//     // Azimuth angle in degrees
//     double azimuth = 30.0;
//     // Beamwidth in degrees
//     double bar = 3.0;
//     // Current speed
//     int current_speed = 0;
//     // Maximum speed
//     int max_speed = 0;
//     // Radar height
//     int radar_height = 0;
//     // Target structure (commented)
//     // struct Target {
//     //     double angle;
//     //     double radius;
//     // };
//     // List of targets
//     QVector<Target> targets;
//     // Entity ID
//     QString id = "";
//     // Hierarchy instance
//     Hierarchy* hierarchy = nullptr;

//     // %%% Drawing Methods %%%
//     // Draw background
//     void drawBackground(QPainter &painter);
//     // Draw concentric circles
//     void drawConcentricCircles(QPainter &painter, int centerX, int centerY, int radius);
//     // Draw vertical line
//     void drawVerticalLine(QPainter &painter, int centerX, int centerY, int radius);
//     // Draw center square
//     void drawCenterSquare(QPainter &painter, int centerX, int centerY);
//     // Draw annotations
//     void drawAnnotations(QPainter &painter, int centerX, int widgetHeight);
//     // Draw azimuth
//     void drawAzimuth(QPainter &painter, int centerX, int centerY, int radius);
//     // Draw target and path
//     void drawTargetAndPath(QPainter &painter, int centerX, int centerY);
//     // Draw info text
//     void drawInfoText(QPainter &painter, int widgetHeight);
// };

//#endif*/ // RADARDISPLAY_H
