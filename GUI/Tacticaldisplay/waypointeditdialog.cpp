/* =============================================================================
 * FILE:         waypointeditdialog.cpp
 * MODULE:       Waypoint Editing Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the WaypointEditDialog class which provides a modal
 *               dialog for editing waypoint parameters (latitude, longitude,
 *               altitude, speed, drop weapon flag, drop count). Used within
 *               trajectory editing workflows to modify individual waypoint
 *               data. Supports setting initial values, retrieving updated
 *               values, and a static convenience method for one‑shot editing.
 *
 * REQUIREMENTS: Implements REQ-WAYPOINT-010 through REQ-WAYPOINT-016
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-WAYPOINT-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "waypointeditdialog.h"
#include "GUI/dialogwindowutils.h"
#include "GUI/Inspector/inspector-styles.h"


WaypointEditDialog::WaypointEditDialog(QWidget *parent)
    : QDialog(parent)
    , latSpinBox(nullptr)
    , lonSpinBox(nullptr)
    , altSpinBox(nullptr)
    , speedSpinBox(nullptr)
    , dropWeaponCheckBox(nullptr)
    , dropCountSpinBox(nullptr)
    , okButton(nullptr)
    , cancelButton(nullptr)
{
    setupUI();
    setupConnections();
    setWindowTitle("Edit Waypoint");
    setMinimumSize(300, 260);
    resize(300, 260);
    makeDialogResizable(this);
    setWindowModality(Qt::WindowModal);
    setStyleSheet("QDialog { background-color: #0F2636; border: 2px solid #27446d; }");
}

WaypointEditDialog::~WaypointEditDialog()
{
}
void WaypointEditDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Sab labels ki same width — isse controls ek vertical line se align honge
    const int labelWidth = 100;

    // Latitude
    QHBoxLayout* latLayout = new QHBoxLayout();
    QLabel* latLabel = new QLabel("Latitude:", this);
    latLabel->setFixedWidth(labelWidth);
    latSpinBox = new QDoubleSpinBox(this);
    latSpinBox->setRange(-90.0, 90.0);
    latSpinBox->setDecimals(6);
    latSpinBox->setSingleStep(0.0001);
    latLayout->addWidget(latLabel);
    latLayout->addWidget(latSpinBox);
    mainLayout->addLayout(latLayout);

    // Longitude
    QHBoxLayout* lonLayout = new QHBoxLayout();
    QLabel* lonLabel = new QLabel("Longitude:", this);
    lonLabel->setFixedWidth(labelWidth);
    lonSpinBox = new QDoubleSpinBox(this);
    lonSpinBox->setRange(-180.0, 180.0);
    lonSpinBox->setDecimals(6);
    lonSpinBox->setSingleStep(0.0001);
    lonLayout->addWidget(lonLabel);
    lonLayout->addWidget(lonSpinBox);
    mainLayout->addLayout(lonLayout);

    // Altitude
    QHBoxLayout* altLayout = new QHBoxLayout();
    QLabel* altLabel = new QLabel("Altitude:", this);
    altLabel->setFixedWidth(labelWidth);
    altSpinBox = new QDoubleSpinBox(this);
    altSpinBox->setRange(-2000.0, 50000.0);
    altSpinBox->setDecimals(2);
    altSpinBox->setSingleStep(10.0);
    altSpinBox->setSuffix(" ft");
    altLayout->addWidget(altLabel);
    altLayout->addWidget(altSpinBox);
    mainLayout->addLayout(altLayout);

    // Speed
    QHBoxLayout* speedLayout = new QHBoxLayout();
    QLabel* speedLabel = new QLabel("Speed:", this);
    speedLabel->setFixedWidth(labelWidth);
    speedSpinBox = new QDoubleSpinBox(this);
    speedSpinBox->setRange(0.0, 1000.0);
    speedSpinBox->setDecimals(2);
    speedSpinBox->setSingleStep(10.0);
    speedSpinBox->setSuffix(" km/h");
    speedLayout->addWidget(speedLabel);
    speedLayout->addWidget(speedSpinBox);
    mainLayout->addLayout(speedLayout);

    // Drop Weapon — checkbox ab spinbox jaisi line se start hoga
    QHBoxLayout* dropWeaponLayout = new QHBoxLayout();
    QLabel* dropWeaponLabel = new QLabel("Drop Weapon:", this);
    dropWeaponLabel->setFixedWidth(labelWidth);
    dropWeaponCheckBox = new QCheckBox(this);
    dropWeaponCheckBox->setChecked(false);
    dropWeaponCheckBox->setStyleSheet(InspectorStyles::CheckBox);
    dropWeaponLayout->addWidget(dropWeaponLabel);
    dropWeaponLayout->addWidget(dropWeaponCheckBox);
    dropWeaponLayout->addStretch();   // checkbox left-aligned, baaki space right mein
    mainLayout->addLayout(dropWeaponLayout);

    // Drop Count
    QHBoxLayout* dropCountLayout = new QHBoxLayout();
    QLabel* dropCountLabel = new QLabel("Drop Count:", this);
    dropCountLabel->setFixedWidth(labelWidth);
    dropCountSpinBox = new QSpinBox(this);
    dropCountSpinBox->setRange(0, 100);
    dropCountSpinBox->setSingleStep(1);
    dropCountSpinBox->setEnabled(false);
    dropCountLayout->addWidget(dropCountLabel);
    dropCountLayout->addWidget(dropCountSpinBox);
    mainLayout->addLayout(dropCountLayout);

    mainLayout->addSpacing(10);

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
    connect(dropWeaponCheckBox, &QCheckBox::toggled, this, &WaypointEditDialog::onDropWeaponToggled);
}

void WaypointEditDialog::onDropWeaponToggled(bool checked)
{
    dropCountSpinBox->setEnabled(checked);
    if (!checked)
        dropCountSpinBox->setValue(0);
}

void WaypointEditDialog::setWaypointValues(double latitude, double longitude, double altitude,
                                           double speed, bool dropWeapon, int dropCount)
{
    latSpinBox->setValue(latitude);
    lonSpinBox->setValue(longitude);
    altSpinBox->setValue(altitude);
    speedSpinBox->setValue(speed);
    dropWeaponCheckBox->setChecked(dropWeapon);
    dropCountSpinBox->setEnabled(dropWeapon);
    dropCountSpinBox->setValue(dropCount);
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

bool WaypointEditDialog::getDropWeapon() const
{
    return dropWeaponCheckBox->isChecked();
}

int WaypointEditDialog::getDropCount() const
{
    return dropCountSpinBox->value();
}

bool WaypointEditDialog::editWaypoint(QWidget* parent, double& latitude, double& longitude,
                                      double& altitude, double& speed, bool& dropWeapon, int& dropCount)
{
    WaypointEditDialog dialog(parent);
    dialog.setWaypointValues(latitude, longitude, altitude, speed, dropWeapon, dropCount);
    if (dialog.exec() == QDialog::Accepted) {
        latitude   = dialog.getLatitude();
        longitude  = dialog.getLongitude();
        altitude   = dialog.getAltitude();
        speed      = dialog.getSpeed();
        dropWeapon = dialog.getDropWeapon();
        dropCount  = dialog.getDropCount();
        return true;
    }
    return false;
}

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
