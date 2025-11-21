
// /* ========================================================================= */
// /* File: IFFDisplay.cpp                                                    */
// /* Purpose: Implements IFF display for friend/foe identification           */
// /* ========================================================================= */

// #include "iffdisplay.h"
// #include <QPainter>
// #include <QPaintEvent>
// #include <QFont>
// #include <QtMath>
// #include <QDebug>
// #include <core/Debug/console.h>
// #include <core/Hierarchy/EntityProfiles/iff.h>  // IFF प्रोफाइल include करें

// // %%% Constructor %%%
// /* Initialize IFF display */
// IFFDisplay::IFFDisplay(QWidget *parent)
//     : QWidget(parent)
// {
//     setStyleSheet("background-color: black;");
//     QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//     policy.setHeightForWidth(true);
//     setMouseTracking(true);

//     setSizePolicy(policy);
//     padding = 40;
// }

// // %%% Size Management %%%
// QSize IFFDisplay::sizeHint() const
// {
//     int defaultWidth = 400;
//     return QSize(defaultWidth, heightForWidth(defaultWidth));
// }

// QSize IFFDisplay::minimumSize() const
// {
//     int minW = 250;
//     return QSize(minW, heightForWidth(minW));
// }

// int IFFDisplay::heightForWidth(int width) const
// {
//     return qRound(width * ASPECT_RATIO);
// }


// void IFFDisplay::selectEntity(Entity* entit)
// {
//     Platform* platform = dynamic_cast<Platform*>(entit);
//     if (!platform) {
//         Console::error("Entity is not a Platform");
//         return;
//     }

//     // Set entity ID and pointer
//     id = QString::fromStdString(platform->ID);
//     entity = platform;

//     // Select first valid IFF
//     iff = nullptr;
//     for (IFF* i : entity->iffList) {
//         if (i) {
//             iff = i;
//             setWindowTitle("IFF Display (" + QString::fromStdString(entity->Name) + ")");
//             qDebug() << "🎯 IFF selected for platform:" << QString::fromStdString(entity->Name)
//                      << "IFF targets count:" << iff->iffTargets.size();

//             // 🔥 CONNECT IFF SIGNALS TO THIS DISPLAY
//             connect(iff, &IFF::iffContactsUpdated, this, &IFFDisplay::updateRadar);
//             break;
//         }
//     }

//     if (!iff) {
//         qDebug() << "❌ No IFF found for platform:" << QString::fromStdString(entity->Name);
//     }
// }

// void IFFDisplay::RemoveEntity(QString ID)
// {
//     if (id == ID) {
//         entity = nullptr;
//         iff = nullptr;
//         setWindowTitle("IFF Display");
//     }
// }



// void IFFDisplay::updateRadar()
// {
//     //return;
//     if (entity && iff) {
//         setRange(iff->emittingRange * 1.0f); // km to meters conversion

//         //qDebug() << "🔄 IFFDisplay updateRadar - Entity:" << QString::fromStdString(entity->Name)
//         //       << "Targets:" << iff->iffTargets.size()
//         //     << "Range:" << range << "meters";

//         // Force repaint
//         update();
//     } else {
//         if (!entity)"";
//         //qDebug() << "❌ IFFDisplay updateRadar - No entity selected";
//         else if (!iff)"";
//         //qDebug() << "❌ IFFDisplay updateRadar - No IFF component";
//     }
// }

// // %%% Paint Event %%%
// void IFFDisplay::paintEvent(QPaintEvent * /*event*/)
// {
//     //return;
//     if (width() <= 0 || height() <= 0) return;

//     QPainter p(this);
//     p.setRenderHint(QPainter::Antialiasing);

//     drawBackground(p);

//     int w = width();
//     int h = height();
//     int outerDiameter = qMin(w - padding*2, h - padding*2);
//     int outerRadius = outerDiameter / 2;
//     QPoint center(w / 2, h / 2);

