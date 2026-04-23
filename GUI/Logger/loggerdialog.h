//============================================================================
// File        : loggerdialog.h
// Description : Header file for LoggerDialog class which provides a dialog
//               interface for logging functionality including recording,
//               playback, bookmarking, and timeline visualization.

//============================================================================


#ifndef LOGGERDIALOG_H
#define LOGGERDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
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
#include "core/Recorder/recorder.h"
#include <QDebug>

#include "core/SQLite/sqlite.h"

class QMouseEvent;  // forward declared in QWidget

class TimelineWidget : public QWidget
{
    Q_OBJECT
private:
    Recorder*              recorder;
    Recorder::loggerModes* loggerMode = nullptr;
    QList<QPair<QString, qint64>>** bookmarksDblPtr = nullptr;
    qint64* leftTimer   = nullptr;
    qint64* rightTimer  = nullptr;
    qint64** durationDblPtr    = nullptr;
    qint64** leftTimerDblPtr   = nullptr;
    qint64** rightTimerDblPtr  = nullptr;
    qint64  zeroTimer   = 0;
public:
    qint64*  maxDurationPtr = nullptr;
public:
    void setValues(
        Recorder::loggerModes &s_loggerMode,
        QList<QPair<QString, qint64>>   **s_bookmarksDblPtr,
        qint64 **s_durationDblPtr){
        loggerMode      = &s_loggerMode  ;
        bookmarksDblPtr = s_bookmarksDblPtr;
        durationDblPtr  = s_durationDblPtr;
        if(*loggerMode == Recorder::RECORDING){
            leftTimer  = &zeroTimer;
            rightTimer = *durationDblPtr;
        }
        else if(*loggerMode == Recorder::REPLAY){
            leftTimer  = *durationDblPtr;
            rightTimer = &zeroTimer;
        }
        inspectTimelineWidget();
    }
public:
    void inspectTimelineWidget(){
        if(loggerMode == nullptr){
            qDebug()<<"LoggerMode: nullptr";
            return;
        }
        switch(*loggerMode){
        case Recorder::RECORDING:
            //qDebug()<<*loggerMode;
            break;
        case Recorder::REPLAY:
            //qDebug()<<*loggerMode;
            break;
        }
        if(bookmarksDblPtr == nullptr){
            //qDebug()<<"bookmarksDblPtr: nullptr";
        }else if(*bookmarksDblPtr == nullptr){
            //qDebug()<<"*bookmarksDblPtr: nullptr";
        }else{
            //qDebug()<<"bookmarksDblPtr: "<<**bookmarksDblPtr;
        }

        if(durationDblPtr == nullptr){
            //qDebug()<<"durationPtr: nullptr";
        }else if(*durationDblPtr == nullptr){
            //qDebug()<<"*durationDblPtr: nullptr";
        }else{
            //qDebug()<<"durationDblPtr: "<<**durationDblPtr;
        }

        if(leftTimer == nullptr){
            //qDebug()<<"leftTimer: nullptr";
        }else{
            //qDebug()<<"leftTimer: "<<formatTime(*leftTimer);
        }

        if(rightTimer == nullptr){
            //qDebug()<<"rightTimer: nullptr";
        }else{
            //qDebug()<<"rightTimer: "<<formatTime(*rightTimer);
        }

    }
    void setLoggerMode(Recorder::loggerModes &s_loggerMode)
    {
        loggerMode = &s_loggerMode;
        inspectTimelineWidget();
    }
    void updateTimelineWidget(){
        update();
    }
    // Update Mechanism Start
public:
    QTimer* timer = nullptr;
    qint64  timePerFrame = 100;
    void startUpdateUI(){
        if (timer == nullptr) {
            timer = new QTimer(this);
        }
        if(timer->isActive() == false){
            timer->start(timePerFrame);
        }
        connect(timer, &QTimer::timeout, this, [this]() {
            inspectTimelineWidget();
        });
    }
    void pause(){
        timer->stop();
    }
    void resume(){
        timer->start(timePerFrame);
    }
    void stop(){
        timer->stop();
        timer->destroyed();
    }
public:
    // Update Mechanism End


