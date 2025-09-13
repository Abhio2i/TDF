
#ifndef GRAPHWIDGETTIME_H
#define GRAPHWIDGETTIME_H

#include <QWidget>
#include <QMap>
#include <QList>
#include <QString>
#include <QTime>
#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <QPen>
#include <QBrush>

class GraphWidgetTime : public QWidget {
    Q_OBJECT

public:
    explicit GraphWidgetTime(QWidget *parent = nullptr);
    void setTimingData(const QList<QMap<QString, QString>> &data, const QString &selectedTeam);
    int calculateGroupedLines();
    void setTeamDivision(const QMap<QString, QString> &teamMap);
    void calculateMinMax();
    void startSimulation();
    static const QString JSON_DATA;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<QMap<QString, QString>> timingData;
    QMap<QString, QString> teamMap;
    QString selectedTeam;
    QTimer *simulationTimer;
    QTime minTimeCalc;
    QTime maxTimeCalc;
    QTime currentSimulationTime;
};

#endif // GRAPHWIDGETTIME_H
