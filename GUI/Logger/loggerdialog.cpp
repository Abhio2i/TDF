

#include "loggerdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QTabWidget>
#include <QStackedWidget>
#include "core/Recorder/recorder.h"


LoggerDialog::LoggerDialog(QWidget *parent, Recorder* recorderParam)
    : QMainWindow(parent), recorder(recorderParam)
{
    setWindowTitle(tr("Logger Control"));
    setAttribute(Qt::WA_DeleteOnClose);
    recordingsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recordings";
    setMinimumSize(400, 550);

    //For Assigning value the Recording Information Start
    loggerStatusModeString[Recorder::S_RECORDING_MODE] = "Recording Mode";
    loggerStatusModeString[Recorder::S_RECORDING] = "Recording";
    loggerStatusModeString[Recorder::S_RECORDING_PAUSED] = "Paused";
    loggerStatusModeString[Recorder::S_RECORDING_STOPPED] = "Stopped";
    loggerStatusModeString[Recorder::S_REPLAY_MODE] = "Replay Mode";
    loggerStatusModeString[Recorder::S_REPLAY_LOADED] = "Replay Loaded";
    loggerStatusModeString[Recorder::S_REPLAY_UNLOADED] = "Replay Unloaded";
    loggerStatusModeString[Recorder::S_REPLAYING] = "Replaying";
    loggerStatusModeString[Recorder::S_REPLAY_PAUSED] = "Paused";
    loggerStatusModeString[Recorder::S_REPLAY_STOPPED] = "Stopped";
    SimulationStatusModeString[Recorder::S_SIMULATION_START] = "Start";
    SimulationStatusModeString[Recorder::S_SIMULATION_PAUSED]= "Paused";
    SimulationStatusModeString[Recorder::S_SIMULATION_STOP]  = "Stop";
    SimulationStatusModeString[Recorder::S_SIMULATION_NA]    = "Not Available";


    setupMenuBar();
    setupUi();

    setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f6fa;
            font-family: 'Segoe UI', Arial, sans-serif;
        }
        QToolButton {
            background-color: transparent;
            border: none;
            padding: 0px;
            margin: 0px;
        }
        QToolButton:hover {
            background-color: #e0e0e0;
            border-radius: 3px;
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
        QLabel {
            font-size: 14px;
            color: #333333;
            padding: 5px;
        }
        QGroupBox {
            font-weight: bold;
            font-size: 14px;
            border: 1px solid #dcdcdc;
            border-radius: 4px;
            margin-top: 10px;
            padding-top: 10px;
            background-color: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        QToolButton#recordingActive {
            background-color: #e74c3c;
            color: white;
        }
        QToolButton#paused {
            background-color: #f39c12;
            color: white;
        }
        QToolButton#replayActive {
            background-color: #27ae60;
            color: white;
        }
        QMenuBar {
            background-color: #e0e0e0;
            color: black;
            font-size: 14px;
        }
        QMenuBar::item {
            background-color: #e0e0e0;
            color: black;
            padding: 4px 10px;
        }
        QMenuBar::item:selected {
            background-color: #d0d0d0;
            color: black;
        }
        QMenuBar::item:pressed {
            background-color: #c0c0c0;
            color: black;
        }
        QMenu {
            background-color: #e0e0e0;
            border: 1px solid #c0c0c0;
            color: black;
        }
        QMenu::item {
            background-color: #e0e0e0;
            color: black;
            padding: 5px 20px;
        }
        QMenu::item:selected {
            background-color: #d0d0d0;
            color: black;
        }
        QMenu::item:pressed {
            background-color: #c0c0c0;
            color: black;
        }
        QTabWidget {
            border: 0px;
        }
        QTabWidget::pane {
            border: 0px;
            margin: 0px;
            padding: 0px;
            background-color: transparent;
        }
        QTabWidget::tab-bar {
            alignment: center;
        }
        QTabBar::tab {
            background-color: #e0e0e0;
            border: 1px solid #dcdcdc;
            padding: 8px 16px;
            margin: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background-color: #0078d4;
            color: white;
        }
        QTabBar::tab:hover:!selected {
            background-color: #d0d0d0;
        }
    )");

}