    bool replayMode = false;
    bool modeisRecording = true;
    QTime  recordingDuration;
    qint64 pausedTimeMs = 0;
    qint64 currentReplayTimeMs = 0;
    qint64 recordingDurationMs = 0;

private:
    QDateTime recordingStartTime;

    QList<QPair<QString, qint64>> bookmarks;
    QList<QPushButton*> bookmarkButtons;
    //By Him
    bool recordingPaused = false;
public:
    QString formatTime(qint64 ms) const {
        int totalSeconds = ms / 1000;
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        return QString("%1:%2:%3")
            .arg(hours,   2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    void setTimelineLimits(qint64 durationMs)
    {
        recordingDurationMs = durationMs;
        update();
    }



    explicit TimelineWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(50);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMouseTracking(true);
    }
    void setRecordingStartTime(const QDateTime &startTime) {
        recordingStartTime = startTime;
        update();
    }
    void setRecordingDuration(qint64 durationMs) {
        recordingDurationMs = durationMs;
        update();
    }
    void setCurrentReplayTime(qint64 t)
    {
        currentReplayTimeMs = t;
        update();
    }
    void setCurrentRecordingTime(qint64 t)
    {
        if (recordingPaused) {
            currentReplayTimeMs = pausedTimeMs;
        } else {
            currentReplayTimeMs = t;
        }
        update();
    }

    void addBookmark(const QString &note, qint64 timestampMs) {
        bookmarks.append({note, timestampMs});
        QPushButton *button = new QPushButton(note, this);
        button->setFlat(true);
        button->setStyleSheet("border: none; color: red; background: transparent; text-align: left; font-size: 12px; padding: 2px;");
        button->setToolTip(note);
        button->setVisible(false);
        QFontMetrics fm(button->font());
        int buttonWidth = fm.horizontalAdvance(note) + 10;
        button->resize(qMin(buttonWidth, 150), 20);
        bookmarkButtons.append(button);
        connect(button, &QPushButton::clicked, this, [this, note, timestampMs]() {
            emit bookmarkButtonClicked(note, timestampMs);
        });
        update();
    }

    void clearBookmarks() {
        bookmarks.clear();
        for (QPushButton *button : bookmarkButtons) {
            delete button;
        }
        bookmarkButtons.clear();
        update();
    }

    void pauseRecording()
    {
        // qDebug()<<"=====pauseRecording=====";
        recordingPaused = true;
        pausedTimeMs = currentReplayTimeMs;
        update();
    }

    void resumeRecording()
    {
        // qDebug()<<"=====resumeRecording=====";
        recordingPaused = false;
        update();
    }

    bool isRecordingPaused() const { return recordingPaused; }

    void mousePressEvent(QMouseEvent *event) override
    {
        if(*loggerMode != Recorder::REPLAY){
            return;
        }
        if (!maxDurationPtr || !(*maxDurationPtr > 0)) {
            return;
        }
        qint64 clickedTimestamp = getTimestampFromPosition(event->pos().x());
        emit sendClickedTimestamp(clickedTimestamp);

    }
    qint64 getTimestampFromPosition(int x)
    {
        int margin = 10;
        int width = this->width() - 2 * margin;
        if (x < margin) x = margin;
        if (x > margin + width) x = margin + width;
        double ratio = double(x - margin) / double(width);
        qint64 timestamp = ratio * (*maxDurationPtr);
        return timestamp;
    }

signals:
    void bookmarkButtonClicked(const QString &note, qint64 timestampMs);
    void bookmarkClicked(const QString &note, qint64 timestampMs);
    void getMaxDuration(qint64 &maxDuration);
    void sendClickedTimestamp(qint64 clickedTimestamp);

protected:
    void paintEventInRecording(){
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), Qt::white);

