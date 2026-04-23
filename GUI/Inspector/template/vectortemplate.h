/* =============================================================================
 * FILE:         vectortemplate.h
 * MODULE:       Vector Template Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the VectorTemplate class which provides a widget for
 *               managing vector templates (numeric arrays or coordinate tuples)
 *               within the Inspector panel. It sets up vector cells in a table,
 *               maintains connected entity IDs and template names, supports
 *               copying/pasting vector data, and emits value changes when a
 *               vector element is modified.
 *
 * REQUIREMENTS: REQ-VECTOR-010  Vector template management widget
 *               REQ-VECTOR-011  Setup vector cell in table widget
 *               REQ-VECTOR-012  Maintain connected ID and template name
 *               REQ-VECTOR-013  Signal valueChanged on vector modification
 *               REQ-VECTOR-014  Format vector numbers for display
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-VECTOR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef VECTORTEMPLATE_H
#define VECTORTEMPLATE_H

#include <QWidget>                                // For widget base class
#include <QJsonObject>                            // For JSON object handling
#include <QTableWidget>                           // For table widget
#include "GUI/Inspector/inspector.h"              // For inspector panel

// %%% Class Definition %%%
/* Widget for vector template management */
class VectorTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize vector template
    explicit VectorTemplate(QWidget *parent = nullptr);
    // Setup vector cell in table
    void setupVectorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
    // Set connected ID
    void setConnectedID(const QString &id) { connectedID = id; }
    // Set template name
    void setName(const QString &n) { name = n; }
    // Set main ID
    void setMainID(const QString &id) { mainID = id; }
    // Set inspector reference
    void setInspectorRef(Inspector *inspector) { inspectorRef = inspector; }
    // Constant for row height
    static constexpr int ROW_HEIGHT = 30;

signals:
    // Signal value change
    void valueChanged(QString ID, QString name, QJsonObject delta);

private:
    // %%% Data Members %%%
    // Connected item ID
    QString connectedID;
    // Template name
    QString name;
    // Main ID
    QString mainID;
    // Inspector reference
    Inspector *inspectorRef;
    // Copied vector data
    QJsonObject copiedVectorData;
    QString formatVectorNumber(double value);
};

#endif // VECTORTEMPLATE_H