//     drawRadarRing(p, center, outerRadius);
//     drawConcentricCircles(p, center, outerRadius);
//     drawTicksAndLabels(p, center, outerRadius);
//     drawCenterMark(p, center);
//     drawTopMarker(p, center, outerRadius);
//     drawTargetAndPath(p);

//     // Draw IFF targets
//     drawIFFTargets(p, center, outerRadius);
// }

// void IFFDisplay::drawIFFTargets(QPainter &p, const QPoint &center, int outerRadius)
// {
//     if (!entity || !iff) return;
//     if (iff->iffTargets.isEmpty()) return;

//     hoveredTargetIndex = -1; // reset per frame
//     QList<QPointF> positions;
//     const int dotSize = 8;

//     for (int i = 0; i < iff->iffTargets.size(); ++i) {
//         const IFF::IFFTarget &target = iff->iffTargets.at(i);
//         // ✅ FIX: Skip drawing if target is out of radar range
//         if (target.radius > range) {
//             continue;  // Don't draw, don't clamp to border
//         }
//         // Convert polar coordinates to screen coordinates
//         float per = qBound(0.0f, target.radius / range, 1.0f);
//         float radius = outerRadius * per;
//         float angle = target.angle;
//         double theta = qDegreesToRadians(angle + 90);

//         QPointF pos(center.x() + radius * cos(theta),
//                     center.y() - radius * sin(theta));
//         positions.append(pos);

//         // Detect hover
//         QRectF dotRect(pos.x() - dotSize / 2, pos.y() - dotSize / 2, dotSize, dotSize);
//         if (dotRect.contains(mousePos)) {
//             hoveredTargetIndex = i;
//         }

//         // Draw color dot (green for friend, red for foe/unknown)
//         QColor color = (target.status == 1) ? Qt::green : Qt::red;
//         p.setBrush(color);
//         p.setPen(Qt::NoPen);
//         p.drawEllipse(dotRect);
//     }

//     // --- Draw tooltip if mouse is hovering a target ---
//     if (hoveredTargetIndex >= 0) {
//         const IFF::IFFTarget &target = iff->iffTargets.at(hoveredTargetIndex);
//         QPointF pos = positions[hoveredTargetIndex];
//         QString info = QString("ID: %1\nMode: %2\nCode: %3\nDist: %4 m\nAngle: %5°")
//                            .arg(QString::fromStdString(target.responderName.empty() ? target.responderId : target.responderName))
//                            .arg(QString::fromStdString(target.mode))
//                            .arg(QString::fromStdString(target.code))
//                            .arg(target.radius, 0, 'f', 1)
//                            .arg(target.angle, 0, 'f', 1);

//         // Draw tooltip box
//         QFontMetrics fm(p.font());
//         QRect infoRect = fm.boundingRect(QRect(), Qt::AlignLeft | Qt::AlignVCenter, info);
//         infoRect.moveTo(pos.x() + 12, pos.y() - infoRect.height() / 2);
//         infoRect.adjust(-6, -4, 6, 4);

//         p.setBrush(QColor(0, 0, 0, 180));
//         p.setPen(QPen(Qt::white, 1));
//         p.drawRect(infoRect);
//         p.drawText(infoRect, Qt::AlignLeft | Qt::AlignVCenter, info);
//     }
// }
// void IFFDisplay::mouseMoveEvent(QMouseEvent *event)
// {
//     mousePos = event->pos();
//     update(); // Repaint so tooltip updates as mouse moves
// }



// void IFFDisplay::drawTargetAndPath(QPainter &painter)
// {
//     int w = width();
//     int h = height();
//     int centerX = w/2;
//     int centerY = h/2;
//     int outerDiameter = qMin(w - padding*2, h - padding*2);
//     int outerRadius = outerDiameter / 2;

//     if (entity && iff) {
//         ang = entity->transform->toEulerAngles().y();

//         // यहाँ IFF specific targets draw करें
//         // (यह method अब drawIFFTargets में handle हो रहा है)
//     }
// }

// void IFFDisplay::drawBackground(QPainter &p)
// {
//     p.save();
//     p.fillRect(rect(), Qt::black);

