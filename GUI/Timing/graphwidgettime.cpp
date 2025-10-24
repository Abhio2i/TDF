/* ========================================================================= */
/* File: graphwidgettime.cpp                                              */
/* Purpose: Implements timeline graph widget for entity activity           */
/* ========================================================================= */

#include "graphwidgettime.h"                       // For timeline graph widget class
#include <QDebug>                                  // For debug logging
#include <algorithm>                               // For std::sort

// %%% JSON Data %%%
/* Sample JSON data for entity timelines */
const QString GraphWidgetTime::JSON_DATA = R"(
[
    {"entity": "Entity 1", "startTime": "00:00:10", "endTime": "00:00:20", "active": "true", "team": "blue"},
    {"entity": "Entity 2", "startTime": "00:00:20", "endTime": "00:00:30", "active": "true", "team": "blue"},
    {"entity": "Entity 3", "startTime": "00:00:30", "endTime": "", "active": "false", "team": "blue"},
    {"entity": "Entity 4", "startTime": "00:00:40", "endTime": "00:00:50", "active": "true", "team": "blue"},
    {"entity": "Entity 5", "startTime": "00:00:50", "endTime": "00:01:00", "active": "true", "team": "blue"},
    {"entity": "Entity 6", "startTime": "00:01:00", "endTime": "", "active": "false", "team": "red"},
    {"entity": "Entity 7", "startTime": "00:01:10", "endTime": "00:01:20", "active": "true", "team": "red"},
    {"entity": "Entity 8", "startTime": "00:01:20", "endTime": "00:01:30", "active": "true", "team": "red"},
    {"entity": "Entity 9", "startTime": "00:01:30", "endTime": "00:01:40", "active": "true", "team": "red"},
    {"entity": "Entity 10", "startTime": "00:01:40", "endTime": "00:01:50", "active": "true", "team": "red"}
]
)";

// %%% Constructor %%%
/* Initialize timeline graph widget */
GraphWidgetTime::GraphWidgetTime(QWidget *parent)
    : QWidget(parent)
{
    // Set minimum dimensions
    setMinimumHeight(100);
    setMinimumWidth(400);
    // Initialize selected team and timer
    selectedTeam = "blue";
    simulationTimer = nullptr;
}

// %%% Data Management %%%
/* Set timing data and selected team */
void GraphWidgetTime::setTimingData(const QList<QMap<QString, QString>> &data, const QString &selectedTeam)
{
    this->selectedTeam = selectedTeam;
    timingData.clear();
    // Handle team selection
    if (selectedTeam == "both") {
        timingData = data;
        std::sort(timingData.begin(), timingData.end(), [](const QMap<QString, QString> &a, const QMap<QString, QString> &b) {
            return a["entity"] < b["entity"];
        });
    } else {
        for (const auto &entry : data) {
            if (teamMap.contains(entry["entity"]) && teamMap[entry["entity"]] == selectedTeam) {
                timingData.append(entry);
            }
        }
    }
    // Adjust widget height based on data
    int numLines = selectedTeam == "both" ? calculateGroupedLines() : timingData.size();
    int minHeight = numLines == 0 ? 100 : 50 + 20 * numLines;
    setMinimumHeight(minHeight);
    // Update time range and repaint
    calculateMinMax();
    update();
    qDebug() << "GraphWidgetTime: Set data for" << timingData.size() << "entities of team" << selectedTeam << ", height:" << minHeight;
}

/* Calculate number of grouped lines for both teams */
int GraphWidgetTime::calculateGroupedLines()
{
    if (selectedTeam != "both") {
        return timingData.size();
    }
    // Split data by team
    QList<QMap<QString, QString>> teamA, teamB;
    for (const auto &data : timingData) {
        if (teamMap.value(data["entity"], "gray") == "blue") {
            teamA.append(data);
        } else if (teamMap.value(data["entity"], "gray") == "red") {
            teamB.append(data);
        }
    }
    return qMin(teamA.size(), teamB.size());
}

/* Set team division map */
void GraphWidgetTime::setTeamDivision(const QMap<QString, QString> &teamMap)
{
    this->teamMap = teamMap;
    update();
}

/* Calculate minimum and maximum times */
void GraphWidgetTime::calculateMinMax()
{
    minTimeCalc = QTime(23, 59, 59);
    maxTimeCalc = QTime(0, 0, 0);
    for (const auto &data : timingData) {
        QTime startTime = QTime::fromString(data["startTime"], "hh:mm:ss");
        QTime endTime = QTime::fromString(data["endTime"], "hh:mm:ss");
        if (startTime.isValid() && startTime < minTimeCalc) minTimeCalc = startTime;
        if (endTime.isValid() && endTime > maxTimeCalc) maxTimeCalc = endTime;
        if (startTime.isValid() && startTime > maxTimeCalc) maxTimeCalc = startTime;
    }
    // Add padding to time range
    minTimeCalc = minTimeCalc.addSecs(-10);
    maxTimeCalc = maxTimeCalc.addSecs(10);
}

