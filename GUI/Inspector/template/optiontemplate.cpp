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

        emit valueChanged(connectedID, name, delta);
    });

    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}


