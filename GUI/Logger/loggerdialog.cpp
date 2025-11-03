
#include "loggerdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include "core/Recorder/recorder.h"


LoggerDialog::LoggerDialog(QWidget *parent, Recorder* recorderParam)
    : QDialog(parent), recorder(recorderParam)
{
    setWindowTitle(tr("Logger Control"));
    setAttribute(Qt::WA_DeleteOnClose);
    recordingsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recordings";
    setMinimumSize(400, 550);
    setupUi();

    setStyleSheet(R"(
        QDialog {
            background-color: #f5f6fa;
            font-family: 'Segoe UI', Arial, sans-serif;
        }
        QToolButton {
            background-color: #ffffff;
            border: 1px solid #dcdcdc;
            border-radius: 4px;
            padding: 5px;
        }
        QToolButton:hover {
            background-color: #e0e0e0;
        }
        QPushButton {
            background-color: #0078d4;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #005ba1;
        }
        QPushButton:disabled {
            background-color: #a0a0a0;
        }
        QCheckBox {
            font-size: 14px;
            padding: 5px;
        }
        QLabel {
            font-size: 14px;
            color: #333333;
        }
        QListWidget {
            border: 1px solid #dcdcdc;
            border-radius: 4px;
            background-color: #ffffff;
            padding: 5px;
        }
        QFrame {
            background-color: #ffffff;
            border: 1px solid #dcdcdc;
            border-radius: 4px;
            padding: 10px;
        }
        QPushButton#recordingActive {
            background-color: #55efc4;
            color: #2c3e50;
        }
        QPushButton#recordingActive:hover {
            background-color: #00bc8c;
        }
    )");
}



void LoggerDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Top toolbar for bookmark and timestamp
    QHBoxLayout *topLayout = new QHBoxLayout();
    bookmarkButton = new QToolButton(this);
    bookmarkButton->setIcon(QIcon(":/icons/images/star.png"));
    bookmarkButton->setToolTip(tr("Add Bookmark"));
    bookmarkButton->setFixedSize(32, 32);
    timestampCheckBox = new QCheckBox(tr("Enable Timestamp"), this);
    timestampCheckBox->setChecked(true);
    topLayout->addWidget(bookmarkButton);
    topLayout->addWidget(timestampCheckBox);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // Event selection frame
    QFrame *eventFrame = new QFrame(this);
    QVBoxLayout *eventLayout = new QVBoxLayout(eventFrame);
    eventLayout->setSpacing(8);
    QLabel *eventLabel = new QLabel(tr("Recordable Events"), this);
    eventLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #2c3e50;");
    actionsCheckBox = new QCheckBox(tr("Actions"), this);
    waypointsCheckBox = new QCheckBox(tr("Waypoints"), this);
    engagementsCheckBox = new QCheckBox(tr("Engagements"), this);
    actionsCheckBox->setChecked(true);
    waypointsCheckBox->setChecked(true);
    engagementsCheckBox->setChecked(true);
    eventLayout->addWidget(eventLabel);
    eventLayout->addWidget(actionsCheckBox);
    eventLayout->addWidget(waypointsCheckBox);
    eventLayout->addWidget(engagementsCheckBox);
    eventLayout->addStretch();
    mainLayout->addWidget(eventFrame);

    // Control frame
    QFrame *controlFrame = new QFrame(this);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlFrame);
    controlLayout->setSpacing(8);

    // Icon-only buttons in a single row
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    QToolButton *startRecordingButton = new QToolButton(this);
    startRecordingButton->setIcon(QIcon(":/icons/images/play.png"));
    startRecordingButton->setToolTip(tr("Start Recording"));
    startRecordingButton->setFixedSize(32, 32);
    startRecordingButton->setObjectName("startRecordingButton");

    QToolButton *replayRecordingButton = new QToolButton(this);
    replayRecordingButton->setIcon(QIcon(":/icons/images/replay.png"));
    replayRecordingButton->setToolTip(tr("Replay Recording"));
    replayRecordingButton->setFixedSize(32, 32);

    QToolButton *stopRecordingButton = new QToolButton(this);
    stopRecordingButton->setIcon(QIcon(":/icons/images/stop.png"));
    stopRecordingButton->setToolTip(tr("Stop Recording"));
    stopRecordingButton->setFixedSize(32, 32);
    stopRecordingButton->setEnabled(false); // Disable Stop button by default

    QToolButton *loadRecordingButton = new QToolButton(this);
    loadRecordingButton->setIcon(QIcon(":/icons/images/loading-arrow.png"));
    loadRecordingButton->setToolTip(tr("Load Recording"));
    loadRecordingButton->setFixedSize(32, 32);

    buttonLayout->addWidget(startRecordingButton);
    buttonLayout->addWidget(replayRecordingButton);
    buttonLayout->addWidget(stopRecordingButton);
    buttonLayout->addWidget(loadRecordingButton);
    buttonLayout->addStretch();
    controlLayout->addLayout(buttonLayout);

    // Timeline widget
    timelineWidget = new TimelineWidget(this);
    timelineWidget->setVisible(true);

    // Add widgets to control layout
    controlLayout->addWidget(timelineWidget);
    controlLayout->addStretch();
    mainLayout->addWidget(controlFrame);

    // Stretch to push content up
    mainLayout->addStretch();

    // Connections
    connect(startRecordingButton, &QToolButton::clicked, this, [this, startRecordingButton, stopRecordingButton]() {
        recordingStartTime = QDateTime::currentDateTime();
        timelineWidget->setRecordingStartTime(recordingStartTime);
        timelineWidget->setRecordingDuration(0);
        timelineWidget->clearBookmarks();
        emit startRecording();
        startRecordingButton->setToolTip(tr("Recording..."));
        startRecordingButton->setObjectName("recordingActive");
        startRecordingButton->setStyleSheet("");
        startRecordingButton->setEnabled(false);
        stopRecordingButton->setEnabled(true);
    });

    connect(stopRecordingButton, &QToolButton::clicked, this, [this, startRecordingButton, stopRecordingButton]() {
        if (recordingStartTime.isValid()) { // Check if recording is active
            // Stop recording
            emit stopRecording();

            // Open file dialog to choose save location and file name
            QString saveFilePath = QFileDialog::getSaveFileName(
                this,
                tr("Save Recording"),
                QDir::homePath(), // Default to home directory instead of recordingsDir
                tr("JSON Files (*.json)")
                );

            if (!saveFilePath.isEmpty()) { // User provided a file name and location
                // Ensure the file has .json extension
                if (!saveFilePath.endsWith(".json", Qt::CaseInsensitive)) {
                    saveFilePath += ".json";
                }
                filePath = saveFilePath; // Store the file path
                emit saveRecording(filePath); // Emit signal with file path
                qDebug() << "Recording stopped and saveRecording signal emitted with file path:" << filePath;
            } else {
                qDebug() << "Save cancelled by user.";
            }

            // Reset UI
            startRecordingButton->setToolTip(tr("Start Recording"));
            startRecordingButton->setObjectName("startRecordingButton");
            startRecordingButton->setStyleSheet("");
            startRecordingButton->setEnabled(true);
            stopRecordingButton->setEnabled(false);
            timelineWidget->setRecordingDuration(0);
            recordingStartTime = QDateTime();
        }
    });

    connect(replayRecordingButton, &QToolButton::clicked, this, [this]() {
        if (filePath.isEmpty()) {
            qDebug() << "No recording loaded to replay!";
            return;
        }
        emit loadRecording(filePath);
        qDebug() << "Replay triggered — file path:" << filePath;
    });

    connect(loadRecordingButton, &QToolButton::clicked, this, [this]() {
        QString selectedFile = QFileDialog::getOpenFileName(
            this,
            tr("Open Recording File"),
            QDir::homePath(),
            tr("JSON Files (*.json)")
            );

        if (selectedFile.isEmpty())
            return; // User cancelled

        filePath = selectedFile; // Store permanently
        emit loadRecording(filePath);
        qDebug() << "Recording loaded — file path:" << filePath;
    });

    connect(actionsCheckBox, &QCheckBox::stateChanged, this, [this]() {
        QStringList eventTypes;
        if (actionsCheckBox->isChecked()) eventTypes << "Actions";
        if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
        if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
        emit eventTypesSelected(eventTypes);
    });

    connect(waypointsCheckBox, &QCheckBox::stateChanged, this, [this]() {
        QStringList eventTypes;
        if (actionsCheckBox->isChecked()) eventTypes << "Actions";
        if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
        if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
        emit eventTypesSelected(eventTypes);
    });

    connect(engagementsCheckBox, &QCheckBox::stateChanged, this, [this]() {
        QStringList eventTypes;
        if (actionsCheckBox->isChecked()) eventTypes << "Actions";
        if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
        if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
        emit eventTypesSelected(eventTypes);
    });

    connect(bookmarkButton, &QToolButton::clicked, this, [this]() {
        QDialog bookmarkDialog(this);
        bookmarkDialog.setWindowTitle(tr("Add Bookmark"));
        bookmarkDialog.setStyleSheet("background-color: #ffffff; font-family: 'Segoe UI', Arial, sans-serif;");
        QVBoxLayout *dialogLayout = new QVBoxLayout(&bookmarkDialog);
        QLineEdit *bookmarkEdit = new QLineEdit(&bookmarkDialog);
        bookmarkEdit->setPlaceholderText(tr("Enter comment"));
        QPushButton *okButton = new QPushButton(tr("OK"), &bookmarkDialog);
        okButton->setStyleSheet("background-color: #0078d4; color: white; border: none; border-radius: 4px; padding: 8px 16px; font-size: 14px;");
        dialogLayout->addWidget(bookmarkEdit);
        dialogLayout->addWidget(okButton);
        dialogLayout->addStretch();

        connect(okButton, &QPushButton::clicked, &bookmarkDialog, [this, &bookmarkDialog, bookmarkEdit]() {
            if (!bookmarkEdit->text().isEmpty()) {
                QString bookmarkNote = bookmarkEdit->text();
                emit bookmarkAdded(bookmarkNote);

                if (recorder && recordingStartTime.isValid()) {
                    qint64 timestampMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());
                    recorder->saveBookmark(bookmarkNote, timestampMs);
                }

                if (timestampCheckBox->isChecked() && recordingStartTime.isValid()) {
                    qint64 timestampMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());
                    addBookmarkWithTimestamp(bookmarkNote, timestampMs);
                }

                bookmarkDialog.accept();
            } else {
                QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter a bookmark comment."));
            }
        });

        bookmarkDialog.exec();
    });

    connect(timestampCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
        bool enabled = (state == Qt::Checked);
        emit timestampToggled(enabled);
        if (!enabled) {
            timelineWidget->setRecordingDuration(0);
            timelineWidget->clearBookmarks();
        }
    });

    connect(timelineWidget, &TimelineWidget::bookmarkButtonClicked, this, [this](const QString &note, qint64 timestampMs) {
        emit bookmarkButtonClicked(note, timestampMs);
        emit bookmarkClicked(note, timestampMs);
        qDebug() << "Bookmark clicked: Note =" << note << ", Timestamp =" << timestampMs << "ms";
    });
}
void LoggerDialog::updateRecordingsList()
{
    recordingsList->clear();
    QStringList filters;
    filters << "*.json";
    QStringList recordingFiles = QDir(recordingsDir).entryList(filters, QDir::Files, QDir::Time);
    for (const QString &file : recordingFiles) {
        QListWidgetItem *item = new QListWidgetItem(QIcon(":/icons/images/file.png"), file, recordingsList);
        item->setToolTip(file);
    }
}

void LoggerDialog::updateRecordingDuration(qint64 durationMs)
{
    if (timestampCheckBox->isChecked()) {
        timelineWidget->setRecordingDuration(durationMs);
    }
}

void LoggerDialog::addBookmarkWithTimestamp(const QString &note, qint64 timestampMs)
{
    if (timestampCheckBox->isChecked()) {
        timelineWidget->addBookmark(note, timestampMs);
    }
}

void LoggerDialog::showBookmarkOnReplay(const QString& note, qint64 timestamp)
{
    if (timelineWidget) {
        timelineWidget->addBookmark(note, timestamp);
    }
}

void LoggerDialog::setTimelineDuration(qint64 duration)
{
    if (timelineWidget) {
        timelineWidget->clearBookmarks();
        timelineWidget->setRecordingDuration(duration);
    }
}

void LoggerDialog::replayFromBookmark(const QString& note, qint64 timestamp){

}
