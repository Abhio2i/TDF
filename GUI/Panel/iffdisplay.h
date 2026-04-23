
/* =============================================================================
 * FILE:         IFFDisplay.h
 * MODULE:       IFF (Identification Friend or Foe) Display
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the IFFDisplay class which provides a widget for
 *               visualising Identification Friend or Foe (IFF) interrogation
 *               responses. It displays detected IFF responders in a polar
 *               (radar‑like) format with configurable range, rings, ticks,
 *               and hover detection. Supports display of responder ID, name,
 *               mode, code, and status. Integrates with Hierarchy and IFF/
 *               Platform entities for real‑time tracking.
 *
 * REQUIREMENTS: REQ-IFF-010  IFF visualisation widget
 *               REQ-IFF-011  Display IFF targets with distance, angle, mode,
 *                            code, responder ID/name, and status
 *               REQ-IFF-012  Configurable range and ring count
 *               REQ-IFF-013  Draw radar rings, ticks, labels, centre mark
 *               REQ-IFF-014  Mouse hover detection over targets
 *               REQ-IFF-015  Integration with Hierarchy and IFF entities
 *               REQ-IFF-016  Update display on entity selection and removal
 *               REQ-IFF-017  Maintain aspect ratio (16:9)
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-IFF-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef IFFDISPLAY_H
#define IFFDISPLAY_H

#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Hierarchy/hierarchy.h"
#include <QWidget>
#include <QComboBox>
#include <QVector>

// %%% IFF Target Structure %%%
struct IFFDisplayTarget {
    float distance = 0.0f;
    std::string responderId;
    std::string responderName;
    std::string mode;
    std::string code;
    float radius;
    float angle;
    int status;
};

// %%% Class Definition %%%
class IFFDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit IFFDisplay(QWidget *parent = nullptr);
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    QSize sizeHint() const override;
    QSize minimumSize() const;
    int range = 5000; // meters
    void setRange(float value) { range = value; }
    int heightForWidth(int width) const override;
    void selectEntity(Entity* entity);
    void RemoveEntity(QString ID);
    void updateRadar();

    // IFF instance
    IFF* iff = nullptr;
    QVector<IFF*> ifflist;
    // Entity platform
    Platform* entity = nullptr;
private slots:
    void onIFFSelected(int index);
protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // %%% Display Properties %%%
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
    QVector<IFF::IFFTarget> targets;
    QComboBox* iffDropdown = nullptr;
    void updateDropdown();

    // %%% Drawing Methods %%%
    void drawBackground(QPainter &p);
    void drawRadarRing(QPainter &p, const QPoint &center, int outerRadius);
    void drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius);
    void drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius);
    void drawCenterMark(QPainter &p, const QPoint &center);
    void drawTopMarker(QPainter &p, const QPoint &center, int outerRadius);
    void drawTargetAndPath(QPainter &painter);
    void drawIFFTargets(QPainter &p, const QPoint &center, int outerRadius);
};

#endif // IFFDISPLAY_H
