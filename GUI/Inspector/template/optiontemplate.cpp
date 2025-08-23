#include "optiontemplate.h"
#include <QHBoxLayout>
#include <QComboBox>

OptionTemplate::OptionTemplate(QWidget *parent) : QWidget(parent) {}

void OptionTemplate::setupOptionCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QComboBox *combo = new QComboBox(this);
    QJsonArray options = obj["options"].toArray();
    QString selected = obj["value"].toString();

    for (const QJsonValue &val : options) {
        combo->addItem(val.toString());
    }

    int index = combo->findText(selected);
    if (index != -1)
        combo->setCurrentIndex(index);

    // Style the QComboBox to set text color to white
    combo->setStyleSheet("QComboBox { background: #333; border: 1px solid #555; border-radius: 3px; color: white; padding: 2px; }"
                         "QComboBox::drop-down { border: none; }"
                         "QComboBox QAbstractItemView { background: #333; color: white; selection-background-color: #555; }");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->addWidget(combo);
    layout->addStretch();

    connect(combo, &QComboBox::currentTextChanged, this, [=](const QString &text) {
        QJsonObject delta;
        QJsonObject optionObj;
        optionObj["value"] = text;
        delta[fullKey] = optionObj;
        emit valueChanged(connectedID, name, delta);
    });

    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}
