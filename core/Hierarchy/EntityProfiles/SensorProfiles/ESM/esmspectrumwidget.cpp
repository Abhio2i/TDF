//============================================================================
// FILE:    esmspectrumwidget.cpp
// MODULE:  ESM (Electronic Support Measures) Display
// PROJECT: Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
//============================================================================
#include "esmspectrumwidget.h"
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPolygonF>
#include <algorithm>
#include <cmath>

namespace {

const QColor kBg(6, 10, 7);
const QColor kFrame(14, 90, 38);
const QColor kGrid(0, 56, 24);
const QColor kText(150, 235, 170);
const QColor kTitle(53, 255, 122);
const QColor kTrace(60, 255, 120);
const QColor kMarker(255, 214, 0);

/* 1-2-5 "nice" step so that about `target` ticks cover `range`. */
double niceStep(double range, int target)
{
    if (!(range > 0.0) || target < 1)
        return 1.0;
    const double raw = range / target;
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    const double n = raw / mag;
    const double s = n < 1.5 ? 1.0 : n < 3.0 ? 2.0 : n < 7.0 ? 5.0 : 10.0;
    return s * mag;
}

int decimalsForStep(double step)
{
    if (step >= 1.0) return 0;
    return qMin(6, int(std::ceil(-std::log10(step))));
}

} // namespace

