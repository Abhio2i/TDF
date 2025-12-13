/* ========================================================================= */
/* File: ESMDisplay.cpp                                                    */
/* Purpose: Implements electronic warfare display for radar visualization   */
/* ========================================================================= */

#include "esmdisplay.h"                           // For EW display class
#include "qelapsedtimer.h"
#include <QPainter>                                // For painting operations
#include <QPaintEvent>                             // For paint events
#include <QFont>                                   // For font settings
#include <QtMath>                                  // For math functions
#include <QDebug>                                  // For debug output
#include <core/Debug/console.h>                    // For console error logging

// %%% Constructor %%%
/* Initialize electronic warfare display */
ESMDisplay::ESMDisplay(QWidget *parent)
    : QWidget(parent)
{
    // Set background color
    setStyleSheet("background-color: black;");
    // Set size policy with aspect ratio
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    // Set padding
    padding = 40;
}

// %%% Size Management %%%
/* Provide size hint for widget */
QSize ESMDisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

/* Provide minimum size for widget */
QSize ESMDisplay::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

/* Calculate height based on width and aspect ratio */
int ESMDisplay::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

/* Select and configure entity for display */
void ESMDisplay::selectEntity(Entity* entit)
{


    entity = nullptr;
    id = "";

    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        setWindowTitle("ESM Display (No Platform)");
        //update();
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;


    sensor = nullptr;
    for (Sensor* s : entity->sensorList) {
        if (s && s->subType == Sensor::SubType::ESM) {
            sensor = s;
            // qDebug() << "🎯 ESM Sensor selected for platform:" << QString::fromStdString(entity->Name)
            //          << "Sensor ID:" << QString::fromStdString(sensor->ID)
            //          << "Range:" << sensor->esrange << "km";

            // // Connect Sensor signals to this display
            // connect(sensor, &Sensor::availableConnectionsUpdated, this, &ESMDisplay::updateRadar);

            // // 🔥 IMMEDIATE UPDATE TRIGGER
            // if (entity->transform) {
            //     sensor->esmScan(entity->ID, entity->transform);
            // }

            setWindowTitle("ESM Display (" + QString::fromStdString(entity->Name) + ")");
            break;
        }
    }

    // if (!sensor) {
    //     qDebug() << "❌ No ESM Sensor found for platform:" << QString::fromStdString(entity->Name);

    //     // 🔥 Alternative: Check if any sensor can be used as ESM
    //     for (Sensor* s : entity->sensorList) {
    //         if (s) {
    //             sensor = s;
    //             qDebug() << "⚠️ Using generic sensor as ESM for:" << QString::fromStdString(entity->Name);
    //             connect(sensor, &Sensor::availableConnectionsUpdated, this, &ESMDisplay::updateRadar);
    //             setWindowTitle("ESM Display (" + QString::fromStdString(entity->Name) + " - Generic)");
    //             break;
    //         }
    //     }

    //     if (!sensor) {
    //         setWindowTitle("ESM Display (No ESM Sensor)");
    //     }
    // }

    // 🔥 FORCE UI UPDATE
    // updateRadar();
    // update();
}
/* Remove entity if ID matches */
void ESMDisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        // Clear entity and sensor
        entity = nullptr;
        sensor = nullptr;
        // Reset window title
        setWindowTitle("Radar Display");
    }
}

/* Update radar display data */
void ESMDisplay::updateRadar()
{
    if (entity && sensor) {
        // Set radar range and trigger repaint
        setRange(sensor->esrange);

        // 🔥 Ensure targets are properly updated
        targets = sensor->esmtargets;

        // // 🔥 DEBUG: Show current targets information
        // qDebug() << "🔄 ESMDisplay update - Entity:" << QString::fromStdString(entity->Name)
        //          << "Targets count:" << targets.size()
        //          << "Range:" << range << "km";

        // for (int i = 0; i < targets.size(); ++i) {
        //     const Target &target = targets.at(i);
        //     Platform* targetPlatform = dynamic_cast<Platform*>(target.entity);
        //     if (targetPlatform) {
        //         qDebug() << "  Target" << i << ":" << QString::fromStdString(targetPlatform->Name)
        //                  << "Dist:" << target.radius << "km, Angle:" << target.angle << "°";
        //     }
        // }

        update();
    } else {
        // if (!entity) {
        //     //qDebug() << "❌ ESMDisplay - No entity selected";
        // } else if (!sensor) {
        //     //qDebug() << "❌ ESMDisplay - No ESM sensor component found";
        // }
        // // Clear display when no entity/sensor
        // update();
    }
}


