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

/* -------------------------------------------------------
 * Recording Implementation Information Start
 * ------------------------------------------------------*/

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

    m_recording = new Recording(m_hierarchy, m_simulation, this, this);  // parented to Recorder
    m_replay    = new Replay   (m_hierarchy, m_simulation, this, this);
}


//Common File
void Recorder::loggerModeCheck(loggerModes mode)
{
    recordingStartTime = QDateTime::currentDateTime();
    duration = 0;
    switch(mode){
    case REPLAY:
        modeOfLogger = mode;
        m_replay->update();
        break;
    case RECORDING:
        modeOfLogger = mode;
        m_recording->update();
        break;
    default:
        break;
    }
}

void Recorder::loggerInfo()
{
    int seconds = duration / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    QString durationText = QString("%1:%2:%3")
                               .arg(hours, 2, 10, QLatin1Char('0'))
                               .arg(minutes % 60, 2, 10, QLatin1Char('0'))
                               .arg(seconds % 60, 2, 10, QLatin1Char('0'));
   // qDebug()<<"Recording: Information of Logger on Change:"
             // <<"\n\t Logger Mode  : "<<modeOfLogger
             // <<"\n\t Starting Time: "<<recordingStartTime
             // <<"\n\t Duration     : "<<durationText
             // <<"\n\t Status       : "<<loggerStatus
             // <<"\n\t Simulation   : "<<simulationStatus;
}

void Recorder::update(LoggerStatusModes loggerStatus)
{
    emit recorderInfoSend(loggerStatus);
}

/* -------------------------------------------------------
 * Recording Implementation Information End
 * ------------------------------------------------------*/



/* -------------------------------------------------------
 * Recording Implementation Start
 * ------------------------------------------------------*/

Recording::Recording(
    Hierarchy* hierarchy,
    Simulation* simulation,
    Recorder *parentRecorder,
    QObject *parent) :
    QObject(parent),
    m_hierarchy(hierarchy),
    m_simulation(simulation),
    m_recorder(parentRecorder)

{
    m_recorder = getRecorder();
}


void Recording::update()
{
    m_recorder->recordingStartTime = QDateTime::currentDateTime();
    m_recorder->duration           = 0;
    m_recorder->loggerStatus       = Recorder::S_RECORDING_MODE;
    m_recorder->simulationStatus   = Recorder::S_SIMULATION_NA;
    emit m_recorder->recorderInfoSendOnce(
        m_recorder->recordingStartTime ,
        m_recorder->duration           ,
        m_recorder->loggerStatus       ,
        m_recorder->simulationStatus   );
    // m_recorder->loggerInfo();
}


void Recording::start()
{
    //m_recorder = getRecorder();
    m_recorder->loggerStatus = Recorder::S_RECORDING;
    m_recorder->update(m_recorder->loggerStatus);
   // qDebug()<<"Recording: Information of Logger on Pressing Start Button"
             // <<"\n\t Status       : "<<m_recorder->loggerStatus;
    // m_recorder->loggerInfo();
    recordingStart();
}

void Recording::pause()
{
    //m_recorder = getRecorder();
    m_recorder->loggerStatus = Recorder::S_RECORDING_PAUSED;
    m_recorder->update(m_recorder->loggerStatus);
   // qDebug()<<"Recording: Information of Logger on Pressing Start Button"
             // <<"\n\t Status       : "<<m_recorder->loggerStatus;
    m_recorder->loggerInfo();
    recordingPauseResume();
}

void Recording::resume()
{
    //m_recorder = getRecorder();
    m_recorder->loggerStatus = Recorder::S_RECORDING;
    m_recorder->update(m_recorder->loggerStatus);
   // qDebug()<<"Recording: Information of Logger on Pressing Start Button"
             // <<"\n\t Status       : "<<m_recorder->loggerStatus;
    m_recorder->loggerInfo();
    recordingPauseResume();
}

void Recording::stop()
{
    m_recorder = getRecorder();
    m_recorder->loggerStatus = Recorder::S_RECORDING_STOPPED;
    m_recorder->update(m_recorder->loggerStatus);
   // qDebug()<<"Recording: Information of Logger on Pressing Start Button"
             // <<"\n\t Status       : "<<m_recorder->loggerStatus;
    m_recorder->loggerInfo();
    recordingStop();
}

void Recording::addBookmark()
{
    return;
}

