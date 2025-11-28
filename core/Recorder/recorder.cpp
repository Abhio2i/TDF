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

    bookmarkArray = QJsonArray();  // reset bookmark array on new recording

    if (!m_hierarchy) {
        qWarning() << "Hierarchy is null. Cannot record.";
        return;
    }

    QJsonObject metaData;
    metaData["bookmark"] = bookmarkArray;
    record(metaData);

    recordingStartTime = QDateTime::currentDateTime();
    pausedTimeOffset = 0;
    isRecordingPaused = false;

    if (!recordingTimer) {
        recordingTimer = new QTimer(this);
        connect(recordingTimer, &QTimer::timeout, this, [this]() {
            qint64 elapsedMs;
            if (isRecordingPaused) {
                elapsedMs = lastElapsedMs;  // freeze playback time
            } else {
                elapsedMs = recordingStartTime.msecsTo(QDateTime::currentDateTime()) - pausedTimeOffset;
                lastElapsedMs = elapsedMs;
            }
            emit recordingTimeUpdated(elapsedMs);
            QJsonObject timeEntry;
            timeEntry["timestamp_ms"] = elapsedMs;
            timeEntry["current_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            timeEntry["snapshot"] = m_hierarchy->toJson();   // SAFE now

            record(timeEntry);
        });

    }

    recordingTimer->start(100);
}
// void Recorder::startRecording()
// {
//     qDebug() << "Recording started.";
//     clear();

//     bookmarkArray = QJsonArray();  // reset bookmarks

//     if (!m_hierarchy) {
//         qWarning() << "Hierarchy is null. Cannot record.";
//         return;
//     }

//     // ---- Write metadata as first frame ----
//     QJsonObject metaData;
//     metaData["bookmark"] = bookmarkArray;
//     record(metaData);

//     // ---- Reset timers ----
//     recordingStartTime = QDateTime::currentDateTime();
//     pausedTimeOffset = 0;
//     lastElapsedMs = 0;
//     isRecordingPaused = false;

//     // ---- Create timer only once ----
//     if (!recordingTimer) {
//         recordingTimer = new QTimer(this);

//         connect(recordingTimer, &QTimer::timeout, this, [this]() {

//             qint64 elapsedMs = 0;

//             if (isRecordingPaused)
//             {
//                 // Freeze time during pause
//                 elapsedMs = lastElapsedMs;
//             }
//             else
//             {
//                 // Actual running time minus total paused duration
//                 elapsedMs =
//                     recordingStartTime.msecsTo(QDateTime::currentDateTime())
//                     - pausedTimeOffset;

//                 lastElapsedMs = elapsedMs;
//             }

//             // Update left side UI timer
//             emit recordingTimeUpdated(elapsedMs);

//             // Save snapshot
//             QJsonObject frame;
//             frame["timestamp_ms"] = elapsedMs;
//             frame["current_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
//             frame["snapshot"] = m_hierarchy->toJson();

//             // Write frame
//             record(frame);
//         });
//     }

//     // Start continuous recording
//     recordingTimer->start(100);
// }


// void Recorder::togglePause()
// {
//     if (!recordingTimer) return;

//     if (recordingTimer->isActive()) {
//         // Pause
//         recordingTimer->stop();
//         isRecordingPaused = true;
//         pauseStartTime = QDateTime::currentDateTime();
//         qDebug() << "[Recorder] Paused at" << pauseStartTime;
//         emit recordingPaused();
//         return;
//     }
//     if (isRecordingPaused) {
//         // Resume
//         qint64 pausedDuration = pauseStartTime.msecsTo(QDateTime::currentDateTime());
//         pausedTimeOffset += pausedDuration;
//         if (!recordingTimer->isActive()){
//             recordingTimer->start(500);
//         }

//         isRecordingPaused = false;

//         qDebug() << "[Recorder] Resumed. Pause duration =" << pausedDuration;