//     QPen pen(radarGreen, 1);
//     p.setPen(pen);
//     p.setBrush(Qt::NoBrush);
//     QRect inner(padding, padding, width() - padding*2, height() - padding*2);
//     p.drawRect(inner);
//     p.restore();
// }

// void IFFDisplay::drawRadarRing(QPainter &p, const QPoint &center, int outerRadius)
// {
//     p.save();
//     QPen ringPen(radarGreen, 2);
//     p.setPen(ringPen);
//     p.setBrush(Qt::NoBrush);
//     QRectF circle(center.x() - outerRadius, center.y() - outerRadius, outerRadius*2.0, outerRadius*2.0);
//     p.drawEllipse(circle);
//     p.restore();
// }

// void IFFDisplay::drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius)
// {
//     p.save();
//     QPen pen(radarGreen, 1, Qt::DashLine);
//     pen.setCosmetic(true);
//     p.setPen(pen);
//     p.setBrush(Qt::NoBrush);

//     for (int i = 1; i <= ringCount; ++i) {
//         double r = outerRadius * (double(i) / double(ringCount + 1));
//         QRectF ring(center.x() - r, center.y() - r, r * 2.0, r * 2.0);
//         p.drawEllipse(ring);

//         // // Range labels
//         // float rangeVal = (range * i) / (ringCount + 1);
//         // p.drawText(center.x() + r - 10, center.y() - r + 12,
//         //            QString("%1m").arg(rangeVal, 0, 'f', 0));
//     }
//     p.restore();
// }

// void IFFDisplay::drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius)
// {
//     p.save();
//     QPen majorPen(radarGreen, 2);
//     QPen minorPen(radarGreen, 1);
//     QFont labelFont("Arial", qMax(8, outerRadius/18), QFont::Bold);
//     p.setFont(labelFont);

//     for (int deg = 0; deg < 360; deg += (majorTickEvery / minorTicksPerMajor)) {
//         double angleDeg = deg - 90.0 + ang;
//         double theta = qDegreesToRadians(angleDeg);
//         bool isMajor = (deg % majorTickEvery == 0);

//         int tickOut = outerRadius;
//         int tickIn = isMajor ? outerRadius - qMax(16, outerRadius/12) : outerRadius - qMax(6, outerRadius/24);

//         int x1 = center.x() + int(tickOut * cos(theta));
//         int y1 = center.y() + int(tickOut * sin(theta));
//         int x2 = center.x() + int(tickIn * cos(theta));
//         int y2 = center.y() + int(tickIn * sin(theta));

//         p.setPen(isMajor ? majorPen : minorPen);
//         p.drawLine(QPoint(x1, y1), QPoint(x2, y2));

//         if (isMajor) {
//             int labelRadius = outerRadius + qMax(16, outerRadius/10);
//             int lx = center.x() + int(labelRadius * cos(theta));
//             int ly = center.y() + int(labelRadius * sin(theta));

//             int heading = (deg % 360);
//             QString text = QString("%1").arg(heading, 3, 10, QChar('0'));
//             QRect textRect = QFontMetrics(p.font()).boundingRect(text);

//             int tx = lx - textRect.width()/2;
//             int ty = ly + textRect.height()/2;

//             p.setPen(radarGreen);
//             p.drawText(tx, ty, text);
//         }
//     }
//     p.restore();
// }

// void IFFDisplay::drawCenterMark(QPainter &p, const QPoint &center)
// {
//     p.save();
//     p.setPen(QPen(radarGreen, 2));
//     int cross = qMax(6, width()/80);
//     p.drawLine(center.x() - cross, center.y(), center.x() + cross, center.y());
//     p.drawLine(center.x(), center.y() - cross, center.x(), center.y() + cross);

//     QRect sq(center.x() - 5, center.y() - 5, 10, 10);
//     p.setBrush(radarGreen);
//     p.drawRect(sq);
//     p.restore();
// }

