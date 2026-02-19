#include "GUI/Inspector/template/colortemplate.h"
#include <QColorDialog>
#include <QHBoxLayout>
#include "GUI/Inspector/inspector.h"
#include "GUI/Inspector/inspector-styles.h"

ColorTemplate::ColorTemplate(Inspector *inspector, QWidget *parent)
    : QWidget(parent), inspectorRef(inspector) {}

void ColorTemplate::setupColorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QString currentColor = obj["value"].toString();

    QPushButton *colorBtn = new QPushButton(currentColor, this);
    colorBtn->setStyleSheet(InspectorStyles::ColorButton +
                            QString(" background-color: %1;").arg(currentColor));

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
            colorBtn->setStyleSheet(InspectorStyles::ColorButton +
                                    QString(" background-color: %1;").arg(hex));

            QJsonObject delta;
            QJsonObject colorObj;
            colorObj["value"] = hex;
            delta[fullKey] = colorObj;

            if (inspectorRef) {
                delta["_id"] = inspectorRef->getMainID();
            }

            emit valueChanged(connectedID, name, delta);
        }
    });
}
