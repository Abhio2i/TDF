

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
    , inspectorRef(nullptr)
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
    combo->setStyleSheet(
        "QComboBox { background: white; border: 1px solid #ccc; border-radius: 3px; color: black; padding: 2px; }"
        "QComboBox:hover { border: 1px solid #999; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox::down-arrow { width: 12px; height: 12px; }"
        "QComboBox QAbstractItemView { background: white; border: 1px solid #ccc; color: black; selection-background-color: #e6f3ff; }"
        "QComboBox QAbstractItemView::item { padding: 4px; }"
        "QComboBox QAbstractItemView::item:selected { background-color: #e6f3ff; color: black; }"
        );

    // Create layout for dropdown
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->addWidget(combo);
    layout->addStretch();

    // Connect dropdown selection change
    connect(combo, &QComboBox::currentTextChanged, this, [=](const QString &text) {
        QJsonObject optionObj;
        optionObj["type"] = "option";
        optionObj["value"] = text;

        // Preserve existing options
        QJsonArray options = obj["options"].toArray();
        optionObj["options"] = options;

        QJsonObject delta;

        // Special handling for nested keys like "passabillity.terrainSurface"
        QStringList parts = fullKey.split(".");
        if (parts.size() == 2) {
            // Nested case: e.g., "passabillity" -> "terrainSurface"
            QJsonObject parentObj;
            parentObj[parts[1]] = optionObj;
            delta[parts[0]] = parentObj;
        } else {
            // Simple top-level case
            delta[fullKey] = optionObj;
        }

        emit valueChanged(connectedID, name, delta);
    });

    // Set row height and add widget to table
    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}
