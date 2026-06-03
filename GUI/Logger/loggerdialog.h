#ifndef LOGGERDIALOG_H
#define LOGGERDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStandardPaths>
#include <QDir>
#include <QToolButton>
#include <QPainter>
#include <QWidget>
#include <QDateTime>
#include <QMenuBar>
#include <QStatusBar>
#include <QMainWindow>
#include <QGroupBox>
#include <QFormLayout>
#include <QTabWidget>
#include <QStackedWidget>
#include <QMouseEvent>
#include <QFrame>
#include <QTimer>
#include <QFontMetrics>
#include <QScrollArea>
#include "core/Recorder/recorder.h"
#include <QDebug>
#include "core/SQLite/sqlite.h"

class QMouseEvent;

// ═══════════════════════════════════════════════════════
//  BookmarkListWidget
//  A plain QWidget that paints ALL bookmark rows and
//  resizes itself to fit them all.  It lives inside a
//  QScrollArea so the user can scroll when there are many.
// ═══════════════════════════════════════════════════════
class BookmarkListWidget : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kRowH = 22;
    explicit BookmarkListWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMouseTracking(true);
        setFixedHeight(4);
    }
    void setScrollArea(QScrollArea *sa) { m_scrollArea = sa; }
    void setBookmarks(const QList<QPair<QString,qint64>> &bm,
                      std::function<QString(qint64)> fmtFn)
    {
        m_bookmarks = bm;
        m_fmt       = fmtFn;
        _updateHeights();
        update();
    }
    void clear()
    {
        m_bookmarks.clear();
        m_hovered = -1;
        _updateHeights();
        update();
    }
   void clearLoadedBookmarks();
signals:
    void bookmarkClicked(const QString &note, qint64 ts);
    void jumpToTimestamp(qint64 ts);
protected:
    void mouseMoveEvent(QMouseEvent *e) override
    {
        int idx = rowAt(e->pos().y());
        if (idx != m_hovered) { m_hovered = idx; update(); }
        setCursor(idx >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }
    void leaveEvent(QEvent *) override
    {
        m_hovered = -1;
        update();
        setCursor(Qt::ArrowCursor);
    }
    void mousePressEvent(QMouseEvent *e) override
    {
        int idx = rowAt(e->pos().y());
        if (idx >= 0) {
            emit bookmarkClicked(m_bookmarks[idx].first, m_bookmarks[idx].second);
            emit jumpToTimestamp(m_bookmarks[idx].second);
        }
    }
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#0f1e2b"));
        QFont lf; lf.setPointSize(9);
        p.setFont(lf);
        QFontMetrics lfm(lf);
        for (int i = 0; i < m_bookmarks.size(); ++i) {
            int  y   = i * kRowH;
            bool hov = (i == m_hovered);

            if (hov)
                p.fillRect(0, y, width(), kRowH, QColor("#1e3a4f"));
            p.setBrush(QColor("#f1c40f")); p.setPen(Qt::NoPen);
            p.drawEllipse(8, y + kRowH/2 - 4, 8, 8);
            QFont nf; nf.setPointSize(6); nf.setBold(true); p.setFont(nf);
            p.setPen(QColor("#1a2a3a"));
            p.drawText(QRect(8, y + kRowH/2 - 4, 8, 8), Qt::AlignCenter,
                       QString::number(i + 1));
            p.setFont(lf);
            p.setPen(hov ? QColor("#ffe066") : QColor("#ccd6e0"));
            QString note = lfm.elidedText(m_bookmarks[i].first,
                                          Qt::ElideRight, width() - 90);
            p.drawText(22, y + kRowH/2 + lfm.ascent()/2, note);
            p.setPen(QColor("#607d8b"));
            QString ts = m_fmt ? m_fmt(m_bookmarks[i].second)
                               : QString::number(m_bookmarks[i].second);
            p.drawText(width() - 62, y + kRowH/2 + lfm.ascent()/2, ts);
            p.setPen(QColor("#1e2e3a"));
            p.drawLine(8, y + kRowH - 1, width() - 8, y + kRowH - 1);
        }
    }

