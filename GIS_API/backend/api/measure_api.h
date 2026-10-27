
#ifndef MEASURE_API_H
#define MEASURE_API_H

#include <QObject>
#include <QPointF>
#include <QVector>
#include <QString>

class MeasureApi : public QObject {
    Q_OBJECT

public:
    static MeasureApi* instance();

    // Session control
    Q_INVOKABLE void startMeasuring();
    Q_INVOKABLE void stopMeasuring();
    Q_INVOKABLE bool isMeasuring() const { return m_active; }

    // Points (geo: x = longitude, y = latitude)
    Q_INVOKABLE void addPoint(double lon, double lat);
    Q_INVOKABLE void addPoint(const QPointF& geoPos);
    Q_INVOKABLE void clearPoints();
    Q_INVOKABLE int pointCount() const { return m_points.size(); }
    Q_INVOKABLE QVector<QPointF> getPoints() const { return m_points; }

    // Distances (meters)
    Q_INVOKABLE QVector<double> getSegmentMeters() const;
    Q_INVOKABLE double getTotalMeters() const;
    Q_INVOKABLE double segmentMetersAt(int index) const;

    // Unit conversion helpers
    Q_INVOKABLE double convertFromMeters(double meters, const QString& unit) const;
    Q_INVOKABLE QString unitSuffix(const QString& unit) const;

    // Display unit (dialog + map overlay sync)
    Q_INVOKABLE void setDisplayUnit(const QString& unit);
    Q_INVOKABLE QString displayUnit() const { return m_displayUnit; }
    Q_INVOKABLE double convertFromMeters(double meters) const;
    Q_INVOKABLE QString currentUnitSuffix() const;

    // Static great-circle distance
    Q_INVOKABLE static double distanceMeters(double lat1, double lon1,
                                             double lat2, double lon2);

signals:
    void measuringChanged(bool active);
    void pointsChanged();
    void cleared();
    void displayUnitChanged(const QString& unit);

private:
    explicit MeasureApi(QObject* parent = nullptr);
    ~MeasureApi() override;
    static MeasureApi* m_instance;
    bool m_active = false;
    QVector<QPointF> m_points;
    QString m_displayUnit = QStringLiteral("meters");
};

#endif // MEASURE_API_H
