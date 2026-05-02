#include "adsbdisplay.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/adsbsensor.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qelapsedtimer.h"
#include <QPainter>                                // For painting operations
#include <QPaintEvent>                             // For paint events
#include <QFont>                                   // For font settings
#include <QtMath>                                  // For math functions
#include <QDebug>                                  // For debug output
#include <core/Debug/console.h>
#include <core/Hierarchy/EntityProfiles/platform.h>

ADSBDisplay::ADSBDisplay(QWidget *parent)
    : QWidget(parent), hoveredTargetIndex(-1)
{
    // Set background color
    setStyleSheet("background-color: black;");
    // Set size policy with aspect ratio
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    // Set padding
    padding = 40;

    // Enable mouse tracking for hover detection
    setMouseTracking(true);
}

// %%% Size Management %%%
/* Provide size hint for widget */
QSize ADSBDisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

/* Provide minimum size for widget */
QSize ADSBDisplay::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

/* Calculate height based on width and aspect ratio */
int ADSBDisplay::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

/* Handle mouse move events for hover detection */
void ADSBDisplay::mouseMoveEvent(QMouseEvent *event)
{

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
    double minDistance = 20.0; // Pixel threshold for hover detection
    int i=0;
    for (const ADSBTarget &t : sensor->detect) {
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
        update(); // Repaint to show/hide labels
    }
    QWidget::mouseMoveEvent(event);
}

/* Handle mouse leave events */
void ADSBDisplay::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hoveredTargetIndex = -1;
    update();
    QWidget::leaveEvent(event);
}

/* Select entity for display */
void ADSBDisplay::selectEntity(Entity* entit)
{
    entity = nullptr;
    id = "";
    Platform* platform = nullptr;
    platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        setWindowTitle("CSM Display (No Platform)");
        update();
        return;
    }
    id = QString::fromStdString(platform->ID);
    // Set entity ID and pointer
    entity = platform;
    sensor = nullptr;
    sensorlist.clear();
    for (auto const& pair :  *entity->sensors->sensors) {
        Sensor* s = pair.second;
        if (s && s->subType == Sensor::SubType::ADSB) {
            ADSBSensor* sono = dynamic_cast<ADSBSensor*>(s);
            if(sono){

                if(sensor == nullptr){
                    sensor = sono;
                }
                sensorlist.append(sono);
            }
            setWindowTitle("ADSB Display (" + QString::fromStdString(entity->Name) + ")");
        }
    }
    // Reset hover state when entity changes
    hoveredTargetIndex = -1;
    update();
}

/* Remove entity from display */
void ADSBDisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        // Clear entity and sensor
        entity = nullptr;
        sensor = nullptr;
        sensorlist.clear();
        // Reset window title
        setWindowTitle("CSM Display");
        // Reset hover state
        hoveredTargetIndex = -1;
    }
}

/* Update radar display with new data */
void ADSBDisplay::updateRadar()
{
    if (entity && sensor) {
        setRange(sensor->range);
        // targets = sensor->ewtargets;
        update();
    } else {
        // Reset targets if no entity/sensor
        targets.clear();
        hoveredTargetIndex = -1;
    }
}