ESMSpectrumWidget::ESMSpectrumWidget(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize ESMSpectrumWidget::sizeHint() const        { return QSize(640, 360); }
QSize ESMSpectrumWidget::minimumSizeHint() const { return QSize(280, 180); }

void ESMSpectrumWidget::setSpectrum(const QVector<double>& frequenciesMHz,
                                    const QVector<double>& psdDbmPerHz)
{
    if (frequenciesMHz.size() != psdDbmPerHz.size() || frequenciesMHz.size() < 2) {
        clearSpectrum();
        return;
    }
    for (int i = 0; i < frequenciesMHz.size(); ++i) {
        if (!std::isfinite(frequenciesMHz.at(i)) || !std::isfinite(psdDbmPerHz.at(i))) {
            clearSpectrum();
            return;
        }
    }
    freqMHz_ = frequenciesMHz;
    psd_ = psdDbmPerHz;
    update();
}

void ESMSpectrumWidget::clearSpectrum()
{
    freqMHz_.clear();
    psd_.clear();
    update();
}

void ESMSpectrumWidget::setMarkerFrequencyMHz(double mhz)
{
    if (!std::isfinite(mhz)) {
        clearMarker();
        return;
    }
    hasMarker_ = true;
    markerMHz_ = mhz;
    update();
}

void ESMSpectrumWidget::clearMarker()
{
    if (!hasMarker_)
        return;
    hasMarker_ = false;
    update();
}

void ESMSpectrumWidget::setSelectedInfo(const QString& text)
{
    if (selectedInfo_ == text)
        return;
    selectedInfo_ = text;
    update();
}

void ESMSpectrumWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.fillRect(rect(), kBg);

    QFont f = font();
    f.setPixelSize(11);
    p.setFont(f);
    const QFontMetrics fm(f);

    const int left = 54;
    const int right = 54;
    const int top = 30;
    const int bottom = 46;
    const QRect plot(left, top, width() - left - right, height() - top - bottom);
    if (plot.width() < 60 || plot.height() < 40)
        return;

    const bool data = hasSpectrum();

    // ---- header ----------------------------------------------------------
    QFont tf = f;
    tf.setBold(true);
    p.setFont(tf);
    p.setPen(kTitle);
    p.drawText(QRect(8, 6, 150, 20), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("SIGNAL SPECTRUM"));
    p.setFont(f);

    double xmin = 0.0, xmax = 1.0, ylo = 0.0, yhi = 1.0;
    QString spanText = QStringLiteral("SPAN --");
    if (data) {
        const auto fr = std::minmax_element(freqMHz_.begin(), freqMHz_.end());
        const auto pw = std::minmax_element(psd_.begin(), psd_.end());
        xmin = *fr.first;
        xmax = *fr.second;
        if (xmax - xmin <= 0.0) { xmin -= 0.5; xmax += 0.5; }
        double ymin = *pw.first, ymax = *pw.second;
        double pad = (ymax - ymin) * 0.08;
        if (pad <= 0.0) pad = 1.0;
        ymin -= pad;
        ymax += pad;
        const double ystep = niceStep(ymax - ymin, 8);
        ylo = std::floor(ymin / ystep) * ystep;
        yhi = std::ceil(ymax / ystep) * ystep;
        spanText = QString("SPAN %1 - %2 MHz").arg(xmin, 0, 'g', 6).arg(xmax, 0, 'g', 6);
    }
    p.setPen(kText);
    p.drawText(QRect(width() - 308, 6, 300, 20), Qt::AlignRight | Qt::AlignVCenter, spanText);
    if (!selectedInfo_.isEmpty()) {
        const QString elided = fm.elidedText(selectedInfo_, Qt::ElideRight, qMax(40, width() - 150 - 316));
        p.setPen(kMarker);
        p.drawText(QRect(160, 6, width() - 150 - 316, 20), Qt::AlignLeft | Qt::AlignVCenter, elided);
    }

    // ---- frame -----------------------------------------------------------
    p.setPen(QPen(kFrame, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(plot);

    auto xToPx = [&](double v) { return plot.left() + (v - xmin) / (xmax - xmin) * plot.width(); };
    auto yToPx = [&](double v) { return plot.bottom() - (v - ylo) / (yhi - ylo) * plot.height(); };

    if (!data) {
        // Empty graph: grid only, no numeric ticks (there is no scale to show).
        p.setPen(QPen(kGrid, 1, Qt::DashLine));
        for (int i = 1; i < 10; ++i) {
            const int x = plot.left() + plot.width() * i / 10;
            p.drawLine(x, plot.top(), x, plot.bottom());
        }
        for (int i = 1; i < 8; ++i) {
            const int y = plot.top() + plot.height() * i / 8;
            p.drawLine(plot.left(), y, plot.right(), y);
        }
        QFont mf = f;
        mf.setPixelSize(15);
        p.setFont(mf);
        p.setPen(kText);
        p.drawText(plot, Qt::AlignCenter, QStringLiteral("Spectrum data unavailable"));
        p.setFont(f);
    } else {
        // Grid + tick labels
        const double xstep = niceStep(xmax - xmin, 10);
        const double ystep = niceStep(yhi - ylo, 8);
        const int xdec = decimalsForStep(xstep);
        const int ydec = decimalsForStep(ystep);

        p.setPen(QPen(kGrid, 1, Qt::DashLine));
        int guard = 0;
        for (double v = std::ceil(xmin / xstep) * xstep; v <= xmax + xstep * 1e-9 && guard < 200; v += xstep, ++guard) {
            const double x = xToPx(v);
            p.setPen(QPen(kGrid, 1, Qt::DashLine));
            p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
            p.setPen(kText);
            p.drawText(QRectF(x - 34, plot.bottom() + 4, 68, 14), Qt::AlignHCenter | Qt::AlignTop,
                       QString::number(v, 'f', xdec));
        }
        guard = 0;
        for (double v = ylo; v <= yhi + ystep * 1e-9 && guard < 200; v += ystep, ++guard) {
            const double y = yToPx(v);
            p.setPen(QPen(kGrid, 1, Qt::DashLine));
            p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
            p.setPen(kText);
            p.drawText(QRectF(2, y - 7, left - 8, 14), Qt::AlignRight | Qt::AlignVCenter,
                       QString::number(v, 'f', ydec));
        }

        p.save();
        p.setClipRect(plot.adjusted(1, 1, -1, -1));

        if (hasMarker_ && markerMHz_ >= xmin && markerMHz_ <= xmax) {
            const double x = xToPx(markerMHz_);
            p.setPen(QPen(kMarker, 1, Qt::DashLine));
            p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        }

        QPolygonF trace;
        trace.reserve(freqMHz_.size());
        for (int i = 0; i < freqMHz_.size(); ++i)
            trace << QPointF(xToPx(freqMHz_.at(i)), yToPx(psd_.at(i)));
        p.setPen(QPen(kTrace, 1));
        p.drawPolyline(trace);
        p.restore();
    }

    // ---- axis titles -----------------------------------------------------
    p.setPen(kText);
    p.drawText(QRect(plot.left(), height() - 20, plot.width(), 16), Qt::AlignHCenter | Qt::AlignVCenter,
               QStringLiteral("FREQUENCY (MHz)"));
    p.save();
    p.translate(14, plot.center().y());
    p.rotate(-90);
    p.drawText(QRect(-plot.height() / 2, -8, plot.height(), 16), Qt::AlignHCenter | Qt::AlignVCenter,
               QStringLiteral("PSD (dBm/Hz)"));
    p.restore();
}
