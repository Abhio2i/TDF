/* ========================================================================= */
/* File: geocordstemplate.cpp                                               */
/* Purpose: Implements geocoordinates input widget for inspector table       */
/* ========================================================================= */

#include "geocordstemplate.h"                      // For geocoordinates template class
#include <QHBoxLayout>                             // For horizontal layout
#include <QLineEdit>                               // For input fields
#include <QDoubleValidator>                        // For double validation
#include <QLabel>                                  // For labels
#include "GUI/Inspector/inspector.h"               // For formatNumberForUI

// %%% Constructor %%%
/* Initialize geocoordinates template widget */
GeocordsTemplate::GeocordsTemplate(QWidget *parent)
    : QWidget(parent)
{
    // No additional initialization needed
}

// %%% Setup Geocords Cell %%%
/* Setup geocoordinates input cell in table */
void GeocordsTemplate::setupGeocordsCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    // Create main vertical layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(2);

    // Create input fields
    QLineEdit *latEdit = new QLineEdit(this);
    QLineEdit *lonEdit = new QLineEdit(this);
    QLineEdit *altEdit = new QLineEdit(this);
    QLineEdit *headEdit = new QLineEdit(this);

    // Set double validator
    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setDecimals(6);
    latEdit->setValidator(validator);
    lonEdit->setValidator(validator);
    altEdit->setValidator(validator);
    headEdit->setValidator(validator);

    // Set initial values from JSON
    latEdit->setText(Inspector::formatNumberForUI(obj.value("latitude").toDouble(0.0)));
    lonEdit->setText(Inspector::formatNumberForUI(obj.value("longitude").toDouble(0.0)));
    altEdit->setText(Inspector::formatNumberForUI(obj.value("altitude").toDouble(0.0)));
    headEdit->setText(Inspector::formatNumberForUI(obj.value("heading").toDouble(0.0)));

    // Style input fields and labels
    QString inputStyle = "QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; color: white; }";
    QString labelStyle = "QLabel { color: white; min-width: 40px; }";
    latEdit->setStyleSheet(inputStyle);
    lonEdit->setStyleSheet(inputStyle);
    altEdit->setStyleSheet(inputStyle);
    headEdit->setStyleSheet(inputStyle);

    // Create labels
    QLabel *latLabel = new QLabel("Lat:", this);
    QLabel *lonLabel = new QLabel("Lon:", this);
    QLabel *altLabel = new QLabel("Alt:", this);
    QLabel *headLabel = new QLabel("Head:", this);
    latLabel->setStyleSheet(labelStyle);
    lonLabel->setStyleSheet(labelStyle);
    altLabel->setStyleSheet(labelStyle);
    headLabel->setStyleSheet(labelStyle);

    // Create horizontal layouts for each field
    QHBoxLayout *latLayout = new QHBoxLayout();
    latLayout->addWidget(latLabel);
    latLayout->addWidget(latEdit);
    latLayout->addStretch();

    QHBoxLayout *lonLayout = new QHBoxLayout();
    lonLayout->addWidget(lonLabel);
    lonLayout->addWidget(lonEdit);
    lonLayout->addStretch();

    QHBoxLayout *altLayout = new QHBoxLayout();
    altLayout->addWidget(altLabel);
    altLayout->addWidget(altEdit);
    altLayout->addStretch();

    QHBoxLayout *headLayout = new QHBoxLayout();
    headLayout->addWidget(headLabel);
    headLayout->addWidget(headEdit);
    headLayout->addStretch();

    // Add layouts to main layout
    mainLayout->addLayout(latLayout);
    mainLayout->addLayout(lonLayout);
    mainLayout->addLayout(altLayout);
    mainLayout->addLayout(headLayout);
    mainLayout->addStretch();

    // Connect editing finished signals
    auto updateValue = [=]() {
        QJsonObject delta;
        QJsonObject geocordObj;
        geocordObj["latitude"] = latEdit->text().toDouble();
        geocordObj["longitude"] = lonEdit->text().toDouble();
        geocordObj["altitude"] = altEdit->text().toDouble();
        geocordObj["heading"] = headEdit->text().toDouble();
        geocordObj["type"] = obj["type"].toString("geocord");
        delta[fullKey] = geocordObj;
        emit valueChanged(connectedID, name, delta);
    };

    connect(latEdit, &QLineEdit::editingFinished, this, updateValue);
    connect(lonEdit, &QLineEdit::editingFinished, this, updateValue);
    connect(altEdit, &QLineEdit::editingFinished, this, updateValue);
    connect(headEdit, &QLineEdit::editingFinished, this, updateValue);

    // Set row height and add widget to table
    tableWidget->setRowHeight(row, ROW_HEIGHT * 4);
    tableWidget->setCellWidget(row, 1, this);
}
