/* ========================================================================= */
/* File: graphwidgettime.h                                                  */
/* Purpose: Defines widget for time-based graph visualization                */
/* ========================================================================= */

#ifndef GRAPHWIDGETTIME_H
#define GRAPHWIDGETTIME_H

#include <QWidget>                                // For widget base class
#include <QMap>                                   // For map container
#include <QList>                                  // For list container
#include <QString>                                // For string handling
#include <QTime>                                  // For time handling
#include <QTimer>                                 // For timer functionality
#include <QPainter>                               // For painting operations
#include <QPaintEvent>                            // For paint event handling
#include <QFont>                                  // For font settings
#include <QPen>                                   // For pen settings
#include <QBrush>                                 // For brush settings

// %%% Class Definition %%%
/* Widget for time-based graph display */
class GraphWidgetTime : public QWidget {
    Q_OBJECT

public:
    // Initialize graph widget
    explicit GraphWidgetTime(QWidget *parent = nullptr);
    // Set timing data and selected team
    void setTimingData(const QList<QMap<QString, QString>> &data, const QString &selectedTeam);
    // Calculate grouped lines
    int calculateGroupedLines();
    // Set team division mapping
    void setTeamDivision(const QMap<QString, QString> &teamMap);
    // Calculate min and max times
    void calculateMinMax();
    // Start simulation
    void startSimulation();
    // JSON data constant
    static const QString JSON_DATA;

protected:
    // Handle paint events
    void paintEvent(QPaintEvent *event) override;

private:
    // %%% Data Members %%%
    // Timing data list
    QList<QMap<QString, QString>> timingData;
    // Team division map
    QMap<QString, QString> teamMap;
    // Selected team name
    QString selectedTeam;
    // Simulation timer
    QTimer *simulationTimer;
    // Minimum calculated time
    QTime minTimeCalc;
    // Maximum calculated time
    QTime maxTimeCalc;
    // Current simulation time
    QTime currentSimulationTime;
};

#endif // GRAPHWIDGETTIME_H
