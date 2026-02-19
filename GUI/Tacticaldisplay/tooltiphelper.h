
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

private:
    static const QMap<QString, QString> tooltipLabels;
    static double calculateCompletionTime(const MeshEntry& entry);
    static QString formatTime(double seconds);
    static int findNearestUpcomingWaypoint(const MeshEntry& entry, const QPointF& currentPos);
    static double calculateHaversineDistance(const QPointF& pos1, const QPointF& pos2);
};

#endif // TOOLTIPHELPER_H
