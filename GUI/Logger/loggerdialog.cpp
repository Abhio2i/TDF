// /* ========================================================================= */
// /* File: loggerdialog.cpp                                                  */
// /* Purpose: Implements dialog for controlling logging and recording         */
// /* ========================================================================= */

// #include "loggerdialog.h"                          // For logger dialog class
// #include <QFileDialog>                             // For file dialog
// #include <QMessageBox>                             // For warning messages
// #include <QDialogButtonBox>                        // For dialog buttons
// #include <QFormLayout>                             // For form layout
// #include <QLineEdit>                               // For input fields
// #include <QPushButton>                             // For buttons
// #include <QStandardPaths>                          // For standard paths
// #include <QDir>                                    // For directory operations
// #include <QToolButton>                             // For toolbar buttons
// #include <QCheckBox>                               // For checkboxes
// #include <QLabel>                                  // For labels
// #include <QListWidget>                             // For list widget
// #include <QVBoxLayout>                             // For vertical layout
// #include <QHBoxLayout>                             // For horizontal layout

// // %%% Constructor %%%
// /* Initialize logger dialog */
// LoggerDialog::LoggerDialog(QWidget *parent)
//     : QDialog(parent)
// {
//     // Set window title and attributes
//     setWindowTitle(tr("Logger Control"));
//     setAttribute(Qt::WA_DeleteOnClose);
//     // Set recordings directory
//     recordingsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recordings";
//     // Set minimum dialog size
//     setMinimumSize(400, 550);
//     // Setup UI components
//     setupUi();
//     // Apply stylesheet
//     setStyleSheet(R"(
//         QDialog { background-color: #f5f6fa; font-family: 'Segoe UI', Arial, sans-serif; }
//         QToolButton { background-color: #ffffff; border: 1px solid #dcdcdc; border-radius: 4px; padding: 5px; }
//         QToolButton:hover { background-color: #e0e0e0; }
//         QPushButton { background-color: #0078d4; color: white; border: none; border-radius: 4px; padding: 8px 16px; font-size: 14px; }
//         QPushButton:hover { background-color: #005ba1; }
//         QPushButton:disabled { background-color: #a0a0a0; }
//         QCheckBox { font-size: 14px; padding: 5px; }
//         QLabel { font-size: 14px; color: #333333; }
//         QListWidget { border: 1px solid #dcdcdc; border-radius: 4px; background-color: #ffffff; padding: 5px; }
//         QFrame { background-color: #ffffff; border: 1px solid #dcdcdc; border-radius: 4px; padding: 10px; }
//         QPushButton#recordingActive { background-color: #55efc4; color: #2c3e50; }
//         QPushButton#recordingActive:hover { background-color: #00bc8c; }
//     )");
// }

// // %%% UI Setup %%%
// /* Setup dialog UI */
// void LoggerDialog::setupUi()
// {
//     // Create main layout
//     QVBoxLayout *mainLayout = new QVBoxLayout(this);
//     mainLayout->setSpacing(10);
//     mainLayout->setContentsMargins(15, 15, 15, 15);

//     // Setup top toolbar
//     QHBoxLayout *topLayout = new QHBoxLayout();
//     bookmarkButton = new QToolButton(this);
//     bookmarkButton->setIcon(QIcon(":/icons/images/star.png"));
//     bookmarkButton->setToolTip(tr("Add Bookmark"));
//     bookmarkButton->setFixedSize(32, 32);
//     timestampCheckBox = new QCheckBox(tr("Enable Timestamp"), this);
//     timestampCheckBox->setChecked(true);
//     topLayout->addWidget(bookmarkButton);
//     topLayout->addWidget(timestampCheckBox);
//     topLayout->addStretch();
//     mainLayout->addLayout(topLayout);

//     // Setup event selection frame
//     QFrame *eventFrame = new QFrame(this);
//     QVBoxLayout *eventLayout = new QVBoxLayout(eventFrame);
//     eventLayout->setSpacing(8);
//     QLabel *eventLabel = new QLabel(tr("Recordable Events"), this);
//     eventLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #2c3e50;");
//     actionsCheckBox = new QCheckBox(tr("Actions"), this);
//     waypointsCheckBox = new QCheckBox(tr("Waypoints"), this);
//     engagementsCheckBox = new QCheckBox(tr("Engagements"), this);
//     actionsCheckBox->setChecked(true);
//     waypointsCheckBox->setChecked(true);
//     engagementsCheckBox->setChecked(true);
//     eventLayout->addWidget(eventLabel);
//     eventLayout->addWidget(actionsCheckBox);
//     eventLayout->addWidget(waypointsCheckBox);
//     eventLayout->addWidget(engagementsCheckBox);
//     eventLayout->addStretch();
//     mainLayout->addWidget(eventFrame);