/* Main paint event handler */
void ADSBDisplay::paintEvent(QPaintEvent * /*event*/)
{
    QElapsedTimer timer;
    timer.start();  // Start measuring

    if (width() <= 0 || height() <= 0) return;

    QPainter p(this);
    //p.setRenderHint(QPainter::Antialiasing);

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
    if ( !sensor->detect.isEmpty()) {
        int i=0;
        for (const ADSBTarget &t : sensor->detect) {
            // const Target &t = targets[i];

            // FIX: Manual bound check
            double per = t.radius / range;
            if (per < 0.0) per = 0.0;
            if (per > 1.0) per = 1.0;

            double r = per * outerRadius;
            double angleDeg = t.angle - 90;
            double theta = qDegreesToRadians(angleDeg);
            int tx = center.x() + int(r * cos(theta));
            int ty = center.y() + int(r * sin(theta));

            // // Draw dotted line from center to target
            // p.setPen(QPen(radarGreen, 1, Qt::DotLine));
            // p.drawLine(center, QPoint(tx, ty));

            // // Draw target dot - blue normally, red if hovered
            // if (i == hoveredTargetIndex) {
            //     p.setBrush(Qt::red);
            // } else {
            //     p.setBrush(Qt::blue);
            // }
            // p.setPen(Qt::NoPen);
            // p.drawEllipse(QPointF(tx, ty), 4, 4);

            // --- Aircraft Shape Drawing ---
            p.save(); // Current painter state save karein[cite: 2]
            p.translate(tx, ty); // Painter ko aircraft ke center position par le jayein[cite: 2]
            p.rotate(t.angle); // Aircraft ko uske heading angle par rotate karein[cite: 2]

            // Hover ke hisaab se color set karein
            if (i == hoveredTargetIndex) {
                p.setBrush(Qt::red); // Hover karne par red[cite: 2]
            } else {
                p.setBrush(Qt::blue); // Normally blue[cite: 2]
            }
            p.setPen(QPen(Qt::white, 0.5)); // Patli white outline

            // Aircraft Polygon define karein (origin 0,0 nose ke thoda niche hai)
            QPolygon aircraft;
            aircraft << QPoint(0, -12)   // Nose (Top)
                     << QPoint(1.5, -5)  // Right Fuselage front
                     << QPoint(12, 0)    // Right Wingtip front
                     << QPoint(12, 2)    // Right Wingtip back
                     << QPoint(1.5, 5)   // Right Fuselage back
                     << QPoint(5, 10)    // Right Tail stabilizer
                     << QPoint(0, 8)     // Rear center (notch)
                     << QPoint(-5, 10)   // Left Tail stabilizer
                     << QPoint(-1.5, 5)  // Left Fuselage back
                     << QPoint(-12, 2)   // Left Wingtip back
                     << QPoint(-12, 0)   // Left Wingtip front
                     << QPoint(-1.5, -5);// Left Fuselage front
            // Polygon automatically connect back to nose.

            p.drawPolygon(aircraft);// Polygon draw karein
                p.restore(); // Painter state restore karein[cite: 2]

            // Draw labels ONLY if this target is hovered
            if (i == hoveredTargetIndex) {
                p.setPen(QPen(Qt::yellow, 1));
                QFont font = p.font();
                font.setPointSize(8);
                p.setFont(font);

                Platform* targetPlatform = dynamic_cast<Platform*>(t.entity);
                QString targetName = targetPlatform ? QString::fromStdString(targetPlatform->Name) : "Unknown";

                // 1. Pehle saare strings prepare kar lete hain (Formatting)
                QString angleText = QString("Angle: %1°").arg(t.angle, 0, 'f', 1);
                QString distText  = QString("Dist: %1km").arg(t.radius, 0, 'f', 1);
                QString callSign  = QString("CallSign: %1").arg(QString::fromStdString(t.call_sign));
                QString hexId     = QString("Hex: %1").arg(QString::fromStdString(t.hex_ident));
                QString flightId  = QString("Flight: %1").arg(QString::fromStdString(t.fligh_Id));

                // Dynamic Data
                QString altText   = QString("ALT: %1 ft").arg(t.altitude);
                QString gsText    = QString("GS: %1 kt").arg(t.entity->dynamicModel->GroundVelocity);
                QString trackText = QString("TRK: %1°").arg(t.track);
                QString climbText = QString("ROC: %1 fpm").arg(t.climb_rate);
                QString posText   = QString("Pos: %1, %2").arg(t.entity->transform->getLatitude(), 0, 'f', 4).arg(t.entity->transform->getLongitude(), 0, 'f', 4);

                // Status Data
                QString squawkText = QString("Squawk: %1").arg(QString::fromStdString(t.squawk));
                QString statusText = QString("Status: %1%2%3")
                                         .arg(t.alert ? "ALERT " : "")
                                         .arg(t.emergency ? "EMERGENCY " : "")
                                         .arg(t.is_on_ground ? "GROUND" : "AIR");

                // 2. Drawing Logic (Y-offset maintain karte hue)
                int x = tx + 10;   // Target se thoda right
                int y = ty - 10;   // Starting height
                int gap = 15;      // Har line ke beech ka gap

                // Text Render Karein
                p.drawText(x, y,          callSign);
                p.drawText(x, y += gap,  hexId);
                p.drawText(x, y += gap,  flightId);
                p.drawText(x, y += gap,  altText);
                p.drawText(x, y += gap,  gsText);
                p.drawText(x, y += gap,  trackText);
                p.drawText(x, y += gap,  climbText);
                p.drawText(x, y += gap,  posText);
                p.drawText(x, y += gap,  angleText);
                p.drawText(x, y += gap,  distText);
                p.drawText(x, y += gap,  squawkText);

                // Agar Emergency hai toh red color mein dikha sakte hain
                if(t.emergency) {
                    p.setPen(Qt::red);
                }
                p.drawText(x, y += gap,  statusText);
            }
            i++;
        }
    } else if (entity && sensor) {
        // No targets message
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPointSize(10);
        p.setFont(font);
        QRect textRect = p.fontMetrics().boundingRect("No CSM Targets Detected");
        p.drawText(center.x() - textRect.width()/2, center.y(), "No CSM Targets Detected");
    }

    qint64 elapsedMs = timer.elapsed();
    // Profiler::currentFrame->ADSBDisplay = elapsedMs;
}

// %%% Drawing Methods %%%
/* Draw display background */
void ADSBDisplay::drawBackground(QPainter &p)
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
void ADSBDisplay::drawRadarRing(QPainter &p, const QPoint &center, int outerRadius)
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
void ADSBDisplay::drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius)
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
void ADSBDisplay::drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius)
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
void ADSBDisplay::drawCenterMark(QPainter &p, const QPoint &center)
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
void ADSBDisplay::drawTopMarker(QPainter &p, const QPoint &center, int outerRadius)
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

/* Draw targets and their paths */
void ADSBDisplay::drawTargetAndPath(QPainter &painter)
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

        for (const Target &target : sensor->csmtargets) {
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
