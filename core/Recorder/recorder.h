
#ifndef RECORDER_H
#define RECORDER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <QTimer>
#include <QJsonValue>
#include <QString>
#include <core/Simulation/simulation_state.h>
#include <core/Hierarchy/entity.h>
#include <core/Hierarchy/EntityProfiles/platform.h>

// Forward declarations to avoid circular includes
class Hierarchy;
class Simulation;
class Recording;
class Replay;

class Recorder : public QObject
{
    Q_OBJECT
public:
    explicit Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent = nullptr);


public:
    // Constructor:Accepts hierarchy and simulation to pull state and recording speed
    //explicit Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent = nullptr);
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
    void recordingTimeUpdated(qint64 ms);
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
    Simulation* m_simulation = nullptr;  // Used for getting simulation speed

    QJsonObject recordedData;  // Main data JSON object
    QJsonArray trajectoryArray;  // Stores all frames for replay
    int sampleRate = 1;  // Sample rate for recording
    int currentFrame = 0;  // Tracks current frame during replay
    QTimer *replayTimer = nullptr;
    QVector<QJsonObject> recordedFrames;
    //By Hima
    QJsonValue getArrayElement(const QJsonArray &array, int index);
    //QDateTime recordingStartTime;  // Start time of the current recording
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
    qint64 lastElapsedMs = 0;

    /* -------------------------------------------------------
 * Shared Object Start
 * ------------------------------------------------------*/

public:
    //For Seleting Modes
    enum loggerModes{
        RECORDING,
        REPLAY
    };
    Q_ENUM(loggerModes)

public:
    //String for Debug
    Recording *m_recording = nullptr;
    Replay    *m_replay    = nullptr;
    QString debugString;
    loggerModes modeOfLogger;

public:
    Recording* getRecording() const { return m_recording; }
    Replay* getReplay()    const { return m_replay;    }

public slots:
    void loggerModeCheck(loggerModes mode);
    void recorderStatus();
    //Recorder Information Start
    //Enum for Logger Status of:
    //S Stands for Status:
public:
    enum LoggerStatusModes{
        //For Recording
        S_RECORDING_MODE,   //0
        S_RECORDING,        //1
        S_RECORDING_PAUSED, //2
        S_RECORDING_STOPPED,//3
        //For REPLAY
        S_REPLAY_MODE,      //4
        S_REPLAY_LOADED,    //5
        S_REPLAY_UNLOADED,  //6
        S_REPLAYING,        //7
        S_REPLAY_PAUSED,    //8
        S_REPLAY_STOPPED,   //9
    };
    Q_ENUM(LoggerStatusModes)

    //Enum for Simulation Status:
    //S Stands for Status:
    enum SimulationStatusModes{
        S_SIMULATION_START, //0
        S_SIMULATION_PAUSED,//1
        S_SIMULATION_STOP,  //2
        S_SIMULATION_NA,    //3
    };
    Q_ENUM(SimulationStatusModes)
    QDateTime             recordingStartTime;
    qint64                 duration;
    Recorder::LoggerStatusModes     loggerStatus    ;
    Recorder::SimulationStatusModes simulationStatus;
    // Recorder* recorder;
    // Recorder* getRecorder() const { return recorder; }
public:
    //To Display input data
    void loggerInfo();
    void update(Recorder::LoggerStatusModes     loggerStatus);
private:

    //Use to Update Recorder Information Logger Status Modes

    //void update();
    // void update(
    //     QDateTime       s_recordingStartTime,
    //     qint64          s_duration,
    //     Recorder::LoggerStatusModes     loggerStatus,
    //     Recorder::SimulationStatusModes simulationStatus);
    //Use to Update Recorder Information only first time
    void recorderInfoUpdateOnce();
    //Use to Update Recorder Information after first
    void recorderInfoUpdateUsual();