        int margin = 10;
        int width = this->width() - 2 * margin;
        int height = this->height() - 2 * margin;
        int timelineY = height / 2;

        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(margin, margin + timelineY, margin + width, margin + timelineY);
        if (durationDblPtr && *durationDblPtr && **durationDblPtr > 0 ) {
            // Draw time labels at both ends
            QString leftTime = formatTime(*leftTimer);
            QString rightTime= formatTime(*rightTimer);

            painter.setPen(Qt::black);
            QFont f = painter.font();
            f.setBold(true);
            painter.setFont(f);

            painter.drawText(margin, margin + timelineY - 15, leftTime);
            painter.drawText(margin + width - 80, margin + timelineY - 15, rightTime);
        }

        if (!durationDblPtr) {
            painter.setPen(QPen(Qt::gray, 1));
            painter.drawText(margin + 10, margin + timelineY - 15, tr("No active recording"));
            return;
        }
        qint64 intervalMs = 0;
        QString intervalUnit;
        if(loggerMode == nullptr){
            return;
        }

        if (**durationDblPtr <= 60000) {
            // Under 1 minute: show 10-second intervals
            intervalMs = 10000; // 10 seconds
            intervalUnit = "s";
        } else if (**durationDblPtr <= 600000) {
            // 1-10 minutes: show 1-minute intervals
            intervalMs = 60000; // 1 minute
            intervalUnit = "m";
        } else if (**durationDblPtr <= 3600000) {
            // 10-60 minutes: show 5-minute intervals
            intervalMs = 300000; // 5 minutes
            intervalUnit = "m";
        } else {
            // Over 1 hour: show 10-minute intervals
            intervalMs = 600000; // 10 minutes
            intervalUnit = "m";
        }

        int numIntervals = **durationDblPtr / intervalMs + 1;

