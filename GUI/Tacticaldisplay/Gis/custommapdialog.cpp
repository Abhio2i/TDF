#include "custommapdialog.h"

CustomMapDialog::CustomMapDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Add Custom Map");
    QVBoxLayout *layout = new QVBoxLayout(this);

    QFormLayout *formLayout = new QFormLayout();

    mapNameEdit = new QLineEdit(this);
    mapNameEdit->setPlaceholderText("e.g., My Custom Map");
    formLayout->addRow("Map Name:", mapNameEdit);

    zoomMinSpinBox = new QSpinBox(this);
    zoomMinSpinBox->setRange(0, 19);
    zoomMinSpinBox->setValue(0);
    formLayout->addRow("Minimum Zoom Level:", zoomMinSpinBox);

    zoomMaxSpinBox = new QSpinBox(this);
    zoomMaxSpinBox->setRange(0, 19);
    zoomMaxSpinBox->setValue(19);
    formLayout->addRow("Maximum Zoom Level:", zoomMaxSpinBox);

    tileUrlEdit = new QLineEdit(this);
    tileUrlEdit->setPlaceholderText("e.g., https://example.com/{z}/{x}/{y}.png");
    formLayout->addRow("Map URL:", tileUrlEdit);

    layout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

int CustomMapDialog::getZoomMin() const {
    return zoomMinSpinBox->value();
}

int CustomMapDialog::getZoomMax() const {
    return zoomMaxSpinBox->value();
}

QString CustomMapDialog::getTileUrl() const {
    return tileUrlEdit->text();
}

QString CustomMapDialog::getMapName() const {
    return mapNameEdit->text();
}