// void IFFDisplay::drawTopMarker(QPainter &p, const QPoint &center, int outerRadius)
// {
//     p.save();
//     double theta = qDegreesToRadians(-90.0);
//     int triCenterX = center.x() + int((outerRadius + 4) * cos(theta));
//     int triCenterY = center.y() + int((outerRadius + 4) * sin(theta));

//     int triW = qMax(6, outerRadius/20);
//     QPoint pts[3] = {
//         QPoint(triCenterX, triCenterY - triW),
//         QPoint(triCenterX - triW, triCenterY + (triW/2)),
//         QPoint(triCenterX + triW, triCenterY + (triW/2))
//     };

//     p.setPen(QPen(radarGreen, 1));
//     p.setBrush(radarGreen);
//     p.drawPolygon(pts, 3);

//     QFont f("Arial", qMax(6, outerRadius/25), QFont::Bold);
//     p.setFont(f);
//     QString txt = "N"; // North marker
//     QRect txtRect = QFontMetrics(f).boundingRect(txt);
//     int tx = triCenterX - txtRect.width()/2;
//     int ty = triCenterY + triW + txtRect.height() + 1;

//     p.setPen(radarGreen);
//     p.drawText(tx, ty, txt);
//     p.restore();
// }



///========================================


/* ========================================================================= */
/* File: IFFDisplay.cpp                                                    */
/* Purpose: Implements IFF display for friend/foe identification           */
/* ========================================================================= */

#include "iffdisplay.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <QtMath>
#include <QDebug>
#include <core/Debug/console.h>
#include <core/Hierarchy/EntityProfiles/iff.h>  // IFF प्रोफाइल include करें

// %%% Constructor %%%
/* Initialize IFF display */
IFFDisplay::IFFDisplay(QWidget *parent)
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
QSize IFFDisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

QSize IFFDisplay::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

int IFFDisplay::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}


void IFFDisplay::selectEntity(Entity* entit)
{
    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;

    // Select first valid IFF
    iff = nullptr;
    for (IFF* i : entity->iffList) {
        if (i) {
            iff = i;
            setWindowTitle("IFF Display (" + QString::fromStdString(entity->Name) + ")");
            qDebug() << "🎯 IFF selected for platform:" << QString::fromStdString(entity->Name)
                     << "IFF targets count:" << iff->iffTargets.size();

            // 🔥 CONNECT IFF SIGNALS TO THIS DISPLAY
            connect(iff, &IFF::iffContactsUpdated, this, &IFFDisplay::updateRadar);
            break;
        }
    }

    if (!iff) {
        qDebug() << "❌ No IFF found for platform:" << QString::fromStdString(entity->Name);
    }
}

void IFFDisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        entity = nullptr;
        iff = nullptr;
        setWindowTitle("IFF Display");
    }
}



void IFFDisplay::updateRadar()
{
    //return;
    if (entity && iff) {
        setRange(iff->emittingRange * 1.0f); // km to meters conversion

        //qDebug() << "🔄 IFFDisplay updateRadar - Entity:" << QString::fromStdString(entity->Name)
        //       << "Targets:" << iff->iffTargets.size()
        //     << "Range:" << range << "meters";

        // Force repaint
        update();
    } else {
        if (!entity)"";
        //qDebug() << "❌ IFFDisplay updateRadar - No entity selected";
        else if (!iff)"";
        //qDebug() << "❌ IFFDisplay updateRadar - No IFF component";
    }
}

// %%% Paint Event %%%
void IFFDisplay::paintEvent(QPaintEvent * /*event*/)
{
    //return;
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

    // Draw IFF targets
    drawIFFTargets(p, center, outerRadius);
}

