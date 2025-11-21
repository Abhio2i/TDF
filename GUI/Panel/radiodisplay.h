#ifndef RADIODISPLAY_H
#define RADIODISPLAY_H

#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/hierarchy.h"
#include <QWidget>
#include <QVector>
#include <QMouseEvent>  // ✅ ADD THIS

// %%% Class Definition %%%
class RADIODisplay : public QWidget
{
    Q_OBJECT

public:
    explicit RADIODisplay(QWidget *parent = nullptr);
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    QSize sizeHint() const override;
    QSize minimumSize() const;
    void setRange(float value) { range = value; }
    int heightForWidth(int width) const override;
    void selectEntity(Entity* entity);
    void RemoveEntity(QString ID);
    void updateRadar();

    // ✅ RADIO INSTANCE (NOT IFF)
    Radio* radio = nullptr;
    // Entity platform
    Platform* entity = nullptr;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    // %%% Display Properties %%%
    int range = 10; // meters
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
    void drawRadioTargets(QPainter &p, const QPoint &center, int outerRadius); // ✅ RENAME TO drawRadioTargets
};

#endif
