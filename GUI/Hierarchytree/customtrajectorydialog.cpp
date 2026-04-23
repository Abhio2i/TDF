/* =============================================================================
 * FILE:         customtrajectorydialog.cpp
 * MODULE:       Custom Trajectory Configuration Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the CustomTrajectoryDialog class which provides a
 *               modal dialog for configuring custom trajectory parameters.
 *               Supports waypoint count, shape selection (line, circle, oval),
 *               geometric dimensions (length, radius, major/minor axis),
 *               orientation, start/circle points, altitude constraints,
 *               speed constraints, and curve type. UI dynamically updates
 *               based on shape selection and checkbox states.
 *
 * REQUIREMENTS: Implements REQ-TRAJ-010 through REQ-TRAJ-018
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-TRAJ-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "customtrajectorydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QFrame>
#include <QDialogButtonBox>
#include <QTimer>

CustomTrajectoryDialog::CustomTrajectoryDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle("Custom Trajectory Configuration");
    setMinimumWidth(400);
    setStyleSheet("QDialog { border: 3px solid #000000; }");
}

void CustomTrajectoryDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QWidget *firstRowWidget = new QWidget(this);
    QHBoxLayout *firstRowLayout = new QHBoxLayout(firstRowWidget);
    QHBoxLayout *waypointsLayout = new QHBoxLayout();
    waypointsLayout->addWidget(new QLabel("No. of Waypoints:", this));
    waypointsSpinBox = new QSpinBox(this);
    waypointsSpinBox->setRange(2, 100);
    waypointsSpinBox->setValue(9);
    waypointsSpinBox->setFixedWidth(60);
    waypointsLayout->addWidget(waypointsSpinBox);
    waypointsLayout->addSpacing(10);
    waypointsLayout->addWidget(new QLabel("Shape:", this));
    shapesComboBox = new QComboBox(this);
    shapesComboBox->addItems({"Line", "Zigzag", "Spiral", "Circle"});
    shapesComboBox->setFixedWidth(120);
    waypointsLayout->addWidget(shapesComboBox);
    waypointsLayout->addStretch();
    firstRowLayout->addLayout(waypointsLayout);
    mainLayout->addWidget(firstRowWidget);
    QFrame *hline = new QFrame();
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(hline);
    connect(shapesComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CustomTrajectoryDialog::onShapeChanged);
    lineGroupBox = new QGroupBox("Line Parameters", this);
    QVBoxLayout *lineLayout = new QVBoxLayout();
    QHBoxLayout *lengthLayout = new QHBoxLayout();
    lengthLayout->addWidget(new QLabel("Length:", this));
    lengthSpinBox = new QDoubleSpinBox(this);
    lengthSpinBox->setRange(10, 1000.0);
    lengthSpinBox->setValue(100.0);
    lengthSpinBox->setSuffix(" km");
    lengthLayout->addWidget(lengthSpinBox);
    lengthLayout->addStretch();
    lineLayout->addLayout(lengthLayout);
    lineGroupBox->setLayout(lineLayout);
    mainLayout->addWidget(lineGroupBox);
    circleGroupBox = new QGroupBox("Circle Parameters", this);
    QVBoxLayout *circleLayout = new QVBoxLayout();
    QHBoxLayout *radiusLayout = new QHBoxLayout();
    radiusLayout->addWidget(new QLabel("Radius:", this));
    radiusSpinBox = new QDoubleSpinBox(this);
    radiusSpinBox->setRange(10, 500.0);
    radiusSpinBox->setValue(100.0);
    radiusSpinBox->setSuffix(" km");
    radiusLayout->addWidget(radiusSpinBox);
    radiusLayout->addStretch();
    circleLayout->addLayout(radiusLayout);
    QHBoxLayout *startPointLayout = new QHBoxLayout();
    startPointLayout->addWidget(new QLabel("Start Point:", this));
    startPointComboBox = new QComboBox(this);
    startPointComboBox->addItems({"center","intial"});
    startPointLayout->addWidget(startPointComboBox);
    startPointLayout->addStretch();
    circleLayout->addLayout(startPointLayout);
    QHBoxLayout *circlePointLayout = new QHBoxLayout();
    circlePointLayout->addWidget(new QLabel("Circle Point:", this));
    circlePointComboBox = new QComboBox(this);
    circlePointComboBox->addItems({"Clockwise", "Anti-clockwise"});
    circlePointLayout->addWidget(circlePointComboBox);
    circlePointLayout->addStretch();
    circleLayout->addLayout(circlePointLayout);
    circleGroupBox->setLayout(circleLayout);
    circleGroupBox->setVisible(false);
    mainLayout->addWidget(circleGroupBox);
    ovalGroupBox = new QGroupBox("Oval Parameters", this);
    QVBoxLayout *ovalLayout = new QVBoxLayout();
    QHBoxLayout *majorAxisLayout = new QHBoxLayout();
    majorAxisLayout->addWidget(new QLabel("Major Axis:", this));
    majorAxisSpinBox = new QDoubleSpinBox(this);
    majorAxisSpinBox->setRange(0.1, 1000.0);
    majorAxisSpinBox->setValue(10.0);
    majorAxisSpinBox->setSuffix(" km");
    majorAxisLayout->addWidget(majorAxisSpinBox);
    majorAxisLayout->addStretch();
    ovalLayout->addLayout(majorAxisLayout);
    QHBoxLayout *minorAxisLayout = new QHBoxLayout();
    minorAxisLayout->addWidget(new QLabel("Minor Axis:", this));
    minorAxisSpinBox = new QDoubleSpinBox(this);
    minorAxisSpinBox->setRange(0.1, 500.0);
    minorAxisSpinBox->setValue(5.0);
    minorAxisSpinBox->setSuffix(" km");
    minorAxisLayout->addWidget(minorAxisSpinBox);
    minorAxisLayout->addStretch();
    ovalLayout->addLayout(minorAxisLayout);
    QHBoxLayout *orientationLayout = new QHBoxLayout();
    orientationLayout->addWidget(new QLabel("Orientation:", this));
    orientationComboBox = new QComboBox(this);
    orientationComboBox->addItems({"Horizontal", "Vertical"});
    orientationLayout->addWidget(orientationComboBox);
    orientationLayout->addStretch();
    ovalLayout->addLayout(orientationLayout);
    ovalGroupBox->setLayout(ovalLayout);
    ovalGroupBox->setVisible(false);
    mainLayout->addWidget(ovalGroupBox);
    speedGroupBox = new QGroupBox("Speed Configuration", this);
    QVBoxLayout *speedLayout = new QVBoxLayout();

    speedCheckBox = new QCheckBox("Enable Speed Control", this);
    speedCheckBox->setChecked(true);
    speedLayout->addWidget(speedCheckBox);

    // Min/Max speed
    QHBoxLayout *minSpeedLayout = new QHBoxLayout();
    minSpeedLayout->addWidget(new QLabel("start Speed:", this));
    minSpeedSpinBox = new QDoubleSpinBox(this);
    minSpeedSpinBox->setRange(0, 1000);
    minSpeedSpinBox->setValue(800);
    minSpeedSpinBox->setSuffix(" km/h");
    connect(minSpeedSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [=](double newValue) {
                if(maxSpeedSpinBox->value()<newValue){
                    minSpeedSpinBox->setValue(maxSpeedSpinBox->value());
                }
            });
    minSpeedLayout->addWidget(minSpeedSpinBox);
    minSpeedLayout->addStretch();
    speedLayout->addLayout(minSpeedLayout);



    QHBoxLayout *maxSpeedLayout = new QHBoxLayout();
    maxSpeedLayout->addWidget(new QLabel("end Speed:", this));
    maxSpeedSpinBox = new QDoubleSpinBox(this);
    maxSpeedSpinBox->setRange(0, 5000);
    maxSpeedSpinBox->setValue(2500);
    maxSpeedSpinBox->setSuffix(" km/h");
    connect(maxSpeedSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [=](double newValue) {
                if(minSpeedSpinBox->value()>newValue){
                    maxSpeedSpinBox->setValue(minSpeedSpinBox->value());
                }
            });
    maxSpeedLayout->addWidget(maxSpeedSpinBox);
    maxSpeedLayout->addStretch();
    speedLayout->addLayout(maxSpeedLayout);

    speedGroupBox->setLayout(speedLayout);
    connect(speedCheckBox, &QCheckBox::stateChanged,
            this, &CustomTrajectoryDialog::onSpeedCheckboxChanged);
    mainLayout->addWidget(speedGroupBox);

    // Altitude group
    altitudeGroupBox = new QGroupBox("Altitude Configuration", this);
    QVBoxLayout *altitudeLayout = new QVBoxLayout();

    altitudeCheckBox = new QCheckBox("Enable Altitude Control", this);
    altitudeCheckBox->setChecked(true);
    altitudeLayout->addWidget(altitudeCheckBox);

    QHBoxLayout *minaltitudeValueLayout = new QHBoxLayout();
    minaltitudeValueLayout->addWidget(new QLabel("Start Altitude:", this));
    minAltitudeSpinBox = new QDoubleSpinBox(this);
    minAltitudeSpinBox->setRange(0, 50000);
    minAltitudeSpinBox->setValue(1000);
    minAltitudeSpinBox->setSuffix(" ft");
    connect(minAltitudeSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [=](double newValue) {
                if(maxAltitudeSpinBox->value()<newValue){
                    minAltitudeSpinBox->setValue(maxAltitudeSpinBox->value());
                }
            });
    minaltitudeValueLayout->addWidget(minAltitudeSpinBox);
    minaltitudeValueLayout->addStretch();
    altitudeLayout->addLayout(minaltitudeValueLayout);



    QHBoxLayout *maxaltitudeValueLayout = new QHBoxLayout();
    maxaltitudeValueLayout->addWidget(new QLabel("End Altitude:", this));
    maxAltitudeSpinBox = new QDoubleSpinBox(this);
    maxAltitudeSpinBox->setRange(0, 50000);
    maxAltitudeSpinBox->setValue(10000);
    maxAltitudeSpinBox->setSuffix(" ft");
    connect(maxAltitudeSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [=](double newValue) {
        if(minAltitudeSpinBox->value()>newValue){
            maxAltitudeSpinBox->setValue(minAltitudeSpinBox->value());
            }
        });
    maxaltitudeValueLayout->addWidget(maxAltitudeSpinBox);
    maxaltitudeValueLayout->addStretch();
    altitudeLayout->addLayout(maxaltitudeValueLayout);


    altitudeGroupBox->setLayout(altitudeLayout);
    connect(altitudeCheckBox, &QCheckBox::stateChanged,
            this, &CustomTrajectoryDialog::onAltitudeCheckboxChanged);
    mainLayout->addWidget(altitudeGroupBox);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CustomTrajectoryDialog::onShapeChanged(int index)
{
    QString shape = shapesComboBox->itemText(index);
    bool showLine = (shape == "Line" || shape == "Zigzag" || shape == "Rectangle");
    lineGroupBox->setVisible(showLine);
    bool showCircle = (shape == "Circle" || shape == "Spiral");
    circleGroupBox->setVisible(showCircle);
    bool showOval = (shape == "Oval");
    ovalGroupBox->setVisible(showOval);
    if (shape == "Line") {
        lineGroupBox->setTitle("Line Parameters");
    } else if (shape == "Zigzag") {
        lineGroupBox->setTitle("Zigzag Parameters");
    } else if (shape == "Rectangle") {
        lineGroupBox->setTitle("Rectangle Parameters");
    } else if (shape == "Circle") {
        circleGroupBox->setTitle("Circle Parameters");
    } else if (shape == "Spiral") {
        ovalGroupBox->setTitle("Spiral Parameters");
    }
    QTimer::singleShot(10, this, [this]() {
        adjustSize();

        if (width() < 400) {
            setMinimumWidth(400);
            adjustSize();
        }
    });
}

void CustomTrajectoryDialog::onSpeedCheckboxChanged(int state)
{
    bool enabled = (state == Qt::Checked);
    minSpeedSpinBox->setEnabled(enabled);
    maxSpeedSpinBox->setEnabled(enabled);
}

void CustomTrajectoryDialog::onAltitudeCheckboxChanged(int state)
{
    bool enabled = (state == Qt::Checked);
    //altitudeSpinBox->setEnabled(enabled);
    minAltitudeSpinBox->setEnabled(enabled);
    maxAltitudeSpinBox->setEnabled(enabled);
}

// Getter methods
int CustomTrajectoryDialog::getWaypointsCount() const
{
    return waypointsSpinBox->value();
}

QString CustomTrajectoryDialog::getShape() const
{
    return shapesComboBox->currentText();
}

double CustomTrajectoryDialog::getLength() const
{
    return lengthSpinBox->value();
}

double CustomTrajectoryDialog::getRadius() const
{
    return radiusSpinBox->value();
}

double CustomTrajectoryDialog::getMajorAxis() const
{
    return majorAxisSpinBox->value();
}

double CustomTrajectoryDialog::getMinorAxis() const
{
    return minorAxisSpinBox->value();
}

QString CustomTrajectoryDialog::getOrientation() const
{
    return orientationComboBox->currentText();
}

QString CustomTrajectoryDialog::getStartPoint() const
{
    return startPointComboBox->currentText();
}

QString CustomTrajectoryDialog::getCirclePoint() const
{
    return circlePointComboBox->currentText();
}

double CustomTrajectoryDialog::getMinAltitude() const
{
    return minAltitudeSpinBox->value();
}

double CustomTrajectoryDialog::getMaxAltitude() const
{
    return maxAltitudeSpinBox->value();
}


double CustomTrajectoryDialog::getMinSpeed() const
{
    return minSpeedSpinBox->value();
}

double CustomTrajectoryDialog::getMaxSpeed() const
{
    return maxSpeedSpinBox->value();
}

QString CustomTrajectoryDialog::getCurveType() const
{
    return curveComboBox->currentText();
}

bool CustomTrajectoryDialog::isAltitudeEnabled() const
{
    return altitudeCheckBox->isChecked();
}

bool CustomTrajectoryDialog::isSpeedEnabled() const
{
    return speedCheckBox->isChecked();
}
