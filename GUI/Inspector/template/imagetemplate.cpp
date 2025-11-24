// // // /* ========================================================================= */
// // // /* File: imagetemplate.cpp                                                  */
// // // /* Purpose: Implements image selection widget for inspector table            */
// // // /* ========================================================================= */

// // // #include "imagetemplate.h"                         // For image template class
// // // #include <QVBoxLayout>                             // For vertical layout
// // // #include <QLineEdit>                               // For input field
// // // #include <QPushButton>                             // For browse button
// // // #include <QLabel>                                  // For image preview
// // // #include <QFileDialog>                             // For file dialog
// // // #include <QDebug>                                  // For debug output

// // // // %%% Constructor %%%
// // // /* Initialize image template widget */
// // // ImageTemplate::ImageTemplate(QWidget *parent)
// // //     : QWidget(parent)
// // // {
// // //     // No additional initialization needed
// // // }

// // // // %%% Setup Image Cell %%%
// // // /* Setup image selection cell in table */
// // // void ImageTemplate::setupImageCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
// // // {
// // //     // Get current image path
// // //     QString currentPath = obj["value"].toString();
// // //     // Create main vertical layout
// // //     QVBoxLayout *layout = new QVBoxLayout(this);
// // //     layout->setContentsMargins(0, 0, 0, 0);

// // //     // Create image preview label
// // //     QLabel *imageLabel = new QLabel();
// // //     imageLabel->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
// // //     imageLabel->setScaledContents(true);
// // //     if (!currentPath.isEmpty())
// // //         imageLabel->setPixmap(QPixmap(currentPath).scaled(imageLabel->size(), Qt::KeepAspectRatio));

// // //     // Create path input field
// // //     QLineEdit *lineEdit = new QLineEdit(currentPath);
// // //     lineEdit->setStyleSheet(
// // //         "QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; color: white; }"
// // //         );

// // //     // Create browse button
// // //     QPushButton *browseBtn = new QPushButton("Browse");
// // //     browseBtn->setStyleSheet(
// // //         "QPushButton { background: #333; border: 1px solid #555; border-radius: 3px; color: white; }"
// // //         );

// // //     // Add widgets to layout
// // //     layout->addWidget(imageLabel);
// // //     layout->addWidget(lineEdit);
// // //     layout->addWidget(browseBtn);

// // //     // Connect browse button to file dialog
// // //     connect(browseBtn, &QPushButton::clicked, this, [=]() {
// // //         // Set default directory
// // //         QString defaultDir = "C:/Users/vivek/Desktop/Sensors Simulation Kit/TDF_v0.7.01/images/Texture";
// // //         // Open file dialog
// // //         QString filePath = QFileDialog::getOpenFileName(this, "Select Sprite", defaultDir, "Images (*.png *.jpg *.bmp)");
// // //         if (!filePath.isEmpty()) {
// // //             // Update input and preview
// // //             lineEdit->setText(filePath);
// // //             imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
// // //             // Emit value changed signal
// // //             QJsonObject delta;
// // //             QJsonObject spriteObj;
// // //             spriteObj["value"] = filePath;
// // //             spriteObj["type"] = "image";
// // //             delta[fullKey] = spriteObj;
// // //             emit valueChanged(connectedID, name, delta);
// // //         }
// // //     });

// // //     // Connect line edit to update preview
// // //     connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
// // //         // Update preview with new path
// // //         QString filePath = lineEdit->text();
// // //         imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
// // //         // Emit value changed signal
// // //         QJsonObject delta;
// // //         QJsonObject spriteObj;
// // //         spriteObj["value"] = filePath;
// // //         spriteObj["type"] = "image";
// // //         delta[fullKey] = spriteObj;
// // //         emit valueChanged(connectedID, name, delta);
// // //     });

// // //     // Set row height and add widget to table
// // //     tableWidget->setRowHeight(row, ROW_HEIGHT);
// // //     tableWidget->setCellWidget(row, 1, this);
// // // }




// // /* ========================================================================= */
// // /* File: imagetemplate.cpp                                                  */
// // /* Purpose: Implements image selection widget for inspector table            */
// // /* ========================================================================= */

