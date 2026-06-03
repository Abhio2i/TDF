/* =============================================================================
 * FILE:         RADIODisplay.cpp
 * MODULE:       Radio Communication Display
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the RADIODisplay class which provides a widget for
 *               visualising radio communication links and detected radio
 *               emissions. It displays radio targets in a polar (radar‑like)
 *               format with configurable range, rings, ticks, and hover
 *               detection. Integrates with Hierarchy and Radio/Platform
 *               entities for real‑time tracking and display updates.
 *
 * REQUIREMENTS: Implements REQ-RADIO-010 through REQ-RADIO-017
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-RADIO-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "radiodisplay.h"                          // For RADIO display class
#include "core/Hierarchy/Utils/entityutils.h"      // For entity utilities
#include <QPainter>                                // For painting
#include <QPaintEvent>                             // For paint events
#include <QFont>                                   // For font handling
#include <QtMath>                                  // For math functions
#include <QDebug>                                  // For debugging
#include <core/Debug/console.h>                    // For console output
#include <core/Hierarchy/EntityProfiles/radio.h>   // For radio entity

// %%% Constructor %%%
/* Initialize Radio display with default settings */
RADIODisplay::RADIODisplay(QWidget *parent)
    : QWidget(parent), hoveredTargetIndex(-1)
{
    // Set black background
    setStyleSheet("background-color: black;");

    // Configure size policy with aspect ratio
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);

    // Enable mouse tracking for hover effects
    setMouseTracking(true);
    setSizePolicy(policy);
    padding = 40;
    radioDropdown = new QComboBox(this);
    radioDropdown->setStyleSheet(
        "QComboBox { background-color: #001a00; color: #00ff00; "
        "border: 1px solid #00ff00; font-size: 10px; padding: 2px; }"
        "QComboBox QAbstractItemView { background-color: #001a00; "
        "color: #00ff00; selection-background-color: #003300; }"
        );
    radioDropdown->hide(); // Pehle chhupa do, entity select hone par dikhao
    connect(radioDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RADIODisplay::onRadioSelected);
}

// %%% Size Management %%%
/* Return preferred widget size */
QSize RADIODisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

/* Return minimum widget size */
QSize RADIODisplay::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

/* Calculate height based on width to maintain aspect ratio */
int RADIODisplay::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

// %%% Mouse Interaction %%%
/* Handle mouse move events for hover detection */
void RADIODisplay::mouseMoveEvent(QMouseEvent *event)
{
        return;
    // Store current mouse position
    mousePos = event->pos();

    // If no targets, clear hover state
    if (radio->targets.isEmpty()) {
        hoveredTargetIndex = -1;
        update();
        return;
    }

    // Calculate display dimensions
    int w = width();
    int h = height();
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    QPoint center(w / 2, h / 2);

    // Find closest target to mouse position
    int closestIndex = -1;
    double minDistance = 20.0;
    int i=0;
    for (const Radio::RadioTarget &t : radio->targets) {
        double per = t.radius / range;
        if (per < 0.0) per = 0.0;
        if (per > 1.0) per = 1.0;

        double r = per * outerRadius;
        double angleDeg = t.angle;
        double theta = qDegreesToRadians(angleDeg - 90.0);
        int tx = center.x() + int(r * cos(theta));
        int ty = center.y() + int(r * sin(theta));

        // Calculate distance from mouse to target
        double dx = mousePos.x() - tx;
        double dy = mousePos.y() - ty;
        double distance = sqrt(dx*dx + dy*dy);

        // Update closest target if within threshold
        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
        i++;
    }

    // Update hover state if changed
    if (hoveredTargetIndex != closestIndex) {
        hoveredTargetIndex = closestIndex;
        update();
    }

    QWidget::mouseMoveEvent(event);
}

/* Handle mouse leave events to clear hover state */
void RADIODisplay::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hoveredTargetIndex = -1;
    update();
    QWidget::leaveEvent(event);
}

// %%% Entity Management %%%
/* Select entity for radio display */
void RADIODisplay::selectEntity(Entity* entit)
{
    // Cast to Platform type
    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        update();
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;

    // Select first valid Radio from entity
    radio = nullptr;
    radiolist.clear();
    if (!entity->radios || !entity->radios->radios) {
        update();
        return;
    }
    for (auto const& pair :  *entity->radios->radios) {
        Radio* r = pair.second;
        if (r) {
            if(radio==nullptr){
                radio = r;
            }
            radiolist.append(r);
            setWindowTitle("RADIO Display (" + QString::fromStdString(entity->Name) + ")");

        }
    }

    // Reset hover state when entity changes
    hoveredTargetIndex = -1;
    updateDropdown();
    update();
}

/* Remove entity from display */
void RADIODisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        entity = nullptr;
        radio = nullptr;
            radiolist.clear();
        setWindowTitle("RADIO Display");
        // Reset hover state
        hoveredTargetIndex = -1;
        if (radioDropdown) radioDropdown->hide();

    }
}