//     // Setup control frame
//     QFrame *controlFrame = new QFrame(this);
//     QVBoxLayout *controlLayout = new QVBoxLayout(controlFrame);
//     controlLayout->setSpacing(8);
//     QPushButton *startRecordingButton = new QPushButton(QIcon(":/icons/images/play.png"), tr("Start Recording"), this);
//     startRecordingButton->setObjectName("startRecordingButton");
//     QPushButton *stopRecordingButton = new QPushButton(QIcon(":/icons/images/stop.png"), tr("Stop Recording"), this);
//     stopRecordingButton->setEnabled(false);

//     // Setup timeline widget
//     timelineWidget = new TimelineWidget(this);
//     timelineWidget->setVisible(timestampCheckBox->isChecked());

//     // Setup recordings list
//     QLabel *recordingsLabel = new QLabel(tr("Recordings"), this);
//     recordingsLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #2c3e50;");
//     recordingsList = new QListWidget(this);
//     recordingsList->setSelectionMode(QAbstractItemView::SingleSelection);
//     replayButton = new QPushButton(QIcon(":/icons/images/replay.png"), tr("Replay Selected"), this);
//     replayButton->setEnabled(false);

//     // Initialize recordings directory
//     QDir dir(recordingsDir);
//     if (!dir.exists()) {
//         dir.mkpath(".");
//     }
//     updateRecordingsList();

//     // Add widgets to control layout
//     controlLayout->addWidget(startRecordingButton);
//     controlLayout->addWidget(stopRecordingButton);
//     controlLayout->addWidget(timelineWidget);
//     controlLayout->addWidget(recordingsLabel);
//     controlLayout->addWidget(recordingsList);
//     controlLayout->addWidget(replayButton);
//     controlLayout->addStretch();
//     mainLayout->addWidget(controlFrame);

//     // Add stretch to layout
//     mainLayout->addStretch();

//     // Connect start recording button
//     connect(startRecordingButton, &QPushButton::clicked, this, [this, startRecordingButton, stopRecordingButton]() {
//         recordingStartTime = QDateTime::currentDateTime();
//         timelineWidget->setRecordingStartTime(recordingStartTime);
//         timelineWidget->setRecordingDuration(0);
//         timelineWidget->clearBookmarks();
//         emit startRecording();
//         startRecordingButton->setText(tr("Recording..."));
//         startRecordingButton->setObjectName("recordingActive");
//         startRecordingButton->setStyleSheet("");
//         startRecordingButton->setEnabled(false);
//         stopRecordingButton->setEnabled(true);
//     });

//     // Connect stop recording button
//     connect(stopRecordingButton, &QPushButton::clicked, this, [this, startRecordingButton, stopRecordingButton]() {
//         if (recordingStartTime.isValid()) {
//             emit stopRecording();
//             startRecordingButton->setText(tr("Start Recording"));
//             startRecordingButton->setObjectName("startRecordingButton");
//             startRecordingButton->setStyleSheet("");
//             startRecordingButton->setEnabled(true);
//             stopRecordingButton->setEnabled(false);
//             updateRecordingsList();
//             timelineWidget->setRecordingDuration(0);
//             recordingStartTime = QDateTime();
//         }
//     });

//     // Connect recordings list selection
//     connect(recordingsList, &QListWidget::itemSelectionChanged, this, [this]() {
//         replayButton->setEnabled(recordingsList->selectedItems().count() > 0);
//     });

//     // Connect replay button
//     connect(replayButton, &QPushButton::clicked, this, [this, startRecordingButton]() {
//         if (recordingsList->selectedItems().count() > 0) {
//             QString selectedFile = recordingsList->selectedItems().first()->text();
//             QString filePath = recordingsDir + "/" + selectedFile;
//             emit replayRecording(filePath);
//             startRecordingButton->setText(tr("Replaying..."));
//             startRecordingButton->setObjectName("recordingActive");
//             startRecordingButton->setStyleSheet("");
//         }
//     });