void Recording::insertRecord(const QJsonObject &data)
{
    recordedFile.append(data);
    recordedData = data;
}
void Recording::recordingStart()
{
    m_recorder = getRecorder();

    // Ensure consistent state
    mode = START;
    recordingStartTime = QDateTime::currentDateTime();
    pausedOffsetMs = 0;         // accumulated time before current segment
    lastElapsedMs = 0;
    isPaused = false;
    noOfFrame = 1;

   // qDebug() <<"Recording: Information of Logger "
             // <<"on Pressing Start Button\n"
             // <<"Start Recording:";

    // Reset bookmark array on new recording
    bookmarks = QJsonArray();
    if (!m_hierarchy) {
        qWarning() << "Hierarchy is null. Cannot record.";
        return;
    }
    /* New Type of recording Start */
    recordedStructure = QJsonObject();
    rs_hierarchyObj   = QJsonObject();
    rs_hierarchyObj.insert("timestamp_ms" , lastElapsedMs);
    rs_hierarchyObj.insert("current_time" , QDateTime::currentDateTime().toString(Qt::ISODate));
    rs_hierarchyObj.insert("hierarchy"    , m_hierarchy->toJson());
    rs_hierarchy      = QJsonArray();
    rs_hierarchy.append(rs_hierarchyObj);

    rs_bookmark = QJsonArray();
    rs_frame    = QJsonArray();

    /* New Type of recording End   */

    // Insert metadata frame (frame 0)
    QJsonObject metaData;
    metaData["bookmark"] = QJsonArray();
    insertRecord(metaData);                // stored at recordedFile[0]

    currentDateTime = QDateTime::currentDateTime();
    recordingPeriod = 100;

    // Create timer once (if not already created)
    if (!recordingTimer) {
        recordingTimer = new QTimer(this);

        // Timer lambda must capture this so it sees current members
        connect(recordingTimer, &QTimer::timeout, this, [this]() {
            qint64 elapsedMs = 0;
            if (mode == STOP) {
                // freeze
                elapsedMs = lastElapsedMs;
                // Addding last value;
               // qDebug()<<"Stop is called in loop";
            }
            else if (mode == PAUSE) {
                // when paused, keep the last elapsed value
                elapsedMs = lastElapsedMs;
                // emit time update for UI if needed, but do not add frames
                emit m_recorder->recordingTimeUpdated(elapsedMs);
                return;  // do not insert a new snapshot while paused
            }
            else { // START
                // total time = already-accumulated + current segment
                elapsedMs = pausedOffsetMs + recordingStartTime.msecsTo(QDateTime::currentDateTime());
                lastElapsedMs = elapsedMs;
                //Adding Additional logic

            }

            // Emit timeline update
            emit m_recorder->recordingTimeUpdated(elapsedMs);

            // Insert a snapshot frame only when recording (not on STOP/PAUSE)
            if(changeInHierarchyInPause){
                //Adding last value;
                rs_hierarchyObj   = QJsonObject();
                rs_hierarchyObj.insert("timestamp_ms" , elapsedMs);
                rs_hierarchyObj.insert("current_time" , QDateTime::currentDateTime().toString(Qt::ISODate));
                rs_hierarchyObj.insert("hierarchy"    , m_hierarchy->toJson());

                changeInHierarchyInPause = false;

            }
            if (mode == START) {
                // QJsonObject timeEntry;
                // timeEntry["timestamp_ms"] = elapsedMs;
                // timeEntry["current_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
                // timeEntry["snapshot"] = m_hierarchy->toJson();
                // insertRecord(timeEntry);
                //// qDebug() <<"\t"<< noOfFrame++ <<". Frame"<< elapsedMs;

                //Creating New Frame Data Object
                rs_frameObj = QJsonObject();
                rs_frameObj.insert("timestamp_ms" , elapsedMs);
                rs_frameObj.insert("current_time" , QDateTime::currentDateTime().toString(Qt::ISODate));
                rs_frameObj.insert("frameEntities",getFrameEntitiesData());

                //Adding to frame array
                rs_frame.append(rs_frameObj);
                //qDebug()<<"\t"<<rs_frameObj;
            }
        });
    }

    // Start (or restart) timer
    if (!recordingTimer->isActive())
        recordingTimer->start(recordingPeriod);

    // notify UI / outer systems
    emit started();
}

QJsonArray Recording::getFrameEntitiesData()
{
    //Resetting Frame Entities Array
    frameEntities = QJsonArray();
    std::unordered_map<std::string, Platform*> *m_Platforms = m_hierarchy->Platforms;
    for(const auto& platform : *m_Platforms){
        //Resetting Entity Object
        entityObj = QJsonObject();
        Entity* entity = platform.second;
        //Getting Name
        const std::string& name = entity->Name.c_str();
        const std::string& id   = entity->ID.c_str();
        //Getting Lat & Long and Alt
        QVector3D     matrix       = platform.second->transform->translation();
        //Rotation angle
        QVector3D     angle_matrix = platform.second->transform->toEulerAngles();
        //Current Speed
        DynamicModel* dynamicModel = platform.second->dynamicModel;

        if (entity) {
            //Setting the name and ID
            //// qDebug()<<"Entity Name: "<<name.c_str()
            //         <<", ID: "<<id.c_str();
            entityObj.insert("name",name.c_str());
            entityObj.insert("id"  ,id.c_str());

            //Setting Coorinates
            //// qDebug()<<"\tCoorinates"
            //         <<" , "<<matrix.x()
            //         <<" , "<<matrix.y()
            //         <<" , "<<matrix.z();
            entityObj.insert("cord_x",matrix.x());
            entityObj.insert("cord_y",matrix.y());
            entityObj.insert("cord_z",matrix.z());

            //Setting Rotation axies
            //// qDebug()<<"\tRotation axies"
            //         <<" : "<<angle_matrix.x()
            //         <<" , "<<angle_matrix.y()
            //         <<" , "<<angle_matrix.z();
            entityObj.insert("axis_x",angle_matrix.x());
            entityObj.insert("axis_y",angle_matrix.y());
            entityObj.insert("axis_z",angle_matrix.z());

            //Setting Dynamic values
            //// qDebug()<<"\tDynamic Components: "
            //         <<" Turn Radius "  <<dynamicModel->turnRadius
            //         <<" Maximum Speed "<<dynamicModel->moveSpeed
            //         <<" Current Speed "<<dynamicModel->currentSpeed;
            entityObj.insert("turn_radius",dynamicModel->turnRadius);
            entityObj.insert("max_speed"  ,dynamicModel->moveSpeed);
            entityObj.insert("curr_speed" ,dynamicModel->currentSpeed);

        }
        //qDebug()<<entityObj;
        frameEntities.append(entityObj);
        //End
    }
    return frameEntities;
}

// QJsonObject Recording::getFrameEntitiesData(Platform *platform)
// {
//     QJsonObject jsonEntity;
//     return jsonEntity;
// }

