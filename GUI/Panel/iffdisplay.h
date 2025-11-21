


/* ========================================================================= */
/* File: IFFDisplay.h                                                        */
/* Purpose: Defines widget for IFF (Identification Friend or Foe) display    */
/* ========================================================================= */

#ifndef IFFDISPLAY_H
#define IFFDISPLAY_H

#include "core/Hierarchy/EntityProfiles/iff.h"    // IFF प्रोफाइल include करें
#include "core/Hierarchy/hierarchy.h"
#include <QWidget>
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
    int status;  // 1 = Friend, 0 = Unknown/Foe
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
    void setRange(float value) { range = value; }
    int heightForWidth(int width) const override;
    void selectEntity(Entity* entity);
    void RemoveEntity(QString ID);
    void updateRadar();

    // IFF instance
    IFF* iff = nullptr;
    // Entity platform
    Platform* entity = nullptr;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    // %%% Display Properties %%%
    int range = 5000; // meters
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
