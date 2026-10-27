//============================================================================
// FILE:    esmppiwidget.cpp
// MODULE:  ESM (Electronic Support Measures) Display
// PROJECT: Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
//============================================================================
#include "esmppiwidget.h"
#include "esmobservationmodel.h"
#include <core/Debug/profiler.h>
#include <QElapsedTimer>
#include <QFont>
#include <QFontMetricsF>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QToolTip>
#include <QtMath>
#include <cmath>

namespace {

const QColor kBg(6, 10, 7);
const QColor kDisc(5, 22, 9);
const QColor kGreen(0, 210, 70);
const QColor kGreenDim(0, 120, 45);
const QColor kGrid(0, 64, 28);
const QColor kText(150, 235, 170);
const QColor kPlot(60, 255, 120);
const QColor kSelected(255, 214, 0);
const QColor kHover(255, 255, 255);

const double kHitRadiusPx = 10.0;

/* Bearing (deg, clockwise from north) -> point on screen. */
QPointF polar(const QPointF& c, double radius, double bearingDeg)
{
    const double theta = qDegreesToRadians(bearingDeg - 90.0);
    return QPointF(c.x() + radius * std::cos(theta), c.y() + radius * std::sin(theta));
}

QPolygonF diamond(const QPointF& c, double s)
{
    QPolygonF poly;
    poly << QPointF(c.x(), c.y() - s) << QPointF(c.x() + s, c.y())
         << QPointF(c.x(), c.y() + s) << QPointF(c.x() - s, c.y());
    return poly;
}

} // namespace

ESMPpiWidget::ESMPpiWidget(ESMObservationModel* model, QWidget* parent)
    : QWidget(parent), model_(model)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (model_)
        connect(model_, &ESMObservationModel::observationsChanged,
                this, &ESMPpiWidget::onModelChanged);
}

QSize ESMPpiWidget::sizeHint() const        { return QSize(520, 520); }
QSize ESMPpiWidget::minimumSizeHint() const { return QSize(240, 240); }

void ESMPpiWidget::setRingCount(int count)
{
    ringCount_ = qBound(0, count, 12);
    update();
}

void ESMPpiWidget::setSelectedKey(const QString& key)
{
    if (selectedKey_ == key)
        return;
    selectedKey_ = key;
    update();
}

void ESMPpiWidget::onModelChanged()
{
    hovered_ = -1;
    QToolTip::hideText();
    update();
}

/* The scope is always a circle: radius comes from the smaller side only. */
bool ESMPpiWidget::scopeGeometry(QPointF& centre, double& radius) const
{
    const double side = qMin(width(), height());
    const double margin = qMax(26.0, side * 0.09);   // room for the degree labels
    radius = side / 2.0 - margin;
    centre = QPointF(width() / 2.0, height() / 2.0);
    return radius >= 20.0;
}

bool ESMPpiWidget::plotPosition(const ESMObservation& o, const QPointF& centre,
                                double radius, QPointF& out) const
{
    const double rangeKm = model_ ? model_->rangeKm() : 0.0;
    if (rangeKm <= 0.0 || !o.bearingValid || !o.rangeValid || o.rangeKm > rangeKm)
        return false;
    out = polar(centre, radius * (o.rangeKm / rangeKm), o.bearingDeg);
    return true;
}

int ESMPpiWidget::hitTest(const QPoint& pos) const
{
    QPointF c;
    double R = 0.0;
    if (!model_ || !scopeGeometry(c, R))
        return -1;

    const QVector<ESMObservation>& obs = model_->observations();
    double best = kHitRadiusPx;
    int bestIdx = -1;
    for (int i = 0; i < obs.size(); ++i) {
        QPointF pt;
        if (!plotPosition(obs.at(i), c, R, pt))
            continue;
        const double d = std::hypot(pos.x() - pt.x(), pos.y() - pt.y());
        if (d < best) {
            best = d;
            bestIdx = i;
        }
    }
    return bestIdx;
}

void ESMPpiWidget::paintEvent(QPaintEvent* /*event*/)
{
    QElapsedTimer timer;
    timer.start();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.fillRect(rect(), kBg);

    QPointF c;
    double R = 0.0;
    if (scopeGeometry(c, R)) {
        drawScope(p, c, R);
        drawObservations(p, c, R);
    }

    qint64 elapsedMs = timer.elapsed();
    Profiler::currentFrame->esmdisplay = elapsedMs;
}

