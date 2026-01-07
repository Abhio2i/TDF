
/* ========================================================================= */
/* File: imagetemplate.cpp                                                  */
/* Purpose: Implements image selection widget for inspector table            */
/* ========================================================================= */

#include "imagetemplate.h"
#include "iconsdialog.h"  // Include the new dialog
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDebug>
#include "GUI/Inspector/inspector.h"  // Include inspector for mainID access

// %%% Constructor %%%
/* Initialize image template widget */
ImageTemplate::ImageTemplate(Inspector *inspector, QWidget *parent)
    : QWidget(parent), inspectorRef(inspector)
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
    imageLabel->setStyleSheet("QLabel { background: #f8f9fa; border: 1px solid #ccc; border-radius: 3px; color: black; }");

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

    // Create path input field
    QLineEdit *lineEdit = new QLineEdit(currentPath);
    lineEdit->setStyleSheet(
        "QLineEdit { background: white; border: 1px solid #ccc; border-radius: 3px; color: black; padding: 2px; }"
        "QLineEdit:focus { border: 1px solid #007bff; }"
        );

    // Create browse button
    QPushButton *browseBtn = new QPushButton("Select Image");
    browseBtn->setStyleSheet(
        "QPushButton { background: #e9ecef; border: 1px solid #ccc; border-radius: 3px; color: black; padding: 5px; }"
        "QPushButton:hover { background: #dde1e4; }"
        "QPushButton:pressed { background: #ced4da; }"
        );

    // Add widgets to layout
    layout->addWidget(imageLabel);
    layout->addWidget(lineEdit);
    layout->addWidget(browseBtn);

    // Connect browse button to custom image selection dialog
    connect(browseBtn, &QPushButton::clicked, this, [=]() {
        IconsDialog dialog(this);  // Use the separate dialog class
        if (dialog.exec() == QDialog::Accepted) {
            QString selectedPath = dialog.selectedImagePath();
            if (!selectedPath.isEmpty()) {
                // Update input and preview
                lineEdit->setText(selectedPath);
                QPixmap pixmap(selectedPath);
                if (!pixmap.isNull()) {
                    imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    imageLabel->setText("");
                }

                // Emit value changed signal
                QJsonObject delta;
                QJsonObject spriteObj;
                spriteObj["value"] = selectedPath;
                spriteObj["type"] = "image";
                delta[fullKey] = spriteObj;

                // Add mainID from inspector
                if (inspectorRef) {
                    delta["_id"] = inspectorRef->getMainID();
                }

                emit valueChanged(connectedID, name, delta);
            }
        }
    });

    // Connect line edit to update preview
    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        // Update preview with new path
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

        // Emit value changed signal
        QJsonObject delta;
        QJsonObject spriteObj;
        spriteObj["value"] = filePath;
        spriteObj["type"] = "image";
        delta[fullKey] = spriteObj;

        // Add mainID from inspector
        if (inspectorRef) {
            delta["_id"] = inspectorRef->getMainID();
        }

        emit valueChanged(connectedID, name, delta);
    });

    // Set row height and add widget to table
    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}
