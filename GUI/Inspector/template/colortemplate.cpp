/* ========================================================================= */
/* File: colortemplate.cpp                                                   */
/* Purpose: Implements color picker widget for inspector table cells         */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */
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

    auto getContrastColor = [](const QString &hexColor) -> QString {
        QColor color(hexColor);
        if (!color.isValid()) return "#ffffff";
        int r = color.red(), g = color.green(), b = color.blue();
        double luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0;
        return (luminance > 0.5) ? "#000000" : "#ffffff";
    };

    auto makeButtonStyle = [&](const QString &hex) -> QString {
        QString textColor = getContrastColor(hex);
        return QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: 1px solid #27446d;
                border-radius: 2px;
                padding: 4px 8px;
                font-size: 11px;
                font-weight: 600;
                min-width: 100px;
                text-align: left;
            }
            QPushButton:hover {
                border: 2px solid #0078D4;
            }
        )").arg(hex).arg(textColor);
    };

    QPushButton *colorBtn = new QPushButton(currentColor, this);
    colorBtn->setStyleSheet(makeButtonStyle(currentColor));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(4);
    layout->addWidget(colorBtn);
    layout->addStretch();

    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);

    connect(colorBtn, &QPushButton::clicked, this, [=]() mutable {
        QColor color = QColorDialog::getColor(QColor(currentColor), this, "Select Color");
        if (color.isValid()) {
            QString hex = color.name();
            colorBtn->setText(hex);
            colorBtn->setStyleSheet(makeButtonStyle(hex));

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