//     // Connect event checkboxes
//     connect(actionsCheckBox, &QCheckBox::stateChanged, this, [this]() {
//         QStringList eventTypes;
//         if (actionsCheckBox->isChecked()) eventTypes << "Actions";
//         if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
//         if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
//         emit eventTypesSelected(eventTypes);
//     });

//     connect(waypointsCheckBox, &QCheckBox::stateChanged, this, [this]() {
//         QStringList eventTypes;
//         if (actionsCheckBox->isChecked()) eventTypes << "Actions";
//         if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
//         if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
//         emit eventTypesSelected(eventTypes);
//     });

//     connect(engagementsCheckBox, &QCheckBox::stateChanged, this, [this]() {
//         QStringList eventTypes;
//         if (actionsCheckBox->isChecked()) eventTypes << "Actions";
//         if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
//         if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
//         emit eventTypesSelected(eventTypes);
//     });

//     // Connect bookmark button
//     connect(bookmarkButton, &QToolButton::clicked, this, [this]() {
//         QDialog bookmarkDialog(this);
//         bookmarkDialog.setWindowTitle(tr("Add Bookmark"));
//         bookmarkDialog.setStyleSheet("background-color: #ffffff; font-family: 'Segoe UI', Arial, sans-serif;");
//         QVBoxLayout *dialogLayout = new QVBoxLayout(&bookmarkDialog);
//         QLineEdit *bookmarkEdit = new QLineEdit(&bookmarkDialog);
//         bookmarkEdit->setPlaceholderText(tr("Enter comment"));
//         QPushButton *okButton = new QPushButton(tr("OK"), &bookmarkDialog);
//         okButton->setStyleSheet("background-color: #0078d4; color: white; border: none; border-radius: 4px; padding: 8px 16px; font-size: 14px;");
//         dialogLayout->addWidget(bookmarkEdit);
//         dialogLayout->addWidget(okButton);
//         dialogLayout->addStretch();

//         // Connect bookmark dialog OK button
//         connect(okButton, &QPushButton::clicked, &bookmarkDialog, [this, &bookmarkDialog, bookmarkEdit]() {
//             if (!bookmarkEdit->text().isEmpty()) {
//                 QString bookmarkNote = bookmarkEdit->text();
//                 emit bookmarkAdded(bookmarkNote);
//                 if (timestampCheckBox->isChecked() && recordingStartTime.isValid()) {
//                     qint64 timestampMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());
//                     addBookmarkWithTimestamp(bookmarkNote, timestampMs);
//                 }
//                 bookmarkDialog.accept();
//             } else {
//                 QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter a bookmark comment."));
//             }
//         });

//         bookmarkDialog.exec();
//     });

//     // Connect timestamp checkbox
//     connect(timestampCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
//         bool enabled = (state == Qt::Checked);
//         emit timestampToggled(enabled);
//         timelineWidget->setVisible(enabled);
//         if (!enabled) {
//             timelineWidget->setRecordingDuration(0);
//             timelineWidget->clearBookmarks();
//         }
//     });
// }

// // %%% Utility Methods %%%
// /* Update recordings list */
// void LoggerDialog::updateRecordingsList()
// {
//     // Clear list
//     recordingsList->clear();
//     QStringList filters;
//     filters << "*.json";
//     // Get JSON files in recordings directory
//     QStringList recordingFiles = QDir(recordingsDir).entryList(filters, QDir::Files, QDir::Time);
//     // Add files to list
//     for (const QString &file : recordingFiles) {
//         QListWidgetItem *item = new QListWidgetItem(QIcon(":/icons/images/file.png"), file, recordingsList);
//         item->setToolTip(file);
//     }
// }

// /* Update recording duration */
// void LoggerDialog::updateRecordingDuration(qint64 durationMs)
// {
//     // Update timeline if timestamp enabled
//     if (timestampCheckBox->isChecked()) {
//         timelineWidget->setRecordingDuration(durationMs);
//     }
// }

// /* Add bookmark with timestamp */
// void LoggerDialog::addBookmarkWithTimestamp(const QString &note, qint64 timestampMs)
// {
//     // Add bookmark to timeline if timestamp enabled
//     if (timestampCheckBox->isChecked()) {
//         timelineWidget->addBookmark(note, timestampMs);
//     }
// }





