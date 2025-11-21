
/* ========================================================================= */
/* File: RADIODisplay.cpp                                                    */
/* Purpose: Implements Radio display for communication detection           */
/* ========================================================================= */

#include "radiodisplay.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <QtMath>
#include <QDebug>
#include <core/Debug/console.h>
#include <core/Hierarchy/EntityProfiles/radio.h>

// %%% Constructor %%%
/* Initialize Radio display */
RADIODisplay::RADIODisplay(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: black;");
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setMouseTracking(true);
    setSizePolicy(policy);
    padding = 40;
}

// %%% Size Management %%%
QSize RADIODisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

QSize RADIODisplay::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

int RADIODisplay::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

// %%% Entity Management %%%
void RADIODisplay::selectEntity(Entity* entit)
{
    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;

    // Select first valid Radio
    radio = nullptr;
    for (Radio* r : entity->radioList) {
        if (r) {
            radio = r;
            setWindowTitle("RADIO Display (" + QString::fromStdString(entity->Name) + ")");
            // qDebug() << "🎯 RADIO selected for platform:" << QString::fromStdString(entity->Name)
            //          << "Radio targets count:" << radio->targets.size();

            // Disconnect previous connections
            if (radio) {
                disconnect(radio, &Radio::availableConnectionsUpdated, this, &RADIODisplay::updateRadar);
            }

            // Connect Radio signals to this display
            connect(radio, &Radio::availableConnectionsUpdated, this, &RADIODisplay::updateRadar);

            // Immediate update
            updateRadar();
            break;
        }
    }

    if (!radio) {
        // qDebug() << "❌ No RADIO found for platform:" << QString::fromStdString(entity->Name);
        // setWindowTitle("RADIO Display (No Radio)");
    }
}

void RADIODisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        entity = nullptr;
        radio = nullptr;
        setWindowTitle("RADIO Display");
    }
}

// %%% Update Methods %%%
void RADIODisplay::updateRadar()
{
    if (entity && radio) {
        setRange(radio->calculateRange()); // Use radio's calculated range


        update();
    } else {
        if (!entity)"";
        //qDebug() << "❌ RADIODisplay updateRadar - No entity selected";
        else if (!radio)"";
        //qDebug() << "❌ RADIODisplay updateRadar - No RADIO component";
    }
}

// %%% Paint Event %%%
void RADIODisplay::paintEvent(QPaintEvent * /*event*/)
{
    if (width() <= 0 || height() <= 0) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

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
    drawTargetAndPath(p);

    // Draw Radio targets
    drawRadioTargets(p, center, outerRadius);
}

// void RADIODisplay::drawRadioTargets(QPainter &p, const QPoint &center, int outerRadius)
// {
//     if (!entity || !radio) {
//         qDebug() << "❌ RADIODisplay - No entity or Radio for drawing";
//         p.setPen(Qt::white);
//         p.drawText(center, "No Radio Selected\nSelect an entity with Radio");
//         return;
//     }

//     if (radio->targets.empty()) {
//         p.setPen(Qt::yellow);
//         p.drawText(center.x() - 50, center.y(), "No Radio Targets");
//         return;
//     }

//     hoveredTargetIndex = -1;
//     QList<QPointF> positions;
//     const int dotSize = 8;

//     for (int i = 0; i < radio->targets.size(); ++i) {
//         const Radio::RadioTarget &target = radio->targets.at(i);

//         // ✅ DO NOT draw if out of range
//         if (target.radius > range) {
//             continue;
//         }

//         float per = target.radius / range;
//         float radius = outerRadius * per;
//         float angle = target.angle;
//         double theta = qDegreesToRadians(angle + 90);

//         QPointF pos(center.x() + radius * cos(theta),
//                     center.y() - radius * sin(theta));
//         positions.append(pos);

//         QRectF dotRect(pos.x() - dotSize / 2, pos.y() - dotSize / 2, dotSize, dotSize);
//         if (dotRect.contains(mousePos)) {
//             hoveredTargetIndex = i;
//         }