signals:
    //Use to Send Recorder Information only first time

    // void update(QDateTime       s_recordingStartTime);
    // void update(qint64          s_duration);

    //For Testing
    void recorderInfoSend(
        Recorder::LoggerStatusModes     loggerStatus);

    //void update(Recorder::SimulationStatusModes simulationStatus);
    void recorderInfoSendOnce(
        QDateTime       s_recordingStartTime,
        qint64          s_duration,
        Recorder::LoggerStatusModes     loggerStatus,
        Recorder::SimulationStatusModes simulationStatus);

    //Use to Send Recorder Information after first
    void recorderInfoSendUsual(
        qint64           s_duration,
        Recorder::LoggerStatusModes     loggerStatus,
        Recorder::SimulationStatusModes simulationStatus);

    //Use to Send Recorder Information only duration
    void recorderInfoSendDuration(qint64 s_duration);

    //Recorder Information End

private:

};

/* -------------------------------------------------------
 * Shared Object End
 * ------------------------------------------------------*/



/* -------------------------------------------------------
 * Recording QObject Start
 * ------------------------------------------------------*/

class Recording : public QObject
{
    Q_OBJECT

public:
    explicit Recording(Hierarchy* hierarchy, Simulation* simulation,Recorder *parentRecorder, QObject *parent = nullptr);
public:
    //Recorder *m_recorder;
    // Recorder* getRecorder() const { return m_recorder; }
    Recording *m_recording;
    //For Recording
    enum recordingModes{
        START,
        PAUSE,
        RESUME,
        STOP
    };

    Q_ENUM(recordingModes)

    Recorder* getRecorder() const { return m_recorder; }

    template<typename T>
    T getValue(T* ptr) const
    {
        return *ptr;
    }

    template<typename T>
    void setValue(T* ptr, const T &value)
    {
        *ptr = value;
    }
    void update();
    QDateTime startTime() const;
    qint64     duration()  const;

public slots:
    void start();
    void resume();
    void pause();
    void stop();
    void addBookmark();
private:
    Recorder*   m_recorder   = nullptr;
    Hierarchy*  m_hierarchy  = nullptr;  // Used for extracting structure snapshot
    Simulation* m_simulation = nullptr;  // Used for getting simulation speed
    QDateTime   m_startTime;
    qint64      m_duration;
    bool        m_active { false };

    //Recording Start Recording Start
public:
    void recordingBookmark(const QString &message, qint64 timestampMs);

    /*------------ New Type of recording Start ------------*/

    // public:
    //     //Definig essential enums
    //     enum typeOfUpdate{
    //         dynamicDynamic,
    //         dynamicStatic,
    //         trajectory,
    //         sensor,
    //         entity,
    //     };

    //     enum updateTypes{
    //         CREATE,
    //         UPDATE,
    //         DELETE
    //     };

private:
    SimUpdateTypes::UpdateTypes     simUpdate     = SimUpdateTypes::UPDATE;
    SimTypeOfUpdates::TypeOfUpdate  simUpdateType = SimTypeOfUpdates::dynamicDynamic;
    QJsonObject recordedStructure;
    /*├──►*/QJsonArray   rs_hierarchy;
    /*│---*/QJsonObject    rs_hierarchyObj;
    /*├──►*/QJsonArray   rs_bookmark ;
    /*│---*/QJsonObject    rs_bookmarkObj;
    /*└─┬►*/QJsonArray   rs_frame    ;
    /*  │---*/QJsonObject  rs_frameObj;
    /*  └─┬►*/QJsonArray     frameEntities;
    /*    ├───►*/ QJsonObject  entityObj;
    /*    ├───►*/ //QJsonObject CurrentSpeed;
    /*    └───►*/
    QJsonArray    getFrameEntitiesData();

public:
    void getSimulationUpdate();
    void changeInHierarchy();
    bool changeInHierarchyInPause = false;
    qint64 lastElapsedMs = 0;

public slots:
    //void getSimulationUpdate();
    /*------------ New Type of recording End   ------------*/

private:
    void recordingStart();
    void recordingStop();
    void recordingPauseResume();
    void saveFile();


    //Recording Main Container
    QJsonArray recordedFile;
    //Meta Data essential for recording
    //   Contain static components of recording
    //QJsonObject metaData; //Add later
    QDateTime   currentDateTime;
    QJsonArray  bookmarks;
    //  Contain snap in data entry
    //QJsonObject timeEntry;
    QJsonObject recordedData;

    //Recording Container Insert Method;
    void insertRecord(const QJsonObject &data);