void IFFDisplay::drawIFFTargets(QPainter &p, const QPoint &center, int outerRadius)
{
    if (!entity || !iff) return;
    if (iff->iffTargets.isEmpty()) return;

    hoveredTargetIndex = -1; // reset per frame
    QList<QPointF> positions;
    QList<int> drawnIndices; // map drawn dot index → iffTargets index
    const int dotSize = 8;

    for (int i = 0; i < iff->iffTargets.size(); ++i) {
        const IFF::IFFTarget &target = iff->iffTargets.at(i);
        // ✅ FIX: Skip drawing if target is out of radar range
        if (target.radius > range) {
            continue;  // Don't draw, don't clamp to border
        }
        // Convert polar coordinates to screen coordinates
        float per = qBound(0.0f, target.radius / range, 1.0f);
        float radius = outerRadius * per;
        float angle = target.angle;
        double theta = qDegreesToRadians(angle + 90);

        QPointF pos(center.x() + radius * cos(theta),
                    center.y() - radius * sin(theta));
        positions.append(pos);
        drawnIndices.append(i);

        // Detect hover
        QRectF dotRect(pos.x() - dotSize / 2, pos.y() - dotSize / 2, dotSize, dotSize);
        if (dotRect.contains(mousePos)) {
            hoveredTargetIndex = drawnIndices.size() - 1;
        }

        // Draw color dot (green for friend, red for foe/unknown)
        QColor color = (target.status == 1) ? Qt::green : Qt::red;
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(dotRect);
    }

    // --- Draw tooltip if mouse is hovering a target ---
    if (hoveredTargetIndex >= 0 && hoveredTargetIndex < drawnIndices.size() && hoveredTargetIndex < positions.size()) {
        const IFF::IFFTarget &target = iff->iffTargets.at(drawnIndices[hoveredTargetIndex]);
        QPointF pos = positions[hoveredTargetIndex];
        QString info = QString("ID: %1\nMode: %2\nCode: %3\nDist: %4 m\nAngle: %5°")
                           .arg(QString::fromStdString(target.responderName.empty() ? target.responderId : target.responderName))
                           .arg(QString::fromStdString(target.mode))
                           .arg(QString::fromStdString(target.code))
                           .arg(target.radius, 0, 'f', 1)
                           .arg(target.angle, 0, 'f', 1);

        // Draw tooltip box
        QFontMetrics fm(p.font());
        QRect infoRect = fm.boundingRect(QRect(), Qt::AlignLeft | Qt::AlignVCenter, info);
        infoRect.moveTo(pos.x() + 12, pos.y() - infoRect.height() / 2);
        infoRect.adjust(-6, -4, 6, 4);

        p.setBrush(QColor(0, 0, 0, 180));
        p.setPen(QPen(Qt::white, 1));
        p.drawRect(infoRect);
        p.drawText(infoRect, Qt::AlignLeft | Qt::AlignVCenter, info);
    }
}
void IFFDisplay::mouseMoveEvent(QMouseEvent *event)
{
    mousePos = event->pos();
    update(); // Repaint so tooltip updates as mouse moves
}



void IFFDisplay::drawTargetAndPath(QPainter &painter)
{
    int w = width();
    int h = height();
    int centerX = w/2;
    int centerY = h/2;
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;

    if (entity && iff) {
        ang = entity->transform->toEulerAngles().y();

        // यहाँ IFF specific targets draw करें
        // (यह method अब drawIFFTargets में handle हो रहा है)
    }
}

void IFFDisplay::drawBackground(QPainter &p)
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

void IFFDisplay::drawRadarRing(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen ringPen(radarGreen, 2);
    p.setPen(ringPen);
    p.setBrush(Qt::NoBrush);
    QRectF circle(center.x() - outerRadius, center.y() - outerRadius, outerRadius*2.0, outerRadius*2.0);
    p.drawEllipse(circle);
    p.restore();
}

void IFFDisplay::drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius)
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

        // // Range labels
        // float rangeVal = (range * i) / (ringCount + 1);
        // p.drawText(center.x() + r - 10, center.y() - r + 12,
        //            QString("%1m").arg(rangeVal, 0, 'f', 0));
    }
    p.restore();
}

void IFFDisplay::drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius)
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

void IFFDisplay::drawCenterMark(QPainter &p, const QPoint &center)
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

void IFFDisplay::drawTopMarker(QPainter &p, const QPoint &center, int outerRadius)
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