#include "loggerdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
//By Hima
//#include "core/Recorder/recorder.h"
//End Hima

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
    //By Hima
    // saveRecordingButton = new QPushButton(QIcon(":/icons/images/folder.png"), tr("Save Recording"), this);
    // saveRecordingButton->setEnabled(true);
    // controlLayout->addWidget(saveRecordingButton);

    loadRecordingButton = new QPushButton(QIcon(":/icons/images/loading-arrow.png"),tr("Load Recording"), this);
    controlLayout->addWidget(loadRecordingButton);
    replayRecordingButton = new QPushButton(QIcon(":/icons/images/loading-arrow.png"), tr("Replay Recording"), this);
    controlLayout->addWidget(replayRecordingButton);
    //End Hima

    // Timeline widget
    timelineWidget = new TimelineWidget(this);
   timelineWidget->setVisible(true);

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

    // connect(stopRecordingButton, &QPushButton::clicked, this, [this, startRecordingButton, stopRecordingButton]() {
    //     if (recordingStartTime.isValid()) { // Check if recording is active
    //         emit stopRecording();
    //         startRecordingButton->setText(tr("Start Recording"));
    //         startRecordingButton->setObjectName("startRecordingButton");
    //         startRecordingButton->setStyleSheet("");
    //         startRecordingButton->setEnabled(true);
    //         stopRecordingButton->setEnabled(false);
    //         updateRecordingsList();
    //         timelineWidget->setRecordingDuration(0);
    //         recordingStartTime = QDateTime();
    //     }
    // });
    // //By Hima

    // connect(saveRecordingButton, &QPushButton::clicked, this, [this]() {
    //     emit saveRecording();  // Signal to runtime
    //     qDebug() << "Save Recording button clicked — signal emitted.";
    // });
    connect(stopRecordingButton, &QPushButton::clicked, this, [this, startRecordingButton, stopRecordingButton]() {
        if (recordingStartTime.isValid()) { // Check if recording is active
            // Stop recording
            emit stopRecording();
            startRecordingButton->setText(tr("Start Recording"));
            startRecordingButton->setObjectName("startRecordingButton");
            startRecordingButton->setStyleSheet("");
            startRecordingButton->setEnabled(true);
            stopRecordingButton->setEnabled(false);
            updateRecordingsList();
            timelineWidget->setRecordingDuration(0);
            recordingStartTime = QDateTime();

            // Trigger save recording
            emit saveRecording();
            qDebug() << "Recording stopped and saveRecording signal emitted.";
        }
    });

    // connect(loadRecordingButton, &QPushButton::clicked, this, [this]() {
    //     filePath = QFileDialog::getOpenFileName(
    //         this,
    //         tr("Open Recording File"),
    //         QDir::homePath(),
    //         tr("JSON Files (*.json)")
    //         );

    //     if (filePath.isEmpty())
    //         return;

    //     emit loadRecording(filePath);
    //     qDebug() << "Load Recording button clicked — file path:" << filePath;
    // });
    // connect(replayRecordingButton, &QPushButton::clicked, this, [this]() {

    //     if (filePath.isEmpty())
    //         qDebug() << "File is not Yet Loaded to play" << filePath;
    //         return;

    //     emit loadRecording(filePath);
    //     qDebug() << "Load Recording button clicked — file path:" << filePath;
    // });
    // Load button: choose a file once
    connect(loadRecordingButton, &QPushButton::clicked, this, [this]() {
        QString selectedFile = QFileDialog::getOpenFileName(
            this,
            tr("Open Recording File"),
            QDir::homePath(),
            tr("JSON Files (*.json)")
            );

        if (selectedFile.isEmpty())
            return; // User cancelled

        filePath = selectedFile; // store permanently

        emit loadRecording(filePath);
        qDebug() << "Recording loaded — file path:" << filePath;
    });

    // Replay button: uses stored filePath
    connect(replayRecordingButton, &QPushButton::clicked, this, [this]() {
        if (filePath.isEmpty()) {
            qDebug() << "No recording loaded to replay!";
            return;
        }

        emit loadRecording(filePath); // reuse the same file
        qDebug() << "Replay triggered — file path:" << filePath;
    });

    //End Hima

    // //By Hima
    // // connect(&LoggerDialog, &LoggerDialog::saveRecordingRequested, &Recorder, [&]() {
    // //     this->saveRecordingToFile(Recorder.getAllRecordings());

    // // });
    // connect(saveButton, &QPushButton::clicked, this, [this, &Recorder]() {
    //     // Grab the full JSON from your Recorder and save it
    //     saveRecordingToFile(Recorder.getAllRecordings());
    // });

    // //End Hima
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
        // timelineWidget->setVisible(enabled);
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
