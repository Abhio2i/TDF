// #ifndef RECORDER_H
// #define RECORDER_H

// #include <QObject>
// #include <QJsonObject>
// #include <QJsonArray>
// #include <QFile>
// #include <QJsonDocument>
// #include <QTimer>

// // Forward declarations to avoid circular includes
// class Hierarchy;
// class Simulation;

// class Recorder : public QObject
// {
//     Q_OBJECT

// public:
//     // Constructor:Accepts hierarchy and simulation to pull state and recording speed
//     explicit Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent = nullptr);

//     void startRecording();
//     void stopRecording();
//     void recordToJson();
//     void record(const QJsonObject &data);       // Store entire JSON data
//     void recordFrame(const QJsonObject &frame); // Store individual frame
//     bool saveToFile(const QString &filePath = QString());
//     bool loadFromFile(const QString &filePath = QString());
//     void clear();  // Clears previously recorded dataz

//     void setRate(int rate);
//     int getRate() const;

//     void startReplay();
//     void stopReplay();

//     QVector<QJsonObject> getRecordedFrames() const;

// signals:
//     void replayFrame(QJsonObject frame);

// private slots:
//     void playNextFrame();

// private:
//     Hierarchy* m_hierarchy = nullptr;  // Used for extracting structure snapshot
//     Simulation* m_simulation = nullptr;  // Used for getting simulation speed

//     QJsonObject recordedData;  // Main data JSON object
//     QJsonArray trajectoryArray;  // Stores all frames for replay
//     int sampleRate = 1;  // Sample rate for recording
//     int currentFrame = 0;  // Tracks current frame during replay

//     QTimer *replayTimer = nullptr;
//     QVector<QJsonObject> recordedFrames;
// };

// #endif // RECORDER_H
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
    // void recordToJson();
    void recordToJson(const QString &filePath);

    void record(const QJsonObject &data);       // Store entire JSON data
    void recordFrame(const QJsonObject &frame); // Store individual frame
    // bool saveToFile();
    bool saveToFile(const QString &filePath);
    void saveBookmark(const QString &message, qint64 timestampMs);
    bool loadFromFile(const QString &filePath);
    void clear();  // Clears previously recorded dataz
    //By Hime
    //void showBookmarkLog(const QString &note, qint64 timestampMs);
    void bookmarkReplay(const QString &note, qint64 timestampMs);
    void startReplayFromTimestamp(qint64 timestampMs);
    //By Hime End
    void setRate(int rate);
    int getRate() const;

    void startReplay();
    void stopReplay();

    QVector<QJsonObject> getRecordedFrames() const;

signals:
    void replayFrame(QJsonObject frame); //p1
    void bookmarkAdded(const QString &note, qint64 timestampMs);
    //By Amz

    void replayBookmark(const QString &note, qint64 timestamp);
    void setReplayDuration(qint64 duration);
    //By Amz

    // void bookmarkAdded(const QString &note, qint64 timestampMs); // emitted during playback when frame has "message"
    // void playbackProgress(qint64 timestampMs);                    // current playback timestamp (ms)
    // void playbackStarted(qint64 totalDurationMs);                // emitted once at start (total duration)
    // void playbackFinished();
private slots:
    void playNextFrame();

private:
    Hierarchy* m_hierarchy = nullptr;  // Used for extracting structure snapshot
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
    QVector<QJsonObject> playbackFrames;
    //End Hima
};

#endif // RECORDER_H