//         emit recordingResumed();           // UI updates
//         return;
//     }
// }
void Recorder::togglePause()
{
    if (!recordingTimer) return;

    if (!isRecordingPaused)
    {
        // Enter pause
        isRecordingPaused = true;
        pauseStartTime = QDateTime::currentDateTime();

        qDebug() << "Recording Paused.";
        emit recordingPaused();
    }
    else
    {
        // Resume
        qint64 pausedDuration = pauseStartTime.msecsTo(QDateTime::currentDateTime());
        pausedTimeOffset += pausedDuration;

        isRecordingPaused = false;

        qDebug() << "Recording Resumed.";
        emit recordingResumed();
    }
}
// void Recorder::togglePause()
// {
//     if (!recordingTimer) {
//         qWarning() << "Recording timer not active. Cannot toggle pause.";
//         return;
//     }

//     // ---- PAUSE ----
//     if (recordingTimer->isActive()) {

//         recordingTimer->stop();
//         isRecordingPaused = true;

//         pauseStartTime = QDateTime::currentDateTime();

//         qDebug() << "Recording Paused.";

//         emit recordingPaused();   // Notify UI (optional)
//     }

//     // ---- RESUME ----
//     else if (isRecordingPaused) {

//         // Measure how long we remained paused
//         qint64 pausedDuration =
//             pauseStartTime.msecsTo(QDateTime::currentDateTime());

//         pausedTimeOffset += pausedDuration;

//         // Resume timer at the same 100 ms interval
//         recordingTimer->start(100);
//         isRecordingPaused = false;

//         qDebug() << "Recording Resumed. Offset added:" << pausedDuration << "ms";

//         emit recordingResumed();  // Notify UI (optional)
//     }
// }







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

void Recorder::recordBookmark(const QString &message, qint64 timestampMs)
{
    if (!recordingStartTime.isValid()) {
        qWarning() << "Cannot save bookmark — recording is not active.";
        return;
    }

    // Make sure metadata frame exists
    if (m_recordings.isEmpty()) {
        qWarning() << "Bookmark cannot be stored — metadata frame missing.";
        return;
    }

    // Convert first frame properly
    QJsonObject firstFrame = m_recordings[0].toObject();

    // Read or create "bookmark" array
    QJsonArray bookmarkArray;
    if (firstFrame.contains("bookmark")) {
        bookmarkArray = firstFrame["bookmark"].toArray();
    }

    // Create new bookmark entry
    QJsonObject bookmarkEntry;
    bookmarkEntry["timestamp_ms"]  = timestampMs;
    bookmarkEntry["current_time"]  = QDateTime::currentDateTime().toString(Qt::ISODate);
    bookmarkEntry["message"]       = message;

    // Append to array
    bookmarkArray.append(bookmarkEntry);

    // Set back into metadata frame
    firstFrame["bookmark"] = bookmarkArray;

    // Write the modified object back to index 0
    m_recordings[0] = firstFrame;

    qDebug().noquote() << "Bookmark updated at" << timestampMs << "ms — message:" << message;
}

// void Recorder::recordBookmark(const QString &message, qint64 timestampMs)
// {
//     if (!recordingStartTime.isValid()) {
//         qWarning() << "Cannot save bookmark — recording is not active.";
//         return;
//     }

//     // Create bookmark entry
//     QJsonObject bookmarkEntry;
//     bookmarkEntry["timestamp_ms"] = timestampMs;
//     bookmarkEntry["current_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
//     bookmarkEntry["message"] = message;

//     // Append new entry to shared bookmark array
//     bookmarkArray.append(bookmarkEntry);

//     // Wrap inside metaData → bookmark → bookmarkEntry
//     QJsonObject metaData;
//     metaData["bookmark"] = bookmarkArray;   // persistent array updated

//     // Add this structured block to main recording frames
//     record(metaData);

//     qDebug().noquote() << "Bookmark saved at" << timestampMs << "ms — message:" << message;
// }

// Store a single frame of recording into trajectory array
void Recorder::recordFrame(const QJsonObject &frame)
{
    trajectoryArray.append(frame);   // Add to array
    recordedData["trajectories"] = trajectoryArray;  // Update in main object
}

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



