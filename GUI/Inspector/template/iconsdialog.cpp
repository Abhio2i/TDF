

/* ========================================================================= */
/* File: iconsdialog.cpp                                                    */
/* Purpose: Implements image selection dialog                                */
/* ========================================================================= */

#include "iconsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDirIterator>
#include <QFileInfo>
#include <QPixmap>
#include <QDebug>
#include <QToolTip>

IconsDialog::IconsDialog(QWidget *parent)
    : QDialog(parent)
    , inspectorRef(nullptr)  // Initialize inspectorRef to nullptr
{
    setWindowTitle("Select Image");
    setFixedSize(900, 600); // Increased size for 6 images per row

    // Set dark background for dialog and tooltip
    setStyleSheet(
        "QDialog { background-color: #2b2b2b; color: white; }"
        "QToolTip { color: white; background-color: #333; border: 1px solid #555; }"
        );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Create list widget for images
    listWidget = new QListWidget(this);
    listWidget->setViewMode(QListWidget::IconMode);
    listWidget->setIconSize(QSize(100, 100)); // Fixed equal size for all icons
    listWidget->setResizeMode(QListWidget::Adjust);
    listWidget->setMovement(QListWidget::Static);
    listWidget->setGridSize(QSize(120, 120)); // Grid size for 6 items per row
    listWidget->setWordWrap(true); // Enable word wrap for long file names

    // Apply stylesheet for white text and dark background
    listWidget->setStyleSheet(
        "QListWidget {"
        "    background-color: #2b2b2b;"
        "    color: white;"
        "    border: 1px solid #555;"
        "    border-radius: 5px;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    background-color: #333;"
        "    border: 1px solid #555;"
        "    border-radius: 3px;"
        "    margin: 2px;"
        "    padding: 5px;"
        "    color: white;"
        "    text-align: center;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #0078d4;"
        "    border: 1px solid #0078d4;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #444;"
        "    border: 1px solid #666;"
        "}"
        );

    // Automatically load all images from resources
    loadAllImagesAutomatically();

    // If no images found
    if (listWidget->count() == 0) {
        QLabel *noImagesLabel = new QLabel("No images found in resources.", this);
        noImagesLabel->setStyleSheet("QLabel { color: white; background: transparent; font-size: 14px; }");
        noImagesLabel->setAlignment(Qt::AlignCenter);
        listWidget->hide();
        mainLayout->addWidget(noImagesLabel);
    }

    // OK and Cancel buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);

    // Style buttons
    okButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #0078d4;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 3px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #106ebe;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #005a9e;"
        "}"
        );

    cancelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #6d6d6d;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 3px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #5d5d5d;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #4d4d4d;"
        "}"
        );

    buttonLayout->addStretch(); // Add stretch to push buttons to center
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch(); // Add stretch to push buttons to center

    if (listWidget->count() > 0) {
        mainLayout->addWidget(listWidget);
    }
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(listWidget, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
}

QString IconsDialog::selectedImagePath() const
{
    QListWidgetItem *item = listWidget->currentItem();
    if (item)
        return item->data(Qt::UserRole).toString();
    return QString();
}

void IconsDialog::loadAllImagesAutomatically()
{
    qDebug() << "Automatically scanning for all images in resources...";

    // List of all resource prefixes to scan
    QStringList resourcePrefixes = {
        ":/icons",
        ":/texture",
        ":/images",
        ":/resources",
        ":/"
    };

    // Image file extensions
    QStringList imageExtensions = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif"};

    // Scan each resource prefix
    for (const QString &prefix : resourcePrefixes) {
        scanResourcePrefix(prefix, imageExtensions);
    }

    qDebug() << "Total images found:" << listWidget->count();
}

void IconsDialog::scanResourcePrefix(const QString &prefix, const QStringList &extensions)
{
    qDebug() << "Scanning prefix:" << prefix;

    // Use QDirIterator to recursively scan the resource prefix
    QDirIterator it(prefix, extensions, QDir::Files, QDirIterator::Subdirectories);

    int foundCount = 0;
    while (it.hasNext()) {
        QString filePath = it.next();
        QString fileName = it.fileName();

        // Add image to list
        if (addImageToList(filePath, fileName)) {
            foundCount++;
        }
    }

    qDebug() << "Found" << foundCount << "images in" << prefix;
}

bool IconsDialog::addImageToList(const QString &imagePath, const QString &fileName)
{
    // Check for duplicates
    for (int i = 0; i < listWidget->count(); ++i) {
        if (listWidget->item(i)->data(Qt::UserRole).toString() == imagePath) {
            return false;
        }
    }

    // Load and validate image
    QPixmap pixmap(imagePath);
    if (!pixmap.isNull()) {
        // Create scaled pixmap with fixed size for consistency
        QPixmap scaledPixmap = pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QListWidgetItem *item = new QListWidgetItem(
            QIcon(scaledPixmap),
            fileName
            );
        item->setData(Qt::UserRole, imagePath);

        // Set tooltip with white text
        item->setToolTip(QString("<div style='color: white; background-color: #333; padding: 5px;'>"
                                 "<b>%1</b><br>"
                                 "<span style='color: #ccc;'>%2</span>"
                                 "</div>").arg(fileName, imagePath));

        // Set text color to white for the item
        item->setForeground(Qt::white);

        // Set text alignment to center
        item->setTextAlignment(Qt::AlignCenter);

        // Enable word wrap for long file names
        item->setFlags(item->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        listWidget->addItem(item);
        return true;
    }

    return false;
}
