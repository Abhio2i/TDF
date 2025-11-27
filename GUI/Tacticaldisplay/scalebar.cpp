#include "GUI/Tacticaldisplay/scalebar.h"
#include <QPainter>
#include <QDebug>
#include <cmath>

ScaleBar::ScaleBar(QWidget *parent) : QWidget(parent)
{
    m_currentConfig = {100, "1 km", 1000};
    setFixedSize(180, 40);
}

void ScaleBar::updateScale(double metersPerPixel, int containerWidth)
{
    m_currentConfig = getOptimalScale(metersPerPixel, containerWidth);
    update(); // Trigger repaint

    qDebug() << "Scale Bar:" << m_currentConfig.barLength << "px ="
             << m_currentConfig.displayText << "(" << m_currentConfig.actualDistance << "m)";
}

ScaleBar::ScaleConfig ScaleBar::getOptimalScale(double metersPerPixel, int containerWidth) const
{
    if (metersPerPixel <= 0) {
        return {100, "1 km", 1000}; // Fallback
    }

    const QVector<int> barLengths = {50, 80, 100, 120, 150};
    const int maxBarWidth = 120; // Fixed max width for scale bar

    // Try to find the best scale that gives a nice number
    for (int barLength : barLengths) {
        double distanceMeters = barLength * metersPerPixel;

        // Convert to km for checking
        double distanceKm = distanceMeters / 1000.0;

        if (isNiceNumber(distanceKm) && barLength <= maxBarWidth) {
            return {
                barLength,
                formatDistance(distanceKm),
                distanceMeters
            };
        }
    }

    // If no nice number found in km, try with meters for small distances
    for (int barLength : barLengths) {
        double distanceMeters = barLength * metersPerPixel;

        if (distanceMeters < 1000) { // Less than 1 km - use meters
            if (isNiceNumber(distanceMeters) && barLength <= maxBarWidth) {
                return {
                    barLength,
                    formatDistance(distanceMeters),
                    distanceMeters
                };
            }
        } else { // 1 km or more - use km
            double distanceKm = distanceMeters / 1000.0;
            if (barLength <= maxBarWidth) {
                // Round to nearest nice number
                double magnitude = std::pow(10, std::floor(std::log10(distanceKm)));
                double normalized = std::round(distanceKm / magnitude);

                if (normalized == 1 || normalized == 2 || normalized == 5 || normalized == 10) {
                    double niceDistanceKm = normalized * magnitude;
                    int niceBarLength = std::round((niceDistanceKm * 1000) / metersPerPixel);

                    if (niceBarLength <= maxBarWidth && niceBarLength >= 30) {
                        return {
                            niceBarLength,
                            formatDistance(niceDistanceKm),
                            niceDistanceKm * 1000
                        };
                    }
                }
            }
        }
    }

    // Final fallback
    return {100, "1 km", 1000};
}

bool ScaleBar::isNiceNumber(double num) const
{
    if (num <= 0) return false;

    double magnitude = std::pow(10, std::floor(std::log10(num)));
    double normalized = num / magnitude;

    // Common "nice" numbers for scale bars
    QVector<double> niceNumbers = {1, 2, 5, 10, 20, 25, 50, 100, 200, 500, 1000};
    for (double nice : niceNumbers) {
        if (std::abs(normalized - nice) < 0.001) {
            return true;
        }
    }
    return false;
}

QString ScaleBar::formatDistance(double distance) const
{
    // If distance is less than 1 km, show in meters
    if (distance < 1.0) {
        int meters = std::round(distance * 1000);

        // For meters, we want nice numbers like 10m, 20m, 50m, 100m, 200m, 500m
        if (meters < 10) {
            return QString::number(meters) + " m";
        } else if (meters < 100) {
            // Round to nearest 10
            int rounded = (meters + 5) / 10 * 10;
            return QString::number(rounded) + " m";
        } else {
            // Round to nearest 50 or 100
            if (meters < 250) {
                int rounded = (meters + 25) / 50 * 50;
                return QString::number(rounded) + " m";
            } else {
                int rounded = (meters + 50) / 100 * 100;
                return QString::number(rounded) + " m";
            }
        }
    } else {
        // For km, show with appropriate precision
        if (distance < 10) {
            return QString::number(distance, 'f', 1) + " km";
        } else {
            return QString::number(std::round(distance)) + " km";
        }
    }
}

void ScaleBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set semi-transparent background
    painter.fillRect(rect(), QColor(255, 255, 255, 220));

    // Draw scale bar
    int barHeight = 6;
    int textMargin = 4;

    // Position the scale bar
    int barX = 10;
    int barY = 15;

    // Draw the main bar with black border
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::black);
    painter.drawRect(barX, barY, m_currentConfig.barLength, barHeight);

    // Draw alternating white segments (like Google Maps)
    painter.setBrush(Qt::white);
    int segmentCount = m_currentConfig.barLength / 20;
    for (int i = 0; i < segmentCount; i++) {
        if (i % 2 == 0) { // Draw white segments on even positions
            painter.drawRect(barX + i * 20, barY, 10, barHeight);
        }
    }

    // Redraw border to ensure clean edges
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(barX, barY, m_currentConfig.barLength, barHeight);

    // Draw text below the scale bar
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(8);
    font.setWeight(QFont::Normal);
    painter.setFont(font);

    painter.drawText(barX, barY + barHeight + 15, m_currentConfig.displayText);
}

QSize ScaleBar::sizeHint() const
{
    return QSize(180, 40);
}