        // for (int i = 0; i <= numIntervals && !(recordingPaused && modeisRecording); ++i) {
        for (int i = 0; i <= numIntervals && durationDblPtr && *durationDblPtr && **durationDblPtr; ++i) {
            int x = margin + (i * width * intervalMs) / **durationDblPtr;
            painter.setPen(QPen(Qt::black, 1));
            painter.drawLine(x, margin + timelineY - 5, x, margin + timelineY + 5);

            // Format the label based on the interval
            QString labelText;
            if (intervalMs == 10000) {
                // For 10-second intervals, show seconds
                labelText = QString("%1%2").arg(i * 10).arg(intervalUnit);
            } else if (intervalMs == 60000) {
                // For 1-minute intervals, show minutes
                labelText = QString("%1%2").arg(i).arg(intervalUnit);
            } else if (intervalMs == 300000) {
                // For 5-minute intervals, show minutes
                labelText = QString("%1%2").arg(i * 5).arg(intervalUnit);
            } else if (intervalMs == 600000) {
                // For 10-minute intervals, show minutes
                labelText = QString("%1%2").arg(i * 10).arg(intervalUnit);
            }

            painter.drawText(x - 20, margin + timelineY + 20, labelText);
        }
    }
    void paintEventInReplay(){
        QPainter painter(this);
        if(maxDurationPtr == nullptr){
            return;
        }
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), Qt::white);

        int margin = 10;
        int width = this->width() - 2 * margin;
        int height = this->height() - 2 * margin;
        int timelineY = height / 2;

        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(margin, margin + timelineY, margin + width, margin + timelineY);
        if (maxDurationPtr && *maxDurationPtr && *maxDurationPtr > 0 ) {
            // Draw time labels at both ends
            QString leftTime = formatTime(*leftTimer);
            QString rightTime= formatTime(*maxDurationPtr);
            painter.setPen(Qt::black);
            QFont f = painter.font();
            f.setBold(true);
            painter.setFont(f);

            painter.drawText(margin, margin + timelineY - 15, leftTime);
            painter.drawText(margin + width - 80, margin + timelineY - 15, rightTime);
        }
        if (!maxDurationPtr) {
            painter.setPen(QPen(Qt::gray, 1));
            painter.drawText(margin + 10, margin + timelineY - 15, tr("No active recording"));
            return;
        }
        qint64 intervalMs = 0;
        QString intervalUnit;

        if (*maxDurationPtr <= 60000) {
            // Under 1 minute: show 10-second intervals
            intervalMs = 10000; // 10 seconds
            intervalUnit = "s";
        } else if (*maxDurationPtr <= 600000) {
            // 1-10 minutes: show 1-minute intervals
            intervalMs = 60000; // 1 minute
            intervalUnit = "m";
        } else if (*maxDurationPtr <= 3600000) {
            // 10-60 minutes: show 5-minute intervals
            intervalMs = 300000; // 5 minutes
            intervalUnit = "m";
        } else {
            // Over 1 hour: show 10-minute intervals
            intervalMs = 600000; // 10 minutes
            intervalUnit = "m";
        }

        int numIntervals = *maxDurationPtr / intervalMs + 1;

        for (int i = 0; i <= numIntervals && maxDurationPtr && *maxDurationPtr; ++i) {
            int x = margin + (i * width * intervalMs) / *maxDurationPtr;
            painter.setPen(QPen(Qt::black, 1));
            painter.drawLine(x, margin + timelineY - 5, x, margin + timelineY + 5);

            // Format the label based on the interval
            QString labelText;
            if (intervalMs == 10000) {
                // For 10-second intervals, show seconds
                labelText = QString("%1%2").arg(i * 10).arg(intervalUnit);
            } else if (intervalMs == 60000) {
                // For 1-minute intervals, show minutes
                labelText = QString("%1%2").arg(i).arg(intervalUnit);
            } else if (intervalMs == 300000) {
                // For 5-minute intervals, show minutes
                labelText = QString("%1%2").arg(i * 5).arg(intervalUnit);
            } else if (intervalMs == 600000) {
                // For 10-minute intervals, show minutes
                labelText = QString("%1%2").arg(i * 10).arg(intervalUnit);
            }

            painter.drawText(x - 20, margin + timelineY + 20, labelText);
        }
        // Draw current replay position line
        if (leftTimer && maxDurationPtr) {
            int x = margin + (*leftTimer * width) / *maxDurationPtr;
            painter.setPen(QPen(Qt::blue, 2));
            painter.drawLine(x, margin, x, margin + height);
        }
    }

    void paintEvent(QPaintEvent *event) override {

        // QPainter painter(this);
        // painter.setRenderHint(QPainter::Antialiasing);
        // painter.fillRect(rect(), Qt::white);

        // int margin = 10;
        // int width = this->width() - 2 * margin;
        // int height = this->height() - 2 * margin;
        // int timelineY = height / 2;

        // painter.setPen(QPen(Qt::black, 2));
        // painter.drawLine(margin, margin + timelineY, margin + width, margin + timelineY);
        // if (durationDblPtr && *durationDblPtr && **durationDblPtr > 0 ) {
        //     // Draw time labels at both ends
        //     QString leftTime = formatTime(*leftTimer);
        //     QString rightTime= formatTime(*rightTimer);

        //     // if (replayMode) {
        //     //     leftTime = formatTime(currentReplayTimeMs);
        //     // } else {
        //     //     leftTime = "00:00:00";
        //     // }

        //     // rightTime = formatTime(recordingDurationMs);

        //     painter.setPen(Qt::black);
        //     QFont f = painter.font();
        //     f.setBold(true);
        //     painter.setFont(f);

        //     painter.drawText(margin, margin + timelineY - 15, leftTime);
        //     painter.drawText(margin + width - 80, margin + timelineY - 15, rightTime);
        // }
        // // if (recordingDurationMs > 0 ) {
        // //     // Draw time labels at both ends
        // //     QString leftTime;
        // //     QString rightTime;

        // //     if (replayMode) {
        // //         leftTime = formatTime(currentReplayTimeMs);
        // //     } else {
        // //         leftTime = "00:00:00";
        // //     }

        // //     rightTime = formatTime(recordingDurationMs);

        // //     painter.setPen(Qt::black);
        // //     QFont f = painter.font();
        // //     f.setBold(true);
        // //     painter.setFont(f);

        // //     painter.drawText(margin, margin + timelineY - 15, leftTime);
        // //     painter.drawText(margin + width - 80, margin + timelineY - 15, rightTime);
        // // }
        // if (!durationDblPtr) {
        //     painter.setPen(QPen(Qt::gray, 1));
        //     painter.drawText(margin + 10, margin + timelineY - 15, tr("No active recording"));
        //     return;
        // }
        // qint64 intervalMs = 0;
        // QString intervalUnit;
        if(loggerMode == nullptr){
            return;
        }
        switch(*loggerMode){
            case Recorder::RECORDING:
                paintEventInRecording();
            break;
            case Recorder::REPLAY:
                paintEventInReplay();
            break;
        }
        // if (**durationDblPtr <= 60000) {
        //     // Under 1 minute: show 10-second intervals
        //     intervalMs = 10000; // 10 seconds
        //     intervalUnit = "s";
        // } else if (**durationDblPtr <= 600000) {
        //     // 1-10 minutes: show 1-minute intervals
        //     intervalMs = 60000; // 1 minute
        //     intervalUnit = "m";
        // } else if (**durationDblPtr <= 3600000) {
        //     // 10-60 minutes: show 5-minute intervals
        //     intervalMs = 300000; // 5 minutes
        //     intervalUnit = "m";
        // } else {
        //     // Over 1 hour: show 10-minute intervals
        //     intervalMs = 600000; // 10 minutes
        //     intervalUnit = "m";
        // }

        // int numIntervals = **durationDblPtr / intervalMs + 1;

        // // for (int i = 0; i <= numIntervals && !(recordingPaused && modeisRecording); ++i) {
        // for (int i = 0; i <= numIntervals && durationDblPtr && *durationDblPtr && **durationDblPtr; ++i) {
        //     int x = margin + (i * width * intervalMs) / **durationDblPtr;
        //     painter.setPen(QPen(Qt::black, 1));
        //     painter.drawLine(x, margin + timelineY - 5, x, margin + timelineY + 5);

        //     // Format the label based on the interval
        //     QString labelText;
        //     if (intervalMs == 10000) {
        //         // For 10-second intervals, show seconds
        //         labelText = QString("%1%2").arg(i * 10).arg(intervalUnit);
        //     } else if (intervalMs == 60000) {
        //         // For 1-minute intervals, show minutes
        //         labelText = QString("%1%2").arg(i).arg(intervalUnit);
        //     } else if (intervalMs == 300000) {
        //         // For 5-minute intervals, show minutes
        //         labelText = QString("%1%2").arg(i * 5).arg(intervalUnit);
        //     } else if (intervalMs == 600000) {
        //         // For 10-minute intervals, show minutes
        //         labelText = QString("%1%2").arg(i * 10).arg(intervalUnit);
        //     }

        //     painter.drawText(x - 20, margin + timelineY + 20, labelText);
        // }

        // Not Use yet Above is used

        // if (recordingDurationMs <= 0) {
        //     painter.setPen(QPen(Qt::gray, 1));
        //     painter.drawText(margin + 10, margin + timelineY - 15, tr("No active recording"));
        //     return;
        // }

        // Dynamically determine the best interval based on total duration
        // qint64 intervalMs;
        // QString intervalUnit;

        // if (recordingDurationMs <= 60000) {
        //     // Under 1 minute: show 10-second intervals
        //     intervalMs = 10000; // 10 seconds
        //     intervalUnit = "s";
        // } else if (recordingDurationMs <= 600000) {
        //     // 1-10 minutes: show 1-minute intervals
        //     intervalMs = 60000; // 1 minute
        //     intervalUnit = "m";
        // } else if (recordingDurationMs <= 3600000) {
        //     // 10-60 minutes: show 5-minute intervals
        //     intervalMs = 300000; // 5 minutes
        //     intervalUnit = "m";
        // } else {
        //     // Over 1 hour: show 10-minute intervals
        //     intervalMs = 600000; // 10 minutes
        //     intervalUnit = "m";
        // }

        // int numIntervals = recordingDurationMs / intervalMs + 1;

        // for (int i = 0; i <= numIntervals && !(recordingPaused && modeisRecording); ++i) {
        //     int x = margin + (i * width * intervalMs) / recordingDurationMs;
        //     painter.setPen(QPen(Qt::black, 1));
        //     painter.drawLine(x, margin + timelineY - 5, x, margin + timelineY + 5);

        //     // Format the label based on the interval
        //     QString labelText;
        //     if (intervalMs == 10000) {
        //         // For 10-second intervals, show seconds
        //         labelText = QString("%1%2").arg(i * 10).arg(intervalUnit);
        //     } else if (intervalMs == 60000) {
        //         // For 1-minute intervals, show minutes
        //         labelText = QString("%1%2").arg(i).arg(intervalUnit);
        //     } else if (intervalMs == 300000) {
        //         // For 5-minute intervals, show minutes
        //         labelText = QString("%1%2").arg(i * 5).arg(intervalUnit);
        //     } else if (intervalMs == 600000) {
        //         // For 10-minute intervals, show minutes
        //         labelText = QString("%1%2").arg(i * 10).arg(intervalUnit);
        //     }

        //     painter.drawText(x - 20, margin + timelineY + 20, labelText);
        // }

        // Draw bookmarks
        // painter.setPen(QPen(Qt::red, 2));
        // for (int i = 0; i < bookmarks.size(); ++i) {
        //     const auto &bookmark = bookmarks[i];
        //     qint64 relativeTimeMs = bookmark.second;
        //     if (relativeTimeMs >= 0 && (relativeTimeMs <= recordingDurationMs && !(recordingPaused && modeisRecording))) {
        //         int x = margin + (relativeTimeMs * width) / recordingDurationMs;
        //         painter.drawLine(x, margin + timelineY - 10, x, margin + timelineY + 10);
        //         if (i < bookmarkButtons.size()) {
        //             QPushButton *button = bookmarkButtons[i];
        //             button->setVisible(true);
        //             button->move(x + 5, margin + timelineY - 25);
        //             button->resize(100, 20);
        //         }
        //     }
        // }

        // Draw current replay position line
        // if ((currentReplayTimeMs > 0 && recordingDurationMs > 0) && !modeisRecording) {
        //     int x = margin + (currentReplayTimeMs * width) / recordingDurationMs;
        //     painter.setPen(QPen(Qt::blue, 2));
        //     painter.drawLine(x, margin, x, margin + height);
        // }
    }



};

