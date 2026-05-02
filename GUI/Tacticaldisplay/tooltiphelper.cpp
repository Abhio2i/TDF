/* =============================================================================
 * FILE:         tooltiphelper.cpp
 * MODULE:       Tooltip Helper
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the TooltipHelper class which provides static helper
 *               functions for generating and displaying rich HTML tooltips
 *               for entities on the tactical display. Supports configurable
 *               fields (basic info, dynamic model data, position, speed),
 *               HTML formatting, compact mode, and tooltip display.
 *
 * REQUIREMENTS: Implements REQ-TOOLTIP-010 through REQ-TOOLTIP-017
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-TOOLTIP-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "tooltiphelper.h"
#include <QVector3D>
#include <cmath>
#include <core/Hierarchy/EntityProfiles/platform.h>
#include <core/Hierarchy/Components/dynamicmodel.h>
#include "tests/tooltiphelpertest/tooltiphelper_test.h"


// Tooltip labels mapping
const QMap<QString, QString> TooltipHelper::tooltipLabels = {
    {"Name", "Name"},
    {"Speed", "Current Speed"},
    {"Move Speed", "Move Speed"},
    {"Min Speed", "Min Speed"},
    {"Max Speed", "Max Speed"},
    {"Acceleration", "Acceleration"},
    {"Deceleration", "Deceleration"},
    {"Turn Rate", "Turn Rate"},
    {"Climb Rate", "Climb Rate"},
    {"Dive Rate", "Dive Rate"},
    {"Latitude", "Latitude"},
    {"Longitude", "Longitude"},
    {"Altitude", "Altitude"},
     {"Trajectory ETA", "ETA to Complete"}
};

QStringList TooltipHelper::getAvailableFields()
{
    return QStringList{
        "Name",
        "Speed",
        "Move Speed",
        "Min Speed",
        "Max Speed",
        "Acceleration",
        "Deceleration",
        "Turn Rate",
        "Climb Rate",
        "Dive Rate",
        "Latitude",
        "Longitude",
        "Altitude",
        "Trajectory ETA"
    };
}

QString TooltipHelper::generateEntityTooltip(const MeshEntry& entry,
                                             const QSet<QString>& activeFields)
{
    QString name = entry.name;
    QMap<QString, QString> data = getDynamicModelInfo(entry);

    QString positionInfo = getPositionInfo(entry);
    if (!positionInfo.isEmpty()) {
        QStringList posParts = positionInfo.split(",");
        if (posParts.size() >= 3) {
            data["Latitude"] = posParts[0];
            data["Longitude"] = posParts[1];
            data["Altitude"] = posParts[2];
        }
    }
    if (entry.trajectory && !entry.trajectory->Trajectories.empty()) {
        double completionTime = calculateCompletionTime(entry);
        if (completionTime > 0) {
            data["Trajectory ETA"] = formatTime(completionTime);
        }
    }

    data["Name"] = name;
    return formatTooltipHTML(name, data, activeFields);
}

QMap<QString, QString> TooltipHelper::getDynamicModelInfo(const MeshEntry& entry)
{
    QMap<QString, QString> data;

    if (entry.platform && entry.platform->dynamicModel) {
        DynamicModel* dm = entry.platform->dynamicModel;

        if (dm->moveSpeed >= 0) {
            data["Move Speed"] = QString::number(dm->moveSpeed, 'f', 2) + " km/h";
        }

        if (dm->minSpeed >= 0) {
            data["Min Speed"] = QString::number(dm->minSpeed, 'f', 2) + " km/h";
        }

        if (dm->maxSpeed >= 0) {
            data["Max Speed"] = QString::number(dm->maxSpeed, 'f', 2) + " km/h";
        }

        // Acceleration data
        if (dm->Acceleration >= 0) {
            data["Acceleration"] = QString::number(dm->Acceleration, 'f', 2) + " m/s²";
        }

        if (dm->Decceleration >= 0) {
            data["Deceleration"] = QString::number(dm->Decceleration, 'f', 2) + " m/s²";
        }

        // Current speed
        if (dm->currentSpeed >= 0) {
            data["Speed"] = QString::number(dm->currentSpeed, 'f', 2) + " km/h";
        } else if (entry.velocity) {
            data["Speed"] = calculateSpeedFromVelocity(entry.velocity);
        }

        // Other parameters
        if (dm->turnRate >= 0) {
            data["Turn Rate"] = QString::number(dm->turnRate, 'f', 2) + " deg/s";
        }

        if (dm->climbRate >= 0) {
            data["Climb Rate"] = QString::number(dm->climbRate, 'f', 2) + " ft/min";
        }

        if (dm->diveRate >= 0) {
            data["Dive Rate"] = QString::number(dm->diveRate, 'f', 2) + " ft/min";
        }
    }

    return data;
}

QString TooltipHelper::getPositionInfo(const MeshEntry& entry)
{
    if (!entry.coreTransform) {
        return "";
    }

    QString latitude = QString::number(entry.coreTransform->getLatitude(), 'f', 6);
    QString longitude = QString::number(entry.coreTransform->getLongitude(), 'f', 6);
    QString altitude = QString::number(entry.coreTransform->getAltitude(), 'f', 2) + " ft";

    return QString("%1,%2,%3").arg(latitude, longitude, altitude);
}

QString TooltipHelper::calculateSpeedFromVelocity(const QVector3D* velocity)
{
    if (!velocity) return "N/A";

    float speedValue = std::sqrt(
        velocity->x() * velocity->x() +
        velocity->y() * velocity->y() +
        velocity->z() * velocity->z()
        );

    return QString::number(speedValue * 3.6, 'f', 2) + " km/h";
}

QString TooltipHelper::formatTooltipHTML(const QString& name,
                                         const QMap<QString, QString>& data,
                                         const QSet<QString>& activeFields)
{
    QStringList tableRows;

    // Define ordered keys
    QStringList orderedKeys = {
        "Name", "Speed", "Move Speed", "Min Speed", "Max Speed",
        "Acceleration", "Deceleration", "Turn Rate", "Climb Rate",
        "Dive Rate", "Latitude", "Longitude", "Altitude","Trajectory ETA"
    };

    bool showAll = activeFields.isEmpty();

    for (const QString& key : orderedKeys) {
        if (!showAll && !activeFields.contains(key)) {
            continue;
        }

        if (data.contains(key)) {
            QString label = tooltipLabels.value(key, key);
            QString value = data[key];

            tableRows.append(QString(
                                 "<tr>"
                                 "  <td style='padding: 3px 8px 3px 0; font-weight: bold; white-space: nowrap; color: #CCCCCC;'>%1:</td>"
                                 "  <td style='padding: 3px 0; color: white;'>%2</td>"
                                 "</tr>"
                                 ).arg(label, value));
        }
    }

    // If no rows, return empty
    if (tableRows.isEmpty()) {
        return QString();
    }

    // Create HTML tooltip - DARK THEME COLORS
    QString html = QString(
                       "<div style='background-color: #1A3652; color: white; border: 1px solid #0078D4; "
                       "padding: 10px; border-radius: 3px; min-width: 280px; font-family: Arial, sans-serif; "
                       "font-size: 12px; box-shadow: 2px 2px 5px rgba(0,0,0,0.5);'>"
                       "<table style='border-collapse: collapse; width: 100%;'>"
                       "%1"
                       "</table>"
                       "</div>"
                       ).arg(tableRows.join(""));

    return html;
}

QString TooltipHelper::formatCompactTooltip(const QString& name,
                                            const QMap<QString, QString>& data)
{
    QStringList lines;
    lines.append(QString("<b>%1</b>").arg(name));

    QStringList importantKeys = {"Speed", "Move Speed", "Latitude", "Longitude", "Altitude"};

    for (const QString& key : importantKeys) {
        if (data.contains(key)) {
            QString label = tooltipLabels.value(key, key);
            lines.append(QString("%1: %2").arg(label, data[key]));
        }
    }

    return lines.join("<br/>");
}

void TooltipHelper::showTooltip(const QString& tooltipText, QWidget* parent)
{
    if (tooltipText.isEmpty()) return;
    QPoint globalPos = QCursor::pos() + QPoint(15, 15);
    if (parent) {
        parent->setStyleSheet(
            "QToolTip {"
            "    background-color: transparent;"
            "    color: transparent;"
            "    border: none;"
            "    padding: 0;"
            "    opacity: 0;"
            "}"
            );
    }
    QToolTip::showText(globalPos, tooltipText, parent, QRect());
}

double TooltipHelper::calculateCompletionTime(const MeshEntry& entry)
{
    if (!entry.trajectory || entry.trajectory->Trajectories.empty()) {
        return -1.0;
    }

    if (!entry.platform || !entry.platform->dynamicModel) {
        return -1.0;
    }

    double currentSpeed = entry.platform->dynamicModel->currentSpeed;

    if (currentSpeed <= 0) {
        currentSpeed = entry.platform->dynamicModel->moveSpeed;
    }

    if (currentSpeed <= 0) {
        return -1.0;
    }

    QPointF currentPos(entry.coreTransform->getLongitude(),
                       entry.coreTransform->getLatitude());

    int nearestWaypointIndex = findNearestUpcomingWaypoint(entry, currentPos);

    if (nearestWaypointIndex < 0) {
        return 0.0;
    }

    double totalTime = 0.0;

    Waypoints* nextWp = entry.trajectory->Trajectories[nearestWaypointIndex];
    QPointF nextWpPos(nextWp->position->z, nextWp->position->x);

    double distanceToNext = calculateHaversineDistance(currentPos, nextWpPos);
    double timeToNext = (distanceToNext / currentSpeed) * 3600.0; // seconds
    totalTime += timeToNext;

    for (size_t i = nearestWaypointIndex; i < entry.trajectory->Trajectories.size() - 1; ++i) {
        Waypoints* wp1 = entry.trajectory->Trajectories[i];
        Waypoints* wp2 = entry.trajectory->Trajectories[i + 1];
        QPointF pos1(wp1->position->z, wp1->position->x);
        QPointF pos2(wp2->position->z, wp2->position->x);
        double distance = calculateHaversineDistance(pos1, pos2);
        double speedForSegment = (wp1->speed > 0) ? wp1->speed : currentSpeed;
        double segmentTime = (distance / speedForSegment) * 3600.0;
        totalTime += segmentTime;
    }
    return totalTime;
}


double TooltipHelper::calculateHaversineDistance(const QPointF& pos1, const QPointF& pos2)
{
    const double R = 6371.0;
    double lat1 = pos1.y() * M_PI / 180.0;
    double lon1 = pos1.x() * M_PI / 180.0;
    double lat2 = pos2.y() * M_PI / 180.0;
    double lon2 = pos2.x() * M_PI / 180.0;
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;
    double a = std::sin(dlat/2) * std::sin(dlat/2) +
               std::cos(lat1) * std::cos(lat2) *
                   std::sin(dlon/2) * std::sin(dlon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c; // Distance in kilometers
}
QString TooltipHelper::formatTime(double seconds)
{
    if (seconds < 0) return "N/A";

    int hours = static_cast<int>(seconds / 3600);
    int minutes = static_cast<int>((seconds - hours * 3600) / 60);
    int secs = static_cast<int>(seconds) % 60;

    if (hours > 0) {
        return QString("%1h %2m %3s").arg(hours).arg(minutes).arg(secs);
    } else if (minutes > 0) {
        return QString("%1m %2s").arg(minutes).arg(secs);
    } else {
        return QString("%1s").arg(secs);
    }
}

int TooltipHelper::findNearestUpcomingWaypoint(const MeshEntry& entry, const QPointF& currentPos)
{
    if (!entry.trajectory || entry.trajectory->Trajectories.empty()) {
        return -1;
    }

    Waypoints* lastWp = entry.trajectory->Trajectories.back();
    QPointF lastWpPos(lastWp->position->z, lastWp->position->x);
    double distToLast = calculateHaversineDistance(currentPos, lastWpPos);
    if (distToLast < 0.05) {
        return -1;
    }


    int currentTargetIndex = entry.trajectory->current;

    if (currentTargetIndex < 0 || currentTargetIndex >= entry.trajectory->Trajectories.size()) {
        double minDistance = std::numeric_limits<double>::max();
        int nearestIndex = 0;

        for (size_t i = 0; i < entry.trajectory->Trajectories.size(); ++i) {
            Waypoints* wp = entry.trajectory->Trajectories[i];
            QPointF wpPos(wp->position->z, wp->position->x);
            double dist = calculateHaversineDistance(currentPos, wpPos);

            if (dist < minDistance) {
                minDistance = dist;
                nearestIndex = i;
            }
        }
        return nearestIndex;
    }

    return currentTargetIndex;
}
