/* ========================================================================= */
/* File: textscriptwidget.cpp                                             */
/* Purpose: Implements widget for managing and displaying script files      */
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
    // Set up layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);
    // Add file name label
    QLabel *nameLabel = new QLabel(fileName, this);
    layout->addWidget(nameLabel);
    layout->addStretch();
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
        "QPushButton[active=true] {"
        "   background-color: #505050;"
        "   border: 1px solid #707070;"
        "}");
    layout->addWidget(playButton);
    // Create pause button
    pauseButton = new QPushButton(this);
    pauseButton->setIcon(QIcon(":/icons/images/pause.png"));
    pauseButton->setFixedSize(24, 24);
    pauseButton->setToolTip("Pause Script");
    pauseButton->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   background: transparent;"
        "}"
        "QPushButton[active=true] {"
        "   background-color: #505050;"
        "   border: 1px solid #707070;"
        "}");
    layout->addWidget(pauseButton);
    // Connect button signals
    connect(playButton, &QPushButton::clicked, this, [this, filePath]() {
        emit playClicked(filePath);
    });
    connect(pauseButton, &QPushButton::clicked, this, [this, filePath]() {
        emit pauseClicked(filePath);
    });
    // Set initial button state
    setActiveButton("none");
    // Verify icons
    if (playButton->icon().isNull()) {
        Console::error("Failed to load play icon from :/icons/images/play.png");
    } else {
        Console::log("Play icon loaded successfully for :/icons/images/play.png");
    }
    if (pauseButton->icon().isNull()) {
        Console::error("Failed to load pause icon from :/icons/images/pause.png");
    } else {
        Console::log("Pause icon loaded successfully for :/icons/images/pause.png");
    }
}

// %%% TextScriptItemWidget Methods %%%
/* Set active state for play or pause button */
void TextScriptItemWidget::setActiveButton(const QString &state)
{
    playButton->setProperty("active", state == "play");
    pauseButton->setProperty("active", state == "pause");
    playButton->style()->unpolish(playButton);
    playButton->style()->polish(playButton);
    pauseButton->style()->unpolish(pauseButton);
    pauseButton->style()->polish(pauseButton);
}

// %%% TextScriptWidget Constructor %%%
/* Initialize script management widget */
TextScriptWidget::TextScriptWidget(QWidget *parent)
    : QWidget(parent)
{
    // Set up main layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);
    // Create add script button
    addScriptButton = new QPushButton(QIcon(":/icons/images/add.png"), tr("Add Script"), this);
    addScriptButton->setFixedWidth(100);
    addScriptButton->setStyleSheet(
        "QPushButton {"
        "   border: 1px solid #707070;"
        "   background-color: #404040;"
        "   padding: 5px;"
        "   color: white;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #505050;"
        "   color: white;"
        "   font-weight: bold;"
        "}");
    layout->addWidget(addScriptButton);
    // Create file list widget
    fileListWidget = new QListWidget(this);
    fileListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(fileListWidget);
    // Connect signals
    connect(fileListWidget, &QListWidget::customContextMenuRequested, this, &TextScriptWidget::handleCustomContextMenu);
    connect(addScriptButton, &QPushButton::clicked, this, &TextScriptWidget::handleAddScriptButtonClicked);
    // Verify add button icon
    if (addScriptButton->icon().isNull()) {
        Console::error("Failed to load add icon from :/icons/images/add.png");
    } else {
        Console::log("Add icon loaded successfully for :/icons/images/add.png");
    }
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
        Console::error("Directory not found: " + directoryPath.toStdString());
        return;
    }
    // Filter for .as files
    dir.setNameFilters(QStringList() << "*.as");
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    QFileInfoList fileList = dir.entryInfoList();
    if (fileList.isEmpty()) {
        fileListWidget->addItem("No .as files found in directory");
        Console::log("No .as files found in directory: " + directoryPath.toStdString());
        return;
    }
    // Clear existing items and status
    fileListWidget->clear();
    fileStatus.clear();
    activeButtonState.clear();
    // Add script items
    for (const QFileInfo &fileInfo : fileList) {
        QString filePath = fileInfo.absoluteFilePath();
        QListWidgetItem *item = new QListWidgetItem(fileListWidget);
        item->setData(Qt::UserRole, filePath);
        TextScriptItemWidget *itemWidget = new TextScriptItemWidget(fileInfo.fileName(), filePath, fileListWidget);
        item->setSizeHint(itemWidget->sizeHint());
        fileListWidget->setItemWidget(item, itemWidget);
        connect(itemWidget, &TextScriptItemWidget::playClicked, this, &TextScriptWidget::handlePlayClicked);
        connect(itemWidget, &TextScriptItemWidget::pauseClicked, this, &TextScriptWidget::handlePauseClicked);
        fileStatus[filePath] = "none";
        activeButtonState[filePath] = "none";
        updateStatusIcon(item, "none");
    }
    Console::log("Loaded " + std::to_string(fileList.size()) + " .as files from directory: " + directoryPath.toStdString());
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
        iconPath = "";
        item->setIcon(QIcon());
        item->setToolTip("Status: Not run");
    }
    if (!item->icon().isNull()) {
        Console::log("Status icon set to " + status.toStdString() + " for item using " + iconPath.toStdString());
    } else if (status != "none") {
        Console::error("Failed to load status icon for status: " + status.toStdString() + " at " + iconPath.toStdString());
    }
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
    emit runScript(filePath);
    activeButtonState[filePath] = "play";
    // Simulate execution result
    QString status;
    int rand = QRandomGenerator::global()->bounded(3);
    switch (rand) {
    case 0: status = "success"; break;
    case 1: status = "warning"; break;
    case 2: status = "error"; break;
    default: status = "success"; break;
    }
    fileStatus[filePath] = status;
    // Update item status
    for (int i = 0; i < fileListWidget->count(); ++i) {
        QListWidgetItem *item = fileListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == filePath) {
            TextScriptItemWidget *itemWidget = qobject_cast<TextScriptItemWidget*>(fileListWidget->itemWidget(item));
            if (itemWidget) {
                itemWidget->setActiveButton("play");
            }
            updateStatusIcon(item, status);
            break;
        }
    }
    Console::log("Play clicked for script: " + filePath.toStdString() + ", status set to: " + status.toStdString());
}