class LoggerDialog : public QMainWindow
{
    Q_OBJECT

    //Common Components Start
public:

private:
    Recorder::loggerModes modeOfLogger;
    //Recorder::LoggerStatusModes loggerStatus;
    QString debugString;

public  slots:
              //void loggerModeChangeStatus(bool mode);
private:
    void loggerModeChange(Recorder::loggerModes mode);

private slots:
               //    void loggerModeChangeShow(bool mode);
signals:
    void loggerModeSend  (Recorder::loggerModes modeOfLogger);
    //Common Components End


    //Recorder Information Start
private:
    TimelineWidget* timelineWidget;
    //QDateTime recordingStartTime = QDateTime(); //Temp Remove
    qint64     duration;
    Recorder::LoggerStatusModes     loggerStatus     { Recorder::S_RECORDING_MODE};
    Recorder::SimulationStatusModes simulationStatus { Recorder::S_SIMULATION_NA };

    QString loggerStatusModeString[10];
    QString SimulationStatusModeString[4];
    //Use to Show Recorder Information
    void recorderInfo();
    void recorderInfo_Update(Recorder::LoggerStatusModes r_loggerStatus);

    void recorderInfoUpdate(
        QDateTime       r_recordingStartTime,
        qint64           r_duration,
        Recorder::LoggerStatusModes     r_loggerStatus,
        Recorder::SimulationStatusModes r_simulationStatus);
    void recorderInfoUpdateRecordingStartTime(QDateTime r_recordingStartTime);
    void recorderInfoUpdateDuration(qint64 r_duration);
    void recorderInfoUpdateLoggerStatus(Recorder::LoggerStatusModes r_loggerStatus);
    void recorderInfoUpdateSimulationStatus(Recorder::SimulationStatusModes r_simulationStatus);

public:
    void updateRecordingDuration(qint64 durationMs);
    void updateRecordingDurationLabel(qint64 durationMs);

public slots:
    //Testing
    void recorderInfoReceive(
        Recorder::LoggerStatusModes     r_loggerStatus);
    // Get the Recorder Information from Core
    void recorderInfoReceiveOnce(
        QDateTime       r_recordingStartTime,
        qint64           r_duration,
        Recorder::LoggerStatusModes     r_loggerStatus,
        Recorder::SimulationStatusModes r_simulationStatus);
    void recorderInfoReceiveUsual(
        qint64           r_duration,
        Recorder::LoggerStatusModes     r_loggerStatus,
        Recorder::SimulationStatusModes r_simulationStatus);
    void recorderInfoReceiveDuration(qint64 r_duration);

signals:
    //Recorder : Recording
    void recordingStart(Recorder &s_recorder);
    void recordingPause();
    void recordingResume();
    void recordingStop();



signals:
    void replayStart();
    void replayPause();
    void replayResume();
    void replayStop();
    void replayRestart();
    void replayFileLoaded();
    void replayFileUnloaded();
public:
    explicit LoggerDialog(QWidget *parent = nullptr, Recorder* recorder = nullptr);
    //void updateRecordingDuration(qint64 durationMs);
    void addBookmarkWithTimestamp(const QString &note, qint64 timestampMs);
    TimelineWidget* getTimelineWidget() const { return timelineWidget; }

public slots:
    void showBookmarkOnReplay(const QString& note, qint64 timestamp);
    void onReplayBookmarkLoaded(const QString& note, qint64 timestamp);
    void setTimelineDuration(qint64 duration);
    void replayFromBookmark(const QString& note, qint64 timestamp);
    void updateReplayProgress(qint64 timestamp);
    void switchToRecordingMode();
    void switchToReplayMode();

private slots:
    void showBookmarkDialog();

signals:
    // Recording Part Start
    void startRecording();
    void pauseRecording();
    void stopRecording();
    void saveRecording(const QString &filePath);
    void loadRecording(const QString &filePath);
    void saveRecordingToFile(const QJsonObject &recordings);
    void saveRecordingRequested();
    // Recording Part End

