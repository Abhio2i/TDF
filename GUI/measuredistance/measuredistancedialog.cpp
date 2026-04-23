/* =============================================================================
 * FILE:         measuredistancedialog.cpp
 * MODULE:       Distance Measurement Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the MeasureDistanceDialog class which provides a
 *               modal dialog for measuring and displaying distances between
 *               points. Supports Cartesian and ellipsoidal (geodesic) distance
 *               modes, multiple unit conversions (meters, kilometers, feet,
 *               miles, degrees), segment list display, total distance
 *               aggregation, and signals for measurement type and unit changes.
 *
 * REQUIREMENTS: Implements REQ-MEASURE-010 through REQ-MEASURE-018
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-MEASURE-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "measuredistancedialog.h"                  // For measure distance dialog
#include "measuredistancedialog-styles.h"           // Include separate CSS file
#include <QVBoxLayout>                             // For vertical layout
#include <QHBoxLayout>                             // For horizontal layout
#include <QLabel>                                  // For labels
#include <QPushButton>                             // For buttons
#include <QMessageBox>                             // For message boxes
#include <QClipboard>                              // For clipboard operations
#include <QApplication>                            // For application access
#include <QIcon>                                   // For icons
#include <QListWidget>                             // For list widget
#include <QComboBox>                               // For dropdown menu
#include <QRadioButton>                            // For radio buttons
#include <QLineEdit>                               // For input fields
#include <QFont>                                   // For font settings

#include <QTimer>
#include <QDebug>
// %%% Constructor %%%
/* Initialize measure distance dialog */
MeasureDistanceDialog::MeasureDistanceDialog(QWidget *parent)
    : QDialog(parent)
{
    // Apply dark theme to dialog
    setStyleSheet(MeasureDistanceDialogStyles::Dialog);

    // Set window title and size
    setWindowTitle("Measure Distance");
    setMinimumSize(400, 300);

    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Setup segments header
    QHBoxLayout *segmentsHeaderLayout = new QHBoxLayout();

    QLabel *xLabel = new QLabel("x");
    QLabel *yLabel = new QLabel("y");
    QLabel *distanceLabel = new QLabel("Distance");

    xLabel->setStyleSheet(MeasureDistanceDialogStyles::HeaderLabel);
    yLabel->setStyleSheet(MeasureDistanceDialogStyles::HeaderLabel);
    distanceLabel->setStyleSheet(MeasureDistanceDialogStyles::HeaderLabel);

    xLabel->setFixedWidth(100);
    yLabel->setFixedWidth(100);
    distanceLabel->setFixedWidth(100);

    segmentsHeaderLayout->addWidget(xLabel);
    segmentsHeaderLayout->addSpacing(20);
    segmentsHeaderLayout->addWidget(yLabel);
    segmentsHeaderLayout->addSpacing(20);
    segmentsHeaderLayout->addWidget(distanceLabel);
    segmentsHeaderLayout->addStretch();

    mainLayout->addLayout(segmentsHeaderLayout);

    // Setup segments list
    segmentsList = new QListWidget(this);
    segmentsList->setFont(QFont("Courier New", 10));
    segmentsList->setStyleSheet(MeasureDistanceDialogStyles::ListWidget);
    mainLayout->addWidget(segmentsList);

    // Setup control layout
    QHBoxLayout *controlLayout = new QHBoxLayout();

    unitComboBox = new QComboBox(this);
    unitComboBox->addItems({"meters", "kilometers", "feet", "miles", "degrees"});
    unitComboBox->setCurrentIndex(Meters);
    unitComboBox->setStyleSheet(MeasureDistanceDialogStyles::ComboBox);
    controlLayout->addWidget(unitComboBox);

    QPushButton *lockButton = new QPushButton(this);
    lockButton->setIcon(QIcon(":/icons/lock.png"));
    lockButton->setFixedSize(24, 24);
    lockButton->setFlat(true);
    lockButton->setStyleSheet(MeasureDistanceDialogStyles::LockButton);
    lockButton->setToolTip("Lock measurements");
    controlLayout->addWidget(lockButton);

    QLabel *totalLabel = new QLabel("Total:");
    totalLabel->setStyleSheet(MeasureDistanceDialogStyles::TotalLabel);
    controlLayout->addWidget(totalLabel);

    totalDistanceEdit = new QLineEdit(this);
    totalDistanceEdit->setReadOnly(true);
    totalDistanceEdit->setPlaceholderText("0.000 m");
    totalDistanceEdit->setStyleSheet(MeasureDistanceDialogStyles::LineEdit);
    controlLayout->addWidget(totalDistanceEdit);
    controlLayout->addStretch();

    mainLayout->addLayout(controlLayout);

    // Setup measurement type selection
    QHBoxLayout *typeLayout = new QHBoxLayout();

    ellipsoidalRadio = new QRadioButton("Ellipsoidal");
    ellipsoidalRadio->setChecked(true);
    ellipsoidalRadio->setStyleSheet(MeasureDistanceDialogStyles::RadioButton);

    typeLayout->addWidget(ellipsoidalRadio);
    typeLayout->addStretch();

    mainLayout->addLayout(typeLayout);

    // Setup button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    QPushButton *infoButton = new QPushButton("Info");
    QPushButton *newButton = new QPushButton("New");
    QPushButton *copyButton = new QPushButton("Copy");
    QPushButton *closeButton = new QPushButton("Close");

    infoButton->setStyleSheet(MeasureDistanceDialogStyles::PushButton);
    newButton->setStyleSheet(MeasureDistanceDialogStyles::PushButton);
    copyButton->setStyleSheet(MeasureDistanceDialogStyles::PushButton);
    closeButton->setStyleSheet(MeasureDistanceDialogStyles::PushButton);

    buttonLayout->addWidget(infoButton);
    buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // Connect button signals
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(newButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onNewButtonClicked);
    connect(copyButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onCopyButtonClicked);
    connect(infoButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onInfoButtonClicked);
    connect(ellipsoidalRadio, &QRadioButton::toggled, this, &MeasureDistanceDialog::onMeasurementTypeChanged);
    connect(unitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MeasureDistanceDialog::onUnitChanged);

    // Set main layout
    setLayout(mainLayout);
}

