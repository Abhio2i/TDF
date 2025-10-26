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
    void recordToJson();
    void record(const QJsonObject &data);       // Store entire JSON data
    void recordFrame(const QJsonObject &frame); // Store individual frame
    bool saveToFile();
    bool loadFromFile(const QString &filePath);
    void clear();  // Clears previously recorded dataz

    void setRate(int rate);
    int getRate() const;

    void startReplay();
    void stopReplay();

    QVector<QJsonObject> getRecordedFrames() const;

signals:
    void replayFrame(QJsonObject frame);

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
    //End Hima
};

#endif // RECORDER_H
