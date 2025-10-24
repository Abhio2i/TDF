/* ========================================================================= */
/* File: ewdisplay.h                                                        */
/* Purpose: Defines widget for electronic warfare display                    */
/* ========================================================================= */

#ifndef EWDISPLAY_H
#define EWDISPLAY_H

#include "core/Hierarchy/EntityProfiles/sensor.h"  // For sensor profile
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include <QWidget>                                // For widget base class
#include <QVector>                                // For vector container

// %%% Data Structures %%%
/* Structure for electronic warfare target */
struct EWTarget {
    double angle = 0.0;                           // Target angle
    double radius = 0.0;                          // Target radius
};

// %%% Class Definition %%%
/* Widget for electronic warfare visualization */
class EWDisplay : public QWidget
{
    Q_OBJECT

public:
    // Initialize EW display
    explicit EWDisplay(QWidget *parent = nullptr);
    // Set hierarchy instance
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    // Get size hint
    QSize sizeHint() const override;
    // Get minimum size
    QSize minimumSize() const;
    // Set radar range
    void setRange(float value) { range = value; }
    // Get height for width
    int heightForWidth(int width) const override;
    // Select entity
    void selectEntity(Entity* entity);
    // Remove entity by ID
    void RemoveEntity(QString ID);
    // Update radar display
    void updateRadar();
    // Sensor instance
    Sensor* sensor = nullptr;
    // Entity platform
    Platform* entity = nullptr;

protected:
    // Handle paint events
    void paintEvent(QPaintEvent *event) override;

private:
    // %%% Display Properties %%%
    // Radar range
    int range = 100;
    // Aspect ratio for display
    const double ASPECT_RATIO = 16.0/9.0;
    // Padding for display
    int padding = 18;
    // Number of radar rings
    int ringCount = 3;
    // Major tick interval
    int majorTickEvery = 30;
    // Minor ticks per major
    int minorTicksPerMajor = 5;
    // Radar green color
    QColor radarGreen = QColor(0, 255, 0);
    // Current angle
    int ang = 0;
    // List of targets
    QVector<Target> targets;
    // Entity ID
    QString id = "";
    // Hierarchy instance
    Hierarchy* hierarchy = nullptr;

    // %%% Drawing Methods %%%
    // Draw background
    void drawBackground(QPainter &p);
    // Draw radar ring
    void drawRadarRing(QPainter &p, const QPoint &center, int outerRadius);
    // Draw ticks and labels
    void drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius);
    // Draw concentric circles
    void drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius);
    // Draw center mark
    void drawCenterMark(QPainter &p, const QPoint &center);
    // Draw top marker
    void drawTopMarker(QPainter &p, const QPoint &center, int outerRadius);
    // Draw target and path
    void drawTargetAndPath(QPainter &painter);
};

#endif // EWDISPLAY_H
