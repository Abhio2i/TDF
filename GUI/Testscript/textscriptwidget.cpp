
#include "textscriptwidget.h"
#include "GUI/Testscript/testscriptdialog.h"
#include <QVBoxLayout>
#include <QFileInfoList>
#include <QDir>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include "core/Debug/console.h"

TextScriptItemWidget::TextScriptItemWidget(const QString &fileName, const QString &filePath, QWidget *parent)
    : QWidget(parent) {
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    QLabel *nameLabel = new QLabel(fileName, this);
    layout->addWidget(nameLabel);
    layout->addStretch();

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

    pauseButton = new QPushButton(this);
    pauseButton->setIcon(QIcon(":/icons/images/pause.png"));
    pauseButton->setFixedSize(24, 24);
    pauseButton->setToolTip("Pause Script");
    playButton->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   background: transparent;"
        "}"
        "QPushButton[active=true] {"
        "   background-color: #505050;"
        "   border: 1px solid #707070;"
        "}");
    layout->addWidget(pauseButton);

    connect(playButton, &QPushButton::clicked, this, [this, filePath]() {
        emit playClicked(filePath);
    });
    connect(pauseButton, &QPushButton::clicked, this, [this, filePath]() {
        emit pauseClicked(filePath);
    });

    setActiveButton("none");

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

void TextScriptItemWidget::setActiveButton(const QString &state) {
    playButton->setProperty("active", state == "play");
    pauseButton->setProperty("active", state == "pause");
    playButton->style()->unpolish(playButton);
    playButton->style()->polish(playButton);
    pauseButton->style()->unpolish(pauseButton);
    pauseButton->style()->polish(pauseButton);
}

TextScriptWidget::TextScriptWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

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

    fileListWidget = new QListWidget(this);
    fileListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(fileListWidget);

    connect(fileListWidget, &QListWidget::customContextMenuRequested, this, &TextScriptWidget::handleCustomContextMenu);
    connect(addScriptButton, &QPushButton::clicked, this, &TextScriptWidget::handleAddScriptButtonClicked);

    if (addScriptButton->icon().isNull()) {
        Console::error("Failed to load add icon from :/icons/images/add.png");
    } else {
        Console::log("Add icon loaded successfully for :/icons/images/add.png");
    }

    loadScriptFiles("/home/arti_rajpoot/Downloads/TDF_with coordinate");
}

void TextScriptWidget::loadScriptFiles(const QString &directoryPath) {
    QDir dir(directoryPath);
    if (!dir.exists()) {
        fileListWidget->addItem("Directory not found: " + directoryPath);
        Console::error("Directory not found: " + directoryPath.toStdString());
        return;
    }

    dir.setNameFilters(QStringList() << "*.as");
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QFileInfoList fileList = dir.entryInfoList();
    if (fileList.isEmpty()) {
        fileListWidget->addItem("No .as files found in directory");
        Console::log("No .as files found in directory: " + directoryPath.toStdString());
        return;
    }

    fileListWidget->clear();
    fileStatus.clear();
    activeButtonState.clear();

    for (const QFileInfo &fileInfo : fileList) {
        QString filePath = fileInfo.absoluteFilePath();
        QListWidgetItem *item = new QListWidgetItem(fileListWidget);
        item->setData(Qt::UserRole, filePath);

        TextScriptItemWidget *itemWidget = new TextScriptItemWidget(fileInfo.fileName(), filePath, fileListWidget);
        item->setSizeHint(itemWidget->sizeHint());
        fileListWidget->setItemWidget(item, itemWidget);

        connect(itemWidget, &TextScriptItemWidget::playClicked, this, &TextScriptWidget::handlePlayClicked);
        connect(itemWidget, &TextScriptItemWidget::pauseClicked, this, &TextScriptWidget::handlePauseClicked);

        fileStatus[filePath] = "none"; // No status icon initially
        activeButtonState[filePath] = "none";
        updateStatusIcon(item, "none");
    }
    Console::log("Loaded " + std::to_string(fileList.size()) + " .as files from directory: " + directoryPath.toStdString());
}

void TextScriptWidget::updateStatusIcon(QListWidgetItem *item, const QString &status) {
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

void TextScriptWidget::handleCustomContextMenu(const QPoint &pos) {
    QListWidgetItem *item = fileListWidget->itemAt(pos);
    if (!item) return;

    QString filePath = item->data(Qt::UserRole).toString();

    QMenu contextMenu(this);
    QAction *renameAction = contextMenu.addAction("Rename");
    QAction *removeAction = contextMenu.addAction("Remove");
    QAction *editAction = contextMenu.addAction("Edit");

    connect(renameAction, &QAction::triggered, this, &TextScriptWidget::handleRenameAction);
    connect(removeAction, &QAction::triggered, this, &TextScriptWidget::handleRemoveAction);
    connect(editAction, &QAction::triggered, this, &TextScriptWidget::handleEditAction);

    fileListWidget->setProperty("selectedFilePath", filePath);

    contextMenu.exec(fileListWidget->mapToGlobal(pos));
}

void TextScriptWidget::handlePlayClicked(const QString &filePath) {
    emit runScript(filePath);

    activeButtonState[filePath] = "play";
    // Simulate execution result (replace with actual logic based on script type or execution)
    QString status;
    int rand = QRandomGenerator::global()->bounded(3); // Temporary: simulate result
    switch (rand) {
    case 0: status = "success"; break;
    case 1: status = "warning"; break;
    case 2: status = "error"; break;
    default: status = "success"; break;
    }
    fileStatus[filePath] = status;

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

void TextScriptWidget::handlePauseClicked(const QString &filePath) {
    emit pauseScript(filePath);

    activeButtonState[filePath] = "pause";
    fileStatus[filePath] = "none"; // No icon on pause

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

void TextScriptWidget::handleRenameAction() {
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

void TextScriptWidget::handleRemoveAction() {
    QString filePath = fileListWidget->property("selectedFilePath").toString();
    QListWidgetItem *item = fileListWidget->currentItem();
    if (!item) return;

    TextScriptItemWidget *itemWidget = qobject_cast<TextScriptItemWidget*>(fileListWidget->itemWidget(item));
    if (!itemWidget) return;

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Remove File",
                                                              "Are you sure you want to remove " + QFileInfo(filePath).fileName() + "?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        emit removeScript(filePath);

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

void TextScriptWidget::handleAddScriptButtonClicked() {
    TestScriptDialog *dialog = new TestScriptDialog(this, false); // Add mode
    if (dialog->exec() == QDialog::Accepted) {
        loadScriptFiles("/home/arti_rajpoot/Downloads/TDF_with coordinate");
        Console::log("New script added, reloading script files");
    }
}

void TextScriptWidget::handleEditAction() {
    QString filePath = fileListWidget->property("selectedFilePath").toString();
    if (filePath.isEmpty()) {
        Console::error("No file selected for editing");
        QMessageBox::warning(this, "Edit Failed", "No file selected for editing.");
        return;
    }

    TestScriptDialog *dialog = new TestScriptDialog(this, true, filePath); // Edit mode
    if (dialog->exec() == QDialog::Accepted) {
        loadScriptFiles("/home/arti_rajpoot/Downloads/TDF_with coordinate");
        Console::log("Script edited and saved, reloading script files: " + filePath.toStdString());
    }
}
