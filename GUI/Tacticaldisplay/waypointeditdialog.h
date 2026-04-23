/* =============================================================================
 * FILE:         WaypointEditDialog.h
 * MODULE:       Waypoint Editing Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the WaypointEditDialog class which provides a modal
 *               dialog for editing waypoint parameters (latitude, longitude,
 *               altitude, speed). Used within trajectory editing workflows
 *               to modify individual waypoint data. Supports setting initial
 *               values, retrieving updated values, and a static convenience
 *               method for one‑shot editing.
 *
 * REQUIREMENTS: REQ-WAYPOINT-010  Waypoint editing dialog
 *               REQ-WAYPOINT-011  Edit latitude, longitude, altitude, speed
 *               REQ-WAYPOINT-012  Set initial waypoint values
 *               REQ-WAYPOINT-013  Retrieve updated values after dialog acceptance
 *               REQ-WAYPOINT-014  Static convenience method for modal editing
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-WAYPOINT-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
/* ========================================================================= */
/* File: WaypointEditDialog.h                                                */
/* Purpose: Dialog for editing waypoint parameters (lat, lon, alt, speed)    */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */

#ifndef WAYPOINTEDITDIALOG_H
#define WAYPOINTEDITDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class WaypointEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WaypointEditDialog(QWidget *parent = nullptr);
    ~WaypointEditDialog();

    // Set initial values
    void setWaypointValues(double latitude, double longitude, double altitude, double speed = 100.0);

    // Get updated values
    double getLatitude() const;
    double getLongitude() const;
    double getAltitude() const;
    double getSpeed() const;
    static bool editWaypoint(QWidget* parent, double& latitude, double& longitude, double& altitude, double& speed);

private:
    void setupUI();
    void setupConnections();

    // UI Components
    QDoubleSpinBox* latSpinBox;
    QDoubleSpinBox* lonSpinBox;
    QDoubleSpinBox* altSpinBox;
    QDoubleSpinBox* speedSpinBox;
    QPushButton* okButton;
    QPushButton* cancelButton;
};

#endif // WAYPOINTEDITDIALOG_H
