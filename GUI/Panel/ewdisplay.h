

#ifndef EWDISPLAY_H
#define EWDISPLAY_H

#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/hierarchy.h"
#include <QWidget>
#include <QVector>

struct EWTarget {
    double angle = 0.0;
    double radius = 0.0;
};

class EWDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit EWDisplay(QWidget *parent = nullptr);

    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    QSize sizeHint() const override;
    QSize minimumSize() const;
    void setRange(float value){range = value;}
    int heightForWidth(int width) const override;
    void selectEntity(Entity* entity);
    void RemoveEntity(QString ID);
    void updateRadar();
    Sensor* sensor = nullptr;
    Platform* entity = nullptr;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int range = 100;
    const double ASPECT_RATIO = 16.0/9.0;
    int padding = 18;
    int ringCount = 3;
    int majorTickEvery = 30;
    int minorTicksPerMajor = 5;
    QColor radarGreen = QColor(0, 255, 0);

    int ang = 0;
    QVector<Target> targets;
    QString id = "";
    Hierarchy* hierarchy = nullptr;


    void drawBackground(QPainter &p);
    void drawRadarRing(QPainter &p, const QPoint &center, int outerRadius);
    void drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius);
    void drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius);
    void drawCenterMark(QPainter &p, const QPoint &center);
    void drawTopMarker(QPainter &p, const QPoint &center, int outerRadius);
    void drawTargetAndPath(QPainter &painter);
};

#endif // EWDISPLAY_H
