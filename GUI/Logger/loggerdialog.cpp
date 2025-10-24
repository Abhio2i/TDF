
#include "loggerdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>

LoggerDialog::LoggerDialog(QWidget *parent) : QDialog(parent)
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
    QPushButton *startRecordingButton = new QPushButton(QIcon(":/icons/images/play.png"), tr("Start Recording"), this);
    startRecordingButton->setObjectName("startRecordingButton");
    QPushButton *stopRecordingButton = new QPushButton(QIcon(":/icons/images/stop.png"), tr("Stop Recording"), this);
    stopRecordingButton->setEnabled(false); // Disable Stop button by default

    // Timeline widget
    timelineWidget = new TimelineWidget(this);
    timelineWidget->setVisible(timestampCheckBox->isChecked());

    // Recordings label and list
    QLabel *recordingsLabel = new QLabel(tr("Recordings"), this);
    recordingsLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #2c3e50;");
    recordingsList = new QListWidget(this);
    recordingsList->setSelectionMode(QAbstractItemView::SingleSelection);
    replayButton = new QPushButton(QIcon(":/icons/images/replay.png"), tr("Replay Selected"), this);
    replayButton->setEnabled(false);

    QDir dir(recordingsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    updateRecordingsList();

    // Add widgets to control layout
    controlLayout->addWidget(startRecordingButton);
    controlLayout->addWidget(stopRecordingButton);
    controlLayout->addWidget(timelineWidget);
    controlLayout->addWidget(recordingsLabel);
    controlLayout->addWidget(recordingsList);
    controlLayout->addWidget(replayButton);
    controlLayout->addStretch();
    mainLayout->addWidget(controlFrame);

    // Stretch to push content up
    mainLayout->addStretch();

    // Connections
    connect(startRecordingButton, &QPushButton::clicked, this, [this, startRecordingButton, stopRecordingButton]() {
        recordingStartTime = QDateTime::currentDateTime();
        timelineWidget->setRecordingStartTime(recordingStartTime);
        timelineWidget->setRecordingDuration(0);
        timelineWidget->clearBookmarks();
        emit startRecording();
        startRecordingButton->setText(tr("Recording..."));
        startRecordingButton->setObjectName("recordingActive");
        startRecordingButton->setStyleSheet("");
        startRecordingButton->setEnabled(false);
        stopRecordingButton->setEnabled(true);
    });

    connect(stopRecordingButton, &QPushButton::clicked, this, [this, startRecordingButton, stopRecordingButton]() {
        if (recordingStartTime.isValid()) { // Check if recording is active
            emit stopRecording();
            startRecordingButton->setText(tr("Start Recording"));
            startRecordingButton->setObjectName("startRecordingButton");
            startRecordingButton->setStyleSheet("");
            startRecordingButton->setEnabled(true);
            stopRecordingButton->setEnabled(false);
            updateRecordingsList();
            timelineWidget->setRecordingDuration(0);
            recordingStartTime = QDateTime();
        }
    });

    connect(recordingsList, &QListWidget::itemSelectionChanged, this, [this]() {
        replayButton->setEnabled(recordingsList->selectedItems().count() > 0);
    });

    connect(replayButton, &QPushButton::clicked, this, [this, startRecordingButton]() {
        if (recordingsList->selectedItems().count() > 0) {
            QString selectedFile = recordingsList->selectedItems().first()->text();
            QString filePath = recordingsDir + "/" + selectedFile;
            emit replayRecording(filePath);
            startRecordingButton->setText(tr("Replaying..."));
            startRecordingButton->setObjectName("recordingActive");
            startRecordingButton->setStyleSheet("");
        }
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
        timelineWidget->setVisible(enabled);
        if (!enabled) {
            timelineWidget->setRecordingDuration(0);
            timelineWidget->clearBookmarks();
        }
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
