
#ifndef RECORDER_H
#define RECORDER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <QTimer>
#include <QJsonValue>

// Forward declarations to avoid circular includes
class Hierarchy;
class Simulation;

class Recorder : public QObject
{
    Q_OBJECT
public:
    // Constructor:Accepts hierarchy and simulation to pull state and recording speed
    explicit Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent = nullptr);
    QJsonObject getAllRecordings() const;
    void startRecording();
    void stopRecording();
    void resumeRecording();
    void pauseRecording();
    void recordToJson(const QString &filePath);
    void record(const QJsonObject &data);        // Store entire JSON data
    void recordBookmark(const QString &message, qint64 timestampMs);// Store entire JSON data for Bookmark
    void recordFrame(const QJsonObject &frame); // Store individual frame
    // bool saveToFile();
    bool saveToFile(const QString &filePath);
    //void saveBookmark(const QString &message, qint64 timestampMs);
    bool loadReplay(const QString &filePath);
    void clear();  // Clears previously recorded dataz
    //By Hime
    //void showBookmarkLog(const QString &note, qint64 timestampMs);
    void bookmarkReplay(const QString &note, qint64 timestampMs);
    void startReplayFromTimestamp(qint64 timestampMs);
    void toggleReplayPause();
    void startReplay();
    void playAgain();
    void goToNextFrame();
    void goToPreviousFrame();
    //By Hime End
    void setRate(int rate);
    int getRate() const;
    void stopReplay();

    QVector<QJsonObject> getRecordedFrames() const;

signals:
    void replayFrame(QJsonObject frame);
    void bookmarkAdded(const QString &note, qint64 timestampMs);
    void replayBookmark(const QString &note, qint64 timestamp);     // fired during loadReplay
    void bookmarkReached(const QString& note, qint64 timestamp);
    void setReplayDuration(qint64 duration);
    //Start Himan
    void recordingPaused();
    void recordingResumed();
    void frameLoaded(const QJsonObject &frame);
    void replayFrameLoaded(qint64 timestampMs);

     void recordingStateChanged(bool recording, bool paused);
    //End Himan
private slots:
    void playNextFrame();

    //Start Himan
public slots:
    void togglePause();
    void resetReplayState();
    //End Himan
private:
    Hierarchy* m_hierarchy = nullptr;  // Used for extracting structure snapshot
    QJsonObject p_hierarchy;
    Simulation* m_simulation = nullptr;  // Used for getting simulation speed

    QJsonObject recordedData;  // Main data JSON object
    QJsonArray trajectoryArray;  // Stores all frames for replay
    int sampleRate = 1;  // Sample rate for recording
    int currentFrame = 0;  // Tracks current frame during replay
    QTimer *replayTimer = nullptr;
    QVector<QJsonObject> recordedFrames;
    //By Hima
    QJsonValue getArrayElement(const QJsonArray &array, int index);
    QDateTime recordingStartTime;  // Start time of the current recording
    QTimer *recordingTimer = nullptr; // Timer to store intervals
    QJsonArray m_recordings;
    QJsonArray bookmarkArray;
    QVector<QJsonObject> playbackFrames;
    bool isRecordingPaused = false;
    qint64 pausedTimeOffset = 0;
    QDateTime pauseStartTime;


    bool isReplayPaused = false;
    QDateTime replayPauseStartTime;
    qint64 replayPausedTimeOffset = 0;

    int currentReplayIndex = 0;
    //End Hima
};

#endif // RECORDER_H