void LoggerDialog::setupMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *fileMenu = menuBar->addMenu(tr("File"));
    QAction *loadAction = fileMenu->addAction(tr("Load Recording"));

    // QMenu *toolsMenu = menuBar->addMenu(tr("Tools"));
    // QAction *settingsAction = toolsMenu->addAction(tr("Settings"));

    // QMenu *helpMenu = menuBar->addMenu(tr("Help"));
    // QAction *aboutAction = helpMenu->addAction(tr("About"));
    // QAction *helpAction = helpMenu->addAction(tr("Help"));

    connect(loadAction, &QAction::triggered, this, [this]() {
        pauseResumeReplayButton->setEnabled(true);
        pauseResumeReplayButton->setIcon(QIcon(":/icons/images/pause.png"));
        pauseResumeReplayButton->setToolTip(tr("Pause Replay"));

        QString selectedFile = QFileDialog::getOpenFileName(
            this,
            tr("Open Recording File"),
            QDir::homePath(),
            tr("JSON Files (*.json)")
            );

        if (selectedFile.isEmpty()) return;

        filePath = selectedFile;
        emit loadRecording(filePath);

        QFileInfo fileInfo(filePath);
        recordingDateLabel->setText(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
        //loggerStatusLabel->setText(tr("Loaded"));
        loggerStatusLabel->setStyleSheet("font-weight: bold; color: #27ae60;");

        startReplayButton->setEnabled(true);
        previousFrameButton->setEnabled(true);
        nextFrameButton->setEnabled(true);

        // qDebug() << "Loading record" << filePath;
    });
}

void LoggerDialog::setupUi()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    modeTabWidget = new QTabWidget(this);
    modeTabWidget->setTabPosition(QTabWidget::North);
    modeTabWidget->setDocumentMode(true);
    modeTabWidget->setFixedHeight(80);

    QWidget *recordingTab = new QWidget();
    recordingTab->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *recordingLayout = new QVBoxLayout(recordingTab);
    recordingLayout->setContentsMargins(0, 0, 0, 0);
    recordingLayout->setSpacing(0);
    recordingLayout->addWidget(createRecordingControls());

    QWidget *replayTab = new QWidget();
    replayTab->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *replayLayout = new QVBoxLayout(replayTab);
    replayLayout->setContentsMargins(0, 0, 0, 0);
    replayLayout->setSpacing(0);
    replayLayout->addWidget(createReplayControls());

    modeTabWidget->addTab(recordingTab, tr("Recording"));
    modeTabWidget->addTab(replayTab, tr("Replay"));
    mainLayout->addWidget(modeTabWidget);

    QHBoxLayout *topLayout = new QHBoxLayout();
    bookmarkButton = new QToolButton(this);
    bookmarkButton->setIcon(QIcon(":/icons/images/star.png"));
    bookmarkButton->setToolTip(tr("Add Bookmark"));
    bookmarkButton->setFixedSize(28, 28);

    timestampCheckBox = new QCheckBox(tr("Enable Timestamp"), this);
    timestampCheckBox->setChecked(true);

    topLayout->addWidget(bookmarkButton);
    topLayout->addWidget(timestampCheckBox);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    QGroupBox *infoGroup = new QGroupBox(tr("Recording Information"), this);
    QFormLayout *formLayout = new QFormLayout(infoGroup);

    recordingDateLabel = new QLabel(tr("No recording"), this);
    durationLabel = new QLabel(tr("00:00:00"), this);
    loggerStatusLabel = new QLabel(tr("Stopped"), this);
    simulationStatusLabel = new QLabel(tr("Not Available"), this);

    recordingDateLabel->setStyleSheet("font-weight: normal; color: #666;");
    durationLabel->setStyleSheet("font-weight: normal; color: #666;");
    loggerStatusLabel->setStyleSheet("font-weight: normal; color: #666;");
    simulationStatusLabel->setStyleSheet("font-weight: normal; color: #666;");

    formLayout->addRow(tr("Recording Date:"), recordingDateLabel);
    formLayout->addRow(tr("Duration:"), durationLabel);
    formLayout->addRow(tr("Logger Status:"), loggerStatusLabel);
    formLayout->addRow(tr("Simulation Status:"), simulationStatusLabel);

    mainLayout->addWidget(infoGroup);

    timelineWidget = new TimelineWidget(this);
    timelineWidget->setVisible(true);
    mainLayout->addWidget(timelineWidget);

    mainLayout->addStretch();


    connect(modeTabWidget, &QTabWidget::currentChanged, this, [this](int index) {


        bool isBusy = (loggerStatus == Recorder::S_RECORDING) ||
                      (loggerStatus == Recorder::S_REPLAYING);


        if (isBusy) {

            QSignalBlocker blocker(modeTabWidget);
            modeTabWidget->setCurrentIndex(index == 0 ? 1 : 0);

            return;
        }

        if (index == 0) {
            switchToRecordingMode();
        } else {
            switchToReplayMode();
        }
    });


    connect(bookmarkButton, &QToolButton::clicked, this, &LoggerDialog::showBookmarkDialog);
    setupConnections();
}

