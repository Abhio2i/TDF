
#include "measure_api.h"
#include <cmath>

MeasureApi* MeasureApi::m_instance = nullptr;

MeasureApi* MeasureApi::instance()
{
    if (!m_instance)
        m_instance = new MeasureApi();
    return m_instance;
}

MeasureApi::MeasureApi(QObject* parent)
    : QObject(parent)
{
}

MeasureApi::~MeasureApi() {}

void MeasureApi::startMeasuring()
{
    if (m_active)
        return;
    m_active = true;
    m_points.clear();
    emit measuringChanged(true);
    emit pointsChanged();
}

void MeasureApi::stopMeasuring()
{
    if (!m_active)
        return;
    m_active = false;
    emit measuringChanged(false);
}

void MeasureApi::addPoint(double lon, double lat)
{
    if (!m_active)
        return;
    m_points.append(QPointF(lon, lat));
    emit pointsChanged();
}

void MeasureApi::addPoint(const QPointF& geoPos)
{
    addPoint(geoPos.x(), geoPos.y());
}

void MeasureApi::clearPoints()
{
    m_points.clear();
    emit cleared();
    emit pointsChanged();
}

QVector<double> MeasureApi::getSegmentMeters() const
{
    QVector<double> segs;
    segs.reserve(m_points.size());
    for (int i = 0; i < m_points.size(); ++i) {
        if (i == 0) {
            segs.append(0.0);
        } else {
            const QPointF& a = m_points[i - 1];
            const QPointF& b = m_points[i];
            segs.append(distanceMeters(a.y(), a.x(), b.y(), b.x()));
        }
    }
    return segs;
}

double MeasureApi::getTotalMeters() const
{
    double total = 0.0;
    const QVector<double> segs = getSegmentMeters();
    for (double d : segs)
        total += d;
    return total;
}

double MeasureApi::segmentMetersAt(int index) const
{
    if (index < 0 || index >= m_points.size())
        return 0.0;
    if (index == 0)
        return 0.0;
    const QPointF& a = m_points[index - 1];
    const QPointF& b = m_points[index];
    return distanceMeters(a.y(), a.x(), b.y(), b.x());
}

double MeasureApi::convertFromMeters(double meters, const QString& unit) const
{
    const QString u = unit.toLower().trimmed();
    if (u == "kilometers" || u == "km")
        return meters / 1000.0;
    if (u == "feet" || u == "ft")
        return meters * 3.280839895;
    if (u == "miles" || u == "mi")
        return meters / 1609.344;
    if (u == "nautical_miles" || u == "nautical miles" || u == "nmi" || u == "nm")
        return meters / 1852.0;
    if (u == "degrees" || u == "deg" || u == "°") {
        const double R = 6371000.0;
        return (meters / R) * (180.0 / M_PI);
    }
    return meters; // meters
}

QString MeasureApi::unitSuffix(const QString& unit) const
{
    const QString u = unit.toLower().trimmed();
    if (u == "kilometers" || u == "km") return QStringLiteral("km");
    if (u == "feet" || u == "ft") return QStringLiteral("ft");
    if (u == "miles" || u == "mi") return QStringLiteral("mi");
    if (u == "nautical_miles" || u == "nautical miles" || u == "nmi" || u == "nm") return QStringLiteral("nmi");
    if (u == "degrees" || u == "deg" || u == "°") return QStringLiteral("°");
    return QStringLiteral("m");
}

void MeasureApi::setDisplayUnit(const QString& unit)
{
    QString u = unit.toLower().trimmed();
    if (u.isEmpty())
        u = QStringLiteral("meters");
    if (u == "km") u = QStringLiteral("kilometers");
    if (u == "ft") u = QStringLiteral("feet");
    if (u == "mi") u = QStringLiteral("miles");
    if (u == "nmi" || u == "nm" || u == "nautical miles") u = QStringLiteral("nautical_miles");
    if (u == "deg" || u == "°") u = QStringLiteral("degrees");
    if (u == m_displayUnit)
        return;
    m_displayUnit = u;
    emit displayUnitChanged(m_displayUnit);
}

double MeasureApi::convertFromMeters(double meters) const
{
    return convertFromMeters(meters, m_displayUnit);
}

QString MeasureApi::currentUnitSuffix() const
{
    return unitSuffix(m_displayUnit);
}

double MeasureApi::distanceMeters(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371000.0;
    const double toRad = M_PI / 180.0;
    double dLat = (lat2 - lat1) * toRad;
    double dLon = (lon2 - lon1) * toRad;
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0)
               + std::cos(lat1 * toRad) * std::cos(lat2 * toRad)
                     * std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}
