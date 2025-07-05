
#include "customparameterdialog.h"
#include <QMessageBox>
#include <QRegularExpression>

CustomParameterDialog::CustomParameterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Add Custom Parameter");
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *nameLabel = new QLabel("Parameter Name:", this);
    nameEdit = new QLineEdit(this);
    layout->addWidget(nameLabel);
    layout->addWidget(nameEdit);

    QLabel *typeLabel = new QLabel("Parameter Type:", this);
    typeCombo = new QComboBox(this);
    typeCombo->addItems({"string", "number", "boolean", "vector", "option", "color", "image"});
    layout->addWidget(typeLabel);
    layout->addWidget(typeCombo);

    QLabel *valueLabel = new QLabel("Parameter Value:", this);
    valueEdit = new QLineEdit(this);
    layout->addWidget(valueLabel);
    layout->addWidget(valueEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &CustomParameterDialog::validateInput);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    setLayout(layout);
}

QString CustomParameterDialog::getParameterName() const
{
    return nameEdit->text();
}

QString CustomParameterDialog::getParameterType() const
{
    return typeCombo->currentText();
}

QString CustomParameterDialog::getParameterValue() const
{
    return valueEdit->text();
}

void CustomParameterDialog::validateInput()
{
    QString name = nameEdit->text().trimmed();
    QString type = typeCombo->currentText();
    QString value = valueEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Parameter name cannot be empty.");
        return;
    }

    if (type == "string") {
        if (value.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "String value cannot be empty.");
            return;
        }
    } else if (type == "number") {
        bool ok;
        value.toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "Invalid Input", "Number value must be a valid number.");
            return;
        }
    } else if (type == "boolean") {
        QString lowerValue = value.toLower();
        if (lowerValue != "true" && lowerValue != "false") {
            QMessageBox::warning(this, "Invalid Input", "Boolean value must be 'true' or 'false'.");
            return;
        }
    } else if (type == "vector") {
        QStringList components = value.split(",");
        if (components.size() != 3) {
            QMessageBox::warning(this, "Invalid Input", "Vector value must have 3 components (e.g., '1.0,2.0,3.0').");
            return;
        }
        bool ok;
        for (const QString &comp : components) {
            comp.trimmed().toDouble(&ok);
            if (!ok) {
                QMessageBox::warning(this, "Invalid Input", "Each vector component must be a valid number.");
                return;
            }
        }
    } else if (type == "option") {
        if (value.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Option value cannot be empty.");
            return;
        }
    } else if (type == "color") {
        QRegularExpression hexRegex("^#[0-9A-Fa-f]{6}$");
        if (!hexRegex.match(value).hasMatch()) {
            QMessageBox::warning(this, "Invalid Input", "Color value must be a valid hex code (e.g., '#FFFFFF').");
            return;
        }
    } else if (type == "image") {
        if (value.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Image path cannot be empty.");
            return;
        }
    }

    accept();
}