private:
    int rowAt(int y) const
    {
        if (m_bookmarks.isEmpty() || y < 0) return -1;
        int idx = y / kRowH;
        return (idx < m_bookmarks.size()) ? idx : -1;
    }
    void _updateHeights()
    {
        static constexpr int kMaxVisible = 15;
        int count   = m_bookmarks.size();
        int innerH  = count > 0 ? count * kRowH : 1;
        int outerH  = qMin(count, kMaxVisible) * kRowH;
        setFixedHeight(innerH);
        if (m_scrollArea) {
            m_scrollArea->setFixedHeight(outerH);
            m_scrollArea->setVisible(count > 0);
        }
    }
    QList<QPair<QString,qint64>>  m_bookmarks;
    std::function<QString(qint64)> m_fmt;
    QScrollArea* m_scrollArea = nullptr;
    int m_hovered = -1;
};


// ═══════════════════════════════════════════════════════
//  TimelineWidget  —  ONLY the scrubber bar (fixed ~80px)
//  Bookmark list has been moved to BookmarkListWidget.
// ═══════════════════════════════════════════════════════
class TimelineWidget : public QWidget
{
    Q_OBJECT

private:
    Recorder::loggerModes*           loggerMode      = nullptr;
    QList<QPair<QString,qint64>>**   bookmarksDblPtr = nullptr;
    static constexpr int kTY = 36;
    qint64*  leftTimer      = nullptr;
    qint64*  rightTimer     = nullptr;
    qint64** durationDblPtr = nullptr;
    qint64   zeroTimer      = 0;

