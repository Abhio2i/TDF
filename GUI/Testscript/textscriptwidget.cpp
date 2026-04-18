/* ========================================================================= */
/* File: textscriptwidget.cpp                                             */
/* Purpose: Implements widget for managing and displaying script files      */
//               Written by Arti Rajpoot
/* ========================================================================= */

#include "textscriptwidget.h"                      // For text script widget class
#include "GUI/Testscript/testscriptdialog.h"       // For test script dialog
#include <QVBoxLayout>                             // For vertical layout
#include <QFileInfoList>                           // For file info list
#include <QDir>                                    // For directory handling
#include <QIcon>                                   // For icon handling
#include <QInputDialog>                            // For input dialog
#include <QMessageBox>                             // For message box
#include <QRandomGenerator>                        // For random number generation
#include <QLabel>                                  // For label widget
#include <QFile>                                   // For file operations
#include <QTextStream>                             // For file text streaming
#include <QCoreApplication>                        // For application paths
#include "core/Debug/console.h"                    // For console logging


// %%% TextScriptItemWidget Constructor %%%
/* Initialize item widget for a script file */
TextScriptItemWidget::TextScriptItemWidget(const QString &fileName, const QString &filePath, QWidget *parent)
    : QWidget(parent)
{
    // Set up layout with proper margins
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    // Make widget expand horizontally and have fixed height
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(32);


    QLabel *nameLabel = new QLabel(fileName, this);
    nameLabel->setStyleSheet("background: transparent; color: white;");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(nameLabel);

    // Create play button
    playButton = new QPushButton(this);
    playButton->setIcon(QIcon(":/icons/images/play.png"));
    playButton->setFixedSize(24, 24);
    playButton->setToolTip("Play Script");
    playButton->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   background: transparent;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1A3652;"
        "   border-radius: 3px;"
        "}"
        "QPushButton[active=true] {"
        "   background-color: #2d5a9c;"
        "   border-radius: 3px;"
        "}");
    layout->addWidget(playButton);

    // Connect button signals
    connect(playButton, &QPushButton::clicked, this, [this, filePath]() {
        emit playClicked(filePath);
    });

    // Set initial button state
    setActiveButton("none");
    if (playButton->icon().isNull()) {

    }
}
// %%% TextScriptItemWidget Methods %%%
/* Set active state for play or pause button */
void TextScriptItemWidget::setActiveButton(const QString &state)
{
    playButton->setProperty("active", state == "play");
    playButton->style()->unpolish(playButton);
    playButton->style()->polish(playButton);
}
/* Initialize script management widget */
TextScriptWidget::TextScriptWidget(QWidget *parent)
    : QWidget(parent)
{
    // Set up main layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    // Create a horizontal layout for button to control alignment
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    // Create add script button - WITHOUT fixed width
    addScriptButton = new QPushButton(QIcon(":/icons/images/add.png"), tr("Add Script"), this);
    addScriptButton->setStyleSheet(
        "QPushButton {"
        "   border: 2px solid #27446d;"
        "   background-color: #0F2636;"
        "   padding: 8px 12px;"
        "   color: white;"
        "   font-weight: bold;"
        "   border-radius: 4px;"
        "   text-align: left;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1A3652;"
        "   border: 2px solid #27446d;"
        "}");

    // Make button expand to full width
    addScriptButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    buttonLayout->addWidget(addScriptButton);
    layout->addLayout(buttonLayout);
    fileListWidget = new QListWidget(this);
    fileListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    fileListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    fileListWidget->setUniformItemSizes(true);
    fileListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // StyleSheet for full line selection
    fileListWidget->setStyleSheet(
        "QListWidget {"
        "   background-color: #0F2636;"
        "   border: 2px solid #27446d;"
        "   outline: none;"
        "}"
        "QListWidget::item {"
        "   background-color: transparent;"
        "   border: none;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #2d5a9c;"
        "   border: none;"
        "}"
        "QListWidget::item:selected:!active {"
        "   background-color: #1A3652;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #1A3652;"
        "}"
        "QListWidget::item:selected:hover {"
        "   background-color: #1A3652;"
        "}");

    layout->addWidget(fileListWidget);

    // Connect signals
    connect(fileListWidget, &QListWidget::customContextMenuRequested,
            this, &TextScriptWidget::handleCustomContextMenu);
    connect(addScriptButton, &QPushButton::clicked,
            this, &TextScriptWidget::handleAddScriptButtonClicked);

    // Load script files
    QString projectDir = QCoreApplication::applicationDirPath() + "/../..";
    QString testScriptPath = QDir(projectDir).absoluteFilePath("Testscript");
    loadScriptFiles(testScriptPath);

}
// %%% Script File Management %%%
/* Load script files from directory */
void TextScriptWidget::loadScriptFiles(const QString &directoryPath)
{
    QDir dir(directoryPath);
    if (!dir.exists()) {
        fileListWidget->addItem("Directory not found: " + directoryPath);
        return;
    }

    // Filter for .as files
    dir.setNameFilters(QStringList() << "*.as");
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    QFileInfoList fileList = dir.entryInfoList();

    if (fileList.isEmpty()) {
        fileListWidget->addItem("No .as files found in directory");
        return;
    }

    // Clear existing items and status
    fileListWidget->clear();
    fileStatus.clear();
    activeButtonState.clear();
    for (const QFileInfo &fileInfo : fileList) {
        QString filePath = fileInfo.absoluteFilePath();
        QListWidgetItem *item = new QListWidgetItem(fileListWidget);
        item->setData(Qt::UserRole, filePath);
        item->setSizeHint(QSize(fileListWidget->width(), 32));
        TextScriptItemWidget *itemWidget = new TextScriptItemWidget(
            fileInfo.fileName(), filePath, fileListWidget);
        fileListWidget->setItemWidget(item, itemWidget);
        connect(itemWidget, &TextScriptItemWidget::playClicked,
                this, &TextScriptWidget::handlePlayClicked);
        connect(itemWidget, &TextScriptItemWidget::pauseClicked,
                this, &TextScriptWidget::handlePauseClicked);
        fileStatus[filePath] = "none";
        activeButtonState[filePath] = "none";
        updateStatusIcon(item, "none");
    }
}
/* Update status icon for script item */
void TextScriptWidget::updateStatusIcon(QListWidgetItem *item, const QString &status)
{
    QString iconPath;
    if (status == "success") {
        iconPath = ":/icons/images/success.png";
        item->setIcon(QIcon(iconPath));
        item->setToolTip("Status: Success");
    } else if (status == "warning") {
        iconPath = ":/icons/images/warning.png";
        item->setIcon(QIcon(iconPath));
        item->setToolTip("Status: Warning");
    } else if (status == "error") {
        iconPath = ":/icons/images/error.png";
        item->setIcon(QIcon(iconPath));
        item->setToolTip("Status: Error");
    } else {
        item->setIcon(QIcon());
        item->setToolTip("Status: Not run");
    }

    // Force item to update
    item->setData(Qt::DecorationRole, item->icon());
}
// %%% Context Menu Handlers %%%
/* Handle context menu for script items */
void TextScriptWidget::handleCustomContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = fileListWidget->itemAt(pos);
    if (!item) return;
    QString filePath = item->data(Qt::UserRole).toString();
    // Create context menu
    QMenu contextMenu(this);
    QAction *renameAction = contextMenu.addAction("Rename");
    QAction *removeAction = contextMenu.addAction("Remove");
    QAction *editAction = contextMenu.addAction("Edit");
    // Connect actions
    connect(renameAction, &QAction::triggered, this, &TextScriptWidget::handleRenameAction);
    connect(removeAction, &QAction::triggered, this, &TextScriptWidget::handleRemoveAction);
    connect(editAction, &QAction::triggered, this, &TextScriptWidget::handleEditAction);
    fileListWidget->setProperty("selectedFilePath", filePath);
    contextMenu.exec(fileListWidget->mapToGlobal(pos));
}

