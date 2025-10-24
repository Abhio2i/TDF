
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

class TimelineWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(50);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    void setRecordingStartTime(const QDateTime &startTime) {
        recordingStartTime = startTime;
        update();
    }
    void setRecordingDuration(qint64 durationMs) {
        recordingDurationMs = durationMs;
        update();
    }
    void addBookmark(const QString &note, qint64 timestampMs) {
        bookmarks.append({note, timestampMs});
        update();
    }
    void clearBookmarks() {
        bookmarks.clear();
        update();
    }

protected:
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
    QDateTime recordingStartTime;
    qint64 recordingDurationMs = 0;
    QList<QPair<QString, qint64>> bookmarks; // {note, timestampMs}
};

class LoggerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoggerDialog(QWidget *parent = nullptr);
    void updateRecordingDuration(qint64 durationMs);
    void addBookmarkWithTimestamp(const QString &note, qint64 timestampMs);

signals:
    void startRecording();
    void stopRecording();
    void replayRecording(const QString &filePath);
    void eventTypesSelected(QStringList eventTypes);
    void bookmarkAdded(const QString &bookmarkNote);
    void timestampToggled(bool enabled);

private:
    void setupUi();
    void updateRecordingsList();

    QCheckBox *actionsCheckBox;
    QCheckBox *waypointsCheckBox;
    QCheckBox *engagementsCheckBox;
    QCheckBox *timestampCheckBox;
    // QLabel *statusLabel;
    QListWidget *recordingsList;
    QPushButton *replayButton;
    QToolButton *bookmarkButton;
    QString recordingsDir;
    TimelineWidget *timelineWidget;
    QDateTime recordingStartTime;
};

#endif // LOGGERDIALOG_H
