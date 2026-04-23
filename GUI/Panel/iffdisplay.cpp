//============================================================================
// FILE:         iffdisplay.cpp
// MODULE:       IFF (Identification Friend or Foe) Display
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen 2 Innovation (O2I).
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements the IFFDisplay class which provides a widget for
//               visualising Identification Friend or Foe (IFF) interrogation
//               responses. It displays detected IFF responders in a polar
//               (radar‑like) format with configurable range, rings, ticks,
//               and hover detection. Supports display of responder ID, name,
//               mode, code, and status. Integrates with Hierarchy and IFF/
//               Platform entities for real‑time tracking.
//
// REQUIREMENTS: Implements REQ-IFF-010 through REQ-IFF-017
//
// AUTHOR:       Arti Rajpoot
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-IFF-001
//
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
//               Restricted circulation — defence simulation use only.
//============================================================================

#include "iffdisplay.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <QtMath>
#include <QDebug>
#include <core/Debug/console.h>
#include <core/Hierarchy/EntityProfiles/iff.h>
#include <QComboBox>

// %%% Constructor %%%
/* Initialize IFF display */
IFFDisplay::IFFDisplay(QWidget *parent)
    : QWidget(parent), hoveredTargetIndex(-1)
{
    setStyleSheet("background-color: black;");
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setMouseTracking(true);
    setSizePolicy(policy);
    padding = 40;
    iffDropdown = new QComboBox(this);
    iffDropdown->setStyleSheet(
        "QComboBox { background-color: #001a00; color: #00ff00; "
        "border: 1px solid #00ff00; font-size: 10px; padding: 2px; }"
        "QComboBox QAbstractItemView { background-color: #001a00; "
        "color: #00ff00; selection-background-color: #003300; }"
        );
    iffDropdown->hide();
    connect(iffDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &IFFDisplay::onIFFSelected);
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


void IFFDisplay::mouseMoveEvent(QMouseEvent *event)
{
        return;
    mousePos = event->pos();

    if (iff->targets.isEmpty()) {
        hoveredTargetIndex = -1;
        update();
        return;
    }

    int w = width();
    int h = height();
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    QPoint center(w / 2, h / 2);


    int closestIndex = -1;
    double minDistance = 15.0;

    int i=0;
    for (const IFF::IFFTarget &t : iff->targets) {


        double per = t.radius / range;
        if (per < 0.0) per = 0.0;
        if (per > 1.0) per = 1.0;

        double r = per * outerRadius;
        double angleDeg = t.angle - 90;
        double theta = qDegreesToRadians(angleDeg);
        int tx = center.x() + int(r * cos(theta));
        int ty = center.y() + int(r * sin(theta));


        double dx = mousePos.x() - tx;
        double dy = mousePos.y() - ty;
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
void IFFDisplay::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hoveredTargetIndex = -1;
    update();
    QWidget::leaveEvent(event);
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
    ifflist.clear();
    for (auto const& pair :  *entity->iffs->iffs) {
        IFF* i = pair.second;
        if (i) {
            if(iff==nullptr){
                iff = i;
            }
            ifflist.append(i);
            setWindowTitle("IFF Display (" + QString::fromStdString(entity->Name) + ")");

        }
    }
    if (!iff) {
    }
    hoveredTargetIndex = -1;
    updateDropdown();

    update();
}

void IFFDisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        entity = nullptr;
        iff = nullptr;
        ifflist.clear();
        setWindowTitle("IFF Display");
        // Reset hover state
        hoveredTargetIndex = -1;
        if (iffDropdown) iffDropdown->hide();

    }
}

void IFFDisplay::updateRadar()
{
    if (entity && iff) {
        setRange(iff->emittingRange * 1.0f);
        // targets = iff->targets;
        update();
    } else {

        targets.clear();
        hoveredTargetIndex = -1;
    }
}

