/* ========================================================================= */
/* File: imagetemplate.cpp                                                   */
/* Purpose: Implements image selection widget for inspector table cells      */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */
#include "imagetemplate.h"
#include "iconsdialog.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDebug>
#include "GUI/Inspector/inspector.h"
#include "GUI/Inspector/inspector-styles.h"

ImageTemplate::ImageTemplate(Inspector *inspector, QWidget *parent)
    : QWidget(parent), inspectorRef(inspector) {}

void ImageTemplate::setupImageCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QString currentPath = obj["value"].toString();
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    QLabel *imageLabel = new QLabel();
    imageLabel->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
    imageLabel->setScaledContents(true);
    imageLabel->setStyleSheet(InspectorStyles::ImagePreviewLabel);
    if (!currentPath.isEmpty()) {
        QPixmap pixmap(currentPath);
        if (!pixmap.isNull()) {
            imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            imageLabel->setText("Invalid Image");
            imageLabel->setAlignment(Qt::AlignCenter);
        }
    } else {
        imageLabel->setText("No Image");
        imageLabel->setAlignment(Qt::AlignCenter);
    }
    QLineEdit *lineEdit = new QLineEdit(currentPath);
    lineEdit->setStyleSheet(InspectorStyles::ImageLineEdit);
    QPushButton *browseBtn = new QPushButton("Select Image");
    browseBtn->setStyleSheet(InspectorStyles::ImageBrowseButton);
    layout->addWidget(imageLabel);
    layout->addWidget(lineEdit);
    layout->addWidget(browseBtn);
    connect(browseBtn, &QPushButton::clicked, this, [=]() {
        IconsDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString selectedPath = dialog.selectedImagePath();
            if (!selectedPath.isEmpty()) {
                lineEdit->setText(selectedPath);
                QPixmap pixmap(selectedPath);
                if (!pixmap.isNull()) {
                    imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    imageLabel->setText("");
                }
                QJsonObject delta;
                QJsonObject spriteObj;
                spriteObj["value"] = selectedPath;
                spriteObj["type"] = "image";
                delta[fullKey] = spriteObj;
                if (inspectorRef) {
                    delta["_id"] = inspectorRef->getMainID();
                }
                emit valueChanged(connectedID, name, delta);
            }
        }
    });

    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        QString filePath = lineEdit->text();
        if (!filePath.isEmpty()) {
            QPixmap pixmap(filePath);
            if (!pixmap.isNull()) {
                imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                imageLabel->setText("");
            } else {
                imageLabel->setText("Invalid Image");
                imageLabel->setAlignment(Qt::AlignCenter);
            }
        } else {
            imageLabel->setText("No Image");
            imageLabel->setAlignment(Qt::AlignCenter);
        }
        QJsonObject delta;
        QJsonObject spriteObj;
        spriteObj["value"] = filePath;
        spriteObj["type"] = "image";
        delta[fullKey] = spriteObj;
        if (inspectorRef) {
            delta["_id"] = inspectorRef->getMainID();
        }
        emit valueChanged(connectedID, name, delta);
    });
    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}