    //Recording Constants
    //RecordinDynamic components of recording
public:
    recordingModes mode = STOP;
    //for logger
    //QTimer *recordingTimer = nullptr;
    //qint64 lastElapsedMs;
    //qint64 pausedOffsetMs = 0;
    //Remove
    qint64 recordingPeriod;
private:
    QTimer *recordingTimer = nullptr;
    QDateTime recordingStartTime;
    qint64 pausedOffsetMs = 0;   // public earlier, that's ok

    bool isPaused = false;
    int noOfFrame = 1;
    //     //QTimer *recordingTimer = nullptr; //error1
    //     QDateTime recordingStartTime;
    //     QDateTime recordingPauseTime;
    //     //qint64 pausedOffsetMs = 0;   // time before pause
    //     bool isPaused = false;
    // //Remove
    //     //Recordin Period is in of Off-Set of 1s = 1000ms
    //     qint64 recordingPeriod;
    //     int    noOfFrame  = 1;
    //     qint64 leftTimer  = 0;
    //     qint64 rightTimer = 0;
    //     //qint64 elapsedMs;                //error1


    QDateTime pauseStartTime;
    //Recording Start Recording Start
signals:
    void started();
    void paused();
    void stopped(qint64);

};
/* -------------------------------------------------------
 * Recording QObject End
 * ------------------------------------------------------*/



/* -------------------------------------------------------
 * Replay QObject Start
 * ------------------------------------------------------*/
class Replay : public QObject
{
    Q_OBJECT

public:
    explicit Replay(Hierarchy* hierarchy, Simulation* simulation,Recorder *parentRecorder, QObject *parent = nullptr);

    //Recorder* recorder;
    //Recording *m_recording = nullptr;
    // Recorder* getRecorder() const { return m_recorder; }
    Recorder* getRecorder() const { return m_recorder; }
    Replay *m_replay = nullptr;
    QTimer *replayTimer = nullptr;


    qint64 pausedTimestamp = 0;

    //For Replay
    enum replayModes{
        Start,
        PAUSE,
        RESUME,
        STOP
    };
    Q_ENUM(replayModes)
    void update() ;
    bool replayLoaded(const QString &filePath);
    void replayStart();
    void connectReplayTimer();
signals:
    void started(QDateTime);
    void paused();
    void stopped(qint64);

    void setReplayDuration(qint64 duration);
    void replayBookmark(const QString &note, qint64 timestamp);
    void frameLoaded(const QJsonObject &frame);

    void replayFrameLoaded(qint64 timestampMs);
public slots:
    void start()  ;
    void pause()  ;
    void resume() ;
    void stop()   ;
    void restart();
    void fileLoaded();
    void fileUnloaded();
    void goToNextFrame();
    void goToPreviousFrame();
    void playAgain();
    void startReplayFromTimestamp(qint64 timestampMs);
    void bookmarkReplay(const QString &note, qint64 timestampMs);



private:
    Recorder* m_recorder;
    Hierarchy* m_hierarchy = nullptr;  // Used for extracting structure snapshot
    Simulation* m_simulation = nullptr;  // Used for getting simulation speed
    QDateTime m_startTime;
    qint64     m_duration;
    bool      m_active { false };

    bool isPaused = false;
    QVector<QJsonObject> playbackFrames;
    //int currentReplayIndex = 1;
    int currentReplayIndex = 0;
public:
    QJsonObject recordedStructure;
    /*├──►*/QJsonArray   rs_hierarchy;
    /*│---*/QJsonObject    rs_hierarchyObj;
    /*├──►*/QJsonArray   rs_bookmark ;
    /*│---*/QJsonObject    rs_bookmarkObj;
    /*└─┬►*/QJsonArray   rs_frame    ;
    /*  │---*/QJsonObject  rs_frameObj;
    /*  └─┬►*/QJsonArray     frameEntities;
    /*    ├───►*/ QJsonObject  entityObj;
    /*    ├───►*/ //QJsonObject CurrentSpeed;
    /*    └───►*/
    void loadFrameEntitiesData(const QJsonValue frame);
    void createTimer();
    void toggle();
signals:
    void updateScene(float deltaTime);
};
/* -------------------------------------------------------
 * Replay QObject End
 * ------------------------------------------------------*/





#endif // RECORDER_H
