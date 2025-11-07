#include "recorder.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Simulation/simulation.h"

#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

// Constructor: Initializes recorder with hierarchy and simulation pointers
Recorder::Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent)
    : QObject(parent), m_hierarchy(hierarchy), m_simulation(simulation)
{
    // Timer for replaying recorded frames
    replayTimer = new QTimer(this);
    connect(replayTimer, &QTimer::timeout, this, &Recorder::playNextFrame);

    // Automatically update sample rate when simulation speed change
    if (m_simulation) {
        connect(m_simulation, &Simulation::speedUpdated, this, [=](int rate) {
            setRate(rate);
        });
    }
}
QJsonObject Recorder::getAllRecordings() const
{
    QJsonObject obj;
    obj["recordings"] = m_recordings; // m_recordings is QJsonArray
    return obj;
}

void Recorder::startRecording()
{
    qDebug() << "Recording started.";
    clear();

    if (!m_hierarchy) {
        qWarning() << "Hierarchy is null. Cannot record.";
        return;
    }

    // Initial snapshot
    //QJsonObject jsonData = m_hierarchy->toJson();
    //jsonData["recording_rate"] = getRate();
    //record(jsonData);

    // Start recording time
    recordingStartTime = QDateTime::currentDateTime();

    // Create timer if not already created
    if (!recordingTimer) {
        recordingTimer = new QTimer(this);

        connect(recordingTimer, &QTimer::timeout, this, [this]() {
            qint64 elapsedMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());

            QJsonObject timeEntry;
            timeEntry["timestamp_ms"] = elapsedMs;
            timeEntry["current_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            //timeEntry["snapshot"] = m_hierarchy->toJson();

            record(timeEntry);

            qDebug() << "Recorded interval at" << elapsedMs << "ms";
        });
    }

    int intervalMs = 100; // record every 1 second
    recordingTimer->start(intervalMs);
}


void Recorder::stopRecording()
{
    qDebug() << "Recording stopped.";

    // Stop and delete the interval timer if it exists
    if (recordingTimer) {
        recordingTimer->stop();
        recordingTimer->deleteLater();
        recordingTimer = nullptr;
    }

    // Reset recording start time
    recordingStartTime = QDateTime();

    // Display the entire JSON recording
    // if (!m_recordings.isEmpty()) {
    //     QJsonDocument doc(m_recordings);
    //     QString jsonString = doc.toJson(QJsonDocument::Indented); // Pretty print
    //     qDebug() << "Full Recording JSON:\n" << jsonString;
    // } else {
    //     qDebug() << "No recordings to display.";
    // }

    qDebug() << "Recorder cleanup done.";
}




// Convert hierarchy to JSON and save it to file
void Recorder::recordToJson(const QString &filePath)
{
    saveToFile(filePath); // Automatically save to file
}

// Store provided JSON data in internal buffer
void Recorder::record(const QJsonObject &data)
{
    m_recordings.append(data);
    recordedData = data;
}

// Store a single frame of recording into trajectory array
void Recorder::recordFrame(const QJsonObject &frame)
{
    trajectoryArray.append(frame);   // Add to array
    recordedData["trajectories"] = trajectoryArray;  // Update in main object
}

// bool Recorder::saveToFile()
// {
//     if (m_recordings.isEmpty()) {
//         qWarning() << "No recordings to save!";
//         return false;
//     }
//     QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recordings";
//     QDir().mkpath(directory); // ensure directory exists
//     // Build file path with timestamp
//     QString finalPath = directory + "/recorder" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".json";

//     QFile file(finalPath);
//     if (!file.open(QIODevice::WriteOnly)) {
//         qWarning() << "Failed to open file for saving:" << finalPath;
//         return false;
//     }

//     // Convert recorded snapshots to JSON document
//     QJsonDocument doc(m_recordings);

//     // Write JSON to file
//     file.write(doc.toJson(QJsonDocument::Indented));
//     file.close();

//     qDebug() << "Recording saved to:" << finalPath;

//     // Optionally open the folder containing the file
//     QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(finalPath).absolutePath()));

