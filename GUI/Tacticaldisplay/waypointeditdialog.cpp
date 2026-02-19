#include "waypointeditdialog.h"

WaypointEditDialog::WaypointEditDialog(QWidget *parent)
    : QDialog(parent)
    , latSpinBox(nullptr)
    , lonSpinBox(nullptr)
    , altSpinBox(nullptr)
    , speedSpinBox(nullptr)
    , okButton(nullptr)
    , cancelButton(nullptr)
{
    setupUI();
    setupConnections();

    // Set dialog properties
    setWindowTitle("Edit Waypoint");
    setFixedSize(300, 200);

    // Set window flags to prevent flickering
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setModal(true);
}

WaypointEditDialog::~WaypointEditDialog()
{
    // All widgets are child of dialog, so they will be deleted automatically
}

void WaypointEditDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Latitude field
    QHBoxLayout* latLayout = new QHBoxLayout();
    QLabel* latLabel = new QLabel("Latitude:", this);
    latSpinBox = new QDoubleSpinBox(this);
    latSpinBox->setRange(-90.0, 90.0);
    latSpinBox->setDecimals(6);
    latSpinBox->setSingleStep(0.0001);
    latLayout->addWidget(latLabel);
    latLayout->addWidget(latSpinBox);
    mainLayout->addLayout(latLayout);

    // Longitude field
    QHBoxLayout* lonLayout = new QHBoxLayout();
    QLabel* lonLabel = new QLabel("Longitude:", this);
    lonSpinBox = new QDoubleSpinBox(this);
    lonSpinBox->setRange(-180.0, 180.0);
    lonSpinBox->setDecimals(6);
    lonSpinBox->setSingleStep(0.0001);
    lonLayout->addWidget(lonLabel);
    lonLayout->addWidget(lonSpinBox);
    mainLayout->addLayout(lonLayout);

    // Altitude field
    QHBoxLayout* altLayout = new QHBoxLayout();
    QLabel* altLabel = new QLabel("Altitude:", this);
    altSpinBox = new QDoubleSpinBox(this);
    altSpinBox->setRange(0.0, 50000.0);
    altSpinBox->setDecimals(2);
    altSpinBox->setSingleStep(10.0);
    altSpinBox->setSuffix(" ft");
    altLayout->addWidget(altLabel);
    altLayout->addWidget(altSpinBox);
    mainLayout->addLayout(altLayout);

    // Speed field
    QHBoxLayout* speedLayout = new QHBoxLayout();
    QLabel* speedLabel = new QLabel("Speed:", this);
    speedSpinBox = new QDoubleSpinBox(this);
    speedSpinBox->setRange(0.0, 1000.0);
    speedSpinBox->setDecimals(2);
    speedSpinBox->setSingleStep(10.0);
    speedSpinBox->setSuffix(" km/h");
    speedLayout->addWidget(speedLabel);
    speedLayout->addWidget(speedSpinBox);
    mainLayout->addLayout(speedLayout);

    // Add some spacing
    mainLayout->addSpacing(10);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

void WaypointEditDialog::setupConnections()
{
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void WaypointEditDialog::setWaypointValues(double latitude, double longitude, double altitude, double speed)
{
    latSpinBox->setValue(latitude);
    lonSpinBox->setValue(longitude);
    altSpinBox->setValue(altitude);
    speedSpinBox->setValue(speed);
}

double WaypointEditDialog::getLatitude() const
{
    return latSpinBox->value();
}

double WaypointEditDialog::getLongitude() const
{
    return lonSpinBox->value();
}

double WaypointEditDialog::getAltitude() const
{
    return altSpinBox->value();
}

double WaypointEditDialog::getSpeed() const
{
    return speedSpinBox->value();
}

// Static method for easy usage
bool WaypointEditDialog::editWaypoint(QWidget* parent, double& latitude, double& longitude, double& altitude, double& speed)
{
    WaypointEditDialog dialog(parent);
    dialog.setWaypointValues(latitude, longitude, altitude, speed);
    if (dialog.exec() == QDialog::Accepted) {
        latitude = dialog.getLatitude();
        longitude = dialog.getLongitude();
        altitude = dialog.getAltitude();
        speed = dialog.getSpeed();
        return true;
    }
    return false;
}
