#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <QObject>
#include <QPainter>
#include <QMouseEvent>
#include <QPointF>
#include <QList>
#include <QString>
#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include "GUI/measuredistance/measuredistancedialog.h"

// ============================================================================
// CHAITANYA JAIN CHANGE
//
// Reason:
// Measurement ownership moved from GISlib/CanvasWidget into a dedicated
// Measurement module to reduce coupling and improve maintainability.
//
// Why this change is safe:
// Existing behavior, rendering, event flow, and calculations remain unchanged.
// Only responsibility ownership has been refactored.
//
// ============================================================================

class Measurement : public QObject {
    Q_OBJECT
public:
    explicit Measurement(GISlib* gislib, QWidget* parentWidget = nullptr);
    ~Measurement();

    void startDistanceMeasurement();
    void endDistanceMeasurement();
    void clearMeasurementPoints();
    void addMeasurePoint(double lon, double lat);
    void setMeasurementUnit(const QString& unit);

    double getLastSegmentDistance() const;
    double getTotalDistance() const;

    bool isMeasuringDistance() const { return measuringDistance; }

    // UI getters
    MeasureDistanceDialog* getDialog();

signals:
    void measurementUpdated();

public:
    // Event handlers to be called by CanvasWidget
    void handleMousePress(QMouseEvent* event);

    void handleMouseMove(QMouseEvent* event);
    void handleMouseRelease(QMouseEvent* event);
    void drawRubberBand(QPainter& painter);
    void drawCommittedSegments(QPainter& painter);

public slots:
    void onMeasurementTypeChanged(bool isEllipsoidal);

private:
    GISlib* m_gislib;
    QWidget* m_parentWidget;
    MeasureDistanceDialog* measureDialog;

    // From CanvasWidget
    QList<QPointF> measurePoints;
    QString measurementUnit;
    double conversionFactor;

    // From GISlib
    bool measuringDistance;
    QPointF currentMouseGeo;

    bool m_isClearing;
};


#endif // MEASUREMENT_H