void IFFDisplay::paintEvent(QPaintEvent * /*event*/)
{
    if (width() <= 0 || height() <= 0) return;

    QPainter p(this);


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

    if(!iff)return;
    // Draw targets with hover functionality
    if (!iff->targets.isEmpty()) {
        int i=0;
        for (const IFF::IFFTarget &t : iff->targets) {

            double per = t.radius / range;
            if (per < 0.0) per = 0.0;
            if (per > 1.0) per = 1.0;

            double r = per * outerRadius;
            double angleDeg = t.angle - 90;
            double theta = qDegreesToRadians(angleDeg);
            int tx = center.x() + int(r * cos(theta));
            int ty = center.y() + int(r * sin(theta));

            // Draw dotted line from center to target
            p.setPen(QPen(Qt::yellow, 1, Qt::DotLine));
            p.drawLine(center, QPoint(tx, ty));

            // Draw target dot with color based on friend/foe status
            if (i == hoveredTargetIndex) {
                // Highlight hovered target
                p.setPen(QPen(Qt::white, 2));
                p.setBrush(Qt::white);
                p.drawEllipse(QPointF(tx, ty), 6, 6);
            }

            // Draw colored dot (non-transparent)
            if (t.status == 1 || t.ally) {
                // Friend/ally - solid green
                p.setBrush(Qt::blue);
                p.setPen(QPen(Qt::blue, 1));
            } else {
                // Foe/unknown - solid red
                p.setBrush(QBrush(QColor(255, 0, 0, 255)));
                p.setPen(QPen(Qt::red, 1));
            }

            p.drawEllipse(QPointF(tx, ty), 4, 4);


            if (i == hoveredTargetIndex) {
                p.setPen(QPen(Qt::cyan, 1));
                QFont font = p.font();
                font.setPointSize(9);
                font.setBold(true);
                p.setFont(font);

                Platform* targetPlatform = dynamic_cast<Platform*>(t.entity);
                QString targetName = targetPlatform ? QString::fromStdString(targetPlatform->Name) : "Unknown";

                // Determine friend/foe status text
                QString statusText = (t.status == 1 || t.ally) ? "Friend" :
                                         (t.status == 0 ? "Unknown" : "Foe");

                // Show detailed info for hovered target
                QString angleText = QString("Angle: %1°").arg(t.angle, 0, 'f', 1);
                QString distText = QString("Dist: %1 km").arg(t.radius, 0, 'f', 1);
                QString statusLabel = QString("Status: %1").arg(statusText);
                QString nameText = QString("Name: %1").arg(targetName);

                // Draw text background for better visibility
                p.setBrush(QColor(0, 0, 0, 180));
                p.setPen(Qt::NoPen);

                // Calculate text positions
                QFontMetrics fm(p.font());
                int textWidth = fm.horizontalAdvance(nameText) + 8;
                int textHeight = 80;
                p.drawRect(tx + 8, ty - 40, textWidth, textHeight);

                // Draw text
                p.setPen(QPen(Qt::cyan, 1));
                p.drawText(tx + 12, ty - 28, angleText);
                p.drawText(tx + 12, ty - 16, distText);
                p.drawText(tx + 12, ty - 4, statusLabel);
                p.drawText(tx + 12, ty + 8, nameText);

                // Additional IFF info if available
                if (!t.mode.empty() || !t.code.empty()) {
                    QString modeText = QString("Mode: %1").arg(QString::fromStdString(t.mode));
                    QString codeText = QString("Code: %1").arg(QString::fromStdString(t.code));
                    p.drawText(tx + 12, ty + 20, modeText);
                    p.drawText(tx + 12, ty + 32, codeText);
                }
            }
            i++;
        }
    } else if (entity && iff) {
        // No targets message
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPointSize(10);
        p.setFont(font);
        QRect textRect = p.fontMetrics().boundingRect("No IFF Targets Detected");
        p.drawText(center.x() - textRect.width()/2, center.y(), "No IFF Targets Detected");
    }
}
void IFFDisplay::drawIFFTargets(QPainter &p, const QPoint &center, int outerRadius)
{
    if (!entity || !iff) return;
    if (iff->iffTargets.isEmpty()) return;

    QList<QPointF> positions;
    QList<int> drawnIndices;
    const int dotSize = 8;

    for (int i = 0; i < iff->iffTargets.size(); ++i) {
        const IFF::IFFTarget &target = iff->iffTargets.at(i);

        if (target.radius > range) {
            continue;
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
        QColor color = (target.status == 1) ? Qt::blue : Qt::red;
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(dotRect);

        // Draw label only for hovered target
        if (i == hoveredTargetIndex) {
            p.setPen(QPen(Qt::yellow, 1));
            QString info = QString("ID: %1").arg(QString::fromStdString(
                target.responderName.empty() ? target.responderId : target.responderName));
            p.drawText(pos.x() + 5, pos.y() - 5, info);
        }
    }
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
    QString txt = "N";
    QRect txtRect = QFontMetrics(f).boundingRect(txt);
    int tx = triCenterX - txtRect.width()/2;
    int ty = triCenterY + triW + txtRect.height() + 1;

    p.setPen(radarGreen);
    p.drawText(tx, ty, txt);
    p.restore();
}
void IFFDisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (iffDropdown) {
        int dropW = 120;
        int dropH = 22;
        iffDropdown->setGeometry(width() - dropW - padding, 4, dropW, dropH);
    }
}
void IFFDisplay::updateDropdown()
{
    if (!iffDropdown) return;

    QSignalBlocker blocker(iffDropdown);
    iffDropdown->clear();

    if (ifflist.isEmpty()) {
        iffDropdown->hide();
        return;
    }

    for (int i = 0; i < ifflist.size(); ++i) {
        IFF* f = ifflist[i];
        QString name = f ? QString::fromStdString(f->Name) : QString("IFF %1").arg(i + 1);
        if (name.trimmed().isEmpty())
            name = QString("IFF %1").arg(i + 1);
        iffDropdown->addItem(name);
    }

    int currentIdx = ifflist.indexOf(iff);
    if (currentIdx >= 0)
        iffDropdown->setCurrentIndex(currentIdx);

    if (ifflist.size() > 1)
        iffDropdown->show();
    else
        iffDropdown->hide();

    int dropW = 120;
    int dropH = 22;
    iffDropdown->setGeometry(width() - dropW - padding, 4, dropW, dropH);
}
void IFFDisplay::onIFFSelected(int index)
{
    if (index < 0 || index >= ifflist.size()) return;
    iff = ifflist[index];
    hoveredTargetIndex = -1;
    if (iff)
        setRange(iff->emittingRange * 1.0f);
    update();
}
