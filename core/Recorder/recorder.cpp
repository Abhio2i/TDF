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

// // Start recording: clears old data and prepares to record new session
// void Recorder::startRecording()
// {
//     qDebug() << "Recording started.";
//     clear();    // Reset previous recordings

//     if (!m_hierarchy) {
//         qWarning() << "Hierarchy is null. Cannot record.";
//         return;
//     }

//     QJsonObject jsonData = m_hierarchy->toJson(); // Convert full structure
//     jsonData["recording_rate"] = getRate();   // Include sample rate
//     record(jsonData);  // Include sample rate
// }
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
            timeEntry["snapshot"] = m_hierarchy->toJson();

            record(timeEntry);

            qDebug() << "Recorded interval at" << elapsedMs << "ms";
        });
    }

    int intervalMs = 100; // record every 1 second
    recordingTimer->start(intervalMs);
}


// // Stop recordings
// void Recorder::stopRecording()
// {
//     qDebug() << "Recording stopped.";
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

    // Display the entire JSON recording
    if (!m_recordings.isEmpty()) {
        QJsonDocument doc(m_recordings);
        QString jsonString = doc.toJson(QJsonDocument::Indented); // Pretty print
        qDebug() << "Full Recording JSON:\n" << jsonString;
    } else {
        qDebug() << "No recordings to display.";
    }

    qDebug() << "Recorder cleanup done.";
}




// Convert hierarchy to JSON and save it to file
void Recorder::recordToJson()
{
    saveToFile();  // Automatically save to file
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

// Save recorded data to a JSON file
// bool Recorder::saveToFile(const QString &filePaths)
// {
//     QString finalPath = filePaths;
//     QString filePath = QFileDialog::getSaveFileName(nullptr, "Save JSON",  QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"), "JSON Files (*.json)");
//     if (!filePath.isEmpty()) {
//         QFile file(filePath);
//         if (file.open(QIODevice::WriteOnly)) {
//             QJsonDocument doc(recordedData);
//             file.write(doc.toJson(QJsonDocument::Indented));
//             file.close();
//             QMessageBox::information(nullptr, "Saved", "JSON saved successfully");
//         }
//     }

//     //finalPath = directory + "/recording_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".json";


//     // QFile file(finalPath);
//     // if (!file.open(QIODevice::WriteOnly)) {
//     //     qWarning() << "Failed to open file for saving:" << finalPath;
//     //     return false;
//     // }

//     // QJsonDocument doc(recordedData);
//     // file.write(doc.toJson(QJsonDocument::Indented));
//     // file.close();

//     qDebug() << "Recording saved to:" << finalPath;

//     // Open the folder where file is saved (auto open location)
//     QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(finalPath).absolutePath()));
//     return true;
// }
bool Recorder::saveToFile()
{
    if (m_recordings.isEmpty()) {
        qWarning() << "No recordings to save!";
        return false;
    }
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recordings";
    QDir().mkpath(directory); // ensure directory exists
    // Build file path with timestamp
    QString finalPath = directory + "/recorder" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".json";

    QFile file(finalPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for saving:" << finalPath;
        return false;
    }

    // Convert recorded snapshots to JSON document
    QJsonDocument doc(m_recordings);

    // Write JSON to file
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "Recording saved to:" << finalPath;

    // Optionally open the folder containing the file
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(finalPath).absolutePath()));

    return true;
}


// Load recording from JSON file into memory
// bool Recorder::loadFromFile(const QString &filePath)
// {
//     QFile file(filePath);
//     if (!file.open(QIODevice::ReadOnly)) {
//         qWarning() << "Failed to open file for loading:" << filePath;
//         return false;
//     }

//     QByteArray data = file.readAll();
//     file.close();

//     QJsonParseError parseError;
//     QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
//     if (parseError.error != QJsonParseError::NoError) {
//         qWarning() << "JSON parse error:" << parseError.errorString();
//         return false;
//     }

//     if (!doc.isObject()) {
//         qWarning() << "Invalid JSON format in file:" << filePath;
//         return false;
//     }

//     QJsonObject jsonObj = doc.object();

//     // Example: Rebuild your recorder’s state
//     recordedData = jsonObj;  // Assuming 'recordedData' is your QJsonObject holding current state

//     qDebug() << "Recording loaded successfully from:" << filePath;
//     qDebug().noquote() << QJsonDocument(recordedData).toJson(QJsonDocument::Indented);

//     return true;
// }
// bool Recorder::loadFromFile(const QString &filePath)
// {
//     QFile file(filePath);
//     if (!file.open(QIODevice::ReadOnly)) {
//         qWarning() << "Failed to open file for loading:" << filePath;
//         return false;
//     }

//     QByteArray data = file.readAll();
//     file.close();

