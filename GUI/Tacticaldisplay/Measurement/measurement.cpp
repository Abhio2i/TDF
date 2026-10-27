#include "measurement.h"
#include <QDebug>
#include <QMessageBox>
#include <cmath>

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

Measurement::Measurement(GISlib* gislib, QWidget* parentWidget)
    : QObject(parentWidget),
    m_gislib(gislib),
    m_parentWidget(parentWidget),
    measuringDistance(false),
    currentMouseGeo(0, 0),
    measurementUnit("m"),
    conversionFactor(1.0),
    m_isClearing(false),
    measureDialog(nullptr)
{
    // Dialog creation and configuration is intentionally delayed (lazy initialization)
    // to prevent early native window creation which causes memory corruption during CanvasWidget construction.
}

Measurement::~Measurement() {
    if (measureDialog) {
        measureDialog->deleteLater();
    }
}

MeasureDistanceDialog* Measurement::getDialog() {
    if (!measureDialog) {
        // ============================================================================
        // CHAITANYA JAIN CHANGE
        //
        // Reason:
        // Confirmed Root Cause: Early Window Initialization causing SIGSEGV memory corruption.
        // Creating a QDialog and modifying its window flags inside the parent's constructor
        // corrupts Qt's native window backing store because the parent is not fully realized.
        //
        // Why this change is safe:
        // We restore LAZY INITIALIZATION (on-demand creation). This perfectly matches the
        // original legacy behavior, ensuring the dialog is only created when the application
        // is fully running and native windows are stable. Zero impact on existing features.
        // ============================================================================

        // Use parent's parent (MainWindow) to match original behavior
        measureDialog = new MeasureDistanceDialog(m_parentWidget ? m_parentWidget->parentWidget() : nullptr);
        measureDialog->setAttribute(Qt::WA_DeleteOnClose, false);
        measureDialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        measureDialog->setModal(false);

        connect(measureDialog, &MeasureDistanceDialog::newMeasurementRequested, this, [this]() {
            this->measurePoints.clear();
            this->currentMouseGeo = QPointF(0, 0);
            emit measurementUpdated();
        });

        connect(measureDialog, &MeasureDistanceDialog::measurementTypeChanged,
                this, &Measurement::onMeasurementTypeChanged);

        connect(measureDialog, &MeasureDistanceDialog::unitChanged,
                this, &Measurement::setMeasurementUnit);
    }
    return measureDialog;
}

void Measurement::startDistanceMeasurement() {
    measuringDistance = true;
    currentMouseGeo = QPointF(0, 0);
    measurePoints.clear();
    if (measureDialog) measureDialog->clearMeasurements();
}

void Measurement::endDistanceMeasurement() {
    if (measuringDistance) {
        measuringDistance = false;
        currentMouseGeo = QPointF(0, 0);
    }
}

void Measurement::clearMeasurementPoints() {
    if (m_isClearing) return;
    m_isClearing = true;
    measurePoints.clear();
    currentMouseGeo = QPointF(0, 0);
    if (measureDialog) measureDialog->clearMeasurements();
    emit measurementUpdated();
    m_isClearing = false;
}

void Measurement::addMeasurePoint(double lon, double lat) {
    if (!measuringDistance) startDistanceMeasurement();
    QPointF geo(lon, lat);
    measurePoints.append(geo);

    currentMouseGeo = geo;

    if (measureDialog && measurePoints.size() >= 2) {
        double dist = m_gislib->calculateDistance(measurePoints[measurePoints.size() - 2],
                                                  measurePoints.last());
        measureDialog->addMeasurement(lon, lat, dist);
    }
}

void Measurement::setMeasurementUnit(const QString &unit) {
    measurementUnit = unit;
    if (unit == "m") conversionFactor = 1.0;
    else if (unit == "km") conversionFactor = 0.001;
    else if (unit == "ft") conversionFactor = 3.28084;
    else if (unit == "mi") conversionFactor = 0.000621371;
    else if (unit == "deg") conversionFactor = 1.0;
}

double Measurement::getLastSegmentDistance() const {
    if (measurePoints.size() < 2) return 0.0;
    return m_gislib->calculateDistance(measurePoints[measurePoints.size() - 2],
                                       measurePoints.last());
}

double Measurement::getTotalDistance() const {
    double total = 0.0;
    for (int i = 1; i < measurePoints.size(); ++i)
        total += m_gislib->calculateDistance(measurePoints[i-1], measurePoints[i]);
    return total;
}

void Measurement::onMeasurementTypeChanged(bool isEllipsoidal) {
    m_gislib->setEllipsoidal(isEllipsoidal);
    measureDialog->clearMeasurements();
    for (int i = 1; i < measurePoints.size(); ++i) {
        double dist = m_gislib->calculateDistance(measurePoints[i-1], measurePoints[i]);
        QPointF curr = measurePoints[i];
        measureDialog->addMeasurement(curr.x(), curr.y(), dist);
    }
}

