
#include "measuredistancedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QIcon>
MeasureDistanceDialog::MeasureDistanceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Measure Distance");
    setMinimumSize(400, 300);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
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
    segmentsList = new QListWidget(this);
    segmentsList->setFont(QFont("Courier"));
    mainLayout->addWidget(segmentsList);
    QHBoxLayout *controlLayout = new QHBoxLayout();
    unitComboBox = new QComboBox(this);
    unitComboBox->addItems({"meters", "kilometers", "feet", "miles", "degrees"});
    unitComboBox->setCurrentIndex(Meters);
    controlLayout->addWidget(unitComboBox);
    QPushButton *lockButton = new QPushButton(this);
    lockButton->setIcon(QIcon(":/icons/lock.png")); // Assumes lock.png is in resources
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
    QHBoxLayout *typeLayout = new QHBoxLayout();
    cartesianRadio = new QRadioButton("Cartesian");
    ellipsoidalRadio = new QRadioButton("Ellipsoidal");
    ellipsoidalRadio->setChecked(true);
    typeLayout->addWidget(cartesianRadio);
    typeLayout->addWidget(ellipsoidalRadio);
    mainLayout->addLayout(typeLayout);
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
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(newButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onNewButtonClicked);
    connect(copyButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onCopyButtonClicked);
    connect(infoButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onInfoButtonClicked);
    connect(configButton, &QPushButton::clicked, this, &MeasureDistanceDialog::onConfigButtonClicked);
    connect(cartesianRadio, &QRadioButton::toggled, this, &MeasureDistanceDialog::onMeasurementTypeChanged);
    connect(ellipsoidalRadio, &QRadioButton::toggled, this, &MeasureDistanceDialog::onMeasurementTypeChanged);
    connect(unitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MeasureDistanceDialog::onUnitChanged);
    setLayout(mainLayout);
}

void MeasureDistanceDialog::addMeasurement(double x, double y, double distance) {
    if (distance < 0) {
        QMessageBox::warning(this, "Invalid Distance", "Distance cannot be negative.");
        return;
    }
    measurements.append({x, y, distance});
    double factor = getConversionFactor(currentUnit);
    double convertedDistance = distance * factor;
    QString itemText = QString("%1 %2 %3")
                           .arg(x, 10, 'f', 4)
                           .arg(y, 10, 'f', 4)
                           .arg(convertedDistance, 10, 'f', 3);
    segmentsList->addItem(itemText);
    updateTotalDistance();
}

void MeasureDistanceDialog::updateTotalDistance() {
    double total = 0.0;
    double factor = getConversionFactor(currentUnit);
    QString unitStr = getUnitString(currentUnit);
    for (const auto &measurement : measurements) {
        total += measurement.distance;
    }
    totalDistanceEdit->setText(QString::number(total * factor, 'f', 3) + " " + unitStr);
}

void MeasureDistanceDialog::clearMeasurements()
{
    segmentsList->clear();
    measurements.clear();
    totalDistanceEdit->setText("0.000 " + getUnitString(currentUnit));
    emit newMeasurementRequested();
}
void MeasureDistanceDialog::updateMeasurementDisplay() {
    segmentsList->clear();
    double total = 0.0;
    double factor = getConversionFactor(currentUnit);
    QString unitStr = getUnitString(currentUnit);
    for (const auto &measurement : measurements) {
        double convertedDistance = measurement.distance * factor;
        total += measurement.distance;
        QString itemText = QString("%1 %2 %3")
                               .arg(measurement.x, 10, 'f', 4)
                               .arg(measurement.y, 10, 'f', 4)
                               .arg(convertedDistance, 10, 'f', 3);
        segmentsList->addItem(itemText);
    }
    totalDistanceEdit->setText(QString::number(total * factor, 'f', 3) + " " + unitStr);
}

bool MeasureDistanceDialog::isEllipsoidal() const
{
    return ellipsoidalRadio->isChecked();
}
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
void MeasureDistanceDialog::onMeasurementTypeChanged()
{
    emit measurementTypeChanged(isEllipsoidal());
    updateMeasurementDisplay();
}
void MeasureDistanceDialog::onNewButtonClicked()
{
    clearMeasurements();
}
void MeasureDistanceDialog::onCopyButtonClicked()
{
    QString text;
    for (int i = 0; i < segmentsList->count(); ++i) {
        text += segmentsList->item(i)->text() + "\n";
    }
    text += "Total: " + totalDistanceEdit->text();
    QApplication::clipboard()->setText(text);
    QMessageBox::information(this, "Copied", "Measurements copied to clipboard.");
}
void MeasureDistanceDialog::onInfoButtonClicked()
{
    QString info = isEllipsoidal()
                       ? "Ellipsoidal: Distances are calculated using an ellipsoidal model (e.g., WGS84)."
                       : "Cartesian: Distances are calculated using flat-plane geometry.";
    info += "\nCurrent unit: " + unitComboBox->currentText();
    QMessageBox::information(this, "Measurement Info", info);
}
void MeasureDistanceDialog::onConfigButtonClicked()
{
    QMessageBox::information(this, "Configuration", "Configuration options are not implemented yet.");
}
void MeasureDistanceDialog::onUnitChanged(int index)
{
    currentUnit = static_cast<Unit>(index);
    emit unitChanged(unitComboBox->currentText());
    updateMeasurementDisplay();
}

double MeasureDistanceDialog::getCurrentConversionFactor() const {
    return getConversionFactor(currentUnit);
}

QString MeasureDistanceDialog::getCurrentUnitString() const {
    return getUnitString(currentUnit);
}