bool Recorder::loadReplay(const QString &filePath)
{
    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open replay file.");
        return false;
    }

    QByteArray saveData = loadFile.readAll();
    loadFile.close();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    if (!loadDoc.isArray()) {
        qWarning("Replay file format error: Expected JSON array.");
        return false;
    }

    QJsonArray array = loadDoc.array();
    playbackFrames.clear();
    playbackFrames.reserve(array.size());

    // Convert JSON array to QVector
    for (const QJsonValue &val : array) {
        if (val.isObject())
            playbackFrames.append(val.toObject());
    }

    qDebug() << "Replay loaded with" << playbackFrames.size() << "frames.";
    // --- Handle bookmarks array (multiple entries) ---
    // Step 1: extract metadata (frame 0)
    const QJsonObject &meta = playbackFrames[0];
    qint64 maxTimestamp = 0;

    // Compute duration
    for (const QJsonObject &frm : playbackFrames) {
        if (frm.contains("timestamp_ms")) {
            maxTimestamp = qMax(maxTimestamp, frm["timestamp_ms"].toVariant().toLongLong());
        }
    }

    // Step 2 — FIRST notify UI about duration
    emit setReplayDuration(maxTimestamp);

    // Step 3 — Load bookmarks ONLY after duration is set
    if (meta.contains("bookmark") && meta["bookmark"].isArray()) {
        QJsonArray arr = meta["bookmark"].toArray();

        for (const QJsonValue &v : arr) {
            if (!v.isObject()) continue;
            QJsonObject b = v.toObject();

            QString message = b["message"].toString();
            qint64 timestamp = b["timestamp_ms"].toVariant().toLongLong();

            emit replayBookmark(message, timestamp);   // UI now has duration; markers will show
        }
    }

    // ===== Load first frame (SKIP metadata at index 0) =====
    if (playbackFrames.size() > 1) {
        currentReplayIndex = 1;   // Start from snapshot frame

        const QJsonObject &firstSnapshot = playbackFrames.at(1);
        if (firstSnapshot.contains("snapshot")) {
            QJsonObject snapshotObj = firstSnapshot["snapshot"].toObject();
            if (m_hierarchy) {
                m_hierarchy->fromJson(snapshotObj);
                qDebug() << "Loaded first snapshot frame at index 1.";
            }

            emit frameLoaded(firstSnapshot);  // Optional UI signal
        }
        else {
            qWarning() << "First snapshot frame missing 'snapshot' property.";
        }
    }
    else {
        qWarning() << "Not enough frames to replay (snapshot required).";
        return false;
    }

    return true;
}

void Recorder::startReplay()
{
    if (playbackFrames.isEmpty()) {
        qWarning() << "No frames available to replay.";
        return;
    }
    currentReplayIndex = 1;
    if (!recordingTimer) {
        recordingTimer = new QTimer(this);
        connect(recordingTimer, &QTimer::timeout, this, [this]() {
            if (currentReplayIndex < playbackFrames.size()) {

                const QJsonObject &frame = playbackFrames.at(currentReplayIndex);

                QJsonValue jv = frame["snapshot"];
                if (jv.isObject()) {
                    m_hierarchy->fromJson(jv.toObject());

                    qint64 ts = frame["timestamp_ms"].toVariant().toLongLong();

                    qDebug() << "Frame" << currentReplayIndex
                             << "Timestamp:" << ts << "ms";

                    // New line — tells UI where we are in timeline
                    emit replayFrameLoaded(ts);
                }

                currentReplayIndex++;
            } else {
                qDebug() << "Replay complete";
                recordingTimer->stop();
            }
        });
    }

    recordingTimer->start(100);
    qDebug() << "Replay started.";
}



void Recorder::toggleReplayPause()
{
    if (!recordingTimer) {
        qWarning() << "Replay timer not active. Cannot toggle pause.";
        return;
    }

    // If currently running → pause
    if (recordingTimer->isActive()) {
        recordingTimer->stop();
        isReplayPaused = true;
        replayPauseStartTime = QDateTime::currentDateTime();
        qDebug() << "Replay Paused.";
    }
    // If paused → resume
    else if (isReplayPaused) {
        qint64 pausedDuration = replayPauseStartTime.msecsTo(QDateTime::currentDateTime());
        replayPausedTimeOffset += pausedDuration;

        recordingTimer->start(100); // match replay interval
        isReplayPaused = false;

        qDebug() << "Replay Resumed. Offset added:" << pausedDuration << "ms";
    }
}

