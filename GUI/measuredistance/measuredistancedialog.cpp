/* ========================================================================= */
/* File: measuredistancedialog.cpp                                         */
/* Purpose: Implements dialog for measuring and displaying distances        */
/* ========================================================================= */

#include "measuredistancedialog.h"                  // For measure distance dialog
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

// %%% Constructor %%%
/* Initialize measure distance dialog */
MeasureDistanceDialog::MeasureDistanceDialog(QWidget *parent)
    : QDialog(parent)
{
    // Set window title and size
    setWindowTitle("Measure Distance");
    setMinimumSize(400, 300);
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // Setup segments header
    QHBoxLayout *segmentsHeaderLayout = new QHBoxLayout();
    QLabel *xLabel = new QLabel("x");
    QLabel *yLabel = new QLabel("y");
    QLabel *distanceLabel = new QLabel("Distance");
    xLabel->setStyleSheet("font-weight: bold; font-family: Courier;");
    yLabel->setStyleSheet("font-weight: bold; font-family: Courier;");
    distanceLabel->setStyleSheet("font-weight: bold; font-family: Courier;");
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
    segmentsList->setFont(QFont("Courier"));
    mainLayout->addWidget(segmentsList);
    // Setup control layout
    QHBoxLayout *controlLayout = new QHBoxLayout();
    unitComboBox = new QComboBox(this);
    unitComboBox->addItems({"meters", "kilometers", "feet", "miles", "degrees"});
    unitComboBox->setCurrentIndex(Meters);
    controlLayout->addWidget(unitComboBox);
    QPushButton *lockButton = new QPushButton(this);
    lockButton->setIcon(QIcon(":/icons/lock.png"));
    lockButton->setFixedSize(24, 24);
    lockButton->setFlat(true);
    lockButton->setToolTip("Lock measurements");
    controlLayout->addWidget(lockButton);
    controlLayout->addWidget(new QLabel("Total:"));
    totalDistanceEdit = new QLineEdit(this);
    totalDistanceEdit->setReadOnly(true);
    totalDistanceEdit->setPlaceholderText("0.000 m");
    controlLayout->addWidget(totalDistanceEdit);
    controlLayout->addStretch();
    mainLayout->addLayout(controlLayout);
    // Setup measurement type selection
    QHBoxLayout *typeLayout = new QHBoxLayout();
    cartesianRadio = new QRadioButton("Cartesian");
    ellipsoidalRadio = new QRadioButton("Ellipsoidal");
    ellipsoidalRadio->setChecked(true);
    typeLayout->addWidget(cartesianRadio);
    typeLayout->addWidget(ellipsoidalRadio);
    mainLayout->addLayout(typeLayout);
    // Setup button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *infoButton = new QPushButton("Info");
    QPushButton *newButton = new QPushButton("New");
    QPushButton *configButton = new QPushButton("Configuration");
    QPushButton *copyButton = new QPushButton("Copy");
    QPushButton *closeButton = new QPushButton("Close");
    buttonLayout->addWidget(infoButton);
    buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(configButton);
    buttonLayout->addWidget(copyButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
    // Connect button signals
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(newButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onNewButtonClicked);
    connect(copyButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onCopyButtonClicked);
    connect(infoButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onInfoButtonClicked);
    connect(configButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onConfigButtonClicked);
    connect(cartesianRadio, &QRadioButton::toggled, this, &MeasureDistanceDialog::onMeasurementTypeChanged);
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
        QMessageBox::warning(this, "Invalid Distance", "Distance cannot be negative.");
        return;
    }
    // Store measurement
    measurements.append({x, y, distance});
    // Convert distance to current unit
    double factor = getConversionFactor(currentUnit);
    double convertedDistance = distance * factor;
    // Format and add item to list
    QString itemText = QString("%1 %2 %3")
                           .arg(x, 10, 'f', 4)
                           .arg(y, 10, 'f', 4)
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
    QMessageBox::information(this, "Copied", "Measurements copied to clipboard.");
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
    QMessageBox::information(this, "Measurement Info", info);
}

/* Handle configuration button click */
void MeasureDistanceDialog::onConfigButtonClicked()
{
    // Show placeholder message
    QMessageBox::information(this, "Configuration", "Configuration options are not implemented yet.");
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