//     return true;
// }
bool Recorder::saveToFile(const QString &filePath)
{
    if (m_recordings.isEmpty()) {
        qWarning() << "No recordings to save!";
        return false;
    }

    // Ensure the directory exists
    QDir dir(QFileInfo(filePath).absolutePath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for saving:" << filePath;
        return false;
    }

    // Convert recorded snapshots to JSON document
    QJsonDocument doc(m_recordings);

    // Write JSON to file
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "Recording saved to:" << filePath;

    // Optionally open the folder containing the file
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));

    return true;
}
void Recorder::saveBookmark(const QString &message, qint64 timestampMs)
{
    if (!recordingStartTime.isValid()) {
        qWarning() << "Cannot save bookmark — recording is not active.";
        return;
    }

    qint64 elapsedMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());

    QJsonObject bookmarkEntry;
    bookmarkEntry["timestamp_ms"] = elapsedMs;
    bookmarkEntry["current_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    bookmarkEntry["message"] = message;

    // Store this entry just like normal recording frames
    record(bookmarkEntry);


    qDebug().noquote() << "Bookmark saved at" << elapsedMs << "ms — message:" << message;
}



bool Recorder::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for loading:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return false;
    }

    // Check JSON format
    if (!doc.isArray()) {
        qWarning() << "Invalid JSON format: expected array of snapshots.";
        return false;
    }

    QJsonArray jsonArray = doc.array();
    if (jsonArray.isEmpty()) {
        qWarning() << "JSON file is empty.";
        return false;
    }

    // Stop any previous playback timers
    if (recordingTimer) {
        recordingTimer->stop();
        delete recordingTimer;
        recordingTimer = nullptr;
    }

    // Copy all snapshots into memory
    playbackFrames.clear();
    playbackFrames.reserve(jsonArray.size());

    // QList<QJsonObject> messageFrames;
    //messageFrames.reserve(jsonArray.size());

    qint64 maxTimestamp = 0;
    for (const QJsonValue &value : jsonArray) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            playbackFrames.append(obj);
            if (obj.contains("timestamp_ms")){
                maxTimestamp = qMax(maxTimestamp, obj["timestamp_ms"].toVariant().toLongLong());
            }
            // else if (obj.contains("message")){
            //     messageFrames.append(obj);
            // }

        }
    }

    qDebug() << "Loaded" << playbackFrames.size() << "frames for playback.";

    // --- SIGNAL: Set total duration on UI before replay (ensure timeline scaling is correct) ---
    emit setReplayDuration(maxTimestamp);
    // for(QJsonObject JO : QList<QJsonObject> messageFrames){
    //     QString note = JO["message"].toString();
    //     qint64 timestampMs = JO["timestamp_ms"].toVariant().toLongLong();
    //     emit replayBookmark(note, timestampMs);
    // }
    // for (const QJsonObject &JO : QList<QJsonObject>(messageFrames)) {
    //     QString note = JO.value("message").toString();
    //     qint64 timestampMs = JO.value("timestamp_ms").toVariant().toLongLong();
    //     emit replayBookmark(note, timestampMs);
    // }
    // if (frame.contains("message")) {
    //     QString note = frame["message"].toString();
    //     qint64 timestampMs = frame["timestamp_ms"].toVariant().toLongLong();
    //     emit replayBookmark(note, timestampMs); // <-- This updates LoggerDialog/TimelineWidget
    // }
    // --- Clear bookmarks before starting replay; slot in LoggerDialog should handle this ---

    // Start playback timer
    int currentIndex = 0;
    recordingTimer = new QTimer(this);

    connect(recordingTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentIndex < playbackFrames.size()) {
            const QJsonObject &frame = playbackFrames.at(currentIndex);

            // Bookmarks: emit for UI when replayed
            if (frame.contains("message")) {
                QString note = frame["message"].toString();
                qint64 timestampMs = frame["timestamp_ms"].toVariant().toLongLong();
                emit replayBookmark(note, timestampMs); // <-- This updates LoggerDialog/TimelineWidget
            }

            // Hierarchy state: replay as usual
            QJsonValue JV = frame["snapshot"];
            if (JV.isObject()) {
                m_hierarchy->fromJson(JV.toObject());
                qDebug().noquote() << "Replayed frame" << currentIndex
                                   << "at timestamp:" << frame["timestamp_ms"].toInt()
                                   << "ms\n";
                //<< QJsonDocument(JV.toObject()).toJson(QJsonDocument::Indented);
            } else if (!frame.contains("message")) {
                // Only warn for snapshot frames, not bookmarks
                qWarning() << "Invalid snapshot type in frame" << currentIndex;
            }

            ++currentIndex;
        } else {
            qDebug() << "Playback completed.";
            recordingTimer->stop();
        }
    });

    recordingTimer->start(100); // replay interval = 0.1 sec
    return true;
}