void Recording::recordingPauseResume()
{
    if (!recordingTimer) return;

    if (mode == START)
    {
        // Enter pause
        mode = PAUSE;
        // accumulate the time we've recorded in this segment
        if (recordingStartTime.isValid()) {
            qint64 segmentMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());
            pausedOffsetMs += segmentMs;
            lastElapsedMs = pausedOffsetMs; // freeze value
        }
        pauseStartTime = QDateTime::currentDateTime();
        isPaused = true;

       // qDebug() <<"Recording: Information of Logger "
                 // <<"on Pressing Pause\n"
                 // <<"Pause Recording:";
        emit paused();
    }
    else if (mode == PAUSE)
    {
        // Resume
        mode = START;
        // reset start reference for the new segment
        recordingStartTime = QDateTime::currentDateTime();
        isPaused = false;

       // qDebug() <<"Recording: Information of Logger "
                 // <<"on Pressing Resume\n"
                 // <<"Resume Recording:";
        emit started();
    }
}
void Recording::recordingStop()
{
    //Introduction Before Start
    if (recordingTimer) {
        recordingTimer->stop();
        recordingTimer->deleteLater();
        recordingTimer = nullptr;
    }
    // Reset recording start time
   // qDebug() <<"Recording: Information of Logger "
             // <<"on Pressing Stop Button\n"
             // <<"Stop Recording:";
    m_startTime = QDateTime();
    noOfFrame = 0;
    //saveFile();
    //qDebug() << "Recorder cleanup done.";

    recordedStructure.insert("rs_frame",rs_frame);
    recordedStructure.insert("rs_bookmark",rs_bookmark);
    recordedStructure.insert("rs_hierarchy",rs_hierarchy);
    saveFile();
    //    QJsonDocument doc(recordedStructure);
    //    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    //   // qDebug()<<jsonData;

}
void Recording::recordingBookmark(const QString &message, qint64 timestampMs)
{
    if (rs_frame.isEmpty()) {
        qWarning() << "Cannot bookmark in empty file";
        return;
    }
    // If caller didn't compute timestampMs, compute using current state:
    qint64 ts = timestampMs;
    if (ts < 0) {
        // compute total elapsed: accumulated + current segment (or lastElapsedMs if paused)
        if (isPaused) {
            ts = lastElapsedMs;
        } else if (recordingStartTime.isValid()) {
            ts = pausedOffsetMs + recordingStartTime.msecsTo(QDateTime::currentDateTime());
        } else {
            ts = pausedOffsetMs; // fallback
        }
    }
    // Bookmark entry
    rs_bookmarkObj = QJsonObject();
    rs_bookmarkObj["timestamp_ms"] = ts;
    rs_bookmarkObj["current_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rs_bookmarkObj["message"]      = message;

    // Append and write back
    rs_bookmark.append(rs_bookmarkObj);
   // qDebug() << "Bookmark Recorded At:" << ts << "Message:" << message<<rs_bookmark;

}

void Recording::getSimulationUpdate()
{

    //qDebug()<<simUpdate<<" "<<simUpdateType;
}

void Recording::changeInHierarchy()
{
    rs_hierarchyObj   = QJsonObject();
    rs_hierarchyObj.insert("timestamp_ms" , lastElapsedMs );
    rs_hierarchyObj.insert("current_time" , QDateTime::currentDateTime().toString(Qt::ISODate));
    rs_hierarchyObj.insert("hierarchy"    , m_hierarchy->toJson());
    rs_hierarchy.append(rs_hierarchyObj);
    //qDebug()<<"Change In Hierarchy";
}

void Recording::saveFile()
{
    if (recordedStructure.isEmpty()) {
        qWarning() << "Recording: Saving Failed"
                   << "/n/tRecorded File is empty, nothing to save.";
        return;
    }
    // ---- Ask User Where to Save ----
    QString defaultName =
        "Recording_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".json";
    QString filePath = QFileDialog::getSaveFileName(
        nullptr,
        tr("Save Recording File"),
        QDir::homePath() + "/" + defaultName,   // default save location
        tr("JSON Files (*.json);;All Files (*)")
        );
    if (filePath.isEmpty()) {
       // qDebug() << "User cancelled save dialog.";
        return;
    }
    // ---- Convert QJsonArray → JSON ----
    QJsonDocument doc(recordedStructure);
    QByteArray json = doc.toJson(QJsonDocument::Indented);

    // ---- Save the file ----
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << filePath;
        return;
    }
    file.write(json);
    file.close();
   // qDebug() << "Recording saved successfully at:" << filePath;

}

QDateTime Recording::startTime() const
{
    return m_startTime;
}

qint64 Recording::duration() const
{
    return m_duration;
}
/* -------------------------------------------------------
 * Recording Implementation End
 * ------------------------------------------------------*/


/* -------------------------------------------------------
 * Replay Implementation Start
 * ------------------------------------------------------*/

Replay::Replay(
    Hierarchy* hierarchy,
    Simulation* simulation,
    Recorder *parentRecorder,
    QObject *parent) :
    QObject(parent),
    m_hierarchy(hierarchy),
    m_simulation(simulation),
    m_recorder(parentRecorder)
{
    m_recorder = getRecorder();
}
void Replay::update()
{
    //m_recorder->recordingStartTime = NULL;
    m_recorder->duration           = 0;
    m_recorder->loggerStatus       = Recorder::S_REPLAY_MODE;
    m_recorder->simulationStatus   = Recorder::S_SIMULATION_STOP;
    emit m_recorder->recorderInfoSendUsual(
        m_recorder->duration           ,
        m_recorder->loggerStatus       ,
        m_recorder->simulationStatus   );
    m_recorder->loggerInfo();
}

// bool Replay::replayLoaded(const QString &filePath)
// {
//     QFile loadFile(filePath);
//     if (!loadFile.open(QIODevice::ReadOnly)) {
//         qWarning("Couldn't open replay file.");
//         return false;
//     }
//     QByteArray saveData = loadFile.readAll();
//     loadFile.close();
//     QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
//     if (!loadDoc.isObject()) {
//         qWarning("Replay file format error: Expected JSON array.");
//         return false;
//     }

//     recordedStructure  = loadDoc.object();
//     rs_frame = recordedStructure["rs_frame"].toArray();
//     playbackFrames.clear();
//     playbackFrames.reserve(rs_frame.size());

//     // Convert JSON array to QVector
//     for (const QJsonValue &val : rs_frame) {
//         if (val.isObject())
//             playbackFrames.append(val.toObject());
//     }

//     qint64 maxTimestamp = 0;

//     // Compute duration
//     for (const QJsonObject &frm : playbackFrames) {
//         if (frm.contains("timestamp_ms")) {
//             maxTimestamp = qMax(maxTimestamp, frm["timestamp_ms"].toVariant().toLongLong());
//         }
//     }
//     emit setReplayDuration(maxTimestamp);

//     // Step 3 — Load bookmarks ONLY after duration is set
//     if (recordedStructure.contains("rs_bookmark") && recordedStructure["rs_bookmark"].isArray()) {
//         QJsonArray arr = recordedStructure["rs_bookmark"].toArray();

//         for (const QJsonValue &v : arr) {
//             if (!v.isObject()) continue;
//             QJsonObject b = v.toObject();

//             QString message = b["message"].toString();
//             qint64 timestamp = b["timestamp_ms"].toVariant().toLongLong();