void Measurement::handleMousePress(QMouseEvent* event) {
    if (measuringDistance && event->button() == Qt::LeftButton) {
        // ============================================================================
        // CHAITANYA JAIN CHANGE
        //
        // Reason:
        // Fixed Measurement rendering to use persistent geographic coordinates
        // instead of screen-space coordinates. Measurement now behaves as a true
        // GIS overlay and remains attached to the map during zoom/pan operations.
        //
        // ============================================================================

        // Convert screen pixel to geographic coordinate (lon, lat)
        QPointF geo = m_gislib->canvasToGeo(event->pos());
        measurePoints.append(geo);

        // Update rubber band end to this exact geographic coordinate
        currentMouseGeo = geo;

        if (measurePoints.size() >= 2) {
            QPointF prev = measurePoints[measurePoints.size() - 2];
            QPointF curr = measurePoints.last();
            // Calculate distance using actual geographic positions
            double dist = m_gislib->calculateDistance(prev, curr);
            if (std::isnan(dist)) dist = 0.0;
            if (measureDialog) {
                measureDialog->addMeasurement(curr.x(), curr.y(), dist);
            }
        }

        // ============================================================================
        // CHAITANYA JAIN CHANGE
        // Root Cause: Rubber band missing updates during interactions.
        // Legacy Behaviour: update() was called natively on GISlib.
        // Current Behaviour: Refresh() only triggered CanvasWidget repaint.
        // Why this fix restores original behaviour: Forces GISlib to repaint.
        // Why this is safe: Matches exact legacy behavior.
        // ============================================================================
        if (m_gislib) m_gislib->update();
        emit measurementUpdated();
    }
}

void Measurement::handleMouseMove(QMouseEvent* event) {
    if (measuringDistance) {
        // Always store rubber band endpoint as a geographic coordinate (lon, lat)
        currentMouseGeo = m_gislib->canvasToGeo(event->pos());

        // ============================================================================
        // CHAITANYA JAIN CHANGE
        // Root Cause: Rubber band missing updates during interactions.
        // Legacy Behaviour: update() was called natively on GISlib.
        // Current Behaviour: Refresh() only triggered CanvasWidget repaint.
        // Why this fix restores original behaviour: Forces GISlib to repaint.
        // Why this is safe: Matches exact legacy behavior.
        // ============================================================================
        if (m_gislib) m_gislib->update();
        emit measurementUpdated();
    }
}

void Measurement::handleMouseRelease(QMouseEvent* event) {
    Q_UNUSED(event);
    // CanvasWidget's logic does not end measurement on mouse release.
}

void Measurement::drawRubberBand(QPainter& painter) {
    // ============================================================================
    // CHAITANYA JAIN CHANGE
    //
    // Reason:
    // Split legacy behavior into two methods: rubber band goes to GISlib,
    // and committed segments go to CanvasWidget.
    //
    // ============================================================================

    // 1. Draw rubber band (migrated from GISlib)
    if (measuringDistance && !measurePoints.isEmpty() && currentMouseGeo != QPointF(0, 0)) {
        // geoToCanvas strictly uses (latitude, longitude)
        QPointF lastPoint = measurePoints.last();
        QPointF startCanvas = m_gislib->geoToCanvas(lastPoint.y(), lastPoint.x());
        QPointF endCanvas = m_gislib->geoToCanvas(currentMouseGeo.y(), currentMouseGeo.x());

        painter.save();
        painter.setPen(QPen(Qt::yellow, 2, Qt::DashLine));
        painter.drawLine(startCanvas, endCanvas);

        double distanceMeters = m_gislib->calculateDistance(lastPoint, currentMouseGeo);
        if (std::isnan(distanceMeters)) distanceMeters = 0.0;

        double factor = measureDialog ? measureDialog->getCurrentConversionFactor() : 1.0;
        QString unit = measureDialog ? measureDialog->getCurrentUnitString() : "m";

        double displayDistance = distanceMeters * factor;
        QString distanceText = QString("%1 %2").arg(displayDistance, 0, 'f', 2).arg(unit);

        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 10));
        QPointF midPoint = (startCanvas + endCanvas) / 2.0;
        painter.drawText(midPoint + QPointF(10, -10), distanceText);
        painter.restore();
    }
}

void Measurement::drawCommittedSegments(QPainter& painter) {
    // 2. Draw committed segments (migrated from CanvasWidget)
    if (measuringDistance && !measurePoints.empty()) {
        painter.save();
        QPen pen(Qt::yellow, 2, Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPolygonF poly;

        // Recompute canvas positions from stored geographic coordinates on every paint
        for (const auto& geo : measurePoints) {
            // geoToCanvas strictly uses (latitude, longitude)
            QPointF canvas = m_gislib->geoToCanvas(geo.y(), geo.x());
            poly << canvas;
            painter.setBrush(Qt::yellow);
            painter.drawEllipse(canvas, 5, 5);
            painter.setBrush(Qt::NoBrush);
        }
        if (poly.size() > 1) painter.drawPolyline(poly);

        painter.setFont(QFont("Arial", 10));
        painter.setPen(Qt::black);
        double factor = measureDialog ? measureDialog->getCurrentConversionFactor() : 1.0;
        QString unit = measureDialog ? measureDialog->getCurrentUnitString() : "m";
        for (int i = 1; i < measurePoints.size(); ++i) {
            // Calculate distance using geographic coordinates
            double dist = m_gislib->calculateDistance(measurePoints[i-1], measurePoints[i]);
            if (std::isnan(dist)) dist = 0.0;
            QPointF mid = (poly[i-1] + poly[i]) / 2.0;
            painter.drawText(mid + QPointF(10, -10),
                             QString("%1 %2").arg(dist * factor, 0, 'f', 2).arg(unit));
        }
        painter.restore();
    }
}