    // Replay Part Start
    void replayRecording(const QString &filePath);
    void startReplay();
    void pauseReplay();
    void resumeReplay();
    void eventTypesSelected(QStringList eventTypes);
    void bookmarkAdded(const QString &bookmarkNote);
    void timestampToggled(bool enabled);
    void bookmarkClicked(const QString &note, qint64 timestampMs);
    void bookmarkButtonClicked(const QString &note, qint64 timestampMs);
    //Start Himan
    void toggleReplayPause();
    void previousFrame();
    void nextFrame();
    void pressPlayAgain();
    void requestReplayReset();
    //End Himan
    // Replay Part End
private:
    void setupUi();
    void setupMenuBar();
    void updateRecordingsList();
    void setupRecordingMode();
    void setupReplayMode();
    QWidget* createRecordingControls();
    QWidget* createReplayControls();
    void setupConnections();

    // UI Components
    QWidget *centralWidget;
    QTabWidget *modeTabWidget;
    QCheckBox *timestampCheckBox;
    QListWidget *recordingsList;
    QToolButton *bookmarkButton;
    //TimelineWidget *timelineWidget;

    // Information labels
    QLabel *recordingDateLabel;
    QLabel *durationLabel;
    QLabel *durationLabelExtra;
    QLabel *loggerStatusLabel;
    QLabel *simulationStatusLabel;

