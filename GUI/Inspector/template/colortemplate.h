
/* =============================================================================
 * FILE:         colortemplate.h
 * MODULE:       Color Template Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the ColorTemplate class which provides a widget for
 *               managing color templates. It interfaces with the Inspector
 *               panel to set up color cells in a table, maintain connected
 *               entity IDs, template names, and emit value changes when a
 *               color is modified.
 *
 * REQUIREMENTS: REQ-COLOR-010  Color template management widget
 *               REQ-COLOR-011  Setup color cell in table widget
 *               REQ-COLOR-012  Maintain connected ID and template name
 *               REQ-COLOR-013  Signal valueChanged on color modification
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-COLOR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef COLORTEMPLATE_H
#define COLORTEMPLATE_H

#include <QWidget>                                // For widget base class
#include <QJsonObject>                            // For JSON object handling
#include <QPushButton>                            // For push button widget
#include <QTableWidget>                           // For table widget

// Forward declaration
class Inspector;

// %%% Class Definition %%%
/* Widget for color template management */
class ColorTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize color template with Inspector reference
    explicit ColorTemplate(Inspector *inspector, QWidget *parent = nullptr);
    // Setup color cell in table
    void setupColorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
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

#endif // COLORTEMPLATE_H