//             emit replayBookmark(message, timestamp);   // UI now has duration; markers will show
//         }
//     }
//     // ===== Load first frame (SKIP metadata at index 0) =====
//     if (playbackFrames.size() > 0) {
//         currentReplayIndex = 0;   // Start from snapshot frame
//         rs_hierarchy = recordedStructure["rs_hierarchy"].toArray();
//         const QJsonObject &firstSnapshot = rs_hierarchy.at(0).toObject();
//         if (firstSnapshot.contains("hierarchy")) {
//             QJsonObject snapshotObj = firstSnapshot["hierarchy"].toObject();
//             if (m_hierarchy) {
//                 m_hierarchy->fromJson(snapshotObj);
//             }
//             emit frameLoaded(firstSnapshot);  // Optional UI signal
//         }
//         else {
//             qWarning() << "First snapshot frame missing 'snapshot' property.";
//         }
//     }
//     else {
//         qWarning() << "Replay: Information of Loading/n/tNot enough frames to replay (snapshot required).";
//         return false;
//     }
//     return true;
// }
bool Replay::replayLoaded(const QString &filePath)
{
    // [CRITICAL] Stop any existing replay before touching data
    stop();

    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open replay file.");
        return false;
    }

    QByteArray saveData = loadFile.readAll();
    loadFile.close();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    if (!loadDoc.isObject()) {
        qWarning("Replay file format error: Expected JSON Object.");
        return false;
    }

    recordedStructure  = loadDoc.object();
    rs_frame = recordedStructure["rs_frame"].toArray();

    playbackFrames.clear();
    playbackFrames.reserve(rs_frame.size());

    // Convert JSON array to QVector
    for (const QJsonValue &val : rs_frame) {
        if (val.isObject())
            playbackFrames.append(val.toObject());
    }

    qint64 maxTimestamp = 0;

    // Compute duration
    for (const QJsonObject &frm : playbackFrames) {
        if (frm.contains("timestamp_ms")) {
            maxTimestamp = qMax(maxTimestamp, frm["timestamp_ms"].toVariant().toLongLong());
        }
    }
    emit setReplayDuration(maxTimestamp);

    // Load bookmarks
    if (recordedStructure.contains("rs_bookmark") && recordedStructure["rs_bookmark"].isArray()) {
        QJsonArray arr = recordedStructure["rs_bookmark"].toArray();
        for (const QJsonValue &v : arr) {
            if (!v.isObject()) continue;
            QJsonObject b = v.toObject();
            QString message = b["message"].toString();
            qint64 timestamp = b["timestamp_ms"].toVariant().toLongLong();
            emit replayBookmark(message, timestamp);
        }
    }

    // Load first frame (SKIP metadata at index 0)
    if (playbackFrames.size() > 0) {
        currentReplayIndex = 0;

        // Try to load initial hierarchy if available
        if(recordedStructure.contains("rs_hierarchy")) {
            rs_hierarchy = recordedStructure["rs_hierarchy"].toArray();
            if(!rs_hierarchy.isEmpty()) {
                const QJsonObject &firstSnapshot = rs_hierarchy.at(0).toObject();
                if (firstSnapshot.contains("hierarchy") && m_hierarchy) {
                    m_hierarchy->fromJson(firstSnapshot["hierarchy"].toObject());
                }
            }
        }
    }
    else {
        qWarning() << "Replay: Not enough frames to replay.";
        return false;
    }

    // [CRITICAL] Reset index to 1 (skipping metadata) so it is ready to play
    currentReplayIndex = 1;

    // Notify UI that file is loaded
    fileLoaded();

    return true;
}
void Replay::createTimer()
{
    // If timer exists, do nothing (or recreate if you prefer safety)
    if (replayTimer) return;

    replayTimer = new QTimer(this);

    //
    connect(replayTimer, &QTimer::timeout, this, [this]() {
        if (currentReplayIndex < playbackFrames.size()) {
            const QJsonValue &frame = playbackFrames.at(currentReplayIndex);

            // 1. Update UI Timeline (Blue Line)
            QJsonObject frameObj = frame.toObject();
            if(frameObj.contains("timestamp_ms")){
                qint64 ts = frameObj["timestamp_ms"].toVariant().toLongLong();
                emit replayFrameLoaded(ts);
            }

            // 2. Update 3D Scene (Movement)
            loadFrameEntitiesData(frame);
            emit updateScene(0.1f);

            currentReplayIndex++;
        } else {
            // End of playback
            stop();
        }
    });
}

void Replay::replayStart()
{

}

// void Replay::start()
// {

//     m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
//              <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
//     //New
//     if (playbackFrames.isEmpty()) {
//         qWarning() << "Replay: No frames to replay";
//         return;
//     }

//     // Reset if fresh start (not resume)
//     if (!isPaused)
//         currentReplayIndex = 1;

//     if (!replayTimer) {
//         replayTimer = new QTimer(this);

//         connect(replayTimer, &QTimer::timeout, this, [this]() {
//             if (currentReplayIndex < playbackFrames.size()) {
//                 const QJsonValue  &frame = playbackFrames.at(currentReplayIndex);
//                 loadFrameEntitiesData(frame);
//                 emit updateScene(0.1f);
//                 currentReplayIndex++;
//             } else {
//                 stop();
//             }
//         });
//     }

//     replayTimer->start(100);
//     isPaused = false;

//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
//     m_recorder->loggerInfo();

//    // qDebug() << "Replay STARTED";
// }
// void Replay::start()
// {
//     m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);

//     // Log the status
//     m_recorder->loggerInfo();

//     if (playbackFrames.isEmpty()) {
//         qWarning() << "Replay: No frames to replay";
//         return;
//     }

//     // If we are not resuming from a pause, start from the beginning (index 1 skips metadata)
//     if (!isPaused) {
//         currentReplayIndex = 1;
//     }

//     // --- LOGIC FIX: Create the timer if it doesn't exist (e.g., after stop() was called) ---
//     if (!replayTimer) {
//         replayTimer = new QTimer(this);

//         connect(replayTimer, &QTimer::timeout, this, [this]() {
//             if (currentReplayIndex < playbackFrames.size()) {
//                 const QJsonValue &frame = playbackFrames.at(currentReplayIndex);
//                 // 3. Update 3D Scene
//                 loadFrameEntitiesData(frame);
//                 emit updateScene(0.1f);

//                 currentReplayIndex++;
//             } else {
//                 // End of playback
//                 stop();
//             }
//         });
//     }