// %%% Simulation Control %%%
/* Start simulation timer */
void GraphWidgetTime::startSimulation()
{
    calculateMinMax();
    currentSimulationTime = minTimeCalc;
    if (!simulationTimer) {
        simulationTimer = new QTimer(this);
        connect(simulationTimer, &QTimer::timeout, this, [this]() {
            currentSimulationTime = currentSimulationTime.addSecs(1);
            update();
        });
    }
    simulationTimer->start(200); // 200ms per second sim time
}

// %%% Painting %%%
/* Paint the timeline graph */
void GraphWidgetTime::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // Get widget dimensions
    int width = this->width();
    int height = this->height();
    // Draw title
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    QString teamName = (selectedTeam == "blue") ? "Blue" : (selectedTeam == "red") ? "Red" : "Both";
    painter.drawText(QRect(0, 10, width, 20), Qt::AlignCenter,
                     QString("Entity Timeline Graph (%1 Team%2)").arg(teamName).arg(selectedTeam == "both" ? "s" : ""));
    // Set up axis
    int axisY = height - 50;
    int axisStartX = 100;
    int axisEndX = width - 50;
    QTime minTime = minTimeCalc;
    QTime maxTime = maxTimeCalc;
    // Adjust max time for simulation
    if (simulationTimer && simulationTimer->isActive()) {
        for (const auto &data : timingData) {
            QTime startTime = QTime::fromString(data["startTime"], "hh:mm:ss");
            if (data["endTime"].isEmpty() && startTime <= currentSimulationTime) {
                maxTime = qMax(maxTime, currentSimulationTime);
            }
        }
    }
    int totalSeconds = minTime.secsTo(maxTime);
    if (totalSeconds <= 0) totalSeconds = 1;
    // Draw time grid
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    for (int i = 0; i <= 10; i++) {
        int x = axisStartX + i * (axisEndX - axisStartX) / 10;
        painter.drawLine(x, 30, x, axisY);
    }
    // Draw axis line
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(axisStartX, axisY, axisEndX, axisY);
    // Draw time labels
    painter.setPen(Qt::black);
    for (int i = 0; i <= 10; i++) {
        int x = axisStartX + i * (axisEndX - axisStartX) / 10;
        QTime time = minTime.addSecs(i * totalSeconds / 10);
        painter.drawText(QRect(x - 25, axisY + 10, 50, 20), Qt::AlignCenter, time.toString("mm:ss"));
    }
    // Draw entities
    int entitySpacing = 20;
    int currentY = axisY - 10;
    painter.setFont(QFont("Arial", 9));
    if (selectedTeam == "both") {
        // Split and sort teams
        QList<QMap<QString, QString>> teamA, teamB;
        for (const auto &data : timingData) {
            if (teamMap.value(data["entity"], "gray") == "blue") {
                teamA.append(data);
            } else if (teamMap.value(data["entity"], "gray") == "red") {
                teamB.append(data);
            }
        }
        std::sort(teamA.begin(), teamA.end(), [](const QMap<QString, QString> &a, const QMap<QString, QString> &b) {
            return a["entity"] < b["entity"];
        });
        std::sort(teamB.begin(), teamB.end(), [](const QMap<QString, QString> &a, const QMap<QString, QString> &b) {
            return a["entity"] < b["entity"];
        });
        // Draw paired entities
        int numPairs = qMin(teamA.size(), teamB.size());
        for (int i = 0; i < numPairs; i++) {
            int y = currentY - i * entitySpacing;
            painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
            painter.drawLine(axisStartX, y, axisEndX, y);
            // Draw Team A entity
            if (i < teamA.size()) {
                const auto &data = teamA[i];
                QString entity = data["entity"];
                QTime startTime = QTime::fromString(data["startTime"], "hh:mm:ss");
                QTime endTime = QTime::fromString(data["endTime"], "hh:mm:ss");
                bool isIncomplete = data["endTime"].isEmpty();
                QTime effectiveEnd = endTime;
                bool isIncompleteNow = isIncomplete;
                bool show = true;
                if (simulationTimer && simulationTimer->isActive()) {
                    if (startTime > currentSimulationTime) {
                        show = false;
                    } else {
                        if (isIncomplete || (endTime.isValid() && endTime > currentSimulationTime)) {
                            effectiveEnd = currentSimulationTime;
                            isIncompleteNow = true;
                        } else {
                            effectiveEnd = endTime;
                            isIncompleteNow = false;
                        }
                    }
                }
                if (!effectiveEnd.isValid()) effectiveEnd = startTime.addSecs(10);
                int startX = axisStartX + (minTime.secsTo(startTime) * (axisEndX - axisStartX)) / totalSeconds;
                int endX = axisStartX + (minTime.secsTo(effectiveEnd) * (axisEndX - axisStartX)) / totalSeconds;
                painter.setPen(QPen(Qt::blue, 3));
                if (show) {
                    painter.drawLine(startX, y, endX, y);
                    painter.setBrush(Qt::blue);
                    painter.drawEllipse(startX - 3, y - 3, 6, 6);
                    if (!isIncompleteNow) {
                        painter.drawEllipse(endX - 3, y - 3, 6, 6);
                    } else {
                        painter.setPen(QPen(QColor("pink"), 3));
                        painter.drawLine(endX - 5, y - 5, endX + 5, y + 5);
                        painter.drawLine(endX - 5, y + 5, endX + 5, y - 5);
                    }
                }
                painter.setPen(Qt::black);
                painter.drawText(QRect(10, y - 15, 80, 20), Qt::AlignLeft, entity);
            }
            // Draw Team B entity
            if (i < teamB.size()) {
                const auto &data = teamB[i];
                QString entity = data["entity"];
                QTime startTime = QTime::fromString(data["startTime"], "hh:mm:ss");
                QTime endTime = QTime::fromString(data["endTime"], "hh:mm:ss");
                bool isIncomplete = data["endTime"].isEmpty();
                QTime effectiveEnd = endTime;
                bool isIncompleteNow = isIncomplete;
                bool show = true;
                if (simulationTimer && simulationTimer->isActive()) {
                    if (startTime > currentSimulationTime) {
                        show = false;
                    } else {
                        if (isIncomplete || (endTime.isValid() && endTime > currentSimulationTime)) {
                            effectiveEnd = currentSimulationTime;
                            isIncompleteNow = true;
                        } else {
                            effectiveEnd = endTime;
                            isIncompleteNow = false;
                        }
                    }
                }
                if (!effectiveEnd.isValid()) effectiveEnd = startTime.addSecs(10);
                int startX = axisStartX + (minTime.secsTo(startTime) * (axisEndX - axisStartX)) / totalSeconds;
                int endX = axisStartX + (minTime.secsTo(effectiveEnd) * (axisEndX - axisStartX)) / totalSeconds;
                painter.setPen(QPen(Qt::red, 3));
                if (show) {
                    painter.drawLine(startX, y, endX, y);
                    painter.setBrush(Qt::red);
                    painter.drawEllipse(startX - 3, y - 3, 6, 6);
                    if (!isIncompleteNow) {
                        painter.drawEllipse(endX - 3, y - 3, 6, 6);
                    } else {
                        painter.setPen(QPen(QColor("pink"), 3));
                        painter.drawLine(endX - 5, y - 5, endX + 5, y + 5);
                        painter.drawLine(endX - 5, y + 5, endX + 5, y - 5);
                    }
                }
                painter.setPen(Qt::black);
                painter.drawText(QRect(10, y + 5, 80, 20), Qt::AlignLeft, entity);
            }
        }
    } else {
        // Draw single-team entities
        int numLines = timingData.size();
        for (int i = 0; i < numLines; i++) {
            int y = currentY - i * entitySpacing;
            painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
            painter.drawLine(axisStartX, y, axisEndX, y);
            const auto &data = timingData[i];
            QString entity = data["entity"];
            QTime startTime = QTime::fromString(data["startTime"], "hh:mm:ss");
            QTime endTime = QTime::fromString(data["endTime"], "hh:mm:ss");
            bool isIncomplete = data["endTime"].isEmpty();
            QTime effectiveEnd = endTime;
            bool isIncompleteNow = isIncomplete;
            bool show = true;
            if (simulationTimer && simulationTimer->isActive()) {
                if (startTime > currentSimulationTime) {
                    show = false;
                } else {
                    if (isIncomplete || (endTime.isValid() && endTime > currentSimulationTime)) {
                        effectiveEnd = currentSimulationTime;
                        isIncompleteNow = true;
                    } else {
                        effectiveEnd = endTime;
                        isIncompleteNow = false;
                    }
                }
            }
            if (!effectiveEnd.isValid()) effectiveEnd = startTime.addSecs(10);
            int startX = axisStartX + (minTime.secsTo(startTime) * (axisEndX - axisStartX)) / totalSeconds;
            int endX = axisStartX + (minTime.secsTo(effectiveEnd) * (axisEndX - axisStartX)) / totalSeconds;
            QColor teamColor = Qt::gray;
            if (teamMap.contains(entity)) {
                if (teamMap[entity] == "blue") teamColor = Qt::blue;
                else if (teamMap[entity] == "red") teamColor = Qt::red;
            }
            painter.setPen(QPen(teamColor, 3));
            if (show) {
                painter.drawLine(startX, y, endX, y);
                painter.setBrush(teamColor);
                painter.drawEllipse(startX - 3, y - 3, 6, 6);
                if (!isIncompleteNow) {
                    painter.drawEllipse(endX - 3, y - 3, 6, 6);
                } else {
                    painter.setPen(QPen(QColor("pink"), 3));
                    painter.drawLine(endX - 5, y - 5, endX + 5, y + 5);
                    painter.drawLine(endX - 5, y + 5, endX + 5, y - 5);
                }
            }
            painter.setPen(Qt::black);
            painter.drawText(QRect(10, y - 15, 80, 20), Qt::AlignLeft, entity);
        }
    }
}