    int    hoveredBookmarkIndex = -1;
    QPoint mousePos;
    QList<QPair<QString,qint64>> bookmarks;
    QList<QPushButton*>          bookmarkButtons;
    BookmarkListWidget *m_listWidget = nullptr;
    bool recordingPaused = false;
    QDateTime recordingStartTime;

public:
    qint64* maxDurationPtr = nullptr;
    void setListWidget(BookmarkListWidget *lw) { m_listWidget = lw; }
    void setValues(Recorder::loggerModes &s_loggerMode,
                   QList<QPair<QString,qint64>> **s_bookmarksDblPtr,
                   qint64 **s_durationDblPtr)
    {
        loggerMode      = &s_loggerMode;
        bookmarksDblPtr = s_bookmarksDblPtr;
        durationDblPtr  = s_durationDblPtr;
        if (*loggerMode == Recorder::RECORDING) {
            leftTimer  = &zeroTimer;
            rightTimer = *durationDblPtr;
        } else {
            leftTimer  = *durationDblPtr;
            rightTimer = &zeroTimer;
        }
    }
    void setLoggerMode(Recorder::loggerModes &s) { loggerMode = &s; }
    void updateTimelineWidget() { update(); }
    void inspectTimelineWidget() {}
    QTimer* timer        = nullptr;
    qint64  timePerFrame = 100;
    void startUpdateUI() {
        if (!timer) timer = new QTimer(this);
        if (!timer->isActive()) timer->start(timePerFrame);
        connect(timer, &QTimer::timeout, this, [this]{ update(); });
    }
    void pause()  { if (timer) timer->stop(); }
    void resume() { if (timer) timer->start(timePerFrame); }
    void stop()   { if (timer) timer->stop(); }
    bool   replayMode      = false;
    bool   modeisRecording = true;
    qint64 pausedTimeMs        = 0;
    qint64 currentReplayTimeMs = 0;
    qint64 recordingDurationMs = 0;
    QString formatTime(qint64 ms) const {
        int s=ms/1000, h=s/3600, m=(s%3600)/60;
        return QString("%1:%2:%3")
            .arg(h,2,10,QLatin1Char('0'))
            .arg(m,2,10,QLatin1Char('0'))
            .arg(s%60,2,10,QLatin1Char('0'));
    }
    explicit TimelineWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedHeight(80);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMouseTracking(true);
    }
    void setRecordingStartTime(const QDateTime &t) { recordingStartTime=t; update(); }
    void setRecordingDuration(qint64 ms)  { recordingDurationMs=ms; update(); }
    void setCurrentReplayTime(qint64 t)   { currentReplayTimeMs=t; update(); }
    void setTimelineLimits(qint64 ms)     { recordingDurationMs=ms; update(); }
    void setCurrentRecordingTime(qint64 t){
        currentReplayTimeMs = recordingPaused ? pausedTimeMs : t;
        update();
    }
    void addBookmark(const QString &note, qint64 tsMs)
    {
        bookmarks.append({note, tsMs});
        update();
        syncListWidget();
    }
    void clearBookmarks()
    {
        bookmarks.clear();
        for (auto *b : bookmarkButtons) delete b;
        bookmarkButtons.clear();
        update();
        syncListWidget();
    }
    void pauseRecording()  { recordingPaused=true;  pausedTimeMs=currentReplayTimeMs; update(); }
    void resumeRecording() { recordingPaused=false; update(); }
    bool isRecordingPaused() const { return recordingPaused; }
    qint64 getTimestampFromPosition(int x) {
        int margin=10, w=this->width()-2*margin;
        x=qBound(margin,x,margin+w);
        if (!maxDurationPtr || *maxDurationPtr<=0) return 0;
        return qint64(double(x-margin)/double(w)*(*maxDurationPtr));
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        mousePos = event->pos();
        int prev = hoveredBookmarkIndex;
        hoveredBookmarkIndex = getPinAt(event->pos());
        if (prev != hoveredBookmarkIndex) update();
        setCursor(hoveredBookmarkIndex>=0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }
    void mousePressEvent(QMouseEvent *event) override {
        if (!loggerMode) return;
        int idx = getPinAt(event->pos());
        if (idx >= 0) {
            emit sendClickedTimestamp(bookmarks[idx].second);
            emit bookmarkButtonClicked(bookmarks[idx].first, bookmarks[idx].second);
            return;
        }
        if (*loggerMode != Recorder::REPLAY) return;
        if (!maxDurationPtr || !(*maxDurationPtr>0)) return;
        emit sendClickedTimestamp(getTimestampFromPosition(event->pos().x()));
    }
    void leaveEvent(QEvent *) override {
        hoveredBookmarkIndex = -1;
        update();
        setCursor(Qt::ArrowCursor);
    }
private:
    void syncListWidget()
    {
        if (!m_listWidget) return;
        m_listWidget->setBookmarks(bookmarks, [this](qint64 ms){ return formatTime(ms); });
    }
    int getPinAt(const QPoint &pos)
    {
        qint64 total = getTotalDuration();
        if (total <= 0) return -1;
        int margin = 10, w = width() - 2 * margin;
        for (int i = 0; i < bookmarks.size(); ++i) {
            int x    = margin + int(double(bookmarks[i].second) / double(total) * w);
            int pinH = (i % 2 == 0) ? 14 : 24;
            QRect dotRect(x - 8, kTY - pinH - 12, 16, 16);
            QRect lineRect(x - 3, kTY - pinH, 6, pinH);
            if (dotRect.contains(pos) || lineRect.contains(pos)) return i;
        }
        return -1;
    }

    qint64 getTotalDuration() const {
        if (loggerMode && *loggerMode==Recorder::REPLAY && maxDurationPtr && *maxDurationPtr>0)
            return *maxDurationPtr;
        if (durationDblPtr && *durationDblPtr && **durationDblPtr>0)
            return **durationDblPtr;
        return 0;
    }

    qint64 getCurrentTime() const {
        if (loggerMode && *loggerMode==Recorder::REPLAY && leftTimer)
            return *leftTimer;
        return 0;
    }

protected:
    void paintEvent(QPaintEvent *) override {
        if (!loggerMode) return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#0f1e2b"));
        int margin=10, w=width()-2*margin, ty=kTY;
        qint64 total   = getTotalDuration();
        qint64 current = getCurrentTime();
        p.setPen(QPen(QColor("#2c4a5e"),2));
        p.drawLine(margin, ty, margin+w, ty);
        if (total > 0) {
            int px = margin + int(double(current)/double(total)*w);
            bool isRec = (*loggerMode == Recorder::RECORDING);
            p.fillRect(margin, ty-3, px-margin, 6,
                       isRec ? QColor("#c0392b") : QColor("#2980b9"));

            p.setPen(QPen(isRec ? QColor("#e74c3c") : QColor("#3498db"), 2));
            p.drawLine(px, 4, px, ty+14);
            QFont f; f.setPointSize(8); f.setBold(true); p.setFont(f);
            p.setPen(QColor("#90a4ae"));
            p.drawText(margin,       ty-7, formatTime(current));
            p.drawText(margin+w-58,  ty-7, formatTime(total));
            qint64 iv=10000;
            if (total>3600000)     { iv=600000; }
            else if (total>600000) { iv=300000; }
            else if (total>60000)  { iv=60000;  }
            int n=total/iv+1;
            QFont tf; tf.setPointSize(7); p.setFont(tf);
            for (int i=0;i<=n;++i) {
                int x=margin+int(double(i)*w*iv/double(total));
                p.setPen(QColor("#2c4a5e"));
                p.drawLine(x,ty-4,x,ty+4);
                QString lbl;
                if      (iv==10000)  lbl=QString("%1s").arg(i*10);
                else if (iv==60000)  lbl=QString("%1m").arg(i);
                else if (iv==300000) lbl=QString("%1m").arg(i*5);
                else                 lbl=QString("%1m").arg(i*10);
                p.setPen(QColor("#607d8b"));
                p.drawText(x-10, ty+16, lbl);
            }
            for (int i=0; i<bookmarks.size(); ++i) {
                int x    = margin + int(double(bookmarks[i].second)/double(total)*w);
                int pinH = (i%2==0) ? 14 : 24;

                p.setPen(QPen(QColor("#f1c40f"),1));
                p.drawLine(x, ty-pinH, x, ty);

                p.setBrush(QColor("#f1c40f")); p.setPen(Qt::NoPen);
                p.drawEllipse(x-5, ty-pinH-10, 10, 10);

                QFont nf; nf.setPointSize(6); nf.setBold(true); p.setFont(nf);
                p.setPen(QColor("#1a2a3a"));
                p.drawText(QRect(x-5, ty-pinH-10, 10, 10), Qt::AlignCenter,
                           QString::number(i+1));
            }
            if (hoveredBookmarkIndex>=0 && hoveredBookmarkIndex<bookmarks.size()) {
                QString tip = bookmarks[hoveredBookmarkIndex].first;
                QFont tf2; tf2.setPointSize(9); p.setFont(tf2);
                QFontMetrics fm(tf2);
                int tw=fm.horizontalAdvance(tip)+16, th=fm.height()+8;
                int bx=mousePos.x()+10, by=mousePos.y()-th-4;
                if (bx+tw > width()-4) bx=mousePos.x()-tw-10;
                if (by < 2) by=4;
                p.setBrush(QColor("#253a4a")); p.setPen(QPen(QColor("#f1c40f"),1));
                p.drawRoundedRect(bx,by,tw,th,4,4);
                p.setPen(QColor("#ffe066"));
                p.drawText(QRect(bx+8,by+4,tw-16,th-8), Qt::AlignVCenter, tip);
            }
        }
    }
signals:
    void bookmarkButtonClicked(const QString &note, qint64 tsMs);
    void bookmarkClicked(const QString &note, qint64 tsMs);
    void getMaxDuration(qint64 &maxDuration);
    void sendClickedTimestamp(qint64 ts);
};
// ═══════════════════════════════════════════════════════
//  LoggerDialog
// ═══════════════════════════════════════════════════════
class LoggerDialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit LoggerDialog(QWidget *parent=nullptr, Recorder *recorder=nullptr);
    void addBookmarkWithTimestamp(const QString &note, qint64 tsMs);
    TimelineWidget* getTimelineWidget() const { return timelineWidget; }
    Recorder*  recorder  = nullptr;
    Recording* recording = nullptr;
    Replay*    replay    = nullptr;
    QList<QPushButton*> bookmarkButtons;
    qint64 pausedTimeMs = 0;
    qint64 getPauseTimeMs()         { return pausedTimeMs; }
    void   setPauseTimeMs(qint64 v) { pausedTimeMs=v; }
    SQLite::Options     mode;
    SQLite::DBStatuses* dbStatusPtr         = nullptr;
    SQLite::DBStatuses* dbStatusOfRecording = nullptr;
    void updateRecordingDuration(qint64 durationMs);
    void updateRecordingDurationLabel(qint64 durationMs);

