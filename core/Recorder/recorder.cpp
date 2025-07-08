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

// Start recording: clears old data and prepares to record new session
void Recorder::startRecording()
{
    qDebug() << "Recording started.";
    clear();    // Reset previous recordings

    if (!m_hierarchy) {
        qWarning() << "Hierarchy is null. Cannot record.";
        return;
    }

    QJsonObject jsonData = m_hierarchy->toJson(); // Convert full structure
    jsonData["recording_rate"] = getRate();   // Include sample rate
    record(jsonData);  // Include sample rate
}

// Stop recordings
void Recorder::stopRecording()
{
    qDebug() << "Recording stopped.";
}

// Convert hierarchy to JSON and save it to file
void Recorder::recordToJson()
{
    saveToFile();  // Automatically save to file
}

// Store provided JSON data in internal buffer
void Recorder::record(const QJsonObject &data)
{
    recordedData = data;
}

// Store a single frame of recording into trajectory array
void Recorder::recordFrame(const QJsonObject &frame)
{
    trajectoryArray.append(frame);   // Add to array
    recordedData["trajectories"] = trajectoryArray;  // Update in main object
}

// Save recorded data to a JSON file
bool Recorder::saveToFile(const QString &filePaths)
{
    QString finalPath = filePaths;
    QString filePath = QFileDialog::getSaveFileName(nullptr, "Save JSON",  QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"), "JSON Files (*.json)");
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(recordedData);
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            QMessageBox::information(nullptr, "Saved", "JSON saved successfully");
        }
    }

    //finalPath = directory + "/recording_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".json";


    // QFile file(finalPath);
    // if (!file.open(QIODevice::WriteOnly)) {
    //     qWarning() << "Failed to open file for saving:" << finalPath;
    //     return false;
    // }

    // QJsonDocument doc(recordedData);
    // file.write(doc.toJson(QJsonDocument::Indented));
    // file.close();

    qDebug() << "Recording saved to:" << finalPath;

    // Open the folder where file is saved (auto open location)
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(finalPath).absolutePath()));
    return true;
}

// Load recording from JSON file into memory
bool Recorder::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << filePath;
        return false;
    }

    // Parse JSON and extract trajectory data
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    recordedData = doc.object();
    m_hierarchy->fromJson(doc.object());
    trajectoryArray = recordedData["trajectories"].toArray(); // Load frames
    currentFrame = 0;
    file.close();

    qDebug() << "Recording loaded. Total frames:" << trajectoryArray.size();
    return true;
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