void ESMDisplay::paintEvent(QPaintEvent * /*event*/)
{

    QElapsedTimer timer;
    timer.start();  // Start measuring
    if (width() <= 0 || height() <= 0) return;

    // Initialize painter
    QPainter p(this);
    //p.setRenderHint(QPainter::Antialiasing);

    // Draw display components
    drawBackground(p);
    int w = width();
    int h = height();
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    QPoint center(w / 2, h / 2);

    drawRadarRing(p, center, outerRadius);
    drawConcentricCircles(p, center, outerRadius);
    drawTicksAndLabels(p, center, outerRadius);
    drawCenterMark(p, center);
    drawTopMarker(p, center, outerRadius);


    // Draw targets with dotted lines and labels
    if (!targets.isEmpty()) {
        for (const Target &t : targets) {
            // FIX: Manual bound check
            double per = t.radius / range;
            if (per < 0.0) per = 0.0;
            if (per > 1.0) per = 1.0;

            double r = per * outerRadius;
            double angleDeg = t.angle;
            double theta = qDegreesToRadians(angleDeg - 90.0);
            int tx = center.x() + int(r * cos(theta));
            int ty = center.y() + int(r * sin(theta));

            // Draw dotted line from center to target
            p.setPen(QPen(radarGreen, 1, Qt::DotLine));
            p.drawLine(center, QPoint(tx, ty));

            // Draw red dot at target position
            p.setBrush(Qt::red);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(tx, ty), 4, 4);

            // Draw labels - sirf angle aur distance
            p.setPen(QPen(Qt::yellow, 1));
            QFont font = p.font();
            font.setPointSize(8);
            p.setFont(font);

            // QString angleText = QString("A:%1°").arg(angleDeg, 0, 'f', 1);
            QString distText = QString("D:%1").arg(t.radius, 0, 'f', 1);

            // Draw text at target position
            // p.drawText(tx + 6, ty - 6, angleText);
            p.drawText(tx + 6, ty + 12, distText);
        }
    } else if (entity && sensor) {
        // No targets message
        p.setPen(Qt::white);
        p.drawText(center, "No ESM Targets Detected");
    }
    qint64 elapsedMs = timer.elapsed();
    Profiler::currentFrame->esmdisplay = elapsedMs;
}
// %%% Drawing Methods %%%
/* Draw targets and their paths */
void ESMDisplay::drawTargetAndPath(QPainter &painter)
{
    int w = width();
    int h = height();
    int centerX = w/2;
    int centerY = h/2;
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    if (entity && sensor) {
        // Get entity angle
        ang = entity->transform->toEulerAngles().y();
        painter.setBrush(Qt::red);
        for (const Target &target : sensor->esmtargets) {




            // Calculate target position
            int panelhigh = outerRadius;
            float per = target.radius/range;
            float radius = panelhigh*per;
            float angle = target.angle;
            double targetAngle = (angle + 90) * M_PI / 180;
            double targetRadius = radius;
            int targetX = centerX + static_cast<int>(targetRadius * cos(targetAngle));
            int targetY = centerY - static_cast<int>(targetRadius * sin(targetAngle));
            // Draw target point
            painter.drawEllipse(targetX - 3, targetY - 3, 6, 6);
            // Draw target labels
            painter.setPen(QPen(Qt::green, 1));
            painter.drawText(targetX - 20, targetY - 10, QString("%1").arg(angle));
            painter.drawText(targetX - 20, targetY + 5, QString("%1").arg(radius));
        }
    }
}