public slots:
    void recorderInfoReceive(Recorder::LoggerStatusModes r);
    void recorderInfoReceiveOnce(QDateTime, qint64,
                                 Recorder::LoggerStatusModes,
                                 Recorder::SimulationStatusModes);
    void recorderInfoReceiveUsual(qint64,
                                  Recorder::LoggerStatusModes,
                                  Recorder::SimulationStatusModes);
    void recorderInfoReceiveDuration(qint64);
    void showBookmarkOnReplay(const QString &note, qint64 ts);
    void onReplayBookmarkLoaded(const QString &note, qint64 ts);
    void setTimelineDuration(qint64 dur);
    void replayFromBookmark(const QString &note, qint64 ts);
    void updateReplayProgress(qint64 ts);
    void switchToRecordingMode();
    void switchToReplayMode();
    void updateDuration();
    void alertViaStr(QString msg);
    void alertViaEnum(Logger_Error err);
    void toggleButton(Recorder::loggerModes mode, toggleModes toggle);
    void freezeButtonOperation(ButtonNOpsList list);
    void setC_Duration();
    void clearLoadedBookmarks();
signals:
    void loggerModeSend(Recorder::loggerModes);
    void recordingStart(Recorder &);
    void recordingPause();
    void recordingResume();
    void recordingStop();
    void replayStart();
    void replayPause();
    void replayResume();
    void replayStop();
    void replayRestart();
    void replayFileLoaded();
    void replayFileUnloaded();
    void startRecording();
    void pauseRecording();
    void stopRecording();
    void saveRecording(const QString &);
    void loadRecording(const QString &);
    void saveRecordingToFile(const QJsonObject &);
    void saveRecordingRequested();
    void replayRecording(const QString &);
    void startReplay();
    void pauseReplay();
    void resumeReplay();
    void eventTypesSelected(QStringList);
    void bookmarkAdded(const QString &);
    void timestampToggled(bool);
    void bookmarkClicked(const QString &, qint64);
    void bookmarkButtonClicked(const QString &, qint64);
    void toggleReplayPause();
    void previousFrame();
    void nextFrame();
    void pressPlayAgain();
    void requestReplayReset();
    void getRecorder();
    void dbInit();
    void getDBStatusOfRecording(SQLite::DBStatuses &);
    void dbConnect();
    void getDBStatus();
    void savedFilePath(QString);
    void getFilePath(QString);
    void getMaxDuration(qint64 &);
    void sendClickedTimestamp(qint64);
    void pauseSimulationRequested();
    void resumeSimulationRequested();
    void dbDisconnectRequested();

