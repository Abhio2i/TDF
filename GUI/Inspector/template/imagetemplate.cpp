/* ========================================================================= */
/* File: imagetemplate.cpp                                                  */
/* Purpose: Implements image selection widget for inspector table            */
/* ========================================================================= */

#include "imagetemplate.h"                         // For image template class
#include <QVBoxLayout>                             // For vertical layout
#include <QLineEdit>                               // For input field
#include <QPushButton>                             // For browse button
#include <QLabel>                                  // For image preview
#include <QFileDialog>                             // For file dialog
#include <QDebug>                                  // For debug output

// %%% Constructor %%%
/* Initialize image template widget */
ImageTemplate::ImageTemplate(QWidget *parent)
    : QWidget(parent)
{
    // No additional initialization needed
}

// %%% Setup Image Cell %%%
/* Setup image selection cell in table */
void ImageTemplate::setupImageCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    // Get current image path
    QString currentPath = obj["value"].toString();
    // Create main vertical layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create image preview label
    QLabel *imageLabel = new QLabel();
    imageLabel->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
    imageLabel->setScaledContents(true);
    if (!currentPath.isEmpty())
        imageLabel->setPixmap(QPixmap(currentPath).scaled(imageLabel->size(), Qt::KeepAspectRatio));

    // Create path input field
    QLineEdit *lineEdit = new QLineEdit(currentPath);
    lineEdit->setStyleSheet(
        "QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; color: white; }"
        );

    // Create browse button
    QPushButton *browseBtn = new QPushButton("Browse");
    browseBtn->setStyleSheet(
        "QPushButton { background: #333; border: 1px solid #555; border-radius: 3px; color: white; }"
        );

    // Add widgets to layout
    layout->addWidget(imageLabel);
    layout->addWidget(lineEdit);
    layout->addWidget(browseBtn);

    // Connect browse button to file dialog
    connect(browseBtn, &QPushButton::clicked, this, [=]() {
        // Set default directory
        QString defaultDir = "C:/Users/vivek/Desktop/Sensors Simulation Kit/TDF_v0.7.01/images/Texture";
        // Open file dialog
        QString filePath = QFileDialog::getOpenFileName(this, "Select Sprite", defaultDir, "Images (*.png *.jpg *.bmp)");
        if (!filePath.isEmpty()) {
            // Update input and preview
            lineEdit->setText(filePath);
            imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
            // Emit value changed signal
            QJsonObject delta;
            QJsonObject spriteObj;
            spriteObj["value"] = filePath;
            spriteObj["type"] = "image";
            delta[fullKey] = spriteObj;
            emit valueChanged(connectedID, name, delta);
        }
    });

    // Connect line edit to update preview
    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        // Update preview with new path
        QString filePath = lineEdit->text();
        imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
        // Emit value changed signal
        QJsonObject delta;
        QJsonObject spriteObj;
        spriteObj["value"] = filePath;
        spriteObj["type"] = "image";
        delta[fullKey] = spriteObj;
        emit valueChanged(connectedID, name, delta);
    });

    // Set row height and add widget to table
    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}