//     QJsonParseError parseError;
//     QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
//     if (parseError.error != QJsonParseError::NoError) {
//         qWarning() << "JSON parse error:" << parseError.errorString();
//         return false;
//     }

//     if (doc.isObject()) {
//         recordedData = doc.object();
//     } else if (doc.isArray()) {
//         // Wrap array in object if needed
//         QJsonObject wrapper;
//         wrapper["recordings"] = doc.array();
//         recordedData = wrapper;
//     } else {
//         qWarning() << "Invalid JSON format in file:" << filePath;
//         return false;
//     }
//     // QJsonDocument doc(recordedData);
//     file.write(doc.toJson(QJsonDocument::Indented));
//     qDebug().noquote() << "File contents:\n" << data;

//     // qDebug() << "Recording loaded successfully from:" << filePath;
//     // qDebug().noquote() << QJsonDocument(recordedData).toJson(QJsonDocument::Indented);
//     return true;
// }
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
    QVector<QJsonObject> playbackFrames;
    playbackFrames.reserve(jsonArray.size());

    for (const QJsonValue &value : jsonArray) {
        if (value.isObject()) {
            playbackFrames.append(value.toObject());
        }
    }

    qDebug() << "Loaded" << playbackFrames.size() << "frames for playback.";

    // Start playback timer
    int currentIndex = 0;
    recordingTimer = new QTimer(this);

    connect(recordingTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentIndex < playbackFrames.size()) {
            const QJsonObject &frame = playbackFrames.at(currentIndex);
            QJsonValue JV = frame["snapshot"];

            if (JV.isObject()) {
                m_hierarchy->fromJson(JV.toObject());
                qDebug().noquote() << "Replayed frame" << currentIndex
                                   << "at timestamp:" << frame["timestamp_ms"].toInt()
                                   << "ms\n"
                                   << QJsonDocument(JV.toObject()).toJson(QJsonDocument::Indented);
            } else {
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

// bool Recorder::loadFromFile(const QString &filePath)
// {
//     QFile file(filePath);
//     if (!file.open(QIODevice::ReadOnly)) {
//         qWarning() << "Failed to open file for loading:" << filePath;
//         return false;
//     }

//     QByteArray data = file.readAll();
//     file.close();

//     QJsonParseError parseError;
//     QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
//     if (parseError.error != QJsonParseError::NoError) {
//         qWarning() << "JSON parse error:" << parseError.errorString();
//         return false;
//     }

//     if (doc.isObject()) {
//         recordedData = doc.object();
//     } else if (doc.isArray()) {
//         QJsonArray doc2 = doc.array();
//         // QJsonValue JV = doc2.at(1)["snapshot"];
//         // qDebug().noquote() << "File contents:\n" << JV;
//         // Make sure the index exists
//         qsizetype arr_size = doc2.size();
//         for(qsizetype i = 0 ; i <arr_size ;i++){
//             qDebug()<<"JV No."<<i;
//             if (doc2.size() > 1 && doc2.at(i).isObject()) {
//                 QJsonObject obj = doc2.at(i).toObject();  // convert QJsonValue → QJsonObject
//                 QJsonValue JV = obj["snapshot"];          // now access the key safely

//                 qDebug().noquote() << "Snapshot content:\n"
//                                    << QJsonDocument(JV.toObject()).toJson(QJsonDocument::Indented);

//                 if (JV.isObject()) {
//                     m_hierarchy->fromJson(JV.toObject());
//                 } else {
//                     qWarning() << "Expected a JSON object for hierarchy data, but got:" << JV;
//                 }

//             } else {
//                 qWarning() << "Invalid index or element is not an object!";
//             }

//         }
//         // if (doc2.size() > 1 && doc2.at(1).isObject()) {
//         //     QJsonObject obj = doc2.at(1).toObject();  // convert QJsonValue → QJsonObject
//         //     QJsonValue JV = obj["snapshot"];          // now access the key safely

//         //     qDebug().noquote() << "Snapshot content:\n"
//         //                        << QJsonDocument(JV.toObject()).toJson(QJsonDocument::Indented);

//         //     if (JV.isObject()) {
//         //         m_hierarchy->fromJson(JV.toObject());
//         //     } else {
//         //         qWarning() << "Expected a JSON object for hierarchy data, but got:" << JV;
//         //     }

//         // } else {
//         //     qWarning() << "Invalid index or element is not an object!";
//         // }

//     } else {
//         qWarning() << "Invalid JSON format in file:" << filePath;
//         return false;
//     }

//     //QJsonDocument doc(recordedData);
//     // file.write(doc.toJson(QJsonDocument::Indented));
//     // qDebug().noquote() << "File contents:\n" << data;
//     return true;
// }

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
    emit replayFrame(frame);   // Emit frame to connected slot
    currentFrame++;
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
