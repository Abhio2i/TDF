#include "sensorsdialog.h"
#include <QLabel>

SensorsDialog::SensorsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setupConnections();

    // Set dialog properties
    setWindowTitle("Sensors Configuration");
    setMinimumWidth(300);
}

void SensorsDialog::setupUI()
{
    // Create main layout
    mainLayout = new QVBoxLayout(this);

    // Create group box for sensors
    sensorsGroupBox = new QGroupBox("Select Sensors", this);
    sensorsLayout = new QVBoxLayout(sensorsGroupBox);

    // Create checkboxes
    sensorsCheckBox = new QCheckBox("Sensors", sensorsGroupBox);
    esmCheckBox = new QCheckBox("ESM", sensorsGroupBox);
    csmCheckBox = new QCheckBox("CSM", sensorsGroupBox);
    radioCheckBox = new QCheckBox("Radio", sensorsGroupBox);
    iffCheckBox = new QCheckBox("IFF", sensorsGroupBox);

    // Set default checked states (optional)
    sensorsCheckBox->setChecked(true);
    esmCheckBox->setChecked(true);
    csmCheckBox->setChecked(true);
    radioCheckBox->setChecked(false);
    iffCheckBox->setChecked(false);

    // Add checkboxes to layout
    sensorsLayout->addWidget(sensorsCheckBox);
    sensorsLayout->addWidget(esmCheckBox);
    sensorsLayout->addWidget(csmCheckBox);
    sensorsLayout->addWidget(radioCheckBox);
    sensorsLayout->addWidget(iffCheckBox);

    // Add group box to main layout
    mainLayout->addWidget(sensorsGroupBox);

    // Create button box
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    // Add button box to main layout
    mainLayout->addWidget(buttonBox);

    // Set the layout
    setLayout(mainLayout);
}

void SensorsDialog::setupConnections()
{
    // Connect OK and Cancel buttons
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SensorsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SensorsDialog::reject);

    // You can add additional connections here if needed
    // For example, connect sensorsCheckBox to enable/disable others
    connect(sensorsCheckBox, &QCheckBox::toggled, [this](bool checked) {
        if (!checked) {
            // If Sensors is unchecked, uncheck all children
            esmCheckBox->setChecked(false);
            csmCheckBox->setChecked(false);
            radioCheckBox->setChecked(false);
            iffCheckBox->setChecked(false);
        }
    });
}

bool SensorsDialog::isSensorsChecked() const
{
    return sensorsCheckBox->isChecked();
}

bool SensorsDialog::isEsmChecked() const
{
    return esmCheckBox->isChecked();
}

bool SensorsDialog::isCsmChecked() const
{
    return csmCheckBox->isChecked();
}

bool SensorsDialog::isRadioChecked() const
{
    return radioCheckBox->isChecked();
}

bool SensorsDialog::isIffChecked() const
{
    return iffCheckBox->isChecked();
}

QStringList SensorsDialog::getSelectedSensors() const
{
    QStringList selected;

    if (isSensorsChecked()) selected << "Sensors";
    if (isEsmChecked()) selected << "ESM";
    if (isCsmChecked()) selected << "CSM";
    if (isRadioChecked()) selected << "Radio";
    if (isIffChecked()) selected << "IFF";

    return selected;
}
