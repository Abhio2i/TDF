/* ========================================================================= */
/* File: radardisplay.h                                                     */
/* Purpose: Defines widget for radar display visualization                   */
/* ========================================================================= */

#ifndef RADARDISPLAY_H
#define RADARDISPLAY_H

#include "core/Hierarchy/EntityProfiles/sensor.h"  // For sensor profile
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include <QWidget>                                // For widget base class
#include <QPainter>                               // For painting operations
#include <QJsonDocument>                          // For JSON document handling
#include <QJsonObject>                            // For JSON object handling
#include <QJsonArray>                             // For JSON array handling
#include <QVector>                                // For vector container

// %%% Class Definition %%%
/* Widget for radar display visualization */
class RadarDisplay : public QWidget
{
    Q_OBJECT

public:
    // Initialize radar display
    explicit RadarDisplay(QWidget *parent = nullptr);
    // Set hierarchy instance
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    // Update display from JSON
    void updateFromJson(const QJsonObject &json);
    // Get size hint
    QSize sizeHint() const override;
    // Get minimum size
    QSize minimumSize() const;
    // Get height for width
    int heightForWidth(int width) const override;
    // Set azimuth angle
    void setAzimuth(float value);
    // Set radar range
    void setRange(float value);
    // Select entity
    void selectEntity(Entity* entity);
    // Remove entity by ID
    void RemoveEntity(QString ID);
    // Update radar display
    void updateRadar();
    // Sensor instance
    Sensor* sensor = nullptr;
    // Entity platform
    Platform* entity = nullptr;

protected:
    // Handle paint events
    void paintEvent(QPaintEvent *event) override;

private:
    // %%% Display Properties %%%
    // Radar range
    int range = 100;
    // Azimuth angle in degrees
    double azimuth = 30.0;
    // Beamwidth in degrees
    double bar = 3.0;
    // Current speed
    int current_speed = 0;
    // Maximum speed
    int max_speed = 0;
    // Radar height
    int radar_height = 0;
    // Target structure (commented)
    // struct Target {
    //     double angle;
    //     double radius;
    // };
    // List of targets
    QVector<Target> targets;
    // Entity ID
    QString id = "";
    // Hierarchy instance
    Hierarchy* hierarchy = nullptr;

    // %%% Drawing Methods %%%
    // Draw background
    void drawBackground(QPainter &painter);
    // Draw concentric circles
    void drawConcentricCircles(QPainter &painter, int centerX, int centerY, int radius);
    // Draw vertical line
    void drawVerticalLine(QPainter &painter, int centerX, int centerY, int radius);
    // Draw center square
    void drawCenterSquare(QPainter &painter, int centerX, int centerY);
    // Draw annotations
    void drawAnnotations(QPainter &painter, int centerX, int widgetHeight);
    // Draw azimuth
    void drawAzimuth(QPainter &painter, int centerX, int centerY, int radius);
    // Draw target and path
    void drawTargetAndPath(QPainter &painter, int centerX, int centerY);
    // Draw info text
    void drawInfoText(QPainter &painter, int widgetHeight);
};

#endif // RADARDISPLAY_H