void ESMPpiWidget::drawScope(QPainter& p, const QPointF& c, double R) const
{
    p.save();

    // Scope disc
    p.setPen(Qt::NoPen);
    p.setBrush(kDisc);
    p.drawEllipse(c, R, R);
    p.setBrush(Qt::NoBrush);

    // Radial grid every 30 degrees
    p.setPen(QPen(kGrid, 1));
    for (int deg = 0; deg < 360; deg += 30)
        p.drawLine(c, polar(c, R, deg));

    // Range rings (ringCount_ dashed rings + the outer ring = configured range)
    const int divisions = ringCount_ + 1;
    p.setPen(QPen(kGreenDim, 1, Qt::DashLine));
    for (int i = 1; i < divisions; ++i) {
        const double rr = R * i / divisions;
        p.drawEllipse(c, rr, rr);
    }
    p.setPen(QPen(kGreen, 1.6));
    p.drawEllipse(c, R, R);

    // Azimuth ticks: minor every 5, major every 30 degrees
    for (int deg = 0; deg < 360; deg += 5) {
        const bool major = (deg % 30 == 0);
        const double len = R * (major ? 0.05 : 0.025);
        p.setPen(QPen(kGreen, major ? 1.5 : 1.0));
        p.drawLine(polar(c, R, deg), polar(c, R - len, deg));
    }

    // Azimuth labels
    QFont f = font();
    f.setPixelSize(qBound(8, int(R / 16.0), 13));
    p.setFont(f);
    const QFontMetricsF fm(f);
    for (int deg = 0; deg < 360; deg += 30) {
        const bool cardinal = (deg % 90 == 0);
        QFont lf = f;
        lf.setBold(cardinal);
        p.setFont(lf);
        const QString text = QString("%1\u00B0").arg(deg, 3, 10, QLatin1Char('0'));
        const QFontMetricsF lfm(lf);
        const double w = lfm.boundingRect(text).width();
        const double h = lfm.height();
        const double theta = qDegreesToRadians(deg - 90.0);
        const double extent = std::fabs(std::cos(theta)) * w / 2.0 + std::fabs(std::sin(theta)) * h / 2.0;
        const QPointF centre = polar(c, R + 5.0 + extent, deg);
        p.setPen(cardinal ? kPlot : kText);
        p.drawText(QRectF(centre.x() - w / 2.0 - 2, centre.y() - h / 2.0, w + 4, h),
                   Qt::AlignCenter, text);
    }
    p.setFont(f);

    // Range labels (only when a range is actually configured)
    const double rangeKm = model_ ? model_->rangeKm() : 0.0;
    if (rangeKm > 0.0) {
        p.setPen(kGreenDim);
        for (int i = 1; i <= divisions; ++i) {
            const double rr = R * i / divisions;
            const QString text = QString::number(rangeKm * i / divisions, 'g', 4) + QStringLiteral(" km");
            p.drawText(QPointF(c.x() + 4.0, c.y() - rr + fm.ascent() + 1.0), text);
        }
    }

    // Own platform marker (centre)
    const double s = qMax(5.0, R * 0.03);
    QPolygonF tri;
    tri << QPointF(c.x(), c.y() - s)
        << QPointF(c.x() - s * 0.85, c.y() + s * 0.7)
        << QPointF(c.x() + s * 0.85, c.y() + s * 0.7);
    p.setPen(QPen(kGreen, 1));
    p.setBrush(kGreen);
    p.drawPolygon(tri);

    p.restore();
}

void ESMPpiWidget::drawObservations(QPainter& p, const QPointF& c, double R) const
{
    if (!model_)
        return;

    p.save();
    QFont f = font();
    f.setPixelSize(qBound(8, int(R / 20.0), 12));
    p.setFont(f);
    const QFontMetricsF fm(f);

    const QVector<ESMObservation>& obs = model_->observations();
    for (int i = 0; i < obs.size(); ++i) {
        const ESMObservation& o = obs.at(i);
        QPointF pt;
        if (!plotPosition(o, c, R, pt))
            continue;

        const bool selected = (!selectedKey_.isEmpty() && o.key == selectedKey_);
        const bool hovered = (i == hovered_);
        const QColor col = selected ? kSelected : kPlot;
        const double size = selected ? 6.0 : 4.5;

        p.setPen(QPen(col, 1));
        p.setBrush(col);
        p.drawPolygon(diamond(pt, size));

        if (selected || hovered) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(selected ? kSelected : kHover, 1.2));
            p.drawEllipse(pt, size + 5.0, size + 5.0);
        }

        // ID label: platform name, falling back to the platform ID
        const QString raw = o.name.isEmpty() ? o.key : o.name;
        const QString label = fm.elidedText(raw, Qt::ElideRight, 90.0);
        p.setPen(selected ? kSelected : kText);
        p.drawText(QPointF(pt.x() + size + 6.0, pt.y() - 4.0), label);
    }

    // Empty-state message
    QString msg;
    switch (model_->sensorState()) {
    case ESMObservationModel::SensorState::NoSensor: msg = QStringLiteral("No ESM sensor selected"); break;
    case ESMObservationModel::SensorState::Inactive: msg = QStringLiteral("ESM sensor inactive"); break;
    case ESMObservationModel::SensorState::Active:
        if (model_->rangeKm() <= 0.0)      msg = QStringLiteral("ESM range not configured");
        else if (obs.isEmpty())            msg = QStringLiteral("No ESM detections");
        break;
    }
    if (!msg.isEmpty()) {
        p.setPen(kText);
        p.drawText(QRectF(c.x() - R, c.y() + R * 0.40, R * 2.0, fm.height() + 4.0),
                   Qt::AlignHCenter | Qt::AlignTop, msg);
    }
    p.restore();
}

void ESMPpiWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint pos = event->pos();
    const int idx = hitTest(pos);

    if (idx != hovered_) {
        hovered_ = idx;
        update();
    }
    if (idx >= 0 && model_) {
        setCursor(Qt::PointingHandCursor);
        QToolTip::showText(mapToGlobal(pos + QPoint(14, 14)), model_->detailText(idx), this);
    } else {
        unsetCursor();
        QToolTip::hideText();
    }
    QWidget::mouseMoveEvent(event);
}

void ESMPpiWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && model_) {
        const int idx = hitTest(event->pos());
        const QString key = (idx >= 0) ? model_->keyAt(idx) : QString();
        setSelectedKey(key);
        emit observationSelected(key);
    }
    QWidget::mousePressEvent(event);
}

void ESMPpiWidget::leaveEvent(QEvent* event)
{
    hovered_ = -1;
    QToolTip::hideText();
    unsetCursor();
    update();
    QWidget::leaveEvent(event);
}
