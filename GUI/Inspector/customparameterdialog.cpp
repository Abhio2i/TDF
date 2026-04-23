
/* =============================================================================
 * FILE:         customparameterdialog.cpp
 * MODULE:       Custom Parameter Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the CustomParameterDialog class which provides a modal
 *               dialog for adding custom parameters (name, type, value). It is
 *               used within the Inspector panel to allow users to extend entity
 *               or component definitions with user-defined key-value pairs.
 *
 * REQUIREMENTS: Implements REQ-CUSTOMPARAM-010 through REQ-CUSTOMPARAM-014
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-CUSTOMPARAM-001
 *
 * MODIFIED:     Added QCheckBox for boolean type to replace manual true/false entry.
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "customparameterdialog.h"                 // For custom parameter dialog class
#include <QMessageBox>                             // For warning messages
#include <QRegularExpression>                      // For regex validation
#include <QVBoxLayout>                             // For vertical layout
#include <QHBoxLayout>                             // For horizontal layout
#include <QLabel>                                  // For labels
#include <QLineEdit>                               // For input fields
#include <QComboBox>                               // For dropdown menu
#include <QCheckBox>                               // For boolean checkbox
#include <QStackedWidget>                          // For switching between line edit and checkbox
#include <QPushButton>                             // For buttons
#include <QAbstractItemView>

// %%% Constructor %%%
CustomParameterDialog::CustomParameterDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Add Custom Parameter");

    // ── Dialog size fix ───────────────────────────────────────────────────
    setMinimumWidth(350);
    setMinimumHeight(250);
    // ─────────────────────────────────────────────────────────────────────

    // ── Dark theme stylesheet ─────────────────────────────────────────────
    setStyleSheet(R"(
        QDialog {
            background-color: #0F2636;
            color: #E0E0E0;
        }
        QLabel {
            color: #B0B0B0;
            font-size: 12px;
            padding-top: 4px;
        }
        QLineEdit {
            background-color: #1A3652;
            color: #E0E0E0;
            border: 1px solid #27446d;
            border-radius: 3px;
            padding: 5px 8px;
            font-size: 12px;
            min-height: 28px;
        }
        QLineEdit:focus {
            border: 1px solid #0078D4;
        }
        QComboBox {
            background-color: #1A3652;
            color: #E0E0E0;
            border: 1px solid #27446d;
            border-radius: 3px;
            padding: 5px 8px;
            font-size: 12px;
            min-height: 28px;
        }
        QComboBox:focus {
            border: 1px solid #0078D4;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox::down-arrow {
            image: url(:/icons/images/down.png);
            width: 12px;
            height: 12px;
            margin-right: 6px;
        }
        QComboBox QAbstractItemView {
            background-color: #0F2636;
            color: #E0E0E0;
            border: 1px solid #27446d;
            selection-background-color: #1A3652;
            selection-color: #FFFFFF;
            outline: none;
        }
        QComboBox QAbstractItemView::item {
            padding: 6px 8px;
            min-height: 28px;
            background-color: #0F2636;
            color: #E0E0E0;
        }
        QComboBox QAbstractItemView::item:hover {
            background-color: #1A3652;
            color: #FFFFFF;
        }
        QComboBox QAbstractItemView::item:selected {
            background-color: #0078D4;
            color: #FFFFFF;
        }
        QPushButton {
            background-color: #1A3652;
            color: #E0E0E0;
            border: 1px solid #27446d;
            border-radius: 3px;
            padding: 6px 16px;
            font-size: 12px;
            min-width: 70px;
            min-height: 28px;
        }
        QPushButton:hover {
            background-color: #0078D4;
            border: 1px solid #0078D4;
            color: #FFFFFF;
        }
        QPushButton:pressed {
            background-color: #005A9E;
        }
        QCheckBox {
            color: #E0E0E0;
            spacing: 8px;
            min-height: 28px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 1px solid #27446d;
            border-radius: 3px;
            background-color: #1A3652;
        }
      QCheckBox::indicator:checked {
            image: url(:/icons/images/check-box.png);
            background-color: transparent;
            border: none;
        }
    )");
    // ─────────────────────────────────────────────────────────────────────

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    // Parameter name
    QLabel *nameLabel = new QLabel("Parameter Name:", this);
    nameEdit = new QLineEdit(this);
    layout->addWidget(nameLabel);
    layout->addWidget(nameEdit);

    // Parameter type
    QLabel *typeLabel = new QLabel("Parameter Type:", this);
    typeCombo = new QComboBox(this);
    typeCombo->addItems({"string", "number", "boolean"});

    typeCombo->view()->setStyleSheet(R"(
        QAbstractItemView {
            background-color: #0F2636;
            color: #E0E0E0;
            border: 1px solid #27446d;
            selection-background-color: #0078D4;
            outline: none;
        }
        QAbstractItemView::item {
            padding: 6px 8px;
            min-height: 28px;
            color: #E0E0E0;
            background-color: #0F2636;
        }
        QAbstractItemView::item:hover {
            background-color: #1A3652;
        }
        QAbstractItemView::item:selected {
            background-color: #0078D4;
            color: white;
        }
    )");

    layout->addWidget(typeLabel);
    layout->addWidget(typeCombo);

    // Parameter value - stacked widget for line edit (non-boolean) and checkbox (boolean)
    QLabel *valueLabel = new QLabel("Parameter Value:", this);
    layout->addWidget(valueLabel);

    valueStack = new QStackedWidget(this);

    // Widget index 0: QLineEdit for string, number, and other types
    valueEdit = new QLineEdit(this);
    valueStack->addWidget(valueEdit);

    // Widget index 1: QCheckBox for boolean
    valueCheckBox = new QCheckBox(" ", this);
    valueStack->addWidget(valueCheckBox);
    layout->addWidget(valueStack);
    layout->addSpacing(8);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    QPushButton *okButton = new QPushButton("OK", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // Connections
    connect(okButton, &QPushButton::clicked, this, &CustomParameterDialog::validateInput);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(typeCombo, &QComboBox::currentTextChanged, this, &CustomParameterDialog::onTypeChanged);

    // Initial setup: show appropriate widget based on default type ("string")
    onTypeChanged(typeCombo->currentText());

    setLayout(layout);
}

// %%% Getter Methods %%%
/* Get parameter name */
QString CustomParameterDialog::getParameterName() const
{
    return nameEdit->text();
}

