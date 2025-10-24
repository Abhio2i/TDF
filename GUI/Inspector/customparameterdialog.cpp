/* ========================================================================= */
/* File: customparameterdialog.cpp                                          */
/* Purpose: Implements dialog for adding custom parameters                  */
/* ========================================================================= */

#include "customparameterdialog.h"                 // For custom parameter dialog class
#include <QMessageBox>                             // For warning messages
#include <QRegularExpression>                      // For regex validation
#include <QVBoxLayout>                             // For vertical layout
#include <QHBoxLayout>                             // For horizontal layout
#include <QLabel>                                  // For labels
#include <QLineEdit>                               // For input fields
#include <QComboBox>                               // For dropdown menu
#include <QPushButton>                             // For buttons

// %%% Constructor %%%
/* Initialize custom parameter dialog */
CustomParameterDialog::CustomParameterDialog(QWidget *parent)
    : QDialog(parent)
{
    // Set dialog title
    setWindowTitle("Add Custom Parameter");
    // Create main vertical layout
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Create name input field
    QLabel *nameLabel = new QLabel("Parameter Name:", this);
    nameEdit = new QLineEdit(this);
    layout->addWidget(nameLabel);
    layout->addWidget(nameEdit);

    // Create type dropdown
    QLabel *typeLabel = new QLabel("Parameter Type:", this);
    typeCombo = new QComboBox(this);
    typeCombo->addItems({"string", "number", "boolean", "vector", "option", "color", "image"});
    layout->addWidget(typeLabel);
    layout->addWidget(typeCombo);

    // Create value input field
    QLabel *valueLabel = new QLabel("Parameter Value:", this);
    valueEdit = new QLineEdit(this);
    layout->addWidget(valueLabel);
    layout->addWidget(valueEdit);

    // Create button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // Connect button signals
    connect(okButton, &QPushButton::clicked, this, &CustomParameterDialog::validateInput);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Set dialog layout
    setLayout(layout);
}

// %%% Getter Methods %%%
/* Get parameter name */
QString CustomParameterDialog::getParameterName() const
{
    // Return name input text
    return nameEdit->text();
}

/* Get parameter type */
QString CustomParameterDialog::getParameterType() const
{
    // Return selected type
    return typeCombo->currentText();
}

/* Get parameter value */
QString CustomParameterDialog::getParameterValue() const
{
    // Return value input text
    return valueEdit->text();
}

// %%% Input Validation %%%
/* Validate dialog input */
void CustomParameterDialog::validateInput()
{
    // Get input values
    QString name = nameEdit->text().trimmed();
    QString type = typeCombo->currentText();
    QString value = valueEdit->text().trimmed();

    // Validate name
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Parameter name cannot be empty.");
        return;
    }

    // Validate string type
    if (type == "string") {
        if (value.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "String value cannot be empty.");
            return;
        }
    }
    // Validate number type
    else if (type == "number") {
        bool ok;
        value.toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "Invalid Input", "Number value must be a valid number.");
            return;
        }
    }
    // Validate boolean type
    else if (type == "boolean") {
        QString lowerValue = value.toLower();
        if (lowerValue != "true" && lowerValue != "false") {
            QMessageBox::warning(this, "Invalid Input", "Boolean value must be 'true' or 'false'.");
            return;
        }
    }
    // Validate vector type
    else if (type == "vector") {
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
    }
    // Validate option type
    else if (type == "option") {
        if (value.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Option value cannot be empty.");
            return;
        }
    }
    // Validate color type
    else if (type == "color") {
        QRegularExpression hexRegex("^#[0-9A-Fa-f]{6}$");
        if (!hexRegex.match(value).hasMatch()) {
            QMessageBox::warning(this, "Invalid Input", "Color value must be a valid hex code (e.g., '#FFFFFF').");
            return;
        }
    }
    // Validate image type
    else if (type == "image") {
        if (value.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Image path cannot be empty.");
            return;
        }
    }

    // Accept valid input
    accept();
}
