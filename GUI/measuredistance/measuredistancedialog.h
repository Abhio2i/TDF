/* ========================================================================= */
/* File: measuredistancedialog.h                                            */
/* Purpose: Defines dialog for measuring distances                          */
/* ========================================================================= */

#ifndef MEASUREDISTANCEDIALOG_H
#define MEASUREDISTANCEDIALOG_H

#include <QDialog>                                // For dialog base class
#include <QListWidget>                            // For list widget
#include <QLineEdit>                              // For text input widget
#include <QRadioButton>                           // For radio button widget
#include <QComboBox>                              // For combo box widget

// %%% Class Definition %%%
/* Dialog for measuring and displaying distances */
class MeasureDistanceDialog : public QDialog
{
    Q_OBJECT

public:
    // Initialize dialog
    explicit MeasureDistanceDialog(QWidget *parent = nullptr);
    // Add measurement to list
    void addMeasurement(double x, double y, double distance);
    // Clear all measurements
    void clearMeasurements();
    // Check if ellipsoidal measurement
    bool isEllipsoidal() const;
    // Get current conversion factor
    double getCurrentConversionFactor() const;
    // Get current unit string
    QString getCurrentUnitString() const;

signals:
    // Signal measurement type change
    void measurementTypeChanged(bool isEllipsoidal);
    // Signal new measurement request
    void newMeasurementRequested();
    // Signal unit change
    void unitChanged(const QString &unit);

private:
    // %%% UI Components %%%
    // List of measurement segments
    QListWidget *segmentsList;
    // Total distance input field
    QLineEdit *totalDistanceEdit;
    // Cartesian radio button
    QRadioButton *cartesianRadio;
    // Ellipsoidal radio button
    QRadioButton *ellipsoidalRadio;
    // Unit selection combo box
    QComboBox *unitComboBox;

    // %%% Data Structures %%%
    // Measurement data structure
    struct Measurement {
        double x;
        double y;
        double distance;
    };
    // List of measurements
    QList<Measurement> measurements;
    // Enum for unit types
    enum Unit { Meters, Kilometers, Feet, Miles, Degrees };
    // Current unit
    Unit currentUnit = Meters;

    // %%% Utility Methods %%%
    // Get conversion factor for unit
    double getConversionFactor(Unit unit) const;
    // Get unit string
    QString getUnitString(Unit unit) const;
    // Update total distance display
    void updateTotalDistance();

private slots:
    // Update measurement display
    void updateMeasurementDisplay();
    // Handle measurement type change
    void onMeasurementTypeChanged();
    // Handle new button click
    void onNewButtonClicked();
    // Handle copy button click
    void onCopyButtonClicked();
    // Handle info button click
    void onInfoButtonClicked();
    // Handle config button click
    void onConfigButtonClicked();
    // Handle unit change
    void onUnitChanged(int index);
};

#endif // MEASUREDISTANCEDIALOG_H