/* Handle pause button click for script */
void TextScriptWidget::handlePauseClicked(const QString &filePath)
{
    emit pauseScript(filePath);
    activeButtonState[filePath] = "pause";
    fileStatus[filePath] = "none";
    // Update item status
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
    Console::log("Pause clicked for script: " + filePath.toStdString() + ", status set to none");
}

/* Handle rename action for script */
void TextScriptWidget::handleRenameAction()
{
    QString filePath = fileListWidget->property("selectedFilePath").toString();
    QListWidgetItem *item = fileListWidget->currentItem();
    if (!item) return;
    TextScriptItemWidget *itemWidget = qobject_cast<TextScriptItemWidget*>(fileListWidget->itemWidget(item));
    if (!itemWidget) return;
    // Prompt for new name
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
            Console::log("Renamed script from " + filePath.toStdString() + " to " + newFilePath.toStdString());
        } else {
            QMessageBox::warning(this, "Rename Failed", "Could not rename the file.");
            Console::error("Failed to rename script: " + filePath.toStdString());
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
            Console::log("Removed script: " + filePath.toStdString());
        } else {
            QMessageBox::warning(this, "Remove Failed", "Could not remove the file.");
            Console::error("Failed to remove script: " + filePath.toStdString());
        }
    }
}

/* Handle add script button click */
void TextScriptWidget::handleAddScriptButtonClicked()
{
    QString projectDir = QCoreApplication::applicationDirPath() + "/../..";
    QString testScriptPath = QDir(projectDir).absoluteFilePath("Testscript");
    // Open add script dialog
    TestScriptDialog *window = new TestScriptDialog(this, false);
    connect(window, &TestScriptDialog::runScriptstring, this, &TextScriptWidget::runScriptstring);
    connect(window, &TestScriptDialog::closed, this, [=]() {
        loadScriptFiles(testScriptPath);
        Console::log("New script added or canceled, reloading script files from: " + testScriptPath.toStdString());
        window->deleteLater();
    });
    window->show();
}

/* Handle edit action for script */
void TextScriptWidget::handleEditAction()
{
    QString filePath = fileListWidget->property("selectedFilePath").toString();
    if (filePath.isEmpty()) {
        Console::error("No file selected for editing");
        QMessageBox::warning(this, "Edit Failed", "No file selected for editing.");
        return;
    }
    QString projectDir = QCoreApplication::applicationDirPath() + "/../..";
    QString testScriptPath = QDir(projectDir).absoluteFilePath("Testscript");
    // Open edit script dialog
    TestScriptDialog *window = new TestScriptDialog(this, true, filePath);
    connect(window, &TestScriptDialog::runScriptstring, this, &TextScriptWidget::runScriptstring);
    connect(window, &TestScriptDialog::closed, this, [=]() {
        loadScriptFiles(testScriptPath);
        Console::log("Script edited and saved or canceled, reloading script files: " + filePath.toStdString());
        window->deleteLater();
    });
    window->show();
}
