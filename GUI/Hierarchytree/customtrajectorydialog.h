/* =============================================================================
 * FILE:         CustomTrajectoryDialog.h
 * MODULE:       Custom Trajectory Configuration Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the CustomTrajectoryDialog class which provides a
 *               modal dialog for configuring custom trajectory parameters.
 *               Supports waypoint count, shape selection (line, circle, oval),
 *               geometric dimensions (length, radius, major/minor axis),
 *               orientation, start/circle points, altitude constraints,
 *               speed constraints, and curve type. UI dynamically updates
 *               based on shape selection and checkbox states.
 *
 * REQUIREMENTS: REQ-TRAJ-010  Waypoint count configuration
 *               REQ-TRAJ-011  Trajectory shape selection (line/circle/oval)
 *               REQ-TRAJ-012  Line trajectory length parameter
 *               REQ-TRAJ-013  Circle trajectory radius and start point
 *               REQ-TRAJ-014  Oval trajectory major/minor axis and orientation
 *               REQ-TRAJ-015  Altitude constraints (min/max or fixed)
 *               REQ-TRAJ-016  Speed constraints (min/max or fixed)
 *               REQ-TRAJ-017  Curve type selection
 *               REQ-TRAJ-018  Dynamic UI updates based on shape
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-TRAJ-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef CUSTOMTRAJECTORYDIALOG_H
#define CUSTOMTRAJECTORYDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
class CustomTrajectoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomTrajectoryDialog(QWidget *parent = nullptr);
    int getWaypointsCount() const;
    QString getShape() const;
    double getLength() const;
    double getRadius() const;
    double getMajorAxis() const;
    double getMinorAxis() const;
    QString getOrientation() const;
    QString getStartPoint() const;
    QString getCirclePoint() const;
    double getMinAltitude() const;
    double getMaxAltitude() const;
    double getMinSpeed() const;
    double getMaxSpeed() const;
    QString getCurveType() const;
    bool isAltitudeEnabled() const;
    bool isSpeedEnabled() const;

private slots:
    void onShapeChanged(int index);
    void onSpeedCheckboxChanged(int state);
    void onAltitudeCheckboxChanged(int state);
private:
    void setupUI();
    QSpinBox *waypointsSpinBox;
    QComboBox *shapesComboBox;
    QGroupBox *lineGroupBox;
    QGroupBox *circleGroupBox;
    QGroupBox *ovalGroupBox;
    QGroupBox *speedGroupBox;
    QGroupBox *altitudeGroupBox;
    QGroupBox *curveGroupBox;
    QDoubleSpinBox *lengthSpinBox;
    QDoubleSpinBox *radiusSpinBox;
    QComboBox *startPointComboBox;
    QComboBox *circlePointComboBox;
    QDoubleSpinBox *majorAxisSpinBox;
    QDoubleSpinBox *minorAxisSpinBox;
    QComboBox *orientationComboBox;
    QCheckBox *speedCheckBox;
    QDoubleSpinBox *minSpeedSpinBox;
    QDoubleSpinBox *maxSpeedSpinBox;
    QDoubleSpinBox *SpeedSpinBox;
    QCheckBox *altitudeCheckBox;
    QDoubleSpinBox *minAltitudeSpinBox;
    QDoubleSpinBox *maxAltitudeSpinBox;
    QDoubleSpinBox *altitudeSpinBox;
    QComboBox *curveComboBox;
};

#endif // CUSTOMTRAJECTORYDIALOG_H
