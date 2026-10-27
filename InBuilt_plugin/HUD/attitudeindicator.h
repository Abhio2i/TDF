#pragma once

#include <QWidget>
#include <QPainter>
#include <QTransform>
#include <QKeyEvent>

class AttitudeIndicator : public QWidget {
    Q_OBJECT

public:
    // Flight Dynamics (State Variables)
    float pitch;    // Degrees (-50 to 50)
    float roll;     // Degrees (-180 to 180)
    float speed;    // Knots (0 to 400)
    float altitude; // Feet (0 to 9900)
    float heading;  // Degrees (0 to 359)

protected:
    void paintEvent(QPaintEvent *event) override;

    // Helper drawing functions to keep code organized
    void drawHorizon(QPainter &painter, const QPoint &center, int w, int h);
    void drawSpeedTape(QPainter &painter, int h);
    void drawAltitudeTape(QPainter &painter, int w, int h);
    void drawHeadingTape(QPainter &painter, int w, int h);
    void drawDataReadouts(QPainter &painter, int w, int h);

public:
    explicit AttitudeIndicator(QWidget *parent = nullptr);
    ~AttitudeIndicator() = default;
};