//     // Start the timer loop (100ms interval)
//     if (!replayTimer->isActive()) {
//         replayTimer->start(100);
//     }

//     isPaused = false;
//    // qDebug() << "Replay STARTED";
// }

// void Replay::start()
// {
//     m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
//     m_recorder->loggerInfo();

//     if (playbackFrames.isEmpty()) {
//         qWarning() << "Replay: No frames to replay";
//         return;
//     }

//     // Ensure timer is created with the CORRECT logic
//     createTimer();

//     // Start the timer if not running
//     if (!replayTimer->isActive()) {
//         replayTimer->start(100);
//     }

//     isPaused = false;
//    // qDebug() << "Replay STARTED";
// }
void Replay::start()
{
    m_recorder = getRecorder();
    m_recorder->loggerStatus = Recorder::S_REPLAYING;
    m_recorder->update(m_recorder->loggerStatus);
    m_recorder->loggerInfo();

    if (playbackFrames.isEmpty()) {
        qWarning() << "Replay: No frames to replay";
        return;
    }

    // --- LOGIC FIX: Don't reset if we are in the middle ---
    // Only reset to 1 if we are starting fresh or reached the end previously.
    // If you scrubbed back (goToPreviousFrame), currentReplayIndex will be < size,
    // so this block is skipped and it plays from where you are.
    if (!isPaused) {
        if (currentReplayIndex >= playbackFrames.size() - 1 || currentReplayIndex <= 0) {
            currentReplayIndex = 1;
        }
    }

    // Ensure timer is created (using the createTimer helper from previous fixes)
    // If you don't have createTimer, paste the timer creation code here.
    createTimer();

    if (!replayTimer->isActive()) {
        replayTimer->start(100);
    }

    isPaused = false;
   // qDebug() << "Replay STARTED from index" << currentReplayIndex;
}
void Replay::loadFrameEntitiesData(const QJsonValue frame)
{
    //Get the Hierarchy
    std::unordered_map<std::string, Platform*> *m_Platforms = m_hierarchy->Platforms;

    rs_frameObj = QJsonObject();
    rs_frameObj = frame.toObject();
    if(rs_frameObj.contains("timestamp_ms")){
        qint64 ts = rs_frameObj["timestamp_ms"].toVariant().toLongLong();
        emit replayFrameLoaded(ts);
    }
    if(rs_frameObj.contains("frameEntities")){
        frameEntities = QJsonArray();
        frameEntities = rs_frameObj["frameEntities"].toArray();
        for(const QJsonValue &rs_entity : frameEntities){
            entityObj = QJsonObject();
            entityObj = rs_entity.toObject();
            std::string id = entityObj["id"].toString().toStdString();


            if(m_Platforms->find(id) != m_Platforms->end()){
                Platform* platform = (* m_hierarchy->Platforms)[id];

                float x = static_cast<float>(entityObj["cord_x"].toDouble());
                float y = static_cast<float>(entityObj["cord_y"].toDouble());
                float z = static_cast<float>(entityObj["cord_z"].toDouble());
                platform->transform->matrix->setTranslation(QVector3D(x,y,z));

                float axis_x = static_cast<float>(entityObj["axis_x"].toDouble());
                float axis_y = static_cast<float>(entityObj["axis_y"].toDouble());
                float axis_z = static_cast<float>(entityObj["axis_z"].toDouble());
                platform->transform->setFromEulerAngles(QVector3D(axis_x,axis_y,axis_z));

                float turn_radius = static_cast<float>(entityObj["turn_radius"].toDouble());
                float max_speed   = static_cast<float>(entityObj["max_speed"].  toDouble());
                float curr_speed  = static_cast<float>(entityObj["curr_speed"]. toDouble());
                platform->dynamicModel->turnRadius   = turn_radius ;
                platform->dynamicModel->moveSpeed    = max_speed   ;
                platform->dynamicModel->currentSpeed = curr_speed  ;
                //qDebug()<<"********It Exist**********";
            }
        }
    }
}

void Replay::toggle()
{
    // Case 1: Currently Playing -> PAUSE
    if (replayTimer && replayTimer->isActive()) {
        pause();
    }
    // Case 2: Currently Paused -> RESUME
    else if (isPaused) {
        resume();
    }
    // Case 3: Stopped (Timer is null) -> START
    // This handles the case where it finished, you moved back, and clicked Toggle.
    else {
        start();
    }
}

void Replay::pause()
{
    m_recorder = getRecorder();
    m_recorder->loggerStatus = Recorder::S_REPLAY_PAUSED;
    m_recorder->update(m_recorder->loggerStatus);
   // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
             // <<"\n\t Status       : "<<m_recorder->loggerStatus;
    m_recorder->loggerInfo();
    //New
    if (replayTimer && replayTimer->isActive()) {
        replayTimer->stop();
        isPaused = true;
        m_recorder->loggerStatus = Recorder::S_REPLAY_PAUSED;
        m_recorder->update(m_recorder->loggerStatus);
        m_recorder->loggerInfo();
       // qDebug() << "Replay PAUSED";
    }
}

// void Replay::resume()
// {
//     m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
//              <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
//     //New
//     if (!isPaused) {
//         qWarning() << "Replay: Resume called but not paused";
//         return;
//     }

//     isPaused = false;
//     replayTimer->start(100);

//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
//     m_recorder->loggerInfo();

//    // qDebug() << "Replay RESUMED";
// }
void Replay::resume()
{
    m_recorder = getRecorder();

    // 1. Safety Check: If we paused, but the timer was deleted (e.g. after Stop), recreate it.
    if (!replayTimer) {
        createTimer();
    }

    // 2. Standard Resume Logic
    if (!isPaused) {
        qWarning() << "Replay: Resume called but not paused";
        return;
    }

    isPaused = false;

    // Check active before starting to avoid warnings
    if (!replayTimer->isActive()) {
        replayTimer->start(100);
    }

    m_recorder->loggerStatus = Recorder::S_REPLAYING;
    m_recorder->update(m_recorder->loggerStatus);
    m_recorder->loggerInfo();

   // qDebug() << "Replay RESUMED";
}

// void Replay::stop()
// {
//     m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAY_STOPPED;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
//              <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
//     //new
//     if (replayTimer) {
//         replayTimer->stop();
//         replayTimer->deleteLater();
//         replayTimer = nullptr;
//     }