/* Draw display background */
void ESMDisplay::drawBackground(QPainter &p)
{
    p.save();
    // Fill background
    p.fillRect(rect(), Qt::black);
    // Draw border
    QPen pen(radarGreen, 1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QRect inner(padding, padding, width() - padding*2, height() - padding*2);
    p.drawRect(inner);
    p.restore();
}

/* Draw outer radar ring */
void ESMDisplay::drawRadarRing(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen ringPen(radarGreen, 2);
    p.setPen(ringPen);
    p.setBrush(Qt::NoBrush);
    QRectF circle(center.x() - outerRadius, center.y() - outerRadius, outerRadius*2.0, outerRadius*2.0);
    p.drawEllipse(circle);
    p.restore();
}

/* Draw concentric radar circles */
void ESMDisplay::drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen pen(radarGreen, 1, Qt::DashLine);
    pen.setCosmetic(true);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    for (int i = 1; i <= ringCount; ++i) {
        double r = outerRadius * (double(i) / double(ringCount + 1));
        QRectF ring(center.x() - r, center.y() - r, r * 2.0, r * 2.0);
        p.drawEllipse(ring);
    }
    p.restore();
}

/* Draw radar ticks and labels */
void ESMDisplay::drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen majorPen(radarGreen, 2);
    QPen minorPen(radarGreen, 1);
    QFont labelFont("Arial", qMax(8, outerRadius/18), QFont::Bold);
    p.setFont(labelFont);
    for (int deg = 0; deg < 360; deg += (majorTickEvery / minorTicksPerMajor)) {
        double angleDeg = deg - 90.0 + ang;
        double theta = qDegreesToRadians(angleDeg);
        bool isMajor = (deg % majorTickEvery == 0);
        int tickOut = outerRadius;
        int tickIn = isMajor ? outerRadius - qMax(16, outerRadius/12) : outerRadius - qMax(6, outerRadius/24);
        int x1 = center.x() + int(tickOut * cos(theta));
        int y1 = center.y() + int(tickOut * sin(theta));
        int x2 = center.x() + int(tickIn * cos(theta));
        int y2 = center.y() + int(tickIn * sin(theta));
        p.setPen(isMajor ? majorPen : minorPen);
        p.drawLine(QPoint(x1, y1), QPoint(x2, y2));
        if (isMajor) {
            // Draw degree labels
            int labelRadius = outerRadius + qMax(16, outerRadius/10);
            int lx = center.x() + int(labelRadius * cos(theta));
            int ly = center.y() + int(labelRadius * sin(theta));
            int heading = (deg % 360);
            QString text = QString("%1").arg(heading, 3, 10, QChar('0'));
            QRect textRect = QFontMetrics(p.font()).boundingRect(text);
            int tx = lx - textRect.width()/2;
            int ty = ly + textRect.height()/2;
            p.setPen(radarGreen);
            p.drawText(tx, ty, text);
        }
    }
    p.restore();
}

/* Draw center cross mark */
void ESMDisplay::drawCenterMark(QPainter &p, const QPoint &center)
{
    p.save();
    p.setPen(QPen(radarGreen, 2));
    int cross = qMax(6, width()/80);
    p.drawLine(center.x() - cross, center.y(), center.x() + cross, center.y());
    p.drawLine(center.x(), center.y() - cross, center.x(), center.y() + cross);
    QRect sq(center.x() - 5, center.y() - 5, 10, 10);
    p.setBrush(radarGreen);
    p.drawRect(sq);
    p.restore();
}

/* Draw top marker triangle and label */
void ESMDisplay::drawTopMarker(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    double theta = qDegreesToRadians(-90.0);
    int triCenterX = center.x() + int((outerRadius + 4) * cos(theta));
    int triCenterY = center.y() + int((outerRadius + 4) * sin(theta));
    int triW = qMax(6, outerRadius/20);
    QPoint pts[3] = {
        QPoint(triCenterX, triCenterY - triW),
        QPoint(triCenterX - triW, triCenterY + (triW/2)),
        QPoint(triCenterX + triW, triCenterY + (triW/2))
    };
    p.setPen(QPen(radarGreen, 1));
    p.setBrush(radarGreen);
    p.drawPolygon(pts, 3);
    QFont f("Arial", qMax(6, outerRadius/25), QFont::Bold);
    p.setFont(f);
    QString txt = "000";
    QRect txtRect = QFontMetrics(f).boundingRect(txt);
    int tx = triCenterX - txtRect.width()/2;
    int ty = triCenterY + triW + txtRect.height() + 1;
    p.setPen(radarGreen);
    p.drawText(tx, ty, txt);
    p.restore();
}