// %%% Update Methods %%%
/* Update radar display with current radio data */
void RADIODisplay::updateRadar()
{
    if (entity && radio) {
        setRange(radio->Range);
        // targets = radio->targets;
        update();
    } else {
        // Clear targets if no radio available
        targets.clear();
        hoveredTargetIndex = -1;
    }
}

// %%% Paint Event %%%
/* Main paint event for drawing the radio display */
void RADIODisplay::paintEvent(QPaintEvent * /*event*/)
{
    if (width() <= 0 || height() <= 0) return;

    QPainter p(this);

    // Draw all display components
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

    if(!radio)return;
    // Draw targets with hover functionality
    if (!radio->targets.isEmpty()) {
        int i=0;
        for (const Radio::RadioTarget &t : radio->targets) {
            // const Target &t = targets[i];

            // Calculate target position
            double per = t.radius / range;
            if (per < 0.0) per = 0.0;
            if (per > 1.0) per = 1.0;

            double r = per * outerRadius;
            double angleDeg = t.angle-90;
            double theta = qDegreesToRadians(angleDeg);
            int tx = center.x() + int(r * cos(theta));
            int ty = center.y() + int(r * sin(theta));

            // Draw line from center to target
            p.setPen(QPen(radarGreen, 1, Qt::DotLine));
            p.drawLine(center, QPoint(tx, ty));

            // Set color based on hover state
            if (i == hoveredTargetIndex) {
                p.setBrush(Qt::cyan);
            } else {
                p.setBrush(Qt::red);
            }

            // Draw target dot
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(tx, ty), 4, 4);

            // Draw labels ONLY if this target is hovered
            if (i == hoveredTargetIndex) {
                p.setPen(QPen(Qt::yellow, 1));
                QFont font = p.font();
                font.setPointSize(8);
                p.setFont(font);

                Platform* targetPlatform = dynamic_cast<Platform*>(t.entity);
                QString targetName = targetPlatform ? QString::fromStdString(targetPlatform->Name) : "Unknown";

                // Show detailed info for hovered target
                QString angleText = QString("A:%1°").arg(angleDeg, 0, 'f', 1);
                QString distText = QString("D:%1m").arg(t.radius, 0, 'f', 1);
                QString freqText = QString("F:%1MHz").arg(t.frequency, 0, 'f', 1);
                QString nameText = QString("N:%1").arg(targetName);

                // Draw text at target position (offset slightly)
                p.drawText(tx + 6, ty - 24, angleText);
                p.drawText(tx + 6, ty - 12, distText);
                p.drawText(tx + 6, ty + 0, freqText);
                p.drawText(tx + 6, ty + 12, nameText);
            }
            i++;
        }
    } else if (entity && radio) {
        // Display message when no targets detected
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPointSize(10);
        p.setFont(font);
        QRect textRect = p.fontMetrics().boundingRect("No Radio Targets Detected");
        p.drawText(center.x() - textRect.width()/2, center.y(), "No Radio Targets Detected");
    }
}

// %%% Drawing Methods %%%
/* Draw radio targets with frequency filtering */
void RADIODisplay::drawRadioTargets(QPainter &p, const QPoint &center, int outerRadius)
{

}

/* Draw entity direction and path */
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
    }
}

/* Draw black background with border */
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

/* Draw main radar ring */
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

/* Draw concentric range circles */
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

/* Draw angle ticks and labels */
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

/* Draw center cross mark */
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

/* Draw North direction marker */
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
    QString txt = "N";
    QRect txtRect = QFontMetrics(f).boundingRect(txt);
    int tx = triCenterX - txtRect.width()/2;
    int ty = triCenterY + triW + txtRect.height() + 1;

    p.setPen(radarGreen);
    p.drawText(tx, ty, txt);
    p.restore();
}
void RADIODisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (radioDropdown) {
        int dropW = 120;
        int dropH = 22;
        radioDropdown->setGeometry(width() - dropW - padding, 4, dropW, dropH);
    }
}
void RADIODisplay::updateDropdown()
{
    if (!radioDropdown) return;

    QSignalBlocker blocker(radioDropdown);
    radioDropdown->clear();

    if (radiolist.isEmpty()) {
        radioDropdown->hide();
        return;
    }

    for (int i = 0; i < radiolist.size(); ++i) {
        Radio* r = radiolist[i];
        QString name = r ? QString::fromStdString(r->Name) : QString("Radio %1").arg(i + 1);
        if (name.trimmed().isEmpty())
            name = QString("Radio %1").arg(i + 1);
        radioDropdown->addItem(name);
    }

    int currentIdx = radiolist.indexOf(radio);
    if (currentIdx >= 0)
        radioDropdown->setCurrentIndex(currentIdx);

    if (radiolist.size() > 1)
        radioDropdown->show();
    else
        radioDropdown->hide();

    int dropW = 120;
    int dropH = 22;
    // Green border ke UPAR
    radioDropdown->setGeometry(width() - dropW - padding, 4, dropW, dropH);
}void RADIODisplay::onRadioSelected(int index)
{
    if (index < 0 || index >= radiolist.size()) return;
    radio = radiolist[index];
    hoveredTargetIndex = -1;
    if (radio)
        setRange(radio->Range);
    update();
}
