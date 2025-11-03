

#ifndef LOGGERDIALOG_H
#define LOGGERDIALOG_H
#include <QDebug>
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
//#include "core/Recorder/recorder.h"
#include "core/Recorder/recorder.h"

class TimelineWidget : public QWidget
{
    Q_OBJECT
public:
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
    // void addBookmark(const QString &note, qint64 timestampMs) {
    //     bookmarks.append({note, timestampMs});
    //     update();
    // }


    void addBookmark(const QString &note, qint64 timestampMs) {
        bookmarks.append({note, timestampMs});
        // Added: Create a button for the bookmark with dynamic sizing based on text
        QPushButton *button = new QPushButton(note, this);
        button->setFlat(true);
        // Modified: Improved styling and added tooltip for full text visibility
        button->setStyleSheet("border: none; color: red; background: transparent; text-align: left; font-size: 12px; padding: 2px;");
        button->setToolTip(note); // Added: Show full note on hover
        button->setVisible(false); // Initially hidden, positioned in paintEvent
        // Modified: Dynamically adjust button width based on text content
        QFontMetrics fm(button->font());
        int buttonWidth = fm.horizontalAdvance(note) + 10; // Add padding
        button->resize(qMin(buttonWidth, 150), 20); // Limit max width to 150px
        bookmarkButtons.append(button);
        // Added: Connect button click to emit bookmarkClicked signal
        connect(button, &QPushButton::clicked, this, [this, note, timestampMs]() {
            emit bookmarkButtonClicked(note, timestampMs);
        });
        update();
    }
    // void clearBookmarks() {
    //     bookmarks.clear();
    //     update();
    // }
    void clearBookmarks() {
        bookmarks.clear();
        // Added: Delete all bookmark buttons
        for (QPushButton *button : bookmarkButtons) {
            delete button;
        }
        bookmarkButtons.clear();
        update();
    }

signals:
    // Added: Signal emitted when a bookmark button is clicked
    void bookmarkButtonClicked(const QString &note, qint64 timestampMs);
    void bookmarkClicked(const QString &note, qint64 timestampMs);
    // protected:
    // void paintEvent(QPaintEvent *event) override {
    //     QPainter painter(this);
    //     painter.setRenderHint(QPainter::Antialiasing);
    //     painter.fillRect(rect(), Qt::white);

    //     int margin = 10;
    //     int width = this->width() - 2 * margin;
    //     int height = this->height() - 2 * margin;
    //     int timelineY = height / 2;

    //     // Always draw the timeline
    //     painter.setPen(QPen(Qt::black, 2));
    //     painter.drawLine(margin, margin + timelineY, margin + width, margin + timelineY);

    //     // If no recording is active, show a placeholder text
    //     if (recordingDurationMs <= 0) {
    //         painter.setPen(QPen(Qt::gray, 1));
    //         painter.drawText(margin + 10, margin + timelineY - 15, tr("No active recording"));
    //         return;
    //     }

    //     // Draw duration markers (every 10 seconds)
    //     qint64 intervalMs = 10000; // 10 seconds
    //     int numIntervals = recordingDurationMs / intervalMs + 1;
    //     for (int i = 0; i <= numIntervals; ++i) {
    //         int x = margin + (i * width * intervalMs) / recordingDurationMs;
    //         painter.setPen(QPen(Qt::black, 1));
    //         painter.drawLine(x, margin + timelineY - 5, x, margin + timelineY + 5);
    //         painter.drawText(x - 20, margin + timelineY + 20, QString("%1s").arg(i * 10));
    //     }

