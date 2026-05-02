
/* =============================================================================
 * FILE:         ESMDisplay.h
 * MODULE:       ESM (Electronic Support Measures) Display
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the ESMDisplay class which provides a widget for
 *               visualising Electronic Support Measures (ESM) / electronic
 *               intelligence data. It displays detected emitters in a polar
 *               (radar‑like) format with configurable range, rings, ticks,
 *               and hover detection. Integrates with Hierarchy and Sensor/
 *               Platform entities for real‑time tracking and display updates.
 *
 * REQUIREMENTS: REQ-ESM-010  ESM visualisation widget
 *               REQ-ESM-011  Display targets with angle and radius
 *               REQ-ESM-012  Configurable range and ring count
 *               REQ-ESM-013  Draw radar rings, ticks, labels, centre mark
 *               REQ-ESM-014  Mouse hover detection over targets
 *               REQ-ESM-015  Integration with Hierarchy and Sensor entities
 *               REQ-ESM-016  Update display on entity selection and removal
 *               REQ-ESM-017  Maintain aspect ratio (16:9)
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-ESM-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef ESMDisplay_H
#define ESMDisplay_H

#include "core/Hierarchy/EntityProfiles/SensorProfiles/esm.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"  // For sensor profile
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include <QWidget>                                // For widget base class
#include <QVector>                                // For vector container
#include <QComboBox>
#include <core/Debug/profiler.h>
// %%% Data Structures %%%
/* Structure for electronic warfare target */


// %%% Class Definition %%%
/* Widget for electronic warfare visualization */
class ESMDisplay : public QWidget
{
    Q_OBJECT

public:
    // Initialize EW display
    explicit ESMDisplay(QWidget *parent = nullptr);
    // Set hierarchy instance
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    // Get size hint
    QSize sizeHint() const override;
    // Get minimum size
     int range = 100;
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
    ESM* sensor = nullptr;
    QVector<ESM*> sensorlist;
    // Entity platform
    Platform* entity = nullptr;

protected:
    // Handle paint events
    void paintEvent(QPaintEvent *event) override;
    // Handle mouse move for hover detection
    void mouseMoveEvent(QMouseEvent *event) override;
    // Handle mouse leave
    void leaveEvent(QEvent *event) override;
       void resizeEvent(QResizeEvent *event) override;
private slots:
    void onSensorSelected(int index);
private:
    // %%% Display Properties %%%
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

    // Hover tracking
    int hoveredTargetIndex = -1;
    QPoint lastMousePos;
    QComboBox* sensorDropdown = nullptr;
    void updateDropdown();
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
};

#endif // ESMDisplay_H