// // #include "imagetemplate.h"                         // For image template class
// // #include <QVBoxLayout>                             // For vertical layout
// // #include <QLineEdit>                               // For input field
// // #include <QPushButton>                             // For browse button
// // #include <QLabel>                                  // For image preview
// // #include <QFileDialog>                             // For file dialog
// // #include <QDebug>                                  // For debug output
// // #include <QDialog>                                 // For custom dialog
// // #include <QListWidget>                             // For image list
// // #include <QHBoxLayout>                             // For horizontal layout
// // #include <QDir>                                    // For directory handling
// // #include <QResource>                               // For resource handling
// // #include <QDirIterator>                            // For directory iteration

// // // %%% Constructor %%%
// // /* Initialize image template widget */
// // ImageTemplate::ImageTemplate(QWidget *parent)
// //     : QWidget(parent)
// // {
// //     // No additional initialization needed
// // }

// // // %%% Image Selection Dialog Class %%%
// // class ImageSelectionDialog : public QDialog
// // {
// // public:
// //     ImageSelectionDialog(QWidget *parent = nullptr) : QDialog(parent)
// //     {
// //         setWindowTitle("Select Image");
// //         setFixedSize(700, 500);

// //         QVBoxLayout *mainLayout = new QVBoxLayout(this);

// //         // Create list widget for images
// //         listWidget = new QListWidget(this);
// //         listWidget->setViewMode(QListWidget::IconMode);
// //         listWidget->setIconSize(QSize(80, 80));
// //         listWidget->setResizeMode(QListWidget::Adjust);
// //         listWidget->setMovement(QListWidget::Static);

// //         // Automatically load all images from resources
// //         loadAllImagesAutomatically();

// //         // If no images found
// //         if (listWidget->count() == 0) {
// //             QLabel *noImagesLabel = new QLabel("No images found in resources.", this);
// //             noImagesLabel->setAlignment(Qt::AlignCenter);
// //             listWidget->hide();
// //             mainLayout->addWidget(noImagesLabel);
// //         }

// //         // OK and Cancel buttons
// //         QHBoxLayout *buttonLayout = new QHBoxLayout();
// //         QPushButton *okButton = new QPushButton("OK", this);
// //         QPushButton *cancelButton = new QPushButton("Cancel", this);

// //         buttonLayout->addWidget(okButton);
// //         buttonLayout->addWidget(cancelButton);

// //         if (listWidget->count() > 0) {
// //             mainLayout->addWidget(listWidget);
// //         }
// //         mainLayout->addLayout(buttonLayout);

// //         // Connect signals
// //         connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
// //         connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
// //         connect(listWidget, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
// //     }

// //     QString selectedImagePath() const
// //     {
// //         QListWidgetItem *item = listWidget->currentItem();
// //         if (item)
// //             return item->data(Qt::UserRole).toString();
// //         return QString();
// //     }

// // private:
// //     void loadAllImagesAutomatically()
// //     {
// //         qDebug() << "Automatically scanning for all images in resources...";

// //         // List of all resource prefixes to scan
// //         QStringList resourcePrefixes = {
// //             ":/icons",
// //             ":/texture",
// //             ":/images",
// //             ":/resources",
// //             ":/"
// //         };

// //         // Image file extensions
// //         QStringList imageExtensions = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif"};

// //         // Scan each resource prefix
// //         for (const QString &prefix : resourcePrefixes) {
// //             scanResourcePrefix(prefix, imageExtensions);
// //         }

// //         qDebug() << "Total images found:" << listWidget->count();
// //     }

// //     void scanResourcePrefix(const QString &prefix, const QStringList &extensions)
// //     {
// //         qDebug() << "Scanning prefix:" << prefix;

// //         // Use QDirIterator to recursively scan the resource prefix
// //         QDirIterator it(prefix, extensions, QDir::Files, QDirIterator::Subdirectories);

// //         int foundCount = 0;
// //         while (it.hasNext()) {
// //             QString filePath = it.next();
// //             QString fileName = it.fileName();

// //             // Add image to list
// //             if (addImageToList(filePath, fileName)) {
// //                 foundCount++;
// //             }
// //         }

// //         qDebug() << "Found" << foundCount << "images in" << prefix;
// //     }

// //     bool addImageToList(const QString &imagePath, const QString &fileName)
// //     {
// //         // Check for duplicates
// //         for (int i = 0; i < listWidget->count(); ++i) {
// //             if (listWidget->item(i)->data(Qt::UserRole).toString() == imagePath) {
// //                 return false;
// //             }
// //         }