    // Recording Mode Controls
    QToolButton *recordButton;
    QToolButton *pauseRecordingButton;
    QToolButton *stopRecordingButton;
    QToolButton *databaseButton;
    QToolButton *databaseButtonReplay;

    // Replay Mode Controls
    QToolButton *startReplayButton;
    QToolButton *pauseResumeReplayButton;
    QToolButton *previousFrameButton;
    QToolButton *nextFrameButton;
    QToolButton *loadRecordingButton;

    QString filePath;
    QString recordingsDir;
public:
    Recorder* recorder = nullptr;
    Recording* recording;
    Replay* replay;
    QList<QPushButton*> bookmarkButtons;

    // Unification Start
private:
    Recorder::loggerModes   loggerMode = Recorder::loggerModes::RECORDING;
    Recorder::loggerModes*  loggerModePtr = nullptr;
    qint64*  durationPtr      = nullptr;
    qint64** durationDblPtr   = nullptr;
    qint64*  leftTimer        = nullptr;
    qint64** leftTimerDblPtr  = nullptr;
    qint64*  rightTimer       = nullptr;
    qint64** rightTimerDblPtr = nullptr;
    // qint64   maxDuration      = 0;
    // qint64*  maxDurationPtr   = &maxDuration;

    // qint64** maxDurationDblPtr = &maxDurationPtr;
    // qint64** maxDurationDblPtr = new qint64*(new qint64(0));


