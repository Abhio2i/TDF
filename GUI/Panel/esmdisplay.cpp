//============================================================================
// FILE:         esmdisplay.cpp
// MODULE:       ESM (Electronic Support Measures) Display
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen 2 Innovation (O2I).
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements the ESMDisplay class which provides a widget for
//               visualising Electronic Support Measures (ESM) / electronic
//               intelligence data. It displays detected emitters in a polar
//               (radar‑like) format with configurable range, rings, ticks,
//               and hover detection. Integrates with Hierarchy and Sensor/
//               Platform entities for real‑time tracking and display updates.
//
// REQUIREMENTS: Implements REQ-ESM-010 through REQ-ESM-017
//
// AUTHOR:       Arti Rajpoot
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-ESM-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Basic ESM display.
//   Rev 2  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Added requirement references and structured file header.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
//               Restricted circulation — defence simulation use only.
//============================================================================
#include "esmdisplay.h"                           // For EW display class
#include "core/Hierarchy/Utils/entityutils.h"
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
    : QWidget(parent), hoveredTargetIndex(-1)
{
    // Set background color
    setStyleSheet("background-color: black;");
    // Set size policy with aspect ratio
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    padding = 40;
    setMouseTracking(true);
    sensorDropdown = new QComboBox(this);
    sensorDropdown->setStyleSheet(
        "QComboBox { background-color: #001a00; color: #00ff00; "
        "border: 1px solid #00ff00; font-size: 10px; padding: 2px; }"
        "QComboBox QAbstractItemView { background-color: #001a00; "
        "color: #00ff00; selection-background-color: #003300; }"
        );
    sensorDropdown->hide();
    connect(sensorDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ESMDisplay::onSensorSelected);
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

/* Handle mouse move events for hover detection */
void ESMDisplay::mouseMoveEvent(QMouseEvent *event)
{
        // return;
    lastMousePos = event->pos();

    if (sensor->detect.isEmpty()) {
        hoveredTargetIndex = -1;
        update();
        return;
    }

    int w = width();
    int h = height();
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    QPoint center(w / 2, h / 2);

    // Check if mouse is near any target
    int closestIndex = -1;
    double minDistance = 20.0;

    int i=0;
    for (const ESMTarget &t : sensor->detect) {

        // Calculate target position on screen
        double per = t.radius / range;
        if (per < 0.0) per = 0.0;
        if (per > 1.0) per = 1.0;

        double r = per * outerRadius;
        double angleDeg = t.angle;
        double theta = qDegreesToRadians(angleDeg - 90.0);
        int tx = center.x() + int(r * cos(theta));
        int ty = center.y() + int(r * sin(theta));

        // Calculate distance from mouse to target
        double dx = lastMousePos.x() - tx;
        double dy = lastMousePos.y() - ty;
        double distance = sqrt(dx*dx + dy*dy);

        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
        i++;
    }

    if (hoveredTargetIndex != closestIndex) {
        hoveredTargetIndex = closestIndex;
        update();
    }

    QWidget::mouseMoveEvent(event);
}

/* Handle mouse leave events */
void ESMDisplay::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hoveredTargetIndex = -1;
    update();
    QWidget::leaveEvent(event);
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
        update();
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;

    sensor = nullptr;
        sensorlist.clear();
    for (auto const& pair :  *entity->sensors->sensors) {
        Sensor* s = pair.second;
        if (s && s->subType == Sensor::SubType::ESM) {

            ESM* sono = dynamic_cast<ESM*>(s);
            if(sono){

                if(sensor == nullptr){
                    sensor = sono;
                }
                sensorlist.append(sono);
            }
            setWindowTitle("ESM Display (" + QString::fromStdString(entity->Name) + ")");

        }
    }

    // Reset hover state when entity changes
    hoveredTargetIndex = -1;
    updateDropdown();

        update();
}

/* Remove entity if ID matches */
void ESMDisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        // Clear entity and sensor
        entity = nullptr;
        sensor = nullptr;
            sensorlist.clear();
        // Reset window title
        setWindowTitle("ESM Display");
        // Reset hover state
        hoveredTargetIndex = -1;
    }
}

/* Update radar display data */
void ESMDisplay::updateRadar()
{
    if (entity && sensor) {
        // Set radar range and trigger repaint
        setRange(sensor->range);
        update();
    } else {
        // Reset targets if no entity/sensor
        targets.clear();
           if (sensorDropdown) sensorDropdown->hide();
        hoveredTargetIndex = -1;
    }
}