void Recorder::goToNextFrame()
{
    if (playbackFrames.isEmpty()) return;

    if (currentReplayIndex < playbackFrames.size() - 50) {
        currentReplayIndex += 50;
        //currentReplayIndex++;
        const QJsonObject &frame = playbackFrames.at(currentReplayIndex);

        if (frame.contains("snapshot")) {
            m_hierarchy->fromJson(frame["snapshot"].toObject());
        }

        qDebug() << "Moved to next frame:" << currentReplayIndex;
    }
}

void Recorder::goToPreviousFrame()
{
    if (playbackFrames.isEmpty()) return;

    // if (recordingTimer && recordingTimer->isActive()) {
    //     recordingTimer->stop();
    //     isReplayPaused = true;
    // }

    if (currentReplayIndex > 50) {
        currentReplayIndex -= 50;
        const QJsonObject &frame = playbackFrames.at(currentReplayIndex);

        if (frame.contains("snapshot")) {
            m_hierarchy->fromJson(frame["snapshot"].toObject());
        }

        qDebug() << "Moved to previous frame:" << currentReplayIndex;
    }
}
void Recorder::playAgain()
{
    if (playbackFrames.isEmpty()) {
        qWarning() << "No frames loaded for replay.";
        return;
    }

    // Reset replay index
    currentReplayIndex = 0;

    // Stop existing timer if running
    if (recordingTimer) {
        recordingTimer->stop();
        delete recordingTimer;
        recordingTimer = nullptr;
    }

    // Create timer again
    recordingTimer = new QTimer(this);

    connect(recordingTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentReplayIndex < playbackFrames.size()) {
            const QJsonObject &frame = playbackFrames.at(currentReplayIndex);

            // Replay hierarchy snapshot
            if (frame.contains("snapshot") && frame["snapshot"].isObject()) {
                m_hierarchy->fromJson(frame["snapshot"].toObject());
                qint64 ts = frame["timestamp_ms"].toVariant().toLongLong();

                qDebug() << "Frame" << currentReplayIndex
                         << "Timestamp:" << ts << "ms";

                // New line — tells UI where we are in timeline
                emit replayFrameLoaded(ts);
                emit frameLoaded(frame);  // optional signal for UI update

            }

            ++currentReplayIndex;
        } else {
            qDebug() << "Replay finished.";
            recordingTimer->stop();
        }
    });

    // Start replay
    recordingTimer->start(100); // interval = 100 ms
    qDebug() << "Replay restarted from frame 0.";
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

    int currentReplayIndex = startIdx;
    if (recordingTimer) {
        recordingTimer->stop();
        delete recordingTimer;
        recordingTimer = nullptr;
    }
    recordingTimer = new QTimer(this);

    connect(recordingTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentReplayIndex < playbackFrames.size()) {
            const QJsonObject &frame = playbackFrames.at(currentReplayIndex);

            // Hierarchy state: replay as usual
            QJsonValue JV = frame["snapshot"];
            if (JV.isObject()) {
                m_hierarchy->fromJson(JV.toObject());
                qint64 ts = frame["timestamp_ms"].toVariant().toLongLong();

                qDebug() << "Frame" << currentReplayIndex
                         << "Timestamp:" << ts << "ms";
                emit replayFrameLoaded(ts);
                emit frameLoaded(frame);  // optional signal for UI update
            }

            ++currentReplayIndex;
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

void Recorder::resetReplayState()
{
    qDebug() << "Resetting replay data...";

    // Stop timer if active
    if (recordingTimer && recordingTimer->isActive())
        recordingTimer->stop();

    // Clear all loaded frames
    playbackFrames.clear();

    // Reset index
    currentReplayIndex = 0;

    // Optional: hierarchy reset
    // if (m_hierarchy) {
    //     QJsonObject empty;
    //     m_hierarchy->fromJson(empty);
    // }

    qDebug() << "Replay data cleared.";
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