QWidget* LoggerDialog::createRecordingControls()
{
    QWidget *container = new QWidget();
    container->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *controlLayout = new QHBoxLayout(container);
    controlLayout->setContentsMargins(5, 0, 5, 0);
    controlLayout->setSpacing(8);

    recordButton = new QToolButton(this);
    recordButton->setIcon(QIcon(":/icons/images/record-button.png"));
    recordButton->setToolTip(tr("Start Recording"));
    recordButton->setFixedSize(28, 28);
    recordButton->setIconSize(QSize(24, 24));

    pauseRecordingButton = new QToolButton(this);
    pauseRecordingButton->setIcon(QIcon(":/icons/images/pause.png"));
    pauseRecordingButton->setToolTip(tr("Pause Recording"));
    pauseRecordingButton->setFixedSize(28, 28);
    pauseRecordingButton->setIconSize(QSize(24, 24));
    pauseRecordingButton->setEnabled(false);

    stopRecordingButton = new QToolButton(this);
    stopRecordingButton->setIcon(QIcon(":/icons/images/stop.png"));
    stopRecordingButton->setToolTip(tr("Stop Recording"));
    stopRecordingButton->setFixedSize(28, 28);
    stopRecordingButton->setIconSize(QSize(24, 24));
    stopRecordingButton->setEnabled(false);

    databaseButton = new QToolButton(this);
    databaseButton->setIcon(QIcon(":/icons/images/database.png"));
    databaseButton->setToolTip(tr("Connect to DataBase"));
    databaseButton->setFixedSize(28, 28);
    databaseButton->setIconSize(QSize(24, 24));
    databaseButton->setEnabled(true);

    debugButton = new QToolButton(this);
    debugButton->setIcon(QIcon(":/icons/images/log-file.png"));
    debugButton->setToolTip(tr("Connect to DataBase"));
    debugButton->setFixedSize(28, 28);
    debugButton->setIconSize(QSize(24, 24));
    debugButton->setEnabled(true);

    controlLayout->addWidget(recordButton);
    controlLayout->addWidget(pauseRecordingButton);
    controlLayout->addWidget(stopRecordingButton);
    controlLayout->addWidget(databaseButton);
    controlLayout->addWidget(debugButton);
    controlLayout->addStretch();

    return container;
}

QWidget* LoggerDialog::createReplayControls()
{
    QWidget *container = new QWidget();
    container->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *controlLayout = new QHBoxLayout(container);
    controlLayout->setContentsMargins(5, 0, 5, 0);
    controlLayout->setSpacing(8);

    startReplayButton = new QToolButton(this);
    startReplayButton->setIcon(QIcon(":/icons/images/play.png"));
    startReplayButton->setToolTip(tr("Start Replay"));
    startReplayButton->setFixedSize(28, 28);
    startReplayButton->setIconSize(QSize(24, 24));

    pauseResumeReplayButton = new QToolButton(this);
    pauseResumeReplayButton->setIcon(QIcon(":/icons/images/pause.png"));
    pauseResumeReplayButton->setToolTip(tr("Pause Replay"));
    pauseResumeReplayButton->setFixedSize(28, 28);
    pauseResumeReplayButton->setIconSize(QSize(24, 24));
    pauseResumeReplayButton->setEnabled(false);

    previousFrameButton = new QToolButton(this);
    previousFrameButton->setIcon(QIcon(":/icons/images/back.png"));
    previousFrameButton->setToolTip(tr("Previous Frame"));
    previousFrameButton->setFixedSize(28, 28);
    previousFrameButton->setIconSize(QSize(24, 24));
    previousFrameButton->setEnabled(false);

    nextFrameButton = new QToolButton(this);
    nextFrameButton->setIcon(QIcon(":/icons/images/next-button.png"));
    nextFrameButton->setToolTip(tr("Next Frame"));
    nextFrameButton->setFixedSize(28, 28);
    nextFrameButton->setIconSize(QSize(24, 24));
    nextFrameButton->setEnabled(false);

    loadRecordingButton = new QToolButton(this);
    loadRecordingButton->setIcon(QIcon(":/icons/images/loading-arrow.png"));
    loadRecordingButton->setToolTip(tr("Load Recording"));
    loadRecordingButton->setFixedSize(28, 28);
    loadRecordingButton->setIconSize(QSize(24, 24));

    databaseButtonReplay = new QToolButton(this);
    databaseButtonReplay->setIcon(QIcon(":/icons/images/database.png"));
    databaseButtonReplay->setToolTip(tr("Connect to DataBase"));
    databaseButtonReplay->setFixedSize(28, 28);
    databaseButtonReplay->setIconSize(QSize(24, 24));
    databaseButtonReplay->setEnabled(true);

    controlLayout->addWidget(startReplayButton);
    controlLayout->addWidget(pauseResumeReplayButton);
    controlLayout->addWidget(previousFrameButton);
    controlLayout->addWidget(nextFrameButton);
    controlLayout->addWidget(loadRecordingButton);
    controlLayout->addWidget(databaseButtonReplay);
    controlLayout->addStretch();

    return container;
}

