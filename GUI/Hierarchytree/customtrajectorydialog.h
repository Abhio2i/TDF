/* ========================================================================= */
/* File: CustomTrajectoryDialog.h                                            */
/* Purpose: Dialog for configuring custom trajectory parameters              */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */
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
