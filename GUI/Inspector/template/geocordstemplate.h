/* =============================================================================
 * FILE:         geocordstemplate.h
 * MODULE:       Geocoordinates Template Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the GeocordsTemplate class which provides a widget for
 *               managing geocoordinate templates. It interfaces with the
 *               Inspector panel to set up geocoordinate cells in a table,
 *               maintain connected entity IDs, template names, and emit
 *               value changes when a coordinate is modified.
 *
 * REQUIREMENTS: REQ-GEO-010  Geocoordinate template management widget
 *               REQ-GEO-011  Setup geocoordinate cell in table widget
 *               REQ-GEO-012  Maintain connected ID and template name
 *               REQ-GEO-013  Signal valueChanged on coordinate modification
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-GEO-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef GEOCORDSTEMPLATE_H
#define GEOCORDSTEMPLATE_H

#include <QWidget>                                // For widget base class
#include <QJsonObject>                            // For JSON object handling
#include <QTableWidget>                           // For table widget

// Forward declaration
class Inspector;

// %%% Class Definition %%%
/* Widget for geocoordinates template management */
class GeocordsTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize geocoordinates template with Inspector reference
    explicit GeocordsTemplate(Inspector *inspector, QWidget *parent = nullptr);  // CHANGED
    // Setup geocoordinates cell in table
    void setupGeocordsCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
    // Set connected ID
    void setConnectedID(const QString &id) { connectedID = id; }
    // Set template name
    void setName(const QString &n) { name = n; }
    // Constant for row height
    static constexpr int ROW_HEIGHT = 30;
  void setMainID(const QString &id) { mainID = id; }
signals:
    // Signal value change
    void valueChanged(QString ID, QString name, QJsonObject delta);

private:
    // %%% Data Members %%%
    // Connected item ID
    QString connectedID;
    // Template name
    QString name;
    // Inspector reference
    Inspector *inspectorRef;
     QString mainID;
};

#endif // GEOCORDSTEMPLATE_H