void LoggerDialog::setupConnections()
{

    connect(recordButton, &QToolButton::clicked, this, [this]() {
        inspectRecorder();
        recordingStartTime = QDateTime::currentDateTime();
        // timelineWidget->setRecordingStartTime(recordingStartTime);
        // timelineWidget->setRecordingDuration(0);
        // timelineWidget->clearBookmarks();
        recordingDateLabel->setText(recordingStartTime.toString("yyyy-MM-dd hh:mm:ss"));
        loggerStatusLabel->setStyleSheet("font-weight: bold; color: #e74c3c;");
        emit recordingStart(*recorder);
        inspectRecorder();
        timelineWidget->setValues(
            loggerMode     ,
            bookmarkDblPtr,
            durationDblPtr);
        timelineWidget->startUpdateUI();
        recordButton->setToolTip(tr("Recording..."));
        recordButton->setObjectName("recordingActive");
        recordButton->setStyleSheet("");
        recordButton->setEnabled(false);
        pauseRecordingButton->setEnabled(true);
        stopRecordingButton->setEnabled(true);

        isRecordingPaused = false;
        pauseRecordingButton->setIcon(QIcon(":/icons/images/pause.png"));
        pauseRecordingButton->setToolTip(tr("Pause Recording"));
    });

    connect(pauseRecordingButton, &QToolButton::clicked, this, [this]() {

        if (!isRecordingPaused) {
            //timelineWidget->pauseRecording();
        } else {
            //timelineWidget->resumeRecording();
        }
        if (!isRecordingPaused) {
            //loggerStatusLabel->setText(tr("Recording Paused"));
            loggerStatusLabel->setStyleSheet("font-weight: bold; color: #f39c12;");
            pauseRecordingButton->setIcon(QIcon(":/icons/images/resume.png"));
            pauseRecordingButton->setToolTip(tr("Resume Recording"));
            isRecordingPaused = true;
            emit recordingPause();
            timelineWidget->pause();
        } else {
            //loggerStatusLabel->setText(tr("Recording"));
            loggerStatusLabel->setStyleSheet("font-weight: bold; color: #e74c3c;");
            pauseRecordingButton->setIcon(QIcon(":/icons/images/pause.png"));
            pauseRecordingButton->setToolTip(tr("Pause Recording"));
            isRecordingPaused = false;
            emit recordingResume();
            timelineWidget->resume();
        }
    });

    connect(stopRecordingButton, &QToolButton::clicked, this, [this]() {
        if (recordingStartTime.isValid()) {

            emit recordingStop();
            timelineWidget->stop();

            recordButton->setToolTip(tr("Start Recording"));
            recordButton->setObjectName("");
            recordButton->setStyleSheet("");
            recordButton->setEnabled(true);
            pauseRecordingButton->setEnabled(false);
            stopRecordingButton->setEnabled(false);
            //loggerStatusLabel->setText(tr("Stopped"));
            loggerStatusLabel->setStyleSheet("font-weight: normal; color: #666;");
            //timelineWidget->setRecordingDuration(0);
            recordingStartTime = QDateTime();

            isRecordingPaused = false;
            pauseRecordingButton->setIcon(QIcon(":/icons/images/pause.png"));
            pauseRecordingButton->setToolTip(tr("Pause Recording"));
        }
    });

    connect(loadRecordingButton, &QToolButton::clicked, this, [this]() {
        if(!dbStatusPtr || dbStatusPtr == nullptr){
            QMessageBox::warning(this, tr("Warning"), tr("DataBase Pointer Not Defined!"));
            return;
        }

        switch(*dbStatusPtr){
        case SQLite::DISCONNECTED:
            QMessageBox::warning(this, tr("Warning"), tr("DataBase is Disconnected!"));
            return;
            break;
        case SQLite::CONNECTED:
            qDebug()<<"DataBase is Connected";
            //QMessageBox::warning(this, tr("Warning"), tr("DataBase is Connected"));
            break;
        }

        pauseResumeReplayButton->setEnabled(true);
        pauseResumeReplayButton->setIcon(QIcon(":/icons/images/pause.png"));
        pauseResumeReplayButton->setToolTip(tr("Pause Replay"));
        emit replayFileLoaded();
        emit pressPlayAgain();
        startReplayButton->setEnabled(true);
        previousFrameButton->setEnabled(true);
        nextFrameButton->setEnabled(true);
        // qDebug() << "Recording Replay Again:" << filePath;
    });

    connect(startReplayButton, &QToolButton::clicked, this, [this]() {
        emit getDBStatus();
        if(!dbStatusPtr || dbStatusPtr == nullptr){
            QMessageBox::warning(this, tr("Warning"), tr("DataBase Pointer Not Defined!"));
            return;
        }
        switch(*dbStatusPtr){
        case SQLite::DISCONNECTED:
            QMessageBox::warning(this, tr("Warning"), tr("DataBase is Disconnected!"));
            return;
            break;
        case SQLite::CONNECTED:
            qDebug()<<"DataBase is Connected";
            //QMessageBox::warning(this, tr("Warning"), tr("DataBase is Connected"));
            break;
        }
        inspectRecorder();
        timelineWidget->setValues(
            loggerMode     ,
            bookmarkDblPtr,
            durationDblPtr);
        emit replayStart();

        //loggerStatusLabel->setText(tr("Replaying"));
        //Enableing buttton Start
        loggerStatusLabel->setStyleSheet("font-weight: bold; color: #27ae60;");
        startReplayButton->setEnabled(false);
        pauseResumeReplayButton->setEnabled(true);
        previousFrameButton->    setEnabled(true);
        nextFrameButton->        setEnabled(true);
        //Enableing buttton End

        pauseResumeReplayButton->setIcon(QIcon(":/icons/images/pause.png"));
        pauseResumeReplayButton->setToolTip(tr("Pause Replay"));
        // qDebug() << "Replay started - file path:" << filePath;
    });

    connect(pauseResumeReplayButton, &QToolButton::clicked, this, [this]() {
        //emit toggleReplayPause();   // Call merged replay function
        if (! (*replayModePtr == Replay::replayModes::PAUSE)) {
            // UI Updates for Pause
            //loggerStatusLabel->setText(tr("Replay Paused"));
            loggerStatusLabel->setStyleSheet("font-weight: bold; color: #f39c12;");
            pauseResumeReplayButton->setIcon(QIcon(":/icons/images/resume.png"));
            pauseResumeReplayButton->setToolTip(tr("Resume Replay"));
            emit replayPause();
        } else {
            // UI Updates for Resume
            //loggerStatusLabel->setText(tr("Replaying"));
            loggerStatusLabel->setStyleSheet("font-weight: bold; color: #27ae60;");
            pauseResumeReplayButton->setIcon(QIcon(":/icons/images/pause.png"));
            pauseResumeReplayButton->setToolTip(tr("Pause Replay"));
            emit replayResume();
        }
    });

    connect(previousFrameButton, &QToolButton::clicked, this, [this]() {
        emit previousFrame();
        //qDebug() << "Previous Frame requested";
    });

    connect(nextFrameButton, &QToolButton::clicked, this, [this]() {
        emit nextFrame();
        //qDebug() << "Next Frame requested";
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
        // qDebug() << "Bookmark clicked: Note =" << note << ", Timestamp =" << timestampMs << "ms";
    });

    connect(databaseButton, &QToolButton::clicked,this, [this]() {
        emit dbInit();
    });
    connect(databaseButtonReplay, &QToolButton::clicked,this, [this]() {
        emit dbConnect();
    });

    qDebug() << "Logger is setting up";
}


