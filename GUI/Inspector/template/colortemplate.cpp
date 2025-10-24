/* ========================================================================= */
/* File: colortemplate.cpp                                                  */
/* Purpose: Implements color picker widget for inspector table              */
/* ========================================================================= */

#include "GUI/Inspector/template/colortemplate.h"   // For color template class
#include <QColorDialog>                            // For color dialog
#include <QHBoxLayout>                             // For horizontal layout

// %%% Constructor %%%
/* Initialize color template widget */
ColorTemplate::ColorTemplate(QWidget *parent)
    : QWidget(parent)
{
    // No additional initialization needed
}

// %%% Setup Color Cell %%%
/* Setup color picker cell in table */
void ColorTemplate::setupColorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    // Get current color value
    QString currentColor = obj["value"].toString();
    // Create color button
    QPushButton *colorBtn = new QPushButton(currentColor, this);
    colorBtn->setStyleSheet(QString("background-color: %1; color: white; border: 1px solid #555; border-radius: 3px;").arg(currentColor));

    // Create layout for button
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->addWidget(colorBtn);
    layout->addStretch();

    // Set row height and cell widget
    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);

    // Connect button click to color dialog
    connect(colorBtn, &QPushButton::clicked, this, [=]() {
        // Open color dialog
        QColor color = QColorDialog::getColor(QColor(currentColor), this, "Select Color");
        if (color.isValid()) {
            // Update button with new color
            QString hex = color.name();
            colorBtn->setText(hex);
            colorBtn->setStyleSheet(QString("background-color: %1; color: white; border: 1px solid #555; border-radius: 3px;").arg(hex));
            // Emit value changed signal
            QJsonObject delta;
            QJsonObject colorObj;
            colorObj["value"] = hex;
            delta[fullKey] = colorObj;
            emit valueChanged(connectedID, name, delta);
        }
    });
}