/* Handle play button click for script */
void TextScriptWidget::handlePlayClicked(const QString &filePath)
{
    emit runScriptFile(filePath);
    activeButtonState[filePath] = "play";
    for (int i = 0; i < fileListWidget->count(); ++i) {
        QListWidgetItem *item = fileListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == filePath) {
            if (auto *itemWidget =
                qobject_cast<TextScriptItemWidget*>(fileListWidget->itemWidget(item))) {
                itemWidget->setActiveButton("play");
            }
            updateStatusIcon(item, "none");
            break;
        }
    }
}

/* Handle pause button click for script */
void TextScriptWidget::handlePauseClicked(const QString &filePath)
{
    emit pauseScript(filePath);
    activeButtonState[filePath] = "pause";
    fileStatus[filePath] = "none";
    for (int i = 0; i < fileListWidget->count(); ++i) {
        QListWidgetItem *item = fileListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == filePath) {
            TextScriptItemWidget *itemWidget = qobject_cast<TextScriptItemWidget*>(fileListWidget->itemWidget(item));
            if (itemWidget) {
                itemWidget->setActiveButton("pause");
            }
            updateStatusIcon(item, "none");
            break;
        }
    }
}

/* Handle rename action for script */
void TextScriptWidget::handleRenameAction()
{
    QString filePath = fileListWidget->property("selectedFilePath").toString();
    QListWidgetItem *item = fileListWidget->currentItem();
    if (!item) return;
    TextScriptItemWidget *itemWidget = qobject_cast<TextScriptItemWidget*>(fileListWidget->itemWidget(item));
    if (!itemWidget) return;
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename File", "New file name:", QLineEdit::Normal,
                                            QFileInfo(filePath).fileName(), &ok);
    if (ok && !newName.isEmpty()) {
        if (!newName.endsWith(".as")) {
            newName += ".as";
        }
        emit renameScript(filePath, newName);
        // Rename file
        QDir dir(QFileInfo(filePath).absolutePath());
        if (dir.rename(QFileInfo(filePath).fileName(), newName)) {
            QString newFilePath = dir.filePath(newName);
            item->setData(Qt::UserRole, newFilePath);
            itemWidget->findChild<QLabel*>()->setText(newName);
            fileStatus[newFilePath] = fileStatus.take(filePath);
            activeButtonState[newFilePath] = activeButtonState.take(filePath);
        } else {
            QMessageBox::warning(this, "Rename Failed", "Could not rename the file.");
        }
    }
}

