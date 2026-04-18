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

    // Static method for easier usage
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