    QList<QPair<QString, qint64>>*  bookmarks = nullptr;
    QList<QPair<QString, qint64>>** bookmarkDblPtr = nullptr;
    void inspectRecorder();
    std::string qintToTime(qint64 m_duration);
    Recording::recordingModes* recordingModePtr = nullptr;
    Replay::replayModes*       replayModePtr = nullptr;
    // Volatile Memory Start
    QTimer* updateDurationTimer;
public slots:
    void updateDuration();
    // Volatile Memory End
    // Unification End
signals:
    void getRecorder();
private:
    // State variables
    void setRecorder();
    bool isRecordingPaused = false;
    bool isReplayPaused = false;
    QDateTime recordingStartTime;
public slots:
    //void receiveRecorder(Recorder &r_recoder);
public:
    qint64 pausedTimeMs = 0;
    qint64 getPauseTimeMs(){
        return pausedTimeMs;
    }
    void setPauseTimeMs(qint64 newPausedTimeMs){
        pausedTimeMs = newPausedTimeMs;
    }
    //==================  Toggle Button Button ====================
public slots:
    void toggleButton(Recorder::loggerModes mode, toggleModes toggle);
    //================ Freeze and Unfreeze Button =================
public slots:
    void freezeButtonOperation(
        ButtonNOpsList buttonOperationList);
    //QToolButton *buttonList[button] = {};
private:
    std::unordered_map<LoggerButton,QToolButton **>
    loggerButtonMap = std::unordered_map<LoggerButton,QToolButton **>(
        {{Recorder_Button  ,&recordButton},
         {Recording_Toggle ,&pauseRecordingButton},
         {Reocrding_Stop   ,&stopRecordingButton},
         {Replay_Start         ,&startReplayButton},
         {Replay_Toggle        ,&pauseResumeReplayButton},
         {Replay_Jump_Forward  ,&nextFrameButton},
         {Replay_Jump_Backward ,&previousFrameButton},
         {Replay_Restart       ,&loadRecordingButton}});

    //================= SQLite DataBase Connection =================
public:

    void setC_Duration();
    //  Start
public:
    SQLite::Options      mode;
    SQLite::DBStatuses*  dbStatusPtr         = nullptr;
    SQLite::DBStatuses*  dbStatusOfRecording = nullptr;
signals:
    void dbInit();
    void getDBStatusOfRecording(SQLite::DBStatuses &dbStatusOfRecording);
    void dbConnect();
    void getDBStatus();

    //================== File Start =================
private:
    bool saveFile();
    void findFile();
signals:
    void savedFilePath(QString path);
    void getFilePath(QString path);
    //==================  File End  =================

    //==========  Timeline Widget Start =============
signals:
    void getMaxDuration(qint64 &maxDuration);
    void sendClickedTimestamp(qint64 clickedTimestamp);
    //===========  Timeline Widget End ==============

    //================= Alert Start =================
public slots:
    void alertViaStr(QString msg);
    void alertViaEnum(Logger_Error err);
private:
    QString errEnumToString[Err_Undefine_Error] = {
        "Database is Not Avalable",
        "Database File Is Not Exist"
        "Undefined Error in Logger"
    };
    //================= Alert End   =================
    //  End
};

#endif // LOGGERDIALOG_H