//     currentReplayIndex = 0;

//     isPaused = false;

//     m_recorder->loggerStatus = Recorder::S_REPLAY_STOPPED;
//     m_recorder->update(m_recorder->loggerStatus);
//     m_recorder->loggerInfo();

//    // qDebug() << "Replay STOPPED";
// }

// void Replay::stop()
// {
//     //m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAY_STOPPED;
//     m_recorder->update(m_recorder->loggerStatus);
//     m_recorder->loggerInfo();

//     if (replayTimer) {
//         replayTimer->stop();
//         replayTimer->deleteLater(); // Deletes the timer
//         replayTimer = nullptr;      // Sets pointer to null
//     }

//     currentReplayIndex = 1; // Reset to beginning
//     isPaused = false;

//    // qDebug() << "Replay STOPPED";
// }
void Replay::stop()
{
    m_recorder = getRecorder(); // Ensure m_recorder is valid
    m_recorder->loggerStatus = Recorder::S_REPLAY_STOPPED;
    m_recorder->update(m_recorder->loggerStatus);
    m_recorder->loggerInfo();

    if (replayTimer) {
        replayTimer->stop();
        replayTimer->deleteLater();
        replayTimer = nullptr; // Important: set to null so createTimer() knows to recreate it
    }

    // Reset to beginning so if user clicks Play, it starts over (optional, or keep current pos)
    // currentReplayIndex = 1;
    isPaused = false;

   // qDebug() << "Replay STOPPED";
}

// void Replay::restart()
// {
//     m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
//              <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
// }

// void Replay::restart()
// {
//    // qDebug() << "Replay: Restart Requested";

//     // 1. Force a full stop first.
//     // This ensures timers are deleted and isPaused is set to false.
//     stop();

//     // 2. Explicitly reset index to 1 (skipping metadata at 0)
//     currentReplayIndex = 1;

//     // 3. Call start().
//     // Since we just called stop(), start() will see !isPaused and create a new timer.
//     start();
// }
void Replay::restart()
{
   // qDebug() << "Replay: Restart Requested";
    stop(); // Clears timer
    currentReplayIndex = 1; // Resets to start
    start(); // Creates new timer and runs
}
void Replay::playAgain()
{
    restart(); // Just reuse restart logic
}

void Replay::fileLoaded()
{
    //m_recorder = getRecorder();
    m_recorder->loggerStatus = Recorder::S_REPLAY_LOADED;
    m_recorder->update(m_recorder->loggerStatus);
   // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
             // <<"\n\t Status       : "<<m_recorder->loggerStatus;
    m_recorder->loggerInfo();
}

void Replay::fileUnloaded()
{
    //m_recorder = getRecorder();
    m_recorder->loggerStatus = Recorder::S_REPLAY_UNLOADED;
    m_recorder->update(m_recorder->loggerStatus);
   // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
             // <<"\n\t Status       : "<<m_recorder->loggerStatus;
    m_recorder->loggerInfo();
}

void Replay::goToNextFrame()
{
    if (playbackFrames.isEmpty()) return;

    if (currentReplayIndex + 50 < playbackFrames.size()) {
        currentReplayIndex += 50;

        const QJsonObject &frame = playbackFrames.at(currentReplayIndex);
        if (frame.contains("hierarchy")) {
            m_hierarchy->fromJson(frame["hierarchy"].toObject());
        }

        emit replayFrameLoaded(frame["timestamp_ms"].toVariant().toLongLong());
    }
}


// void Replay::goToPreviousFrame()
// {
//     if (playbackFrames.isEmpty()) return;
//     if(!replayTimer->isActive() && 0 < (playbackFrames.size() - 50)){
//         currentReplayIndex = playbackFrames.size() - 50;
//         createTimer();
//     }
//     if (currentReplayIndex >= 50) {
//         currentReplayIndex -= 50;

//         const QJsonObject &frame = playbackFrames.at(currentReplayIndex);
//         if (frame.contains("hierarchy")) {
//             m_hierarchy->fromJson(frame["hierarchy"].toObject());
//         }

//         emit replayFrameLoaded(frame["timestamp_ms"].toVariant().toLongLong());
//     }
// }
// void Replay::goToPreviousFrame()
// {
//     if (playbackFrames.isEmpty()) return;

//     // 1. CRASH FIX: Check if timer exists before checking if it's active
//     bool isTimerRunning = (replayTimer && replayTimer->isActive());

//     // 2. Handle "End of Replay" scenario
//     // If stopped or finished, snap to the end so we can step back from there
//     if (!isTimerRunning && currentReplayIndex == 0) {
//         currentReplayIndex = playbackFrames.size();
//     }
//     // Safety clamp if index is somehow beyond bounds
//     if (currentReplayIndex > playbackFrames.size()) {
//         currentReplayIndex = playbackFrames.size();
//     }

//     // 3. Perform the Step Back
//     if (currentReplayIndex >= 50) {
//         currentReplayIndex -= 50;
//     } else {
//         currentReplayIndex = 0; // Clamp to start
//     }

//     // 4. Update Data and Visuals
//     // Note: playbackFrames stores QJsonValue or QJsonObject.
//     // using .toObject() ensures conversion if it's a Value.
//     QJsonObject frame = playbackFrames.at(currentReplayIndex);

//     // Update Hierarchy Data
//     if (frame.contains("hierarchy")) {
//         m_hierarchy->fromJson(frame["hierarchy"].toObject());
//     }

//     // Update 3D Scene (CRITICAL for visuals)
//     loadFrameEntitiesData(frame);
//     emit updateScene(0.1f);

//     // Update Timeline
//     if (frame.contains("timestamp_ms")) {
//         emit replayFrameLoaded(frame["timestamp_ms"].toVariant().toLongLong());
//     }
// }

