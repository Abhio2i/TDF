

#include "custommapdialog.h"

CustomMapDialog::CustomMapDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Add Custom Map");
    setupUi();
}

CustomMapDialog::CustomMapDialog(const QString &name, const QString &tileUrl, int zoomMin, int zoomMax,
                                 qreal opacity, const QString &type, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Edit Custom Map");
    setupUi();
    mapNameEdit->setText(name);
    tileUrlEdit->setText(tileUrl);
    zoomMinSpinBox->setValue(zoomMin);
    zoomMaxSpinBox->setValue(zoomMax);


    QString opacityText = opacity >= 0.0 ? QString::number(opacity * 100, 'f', 0) + "%" : "100%";
    opacityComboBox->setCurrentText(opacityText);



    typeComboBox->setCurrentText(type.isEmpty() || type == "N/A" ? "Raster" : type);
}

void CustomMapDialog::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    mapNameEdit = new QLineEdit(this);
    mapNameEdit->setPlaceholderText("e.g., My Custom Map");
    formLayout->addRow("Map Name:", mapNameEdit);

    tileUrlEdit = new QLineEdit(this);
    tileUrlEdit->setPlaceholderText("e.g., https://example.com/{z}/{x}/{y}.png");
    formLayout->addRow("Map URL:", tileUrlEdit);

    zoomMinSpinBox = new QSpinBox(this);
    zoomMinSpinBox->setRange(0, 30);
    zoomMinSpinBox->setValue(0);
    formLayout->addRow("Minimum Zoom Level:", zoomMinSpinBox);

    zoomMaxSpinBox = new QSpinBox(this);
    zoomMaxSpinBox->setRange(0, 30);
    zoomMaxSpinBox->setValue(19);
    formLayout->addRow("Maximum Zoom Level:", zoomMaxSpinBox);

    opacityComboBox = new QComboBox(this);
    opacityComboBox->addItems({"0%", "20%", "50%", "80%", "100%"});
    opacityComboBox->setCurrentText("100%");
    formLayout->addRow("Opacity:", opacityComboBox);


    typeComboBox = new QComboBox(this);
    typeComboBox->addItems({"Raster", "Vector", "Other"});
    formLayout->addRow("Type:", typeComboBox);

    layout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    setLayout(layout);
}

QString CustomMapDialog::getMapName() const {
    return mapNameEdit->text().trimmed();
}

QString CustomMapDialog::getTileUrl() const {
    return tileUrlEdit->text().trimmed();
}

int CustomMapDialog::getZoomMin() const {
    return zoomMinSpinBox->value();
}

int CustomMapDialog::getZoomMax() const {
    return zoomMaxSpinBox->value();
}

qreal CustomMapDialog::getOpacity() const {
    QString selectedOpacity = opacityComboBox->currentText();
    // Remove '%' and convert to qreal (0.0 to 1.0)
    bool ok;
    qreal opacity = selectedOpacity.remove('%').toDouble(&ok) / 100.0;
    return ok ? opacity : 1.0; // Default to 1.0 if conversion fails
}

// QString CustomMapDialog::getResolution() const {
//     QString resolution = resolutionComboBox->currentText();
//     return resolution.isEmpty() ? "N/A" : resolution;
// }

QString CustomMapDialog::getType() const {
    return typeComboBox->currentText();
}