private:
    void setupUi();
    void setupConnections();
    void _startReplay();
    static constexpr int kTY = 36;
    QToolButton* makeBtn(const QString &iconPath, const QString &tip, int size=28);
    void loggerModeChange(Recorder::loggerModes);
    void recorderInfo();
    void recorderInfo_Update(Recorder::LoggerStatusModes);
    void recorderInfoUpdate(QDateTime, qint64,
                            Recorder::LoggerStatusModes,
                            Recorder::SimulationStatusModes);
    void recorderInfoUpdateRecordingStartTime(QDateTime);
    void recorderInfoUpdateDuration(qint64);
    void recorderInfoUpdateLoggerStatus(Recorder::LoggerStatusModes);
    // void recorderInfoUpdateSimulationStatus(Recorder::SimulationStatusModes);
    void inspectRecorder();
    void setRecorder();
    bool saveFile();
    void findFile();
    void updateRecordingsList() {}
    void _setDateRowVisible(bool visible);
    std::string qintToTime(qint64);

    // widgets
    QWidget      *centralWidget      = nullptr;
    QTabWidget   *modeTabWidget      = nullptr;
    TimelineWidget     *timelineWidget     = nullptr;
    BookmarkListWidget *m_bookmarkList     = nullptr;
    QScrollArea        *m_bookmarkScroll   = nullptr;

    QLabel *recordingDateLabel    = nullptr;
    QLabel *durationLabel         = nullptr;
    QLabel *durationLabelExtra    = nullptr;
    QLabel *loggerStatusLabel     = nullptr;
    // QLabel *simulationStatusLabel = nullptr;
    QLabel *dbStatusLabel         = nullptr;
    QLabel *fileNameLabel         = nullptr;

    // Recording controls
    QToolButton *recordButton        = nullptr;
    QToolButton *recPlayPauseButton  = nullptr;
    QToolButton *stopRecordingButton = nullptr;
    QToolButton *bookmarkButton      = nullptr;
    QCheckBox   *timestampCheckBox   = nullptr;

    // Replay controls
    QToolButton *repPlayPauseButton  = nullptr;
    QToolButton *reloadReplayButton  = nullptr;

    // backward compat aliases
    QToolButton *startReplayButton       = nullptr;
    QToolButton *pauseResumeReplayButton = nullptr;
    QToolButton *loadRecordingButton     = nullptr;
    QToolButton *databaseButton          = nullptr;
    QToolButton *databaseButtonReplay    = nullptr;
    QToolButton *pauseRecordingButton    = nullptr;

    QString filePath;
    QString recordingsDir;

    bool isRecordingPaused = false;
    bool isReplayPaused    = false;
    bool repPlaying        = false;
    QDateTime recordingStartTime;

    Recorder::loggerModes   loggerMode    = Recorder::RECORDING;
    Recorder::loggerModes*  loggerModePtr = nullptr;
    Recorder::LoggerStatusModes     loggerStatus     { Recorder::S_RECORDING_MODE };
    Recorder::SimulationStatusModes simulationStatus { Recorder::S_SIMULATION_NA  };
    qint64 duration = 0;

    qint64*  durationPtr      = nullptr;
    qint64** durationDblPtr   = nullptr;
    qint64*  leftTimer        = nullptr;
    qint64** leftTimerDblPtr  = nullptr;
    qint64*  rightTimer       = nullptr;
    qint64** rightTimerDblPtr = nullptr;

    QList<QPair<QString,qint64>>*  bookmarks     = nullptr;
    QList<QPair<QString,qint64>>** bookmarkDblPtr = nullptr;

    Recording::recordingModes* recordingModePtr = nullptr;
    Replay::replayModes*       replayModePtr    = nullptr;

    QTimer* updateDurationTimer = nullptr;

    QString loggerStatusModeString[10];
    QString SimulationStatusModeString[4];
    QString debugString;
    Recorder::loggerModes modeOfLogger;

    std::unordered_map<LoggerButton, QToolButton**> loggerButtonMap;
    QSet<qint64> m_loadedBookmarkTimestamps;
    QToolButton*               m_filterBtn    = nullptr;
    QMenu*                     m_filterMenu   = nullptr;
    QMap<QString, QCheckBox*>  m_recordFilters;

    void buildFilterMenu();
    void applyFiltersToRecording();

    QString errEnumToString[Err_Undefine_Error] = {
        "Database is Not Available",
        "Database File Does Not Exist",
        "Undefined Error in Logger"
    };
    QToolButton *loadNewFileButton = nullptr;
};

#endif // LOGGERDIALOG_H