void Replay::goToPreviousFrame()
{
    if (playbackFrames.isEmpty()) return;

    // 1. REMOVE STOP LOGIC
    // We want the replay to keep running (or start running), so do NOT stop the timer here.

    // 2. Handle "End of Replay" scenario
    // If we were stopped/finished, reset index from the end to allow stepping back.
    if (currentReplayIndex >= playbackFrames.size()) {
        currentReplayIndex = playbackFrames.size();
    }

    // 3. Perform the Step Back
    if (currentReplayIndex >= 50) {
        currentReplayIndex -= 50;
    } else {
        currentReplayIndex = 0; // Clamp to start
    }

    // 4. Update Data and Visuals (Immediate Feedback)
    if (currentReplayIndex < playbackFrames.size()) {
        QJsonObject frame = playbackFrames.at(currentReplayIndex);

        // Update Hierarchy
        if (frame.contains("hierarchy")) {
            m_hierarchy->fromJson(frame["hierarchy"].toObject());
        }

        // Update 3D Scene
        loadFrameEntitiesData(frame);
        emit updateScene(0.1f);

        // Update Timeline
        if (frame.contains("timestamp_ms")) {
            emit replayFrameLoaded(frame["timestamp_ms"].toVariant().toLongLong());
        }
    }

    // =========================================================
    // CRITICAL FIX: AUTO-RESUME / FORCE PLAY
    // =========================================================
    // The user wants replay to START (or continue) immediately.

    // A. Ensure Timer Exists
    if (!replayTimer) {
        replayTimer = new QTimer(this);
        // Re-connect the timer loop logic if you created a new one
        connect(replayTimer, &QTimer::timeout, this, [this]() {
            if (currentReplayIndex < playbackFrames.size()) {
                const QJsonValue &frame = playbackFrames.at(currentReplayIndex);
                loadFrameEntitiesData(frame);
                emit updateScene(0.1f);

                // Update Timeline
                if(frame.toObject().contains("timestamp_ms")){
                    emit replayFrameLoaded(frame.toObject()["timestamp_ms"].toVariant().toLongLong());
                }

                currentReplayIndex++;
            } else {
                stop();
            }
        });
    }

    // B. Set Flag to Playing
    isPaused = false;

    // C. Start Timer if not running (e.g., if we were finished/stopped)
    if (!replayTimer->isActive()) {
        replayTimer->start(100);
    }

    // D. Update UI Status to REPLAYING (So "Pause" button appears)
    if (!m_recorder) m_recorder = getRecorder();
    if (m_recorder) {
        m_recorder->loggerStatus = Recorder::S_REPLAYING;
        m_recorder->update(m_recorder->loggerStatus);
        // m_recorder->loggerInfo();
    }
}

// void Replay::goToPreviousFrame()
// {
//     if (playbackFrames.isEmpty()) return;

//     // 1. Check if timer is currently running
//     bool isTimerRunning = (replayTimer && replayTimer->isActive());

//     // 2. If running, stop it (Stepping manually implies pausing)
//     if (isTimerRunning) {
//         replayTimer->stop();
//         // We do not delete the timer here, just stop it
//     }

//     // 3. Handle "End of Replay" scenario
//     // If we were stopped/finished, we might be at the very end
//     if (currentReplayIndex >= playbackFrames.size()) {
//         currentReplayIndex = playbackFrames.size();
//     }

//     // 4. Perform the Step Back (Move index back by 50 or clamp to 0)
//     if (currentReplayIndex >= 50) {
//         currentReplayIndex -= 50;
//     } else {
//         currentReplayIndex = 0;
//     }

//     // 5. Update Data and Visuals
//     if (currentReplayIndex < playbackFrames.size()) {
//         QJsonObject frame = playbackFrames.at(currentReplayIndex);

//         // Update Hierarchy
//         if (frame.contains("hierarchy")) {
//             m_hierarchy->fromJson(frame["hierarchy"].toObject());
//         }

//         // Update 3D Scene
//         loadFrameEntitiesData(frame);
//         emit updateScene(0.1f);

//         // Update Timeline
//         if (frame.contains("timestamp_ms")) {
//             emit replayFrameLoaded(frame["timestamp_ms"].toVariant().toLongLong());
//         }
//     }

//     // =========================================================
//     // CRITICAL FIX: Update State to PAUSED
//     // =========================================================
//     // We are now at a valid frame, but not playing. This is the definition of PAUSED.
//     isPaused = true;

//     // Ensure m_recorder is valid
//     if (!m_recorder) m_recorder = getRecorder();

//     // Notify the UI that we are now PAUSED (so Start button behaves like Resume)
//     if (m_recorder) {
//         m_recorder->loggerStatus = Recorder::S_REPLAY_PAUSED;
//         m_recorder->update(m_recorder->loggerStatus);

//         // Optional: Force logger info update if your UI relies on it
//         // m_recorder->loggerInfo();
//     }
// }

// void Replay::playAgain()
// {
//     if (playbackFrames.isEmpty()) {
//         qWarning() << "Replay: No frames loaded.";
//         return;
//     }

//     currentReplayIndex = 1;
//     isPaused = false;

//     if (!replayTimer) {
//         replayTimer = new QTimer(this);
//         connectReplayTimer();
//     }

//     replayTimer->start(100);

//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
// }


// void Replay::startReplayFromTimestamp(qint64 timestampMs)
// {
//     if (playbackFrames.isEmpty())
//         return;

//     // Find frame with >= requested timestamp
//     for (int i = 0; i < playbackFrames.size(); i++) {
//         qint64 frameTime = playbackFrames[i]["timestamp_ms"].toVariant().toLongLong();
//         if (frameTime >= timestampMs) {
//             currentReplayIndex = i;
//             break;
//         }
//     }

//     if (!replayTimer) {
//         replayTimer = new QTimer(this);
//         connectReplayTimer();
//     }

//     replayTimer->start(100);
//     isPaused = false;

//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
// }

void Replay::startReplayFromTimestamp(qint64 timestampMs)
{
    if (playbackFrames.isEmpty()) return;

    // Stop current playback
    if (replayTimer && replayTimer->isActive()) {
        replayTimer->stop();
    }
    // Note: We don't call full stop() here because we don't want to reset currentReplayIndex to 1

    // Find correct index
    for (int i = 0; i < playbackFrames.size(); i++) {
        qint64 frameTime = playbackFrames[i]["timestamp_ms"].toVariant().toLongLong();
        if (frameTime >= timestampMs) {
            currentReplayIndex = i;
            break;
        }
    }

    // Resume/Start from this new index
    start();
}