void LoggerDialog::loggerModeChange(Recorder::loggerModes mode)
{
    setRecorder();

    modeOfLogger = mode;
    switch(modeOfLogger){
    case Recorder::RECORDING:
        debugString = "RECORDING";

        break;
    case Recorder::REPLAY:
        debugString = "REPLAY";
        recordingDateLabel->setText("No recording");

        break;
    default:
        break;
    }
    // qDebug()<<"Logger : Logger mode is changed"<<debugString;
    recorderInfo();
    emit loggerModeSend(modeOfLogger);
    if(!timelineWidget){
        timelineWidget->setValues(
            loggerMode     ,
            bookmarkDblPtr,
            durationDblPtr);
    }
    inspectRecorder();
}

void LoggerDialog::recorderInfo()
{


    int seconds = duration / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    QString durationText = QString("%1:%2:%3")
                               .arg(hours, 2, 10, QLatin1Char('0'))
                               .arg(minutes % 60, 2, 10, QLatin1Char('0'))
                               .arg(seconds % 60, 2, 10, QLatin1Char('0'));


}



void LoggerDialog::recorderInfoReceive(Recorder::LoggerStatusModes r_loggerStatus)
{
    loggerStatus = r_loggerStatus;
    recorderInfo_Update(loggerStatus);
    if(loggerStatus == Recorder::S_REPLAY_STOPPED){
        pauseResumeReplayButton->setIcon(QIcon(":/icons/images/resume.png"));
        pauseResumeReplayButton->setEnabled(true);
        startReplayButton->setEnabled(false);
        //:/icons/images/play.png
        isReplayPaused = true;
    }
    if(loggerStatus == Recorder::S_REPLAYING){
        pauseResumeReplayButton->setIcon(QIcon(":/icons/images/pause.png"));
        pauseResumeReplayButton->setEnabled(true);
        startReplayButton->setEnabled(false);
    }
    if(loggerStatus == Recorder::S_REPLAY_LOADED){
        startReplayButton->setEnabled(true);
        pauseResumeReplayButton->setEnabled(false);
    }
}