//         p.setBrush(Qt::blue);
//         p.setPen(Qt::NoPen);
//         p.drawEllipse(dotRect);

//         p.setPen(QPen(radarGreen, 1, Qt::DotLine));
//         p.drawLine(center, QPoint(pos.x(), pos.y()));

//         p.setPen(QPen(Qt::white, 1));
//         QString info = QString("D:%1 A:%2").arg(target.radius, 0, 'f', 1).arg(target.angle, 0, 'f', 1);
//         p.drawText(pos.x() + 5, pos.y() - 5, info);
//     }

//     if (hoveredTargetIndex >= 0 && hoveredTargetIndex < positions.size()) {
//         const Radio::RadioTarget &target = radio->targets.at(hoveredTargetIndex);
//         QPointF pos = positions[hoveredTargetIndex];
//         QString info = QString("Radio: %1\nDist: %2 m\nAngle: %3°\nFreq: %4 MHz\nRange: %5 m")
//                            .arg(QString::fromStdString(target.name))
//                            .arg(target.radius, 0, 'f', 1)
//                            .arg(target.angle, 0, 'f', 1)
//                            .arg(target.frequency, 0, 'f', 1)
//                            .arg(target.range, 0, 'f', 1);

//         QFontMetrics fm(p.font());
//         QRect infoRect = fm.boundingRect(QRect(), Qt::AlignLeft | Qt::AlignTop, info);
//         infoRect.moveTo(pos.x() + 12, pos.y() - infoRect.height() / 2);
//         infoRect.adjust(-6, -4, 6, 4);

//         p.setBrush(QColor(0, 0, 0, 220));
//         p.setPen(QPen(Qt::white, 1));
//         p.drawRect(infoRect);
//         p.drawText(infoRect, Qt::AlignLeft | Qt::AlignTop, info);
//     }
// }


