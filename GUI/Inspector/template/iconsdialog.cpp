/* ========================================================================= */
/* File: iconsdialog.cpp                                                    */
/* Purpose: Implements image selection dialog                                */
//               Written by Arti Rajpoot
/* ========================================================================= */

#include "iconsdialog.h"
#include "qlineedit.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDirIterator>
#include <QFileInfo>
#include <QPixmap>
#include <QDebug>
#include <QToolTip>
#include <QFileDialog>
#include "GUI/Inspector/inspector-styles.h"

IconsDialog::IconsDialog(QWidget *parent)
    : QDialog(parent)
    , inspectorRef(nullptr)
    , searchBox(nullptr)
    , listWidget(nullptr)
{
    setWindowTitle("Select Image");
    setFixedSize(900, 600);
    setStyleSheet(InspectorStyles::IconsDialog_main);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ========== SEARCH BAR WITH BROWSE BUTTON ==========
    QHBoxLayout *searchLayout = new QHBoxLayout();

    QPushButton *browseBtn = new QPushButton("Browse", this);
    browseBtn->setStyleSheet(InspectorStyles::IconsDialog_browseButton);

    QLabel *searchLabel = new QLabel("Search:", this);
    searchLabel->setStyleSheet(InspectorStyles::IconsDialog_searchLabel);

    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText("Type image name to search...");
    searchBox->setStyleSheet(InspectorStyles::IconsDialog_searchBox);

    searchLayout->addWidget(browseBtn);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(searchBox, 1);
    mainLayout->addLayout(searchLayout);

    listWidget = new QListWidget(this);
    listWidget->setViewMode(QListWidget::IconMode);
    listWidget->setIconSize(QSize(100, 100));
    listWidget->setResizeMode(QListWidget::Adjust);
    listWidget->setMovement(QListWidget::Static);
    listWidget->setGridSize(QSize(120, 120));
    listWidget->setWordWrap(true);
    listWidget->setStyleSheet(InspectorStyles::IconsDialog_listWidget);
    loadAllImagesAutomatically();
    if (allImages.isEmpty()) {
        QLabel *noImagesLabel = new QLabel("No images found in resources.", this);
        noImagesLabel->setStyleSheet(InspectorStyles::IconsDialog_noImagesLabel);
        noImagesLabel->setAlignment(Qt::AlignCenter);
        listWidget->hide();
        mainLayout->addWidget(noImagesLabel);
    }
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);

    okButton->setStyleSheet(InspectorStyles::IconsDialog_okButton);
    cancelButton->setStyleSheet(InspectorStyles::IconsDialog_cancelButton);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();

    if (!allImages.isEmpty()) {
        mainLayout->addWidget(listWidget);
    }
    mainLayout->addLayout(buttonLayout);

    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Select Image File",
            "",
            "Images (*.png *.jpg *.jpeg *.bmp *.gif *.svg);;All Files (*)"
            );
        if (!filePath.isEmpty()) {
            m_selectedPath = filePath;
            accept();
        }
    });
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(listWidget, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
    connect(searchBox, &QLineEdit::textChanged, this, &IconsDialog::onSearchTextChanged);
}

QString IconsDialog::selectedImagePath() const
{
    if (!m_selectedPath.isEmpty()) {
        return m_selectedPath;
    }

    QListWidgetItem *item = listWidget->currentItem();
    if (item)
        return item->data(Qt::UserRole).toString();
    return QString();
}

void IconsDialog::loadAllImagesAutomatically()
{
    qDebug() << "Automatically scanning for all images in resources...";

    // Clear existing images
    allImages.clear();

    // List of all resource prefixes to scan
    QStringList resourcePrefixes = {
        ":/icons",
        ":/texture",
        ":/images",
        ":/resources",
        ":/air",
        ":/ground",
        ":/"
    };

    // Image file extensions
    QStringList imageExtensions = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.svg"};

    // Scan each resource prefix
    for (const QString &prefix : resourcePrefixes) {
        scanResourcePrefix(prefix, imageExtensions);
    }
    // Display all images initially
    filterImages("");
}

void IconsDialog::scanResourcePrefix(const QString &prefix, const QStringList &extensions)
{
    // Use QDirIterator to recursively scan the resource prefix
    QDirIterator it(prefix, extensions, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();
        QString fileName = it.fileName();

        // Check for duplicates
        bool isDuplicate = false;
        for (const auto &pair : allImages) {
            if (pair.second == filePath) {
                isDuplicate = true;
                break;
            }
        }
        if (!isDuplicate) {
            // Validate image
            QPixmap pixmap(filePath);
            if (!pixmap.isNull()) {
                allImages.append(qMakePair(fileName, filePath));
            }
        }
    }
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
void IconsDialog::filterImages(const QString &searchText)
{
    if (!listWidget) return;
    listWidget->clear();
    for (const auto &imagePair : allImages) {
        QString fileName = imagePair.first;
        QString filePath = imagePair.second;

        // Filter by search text (case-insensitive)
        if (searchText.isEmpty() || fileName.contains(searchText, Qt::CaseInsensitive)) {
            addImageToList(filePath, fileName);
        }
    }

}

void IconsDialog::onSearchTextChanged(const QString &text)
{
    filterImages(text);
}

void IconsDialog::setSearchFilter(const QString &filter)
{
    if (searchBox) {
        searchBox->setText(filter);
        filterImages(filter);
    }
}
