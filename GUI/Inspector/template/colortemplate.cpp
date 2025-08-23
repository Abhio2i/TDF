
#include "GUI/Inspector/template/colortemplate.h"
#include <QColorDialog>
#include <QHBoxLayout>

ColorTemplate::ColorTemplate(QWidget *parent) : QWidget(parent) {}

void ColorTemplate::setupColorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QString currentColor = obj["value"].toString();
    QPushButton *colorBtn = new QPushButton(currentColor, this);
    colorBtn->setStyleSheet(QString("background-color: %1; color: white; border: 1px solid #555; border-radius: 3px;").arg(currentColor));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->addWidget(colorBtn);
    layout->addStretch();

    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);

    connect(colorBtn, &QPushButton::clicked, this, [=]() {
        QColor color = QColorDialog::getColor(QColor(currentColor), this, "Select Color");
        if (color.isValid()) {
            QString hex = color.name();
            colorBtn->setText(hex);
            colorBtn->setStyleSheet(QString("background-color: %1; color: white; border: 1px solid #555; border-radius: 3px;").arg(hex));
            QJsonObject delta;
            QJsonObject colorObj;
            colorObj["value"] = hex;
            delta[fullKey] = colorObj;
            emit valueChanged(connectedID, name, delta);
        }
    });
}