void Replay::bookmarkReplay(const QString &note, qint64 timestampMs)
{
    // Stop if replay already running
    if (replayTimer && replayTimer->isActive()) {
        replayTimer->stop();
    }

    // Reset replay state
    isPaused = false;

    // Start from timestamp
    startReplayFromTimestamp(timestampMs);

   // qDebug() << "Replay jumped to bookmark: " << note << " @ " << timestampMs << " ms";
}



void Replay::connectReplayTimer()
{
    connect(replayTimer, &QTimer::timeout, this, [this]() {
        if (currentReplayIndex < playbackFrames.size()) {

            const QJsonObject &frame = playbackFrames.at(currentReplayIndex);

            if (frame.contains("hierarchy")) {
                m_hierarchy->fromJson(frame["hierarchy"].toObject());
                emit replayFrameLoaded(frame["timestamp_ms"].toVariant().toLongLong());
                emit frameLoaded(frame);
            }

            currentReplayIndex++;

        } else {
            stop();
        }
    });
}







/* -------------------------------------------------------
 * Replay Implementation End
 * ------------------------------------------------------*/



void Recorder::recorderStatus()
{

}


QJsonObject Recorder::getAllRecordings() const
{
    QJsonObject obj;
    obj["recordings"] = m_recordings; // m_recordings is QJsonArray
    return obj;
}

void Recorder::startRecording()
{
   // qDebug() << "Recording started.";
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

void Recorder::togglePause()
{
    if (!recordingTimer) return;

    if (!isRecordingPaused)
    {
        // Enter pause
        isRecordingPaused = true;
        pauseStartTime = QDateTime::currentDateTime();

        //qDebug() << "Recording Paused.";
        emit recordingPaused();
    }
    else
    {
        // Resume
        qint64 pausedDuration = pauseStartTime.msecsTo(QDateTime::currentDateTime());
        pausedTimeOffset += pausedDuration;

        isRecordingPaused = false;

        //qDebug() << "Recording Resumed.";
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

//        // qDebug() << "Recording Paused.";

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

//        // qDebug() << "Recording Resumed. Offset added:" << pausedDuration << "ms";

//         emit recordingResumed();  // Notify UI (optional)
//     }
// }







void Recorder::stopRecording()
{
    //qDebug() << "Recording stopped.";
    // Stop and delete the interval timer if it exists
    if (recordingTimer) {
        recordingTimer->stop();
        recordingTimer->deleteLater();
        recordingTimer = nullptr;
    }
    // Reset recording start time
    recordingStartTime = QDateTime();
    //qDebug() << "Recorder cleanup done.";
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

    //qDebug().noquote() << "Bookmark updated at" << timestampMs << "ms — message:" << message;
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

//    // qDebug().noquote() << "Bookmark saved at" << timestampMs << "ms — message:" << message;
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

    //qDebug() << "Recording saved to:" << filePath;

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

    //qDebug() << "Replay loaded with" << playbackFrames.size() << "frames.";
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
                //qDebug() << "Loaded first snapshot frame at index 1.";
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

                    //// qDebug() << "Frame" << currentReplayIndex
                    //          << "Timestamp:" << ts << "ms";

                    // New line — tells UI where we are in timeline
                    emit replayFrameLoaded(ts);
                }

                currentReplayIndex++;
            } else {
                //qDebug() << "Replay complete";
                recordingTimer->stop();
            }
        });
    }

    recordingTimer->start(100);
    //qDebug() << "Replay started.";
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
        //qDebug() << "Replay Paused.";
    }
    // If paused → resume
    else if (isReplayPaused) {
        qint64 pausedDuration = replayPauseStartTime.msecsTo(QDateTime::currentDateTime());
        replayPausedTimeOffset += pausedDuration;

        recordingTimer->start(100); // match replay interval
        isReplayPaused = false;

        //qDebug() << "Replay Resumed. Offset added:" << pausedDuration << "ms";
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

        //qDebug() << "Moved to next frame:" << currentReplayIndex;
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

        //qDebug() << "Moved to previous frame:" << currentReplayIndex;
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

                //// qDebug() << "Frame" << currentReplayIndex
                //          << "Timestamp:" << ts << "ms";

                // New line — tells UI where we are in timeline
                emit replayFrameLoaded(ts);
                emit frameLoaded(frame);  // optional signal for UI update

            }

            ++currentReplayIndex;
        } else {
            //qDebug() << "Replay finished.";
            recordingTimer->stop();
        }
    });

    // Start replay
    recordingTimer->start(100); // interval = 100 ms
    //qDebug() << "Replay restarted from frame 0.";
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

                //qDebug() << "Frame" << currentReplayIndex
                //<< "Timestamp:" << ts << "ms";
                emit replayFrameLoaded(ts);
                emit frameLoaded(frame);  // optional signal for UI update
            }

            ++currentReplayIndex;
        } else {
            //qDebug() << "Playback completed.";
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
    //qDebug() << "Accessed element at index" << index << ":";

    if (element.isObject()) {
        //qDebug().noquote() << QJsonDocument(element.toObject()).toJson(QJsonDocument::Indented);
    } else if (element.isArray()) {
        //qDebug().noquote() << QJsonDocument(element.toArray()).toJson(QJsonDocument::Indented);
    } else {
        //qDebug() << "Value:" << element;
    }

    return element;
}




// Stop the replay process
void Recorder::stopReplay()
{
    replayTimer->stop();
    currentFrame = 0;
    //qDebug() << "Replay stopped.";
}

// Called by timer to emit the next frame in the replay
void Recorder::playNextFrame()
{
    if (currentFrame >= trajectoryArray.size()) {
        replayTimer->stop();
        //qDebug() << "Replay finished.";
        return;
    }

    QJsonObject frame = trajectoryArray[currentFrame].toObject();
    //emit replayFrame(frame);   // Emit frame to connected slot
    currentFrame++;
}

// void Recorder::showBookmarkLog(const QString &note, qint64 timestampMs){
//    // qDebug() << "By Recorder : Bookmark clicked : Note =" << note << ", Timestamp =" << timestampMs << "ms";
// }


void Recorder::bookmarkReplay(const QString &note, qint64 timestampMs){
    //qDebug() << "By Recorder : Bookmark clicked : Note =" << note << ", Timestamp =" << timestampMs << "ms";
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
    //qDebug() << "Resetting replay data...";

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

    //qDebug() << "Replay data cleared.";
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