/* Main paint event handler */
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

    if(!sensor)return;
    // Draw targets with dotted lines and labels
    if (!sensor->detect.isEmpty()) {
        int i=0;
        for (const ESMTarget &t : sensor->detect) {
            // const Target &t = targets[i];
            double per = t.radius / range;
            if (per < 0.0) per = 0.0;
            if (per > 1.0) per = 1.0;

            double r = per * outerRadius;
            double angleDeg = t.angle-90;
            double theta = qDegreesToRadians(angleDeg);
            int tx = center.x() + int(r * cos(theta));
            int ty = center.y() + int(r * sin(theta));

            // Draw dotted line from center to target
            // p.setPen(QPen(radarGreen, 0, Qt::DotLine));
            // p.drawLine(center, QPoint(tx, ty));

            // Draw target dot - blue normally, red if hovered
            if (i == hoveredTargetIndex) {
                p.setBrush(Qt::red);
            } else {
                p.setBrush(Qt::blue);
            }
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(tx, ty), 4, 4);

            // Draw labels ONLY if this target is hovered
            if (i == hoveredTargetIndex) {
                p.setPen(QPen(Qt::yellow, 1));
                QFont font = p.font();
                font.setPointSize(8);
                p.setFont(font);

                // Show angle and distance for hovered target
                QString angleText = QString("A:%1°").arg(angleDeg, 0, 'f', 1);
                QString distText = QString("D:%1").arg(t.radius, 0, 'f', 1);
                QString emmitterID = QString::fromStdString(t.entity->ID);//, 0, 'f', 1);
                QString emmittername = QString::fromStdString(t.entity->Name);//QString("D:%1").arg(t.radius, 0, 'f', 1);
                QString frequency = QString("F:%1 MHz").arg(t.frequency);
                QString frequencytype = "UHF";

                // Draw text at target position
                p.drawText(tx + 6, ty - 6, angleText);
                p.drawText(tx + 6, ty + 12, distText);
                p.drawText(tx + 6, ty + 30, emmitterID);
                p.drawText(tx + 6, ty + 48, emmittername);
                p.drawText(tx + 6, ty + 66, frequency);
                p.drawText(tx + 6, ty + 84, frequencytype);


                // Platform* targetPlatform = dynamic_cast<Platform*>(t.entity);
                // if (targetPlatform) {
                //     QString nameText = QString::fromStdString(targetPlatform->Name);
                //     p.drawText(tx + 6, ty + 30, nameText);
                // }
            }
            i++;
        }
    } else if (entity && sensor) {
        // No targets message
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPointSize(10);
        p.setFont(font);
        QRect textRect = p.fontMetrics().boundingRect("No ESM Targets Detected");
        p.drawText(center.x() - textRect.width()/2, center.y(), "No ESM Targets Detected");
    }

    qint64 elapsedMs = timer.elapsed();
    Profiler::currentFrame->esmdisplay = elapsedMs;
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
void ESMDisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (sensorDropdown) {
        int dropW = 120;
        int dropH = 22;
        sensorDropdown->setGeometry(width() - dropW - padding, 4, dropW, dropH);
    }
}

void ESMDisplay::updateDropdown()
{
    if (!sensorDropdown) return;

    QSignalBlocker blocker(sensorDropdown);
    sensorDropdown->clear();

    if (sensorlist.isEmpty()) {
        sensorDropdown->hide();
        return;
    }

    for (int i = 0; i < sensorlist.size(); ++i) {
        Sensor* s = sensorlist[i];
        QString name = s ? QString::fromStdString(s->Name) : QString("ESM %1").arg(i + 1);
        if (name.trimmed().isEmpty())
            name = QString("ESM %1").arg(i + 1);
        sensorDropdown->addItem(name);
    }

    int currentIdx = sensorlist.indexOf(sensor);
    if (currentIdx >= 0)
        sensorDropdown->setCurrentIndex(currentIdx);

    if (sensorlist.size() > 1)
        sensorDropdown->show();
    else
        sensorDropdown->hide();

    sensorDropdown->setGeometry(width() - 120 - padding, 4, 120, 22);
}

void ESMDisplay::onSensorSelected(int index)
{
    if (index < 0 || index >= sensorlist.size()) return;
    sensor = sensorlist[index];
    hoveredTargetIndex = -1;
    if (sensor)
        setRange(sensor->range);
    update();
}
