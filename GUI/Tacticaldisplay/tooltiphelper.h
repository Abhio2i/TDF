/* =============================================================================
 * FILE:         TooltipHelper.h
 * MODULE:       Tooltip Helper
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the TooltipHelper class which provides static helper
 *               functions for generating and displaying rich HTML tooltips
 *               for entities on the tactical display. Supports configurable
 *               fields (basic info, dynamic model data, position, speed),
 *               HTML formatting, compact mode, and tooltip display.
 *
 * REQUIREMENTS: REQ-TOOLTIP-010  Generate entity tooltip with configurable fields
 *               REQ-TOOLTIP-011  Extract basic entity information (ID, name, type)
 *               REQ-TOOLTIP-012  Extract dynamic model information (waypoint ETA,
 *                                distance to next, completion time)
 *               REQ-TOOLTIP-013  Extract position information (latitude, longitude,
 *                                altitude, heading)
 *               REQ-TOOLTIP-014  Calculate speed from velocity vector
 *               REQ-TOOLTIP-015  Format tooltip as HTML or compact text
 *               REQ-TOOLTIP-016  Display tooltip at cursor position
 *               REQ-TOOLTIP-017  Provide list of all available tooltip fields
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-TOOLTIP-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef TOOLTIPHELPER_H
#define TOOLTIPHELPER_H

#include <QString>
#include <QPoint>
#include <QToolTip>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <cmath>
#include <core/Hierarchy/EntityProfiles/platform.h>
#include "canvaswidget.h"

class MeshEntry;
class DynamicModel;

class TooltipHelper
{
public:
    // Main function to generate tooltip with configurable fields
    static QString generateEntityTooltip(const MeshEntry& entry,
                                         const QSet<QString>& activeFields = QSet<QString>());

    // Helper functions
    static QString getEntityBasicInfo(const MeshEntry& entry);
    static QMap<QString, QString> getDynamicModelInfo(const MeshEntry& entry);
    static QString getPositionInfo(const MeshEntry& entry);
    static QString calculateSpeedFromVelocity(const QVector3D* velocity);

    // Formatting functions
    static QString formatTooltipHTML(const QString& name,
                                     const QMap<QString, QString>& data,
                                     const QSet<QString>& activeFields);
    static QString formatCompactTooltip(const QString& name,
                                        const QMap<QString, QString>& data);

    // Display tooltip
    static void showTooltip(const QString& tooltipText, QWidget* parent = nullptr);

    // Get all available tooltip fields
    static QStringList getAvailableFields();
    // static void runUnitTestsOnce();

private:
    static const QMap<QString, QString> tooltipLabels;
    static double calculateCompletionTime(const MeshEntry& entry);
    static QString formatTime(double seconds);
    static int findNearestUpcomingWaypoint(const MeshEntry& entry, const QPointF& currentPos);
    static double calculateHaversineDistance(const QPointF& pos1, const QPointF& pos2);
};

#endif // TOOLTIPHELPER_H