// //         // Load and validate image
// //         QPixmap pixmap(imagePath);
// //         if (!pixmap.isNull()) {
// //             QListWidgetItem *item = new QListWidgetItem(
// //                 QIcon(pixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation)),
// //                 fileName
// //                 );
// //             item->setData(Qt::UserRole, imagePath);
// //             item->setToolTip(fileName + "\n" + imagePath);
// //             listWidget->addItem(item);
// //             return true;
// //         }

// //         return false;
// //     }

// //     QListWidget *listWidget;
// // };

// // // %%% Setup Image Cell %%%
// // /* Setup image selection cell in table */
// // void ImageTemplate::setupImageCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
// // {
// //     // Get current image path
// //     QString currentPath = obj["value"].toString();
// //     // Create main vertical layout
// //     QVBoxLayout *layout = new QVBoxLayout(this);
// //     layout->setContentsMargins(0, 0, 0, 0);

// //     // Create image preview label
// //     QLabel *imageLabel = new QLabel();
// //     imageLabel->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
// //     imageLabel->setScaledContents(true);
// //     imageLabel->setStyleSheet("QLabel { background: #222; border: 1px solid #555; border-radius: 3px; }");

// //     if (!currentPath.isEmpty()) {
// //         QPixmap pixmap(currentPath);
// //         if (!pixmap.isNull()) {
// //             imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
// //         } else {
// //             imageLabel->setText("Invalid Image");
// //             imageLabel->setAlignment(Qt::AlignCenter);
// //         }
// //     } else {
// //         imageLabel->setText("No Image");
// //         imageLabel->setAlignment(Qt::AlignCenter);
// //     }

// //     // Create path input field
// //     QLineEdit *lineEdit = new QLineEdit(currentPath);
// //     lineEdit->setStyleSheet(
// //         "QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; color: white; padding: 2px; }"
// //         );

// //     // Create browse button
// //     QPushButton *browseBtn = new QPushButton("Select Image");
// //     browseBtn->setStyleSheet(
// //         "QPushButton { background: #444; border: 1px solid #666; border-radius: 3px; color: white; padding: 5px; }"
// //         "QPushButton:hover { background: #555; }"
// //         );

// //     // Add widgets to layout
// //     layout->addWidget(imageLabel);
// //     layout->addWidget(lineEdit);
// //     layout->addWidget(browseBtn);

// //     // Connect browse button to custom image selection dialog
// //     connect(browseBtn, &QPushButton::clicked, this, [=]() {
// //         ImageSelectionDialog dialog(this);
// //         if (dialog.exec() == QDialog::Accepted) {
// //             QString selectedPath = dialog.selectedImagePath();
// //             if (!selectedPath.isEmpty()) {
// //                 // Update input and preview
// //                 lineEdit->setText(selectedPath);
// //                 QPixmap pixmap(selectedPath);
// //                 if (!pixmap.isNull()) {
// //                     imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
// //                     imageLabel->setText("");
// //                 }

// //                 // Emit value changed signal
// //                 QJsonObject delta;
// //                 QJsonObject spriteObj;
// //                 spriteObj["value"] = selectedPath;
// //                 spriteObj["type"] = "image";
// //                 delta[fullKey] = spriteObj;
// //                 emit valueChanged(connectedID, name, delta);
// //             }
// //         }
// //     });

// //     // Connect line edit to update preview
// //     connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
// //         // Update preview with new path
// //         QString filePath = lineEdit->text();
// //         if (!filePath.isEmpty()) {
// //             QPixmap pixmap(filePath);
// //             if (!pixmap.isNull()) {
// //                 imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
// //                 imageLabel->setText("");
// //             } else {
// //                 imageLabel->setText("Invalid Image");
// //                 imageLabel->setAlignment(Qt::AlignCenter);
// //             }
// //         } else {
// //             imageLabel->setText("No Image");
// //             imageLabel->setAlignment(Qt::AlignCenter);
// //         }

// //         // Emit value changed signal
// //         QJsonObject delta;
// //         QJsonObject spriteObj;
// //         spriteObj["value"] = filePath;
// //         spriteObj["type"] = "image";
// //         delta[fullKey] = spriteObj;
// //         emit valueChanged(connectedID, name, delta);
// //     });

// //     // Set row height and add widget to table
// //     tableWidget->setRowHeight(row, ROW_HEIGHT);
// //     tableWidget->setCellWidget(row, 1, this);
// // }



