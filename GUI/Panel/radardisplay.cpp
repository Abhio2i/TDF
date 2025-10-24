/* ========================================================================= */
/* File: radardisplay.cpp                                                 */
/* Purpose: Implements radar display widget for visualization              */
/* ========================================================================= */

#include "radardisplay.h"                          // For radar display class
#include <cmath>                                   // For math functions
#include <QDebug>                                  // For debug output
#include <QJsonParseError>                         // For JSON parsing errors
#include <QHBoxLayout>                             // For horizontal layout
#include <QPainter>                                // For painting operations
#include <core/Debug/console.h>                    // For console error logging

// Define aspect ratio constant
constexpr double ASPECT_RATIO = 16.0 / 9.0;

// %%% Size Management %%%
/* Calculate height based on width and aspect ratio */
int RadarDisplay::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

/* Provide size hint for widget */
QSize RadarDisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

/* Provide minimum size for widget */
QSize RadarDisplay::minimumSize() const
{
    int minWidth = 90;
    return QSize(minWidth, heightForWidth(minWidth));
}

// %%% Constructor %%%
/* Initialize radar display widget */
RadarDisplay::RadarDisplay(QWidget *parent)
    : QWidget(parent)
{
    // Set background color
    setStyleSheet("background-color: black;");
    // Set size policy with aspect ratio
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setWindowTitle("Radar Display");
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    // Initialize test targets
    for (int i = 1; i < 10; i++) {
        Target target;
        target.angle = 9 * i;
        target.radius = 10 * i;
        targets.append(target);
    }
}

// %%% Entity Management %%%
/* Select and configure entity for display */
void RadarDisplay::selectEntity(Entity* entit)
{
    // Cast entity to Platform
    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        return;
    }
    qDebug() << "csdvfdsagdsb";
    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;
    // Select first valid sensor
    for (Sensor* s : entity->sensorList) {
        if (s) {
            sensor = s;
            // Set window title with platform name
            setWindowTitle("Radar Display (" + QString::fromStdString(entity->Name) + ")");
            qDebug() << "csdvfyjkygj";
            break;
        }
    }
}

/* Remove entity if ID matches */
void RadarDisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        // Clear entity and sensor
        entity = nullptr;
        sensor = nullptr;
        // Reset window title
        setWindowTitle("Radar Display");
    }
}

// %%% Update Methods %%%
/* Update radar display data */
void RadarDisplay::updateRadar()
{
    if (entity && sensor) {
        // Set radar range and azimuth
        setRange(sensor->range);
        setAzimuth(sensor->maxDetectionAngle);
        // Trigger repaint
        update();
    }
}

/* Update display from JSON data */
void RadarDisplay::updateFromJson(const QJsonObject &json)
{
    // Update range
    if (json.contains("range") && json["range"].isDouble()) {
        range = json["range"].toInt();
    }
    // Update azimuth
    if (json.contains("azimuth") && json["azimuth"].isDouble()) {
        azimuth = json["azimuth"].toDouble();
    }
    // Update bar
    if (json.contains("bar") && json["bar"].isDouble()) {
        bar = json["bar"].toDouble();
    }
    // Update current speed
    if (json.contains("current_speed") && json["current_speed"].isDouble()) {
        current_speed = json["current_speed"].toInt();
    }
    // Update max speed
    if (json.contains("max_speed") && json["max_speed"].isDouble()) {
        max_speed = json["max_speed"].toInt();
    }
    // Update radar height
    if (json.contains("height") && json["height"].isDouble()) {
        radar_height = json["height"].toInt();
    }
    // Update targets
    if (json.contains("targets") && json["targets"].isArray()) {
        targets.clear();
        QJsonArray targetArray = json["targets"].toArray();
        for (const QJsonValue &value : targetArray) {
            QJsonObject targetObj = value.toObject();
            if (targetObj.contains("angle") && targetObj.contains("radius")) {
                Target target;
                target.angle = targetObj["angle"].toDouble();
                target.radius = targetObj["radius"].toDouble();
                targets.append(target);
            }
        }
    }
    // Trigger repaint
    update();
}

// %%% Drawing Methods %%%
/* Draw display background */
void RadarDisplay::drawBackground(QPainter &painter)
{
    painter.setBrush(Qt::black);
    // Fill background
    painter.drawRect(rect());
    painter.setPen(QPen(Qt::green, 1));
    painter.setBrush(Qt::black);
    // Draw border
    painter.drawRect(30, 30, QWidget::width() - 60, QWidget::height() - 60);
}

/* Set azimuth value */
void RadarDisplay::setAzimuth(float value)
{
    azimuth = value;
}

/* Set range value */
void RadarDisplay::setRange(float value)
{
    range = value;
}