void LoggerDialog::recorderInfoReceiveOnce(
    QDateTime r_recordingStartTime,
    qint64 r_duration,
    Recorder::LoggerStatusModes     r_loggerStatus,
    Recorder::SimulationStatusModes r_simulationStatus)
{
    recordingStartTime = r_recordingStartTime;
    duration           = r_duration          ;
    loggerStatus       = r_loggerStatus      ;
    simulationStatus   = r_simulationStatus  ;
    recorderInfoUpdate(r_recordingStartTime,
                       r_duration,
                       r_loggerStatus,
                       r_simulationStatus);
    recorderInfo();
}
void LoggerDialog::recorderInfoReceiveUsual(
    qint64    r_duration,
    Recorder::LoggerStatusModes       r_loggerStatus,
    Recorder::SimulationStatusModes   r_simulationStatus)
{
    duration         = r_duration;
    loggerStatus     = r_loggerStatus;
    simulationStatus = r_simulationStatus;
    recorderInfoUpdateDuration(r_duration);
    recorderInfoUpdateLoggerStatus(r_loggerStatus);
    recorderInfoUpdateSimulationStatus(r_simulationStatus);
    recorderInfo();
}
void LoggerDialog::recorderInfoReceiveDuration(qint64 r_duration)
{
    duration = r_duration;
    recorderInfoUpdateDuration(r_duration);
    recorderInfo();
}

void LoggerDialog::recorderInfoUpdate(
    QDateTime r_recordingStartTime,
    qint64 r_duration,
    Recorder::LoggerStatusModes r_loggerStatus,
    Recorder::SimulationStatusModes r_simulationStatus)
{
    recorderInfoUpdateRecordingStartTime(r_recordingStartTime);
    recorderInfoUpdateDuration(r_duration);
    recorderInfoUpdateLoggerStatus(r_loggerStatus);
    recorderInfoUpdateSimulationStatus(r_simulationStatus);
}

void LoggerDialog::recorderInfo_Update(Recorder::LoggerStatusModes r_loggerStatus)
{
    loggerStatusLabel->    setText(loggerStatusModeString[r_loggerStatus]);
    // qDebug()<<"Logger : Recorder Info Update Recording Logger Status";
}

void LoggerDialog::recorderInfoUpdateRecordingStartTime(
    QDateTime r_recordingStartTime)
{
    recordingDateLabel->setText(r_recordingStartTime.toString("yyyy-MM-dd hh:mm:ss"));
    // qDebug()<<"Logger : Recorder InfoUpdate Recording Start Time is Called"
    //         <<"\n\t"<<r_recordingStartTime.toString("hh:mm:ss");
}

void LoggerDialog::recorderInfoUpdateDuration(
    qint64 r_duration)
{
    updateRecordingDuration(r_duration);
    // qDebug()<<"Logger : Recorder InfoUpdate Recording Start Time is Called"
    //          <<"\n\t"<<r_duration;
}
void LoggerDialog::updateRecordingDuration(qint64 durationMs)
{
    if (timelineWidget) {
        timelineWidget->setRecordingDuration(durationMs);

        int seconds = durationMs / 1000;
        int minutes = seconds / 60;
        int hours = minutes / 60;
        QString durationText = QString("%1:%2:%3")
                                   .arg(hours, 2, 10, QLatin1Char('0'))
                                   .arg(minutes % 60, 2, 10, QLatin1Char('0'))
                                   .arg(seconds % 60, 2, 10, QLatin1Char('0'));
        durationLabel->setText(durationText);
    }
}

void LoggerDialog::recorderInfoUpdateLoggerStatus(
    Recorder::LoggerStatusModes r_loggerStatus)
{
    loggerStatusLabel->    setText(loggerStatusModeString[r_loggerStatus]);
    // qDebug()<<"Logger : Recorder Info Update Recording Logger Status";
}

void LoggerDialog::recorderInfoUpdateSimulationStatus(
    Recorder::SimulationStatusModes r_simulationStatus)
{
    simulationStatusLabel->setText(SimulationStatusModeString[r_simulationStatus]);
    // qDebug()<<"Logger : Recorder Info Update Recording Simulation Status";
}


void LoggerDialog::showBookmarkDialog()
{
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

            }

            bookmarkDialog.accept();
        } else {
            QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter a bookmark comment."));
        }
    });

    bookmarkDialog.exec();
}