    //     // Draw bookmarks
    //     painter.setPen(QPen(Qt::red, 2));
    //     for (const auto &bookmark : bookmarks) {
    //         qint64 relativeTimeMs = bookmark.second;
    //         if (relativeTimeMs >= 0 && relativeTimeMs <= recordingDurationMs) {
    //             int x = margin + (relativeTimeMs * width) / recordingDurationMs;
    //             painter.drawLine(x, margin + timelineY - 10, x, margin + timelineY + 10);
    //             painter.drawText(x + 5, margin + timelineY - 15, bookmark.first);
    //         }
    //     }
    // }
protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), Qt::white);

        int margin = 10;
        int width = this->width() - 2 * margin;
        int height = this->height() - 2 * margin;
        int timelineY = height / 2;

        // Always draw the timeline
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(margin, margin + timelineY, margin + width, margin + timelineY);

        // If no recording is active, show a placeholder text
        if (recordingDurationMs <= 0) {
            painter.setPen(QPen(Qt::gray, 1));
            painter.drawText(margin + 10, margin + timelineY - 15, tr("No active recording"));
            return;
        }

        // Draw duration markers (every 10 seconds)
        qint64 intervalMs = 10000; // 10 seconds
        int numIntervals = recordingDurationMs / intervalMs + 1;
        for (int i = 0; i <= numIntervals; ++i) {
            int x = margin + (i * width * intervalMs) / recordingDurationMs;
            painter.setPen(QPen(Qt::black, 1));
            painter.drawLine(x, margin + timelineY - 5, x, margin + timelineY + 5);
            painter.drawText(x - 20, margin + timelineY + 20, QString("%1s").arg(i * 10));
        }

        // Draw bookmarks as lines and position buttons
        painter.setPen(QPen(Qt::red, 2));
        for (int i = 0; i < bookmarks.size(); ++i) {
            const auto &bookmark = bookmarks[i];
            qint64 relativeTimeMs = bookmark.second;
            if (relativeTimeMs >= 0 && relativeTimeMs <= recordingDurationMs) {
                int x = margin + (relativeTimeMs * width) / recordingDurationMs;
                painter.drawLine(x, margin + timelineY - 10, x, margin + timelineY + 10);
                // Added: Position the corresponding button
                if (i < bookmarkButtons.size()) {
                    QPushButton *button = bookmarkButtons[i];
                    button->setVisible(true);
                    button->move(x + 5, margin + timelineY - 25);
                    button->resize(100, 20); // Adjust size as needed
                }
            }
        }
    }

private:
    QDateTime recordingStartTime;
    qint64 recordingDurationMs = 0;
    QList<QPair<QString, qint64>> bookmarks; // {note, timestampMs}
    //By Hima
    void saveRecordingToFile();   // <--- Add this new method
    //End Hima

    QList<QPushButton*> bookmarkButtons;
};

class LoggerDialog : public QDialog
{
    Q_OBJECT
public:
        // explicit LoggerDialog(QWidget *parent = nullptr);
    explicit LoggerDialog(QWidget *parent = nullptr, Recorder* recorder = nullptr);
    void updateRecordingDuration(qint64 durationMs);
    void addBookmarkWithTimestamp(const QString &note, qint64 timestampMs);

public slots:
    void showBookmarkOnReplay(const QString& note, qint64 timestamp);
    void setTimelineDuration(qint64 duration);
    void replayFromBookmark(const QString& note, qint64 timestamp);
signals:
    void startRecording();
    void stopRecording();
    // void saveRecording();
    void saveRecording(const QString &filePath);

    //By Hima
    void loadRecording(const QString &filePath);
    void saveRecordingToFile(const QJsonObject &recordings);
    void saveRecordingRequested();
    //End Hima
    void replayRecording(const QString &filePath);
    void eventTypesSelected(QStringList eventTypes);
    void bookmarkAdded(const QString &bookmarkNote);
    void timestampToggled(bool enabled);
    void bookmarkClicked(const QString &note, qint64 timestampMs);
    void bookmarkButtonClicked(const QString &note, qint64 timestampMs);

private:

    void setupUi();
    void updateRecordingsList();
    void saveRecordingToFile();
    QCheckBox *actionsCheckBox;
    QCheckBox *waypointsCheckBox;
    QCheckBox *engagementsCheckBox;
    QCheckBox *timestampCheckBox;
    // QLabel *statusLabel;
    QListWidget *recordingsList;
    QPushButton *replayButton;
    QToolButton *bookmarkButton;
    //By Hima
    //Recorder *recorder = nullptr;
    QString filePath;
    QPushButton *loadRecordingButton;
    QPushButton *replayRecordingButton;

    //QPushButton *saveRecordingButton;
    //End Hima
    QString recordingsDir;
    TimelineWidget *timelineWidget;
    QDateTime recordingStartTime;

    Recorder* recorder;


    // Added: List to store bookmark buttons
    QList<QPushButton*> bookmarkButtons;


};

#endif // LOGGERDIALOG_H
