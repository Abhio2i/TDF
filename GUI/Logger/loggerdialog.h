/* ========================================================================= */
/* File: loggerdialog.h                                                     */
/* Purpose: Defines dialog and widget for logging and timeline visualization */
/* ========================================================================= */

#ifndef LOGGERDIALOG_H
#define LOGGERDIALOG_H

#include <QDialog>                                // For dialog base class
#include <QCheckBox>                              // For checkbox widget
#include <QLabel>                                 // For label widget
#include <QListWidget>                            // For list widget
#include <QPushButton>                            // For push button widget
#include <QVBoxLayout>                            // For vertical layout
#include <QGridLayout>                            // For grid layout
#include <QStandardPaths>                         // For standard paths
#include <QDir>                                   // For directory handling
#include <QToolButton>                            // For tool button widget
#include <QPainter>                               // For painting operations
#include <QWidget>                                // For widget base class
#include <QDateTime>                              // For date and time handling

// %%% TimelineWidget Class %%%
/* Widget for visualizing recording timeline */
class TimelineWidget : public QWidget
{
    Q_OBJECT

public:
    // Initialize timeline widget
    explicit TimelineWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(50);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    // Set recording start time
    void setRecordingStartTime(const QDateTime &startTime) {
        recordingStartTime = startTime;
        update();
    }
    // Set recording duration
    void setRecordingDuration(qint64 durationMs) {
        recordingDurationMs = durationMs;
        update();
    }
    // Add bookmark with note and timestamp
    void addBookmark(const QString &note, qint64 timestampMs) {
        bookmarks.append({note, timestampMs});
        update();
    }
    // Clear all bookmarks
    void clearBookmarks() {
        bookmarks.clear();
        update();
    }

protected:
    // Handle paint events
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), Qt::white);

        if (recordingDurationMs <= 0) return;

        int margin = 10;
        int width = this->width() - 2 * margin;
        int height = this->height() - 2 * margin;
        int timelineY = height / 2;

        // Draw timeline
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(margin, margin + timelineY, margin + width, margin + timelineY);

        // Draw duration markers (every 10 seconds)
        qint64 intervalMs = 10000; // 10 seconds
        int numIntervals = recordingDurationMs / intervalMs + 1;
        for (int i = 0; i <= numIntervals; ++i) {
            int x = margin + (i * width * intervalMs) / recordingDurationMs;
            painter.setPen(QPen(Qt::black, 1));
            painter.drawLine(x, margin + timelineY - 5, x, margin + timelineY + 5);
            painter.drawText(x - 20, margin + timelineY + 20, QString("%1s").arg(i * 10));
        }

        // Draw bookmarks
        painter.setPen(QPen(Qt::red, 2));
        for (const auto &bookmark : bookmarks) {
            qint64 relativeTimeMs = bookmark.second;
            if (relativeTimeMs >= 0 && relativeTimeMs <= recordingDurationMs) {
                int x = margin + (relativeTimeMs * width) / recordingDurationMs;
                painter.drawLine(x, margin + timelineY - 10, x, margin + timelineY + 10);
                painter.drawText(x + 5, margin + timelineY - 15, bookmark.first);
            }
        }
    }

private:
    // %%% Data Members %%%
    // Recording start time
    QDateTime recordingStartTime;
    // Recording duration in milliseconds
    qint64 recordingDurationMs = 0;
    // List of bookmarks (note, timestamp)
    QList<QPair<QString, qint64>> bookmarks;
};

// %%% LoggerDialog Class %%%
/* Dialog for managing logging operations */
class LoggerDialog : public QDialog
{
    Q_OBJECT

public:
    // Initialize logger dialog
    explicit LoggerDialog(QWidget *parent = nullptr);
    // Update recording duration
    void updateRecordingDuration(qint64 durationMs);
    // Add bookmark with timestamp
    void addBookmarkWithTimestamp(const QString &note, qint64 timestampMs);

signals:
    // Signal start recording
    void startRecording();
    // Signal stop recording
    void stopRecording();
    // Signal replay recording
    void replayRecording(const QString &filePath);
    // Signal event types selected
    void eventTypesSelected(QStringList eventTypes);
    // Signal bookmark added
    void bookmarkAdded(const QString &bookmarkNote);
    // Signal timestamp toggle
    void timestampToggled(bool enabled);

private:
    // %%% UI Setup Methods %%%
    // Configure UI components
    void setupUi();
    // Update recordings list
    void updateRecordingsList();

    // %%% UI Components %%%
    // Actions checkbox
    QCheckBox *actionsCheckBox;
    // Waypoints checkbox
    QCheckBox *waypointsCheckBox;
    // Engagements checkbox
    QCheckBox *engagementsCheckBox;
    // Timestamp checkbox
    QCheckBox *timestampCheckBox;
    // Status label (commented)
    // QLabel *statusLabel;
    // Recordings list widget
    QListWidget *recordingsList;
    // Replay button
    QPushButton *replayButton;
    // Bookmark button
    QToolButton *bookmarkButton;
    // Recordings directory path
    QString recordingsDir;
    // Timeline widget
    TimelineWidget *timelineWidget;
    // Recording start time
    QDateTime recordingStartTime;
};

#endif // LOGGERDIALOG_H