void LoggerDialog::switchToRecordingMode()
{

    loggerModeChange(Recorder::RECORDING);
    inspectRecorder();

    timelineWidget->replayMode = false;
    timelineWidget->modeisRecording = true;
    //timelineWidget->toRunInRecording = true;
    //loggerStatusLabel->setText(tr("Recording Mode"));
    loggerStatusLabel->setStyleSheet("font-weight: bold; color: #e74c3c;");
    timelineWidget->clearBookmarks();
    timelineWidget->setRecordingDuration(0);
    updateRecordingDurationLabel(0);

    emit requestReplayReset();
    qDebug() << "Switched to Recording Mode";
    if(timelineWidget){
        timelineWidget->setValues(
            loggerMode     ,
            bookmarkDblPtr,
            durationDblPtr);
    }
}

void LoggerDialog::switchToReplayMode()
{
    //Defining the mode of Logger

    loggerModeChange(Recorder::REPLAY);
    inspectRecorder();
    timelineWidget->replayMode = true;
    timelineWidget->modeisRecording = false;
    //loggerStatusLabel->setText(tr("Replay Mode"));
    loggerStatusLabel->setStyleSheet("font-weight: bold; color: #27ae60;");
    timelineWidget->clearBookmarks();
    timelineWidget->setRecordingDuration(0);
    updateRecordingDurationLabel(0);
    emit requestReplayReset();
    if(timelineWidget){
        timelineWidget->setValues(
            loggerMode    ,
            bookmarkDblPtr,
            durationDblPtr);
    }

}



void LoggerDialog::updateRecordingsList()
{
}


void LoggerDialog::updateRecordingDurationLabel(qint64 durationMs)
{
    if (timelineWidget) {

        int seconds = durationMs / 1000;
        int minutes = seconds / 60;
        int hours = minutes / 60;
        QString durationText = QString("%1:%2:%3")
                                   .arg(hours, 2, 10, QLatin1Char('0'))
                                   .arg(minutes % 60, 2, 10, QLatin1Char('0'))
                                   .arg(seconds % 60, 2, 10, QLatin1Char('0'));
        durationLabel->setText(durationText);
    }
}






void LoggerDialog::addBookmarkWithTimestamp(const QString &note, qint64 timestampMs)
{
    if (timestampCheckBox->isChecked()) {
        timelineWidget->addBookmark(note, timestampMs);
        // qDebug()<<"****add bookmark****";
    }
}

void LoggerDialog::showBookmarkOnReplay(const QString& note, qint64 timestamp)
{
    if (timelineWidget) {
        timelineWidget->addBookmark(note, timestamp);
    }
}


void LoggerDialog::onReplayBookmarkLoaded(const QString& note, qint64 timestamp)
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

void LoggerDialog::updateReplayProgress(qint64 timestamp)
{
    if (timelineWidget) {
        updateRecordingDurationLabel(timestamp);
        timelineWidget->setCurrentReplayTime(timestamp);
    }
}

void LoggerDialog::replayFromBookmark(const QString& note, qint64 timestamp)
{
}


void LoggerDialog::setC_Duration()
{

}