/* Get parameter type */
QString CustomParameterDialog::getParameterType() const
{
    return typeCombo->currentText();
}

/* Get parameter value - returns "true"/"false" for boolean, otherwise trimmed text */
QString CustomParameterDialog::getParameterValue() const
{
    if (typeCombo->currentText() == "boolean") {
        return valueCheckBox->isChecked() ? "true" : "false";
    } else {
        return valueEdit->text().trimmed();
    }
}

// %%% Slot: Handle type change %%%
void CustomParameterDialog::onTypeChanged(const QString &type)
{
    if (type == "boolean") {
        valueStack->setCurrentIndex(1);
    } else {
        valueStack->setCurrentIndex(0);
    }
}


// %%% Input Validation %%%
void CustomParameterDialog::validateInput()
{
    QString name = nameEdit->text().trimmed();
    QString type = typeCombo->currentText();

    // Validate name
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Parameter name cannot be empty.");
        return;
    }
    bool onlyDigits = true;
    for (QChar ch : name) {
        if (!ch.isDigit()) {
            onlyDigits = false;
            break;
        }
    }
    if (onlyDigits) {
        QMessageBox::warning(this, "Invalid Input", "Parameter name cannot be a numeric value only (e.g., '1234'). Please use letters, numbers, or underscores.");
        return;
    }
    if (type == "string") {
        QString value = valueEdit->text().trimmed();
        if (value.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "String value cannot be empty.");
            return;
        }
    }
    else if (type == "number") {
        QString value = valueEdit->text().trimmed();
        bool ok;
        value.toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "Invalid Input", "Number value must be a valid number.");
            return;
        }
    }

    else if (type == "boolean") {
    }
    else if (type == "vector") {
        QString value = valueEdit->text().trimmed();
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
    else if (type == "option") {
        QString value = valueEdit->text().trimmed();
        if (value.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Option value cannot be empty.");
            return;
        }
    }
    else if (type == "color") {
        QString value = valueEdit->text().trimmed();
        QRegularExpression hexRegex("^#[0-9A-Fa-f]{6}$");
        if (!hexRegex.match(value).hasMatch()) {
            QMessageBox::warning(this, "Invalid Input", "Color value must be a valid hex code (e.g., '#FFFFFF').");
            return;
        }
    }
    else if (type == "image") {
        QString value = valueEdit->text().trimmed();
        if (value.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Image path cannot be empty.");
            return;
        }
    }
    accept();
}
