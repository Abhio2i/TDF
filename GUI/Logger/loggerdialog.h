
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
#include "core/Recorder/recorder.h"
#include <QDebug>
class TimelineWidget : public QWidget
{
    Q_OBJECT
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

    bool replayMode = false;
    bool modeisRecording = true;
    //bool toRunInRecording = true;
    qint64 pausedTimeMs = 0;
    qint64 currentReplayTimeMs = 0;
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
        qDebug()<<"=====pauseRecording=====";
        recordingPaused = true;
        pausedTimeMs = currentReplayTimeMs;   // freeze the progress
        update();
    }

    void resumeRecording()
    {
        qDebug()<<"=====resumeRecording=====";
        recordingPaused = false;
        update();
    }

    bool isRecordingPaused() const { return recordingPaused; }
signals:
    void bookmarkButtonClicked(const QString &note, qint64 timestampMs);
    void bookmarkClicked(const QString &note, qint64 timestampMs);

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), Qt::white);

        int margin = 10;
        int width = this->width() - 2 * margin;
        int height = this->height() - 2 * margin;
        int timelineY = height / 2;

        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(margin, margin + timelineY, margin + width, margin + timelineY);

        if (recordingDurationMs > 0 ) {
            // Draw time labels at both ends
            QString leftTime;
            QString rightTime;

            if (replayMode) {
                leftTime = formatTime(currentReplayTimeMs);
            } else {
                leftTime = "00:00:00";
            }

            rightTime = formatTime(recordingDurationMs);

            painter.setPen(Qt::black);
            QFont f = painter.font();
            f.setBold(true);
            painter.setFont(f);

            painter.drawText(margin, margin + timelineY - 15, leftTime);
            painter.drawText(margin + width - 80, margin + timelineY - 15, rightTime);
        }

        if (recordingDurationMs <= 0) {
            painter.setPen(QPen(Qt::gray, 1));
            painter.drawText(margin + 10, margin + timelineY - 15, tr("No active recording"));
            return;
        }

        // Dynamically determine the best interval based on total duration
        qint64 intervalMs;
        QString intervalUnit;

        if (recordingDurationMs <= 60000) {
            // Under 1 minute: show 10-second intervals
            intervalMs = 10000; // 10 seconds
            intervalUnit = "s";
        } else if (recordingDurationMs <= 600000) {
            // 1-10 minutes: show 1-minute intervals
            intervalMs = 60000; // 1 minute
            intervalUnit = "m";
        } else if (recordingDurationMs <= 3600000) {
            // 10-60 minutes: show 5-minute intervals
            intervalMs = 300000; // 5 minutes
            intervalUnit = "m";
        } else {
            // Over 1 hour: show 10-minute intervals
            intervalMs = 600000; // 10 minutes
            intervalUnit = "m";
        }

        int numIntervals = recordingDurationMs / intervalMs + 1;

        for (int i = 0; i <= numIntervals && !(recordingPaused && modeisRecording); ++i) {
            int x = margin + (i * width * intervalMs) / recordingDurationMs;
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

        // Draw bookmarks
        painter.setPen(QPen(Qt::red, 2));
        for (int i = 0; i < bookmarks.size(); ++i) {
            const auto &bookmark = bookmarks[i];
            qint64 relativeTimeMs = bookmark.second;
            if (relativeTimeMs >= 0 && (relativeTimeMs <= recordingDurationMs && !(recordingPaused && modeisRecording))) {
                int x = margin + (relativeTimeMs * width) / recordingDurationMs;
                painter.drawLine(x, margin + timelineY - 10, x, margin + timelineY + 10);
                if (i < bookmarkButtons.size()) {
                    QPushButton *button = bookmarkButtons[i];
                    button->setVisible(true);
                    button->move(x + 5, margin + timelineY - 25);
                    button->resize(100, 20);
                }
            }
        }

        // Draw current replay position line
        if ((currentReplayTimeMs > 0 && recordingDurationMs > 0) && !modeisRecording) {
            int x = margin + (currentReplayTimeMs * width) / recordingDurationMs;
            painter.setPen(QPen(Qt::blue, 2));
            painter.drawLine(x, margin, x, margin + height);
        }
    }

private:
    QDateTime recordingStartTime;
    qint64 recordingDurationMs = 0;
    QList<QPair<QString, qint64>> bookmarks;
    QList<QPushButton*> bookmarkButtons;

    bool recordingPaused = false;

};

class LoggerDialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit LoggerDialog(QWidget *parent = nullptr, Recorder* recorder = nullptr);
    void updateRecordingDuration(qint64 durationMs);
    void addBookmarkWithTimestamp(const QString &note, qint64 timestampMs);
    void updateRecordingDurationLabel(qint64 durationMs);

    TimelineWidget* getTimelineWidget() const { return timelineWidget; }

public slots:
    void showBookmarkOnReplay(const QString& note, qint64 timestamp);
    void onReplayBookmarkLoaded(const QString& note, qint64 timestamp);
    void setTimelineDuration(qint64 duration);
    void replayFromBookmark(const QString& note, qint64 timestamp);
    void updateReplayProgress(qint64 timestamp);
    void switchToRecordingMode();
    void switchToReplayMode();
    // void setReplayTimelineDuration(qint64 duration);
    // Hime
    // Hime
private slots:
    void showBookmarkDialog();

signals:
    void startRecording();
    void pauseRecording();
    void stopRecording();
    void saveRecording(const QString &filePath);
    void loadRecording(const QString &filePath);
    void saveRecordingToFile(const QJsonObject &recordings);
    void saveRecordingRequested();
    void replayRecording(const QString &filePath);
    void startReplay();
    void pauseReplay();
    void resumeReplay();
    void eventTypesSelected(QStringList eventTypes);
    void bookmarkAdded(const QString &bookmarkNote);
    void timestampToggled(bool enabled);
    void bookmarkClicked(const QString &note, qint64 timestampMs);
    void bookmarkButtonClicked(const QString &note, qint64 timestampMs);

    void toggleReplayPause();
    void previousFrame();
    void nextFrame();
    void pressPlayAgain();
    void requestReplayReset();

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
    TimelineWidget *timelineWidget;

    // Information labels
    QLabel *recordingDateLabel;
    QLabel *durationLabel;
    QLabel *loggerStatusLabel;
    QLabel *simulationStatusLabel;

    // Recording Mode Controls
    QToolButton *recordButton;
    QToolButton *pauseRecordingButton;
    QToolButton *stopRecordingButton;

    // Replay Mode Controls
    QToolButton *startReplayButton;
    QToolButton *pauseResumeReplayButton;
    QToolButton *previousFrameButton;
    QToolButton *nextFrameButton;
    QToolButton *loadRecordingButton;

    QString filePath;
    QString recordingsDir;
    QDateTime recordingStartTime;
    Recorder* recorder;
    QList<QPushButton*> bookmarkButtons;

    // State variables
    bool isRecordingPaused = false;
    bool isReplayPaused = false;



};

#endif // LOGGERDIALOG_H