void LoggerDialog::inspectRecorder()
{
    std::string report = "Inpecter Report:\n";

    //Recorder Start
    if(recorder){
        report += "\tRecorder Exist\n";
    }else{
        report += "\tRecorder Not Exist\n";
        qDebug()<<report.c_str();
        return;
    }
    //Recorder End

    //Mode Start
    qDebug()<<"Inside inspect:"<<recorder->modeOfLogger;
    if(recorder->modeOfLogger == Recorder::loggerModes::RECORDING){
        report += "\tLogger Mode Exist\n";
        report += "\tLogger Mode is: ";
        loggerMode    = recorder->modeOfLogger;
        loggerModePtr = &recorder->modeOfLogger;
        report += "Recording\n";
        report += "\tSet Up Logger Mode Double pointer\n";

        recordingModePtr = &recorder->m_recording->mode;
        report += "Recording Mode\n";
        report += "\tSet Up Recording Mode pointer\n";
        qDebug()<<*recordingModePtr;
    }else if(recorder->modeOfLogger == Recorder::loggerModes::REPLAY){
        report += "\tLogger Mode Exist\n";
        report += "\tLogger Mode is: ";
        loggerMode    = recorder->modeOfLogger;
        loggerModePtr = &recorder->modeOfLogger;
        report += "Replay\n";
        report += "\tSet Up Logger Mode Double pointer\n";

        replayModePtr = &recorder->m_replay->mode;
        report += "Replay Mode\n";
        report += "\tSet Up Replay Mode pointer\n";
        qDebug()<<*replayModePtr;
    }
    else{
        report += "\tLogger Mode Exist Not Exist\n";
        qDebug()<<report.c_str();
        return;
    }


    //Mode End

    if(recorder->durationPtr){
        report += "\tDuration Exist\n";
    }else if(recorder->durationPtr == nullptr){
        report += "\tDuration is Nullptr\n";
    }else{
        report += "\tDuration Not Exist\n";
        qDebug()<<report.c_str();
        return;
    }

    if(recorder->durationPtr != nullptr){
        report += "\tDuration is Have Value: ";
        durationPtr = recorder->durationPtr;
        report += qintToTime(*durationPtr);
        report += "\n";
    }
    if(recorder->durationPtr != nullptr){
        report += "\tDuration Double Pointer is Have Value: ";
        durationDblPtr = &recorder->durationPtr;
        report += qintToTime(**durationDblPtr);
        report += "\n";
    }

    // Left Right Timer Start
    if(recorder->leftTimer && recorder->rightTimer){
        report += "\tLeft & Right Timer Exist\n";
    }else{
        report += "\tLeft & Right Timer Not Exist\n";
        qDebug()<<report.c_str();
        return;
    }

    if(recorder->leftTimer != nullptr || recorder->rightTimer != nullptr){
        leftTimer  = recorder->leftTimer;
        rightTimer = recorder->rightTimer;
        report += "\tTimer has Value:-> Left: ";
        report += qintToTime(*leftTimer);
        report += " Right: ";
        report += qintToTime(*rightTimer);
        report += "\n";
    }else{
        report += "\tLeft & Right Timer is Nullptr\n";
    }

    if(recorder->leftTimer != nullptr || recorder->rightTimer != nullptr){
        leftTimerDblPtr  = &recorder->leftTimer;
        rightTimerDblPtr = &recorder->rightTimer;
        report += "\tSet up Double pointer in Timer has Value:-> Left: ";
        report += qintToTime(**leftTimerDblPtr);
        report += " Right: ";
        report += qintToTime(**leftTimerDblPtr);
        report += "\n";
    }else{
        report += "\tLeft & Right Timer is Nullptr\n";
    }
    // Left Right Timer Start

    // Bookmark Start
    if(recorder->bookmarks){
        report += "\tBookmark Exist\n";
    }else{
        report += "\tBookmark Not Exist\n";
        qDebug()<<report.c_str();
        return;
    }

    if(recorder->bookmarks != nullptr){
        report += "\tBookmark Has Values:->";
        bookmarks = recorder->bookmarks;
        for(auto i = bookmarks->begin(); i != bookmarks->end(); i++){
            report += " (t:";
            report += qintToTime(i->second);
            report += ", m:";
            report += i->first.toStdString();
            report += "),";
        }
        report += "\n";
    }else{
        report += "\tBookmark is Nullptr\n";
    }

    if(recorder->bookmarks != nullptr){
        report += "\tSet Up Bookmark Double Pointer Has Values:->";
        bookmarkDblPtr = &recorder->bookmarks;
        QList<QPair<QString, qint64>>* tempbookmarks = *bookmarkDblPtr;
        for(auto i = tempbookmarks->begin(); i != tempbookmarks->end(); i++){
            report += " (t:";
            report += qintToTime(i->second);
            report += ", m:";
            report += i->first.toStdString();
            report += "),";
        }
        report += "\n";
    }else{
        report += "\tBookmark is Nullptr\n";
    }
    // Bookmark End

    qDebug()<<report.c_str();
}

void LoggerDialog::setRecorder()
{
    qDebug()<<"Logger Recorder is set";
    if(recorder == nullptr){
        emit getRecorder();
    }

}

// void LoggerDialog::receiveRecorder(Recorder &r_recoder)
// {
//     recorder = &r_recoder;
// }

void LoggerDialog::updateDuration()
{

    int seconds = **(durationDblPtr) / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    QString durationText = QString("%1:%2:%3")
                               .arg(hours, 2, 10, QLatin1Char('0'))
                               .arg(minutes % 60, 2, 10, QLatin1Char('0'))
                               .arg(seconds % 60, 2, 10, QLatin1Char('0'));
    //qDebug()<<durationText;
    if(timelineWidget){
        timelineWidget->updateTimelineWidget();
    }
    durationLabel->setText(durationText);
}


std::string LoggerDialog::qintToTime(qint64 m_duration)
{
    int seconds = m_duration / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    QString durationText = QString("%1:%2:%3")
                               .arg(hours, 2, 10, QLatin1Char('0'))
                               .arg(minutes % 60, 2, 10, QLatin1Char('0'))
                               .arg(seconds % 60, 2, 10, QLatin1Char('0'));
    std::string stdString = durationText.toStdString();
    return stdString;
}