/* Handle remove action for script */
void TextScriptWidget::handleRemoveAction()
{
    QString filePath = fileListWidget->property("selectedFilePath").toString();
    QListWidgetItem *item = fileListWidget->currentItem();
    if (!item) return;
    TextScriptItemWidget *itemWidget = qobject_cast<TextScriptItemWidget*>(fileListWidget->itemWidget(item));
    if (!itemWidget) return;
    // Confirm removal
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Remove File",
                                                              "Are you sure you want to remove " +
                                                                  QFileInfo(filePath).fileName() + "?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        emit removeScript(filePath);
        // Remove file
        if (QFile::remove(filePath)) {
            delete item;
            fileStatus.remove(filePath);
            activeButtonState.remove(filePath);
        } else {
            QMessageBox::warning(this, "Remove Failed", "Could not remove the file.");
        }
    }
}

/* Handle add script button click */
void TextScriptWidget::handleAddScriptButtonClicked()
{
    QString projectDir = QCoreApplication::applicationDirPath() + "/../..";
    QString testScriptPath = QDir(projectDir).absoluteFilePath("Testscript");
    TestScriptDialog *window = new TestScriptDialog(this, false);
    connect(window, &TestScriptDialog::runScriptstring, this, &TextScriptWidget::runScriptstring);
    connect(window, &TestScriptDialog::closed, this, [=]() {
        loadScriptFiles(testScriptPath);
        window->deleteLater();
    });
    window->show();
}

/* Handle edit action for script */
void TextScriptWidget::handleEditAction()
{
    QString filePath = fileListWidget->property("selectedFilePath").toString();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Edit Failed", "No file selected for editing.");
        return;
    }
    QString projectDir = QCoreApplication::applicationDirPath() + "/../..";
    QString testScriptPath = QDir(projectDir).absoluteFilePath("Testscript");
    TestScriptDialog *window = new TestScriptDialog(this, true, filePath);
    connect(window, &TestScriptDialog::runScriptstring, this, &TextScriptWidget::runScriptstring);
    connect(window, &TestScriptDialog::closed, this, [=]() {
        loadScriptFiles(testScriptPath);
        window->deleteLater();
    });
    window->show();
}

