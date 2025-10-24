#ifndef RADARDISPLAY_H
#define RADARDISPLAY_H

#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/hierarchy.h"
#include <QWidget>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>


class RadarDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit RadarDisplay(QWidget *parent = nullptr);
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    void updateFromJson(const QJsonObject &json);
    QSize sizeHint() const override;
    QSize minimumSize() const ;
    int heightForWidth(int width) const override;
    void setAzimuth(float value);
    void setRange(float value);
    void selectEntity(Entity* entity);
    void RemoveEntity(QString ID);
    void updateRadar();
    Sensor* sensor = nullptr;
    Platform* entity = nullptr;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int range = 100;         // Default range for rendering
    double azimuth = 30.0;    // Default azimuth angle in degrees
    double bar = 3.0;        // Default beamwidth in degrees
    int current_speed = 0;   // No default, set via JSON
    int max_speed = 0;       // No default, set via JSON
    int radar_height = 0;    // No default, set via JSON
    // struct Target {
    //     double angle;
    //     double radius;
    // };
    QVector<Target> targets;
    QString id = "";

    Hierarchy* hierarchy = nullptr;

    // Separated drawing functions
    void drawBackground(QPainter &painter);
    void drawConcentricCircles(QPainter &painter, int centerX, int centerY, int radius);
    void drawVerticalLine(QPainter &painter, int centerX, int centerY, int radius);
    void drawCenterSquare(QPainter &painter, int centerX, int centerY);
    void drawAnnotations(QPainter &painter, int centerX, int widgetHeight);
    void drawAzimuth(QPainter &painter, int centerX, int centerY, int radius);
    void drawTargetAndPath(QPainter &painter, int centerX, int centerY);
    void drawInfoText(QPainter &painter, int widgetHeight);
};

#endif // RADARDISPLAY_H