// %%% Measurement Management %%%
/* Add a new measurement to the list */
void MeasureDistanceDialog::addMeasurement(double x, double y, double distance)
{
    // Validate distance
    if (distance < 0) {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(MeasureDistanceDialogStyles::Dialog);
        msgBox.setWindowTitle("Invalid Distance");
        msgBox.setText("Distance cannot be negative.");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    // Store measurement
    measurements.append({x, y, distance});

    // Convert distance to current unit
    double factor = getConversionFactor(currentUnit);
    double convertedDistance = distance * factor;

    // Format and add item to list
    QString itemText = QString("%1 %2 %3")
                           .arg(x, 10, 'f', 6)
                           .arg(y, 10, 'f', 6)
                           .arg(convertedDistance, 10, 'f', 3);
    segmentsList->addItem(itemText);

    // Update total distance
    updateTotalDistance();
}

/* Update total distance display */
void MeasureDistanceDialog::updateTotalDistance()
{
    double total = 0.0;
    double factor = getConversionFactor(currentUnit);
    QString unitStr = getUnitString(currentUnit);

    // Sum distances
    for (const auto &measurement : measurements) {
        total += measurement.distance;
    }

    // Update total display
    totalDistanceEdit->setText(QString::number(total * factor, 'f', 3) + " " + unitStr);
}

/* Clear all measurements */
void MeasureDistanceDialog::clearMeasurements()
{
    // Clear list and measurements
    segmentsList->clear();
    measurements.clear();

    // Reset total display
    totalDistanceEdit->setText("0.000 " + getUnitString(currentUnit));

    // Emit new measurement signal
    emit newMeasurementRequested();
}

/* Update measurement display with current unit */
void MeasureDistanceDialog::updateMeasurementDisplay()
{
    // Clear current list
    segmentsList->clear();
    double total = 0.0;
    double factor = getConversionFactor(currentUnit);
    QString unitStr = getUnitString(currentUnit);

    // Repopulate list with converted measurements
    for (const auto &measurement : measurements) {
        double convertedDistance = measurement.distance * factor;
        total += measurement.distance;
        QString itemText = QString("%1 %2 %3")
                               .arg(measurement.x, 10, 'f', 4)
                               .arg(measurement.y, 10, 'f', 4)
                               .arg(convertedDistance, 10, 'f', 3);
        segmentsList->addItem(itemText);
    }

    // Update total display
    totalDistanceEdit->setText(QString::number(total * factor, 'f', 3) + " " + unitStr);
}

/* Check if ellipsoidal measurement is selected */
bool MeasureDistanceDialog::isEllipsoidal() const
{
    return ellipsoidalRadio->isChecked();
}

// %%% Utility Methods %%%
/* Get conversion factor for unit */
double MeasureDistanceDialog::getConversionFactor(Unit unit) const
{
    switch (unit) {
    case Meters: return 1.0;
    case Kilometers: return 0.001;
    case Feet: return 3.28084;
    case Miles: return 0.000621371;
    case Degrees: return 1.0 / 111139.0;
    default: return 1.0;
    }
}

/* Get unit string for display */
QString MeasureDistanceDialog::getUnitString(Unit unit) const
{
    switch (unit) {
    case Meters: return "m";
    case Kilometers: return "km";
    case Feet: return "ft";
    case Miles: return "mi";
    case Degrees: return "deg";
    default: return "m";
    }
}

/* Handle measurement type change */
void MeasureDistanceDialog::onMeasurementTypeChanged()
{
    // Emit type change signal
    emit measurementTypeChanged(isEllipsoidal());
    // Update display
    updateMeasurementDisplay();
}

/* Handle new button click */
void MeasureDistanceDialog::onNewButtonClicked()
{
    // Clear measurements
    clearMeasurements();
}

/* Handle copy button click */
void MeasureDistanceDialog::onCopyButtonClicked()
{
    QString text;

    // Copy all list items
    for (int i = 0; i < segmentsList->count(); ++i) {
        text += segmentsList->item(i)->text() + "\n";
    }

    // Append total distance
    text += "Total: " + totalDistanceEdit->text();

    // Copy to clipboard
    QApplication::clipboard()->setText(text);

    // Show confirmation
    QMessageBox msgBox(this);
    msgBox.setStyleSheet(MeasureDistanceDialogStyles::Dialog);
    msgBox.setWindowTitle("Copied");
    msgBox.setText("Measurements copied to clipboard.");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

/* Handle info button click */
void MeasureDistanceDialog::onInfoButtonClicked()
{
    // Prepare info message
    QString info = isEllipsoidal()
                       ? "Ellipsoidal: Distances are calculated using an ellipsoidal model (e.g., WGS84)."
                       : "Cartesian: Distances are calculated using flat-plane geometry.";
    info += "\nCurrent unit: " + unitComboBox->currentText();

    // Show info dialog
    QMessageBox msgBox(this);
    msgBox.setStyleSheet(MeasureDistanceDialogStyles::Dialog);
    msgBox.setWindowTitle("Measurement Info");
    msgBox.setText(info);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

/* Handle configuration button click */
void MeasureDistanceDialog::onConfigButtonClicked()
{
    // Show placeholder message
    QMessageBox msgBox(this);
    msgBox.setStyleSheet(MeasureDistanceDialogStyles::Dialog);
    msgBox.setWindowTitle("Configuration");
    msgBox.setText("Configuration options are not implemented yet.");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

/* Handle unit change */
void MeasureDistanceDialog::onUnitChanged(int index)
{
    // Update current unit
    currentUnit = static_cast<Unit>(index);

    // Emit unit change signal
    emit unitChanged(unitComboBox->currentText());

    // Update display
    updateMeasurementDisplay();
}

/* Get current conversion factor */
double MeasureDistanceDialog::getCurrentConversionFactor() const
{
    return getConversionFactor(currentUnit);
}

/* Get current unit string */
QString MeasureDistanceDialog::getCurrentUnitString() const
{
    return getUnitString(currentUnit);
}