void RADIODisplay::drawRadioTargets(QPainter &p, const QPoint &center, int outerRadius)
{
    if (!entity || !radio) {
        qDebug() << "❌ RADIODisplay - No entity or Radio for drawing";
        p.setPen(Qt::white);
        p.drawText(center, "No Radio Selected\nSelect an entity with Radio");
        return;
    }

    if (radio->targets.empty()) {
        p.setPen(Qt::yellow);
        p.drawText(center.x() - 50, center.y(), "No Radio Targets");
        return;
    }

    hoveredTargetIndex = -1;
    QList<QPointF> positions;
    const int dotSize = 8;

    // Get current radio's frequency range for filtering
    float currentFreqMin = radio->frequencyMin;
    float currentFreqMax = radio->frequencyMax;
    float currentFreqUsed = radio->frequencyUsed;

    for (int i = 0; i < radio->targets.size(); ++i) {
        const Radio::RadioTarget &target = radio->targets.at(i);

        // ✅ RANGE FILTER: Skip if out of range
        if (target.radius > range) {
            continue;
        }

        // ✅ FREQUENCY FILTER: Skip if frequency doesn't match
        bool frequencyMatch = false;

        if (currentFreqUsed > 0 && target.frequency > 0) {
            // If both have specific frequencies used, check if they match (with some tolerance)
            float freqTolerance = 1.0f; // 1 MHz tolerance
            frequencyMatch = (std::abs(currentFreqUsed - target.frequency) <= freqTolerance);
        } else {
            // Check frequency range overlap
            float targetFreqMin = target.frequency - (radio->bandwidth / 2 / 1000.0f); // Convert kHz to MHz
            float targetFreqMax = target.frequency + (radio->bandwidth / 2 / 1000.0f);

            frequencyMatch = (currentFreqMax >= targetFreqMin && currentFreqMin <= targetFreqMax);
        }

        if (!frequencyMatch) {
            continue;
        }

        // If both range and frequency filters pass, draw the target
        float per = target.radius / range;
        float radius = outerRadius * per;
        float angle = target.angle;
        double theta = qDegreesToRadians(angle + 90);

        QPointF pos(center.x() + radius * cos(theta),
                    center.y() - radius * sin(theta));
        positions.append(pos);

        QRectF dotRect(pos.x() - dotSize / 2, pos.y() - dotSize / 2, dotSize, dotSize);
        if (dotRect.contains(mousePos)) {
            hoveredTargetIndex = i;
        }

        p.setBrush(Qt::blue);
        p.setPen(Qt::NoPen);
        p.drawEllipse(dotRect);

        p.setPen(QPen(radarGreen, 1, Qt::DotLine));
        p.drawLine(center, QPoint(pos.x(), pos.y()));

        p.setPen(QPen(Qt::white, 1));
        QString info = QString("D:%1 A:%2 F:%3").arg(target.radius, 0, 'f', 1).arg(target.angle, 0, 'f', 1).arg(target.frequency, 0, 'f', 1);
        p.drawText(pos.x() + 5, pos.y() - 5, info);
    }

    if (hoveredTargetIndex >= 0 && hoveredTargetIndex < positions.size()) {
        const Radio::RadioTarget &target = radio->targets.at(hoveredTargetIndex);
        QPointF pos = positions[hoveredTargetIndex];
        QString info = QString("Radio: %1\nDist: %2 m\nAngle: %3°\nFreq: %4 MHz\nRange: %5 m")
                           .arg(QString::fromStdString(target.name))
                           .arg(target.radius, 0, 'f', 1)
                           .arg(target.angle, 0, 'f', 1)
                           .arg(target.frequency, 0, 'f', 1)
                           .arg(target.range, 0, 'f', 1);

        QFontMetrics fm(p.font());
        QRect infoRect = fm.boundingRect(QRect(), Qt::AlignLeft | Qt::AlignTop, info);
        infoRect.moveTo(pos.x() + 12, pos.y() - infoRect.height() / 2);
        infoRect.adjust(-6, -4, 6, 4);

        p.setBrush(QColor(0, 0, 0, 220));
        p.setPen(QPen(Qt::white, 1));
        p.drawRect(infoRect);
        p.drawText(infoRect, Qt::AlignLeft | Qt::AlignTop, info);
    }
}
void RADIODisplay::mouseMoveEvent(QMouseEvent *event)
{
    mousePos = event->pos();
    update(); // Repaint so tooltip updates as mouse moves
}

void RADIODisplay::drawTargetAndPath(QPainter &painter)
{
    int w = width();
    int h = height();
    int centerX = w/2;
    int centerY = h/2;
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;

    if (entity && radio) {
        ang = entity->transform->toEulerAngles().y();

        // Radio specific targets are now handled in drawRadioTargets
    }
}

void RADIODisplay::drawBackground(QPainter &p)
{
    p.save();
    p.fillRect(rect(), Qt::black);

    QPen pen(radarGreen, 1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QRect inner(padding, padding, width() - padding*2, height() - padding*2);
    p.drawRect(inner);
    p.restore();
}

void RADIODisplay::drawRadarRing(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen ringPen(radarGreen, 2);
    p.setPen(ringPen);
    p.setBrush(Qt::NoBrush);
    QRectF circle(center.x() - outerRadius, center.y() - outerRadius, outerRadius*2.0, outerRadius*2.0);
    p.drawEllipse(circle);
    p.restore();
}

void RADIODisplay::drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius)
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

void RADIODisplay::drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius)
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

void RADIODisplay::drawCenterMark(QPainter &p, const QPoint &center)
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

void RADIODisplay::drawTopMarker(QPainter &p, const QPoint &center, int outerRadius)
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
    QString txt = "N"; // North marker
    QRect txtRect = QFontMetrics(f).boundingRect(txt);
    int tx = triCenterX - txtRect.width()/2;
    int ty = triCenterY + triW + txtRect.height() + 1;

    p.setPen(radarGreen);
    p.drawText(tx, ty, txt);
    p.restore();
}
