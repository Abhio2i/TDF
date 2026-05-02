/* =============================================================================
 * FILE:         optiontemplate.cpp
 * MODULE:       Option Template Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the OptionTemplate class which provides a widget for
 *               managing option templates (dropdown selections) within the
 *               Inspector panel. It sets up option cells in a table, maintains
 *               connected entity IDs and template names, and emits value
 *               changes when an option is modified.
 *
 * REQUIREMENTS: Implements REQ-OPTION-010 through REQ-OPTION-013
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-OPTION-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "optiontemplate.h"
#include <QHBoxLayout>
#include <QComboBox>
#include "GUI/Inspector/inspector-styles.h"

OptionTemplate::OptionTemplate(QWidget *parent)
    : QWidget(parent), inspectorRef(nullptr) {}

void OptionTemplate::setupOptionCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QComboBox *combo = new QComboBox(this);
    combo->setStyleSheet(InspectorStyles::OptionComboBox);

    QJsonArray options = obj["options"].toArray();
    QString selected = obj["value"].toString();

    for (const QJsonValue &val : options) {
        combo->addItem(val.toString());
    }

    int index = combo->findText(selected);
    if (index != -1)
        combo->setCurrentIndex(index);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->addWidget(combo);
    layout->addStretch();

    connect(combo, &QComboBox::currentTextChanged, this, [=](const QString &text) {
        QJsonObject optionObj;
        optionObj["type"] = "option";
        optionObj["value"] = text;
        optionObj["options"] = obj["options"].toArray();

        QJsonObject delta;
        QStringList parts = fullKey.split(".");

        if (parts.size() == 2) {
            QJsonObject parentObj;
            parentObj[parts[1]] = optionObj;
            delta[parts[0]] = parentObj;
        } else {
            delta[fullKey] = optionObj;
        }

   delta["_id"] = mainID;
        emit valueChanged(connectedID, name, delta);
    });

    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}