// /* ========================================================================= */
// /* File: imagetemplate.cpp                                                  */
// /* Purpose: Implements image selection widget for inspector table            */
// /* ========================================================================= */

// #include "imagetemplate.h"
// #include "iconsdialog.h"  // Include the new dialog
// #include <QVBoxLayout>
// #include <QLineEdit>
// #include <QPushButton>
// #include <QLabel>
// #include <QFileDialog>
// #include <QDebug>

// // %%% Constructor %%%
// /* Initialize image template widget */
// ImageTemplate::ImageTemplate(QWidget *parent)
//     : QWidget(parent)
// {
//     // No additional initialization needed
// }

// // %%% Setup Image Cell %%%
// /* Setup image selection cell in table */
// void ImageTemplate::setupImageCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
// {
//     // Get current image path
//     QString currentPath = obj["value"].toString();
//     // Create main vertical layout
//     QVBoxLayout *layout = new QVBoxLayout(this);
//     layout->setContentsMargins(0, 0, 0, 0);

//     // Create image preview label
//     QLabel *imageLabel = new QLabel();
//     imageLabel->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
//     imageLabel->setScaledContents(true);
//     imageLabel->setStyleSheet("QLabel { background: #222; border: 1px solid #555; border-radius: 3px; }");

//     if (!currentPath.isEmpty()) {
//         QPixmap pixmap(currentPath);
//         if (!pixmap.isNull()) {
//             imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
//         } else {
//             imageLabel->setText("Invalid Image");
//             imageLabel->setAlignment(Qt::AlignCenter);
//         }
//     } else {
//         imageLabel->setText("No Image");
//         imageLabel->setAlignment(Qt::AlignCenter);
//     }

//     // Create path input field
//     QLineEdit *lineEdit = new QLineEdit(currentPath);
//     lineEdit->setStyleSheet(
//         "QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; color: white; padding: 2px; }"
//         );

//     // Create browse button
//     QPushButton *browseBtn = new QPushButton("Select Image");
//     browseBtn->setStyleSheet(
//         "QPushButton { background: #444; border: 1px solid #666; border-radius: 3px; color: white; padding: 5px; }"
//         "QPushButton:hover { background: #555; }"
//         );

//     // Add widgets to layout
//     layout->addWidget(imageLabel);
//     layout->addWidget(lineEdit);
//     layout->addWidget(browseBtn);

//     // Connect browse button to custom image selection dialog
//     connect(browseBtn, &QPushButton::clicked, this, [=]() {
//         IconsDialog dialog(this);  // Use the separate dialog class
//         if (dialog.exec() == QDialog::Accepted) {
//             QString selectedPath = dialog.selectedImagePath();
//             if (!selectedPath.isEmpty()) {
//                 // Update input and preview
//                 lineEdit->setText(selectedPath);
//                 QPixmap pixmap(selectedPath);
//                 if (!pixmap.isNull()) {
//                     imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
//                     imageLabel->setText("");
//                 }

//                 // Emit value changed signal
//                 QJsonObject delta;
//                 QJsonObject spriteObj;
//                 spriteObj["value"] = selectedPath;
//                 spriteObj["type"] = "image";
//                 delta[fullKey] = spriteObj;
//                 emit valueChanged(connectedID, name, delta);
//             }
//         }
//     });

//     // Connect line edit to update preview
//     connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
//         // Update preview with new path
//         QString filePath = lineEdit->text();
//         if (!filePath.isEmpty()) {
//             QPixmap pixmap(filePath);
//             if (!pixmap.isNull()) {
//                 imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
//                 imageLabel->setText("");
//             } else {
//                 imageLabel->setText("Invalid Image");
//                 imageLabel->setAlignment(Qt::AlignCenter);
//             }
//         } else {
//             imageLabel->setText("No Image");
//             imageLabel->setAlignment(Qt::AlignCenter);
//         }

//         // Emit value changed signal
//         QJsonObject delta;
//         QJsonObject spriteObj;
//         spriteObj["value"] = filePath;
//         spriteObj["type"] = "image";
//         delta[fullKey] = spriteObj;
//         emit valueChanged(connectedID, name, delta);
//     });

//     // Set row height and add widget to table
//     tableWidget->setRowHeight(row, ROW_HEIGHT);
//     tableWidget->setCellWidget(row, 1, this);
// }
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
        emit valueChanged(connectedID, name, delta);
    });

    // Set row height and add widget to table
    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}