/* Draw concentric radar circles */
void RadarDisplay::drawConcentricCircles(QPainter &painter, int centerX, int centerY, int radius)
{
    painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
    painter.setBrush(Qt::transparent);
    int panelhigh = QWidget::height() - 60;
    int offset = 10;
    float num = (range < offset ? offset : range) / 10;
    // Draw circles based on range
    for (float i = 1; i <= num; i++) {
        float per = (i * offset) / range;
        float radiu = panelhigh * per;
        painter.drawEllipse(centerX - radiu, centerY - radiu, radiu * 2, radiu * 2);
    }
}

/* Draw vertical center line */
void RadarDisplay::drawVerticalLine(QPainter &painter, int centerX, int centerY, int radius)
{
    painter.setPen(QPen(Qt::green, 1, Qt::DashLine));
    painter.drawLine(centerX, centerY, centerX, 30);
}

/* Draw center square */
void RadarDisplay::drawCenterSquare(QPainter &painter, int centerX, int centerY)
{
    painter.setBrush(Qt::white);
    painter.drawRect(centerX - 5, centerY - 5, 10, 10);
}

/* Draw radar annotations */
void RadarDisplay::drawAnnotations(QPainter &painter, int centerX, int widgetHeight)
{
    Q_UNUSED(centerX);
    Q_UNUSED(widgetHeight);
    painter.setPen(QPen(Qt::green, 1));
    painter.setFont(QFont("Arial", 9));
    // Draw mode annotations
    int width = QWidget::width();
    int spacing = width / 6;
    painter.drawText(spacing * 1 - 20, 15, "CRM");
    painter.drawText(spacing * 2 - 20, 15, "ACM");
    painter.drawText(spacing * 3 - 20, 15, "TWS");
    painter.drawText(spacing * 4 - 20, 15, "ATA");
    painter.drawText(spacing * 5 - 20, 15, "AAST");
    // Draw range
    painter.drawText(5, 30, QString("%1").arg(range));
    // Draw azimuth
    painter.drawText(5, 45, QString("%1 A").arg(azimuth));
}

/* Draw azimuth lines */
void RadarDisplay::drawAzimuth(QPainter &painter, int centerX, int centerY, int radius)
{
    radius *= 2;
    painter.setPen(QPen(Qt::blue, 2));
    double azimuthRad = 90 * M_PI / 180;
    double halfBeamWidth = azimuth / 2.0;
    double leftAngle = azimuthRad - (halfBeamWidth * M_PI / 180);
    double rightAngle = azimuthRad + (halfBeamWidth * M_PI / 180);
    int leftEndX = centerX + static_cast<int>(radius * cos(leftAngle));
    int leftEndY = centerY - static_cast<int>(radius * sin(leftAngle));
    int rightEndX = centerX + static_cast<int>(radius * cos(rightAngle));
    int rightEndY = centerY - static_cast<int>(radius * sin(rightAngle));
    painter.drawLine(centerX, centerY, leftEndX, leftEndY);
    painter.drawLine(centerX, centerY, rightEndX, rightEndY);
}

/* Draw targets and their paths */
void RadarDisplay::drawTargetAndPath(QPainter &painter, int centerX, int centerY)
{
    if (entity && sensor) {
        painter.setBrush(Qt::red);
        for (const Target &target : sensor->targets) {
            int panelhigh = QWidget::height() - 60;
            float per = target.radius / range;
            float radius = panelhigh * per;
            float angle = target.angle;
            if (std::abs(angle) > (azimuth / 2)) continue;
            double targetAngle = (angle + 90) * M_PI / 180;
            double targetRadius = radius;
            int targetX = centerX + static_cast<int>(targetRadius * cos(targetAngle));
            int targetY = centerY - static_cast<int>(targetRadius * sin(targetAngle));
            // Draw target point
            painter.drawEllipse(targetX - 3, targetY - 3, 6, 6);
            // Draw path to target
            painter.setPen(QPen(Qt::cyan, 1, Qt::DotLine));
            QPointF pathPoints[] = {
                QPointF(centerX, centerY),
                QPointF(centerX, centerY - static_cast<int>(0.4 * radius)),
                QPointF(targetX, targetY)
            };
            painter.drawPolyline(pathPoints, 3);
            // Draw target labels
            painter.setPen(QPen(Qt::green, 1));
            painter.drawText(targetX - 20, targetY - 10, QString("%1").arg(angle));
            painter.drawText(targetX - 20, targetY + 5, QString("%1").arg(radius));
        }
    }
}

// %%% Paint Event %%%
/* Handle painting of radar display */
void RadarDisplay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int centerX = width() / 2;
    int centerY = QWidget::height() - 30;
    int radius = (QWidget::height() - 100) / 2;
    // Draw display components
    drawBackground(painter);
    drawConcentricCircles(painter, centerX, centerY, radius);
    drawVerticalLine(painter, centerX, centerY, radius);
    drawAnnotations(painter, centerX, QWidget::height());
    drawAzimuth(painter, centerX, centerY, width() * 2);
    drawCenterSquare(painter, centerX, centerY);
    drawTargetAndPath(painter, centerX, centerY);
    // Call base class paint event
    QWidget::paintEvent(event);
}