void Recorder::startReplayFromTimestamp(qint64 timestampMs)
{
    if (playbackFrames.isEmpty())
        return;

    // Find first frame with timestamp >= the requested time
    int startIdx = 0;
    for (int i = 0; i < playbackFrames.size(); ++i) {
        qint64 frameTime = playbackFrames[i].value("timestamp_ms").toVariant().toLongLong();
        if (frameTime >= timestampMs) {
            startIdx = i;
            break;
        }
    }

    int currentIndex = startIdx;
    if (recordingTimer) {
        recordingTimer->stop();
        delete recordingTimer;
        recordingTimer = nullptr;
    }
    recordingTimer = new QTimer(this);

    connect(recordingTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentIndex < playbackFrames.size()) {
            const QJsonObject &frame = playbackFrames.at(currentIndex);

            // Bookmarks: emit for UI when replayed
            if (frame.contains("message")) {
                QString note = frame["message"].toString();
                qint64 timestampVal = frame["timestamp_ms"].toVariant().toLongLong();
                emit replayBookmark(note, timestampVal);
            }

            // Hierarchy state: replay as usual
            QJsonValue JV = frame["snapshot"];
            if (JV.isObject()) {
                m_hierarchy->fromJson(JV.toObject());
                qDebug().noquote() << "Replayed frame" << currentIndex
                                   << "at timestamp:" << frame["timestamp_ms"].toInt()
                                   << "ms\n";
            } else if (!frame.contains("message")) {
                qWarning() << "Invalid snapshot type in frame" << currentIndex;
            }

            ++currentIndex;
        } else {
            qDebug() << "Playback completed.";
            recordingTimer->stop();
        }
    });

    recordingTimer->start(100); // replay interval = 0.1 sec
}

QJsonValue Recorder::getArrayElement(const QJsonArray &array, int index)
{
    if (array.isEmpty()) {
        qWarning() << "The provided JSON array is empty.";
        return QJsonValue();
    }

    if (index < 0 || index >= array.size()) {
        qWarning() << "Invalid index:" << index << ". Array size:" << array.size();
        return QJsonValue();
    }

    QJsonValue element = array.at(index);
    qDebug() << "Accessed element at index" << index << ":";

    if (element.isObject()) {
        qDebug().noquote() << QJsonDocument(element.toObject()).toJson(QJsonDocument::Indented);
    } else if (element.isArray()) {
        qDebug().noquote() << QJsonDocument(element.toArray()).toJson(QJsonDocument::Indented);
    } else {
        qDebug() << "Value:" << element;
    }

    return element;
}


// Start replaying recorded frames using a timer
void Recorder::startReplay()
{
    if (trajectoryArray.isEmpty()) {
        qWarning() << "No recorded data to replay.";
        return;
    }

    currentFrame = 0;
    replayTimer->start(100);  // Replay speed
}

// Stop the replay process
void Recorder::stopReplay()
{
    replayTimer->stop();
    currentFrame = 0;
    qDebug() << "Replay stopped.";
}

// Called by timer to emit the next frame in the replay
void Recorder::playNextFrame()
{
    if (currentFrame >= trajectoryArray.size()) {
        replayTimer->stop();
        qDebug() << "Replay finished.";
        return;
    }

    QJsonObject frame = trajectoryArray[currentFrame].toObject();
    //emit replayFrame(frame);   // Emit frame to connected slot
    currentFrame++;
}

// void Recorder::showBookmarkLog(const QString &note, qint64 timestampMs){
//     qDebug() << "By Recorder : Bookmark clicked : Note =" << note << ", Timestamp =" << timestampMs << "ms";
// }


void Recorder::bookmarkReplay(const QString &note, qint64 timestampMs){
    qDebug() << "By Recorder : Bookmark clicked : Note =" << note << ", Timestamp =" << timestampMs << "ms";
    // Stop any current playback
    if (recordingTimer) {
        recordingTimer->stop();
        delete recordingTimer;
        recordingTimer = nullptr;
    }
    // Start a new replay sequence from the given timestamp
    startReplayFromTimestamp(timestampMs);
}


QVector<QJsonObject> Recorder::getRecordedFrames() const {
    return recordedFrames;
}

// Reset internal state and buffers
void Recorder::clear()
{
    recordedData = QJsonObject();
    trajectoryArray = QJsonArray();
    currentFrame = 0;
}

// Set the sample rate for recording
void Recorder::setRate(int rate)
{
    sampleRate = rate;
    recordedData["sample_rate"] = rate;
}

// Get current sample rate
int Recorder::getRate() const
{
    return sampleRate;
}
