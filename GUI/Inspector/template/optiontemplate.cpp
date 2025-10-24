/* ========================================================================= */
/* File: optiontemplate.cpp                                                 */
/* Purpose: Implements dropdown option widget for inspector table            */
/* ========================================================================= */

#include "optiontemplate.h"                        // For option template class
#include <QHBoxLayout>                             // For horizontal layout
#include <QComboBox>                               // For dropdown menu

// %%% Constructor %%%
/* Initialize option template widget */
OptionTemplate::OptionTemplate(QWidget *parent)
    : QWidget(parent)
{
    // No additional initialization needed
}

// %%% Setup Option Cell %%%
/* Setup dropdown option cell in table */
void OptionTemplate::setupOptionCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    // Create dropdown menu
    QComboBox *combo = new QComboBox(this);
    QJsonArray options = obj["options"].toArray();
    QString selected = obj["value"].toString();

    // Populate dropdown with options
    for (const QJsonValue &val : options) {
        combo->addItem(val.toString());
    }

    // Set selected option
    int index = combo->findText(selected);
    if (index != -1)
        combo->setCurrentIndex(index);

    // Style dropdown menu
    combo->setStyleSheet("QComboBox { background: #333; border: 1px solid #555; border-radius: 3px; color: white; padding: 2px; }"
                         "QComboBox::drop-down { border: none; }"
                         "QComboBox QAbstractItemView { background: #333; color: white; selection-background-color: #555; }");

    // Create layout for dropdown
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->addWidget(combo);
    layout->addStretch();

    // Connect dropdown selection change
    connect(combo, &QComboBox::currentTextChanged, this, [=](const QString &text) {
        // Emit value changed signal
        QJsonObject delta;
        QJsonObject optionObj;
        optionObj["value"] = text;
        delta[fullKey] = optionObj;
        emit valueChanged(connectedID, name, delta);
    });

    // Set row height and add widget to table
    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}
