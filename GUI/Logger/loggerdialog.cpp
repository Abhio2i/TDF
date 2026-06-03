#include "loggerdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QMenuBar>
#include <QTabWidget>
#include <QFrame>
#include <QTimer>
#include "core/Recorder/recorder.h"
#include <QWidgetAction>

// ═══════════════════════════════════════════════════════
//  Dark stylesheet for the whole dialog
// ═══════════════════════════════════════════════════════
static const char* kDarkStyle = R"(
QMainWindow, QWidget {
    background-color: #1a2a3a;
    color: #ccd6e0;
    font-family: 'Segoe UI', Arial, sans-serif;
    font-size: 13px;

}
QTabWidget::pane  { border: 0px; background: transparent; }
QTabWidget::tab-bar { alignment: center; }
QTabBar::tab {
    background: transparent;
    color: #7ec8e3;
    padding: 5px 18px;
    border: none;
    border-bottom: 2px solid transparent;
}

QTabBar::tab:selected {
    color: #ffffff;
    border-bottom: 2px solid #3498db;
}

QTabBar::tab:hover:!selected { color: #aaddee; }
QToolButton {
    background: #1e3a4f;
    border: 1px solid #2c4a5e;
    border-radius: 5px;
    padding: 2px;
}
QToolButton:hover   { background: #254d66; border-color: #3a7ca5; }
QToolButton:pressed { background: #1a3348; }
QToolButton:disabled { background: #162330; border-color: #1e2e3a; opacity: 0.5; }
QLabel  { color: #90a4ae; padding: 0px; }
QLabel#statusLabel { font-weight: bold; }
QCheckBox { color: #90a4ae; }
QCheckBox::indicator { width:14px; height:14px; border:1px solid #3a5a70; border-radius:3px; background:#162330; }
QCheckBox::indicator:checked { background:#3498db; border-color:#3498db; }
QFrame#separator { background: #2c4a5e; }
)";

// ═══════════════════════════════════════════════════════
//  Helper: make a tool button
// ═══════════════════════════════════════════════════════
QToolButton* LoggerDialog::makeBtn(const QString &iconPath, const QString &tip, int size)
{
    QToolButton *b = new QToolButton(this);
    if (!iconPath.isEmpty()) {
        b->setIcon(QIcon(iconPath));
        b->setIconSize(QSize(size-6, size-6));
    }
    b->setToolTip(tip);
    b->setFixedSize(size, size);
    return b;
}

// ═══════════════════════════════════════════════════════
//  Constructor
// ═══════════════════════════════════════════════════════
LoggerDialog::LoggerDialog(QWidget *parent, Recorder *recorderParam)
    : QMainWindow(parent), recorder(recorderParam)
{
    setWindowTitle(tr("Logger"));
    setAttribute(Qt::WA_DeleteOnClose);
    recordingsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recordings";
    setMinimumSize(320, 480);

    loggerStatusModeString[Recorder::S_RECORDING_MODE]   = "Ready";
    loggerStatusModeString[Recorder::S_RECORDING]        = "Recording";
    loggerStatusModeString[Recorder::S_RECORDING_PAUSED] = "Paused";
    loggerStatusModeString[Recorder::S_RECORDING_STOPPED]= "Stopped";
    loggerStatusModeString[Recorder::S_REPLAY_MODE]      = "Replay Ready";
    loggerStatusModeString[Recorder::S_REPLAY_LOADED]    = "Loaded";
    loggerStatusModeString[Recorder::S_REPLAY_UNLOADED]  = "Unloaded";
    loggerStatusModeString[Recorder::S_REPLAYING]        = "Replaying";
    loggerStatusModeString[Recorder::S_REPLAY_PAUSED]    = "Paused";
    loggerStatusModeString[Recorder::S_REPLAY_STOPPED]   = "Stopped";

    SimulationStatusModeString[Recorder::S_SIMULATION_START]  = "Start";
    SimulationStatusModeString[Recorder::S_SIMULATION_PAUSED] = "Paused";
    SimulationStatusModeString[Recorder::S_SIMULATION_STOP]   = "Stop";
    SimulationStatusModeString[Recorder::S_SIMULATION_NA]     = "Not Available";

    setStyleSheet(kDarkStyle);
    setupUi();
}

// ═══════════════════════════════════════════════════════
//  setupUi
// ═══════════════════════════════════════════════════════
void LoggerDialog::setupUi()
{
    QScrollArea *outerScroll = new QScrollArea(this);
    outerScroll->setWidgetResizable(true);
    outerScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    outerScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    outerScroll->setFrameShape(QFrame::NoFrame);
    outerScroll->setStyleSheet(
        "QScrollArea          { background:#1a2a3a; border:none; }"
        "QScrollBar:vertical  { background:#1a2a3a; width:6px; border:none; }"
        "QScrollBar::handle:vertical { background:#2c4a5e; border-radius:3px; min-height:20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0px; }");
    setCentralWidget(outerScroll);

    centralWidget = new QWidget(this);
    centralWidget->setStyleSheet(
        "QWidget#outerContainer { border: 2px solid #27446d; border-radius: 2px; }");
    centralWidget->setObjectName("outerContainer");
    centralWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    outerScroll->setWidget(centralWidget);

    QVBoxLayout *main = new QVBoxLayout(centralWidget);
    main->setSpacing(8);
    main->setContentsMargins(10, 8, 10, 10);

    // ── Tab widget ────────────────────────────────────────────────────────
    modeTabWidget = new QTabWidget(this);
    modeTabWidget->setDocumentMode(true);
    modeTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // ══════════════════════════════════
    //  RECORDING TAB
    // ══════════════════════════════════
    QWidget *recTab = new QWidget();
    QVBoxLayout *recTabLay = new QVBoxLayout(recTab);
    recTabLay->setContentsMargins(0, 6, 0, 0);
    recTabLay->setSpacing(4);

    // ── Row 1: control buttons ────────────────────────────────────────────
    QHBoxLayout *recCtrl = new QHBoxLayout();
    recCtrl->setSpacing(6);

    recordButton = new QToolButton(this);
    recordButton->setFixedSize(32, 32);
    recordButton->setStyleSheet(
        "QToolButton { background:#c0392b; border-radius:16px; border:none; }"
        "QToolButton:hover { background:#e74c3c; }"
        "QToolButton:pressed { background:#962d22; }");
    recordButton->setToolTip(tr("Start Recording"));
    QLabel *recDot = new QLabel(recordButton);
    recDot->setFixedSize(12, 12);
    recDot->setStyleSheet("background:white; border-radius:6px;");
    recDot->move(10, 10);

    recPlayPauseButton = makeBtn(":/icons/images/pause.png", tr("Pause Recording"));
    recPlayPauseButton->setEnabled(false);

    stopRecordingButton = makeBtn(":/icons/images/stop.png", tr("Stop Recording"));
    stopRecordingButton->setEnabled(false);

    bookmarkButton = makeBtn(":/icons/images/star.png", tr("Add Bookmark"));
    bookmarkButton->setEnabled(false);

    recCtrl->addWidget(recordButton);
    recCtrl->addWidget(recPlayPauseButton);
    recCtrl->addWidget(stopRecordingButton);
    recCtrl->addWidget(bookmarkButton);
    recCtrl->addStretch();

    timestampCheckBox = new QCheckBox(tr("Timestamp"), this);
    timestampCheckBox->setChecked(true);
    recCtrl->addWidget(timestampCheckBox);
    recTabLay->addLayout(recCtrl);

    // ── Row 2: Filter dropdown button ─────────────────────────────────────
    QHBoxLayout* filterRow = new QHBoxLayout();
    filterRow->setContentsMargins(0, 2, 0, 2);
    filterRow->setSpacing(4);

    m_filterBtn = new QToolButton(this);
    m_filterBtn->setText(tr("Record Filters ▾"));
    m_filterBtn->setToolTip(tr("Select what data to record"));
    m_filterBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_filterBtn->setPopupMode(QToolButton::InstantPopup);
    m_filterBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_filterBtn->setStyleSheet(
        "QToolButton {"
        "  background:#162330;"
        "  border:1px solid #2c4a5e;"
        "  border-radius:4px;"
        "  color:#7ec8e3;"
        "  padding:3px 10px;"
        "  font-size:11px;"
        "}"
        "QToolButton:hover  { background:#1e3a4f; border-color:#3a7ca5; }"
        "QToolButton:pressed{ background:#0f1e2b; }"
        "QToolButton::menu-indicator { width:0px; }");   // hide default arrow
    buildFilterMenu();
    m_filterBtn->setMenu(m_filterMenu);

    filterRow->addWidget(m_filterBtn);
    filterRow->addStretch();
    recTabLay->addLayout(filterRow);

    // backward compat aliases
    pauseRecordingButton = recPlayPauseButton;
    databaseButton       = makeBtn("",""); databaseButton->hide();
    modeTabWidget->addTab(recTab, tr("Recording"));

    // ══════════════════════════════════
    //  REPLAY TAB
    // ══════════════════════════════════
    QWidget *repTab = new QWidget();
    QVBoxLayout *repTabLay = new QVBoxLayout(repTab);
    repTabLay->setContentsMargins(0, 6, 0, 0);
    repTabLay->setSpacing(6);

    QHBoxLayout *repCtrl = new QHBoxLayout();
    repCtrl->setSpacing(6);

    repPlayPauseButton = makeBtn(":/icons/images/play.png", tr("Load & Play"));
    repPlayPauseButton->setFixedSize(32,32);
    repPlayPauseButton->setStyleSheet(
        "QToolButton{background:#1e3a4f;border:1px solid #27ae60;border-radius:5px;}"
        "QToolButton:hover{background:#254d66;}"
        "QToolButton:disabled{border-color:#2c4a5e;}");

    reloadReplayButton = makeBtn(":/icons/images/loading-arrow.png", tr("Replay Again"));
    reloadReplayButton->setEnabled(false);

    loadNewFileButton = makeBtn(":/icons/images/folder1.png", tr("Load New File"));
    loadNewFileButton->setFixedSize(28,28);
    loadNewFileButton->setStyleSheet(
        "QToolButton{background:#1e3a4f;border:1px solid #3498db;border-radius:5px;}"
        "QToolButton:hover{background:#254d66;}"
        "QToolButton:disabled{border-color:#2c4a5e;}");
    loadNewFileButton->setEnabled(true);

    repCtrl->addWidget(repPlayPauseButton);
    repCtrl->addWidget(reloadReplayButton);
    repCtrl->addWidget(loadNewFileButton);
    repCtrl->addStretch();

    dbStatusLabel = new QLabel(tr("⬤  DB"), this);
    dbStatusLabel->setStyleSheet("color:#607d8b; font-size:11px;");
    repCtrl->addWidget(dbStatusLabel);
    repTabLay->addLayout(repCtrl);

    fileNameLabel = new QLabel(tr("No file loaded"), this);
    fileNameLabel->setStyleSheet("color:#607d8b; font-size:11px;");
    repTabLay->addWidget(fileNameLabel);

    startReplayButton       = repPlayPauseButton;
    pauseResumeReplayButton = repPlayPauseButton;
    loadRecordingButton     = reloadReplayButton;
    databaseButtonReplay    = makeBtn("",""); databaseButtonReplay->hide();

    modeTabWidget->addTab(repTab, tr("Replay"));
    main->addWidget(modeTabWidget);

    // ── Separator ────────────────────────────────────────────────────────
    QFrame *sep = new QFrame(this);
    sep->setObjectName("separator");
    sep->setFrameShape(QFrame::HLine);
    sep->setFixedHeight(1);
    main->addWidget(sep);

    // ── Info panel ───────────────────────────────────────────────────────
    QWidget *infoW = new QWidget(this);
    QGridLayout *infoGrid = new QGridLayout(infoW);
    infoGrid->setContentsMargins(0,4,0,4);
    infoGrid->setSpacing(4);

    auto addRow = [&](int row, const QString &lbl, QLabel *&out) {
        QLabel *l = new QLabel(lbl, this);
        l->setStyleSheet("color:#607d8b; font-size:12px;");
        out = new QLabel("—", this);
        out->setObjectName("statusLabel");
        out->setStyleSheet("color:#90a4ae; font-size:12px;");
        out->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        infoGrid->addWidget(l,   row, 0);
        infoGrid->addWidget(out, row, 1);
    };

    recordingDateLabel = new QLabel("", this); recordingDateLabel->hide();
    addRow(0, tr("Duration"),   durationLabel);
    addRow(1, tr("Status"),     loggerStatusLabel);
    // addRow(2, tr("Simulation"), simulationStatusLabel);
    durationLabel->setText("00:00:00");
    loggerStatusLabel->setText("Idle");
    // simulationStatusLabel->setText("Not Available");
    main->addWidget(infoW);

    QFrame *sep2 = new QFrame(this);
    sep2->setObjectName("separator");
    sep2->setFrameShape(QFrame::HLine);
    sep2->setFixedHeight(1);
    main->addWidget(sep2);

    // ── Timeline ─────────────────────────────────────────────────────────
    timelineWidget = new TimelineWidget(this);
    timelineWidget->setVisible(true);
    main->addWidget(timelineWidget);

    // ── Bookmark list ────────────────────────────────────────────────────
    m_bookmarkList   = new BookmarkListWidget(this);
    m_bookmarkScroll = new QScrollArea(this);
    m_bookmarkScroll->setWidget(m_bookmarkList);
    m_bookmarkScroll->setWidgetResizable(true);
    m_bookmarkScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_bookmarkScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_bookmarkScroll->setFixedHeight(0);
    m_bookmarkScroll->setVisible(false);
    m_bookmarkScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_bookmarkScroll->setStyleSheet(
        "QScrollArea{background:#0f1e2b;border:none;}"
        "QScrollBar:vertical{background:#0f1e2b;width:6px;border:none;}"
        "QScrollBar::handle:vertical{background:#2c4a5e;border-radius:3px;min-height:20px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0px;}");
    main->addWidget(m_bookmarkScroll);
    m_bookmarkList->setScrollArea(m_bookmarkScroll);
    timelineWidget->setListWidget(m_bookmarkList);
    main->addStretch();

    // ── Button map ───────────────────────────────────────────────────────
    loggerButtonMap = {
        {Recorder_Button,  &recordButton},
        {Recording_Toggle, &recPlayPauseButton},
        {Reocrding_Stop,   &stopRecordingButton},
        {Replay_Start,     &repPlayPauseButton},
        {Replay_Toggle,    &pauseResumeReplayButton},
        {Replay_Restart,   &reloadReplayButton}
    };

    setupConnections();
}// ═══════════════════════════════════════════════════════
//  setupConnections
// ═══════════════════════════════════════════════════════
void LoggerDialog::setupConnections()
{
    // ── Tab switch ──
    connect(modeTabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        bool busy = (loggerStatus==Recorder::S_RECORDING)||(loggerStatus==Recorder::S_REPLAYING);
        if (busy) {
            QSignalBlocker b(modeTabWidget);
            modeTabWidget->setCurrentIndex(index==0?1:0);
            return;
        }
        index==0 ? switchToRecordingMode() : switchToReplayMode();
    });
    // ── Record button ──
    connect(recordButton, &QToolButton::clicked, this, [this]() {
        inspectRecorder();
        recordingStartTime = QDateTime::currentDateTime();
        recordingDateLabel->setText(recordingStartTime.toString("yyyy-MM-dd hh:mm:ss"));
        loggerStatusLabel->setStyleSheet("font-weight:bold; color:#e74c3c;");
        if (!saveFile()) return;
        applyFiltersToRecording();
        if (m_filterBtn) m_filterBtn->setEnabled(false);
        emit recordingStart(*recorder);
        inspectRecorder();
        timelineWidget->setValues(loggerMode, bookmarkDblPtr, durationDblPtr);
        timelineWidget->startUpdateUI();
        recPlayPauseButton->setEnabled(true);
        stopRecordingButton->setEnabled(true);
        bookmarkButton->setEnabled(true);
        isRecordingPaused = false;
        recPlayPauseButton->setIcon(QIcon(":/icons/images/pause.png"));
        recPlayPauseButton->setToolTip(tr("Pause Recording"));
    });
    // ── Rec pause/resume (single button) ──
    connect(recPlayPauseButton, &QToolButton::clicked, this, [this]() {
        if (!isRecordingPaused) {
            loggerStatusLabel->setStyleSheet("font-weight:bold; color:#f39c12;");
            recPlayPauseButton->setIcon(QIcon(":/icons/images/play.png"));
            recPlayPauseButton->setToolTip(tr("Resume Recording"));
            isRecordingPaused = true;
            emit recordingPause();
            timelineWidget->pause();
        } else {
            loggerStatusLabel->setStyleSheet("font-weight:bold; color:#e74c3c;");
            recPlayPauseButton->setIcon(QIcon(":/icons/images/pause.png"));
            recPlayPauseButton->setToolTip(tr("Pause Recording"));
            isRecordingPaused = false;
            emit recordingResume();
            timelineWidget->resume();
        }
    });

    // ── Stop recording ──
    connect(stopRecordingButton, &QToolButton::clicked, this, [this]() {
        if (!recordingStartTime.isValid()) return;
        emit recordingStop();
        timelineWidget->stop();
        recPlayPauseButton->setEnabled(false);
        stopRecordingButton->setEnabled(false);
        bookmarkButton->setEnabled(false);
        loggerStatusLabel->setStyleSheet("color:#607d8b;");
        recordingStartTime = QDateTime();
        isRecordingPaused  = false;
        recPlayPauseButton->setIcon(QIcon(":/icons/images/pause.png"));
        recPlayPauseButton->setToolTip(tr("Pause Recording"));
        if (m_filterBtn) m_filterBtn->setEnabled(true);
        emit dbDisconnectRequested();
         recordButton->setEnabled(true);

    });

    // ── Bookmark ──
    connect(bookmarkButton, &QToolButton::clicked, this, [this]() {
        // inline input - no dialog
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Bookmark"));
        dlg.setStyleSheet("background:#1a2a3a; color:#ccd6e0;");
        QVBoxLayout *lay = new QVBoxLayout(&dlg);
        QLineEdit *ed = new QLineEdit(&dlg);
        ed->setPlaceholderText(tr("Note (optional)"));
        ed->setStyleSheet("background:#0f1e2b; color:#ccd6e0; border:1px solid #2c4a5e; border-radius:4px; padding:4px;");
        QPushButton *ok = new QPushButton(tr("Add"), &dlg);
        ok->setStyleSheet("background:#3498db; color:white; border:none; border-radius:4px; padding:6px 16px;");
        lay->addWidget(ed);
        lay->addWidget(ok);
        connect(ok, &QPushButton::clicked, &dlg, [&]{ dlg.accept(); });
        ed->setFocus();
        if (dlg.exec() == QDialog::Accepted) {
            QString note = ed->text().trimmed();
            if (note.isEmpty()) note = tr("Bookmark");
            emit bookmarkAdded(note);
        }
    });

    // ── Timestamp checkbox ──
    connect(timestampCheckBox, &QCheckBox::stateChanged, this, [this](int st) {
        bool on = (st == Qt::Checked);
        emit timestampToggled(on);
        if (!on) { timelineWidget->setRecordingDuration(0); timelineWidget->clearBookmarks(); }
    });

    // ── Replay: single play/pause button ──
    connect(repPlayPauseButton, &QToolButton::clicked, this, [this]() {
        if (!repPlaying) {

            findFile();
        } else {
            if (!replayModePtr) return;
            if (*replayModePtr != Replay::replayModes::PAUSE) {
                loggerStatusLabel->setStyleSheet("font-weight:bold; color:#f39c12;");
                repPlayPauseButton->setIcon(QIcon(":/icons/images/play.png"));
                repPlayPauseButton->setToolTip(tr("Resume"));
                repPlayPauseButton->setStyleSheet(
                    "QToolButton{background:#1e3a4f;border:1px solid #f39c12;border-radius:5px;}"
                    "QToolButton:hover{background:#254d66;}");
                emit replayPause();
            } else {
                loggerStatusLabel->setStyleSheet("font-weight:bold; color:#27ae60;");
                repPlayPauseButton->setIcon(QIcon(":/icons/images/pause.png"));
                repPlayPauseButton->setToolTip(tr("Pause"));
                repPlayPauseButton->setStyleSheet(
                    "QToolButton{background:#1e3a4f;border:1px solid #27ae60;border-radius:5px;}"
                    "QToolButton:hover{background:#254d66;}");
                emit replayResume();
            }
        }
    });

    // ── Prev / Next frame ──
    // connect(previousFrameButton, &QToolButton::clicked, this, [this]{ emit previousFrame(); });
    // connect(nextFrameButton,     &QToolButton::clicked, this, [this]{ emit nextFrame();     });
    connect(reloadReplayButton, &QToolButton::clicked, this, [this]() {
        if (!dbStatusPtr || *dbStatusPtr == SQLite::DISCONNECTED) return;
        repPlaying = false;
        m_loadedBookmarkTimestamps.clear();
        timelineWidget->clearBookmarks();
        emit pressPlayAgain();
        _startReplay();
    });

    // ── BookmarkList row click → replay seek ──
    connect(m_bookmarkList, &BookmarkListWidget::jumpToTimestamp,
            this, [this](qint64 t) {
                emit sendClickedTimestamp(t);
            });

    // ── BookmarkList row click → other listeners ──
    connect(m_bookmarkList, &BookmarkListWidget::bookmarkClicked,
            this, [this](const QString &n, qint64 t) {
                emit bookmarkButtonClicked(n, t);
                emit bookmarkClicked(n, t);
            });

    // ── Timeline pin clicks ──
    connect(timelineWidget, &TimelineWidget::bookmarkButtonClicked,
            this, [this](const QString &n, qint64 t){
                emit bookmarkButtonClicked(n, t);
                emit bookmarkClicked(n, t);
            });
    connect(timelineWidget, &TimelineWidget::getMaxDuration,
            this, [this](qint64 &v){ emit getMaxDuration(v); });
    connect(timelineWidget, &TimelineWidget::sendClickedTimestamp,
            this, [this](qint64 t){ emit sendClickedTimestamp(t); });
    connect(loadNewFileButton, &QToolButton::clicked, this, [this]() {
        if (repPlaying) {
            emit replayStop();
            repPlaying = false;
            repPlayPauseButton->setIcon(QIcon(":/icons/images/play.png"));
            repPlayPauseButton->setToolTip(tr("Load & Play"));
            repPlayPauseButton->setStyleSheet(
                "QToolButton{background:#1e3a4f;border:1px solid #27ae60;border-radius:5px;}"
                "QToolButton:hover{background:#254d66;}");
            // previousFrameButton->setEnabled(false);
            // nextFrameButton->setEnabled(false);
            reloadReplayButton->setEnabled(false);
            loggerStatusLabel->setStyleSheet("color:#607d8b;");
            loggerStatusLabel->setText("Replay Ready");
        }

        // Timeline aur bookmarks reset karo
        m_loadedBookmarkTimestamps.clear();
        timelineWidget->clearBookmarks();
        timelineWidget->setRecordingDuration(0);
        updateRecordingDurationLabel(0);
        // File unload signal
        emit replayFileUnloaded();
        findFile();
    });
}

// ═══════════════════════════════════════════════════════
//  Internal: start replay after file loaded
// ═══════════════════════════════════════════════════════
void LoggerDialog::_startReplay()
{
    repPlaying = true;
    inspectRecorder();
    timelineWidget->setValues(loggerMode, bookmarkDblPtr, durationDblPtr);
    emit replayStart();
    loggerStatusLabel->setStyleSheet("font-weight:bold; color:#27ae60;");
    repPlayPauseButton->setIcon(QIcon(":/icons/images/pause.png"));
    repPlayPauseButton->setToolTip(tr("Pause"));
    repPlayPauseButton->setStyleSheet(
        "QToolButton{background:#1e3a4f;border:1px solid #27ae60;border-radius:5px;}"
        "QToolButton:hover{background:#254d66;}");
    // previousFrameButton->setEnabled(true);
    // nextFrameButton->setEnabled(true);
    reloadReplayButton->setEnabled(true);
}

// ═══════════════════════════════════════════════════════
//  findFile  →  emits getFilePath  →  runtimeeditor
//           →  DB auto-connect  →  then start replay
// ═══════════════════════════════════════════════════════

void LoggerDialog::findFile()
{
    emit pauseSimulationRequested();
    QCoreApplication::processEvents();
    QFileDialog dlg(
        this,
        tr("Open Recording File"),
        QDir::homePath() + "/TDF/Recordings",
        tr("Database Files (*.db);;All Files (*)")
        );
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setWindowModality(Qt::ApplicationModal);
    dlg.setWindowFlags(Qt::Dialog
                       | Qt::WindowTitleHint
                       | Qt::WindowCloseButtonHint
                       | Qt::WindowStaysOnTopHint);

    if (parentWidget()) {
        QRect parentRect = parentWidget()->window()->geometry();
        dlg.move(parentRect.center() - dlg.rect().center());
    }
    dlg.show();
    dlg.raise();
    dlg.activateWindow();
    bool accepted = (dlg.exec() == QDialog::Accepted);
    emit resumeSimulationRequested();
    if (!accepted) return;

    QString path = dlg.selectedFiles().value(0);
    if (path.isEmpty()) return;

    QFileInfo fi(path);
    fileNameLabel->setText(fi.fileName());
    fileNameLabel->setStyleSheet("color:#52c78a; font-size:11px;");
    dbStatusLabel->setText(tr("⬤  DB connecting…"));
    dbStatusLabel->setStyleSheet("color:#f39c12; font-size:11px;");

    emit getDBStatus();
    emit getFilePath(path);

    QTimer *checkTimer = new QTimer(this);
    checkTimer->setInterval(100);
    int *attempts = new int(0);

    connect(checkTimer, &QTimer::timeout, this, [this, checkTimer, attempts]() {
        if (!checkTimer) return;
        (*attempts)++;
        emit getDBStatus();
        if (dbStatusPtr && *dbStatusPtr == SQLite::CONNECTED) {
            checkTimer->stop();
            checkTimer->deleteLater();
            delete attempts;
            if (dbStatusLabel)
                dbStatusLabel->setText(tr("⬤  DB connected"));
            if (dbStatusLabel)
                dbStatusLabel->setStyleSheet("color:#27ae60; font-size:11px;");
            _startReplay();
        } else if (*attempts >= 20) {
            checkTimer->stop();
            checkTimer->deleteLater();
            delete attempts;
            if (dbStatusLabel)
                dbStatusLabel->setText(tr("⬤  DB error"));
            if (dbStatusLabel)
                dbStatusLabel->setStyleSheet("color:#e74c3c; font-size:11px;");
        }
    });
    checkTimer->start();
}
// ═══════════════════════════════════════════════════════
//  saveFile
// ═══════════════════════════════════════════════════════
bool LoggerDialog::saveFile()
{
    // Simulation pause karo pehle
    emit pauseSimulationRequested();
    QCoreApplication::processEvents();

    QFileDialog dlg(
        this,
        tr("Create Recording File"),
        QDir::homePath() + "/TDF/Recordings",
        tr("SQLite Database (*.db);;All Files (*)")
        );
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setDefaultSuffix("db");
    dlg.setWindowModality(Qt::ApplicationModal);
    dlg.setWindowFlags(Qt::Dialog
                       | Qt::WindowTitleHint
                       | Qt::WindowCloseButtonHint
                       | Qt::WindowStaysOnTopHint);
    if (parentWidget()) {
        QRect parentRect = parentWidget()->window()->geometry();
        dlg.move(parentRect.center() - dlg.rect().center());
    }
    dlg.show();
    dlg.raise();
    dlg.activateWindow();
    bool accepted = (dlg.exec() == QDialog::Accepted);
    emit resumeSimulationRequested();
    if (!accepted) return false;
    QString path = dlg.selectedFiles().value(0);
    if (path.isEmpty()) return false;
    if (!path.endsWith(".db", Qt::CaseInsensitive))
        path += ".db";
    // TDF/Recordings folder ensure karo
    QDir dir;
    dir.mkpath(QDir::homePath() + "/TDF/Recordings");
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    f.close();
    emit savedFilePath(path);
    return true;
}

// ═══════════════════════════════════════════════════════
//  Mode switches
// ═══════════════════════════════════════════════════════

void LoggerDialog::switchToRecordingMode()
{
    loggerModeChange(Recorder::RECORDING);
    inspectRecorder();
    timelineWidget->replayMode      = false;
    timelineWidget->modeisRecording = true;
    loggerStatusLabel->setStyleSheet("color:#90a4ae;");
    loggerStatusLabel->setText("Ready");
    timelineWidget->clearBookmarks();
    timelineWidget->setRecordingDuration(0);
    updateRecordingDurationLabel(0);
    repPlaying = false;
    emit requestReplayReset();
    if (timelineWidget)
        timelineWidget->setValues(loggerMode, bookmarkDblPtr, durationDblPtr);
}

void LoggerDialog::switchToReplayMode()
{
    loggerModeChange(Recorder::REPLAY);
    inspectRecorder();
    timelineWidget->replayMode      = true;
    timelineWidget->modeisRecording = false;
    loggerStatusLabel->setStyleSheet("color:#90a4ae;");
    loggerStatusLabel->setText("Replay Ready");
    timelineWidget->clearBookmarks();
    timelineWidget->setRecordingDuration(0);
    updateRecordingDurationLabel(0);
    repPlaying = false;
    emit requestReplayReset();
    if (timelineWidget)
        timelineWidget->setValues(loggerMode, bookmarkDblPtr, durationDblPtr);
}

void LoggerDialog::loggerModeChange(Recorder::loggerModes m)
{
    setRecorder();
    modeOfLogger = m;
    debugString  = (m == Recorder::RECORDING) ? "RECORDING" : "REPLAY";
    if (m == Recorder::REPLAY) recordingDateLabel->setText("No recording");
    recorderInfo();
    emit loggerModeSend(modeOfLogger);
    inspectRecorder();
}

// ═══════════════════════════════════════════════════════
//  Recorder info updates
// ═══════════════════════════════════════════════════════
void LoggerDialog::recorderInfo() {}
void LoggerDialog::recorderInfo_Update(Recorder::LoggerStatusModes s)
{
    loggerStatusLabel->setText(loggerStatusModeString[s]);
}

void LoggerDialog::recorderInfoUpdate(QDateTime dt, qint64 dur,
                                      Recorder::LoggerStatusModes ls, Recorder::SimulationStatusModes ss)
{
    recorderInfoUpdateRecordingStartTime(dt);
    recorderInfoUpdateDuration(dur);
    recorderInfoUpdateLoggerStatus(ls);
    // recorderInfoUpdateSimulationStatus(ss);
}

void LoggerDialog::recorderInfoUpdateRecordingStartTime(QDateTime dt)
{ recordingDateLabel->setText(dt.toString("yyyy-MM-dd hh:mm:ss")); }

void LoggerDialog::recorderInfoUpdateDuration(qint64 dur)
{ updateRecordingDuration(dur); }

void LoggerDialog::recorderInfoUpdateLoggerStatus(Recorder::LoggerStatusModes s)
{ loggerStatusLabel->setText(loggerStatusModeString[s]); }

// void LoggerDialog::recorderInfoUpdateSimulationStatus(Recorder::SimulationStatusModes s)
// { simulationStatusLabel->setText(SimulationStatusModeString[s]); }

void LoggerDialog::recorderInfoReceive(Recorder::LoggerStatusModes s)
{
    loggerStatus = s;
    recorderInfo_Update(s);
    if (s == Recorder::S_REPLAY_STOPPED) {
        repPlaying = false;
        repPlayPauseButton->setIcon(QIcon(":/icons/images/play.png"));
        repPlayPauseButton->setToolTip(tr("Load & Play"));
        repPlayPauseButton->setStyleSheet(
            "QToolButton{background:#1e3a4f;border:1px solid #27ae60;border-radius:5px;}"
            "QToolButton:hover{background:#254d66;}");
    }
    if (s == Recorder::S_REPLAYING) {
        repPlayPauseButton->setIcon(QIcon(":/icons/images/pause.png"));
        repPlayPauseButton->setToolTip(tr("Pause"));
    }
}

void LoggerDialog::recorderInfoReceiveOnce(QDateTime dt, qint64 dur,
                                           Recorder::LoggerStatusModes ls, Recorder::SimulationStatusModes ss)
{
    recordingStartTime=dt; duration=dur; loggerStatus=ls; simulationStatus=ss;
    recorderInfoUpdate(dt,dur,ls,ss);
    recorderInfo();
}

void LoggerDialog::recorderInfoReceiveUsual(qint64 dur,
                                            Recorder::LoggerStatusModes ls, Recorder::SimulationStatusModes ss)
{
    duration=dur; loggerStatus=ls; simulationStatus=ss;
    recorderInfoUpdateDuration(dur);
    recorderInfoUpdateLoggerStatus(ls);
    // recorderInfoUpdateSimulationStatus(ss);
}

void LoggerDialog::recorderInfoReceiveDuration(qint64 dur)
{ duration=dur; recorderInfoUpdateDuration(dur); }

void LoggerDialog::updateRecordingDuration(qint64 ms)
{
    if (!timelineWidget) return;
    timelineWidget->setRecordingDuration(ms);
    int s=ms/1000, h=s/3600, m=(s%3600)/60;
    durationLabel->setText(QString("%1:%2:%3")
                               .arg(h,2,10,QLatin1Char('0'))
                               .arg(m,2,10,QLatin1Char('0'))
                               .arg(s%60,2,10,QLatin1Char('0')));
}

void LoggerDialog::updateRecordingDurationLabel(qint64 ms)
{
    int s=ms/1000, h=s/3600, m=(s%3600)/60;
    durationLabel->setText(QString("%1:%2:%3")
                               .arg(h,2,10,QLatin1Char('0'))
                               .arg(m,2,10,QLatin1Char('0'))
                               .arg(s%60,2,10,QLatin1Char('0')));
}

void LoggerDialog::updateDuration()
{
    if (!durationDblPtr || !*durationDblPtr) return;
    updateRecordingDuration(**durationDblPtr);
    if (timelineWidget) timelineWidget->updateTimelineWidget();
}

// ═══════════════════════════════════════════════════════
//  Bookmark helpers
// ═══════════════════════════════════════════════════════

void LoggerDialog::addBookmarkWithTimestamp(const QString &note, qint64 ts)
{
    if (timestampCheckBox && timestampCheckBox->isChecked())
        timelineWidget->addBookmark(note, ts);
}

void LoggerDialog::showBookmarkOnReplay(const QString &note, qint64 ts)
{ if (timelineWidget) timelineWidget->addBookmark(note, ts); }

void LoggerDialog::onReplayBookmarkLoaded(const QString &note, qint64 ts)
{
    if (!timelineWidget) return;
    if (m_loadedBookmarkTimestamps.contains(ts)) return;
    m_loadedBookmarkTimestamps.insert(ts);
    timelineWidget->addBookmark(note, ts);
}

void LoggerDialog::setTimelineDuration(qint64 dur)
{
    if (timelineWidget) {
        timelineWidget->clearBookmarks();
        timelineWidget->setRecordingDuration(dur);
    }
}

void LoggerDialog::updateReplayProgress(qint64 ts)
{
    if (timelineWidget) {
        updateRecordingDurationLabel(ts);
        timelineWidget->setCurrentReplayTime(ts);
    }
}

void LoggerDialog::replayFromBookmark(const QString &, qint64) {}
void LoggerDialog::setC_Duration() {}

// ═══════════════════════════════════════════════════════
//  Button freeze/toggle
// ═══════════════════════════════════════════════════════
void LoggerDialog::freezeButtonOperation(ButtonNOpsList list)
{
    for (auto &p : list) {
        auto it = loggerButtonMap.find(p.first);
        if (it == loggerButtonMap.end()) continue;
        (*it->second)->setEnabled(p.second == Unfreeze);
    }
}

void LoggerDialog::toggleButton(Recorder::loggerModes m, toggleModes t)
{
    QToolButton *btn = (m==Recorder::RECORDING) ? recPlayPauseButton : repPlayPauseButton;
    if (!btn) return;
    if (t == togglePlay) {
        btn->setIcon(QIcon(":/icons/images/play.png"));
        btn->setToolTip(m==Recorder::RECORDING ? tr("Resume Recording") : tr("Resume"));
    } else {
        btn->setIcon(QIcon(":/icons/images/pause.png"));
        btn->setToolTip(m==Recorder::RECORDING ? tr("Pause Recording") : tr("Pause"));
    }
}
// ═══════════════════════════════════════════════════════
//  Alerts
// ═══════════════════════════════════════════════════════
void LoggerDialog::alertViaStr(QString msg)
{
    // show inline in status instead of popup
    loggerStatusLabel->setText(msg);
    loggerStatusLabel->setStyleSheet("color:#e74c3c; font-size:12px;");
    QTimer::singleShot(3000, this, [this]{
        loggerStatusLabel->setStyleSheet("color:#90a4ae; font-size:12px;");
    });
}

void LoggerDialog::alertViaEnum(Logger_Error err)
{ alertViaStr(errEnumToString[err]); }

// ═══════════════════════════════════════════════════════
//  inspectRecorder / setRecorder
// ═══════════════════════════════════════════════════════
void LoggerDialog::setRecorder()
{
    if (recorder == nullptr) emit getRecorder();
}

void LoggerDialog::inspectRecorder()
{
    if (!recorder) { emit getRecorder(); return; }

    qDebug() << "Inside inspect:" << recorder->modeOfLogger;
    if (recorder->modeOfLogger == Recorder::RECORDING) {
        loggerMode    = recorder->modeOfLogger;
        loggerModePtr = &recorder->modeOfLogger;
        // recordingModePtr = &recorder->m_recording->mode;
        recordingModePtr = nullptr;
    } else if (recorder->modeOfLogger == Recorder::REPLAY) {
        loggerMode    = recorder->modeOfLogger;
        loggerModePtr = &recorder->modeOfLogger;
        replayModePtr = &recorder->m_replay->mode;
    } else return;
    if (!recorder->durationPtr) return;
    durationPtr    = recorder->durationPtr;
    durationDblPtr = &recorder->durationPtr;

    if (!recorder->leftTimer || !recorder->rightTimer) return;
    leftTimer       = recorder->leftTimer;
    rightTimer      = recorder->rightTimer;
    leftTimerDblPtr  = &recorder->leftTimer;
    rightTimerDblPtr = &recorder->rightTimer;

    if (!recorder->bookmarks) return;
    bookmarks      = recorder->bookmarks;
    bookmarkDblPtr = &recorder->bookmarks;
}
std::string LoggerDialog::qintToTime(qint64 ms)
{
    int s=ms/1000, h=s/3600, m=(s%3600)/60;
    return QString("%1:%2:%3")
        .arg(h,2,10,QLatin1Char('0'))
        .arg(m,2,10,QLatin1Char('0'))
        .arg(s%60,2,10,QLatin1Char('0')).toStdString();
}
void LoggerDialog::clearLoadedBookmarks()
{
    m_loadedBookmarkTimestamps.clear();
}
void LoggerDialog::buildFilterMenu()
{
    m_filterMenu = new QMenu(this);
    m_filterMenu->setStyleSheet(
        "QMenu {"
        "  background:#0f1e2b;"
        "  border:1px solid #2c4a5e;"
        "  padding:4px 2px;"
        "}"
        "QMenu::separator {"
        "  height:1px;"
        "  background:#2c4a5e;"
        "  margin:3px 8px;"
        "}");

    // ── Helper: section header (non-interactive label) ────────────────────
    auto addHeader = [&](const QString& text) {
        QLabel* lbl = new QLabel(text, this);
        lbl->setStyleSheet(
            "color:#3498db; font-size:10px; font-weight:bold;"
            "padding:4px 10px 2px 10px; letter-spacing:1px;");
        QWidgetAction* wa = new QWidgetAction(m_filterMenu);
        wa->setDefaultWidget(lbl);
        m_filterMenu->addAction(wa);
    };

    // ── Helper: checkbox row ─────────────────────────────────────────────
    auto addCheck = [&](const QString& key, const QString& label, bool indent = false) {
        QCheckBox* cb = new QCheckBox(
            (indent ? QString("    ") : QString()) + label, this);
        cb->setChecked(true);
        cb->setStyleSheet(
            "QCheckBox {"
            "  color:#ccd6e0;"
            "  padding:3px 10px;"
            "  font-size:12px;"
            "  spacing:8px;"
            "}"
            "QCheckBox::indicator {"
            "  width:13px; height:13px;"
            "  border:1px solid #3a5a70;"
            "  border-radius:2px;"
            "  background:#162330;"
            "}"
            "QCheckBox::indicator:checked {"
            "  image: url(:/icons/images/check.png);"
            "  background: transparent;"
            "  border-color: #3498db;"
            "}"
            "QCheckBox:hover { color:#ffffff; }");
        m_recordFilters[key] = cb;

        QWidgetAction* wa = new QWidgetAction(m_filterMenu);
        wa->setDefaultWidget(cb);
        m_filterMenu->addAction(wa);
    };

    // ── Entity types ──────────────────────────────────────────────────────
    // addHeader(tr("ENTITY TYPES"));
    addCheck("FixedPoints",  tr("Fixed Points"));
    addCheck("SpecialZone",  tr("Special Zones"));
    addCheck("Trajectories", tr("Trajectories"));
    m_filterMenu->addSeparator();

    // ── Sub-systems ───────────────────────────────────────────────────────
    // addHeader(tr("SUB-SYSTEMS"));
    addCheck("Radio", tr("Radio"));
    addCheck("IFF",   tr("IFF"));
    m_filterMenu->addSeparator();

    // ── Sensors ───────────────────────────────────────────────────────────
    addHeader(tr("SENSORS"));
    addCheck("Sensors", tr("All Sensors"));
    addCheck("Radar",   tr("Radar"),    true);
    addCheck("AESA",    tr("AESA"),     true);
    addCheck("CSM",     tr("CSM"),      true);
    addCheck("ESM",     tr("ESM"),      true);
    // addCheck("EO",      tr("EO / IR"),  true);
    addCheck("Sonar",   tr("Sonar"),    true);
    addCheck("AIS",     tr("AIS"),      true);
    addCheck("ADSB",    tr("ADSB"),    true);

    // ── "All Sensors" master toggle ───────────────────────────────────────
    const QStringList sensorKeys = {
        "Radar","AESA","CSM","ESM","Sonar","AIS","ADSB"
    };
    connect(m_recordFilters["Sensors"], &QCheckBox::toggled,
            this, [this, sensorKeys](bool checked) {
                for (const QString& k : sensorKeys) {
                    if (m_recordFilters.contains(k))
                        m_recordFilters[k]->setChecked(checked);
                }
            });
    // ── When any sub-sensor is unchecked, uncheck master ─────────────────
    for (const QString& k : sensorKeys) {
        connect(m_recordFilters[k], &QCheckBox::toggled,
                this, [this, sensorKeys](bool) {
                    bool allOn = true;
                    for (const QString& sk : sensorKeys) {
                        if (m_recordFilters.contains(sk) &&
                            !m_recordFilters[sk]->isChecked()) {
                            allOn = false; break;
                        }
                    }
                    QSignalBlocker blk(m_recordFilters["Sensors"]);
                    m_recordFilters["Sensors"]->setChecked(allOn);
                });
    }
}

void LoggerDialog::applyFiltersToRecording()
{
    if (!recorder || !recorder->m_recording) return;
    auto* rec = recorder->m_recording;
    auto flag = [&](const QString& key) -> bool {
        return m_recordFilters.contains(key)
                   ? m_recordFilters[key]->isChecked()
                   : true;
    };
    rec->filterTrajectories = flag("Trajectories");
    rec->filterRadio        = flag("Radio");
    rec->filterIFF          = flag("IFF");
    rec->filterFixedPoints  = flag("FixedPoints");
    rec->filterSpecialZone  = flag("SpecialZone");
    rec->filterSensors      = flag("Sensors");
    rec->filterRadar        = flag("Radar");
    rec->filterAESA         = flag("AESA");
    rec->filterCSM          = flag("CSM");
    rec->filterESM          = flag("ESM");
    // rec->filterEO           = flag("EO");
    rec->filterSonar        = flag("Sonar");
    rec->filterAIS          = flag("AIS");
    rec->filterADSB         = flag("ADSB");
}
