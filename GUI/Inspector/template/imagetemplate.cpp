
#include "imagetemplate.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDebug>

ImageTemplate::ImageTemplate(QWidget *parent) : QWidget(parent) {}

void ImageTemplate::setupImageCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QString currentPath = obj["value"].toString();
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *imageLabel = new QLabel();
    imageLabel->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
    imageLabel->setScaledContents(true);
    if (!currentPath.isEmpty())
        imageLabel->setPixmap(QPixmap(currentPath).scaled(imageLabel->size(), Qt::KeepAspectRatio));

    QLineEdit *lineEdit = new QLineEdit(currentPath);
    lineEdit->setStyleSheet(
        "QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; color: white; }"
        );

    QPushButton *browseBtn = new QPushButton("Browse");
    browseBtn->setStyleSheet(
        "QPushButton { background: #333; border: 1px solid #555; border-radius: 3px; color: white; }"
        );

    layout->addWidget(imageLabel);
    layout->addWidget(lineEdit);
    layout->addWidget(browseBtn);

    connect(browseBtn, &QPushButton::clicked, this, [=]() {
        QString defaultDir = "C:/Users/vivek/Desktop/Sensors Simulation Kit/TDF_v0.7.01/images/Texture";
        QString filePath = QFileDialog::getOpenFileName(this, "Select Sprite", defaultDir, "Images (*.png *.jpg *.bmp)");
        if (!filePath.isEmpty()) {
            lineEdit->setText(filePath);
            imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
            QJsonObject delta;
            QJsonObject spriteObj;
            spriteObj["value"] = filePath;
            spriteObj["type"] = "image";
            delta[fullKey] = spriteObj;

            emit valueChanged(connectedID, name, delta);
        }
    });

    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        QString filePath = lineEdit->text();
        imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
        QJsonObject delta;
        QJsonObject spriteObj;
        spriteObj["value"] = filePath;
        spriteObj["type"] = "image";
        delta[fullKey] = spriteObj;

        emit valueChanged(connectedID, name, delta);
    });

    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}
