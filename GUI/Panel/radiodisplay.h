/* =============================================================================
 * FILE:         RADIODisplay.h
 * MODULE:       Radio Communication Display
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the RADIODisplay class which provides a widget for
 *               visualising radio communication links and detected radio
 *               emissions. It displays radio targets in a polar (radar‑like)
 *               format with configurable range, rings, ticks, and hover
 *               detection. Integrates with Hierarchy and Radio/Platform
 *               entities for real‑time tracking and display updates.
 *
 * REQUIREMENTS: REQ-RADIO-010  Radio communication visualisation widget
 *               REQ-RADIO-011  Display radio targets with distance and angle
 *               REQ-RADIO-012  Configurable range and ring count
 *               REQ-RADIO-013  Draw radar rings, ticks, labels, centre mark
 *               REQ-RADIO-014  Mouse hover detection over targets
 *               REQ-RADIO-015  Integration with Hierarchy and Radio entities
 *               REQ-RADIO-016  Update display on entity selection and removal
 *               REQ-RADIO-017  Maintain aspect ratio (16:9)
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-RADIO-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef RADIODISPLAY_H
#define RADIODISPLAY_H

#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/hierarchy.h"
#include <QWidget>
#include <QVector>
#include <QMouseEvent>
#include <QComboBox>

// %%% Class Definition %%%
class RADIODisplay : public QWidget
{
    Q_OBJECT

public:
    explicit RADIODisplay(QWidget *parent = nullptr);
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    QSize sizeHint() const override;
    QSize minimumSize() const;
     int range = 10; // meters
    void setRange(float value) { range = value; }
    int heightForWidth(int width) const override;
    void selectEntity(Entity* entity);
    void RemoveEntity(QString ID);
    void updateRadar();


    Radio* radio = nullptr;
        QVector<Radio*> radiolist;
    // Entity platform
    Platform* entity = nullptr;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void onRadioSelected(int index);
private:
    // %%% Display Properties %%%
    // int range = 10; // meters
    const double ASPECT_RATIO = 16.0/9.0;
    int padding = 18;
    int ringCount = 3;
    int majorTickEvery = 30;
    int minorTicksPerMajor = 5;
    QColor radarGreen = QColor(0, 255, 0);
    int ang = 0;
    QString id = "";
    Hierarchy* hierarchy = nullptr;
    QPoint mousePos;
    int hoveredTargetIndex = -1;
    QVector<Radio::RadioTarget> targets;
    QComboBox* radioDropdown = nullptr;
    void updateDropdown();

    // %%% Drawing Methods %%%
    void drawBackground(QPainter &p);
    void drawRadarRing(QPainter &p, const QPoint &center, int outerRadius);
    void drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius);
    void drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius);
    void drawCenterMark(QPainter &p, const QPoint &center);
    void drawTopMarker(QPainter &p, const QPoint &center, int outerRadius);
    void drawTargetAndPath(QPainter &painter);
    void drawRadioTargets(QPainter &p, const QPoint &center, int outerRadius);
};

#endif
